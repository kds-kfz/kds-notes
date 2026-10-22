#include "ClientInstance.h"
#include "ClientProtocol.h"
#include "ClientHPSocket.h"
#include "HttpContentDecoder.h"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>

#include <openssl/x509v3.h>

namespace
{
    constexpr int SOCKET_CLIENT_TLS_HOSTNAME_MISMATCH = -20002;
}


// 按协议类型创建唯一底层组件，其他状态严格属于当前实例。
ClientInstance::ClientInstance(EN_SOCKET_CLIENT_TYPE p_enType, const char* p_szName,
    unsigned long long p_ullId)
    : m_enType(p_enType), m_bSecure(p_enType == EN_SOCKET_CLIENT_HTTPS || p_enType == EN_SOCKET_CLIENT_WSS),
      m_bTlsConfigured(!m_bSecure), m_bTlsVerifyPeer(false),
      m_strName(p_szName), m_ullId(p_ullId), m_clTcpListener(*this),
      m_clHttpListener(*this), m_pTcpClient(nullptr), m_pHttpClient(nullptr), m_pfnNotify(nullptr),
      m_pContext(nullptr), m_bStarted(false), m_bConnected(false), m_bUpgraded(false),
      m_bStopping(false), m_usStatus(0), m_bHttpRequestActive(false), m_usPort(0), m_byFrameOpcode(0),
      m_bFrameFinal(false), m_ullWireBodyBytes(0)
{
    if (m_enType == EN_SOCKET_CLIENT_TCP)
        m_pTcpClient = HP_Create_TcpClient(&m_clTcpListener);
    else
    {
        m_pHttpClient = m_bSecure ? HP_Create_HttpsClient(&m_clHttpListener) : HP_Create_HttpClient(&m_clHttpListener);
        m_pTcpClient = m_pHttpClient;
    }
}

// 先排空底层回调，再按实际组件类型准确销毁一次。
ClientInstance::~ClientInstance()
{
    Stop();
    if (m_pHttpClient && m_bSecure) HP_Destroy_HttpsClient(m_pHttpClient);
    else if (m_pHttpClient) HP_Destroy_HttpClient(m_pHttpClient);
    else if (m_pTcpClient) HP_Destroy_TcpClient(m_pTcpClient);
}

// 底层构造失败的实例不得进入工厂登记表。
bool ClientInstance::Ready() const { return m_pTcpClient != nullptr; }

// HPSocket/OpenSSL 不直接使用 Windows 证书存储，因此可信 CA 由宿主配置明确传入。
//
// 动态、静态构建在握手完成后都会校验证书中的 DNS/IP 身份；CA 链和主机名必须同时通过。
bool ClientInstance::ConfigureTls(const char* p_szCaCertificateFile, bool p_bVerifyPeer)
{
    std::lock_guard<std::recursive_mutex> lock(m_clControlMutex);
    if (!m_bSecure || m_pHttpClient == nullptr || m_bStarted || m_bStopping) return false;
    if (p_bVerifyPeer && (p_szCaCertificateFile == nullptr || *p_szCaCertificateFile == '\0'))
        return false;
    const int verifyMode = p_bVerifyPeer ? SSL_VM_PEER : SSL_VM_NONE;
    if (!p_bVerifyPeer)
    {
        m_bTlsVerifyPeer = false;
        m_bTlsConfigured = m_pHttpClient->SetupSSLContext(verifyMode) != FALSE;
        return m_bTlsConfigured;
    }

    // HPSocket 的多字节文件接口无法可靠打开中文路径，先用标准文件系统读取再以内存方式加载。
    std::ifstream input(std::filesystem::u8path(p_szCaCertificateFile), std::ios::binary);
    if (!input) return false;
    const std::string caPem((std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>());
    if (caPem.empty()) return false;
    m_bTlsConfigured = m_pHttpClient->SetupSSLContextByMemory(verifyMode, nullptr, nullptr, nullptr,
        caPem.c_str()) != FALSE;
    m_bTlsVerifyPeer = m_bTlsConfigured;
    return m_bTlsConfigured;
}

// TLS 实例在 TCP OnConnect 后还必须等待 OnHandShake，明文实例无需该阶段。
bool ClientInstance::IsSecure() const { return m_bSecure; }

// HP-Socket 已经完成 CA 链校验；这里再把 Connect 使用的目标主机绑定到证书 SAN/CN。
bool ClientInstance::CompleteSecureHandshake()
{
    if (!m_bSecure || !m_bTlsVerifyPeer) return true;
    LPVOID peerCertificate = nullptr;
    if (!m_pHttpClient || !m_pHttpClient->GetSSLSessionInfo(SSL_SSI_PEER_CERT, &peerCertificate) ||
        peerCertificate == nullptr)
        return false;

    X509* certificate = static_cast<X509*>(peerCertificate);
    int matched = X509_check_ip_asc(certificate, m_strHost.c_str(), 0);
    if (matched == -2)
    {
        matched = X509_check_host(certificate, m_strHost.c_str(), m_strHost.size(),
            X509_CHECK_FLAG_NO_PARTIAL_WILDCARDS, nullptr);
    }
    if (matched == 1) return true;

    ST_SOCKET_CLIENT_EVENT event = ClientProtocol::Event(EN_SOCKET_CLIENT_HTTP_ERROR);
    event.iErrorCode = SOCKET_CLIENT_TLS_HOSTNAME_MISMATCH;
    static const char description[] = "TLS peer certificate does not match the requested host";
    event.pData = description;
    event.iDataLength = static_cast<int>(sizeof(description) - 1);
    Notify(event);
    return false;
}

// 将回调、远端地址和协议状态绑定到本次异步连接。
bool ClientInstance::Connect(const char* p_szHost, unsigned short p_unPort,
    SOCKET_CLIENT_NOTIFY_PROC p_pNotify, void* p_pContext)
{
    if (!p_szHost || !*p_szHost || !p_unPort || !p_pNotify) return false;
    size_t length = 0;
    while (length < 256 && p_szHost[length]) ++length;
    if (!length || length >= 256) return false;
    std::lock_guard<std::recursive_mutex> lock(m_clControlMutex);
    if (!m_pTcpClient || m_bStarted || m_bStopping || (m_bSecure && !m_bTlsConfigured)) return false;
    m_strHost.assign(p_szHost, length);
    m_usPort = p_unPort;
    m_pContext.store(p_pContext);
    m_pfnNotify.store(p_pNotify);
    m_bConnected = false;
    m_bUpgraded = false;
    m_bHttpRequestActive = false;
    m_usStatus = 0;
    if (!m_pTcpClient->Start(p_szHost, p_unPort, TRUE))
    {
        // Start 失败不会触发 OnClose，主动把 HPSocket 启动错误交给上层诊断。
        ST_SOCKET_CLIENT_EVENT event = ClientProtocol::Event(EN_SOCKET_CLIENT_CLOSED);
        event.iErrorCode = static_cast<int>(m_pTcpClient->GetLastError());
        Notify(event);
        return false;
    }
    m_bStarted = true;
    return true;
}

// 标记停止后释放控制锁，避免底层等待回调时与业务重入形成锁反转。
void ClientInstance::Stop()
{
    {
        std::lock_guard<std::recursive_mutex> lock(m_clControlMutex);
        if (!m_pTcpClient || !m_bStarted || m_bStopping) return;
        m_bStopping = true;
    }
    m_pTcpClient->Stop();
    m_pTcpClient->Wait(INFINITE);
    std::lock_guard<std::recursive_mutex> lock(m_clControlMutex);
    m_bConnected = false;
    m_bUpgraded = false;
    m_bHttpRequestActive = false;
    m_bStarted = false;
    m_bStopping = false;
}

// 读取传输连接状态，不隐含 WebSocket 握手成功。
bool ClientInstance::IsConnected() const { return m_bConnected.load(); }

// TCP 只发送原始字节，业务层自行完成报文分帧。
bool ClientInstance::Send(const void* p_pData, int p_iLength)
{
    std::lock_guard<std::recursive_mutex> lock(m_clControlMutex);
    return m_enType == EN_SOCKET_CLIENT_TCP && m_bConnected && !m_bStopping &&
        p_pData && p_iLength > 0 &&
        m_pTcpClient->Send(static_cast<const BYTE*>(p_pData), p_iLength) != FALSE;
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
    std::lock_guard<std::recursive_mutex> lock(m_clControlMutex);
    if ((m_enType != EN_SOCKET_CLIENT_HTTP && m_enType != EN_SOCKET_CLIENT_HTTPS) ||
        !m_bConnected || m_bStopping) return false;

    // HP-Socket 支持流水线发送，但公开事件没有请求 ID；禁止覆盖尚未完成响应的解析状态。
    bool expected = false;
    if (!m_bHttpRequestActive.compare_exchange_strong(expected, true)) return false;
    {
        // 每个请求在发送前清空上一次响应状态，防止复用连接时编码信息串扰。
        std::lock_guard<std::mutex> bodyLock(m_clHttpBodyMutex);
        m_strContentEncoding.clear();
        m_vecEncodedBody.clear();
        m_ullWireBodyBytes = 0;
    }
    const bool sent = m_pHttpClient->SendRequest(p_szMethod, p_szPath,
        headers.empty() ? nullptr : headers.data(), p_iCount,
        static_cast<const BYTE*>(p_pBody), p_iLength) != FALSE;
    if (!sent) m_bHttpRequestActive = false;
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
    std::lock_guard<std::recursive_mutex> lock(m_clControlMutex);
    if ((m_enType != EN_SOCKET_CLIENT_WEB && m_enType != EN_SOCKET_CLIENT_WSS) ||
        !m_bConnected || m_bUpgraded || m_bStopping) return false;
    {
        std::lock_guard<std::mutex> frameLock(m_clFrameMutex);
        m_strExpectedAccept = accept;
        m_strReceivedAccept.clear();
    }
    m_usStatus = 0;
    return m_pHttpClient->SendRequest("GET", p_szPath, headers.data(), static_cast<int>(headers.size())) != FALSE;
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
    std::lock_guard<std::recursive_mutex> lock(m_clControlMutex);
    return (m_enType == EN_SOCKET_CLIENT_WEB || m_enType == EN_SOCKET_CLIENT_WSS) && m_bUpgraded && !m_bStopping &&
        m_pHttpClient->SendWSMessage(TRUE, 0, p_byOpcode, mask,
            static_cast<const BYTE*>(p_pData), p_iLength) != FALSE;
}

// 复制并初始化固定大小的工厂运行状态结构体。
void ClientInstance::Snapshot(ST_SOCKET_CLIENT_RUNTIME_INFO& p_refInfo) const
{
    std::lock_guard<std::recursive_mutex> lock(m_clControlMutex);
    p_refInfo = {};
    p_refInfo.uiStructSize = sizeof(p_refInfo);
    p_refInfo.uiAbiVersion = SOCKET_CLIENT_ABI_VERSION;
    p_refInfo.enType = m_enType;
    p_refInfo.ullInstanceId = m_ullId;
    p_refInfo.bStarted = m_bStarted ? 1 : 0;
    p_refInfo.bConnected = m_bConnected ? 1 : 0;
    p_refInfo.bSecure = m_bSecure ? 1 : 0;
    std::strncpy(p_refInfo.szName, m_strName.c_str(), sizeof(p_refInfo.szName) - 1);
    std::strncpy(p_refInfo.szRemoteHost, m_strHost.c_str(), sizeof(p_refInfo.szRemoteHost) - 1);
    p_refInfo.usRemotePort = m_usPort;
}

// 业务回调在无库锁状态执行，异常不允许进入 HP 事件分发栈。
void ClientInstance::Notify(const ST_SOCKET_CLIENT_EVENT& p_refEvent)
{
    SOCKET_CLIENT_NOTIFY_PROC callback = m_pfnNotify.load();
    if (callback)
    {
        try { callback(this, &p_refEvent, m_pContext.load()); }
        catch (...) { /* 隔离业务回调抛出的异常。 */ }
    }
}

// 发布传输层连接事件，WebSocket 仍须等待后续升级校验。
void ClientInstance::Connected()
{
    m_bConnected = true;
    Notify(ClientProtocol::Event(EN_SOCKET_CLIENT_CONNECTED));
}

// 在通知业务前先清理连接及升级状态。
void ClientInstance::Closed(int p_iError)
{
    m_bConnected = false;
    m_bUpgraded = false;
    m_bHttpRequestActive = false;
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
    m_usStatus = p_unStatus;
    ST_SOCKET_CLIENT_EVENT event = ClientProtocol::Event(EN_SOCKET_CLIENT_HTTP_STATUS);
    event.usStatus = p_unStatus;
    event.pData = p_szDescription;
    event.iDataLength = p_szDescription ? static_cast<int>(strlen(p_szDescription)) : 0;
    if (m_enType == EN_SOCKET_CLIENT_HTTP || m_enType == EN_SOCKET_CLIENT_HTTPS) Notify(event);
}

// WebSocket 应答字段单独保存；普通 HTTP 字段直接转发。
void ClientInstance::Header(const char* p_szName, const char* p_szValue)
{
    if ((m_enType == EN_SOCKET_CLIENT_WEB || m_enType == EN_SOCKET_CLIENT_WSS) &&
        ClientProtocol::HeaderEquals(p_szName, "Sec-WebSocket-Accept"))
    {
        std::lock_guard<std::mutex> lock(m_clFrameMutex);
        m_strReceivedAccept = p_szValue ? p_szValue : "";
    }
    if (m_enType == EN_SOCKET_CLIENT_HTTP || m_enType == EN_SOCKET_CLIENT_HTTPS)
    {
        if (ClientProtocol::HeaderEquals(p_szName, "Content-Encoding"))
        {
            std::lock_guard<std::mutex> lock(m_clHttpBodyMutex);
            m_strContentEncoding = p_szValue ? p_szValue : "";
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
        std::lock_guard<std::mutex> lock(m_clHttpBodyMutex);
        m_ullWireBodyBytes += static_cast<unsigned long long>(p_iLength);
        compressed = !m_strContentEncoding.empty() && m_strContentEncoding != "identity";
        if (compressed && p_iLength > 0)
        {
            if (static_cast<size_t>(p_iLength) >
                HttpContentDecoder::kMaxDecodedBodyBytes - m_vecEncodedBody.size()) return false;
            m_vecEncodedBody.insert(m_vecEncodedBody.end(), p_byData, p_byData + p_iLength);
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
        std::lock_guard<std::mutex> lock(m_clHttpBodyMutex);
        encoding = m_strContentEncoding;
        encoded.swap(m_vecEncodedBody);
        wireBytes = m_ullWireBodyBytes;
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
            m_bHttpRequestActive = false;
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
    m_bHttpRequestActive = false;
    ST_SOCKET_CLIENT_EVENT complete = ClientProtocol::Event(EN_SOCKET_CLIENT_HTTP_COMPLETE);
    complete.ullWireBodyBytes = wireBytes;
    Notify(complete);
}

// 解析器拒绝当前响应时先释放在途标记；底层是否关闭连接由 HP-Socket 决定。
void ClientInstance::HttpFailed()
{
    m_bHttpRequestActive = false;
}

// 仅服务端应答和挑战值均正确时开放 WebSocket 帧接口。
bool ClientInstance::WebOpened(EnHttpUpgradeType p_enUpgrade)
{
    {
        std::lock_guard<std::mutex> lock(m_clFrameMutex);
        if ((m_enType != EN_SOCKET_CLIENT_WEB && m_enType != EN_SOCKET_CLIENT_WSS) ||
            p_enUpgrade != HUT_WEB_SOCKET ||
            m_usStatus != 101 || m_strExpectedAccept.empty() ||
            m_strReceivedAccept != m_strExpectedAccept) return false;
    }
    m_bUpgraded = true;
    Notify(ClientProtocol::Event(EN_SOCKET_CLIENT_WEB_OPEN));
    return true;
}

// HTTP 解析器在 101 响应头结束时切换协议。
bool ClientInstance::ShouldUpgrade() const
{ return (m_enType == EN_SOCKET_CLIENT_WEB || m_enType == EN_SOCKET_CLIENT_WSS) && m_usStatus == 101; }

// 预先拒绝超过单帧上限的服务器数据。
bool ClientInstance::FrameHeader(BOOL p_bFinal, BYTE p_byOpcode, ULONGLONG p_ullLength)
{
    std::lock_guard<std::mutex> lock(m_clFrameMutex);
    if (!m_bUpgraded || p_ullLength > ClientProtocol::kMaxWebFrame) return false;
    m_vecFrame.clear();
    m_vecFrame.reserve(static_cast<size_t>(p_ullLength));
    m_byFrameOpcode = p_byOpcode;
    m_bFrameFinal = p_bFinal != FALSE;
    return true;
}

// 累积 WebSocket 帧片段，超过本实例上限则拒绝。
bool ClientInstance::FrameBody(const BYTE* p_byData, int p_iLength)
{
    std::lock_guard<std::mutex> lock(m_clFrameMutex);
    if (p_iLength < 0 || (p_iLength && !p_byData) ||
        static_cast<size_t>(p_iLength) > ClientProtocol::kMaxWebFrame - m_vecFrame.size())
        return false;
    if (p_iLength) m_vecFrame.insert(m_vecFrame.end(), p_byData, p_byData + p_iLength);
    return true;
}

// 帧缓存移到局部变量，业务回调可安全读取到回调结束。
void ClientInstance::FrameComplete()
{
    std::vector<BYTE> frame;
    BYTE opcode;
    bool final;
    {
        std::lock_guard<std::mutex> lock(m_clFrameMutex);
        frame.swap(m_vecFrame);
        opcode = m_byFrameOpcode;
        final = m_bFrameFinal;
    }
    ST_SOCKET_CLIENT_EVENT event = ClientProtocol::Event(EN_SOCKET_CLIENT_WEB_FRAME);
    event.pData = frame.empty() ? nullptr : frame.data();
    event.iDataLength = static_cast<int>(frame.size());
    event.byOpcode = opcode;
    event.bFinal = final ? 1 : 0;
    Notify(event);
}
