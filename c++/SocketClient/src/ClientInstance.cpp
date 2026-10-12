#include "ClientInstance.h"
#include "ClientProtocol.h"
#include "ClientHPSocket.h"
#include "HttpContentDecoder.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>


// 按协议类型创建唯一底层组件，其他状态严格属于当前实例。
ClientInstance::ClientInstance(EN_SOCKET_CLIENT_TYPE p_enType, const char* p_szName,
    unsigned long long p_ullId)
    : type_(p_enType), secure_(p_enType == EN_SOCKET_CLIENT_HTTPS || p_enType == EN_SOCKET_CLIENT_WSS),
      tlsConfigured_(!secure_),
      name_(p_szName), id_(p_ullId), tcpListener_(*this),
      httpListener_(*this), tcp_(nullptr), http_(nullptr), notify_(nullptr),
      context_(nullptr), started_(false), connected_(false), upgraded_(false),
      stopping_(false), status_(0), httpRequestActive_(false), port_(0), frameOpcode_(0),
      frameFinal_(false), wireBodyBytes_(0)
{
    if (type_ == EN_SOCKET_CLIENT_TCP)
        tcp_ = HP_Create_TcpClient(&tcpListener_);
    else
    {
        http_ = secure_ ? HP_Create_HttpsClient(&httpListener_) : HP_Create_HttpClient(&httpListener_);
        tcp_ = http_;
    }
}

// 先排空底层回调，再按实际组件类型准确销毁一次。
ClientInstance::~ClientInstance()
{
    Stop();
    if (http_ && secure_) HP_Destroy_HttpsClient(http_);
    else if (http_) HP_Destroy_HttpClient(http_);
    else if (tcp_) HP_Destroy_TcpClient(tcp_);
}

// 底层构造失败的实例不得进入工厂登记表。
bool ClientInstance::Ready() const { return tcp_ != nullptr; }

// HPSocket/OpenSSL 不直接使用 Windows 证书存储，因此可信 CA 由宿主配置明确传入。
//
// p_bVerifyPeer=true 只启用官方 HP-Socket 6.0.9 预编译库提供的 CA 证书链校验；该库在客户端握手时
// 设置 SNI，但没有调用 SSL_set1_host/X509_check_host 校验目标主机名。这里不能在封装层事后补做同等
// 安全的主机名认证，也不能把“链校验通过”描述为“站点身份严格校验”。需要严格主机名认证时，应由
// Nginx 等受控入口终止 TLS，或改用补齐主机名校验并重新构建的 HP-Socket，再完成独立安全回归。
bool ClientInstance::ConfigureTls(const char* p_szCaCertificateFile, bool p_bVerifyPeer)
{
    std::lock_guard<std::recursive_mutex> lock(control_);
    if (!secure_ || http_ == nullptr || started_ || stopping_) return false;
    if (p_bVerifyPeer && (p_szCaCertificateFile == nullptr || *p_szCaCertificateFile == '\0'))
        return false;
    const int verifyMode = p_bVerifyPeer ? SSL_VM_PEER : SSL_VM_NONE;
    if (!p_bVerifyPeer)
    {
        tlsConfigured_ = http_->SetupSSLContext(verifyMode) != FALSE;
        return tlsConfigured_;
    }

    // HPSocket 的多字节文件接口无法可靠打开中文路径，先用标准文件系统读取再以内存方式加载。
    std::ifstream input(std::filesystem::u8path(p_szCaCertificateFile), std::ios::binary);
    if (!input) return false;
    const std::string caPem((std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>());
    if (caPem.empty()) return false;
    tlsConfigured_ = http_->SetupSSLContextByMemory(verifyMode, nullptr, nullptr, nullptr,
        caPem.c_str()) != FALSE;
    return tlsConfigured_;
}

// TLS 实例在 TCP OnConnect 后还必须等待 OnHandShake，明文实例无需该阶段。
bool ClientInstance::IsSecure() const { return secure_; }

// 将回调、远端地址和协议状态绑定到本次异步连接。
bool ClientInstance::Connect(const char* p_szHost, unsigned short p_unPort,
    SOCKET_CLIENT_NOTIFY_PROC p_pNotify, void* p_pContext)
{
    if (!p_szHost || !*p_szHost || !p_unPort || !p_pNotify) return false;
    size_t length = 0;
    while (length < 256 && p_szHost[length]) ++length;
    if (!length || length >= 256) return false;
    std::lock_guard<std::recursive_mutex> lock(control_);
    if (!tcp_ || started_ || stopping_ || (secure_ && !tlsConfigured_)) return false;
    host_.assign(p_szHost, length);
    port_ = p_unPort;
    context_.store(p_pContext);
    notify_.store(p_pNotify);
    connected_ = false;
    upgraded_ = false;
    httpRequestActive_ = false;
    status_ = 0;
    if (!tcp_->Start(p_szHost, p_unPort, TRUE))
    {
        // Start 失败不会触发 OnClose，主动把 HPSocket 启动错误交给上层诊断。
        ST_SOCKET_CLIENT_EVENT event = ClientProtocol::Event(EN_SOCKET_CLIENT_CLOSED);
        event.iErrorCode = static_cast<int>(tcp_->GetLastError());
        Notify(event);
        return false;
    }
    started_ = true;
    return true;
}

// 标记停止后释放控制锁，避免底层等待回调时与业务重入形成锁反转。
void ClientInstance::Stop()
{
    {
        std::lock_guard<std::recursive_mutex> lock(control_);
        if (!tcp_ || !started_ || stopping_) return;
        stopping_ = true;
    }
    tcp_->Stop();
    tcp_->Wait(INFINITE);
    std::lock_guard<std::recursive_mutex> lock(control_);
    connected_ = false;
    upgraded_ = false;
    httpRequestActive_ = false;
    started_ = false;
    stopping_ = false;
}

// 读取传输连接状态，不隐含 WebSocket 握手成功。
bool ClientInstance::IsConnected() const { return connected_.load(); }

// TCP 只发送原始字节，业务层自行完成报文分帧。
bool ClientInstance::Send(const void* p_pData, int p_iLength)
{
    std::lock_guard<std::recursive_mutex> lock(control_);
    return type_ == EN_SOCKET_CLIENT_TCP && connected_ && !stopping_ &&
        p_pData && p_iLength > 0 &&
        tcp_->Send(static_cast<const BYTE*>(p_pData), p_iLength) != FALSE;
}

// 通过 HP-Socket HTTP 协议层发送请求，不允许对 Web 实例混用。
bool ClientInstance::Request(const char* p_szMethod, const char* p_szPath,
    const char* const* p_ppszNames, const char* const* p_ppszValues,
    int p_iCount, const void* p_pBody, int p_iLength)
{
    if (!p_szMethod || !*p_szMethod || !p_szPath || p_szPath[0] != '/' ||
        p_iCount < 0 || p_iLength < 0 || (p_iLength && !p_pBody) ||
        (p_iCount && (!p_ppszNames || !p_ppszValues))) return false;
    std::vector<THeader> headers;
    for (int i = 0; i < p_iCount; ++i)
    {
        if (!p_ppszNames[i] || !p_ppszValues[i]) return false;
        THeader header = {p_ppszNames[i], p_ppszValues[i]};
        headers.push_back(header);
    }
    std::lock_guard<std::recursive_mutex> lock(control_);
    if ((type_ != EN_SOCKET_CLIENT_HTTP && type_ != EN_SOCKET_CLIENT_HTTPS) ||
        !connected_ || stopping_) return false;

    // HP-Socket 支持流水线发送，但公开事件没有请求 ID；禁止覆盖尚未完成响应的解析状态。
    bool expected = false;
    if (!httpRequestActive_.compare_exchange_strong(expected, true)) return false;
    {
        // 每个请求在发送前清空上一次响应状态，防止复用连接时编码信息串扰。
        std::lock_guard<std::mutex> bodyLock(httpBodyMutex_);
        contentEncoding_.clear();
        encodedBody_.clear();
        wireBodyBytes_ = 0;
    }
    const bool sent = http_->SendRequest(p_szMethod, p_szPath,
        headers.empty() ? nullptr : headers.data(), p_iCount,
        static_cast<const BYTE*>(p_pBody), p_iLength) != FALSE;
    if (!sent) httpRequestActive_ = false;
    return sent;
}

// 生成随机挑战，以服务端工程的 Base64 算法编码并发送升级请求。
bool ClientInstance::Upgrade(const char* p_szPath, const char* p_szHost)
{
    return UpgradeWithHeaders(p_szPath, p_szHost, nullptr, nullptr, 0);
}

// 标准握手字段由库独占生成，调用方附加字段主要用于 Authorization 等鉴权信息。
bool ClientInstance::UpgradeWithHeaders(const char* p_szPath, const char* p_szHost,
    const char* const* p_ppszNames, const char* const* p_ppszValues, int p_iCount)
{
    if (!p_szPath || p_szPath[0] != '/' || !p_szHost || !*p_szHost || p_iCount < 0 ||
        (p_iCount > 0 && (!p_ppszNames || !p_ppszValues))) return false;
    unsigned char nonce[16];
    if (!ClientProtocol::RandomBytes(nonce, sizeof(nonce))) return false;
    const std::string key = ClientProtocol::EncodeBase64(nonce, sizeof(nonce));
    std::string accept;
    if (!ClientProtocol::WebAccept(key, accept)) return false;
    std::vector<THeader> headers = {
        {"Host", p_szHost}, {"Connection", "Upgrade"}, {"Upgrade", "websocket"},
        {"Sec-WebSocket-Version", "13"}, {"Sec-WebSocket-Key", key.c_str()}
    };
    for (int index = 0; index < p_iCount; ++index)
    {
        if (!p_ppszNames[index] || !p_ppszValues[index] ||
            ClientProtocol::HeaderEquals(p_ppszNames[index], "Host") ||
            ClientProtocol::HeaderEquals(p_ppszNames[index], "Connection") ||
            ClientProtocol::HeaderEquals(p_ppszNames[index], "Upgrade") ||
            ClientProtocol::HeaderEquals(p_ppszNames[index], "Sec-WebSocket-Version") ||
            ClientProtocol::HeaderEquals(p_ppszNames[index], "Sec-WebSocket-Key"))
            continue;
        headers.push_back({p_ppszNames[index], p_ppszValues[index]});
    }
    std::lock_guard<std::recursive_mutex> lock(control_);
    if ((type_ != EN_SOCKET_CLIENT_WEB && type_ != EN_SOCKET_CLIENT_WSS) ||
        !connected_ || upgraded_ || stopping_) return false;
    {
        std::lock_guard<std::mutex> frameLock(frameMutex_);
        expectedAccept_ = accept;
        receivedAccept_.clear();
    }
    status_ = 0;
    return http_->SendRequest("GET", p_szPath, headers.data(), static_cast<int>(headers.size())) != FALSE;
}

// RFC 6455 要求客户端每次发帧使用新随机掩码。
bool ClientInstance::SendFrame(unsigned char p_byOpcode, const void* p_pData, int p_iLength)
{
    if ((p_byOpcode != 1 && p_byOpcode != 2 && p_byOpcode != 8 &&
         p_byOpcode != 9 && p_byOpcode != 10) || p_iLength < 0 ||
        (p_iLength && !p_pData) ||
        ((p_byOpcode == 8 || p_byOpcode == 9 || p_byOpcode == 10) && p_iLength > 125))
        return false;
    unsigned char mask[4];
    if (!ClientProtocol::RandomBytes(mask, sizeof(mask))) return false;
    std::lock_guard<std::recursive_mutex> lock(control_);
    return (type_ == EN_SOCKET_CLIENT_WEB || type_ == EN_SOCKET_CLIENT_WSS) && upgraded_ && !stopping_ &&
        http_->SendWSMessage(TRUE, 0, p_byOpcode, mask,
            static_cast<const BYTE*>(p_pData), p_iLength) != FALSE;
}

// 复制并初始化固定大小的工厂运行状态结构体。
void ClientInstance::Snapshot(ST_SOCKET_CLIENT_RUNTIME_INFO& p_refInfo) const
{
    std::lock_guard<std::recursive_mutex> lock(control_);
    p_refInfo = {};
    p_refInfo.uiStructSize = sizeof(p_refInfo);
    p_refInfo.uiAbiVersion = SOCKET_CLIENT_ABI_VERSION;
    p_refInfo.enType = type_;
    p_refInfo.ullInstanceId = id_;
    p_refInfo.bStarted = started_ ? 1 : 0;
    p_refInfo.bConnected = connected_ ? 1 : 0;
    p_refInfo.bSecure = secure_ ? 1 : 0;
    std::strncpy(p_refInfo.szName, name_.c_str(), sizeof(p_refInfo.szName) - 1);
    std::strncpy(p_refInfo.szRemoteHost, host_.c_str(), sizeof(p_refInfo.szRemoteHost) - 1);
    p_refInfo.usRemotePort = port_;
}

// 业务回调在无库锁状态执行，异常不允许进入 HP 事件分发栈。
void ClientInstance::Notify(const ST_SOCKET_CLIENT_EVENT& p_refEvent)
{
    SOCKET_CLIENT_NOTIFY_PROC callback = notify_.load();
    if (callback)
    {
        try { callback(this, &p_refEvent, context_.load()); }
        catch (...) { /* 隔离业务回调抛出的异常。 */ }
    }
}

// 发布传输层连接事件，WebSocket 仍须等待后续升级校验。
void ClientInstance::Connected()
{
    connected_ = true;
    Notify(ClientProtocol::Event(EN_SOCKET_CLIENT_CONNECTED));
}

// 在通知业务前先清理连接及升级状态。
void ClientInstance::Closed(int p_iError)
{
    connected_ = false;
    upgraded_ = false;
    httpRequestActive_ = false;
    ST_SOCKET_CLIENT_EVENT event = ClientProtocol::Event(EN_SOCKET_CLIENT_CLOSED);
    event.iErrorCode = p_iError;
    Notify(event);
}

// 转发一段未分帧的 TCP 字节。
void ClientInstance::Data(const BYTE* p_byData, int p_iLength)
{
    ST_SOCKET_CLIENT_EVENT event = ClientProtocol::Event(EN_SOCKET_CLIENT_DATA);
    event.pData = p_byData;
    event.iDataLength = p_iLength;
    Notify(event);
}

// 保存状态码；HTTP 与 HTTPS 对上层暴露完全一致的响应事件。
void ClientInstance::Status(unsigned short p_unStatus, const char* p_szDescription)
{
    status_ = p_unStatus;
    ST_SOCKET_CLIENT_EVENT event = ClientProtocol::Event(EN_SOCKET_CLIENT_HTTP_STATUS);
    event.usStatus = p_unStatus;
    event.pData = p_szDescription;
    event.iDataLength = p_szDescription ? static_cast<int>(strlen(p_szDescription)) : 0;
    if (type_ == EN_SOCKET_CLIENT_HTTP || type_ == EN_SOCKET_CLIENT_HTTPS) Notify(event);
}

// WebSocket 应答字段单独保存；普通 HTTP 字段直接转发。
void ClientInstance::Header(const char* p_szName, const char* p_szValue)
{
    if ((type_ == EN_SOCKET_CLIENT_WEB || type_ == EN_SOCKET_CLIENT_WSS) &&
        ClientProtocol::HeaderEquals(p_szName, "Sec-WebSocket-Accept"))
    {
        std::lock_guard<std::mutex> lock(frameMutex_);
        receivedAccept_ = p_szValue ? p_szValue : "";
    }
    if (type_ == EN_SOCKET_CLIENT_HTTP || type_ == EN_SOCKET_CLIENT_HTTPS)
    {
        if (ClientProtocol::HeaderEquals(p_szName, "Content-Encoding"))
        {
            std::lock_guard<std::mutex> lock(httpBodyMutex_);
            contentEncoding_ = p_szValue ? p_szValue : "";
        }
        ST_SOCKET_CLIENT_EVENT event = ClientProtocol::Event(EN_SOCKET_CLIENT_HTTP_HEADER);
        event.pszName = p_szName;
        event.pszValue = p_szValue;
        Notify(event);
    }
}

// 未压缩正文保持流式交付；压缩正文必须先完整缓存，才能由 zlib 验证尾部和校验和。
bool ClientInstance::HttpBody(const BYTE* p_byData, int p_iLength)
{
    if (p_iLength < 0 || (p_iLength > 0 && p_byData == nullptr)) return false;
    bool compressed = false;
    {
        std::lock_guard<std::mutex> lock(httpBodyMutex_);
        wireBodyBytes_ += static_cast<unsigned long long>(p_iLength);
        compressed = !contentEncoding_.empty() && contentEncoding_ != "identity";
        if (compressed && p_iLength > 0)
        {
            if (static_cast<size_t>(p_iLength) >
                HttpContentDecoder::kMaxDecodedBodyBytes - encodedBody_.size()) return false;
            encodedBody_.insert(encodedBody_.end(), p_byData, p_byData + p_iLength);
        }
    }
    if (!compressed && p_iLength > 0)
    {
        ST_SOCKET_CLIENT_EVENT event = ClientProtocol::Event(EN_SOCKET_CLIENT_HTTP_BODY);
        event.pData = p_byData;
        event.iDataLength = p_iLength;
        Notify(event);
    }
    return true;
}

// 解码失败只发布 HTTP_ERROR，不再伪造完成事件，调用方可以明确区分协议失败和 JSON 失败。
void ClientInstance::HttpComplete()
{
    std::string encoding;
    std::vector<unsigned char> encoded;
    unsigned long long wireBytes = 0;
    {
        std::lock_guard<std::mutex> lock(httpBodyMutex_);
        encoding = contentEncoding_;
        encoded.swap(encodedBody_);
        wireBytes = wireBodyBytes_;
    }
    if (!encoding.empty() && encoding != "identity")
    {
        std::vector<unsigned char> decoded;
        std::string error;
        if (!HttpContentDecoder::Decode(encoding, encoded, decoded, error))
        {
            ST_SOCKET_CLIENT_EVENT event = ClientProtocol::Event(EN_SOCKET_CLIENT_HTTP_ERROR);
            event.iErrorCode = -20001;
            event.pData = error.data();
            event.iDataLength = static_cast<int>(error.size());
            httpRequestActive_ = false;
            Notify(event);
            return;
        }
        if (!decoded.empty())
        {
            ST_SOCKET_CLIENT_EVENT event = ClientProtocol::Event(EN_SOCKET_CLIENT_HTTP_BODY);
            event.pData = decoded.data();
            event.iDataLength = static_cast<int>(decoded.size());
            Notify(event);
        }
    }
    // 在通知完成前开放下一次请求，允许业务回调直接安排同连接的后续请求。
    httpRequestActive_ = false;
    ST_SOCKET_CLIENT_EVENT complete = ClientProtocol::Event(EN_SOCKET_CLIENT_HTTP_COMPLETE);
    complete.ullWireBodyBytes = wireBytes;
    Notify(complete);
}

// 解析器拒绝当前响应时先释放在途标记；底层是否关闭连接由 HP-Socket 决定。
void ClientInstance::HttpFailed()
{
    httpRequestActive_ = false;
}

// 仅服务端应答和挑战值均正确时开放 WebSocket 帧接口。
bool ClientInstance::WebOpened(EnHttpUpgradeType p_enUpgrade)
{
    {
        std::lock_guard<std::mutex> lock(frameMutex_);
        if ((type_ != EN_SOCKET_CLIENT_WEB && type_ != EN_SOCKET_CLIENT_WSS) ||
            p_enUpgrade != HUT_WEB_SOCKET ||
            status_ != 101 || expectedAccept_.empty() ||
            receivedAccept_ != expectedAccept_) return false;
    }
    upgraded_ = true;
    Notify(ClientProtocol::Event(EN_SOCKET_CLIENT_WEB_OPEN));
    return true;
}

// HTTP 解析器在 101 响应头结束时切换协议。
bool ClientInstance::ShouldUpgrade() const
{ return (type_ == EN_SOCKET_CLIENT_WEB || type_ == EN_SOCKET_CLIENT_WSS) && status_ == 101; }

// 预先拒绝超过单帧上限的服务器数据。
bool ClientInstance::FrameHeader(BOOL p_bFinal, BYTE p_byOpcode, ULONGLONG p_ullLength)
{
    std::lock_guard<std::mutex> lock(frameMutex_);
    if (!upgraded_ || p_ullLength > ClientProtocol::kMaxWebFrame) return false;
    frame_.clear();
    frame_.reserve(static_cast<size_t>(p_ullLength));
    frameOpcode_ = p_byOpcode;
    frameFinal_ = p_bFinal != FALSE;
    return true;
}

// 累积 WebSocket 帧片段，超过本实例上限则拒绝。
bool ClientInstance::FrameBody(const BYTE* p_byData, int p_iLength)
{
    std::lock_guard<std::mutex> lock(frameMutex_);
    if (p_iLength < 0 || (p_iLength && !p_byData) ||
        static_cast<size_t>(p_iLength) > ClientProtocol::kMaxWebFrame - frame_.size())
        return false;
    if (p_iLength) frame_.insert(frame_.end(), p_byData, p_byData + p_iLength);
    return true;
}

// 帧缓存移到局部变量，业务回调可安全读取到回调结束。
void ClientInstance::FrameComplete()
{
    std::vector<BYTE> frame;
    BYTE opcode;
    bool final;
    {
        std::lock_guard<std::mutex> lock(frameMutex_);
        frame.swap(frame_);
        opcode = frameOpcode_;
        final = frameFinal_;
    }
    ST_SOCKET_CLIENT_EVENT event = ClientProtocol::Event(EN_SOCKET_CLIENT_WEB_FRAME);
    event.pData = frame.empty() ? nullptr : frame.data();
    event.iDataLength = static_cast<int>(frame.size());
    event.byOpcode = opcode;
    event.bFinal = final ? 1 : 0;
    Notify(event);
}
