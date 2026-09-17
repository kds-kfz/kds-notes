#ifndef SOCKET_CLIENT_H
#define SOCKET_CLIENT_H

// 公共虚函数布局或结构体变化时递增 ABI 版本。
#define SOCKET_CLIENT_ABI_VERSION 1U

#if defined(_WIN32)
#if defined(SOCKETCLIENT_EXPORTS)
#define SOCKETCLIENT_API __declspec(dllexport)
#else
#define SOCKETCLIENT_API __declspec(dllimport)
#endif
#else
#define SOCKETCLIENT_API __attribute__((visibility("default")))
#endif

// 客户端协议类型；同一类型内名称唯一，每个实例拥有一条连接。
enum EN_SOCKET_CLIENT_TYPE
{
    EN_SOCKET_CLIENT_TCP = 1,  // 原始 TCP 字节流。
    EN_SOCKET_CLIENT_HTTP = 2, // HTTP/1.1 请求与响应连接。
    EN_SOCKET_CLIENT_WEB = 3   // 通过 HTTP 升级建立的 WebSocket 连接。
};

// 回调事件类型；数据在回调期间借用，返回前应复制所需内容。
enum EN_SOCKET_CLIENT_EVENT
{
    EN_SOCKET_CLIENT_CONNECTED = 1, // TCP 传输连接建立。
    EN_SOCKET_CLIENT_CLOSED = 2,    // 传输连接关闭，错误码可为零。
    EN_SOCKET_CLIENT_DATA = 3,      // 一段 TCP 字节，不保证是完整业务报文。
    EN_SOCKET_CLIENT_HTTP_STATUS = 4, // HTTP 响应状态码及说明。
    EN_SOCKET_CLIENT_HTTP_HEADER = 5, // HTTP 响应头名称和值。
    EN_SOCKET_CLIENT_HTTP_BODY = 6,   // 一段 HTTP 响应正文。
    EN_SOCKET_CLIENT_HTTP_COMPLETE = 7, // HTTP 响应接收完成。
    EN_SOCKET_CLIENT_HTTP_ERROR = 8, // HTTP 解析错误。
    EN_SOCKET_CLIENT_WEB_OPEN = 9,   // WebSocket 握手校验成功。
    EN_SOCKET_CLIENT_WEB_FRAME = 10  // 完整 WebSocket 帧，包含控制帧。
};

// 回调事件数据；指针均为借用，仅在当前回调期间有效。
struct ST_SOCKET_CLIENT_EVENT
{
    EN_SOCKET_CLIENT_EVENT enEvent; // 事件类型。
    const void* pData;              // TCP、正文、帧内容或 HTTP 状态说明。
    int iDataLength;                // 数据字节数；无数据时为零。
    const char* pszName;            // HTTP_HEADER 的头名称，其他事件为空。
    const char* pszValue;           // HTTP_HEADER 的头值，其他事件为空。
    unsigned short usStatus;        // HTTP_STATUS 的状态码，其他事件为零。
    unsigned char byOpcode;         // WEB_FRAME 的操作码，其他事件为零。
    int bFinal;                     // WEB_FRAME 的 FIN 标志，其他事件为零。
    int iErrorCode;                 // CLOSED 或 HTTP_ERROR 的底层错误码。
};

// 业务回调类型；上下文对象由调用方持有，客户端实例由工厂持有。
class CSocketClient;
typedef void (*SOCKET_CLIENT_NOTIFY_PROC)(CSocketClient* p_pClient,
    const ST_SOCKET_CLIENT_EVENT* p_pEvent, void* p_pContext);

// 定长诊断快照；查询前必须先设置 uiStructSize。
struct ST_SOCKET_CLIENT_RUNTIME_INFO
{
    unsigned int uiStructSize;      // 必须等于本结构体的 sizeof 值。
    unsigned int uiAbiVersion;      // 当前动态库 ABI 版本。
    EN_SOCKET_CLIENT_TYPE enType;   // 工厂创建时指定的协议类型。
    unsigned long long ullInstanceId; // 进程内单调递增的诊断编号。
    int bStarted;                   // HP-Socket 组件是否已启动。
    int bConnected;                 // 传输连接当前是否有效。
    char szName[64];                // 工厂逻辑名称，以零结束。
    char szRemoteHost[256];         // 最近一次请求连接的远端地址，以零结束。
    unsigned short usRemotePort;    // 最近一次请求连接的远端端口。
    unsigned short usReserved;      // 保留字段，始终为零。
};

// 一个命名客户端实例仅对应一条连接；TCP、HTTP、Web 各使用自己的协议方法。
// 不得从该实例自身回调调用 Stop 或删除函数，应交由其他线程执行。
class CSocketClient
{
public:
    // 虚析构函数允许工厂从基类指针安全销毁实例。
    virtual ~CSocketClient() {}

    // 异步建立连接；回调及上下文至少应保持有效直到 Stop 返回。
    virtual bool Connect(const char* p_szHost, unsigned short p_unPort,
        SOCKET_CLIENT_NOTIFY_PROC p_pNotify, void* p_pContext) = 0;

    // 停止连接并等待回调退出；之后可再次 Connect 重用实例。
    virtual void Stop() = 0;

    // 查询传输连接状态；不代表 WebSocket 升级已完成。
    virtual bool IsConnected() const = 0;

    // 发送原始 TCP 字节；HTTP/Web 类型或未连接时返回 false。
    virtual bool Send(const void* p_pData, int p_iLength) = 0;

    // 在 HTTP 连接上发送请求；名称和值数组均有 iHeaderCount 项。
    // HP-Socket 在调用期间读取请求头；调用方仍拥有原始缓冲区。
    virtual bool Request(const char* p_szMethod, const char* p_szPath,
        const char* const* p_ppszHeaderNames, const char* const* p_ppszHeaderValues,
        int p_iHeaderCount, const void* p_pBody, int p_iBodyLength) = 0;

    // 在 Web 实例上发送标准升级请求；p_szHost 用作 HTTP Host 头。
    // 非默认端口需要在 p_szHost 中显式带上端口号。
    virtual bool Upgrade(const char* p_szPath, const char* p_szHost) = 0;

    // 使用新随机掩码发送完整 WebSocket 帧；操作码 1/2/8/9/10
    // 分别表示文本、二进制、关闭、Ping、Pong。
    virtual bool SendFrame(unsigned char p_byOpcode, const void* p_pData, int p_iLength) = 0;
};

extern "C"
{
    // 返回当前客户端动态库的 ABI 版本。
    SOCKETCLIENT_API unsigned int GetSocketClientAbiVersion();

    // 复制有效工厂实例的诊断信息；拒绝未知或失效指针。
    SOCKETCLIENT_API bool GetSocketClientRuntimeInfo(CSocketClient* p_pClient,
        ST_SOCKET_CLIENT_RUNTIME_INFO* p_pInfo);

    // 创建独立的命名连接；同类型同名称重复创建会失败。
    SOCKETCLIENT_API CSocketClient* CreateTcpClientInstanceByName(const char* p_szName);
    SOCKETCLIENT_API CSocketClient* CreateHttpClientInstanceByName(const char* p_szName);
    SOCKETCLIENT_API CSocketClient* CreateWebClientInstanceByName(const char* p_szName);

    // 停止并销毁相应类型的实例，再将指针置空。
    // 调用方需确保删除与该实例其他公开操作不会并发。
    SOCKETCLIENT_API void DelTcpClientInstance(CSocketClient*& p_pClient);
    SOCKETCLIENT_API void DelHttpClientInstance(CSocketClient*& p_pClient);
    SOCKETCLIENT_API void DelWebClientInstance(CSocketClient*& p_pClient);
}

#endif
