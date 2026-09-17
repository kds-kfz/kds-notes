#include "ClientInstance.h"
#include "ClientProtocol.h"
#include "ClientHPSocket.h"
#include <cstring>


// 按协议类型创建唯一底层组件，其他状态严格属于当前实例。
ClientInstance::ClientInstance(EN_SOCKET_CLIENT_TYPE p_enType, const char* p_szName,
    unsigned long long p_ullId)
    : type_(p_enType), name_(p_szName), id_(p_ullId), tcpListener_(*this),
      httpListener_(*this), tcp_(nullptr), http_(nullptr), notify_(nullptr),
      context_(nullptr), started_(false), connected_(false), upgraded_(false),
      stopping_(false), status_(0), port_(0), frameOpcode_(0), frameFinal_(false)
{
    if (type_ == EN_SOCKET_CLIENT_TCP)
        tcp_ = HP_Create_TcpClient(&tcpListener_);
    else
    {
        http_ = HP_Create_HttpClient(&httpListener_);
        tcp_ = http_;
    }
}

// 先排空底层回调，再按实际组件类型准确销毁一次。
ClientInstance::~ClientInstance()
{
    Stop();
    if (http_) HP_Destroy_HttpClient(http_);
    else if (tcp_) HP_Destroy_TcpClient(tcp_);
}

// 底层构造失败的实例不得进入工厂登记表。
bool ClientInstance::Ready() const { return tcp_ != nullptr; }

// 将回调、远端地址和协议状态绑定到本次异步连接。
bool ClientInstance::Connect(const char* p_szHost, unsigned short p_unPort,
    SOCKET_CLIENT_NOTIFY_PROC p_pNotify, void* p_pContext)
{
    if (!p_szHost || !*p_szHost || !p_unPort || !p_pNotify) return false;
    size_t length = 0;
    while (length < 256 && p_szHost[length]) ++length;
    if (!length || length >= 256) return false;
    std::lock_guard<std::recursive_mutex> lock(control_);
    if (!tcp_ || started_ || stopping_) return false;
    host_.assign(p_szHost, length);
    port_ = p_unPort;
    context_.store(p_pContext);
    notify_.store(p_pNotify);
    connected_ = false;
    upgraded_ = false;
    status_ = 0;
    if (!tcp_->Start(p_szHost, p_unPort, TRUE)) return false;
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
    return type_ == EN_SOCKET_CLIENT_HTTP && connected_ && !stopping_ &&
        http_->SendRequest(p_szMethod, p_szPath,
            headers.empty() ? nullptr : headers.data(), p_iCount,
            static_cast<const BYTE*>(p_pBody), p_iLength) != FALSE;
}

// 生成随机挑战，以服务端工程的 Base64 算法编码并发送升级请求。
bool ClientInstance::Upgrade(const char* p_szPath, const char* p_szHost)
{
    if (!p_szPath || p_szPath[0] != '/' || !p_szHost || !*p_szHost) return false;
    unsigned char nonce[16];
    if (!ClientProtocol::RandomBytes(nonce, sizeof(nonce))) return false;
    const std::string key = ClientProtocol::EncodeBase64(nonce, sizeof(nonce));
    std::string accept;
    if (!ClientProtocol::WebAccept(key, accept)) return false;
    const THeader headers[] = {
        {"Host", p_szHost}, {"Connection", "Upgrade"}, {"Upgrade", "websocket"},
        {"Sec-WebSocket-Version", "13"}, {"Sec-WebSocket-Key", key.c_str()}
    };
    std::lock_guard<std::recursive_mutex> lock(control_);
    if (type_ != EN_SOCKET_CLIENT_WEB || !connected_ || upgraded_ || stopping_) return false;
    {
        std::lock_guard<std::mutex> frameLock(frameMutex_);
        expectedAccept_ = accept;
        receivedAccept_.clear();
    }
    status_ = 0;
    return http_->SendRequest("GET", p_szPath, headers, 5) != FALSE;
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
    return type_ == EN_SOCKET_CLIENT_WEB && upgraded_ && !stopping_ &&
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

// 保存状态码；仅 HTTP 请求暴露普通响应事件。
void ClientInstance::Status(unsigned short p_unStatus, const char* p_szDescription)
{
    status_ = p_unStatus;
    ST_SOCKET_CLIENT_EVENT event = ClientProtocol::Event(EN_SOCKET_CLIENT_HTTP_STATUS);
    event.usStatus = p_unStatus;
    event.pData = p_szDescription;
    event.iDataLength = p_szDescription ? static_cast<int>(strlen(p_szDescription)) : 0;
    if (type_ == EN_SOCKET_CLIENT_HTTP) Notify(event);
}

// WebSocket 应答字段单独保存；普通 HTTP 字段直接转发。
void ClientInstance::Header(const char* p_szName, const char* p_szValue)
{
    if (type_ == EN_SOCKET_CLIENT_WEB &&
        ClientProtocol::HeaderEquals(p_szName, "Sec-WebSocket-Accept"))
    {
        std::lock_guard<std::mutex> lock(frameMutex_);
        receivedAccept_ = p_szValue ? p_szValue : "";
    }
    if (type_ == EN_SOCKET_CLIENT_HTTP)
    {
        ST_SOCKET_CLIENT_EVENT event = ClientProtocol::Event(EN_SOCKET_CLIENT_HTTP_HEADER);
        event.pszName = p_szName;
        event.pszValue = p_szValue;
        Notify(event);
    }
}

// 仅服务端应答和挑战值均正确时开放 WebSocket 帧接口。
bool ClientInstance::WebOpened(EnHttpUpgradeType p_enUpgrade)
{
    {
        std::lock_guard<std::mutex> lock(frameMutex_);
        if (type_ != EN_SOCKET_CLIENT_WEB || p_enUpgrade != HUT_WEB_SOCKET ||
            status_ != 101 || expectedAccept_.empty() ||
            receivedAccept_ != expectedAccept_) return false;
    }
    upgraded_ = true;
    Notify(ClientProtocol::Event(EN_SOCKET_CLIENT_WEB_OPEN));
    return true;
}

// HTTP 解析器在 101 响应头结束时切换协议。
bool ClientInstance::ShouldUpgrade() const
{ return type_ == EN_SOCKET_CLIENT_WEB && status_ == 101; }

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
