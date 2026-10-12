#ifndef SOCKET_CLIENT_INSTANCE_H
#define SOCKET_CLIENT_INSTANCE_H

#include "SocketClient.h"
#include "TcpClientListener.h"
#include "HttpClientListener.h"
#include <atomic>
#include <mutex>
#include <string>
#include <vector>

// 每个工厂实例拥有一个 HP-Socket 组件、监听器、连接状态及回调上下文。
class ClientInstance : public CSocketClient
{
public:
    // 构造指定协议和名称的客户端，底层组件由当前实例独占。
    ClientInstance(EN_SOCKET_CLIENT_TYPE p_enType, const char* p_szName,
        unsigned long long p_ullId);

    // 停止连接并等待本实例回调退出，再释放 HP 组件。
    ~ClientInstance() override;

    // 校验底层 HP 组件是否成功创建。
    bool Ready() const;

    // 在连接开始前加载可信 CA 链并初始化 TLS 上下文。
    bool ConfigureTls(const char* p_szCaCertificateFile, bool p_bVerifyPeer);

    // 判断当前实例是否要求先完成 TLS 握手。
    bool IsSecure() const;

    // 异步连接远端；已经运行时应先调用 Stop。
    bool Connect(const char* p_szHost, unsigned short p_unPort,
        SOCKET_CLIENT_NOTIFY_PROC p_pNotify, void* p_pContext) override;

    // 停止连接并等待底层全部回调退出。
    void Stop() override;

    // 获取最近一次传输层回调报告的连接状态。
    bool IsConnected() const override;

    // 仅 TCP 实例可发送原始字节。
    bool Send(const void* p_pData, int p_iLength) override;

    // 仅 HTTP 实例可发送带请求头和正文的请求。
    bool Request(const char* p_szMethod, const char* p_szPath,
        const char* const* p_ppszNames, const char* const* p_ppszValues,
        int p_iCount, const void* p_pBody, int p_iLength) override;

    // 仅 WebSocket 实例可发起 HTTP 升级握手。
    bool Upgrade(const char* p_szPath, const char* p_szHost) override;

    // 在自动生成的标准握手头之外附加鉴权等业务请求头。
    bool UpgradeWithHeaders(const char* p_szPath, const char* p_szHost,
        const char* const* p_ppszNames, const char* const* p_ppszValues,
        int p_iCount) override;

    // 仅握手成功的 WebSocket 实例可发送带掩码的完整帧。
    bool SendFrame(unsigned char p_byOpcode, const void* p_pData, int p_iLength) override;

    // 将当前实例的状态复制到工厂验证过的诊断结构体。
    void Snapshot(ST_SOCKET_CLIENT_RUNTIME_INFO& p_refInfo) const;

    // 调用业务回调，捕获并隔离穿过底层回调边界的异常。
    void Notify(const ST_SOCKET_CLIENT_EVENT& p_refEvent);

    // 底层建立传输连接时更新状态并通知业务层。
    void Connected();

    // 底层关闭连接时使协议状态失效并通知业务层。
    void Closed(int p_iError);

    // 转发一段未分帧的 TCP 接收字节。
    void Data(const BYTE* p_byData, int p_iLength);

    // 保存 HTTP 状态码，并对 HTTP 类型通知业务层。
    void Status(unsigned short p_unStatus, const char* p_szDescription);

    // 交付 HTTP 头，或保存 WebSocket 握手应答头。
    void Header(const char* p_szName, const char* p_szValue);

    // 累计线上 HTTP 正文字节；压缩响应延迟到完整接收后统一解码。
    bool HttpBody(const BYTE* p_byData, int p_iLength);

    // 解码并交付完整 HTTP 正文，同时在完成事件中保留线上字节口径。
    void HttpComplete();

    // HTTP 解析失败时释放当前请求门禁，使 Keep-Alive 连接可由调用方决定是否继续复用。
    void HttpFailed();

    // 校验 101 状态和挑战应答，再进入 WebSocket 模式。
    bool WebOpened(EnHttpUpgradeType p_enUpgrade);

    // 判断 HP 解析器当前是否应切换到 WebSocket 模式。
    bool ShouldUpgrade() const;

    // 限制单帧长度，并初始化当前 WebSocket 帧接收状态。
    bool FrameHeader(BOOL p_bFinal, BYTE p_byOpcode, ULONGLONG p_ullLength);

    // 将当前帧的接收片段追加到本实例的有界缓存。
    bool FrameBody(const BYTE* p_byData, int p_iLength);

    // 将完整 WebSocket 帧送入业务回调。
    void FrameComplete();

private:
    EN_SOCKET_CLIENT_TYPE type_; // 工厂确定的协议类型。
    bool secure_; // HTTPS/WSS 使用 HPSocket TLS 组件；校验开启时验证 CA 链，但官方预编译库不验证主机名。
    bool tlsConfigured_; // TLS 上下文是否已按调用方策略初始化。
    std::string name_; // 创建时确定的逻辑名称。
    unsigned long long id_; // 不复用的诊断实例编号。
    TcpClientListener tcpListener_; // TCP 专用监听器。
    HttpClientListener httpListener_; // HTTP/WebSocket 专用监听器。
    ITcpClient* tcp_; // 自有 HP 组件，HTTP/Web 时与 http_ 别名。
    IHttpClient* http_; // TCP 类型为空；仅在 HTTP/Web 时销毁。
    mutable std::recursive_mutex control_; // 串行化公开组件操作。
    std::atomic<SOCKET_CLIENT_NOTIFY_PROC> notify_; // 业务回调函数。
    std::atomic<void*> context_; // 业务回调上下文。
    std::atomic<bool> started_; // Start 成功且 Stop 尚未完成。
    std::atomic<bool> connected_; // 传输连接有效状态。
    std::atomic<bool> upgraded_; // WebSocket 握手成功状态。
    bool stopping_; // 由控制锁保护的停机标志。
    std::atomic<unsigned short> status_; // 最近一次 HTTP 状态码。
    std::atomic<bool> httpRequestActive_; // HTTP/HTTPS 每条连接只允许一个尚未完成的响应。
    std::string host_; // 最近一次请求连接的地址。
    unsigned short port_; // 最近一次请求连接的端口。
    std::mutex frameMutex_; // 保护 WebSocket 握手及帧缓冲区。
    std::string expectedAccept_; // 预期的服务端握手应答。
    std::string receivedAccept_; // 实际收到的握手应答。
    std::vector<BYTE> frame_; // 当前 WebSocket 帧的数据缓存。
    BYTE frameOpcode_; // 当前帧的操作码。
    bool frameFinal_; // 当前帧的 FIN 标志。
    std::mutex httpBodyMutex_; // 保护单个 HTTP 响应的编码名称、缓存和线上字节数。
    std::string contentEncoding_; // 服务端 Content-Encoding；空值表示未压缩正文。
    std::vector<BYTE> encodedBody_; // 压缩响应在完成前的原始字节缓存。
    unsigned long long wireBodyBytes_; // 当前 HTTP 响应在线路上传输的正文累计字节数。
};

#endif
