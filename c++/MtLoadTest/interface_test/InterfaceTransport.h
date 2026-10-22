#pragma once

// SocketClient 传输适配层。每次 HTTP 调用独占一个客户端实例；WebSocket
// 连接由对象持有，并在 Disconnect/析构时停止心跳、等待回调并确定性释放。

#include "InterfaceTestModel.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

class CSocketClient;
struct ST_SOCKET_CLIENT_EVENT;

namespace interface_test
{
struct HttpRequest
{
    ServerProfile server;
    std::string method;
    std::string path;
    std::vector<std::pair<std::string, std::string>> headers;
    std::string body;
};

struct HttpResponse
{
    bool transportSucceeded = false;
    int statusCode = 0;
    std::string responseHeaders;
    std::string responseBody;
    double elapsedMs = 0.0;
    uint64_t requestBytes = 0;
    uint64_t responseBytes = 0;
    std::string error;
};

struct WebMessage
{
    std::string time;
    std::string direction;
    std::string funcId;
    std::string topic;
    std::string eventType;
    uint64_t bytes = 0;
    std::string summary;
    std::string json;
};

class SocketHttpTransport
{
public:
    SocketHttpTransport() = default;
    ~SocketHttpTransport();
    SocketHttpTransport(const SocketHttpTransport&) = delete;
    SocketHttpTransport& operator=(const SocketHttpTransport&) = delete;
    HttpResponse Execute(const HttpRequest& request);
    void Cancel();

private:
    std::mutex m_clExecutionMutex;
    std::mutex m_clActiveMutex;
    CSocketClient* m_pActiveClient = nullptr;
    std::atomic<bool> m_bCancelRequested{false};
};

class SocketWebTransport
{
public:
    using MessageCallback = std::function<void(const WebMessage&)>;
    using StateCallback = std::function<void(bool, const std::string&)>;
    SocketWebTransport() = default;
    ~SocketWebTransport();
    SocketWebTransport(const SocketWebTransport&) = delete;
    SocketWebTransport& operator=(const SocketWebTransport&) = delete;
    bool Connect(const ServerProfile& server,
        const std::vector<std::pair<std::string, std::string>>& headers,
        MessageCallback messageCallback, StateCallback stateCallback, std::string& error);
    bool Send(const std::string& json, std::string& error);
    void SetHeartbeat(const std::string& json, uint32_t intervalMs);
    void Disconnect();
    bool IsConnected() const;

private:
    static void OnSocketEvent(CSocketClient* client,
        const ST_SOCKET_CLIENT_EVENT* event, void* context);
    void HandleSocketEvent(CSocketClient* source, const ST_SOCKET_CLIENT_EVENT& event);
    void HeartbeatLoop();

    // 客户端公开调用与销毁串行化；SocketClient 回调只使用下面的状态锁，
    // Stop 等待回调退出时不会形成反向等待。
    mutable std::mutex m_clClientCallMutex;
    mutable std::mutex m_clMutex;
    std::condition_variable m_clChanged;
    CSocketClient* m_pClient = nullptr;
    bool m_bSecure = false;
    bool m_bTransportConnected = false;
    bool m_bWebOpened = false;
    bool m_bClosed = false;
    int m_iErrorCode = 0;
    std::atomic<bool> m_bStopping{false};
    std::atomic<bool> m_bHeartbeatStop{false};
    MessageCallback m_clMessageCallback;
    StateCallback m_clStateCallback;
    std::string m_strHeartbeatJson;
    uint32_t m_u32HeartbeatIntervalMs = 0;
    std::thread m_clHeartbeatThread;
};
} // namespace interface_test
