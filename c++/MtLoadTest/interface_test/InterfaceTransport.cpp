#include "pch.h"
#include "InterfaceTransport.h"

#include <SocketClient.h>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <map>
#include <sstream>

namespace interface_test
{
namespace
{
struct HttpCallbackState
{
    std::mutex mutex;
    std::condition_variable changed;
    bool connected = false;
    bool closed = false;
    bool complete = false;
    bool tooLarge = false;
    int status = 0;
    int errorCode = 0;
    uint32_t maxResponseBytes = 0;
    uint64_t wireBodyBytes = 0;
    std::string headers;
    std::string body;
};

void OnHttpEvent(CSocketClient*, const ST_SOCKET_CLIENT_EVENT* event, void* context)
{
    if (event == nullptr || context == nullptr) return;
    auto& state = *static_cast<HttpCallbackState*>(context);
    std::lock_guard<std::mutex> guard(state.mutex);
    switch (event->enEvent)
    {
    case EN_SOCKET_CLIENT_CONNECTED: state.connected = true; break;
    case EN_SOCKET_CLIENT_CLOSED:
        state.closed = true; state.errorCode = event->iErrorCode; break;
    case EN_SOCKET_CLIENT_HTTP_STATUS: state.status = event->usStatus; break;
    case EN_SOCKET_CLIENT_HTTP_HEADER:
        if (event->pszName != nullptr && event->pszValue != nullptr)
            state.headers += std::string(event->pszName) + ": " + event->pszValue + "\r\n";
        break;
    case EN_SOCKET_CLIENT_HTTP_BODY:
        if (event->pData != nullptr && event->iDataLength > 0)
        {
            const size_t bytes = static_cast<size_t>(event->iDataLength);
            if (bytes > state.maxResponseBytes || state.body.size() > state.maxResponseBytes - bytes)
                state.tooLarge = true;
            else state.body.append(static_cast<const char*>(event->pData), bytes);
        }
        break;
    case EN_SOCKET_CLIENT_HTTP_COMPLETE:
        state.wireBodyBytes = event->ullWireBodyBytes; state.complete = true; break;
    case EN_SOCKET_CLIENT_HTTP_ERROR:
        state.closed = true; state.errorCode = event->iErrorCode; break;
    default: break;
    }
    state.changed.notify_all();
}

std::string NextClientName(const char* prefix)
{
    static std::atomic<unsigned long long> sequence{1};
    return std::string(prefix) + std::to_string(sequence.fetch_add(1));
}

template<typename Predicate>
bool WaitFor(std::condition_variable& changed, std::unique_lock<std::mutex>& lock,
    uint32_t timeoutMs, const std::atomic<bool>& cancelled, Predicate predicate)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (!predicate() && !cancelled.load())
    {
        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) return false;
        changed.wait_until(lock, (std::min)(deadline, now + std::chrono::milliseconds(50)));
    }
    return predicate();
}

std::string RequestPath(const ServerProfile& server, const std::string& path)
{
    std::string result = server.basePath;
    if (result.empty()) result = "/";
    if (!path.empty())
    {
        if (result.back() == '/' && path.front() == '/') result += path.substr(1);
        else if (result.back() != '/' && path.front() != '/') result += "/" + path;
        else result += path;
    }
    if (result.empty() || result.front() != '/') result.insert(result.begin(), '/');
    return result;
}

bool HeaderEquals(const std::string& left, const char* right)
{
    return _stricmp(left.c_str(), right) == 0;
}

std::string TimeText()
{
    const std::time_t now = std::time(nullptr);
    std::tm local{};
    localtime_s(&local, &now);
    std::ostringstream stream;
    stream << std::put_time(&local, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}

std::string JsonColumn(const nlohmann::json& value, const char* name)
{
    const auto iterator = value.find(name);
    if (iterator == value.end()) return {};
    return iterator->is_string() ? iterator->get<std::string>() : iterator->dump();
}

WebMessage MakeWebMessage(const std::string& direction, const std::string& json)
{
    WebMessage message;
    message.time = TimeText(); message.direction = direction; message.bytes = json.size(); message.json = json;
    try
    {
        const auto value = nlohmann::json::parse(json);
        message.funcId = JsonColumn(value, "FuncId");
        message.topic = JsonColumn(value, "Topic");
        message.eventType = JsonColumn(value, "EventType");
        message.summary = value.dump();
    }
    catch (...)
    {
        message.summary = json;
    }
    if (message.summary.size() > 200) message.summary.resize(200), message.summary += "...";
    return message;
}
} // namespace

SocketHttpTransport::~SocketHttpTransport()
{
    Cancel();
    std::lock_guard<std::mutex> execution(m_clExecutionMutex);
}

HttpResponse SocketHttpTransport::Execute(const HttpRequest& request)
{
    std::lock_guard<std::mutex> execution(m_clExecutionMutex);
    HttpResponse response;
    const auto started = std::chrono::steady_clock::now();
    m_bCancelRequested.store(false);
    HttpCallbackState callback;
    callback.maxResponseBytes = request.server.maxResponseBytes;
    const bool secure = request.server.transport == TransportKind::Https || request.server.scheme == "https";
    const std::string name = NextClientName("mt-interface-http-");
    CSocketClient* client = secure ? CreateHttpsClientInstanceByName(name.c_str()) :
        CreateHttpClientInstanceByName(name.c_str());
    if (client == nullptr)
    {
        response.error = "创建 SocketClient HTTP 实例失败";
        return response;
    }
    auto destroy = [&]() {
        std::lock_guard<std::mutex> active(m_clActiveMutex);
        if (m_pActiveClient == client) m_pActiveClient = nullptr;
        client->Stop();
        if (secure) DelHttpsClientInstance(client); else DelHttpClientInstance(client);
        response.elapsedMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - started).count();
    };
    if (secure && !ConfigureSocketClientTls(client, request.server.tlsCaFile.c_str(),
        request.server.tlsVerify ? 1 : 0))
    {
        response.error = "HTTPS 证书链配置失败: " + request.server.tlsCaFile;
        destroy();
        return response;
    }
    {
        std::lock_guard<std::mutex> active(m_clActiveMutex);
        m_pActiveClient = client;
        if (m_bCancelRequested.load()) client->Stop();
    }
    if (m_bCancelRequested.load())
    {
        response.error = "HTTP 请求已取消";
        destroy();
        return response;
    }
    if (!client->Connect(request.server.host.c_str(), request.server.port, OnHttpEvent, &callback))
    {
        response.error = "SocketClient Connect 调用失败";
        destroy();
        return response;
    }
    {
        std::unique_lock<std::mutex> lock(callback.mutex);
        const bool ready = WaitFor(callback.changed, lock, request.server.connectTimeoutMs,
            m_bCancelRequested, [&]() { return callback.connected || callback.closed; });
        if (m_bCancelRequested.load()) response.error = "HTTP 请求已取消";
        else if (!ready || !callback.connected)
            response.error = "HTTP 连接超时或断开，SocketClient 错误码=" + std::to_string(callback.errorCode);
        if (!response.error.empty()) { lock.unlock(); destroy(); return response; }
    }

    std::map<std::string, std::string> merged;
    for (const auto& header : request.headers) merged[header.first] = header.second;
    const bool hasHost = std::any_of(merged.begin(), merged.end(), [](const auto& header) {
        return HeaderEquals(header.first, "Host");
    });
    if (!hasHost)
    {
        const bool defaultPort = (secure && request.server.port == 443) || (!secure && request.server.port == 80);
        merged["Host"] = defaultPort ? request.server.host :
            request.server.host + ":" + std::to_string(request.server.port);
    }
    std::vector<std::string> names, values;
    std::vector<const char*> namePointers, valuePointers;
    for (const auto& header : merged) { names.push_back(header.first); values.push_back(header.second); }
    for (size_t index = 0; index < names.size(); ++index)
    { namePointers.push_back(names[index].c_str()); valuePointers.push_back(values[index].c_str()); }
    response.requestBytes = request.body.size();
    const std::string path = RequestPath(request.server, request.path);
    if (!client->Request(request.method.c_str(), path.c_str(), namePointers.data(),
        valuePointers.data(), static_cast<int>(namePointers.size()),
        request.body.empty() ? nullptr : request.body.data(), static_cast<int>(request.body.size())))
    {
        response.error = "SocketClient Request 调用失败";
        destroy();
        return response;
    }
    {
        std::unique_lock<std::mutex> lock(callback.mutex);
        const bool ready = WaitFor(callback.changed, lock, request.server.requestTimeoutMs,
            m_bCancelRequested, [&]() { return callback.complete || callback.closed || callback.tooLarge; });
        response.statusCode = callback.status;
        response.responseHeaders = callback.headers;
        response.responseBody = callback.body;
        response.responseBytes = callback.complete && callback.wireBodyBytes != 0 ?
            callback.wireBodyBytes : callback.body.size();
        if (m_bCancelRequested.load()) response.error = "HTTP 请求已取消";
        else if (!ready) response.error = "HTTP 响应超时";
        else if (callback.tooLarge) response.error = "HTTP 响应超过 maxResponseBytes";
        else if (!callback.complete)
            response.error = "HTTP 响应完成前连接断开，SocketClient 错误码=" + std::to_string(callback.errorCode);
        else response.transportSucceeded = true;
    }
    destroy();
    return response;
}

void SocketHttpTransport::Cancel()
{
    m_bCancelRequested.store(true);
    std::lock_guard<std::mutex> active(m_clActiveMutex);
    if (m_pActiveClient != nullptr) m_pActiveClient->Stop();
}

SocketWebTransport::~SocketWebTransport() { Disconnect(); }

bool SocketWebTransport::Connect(const ServerProfile& server,
    const std::vector<std::pair<std::string, std::string>>& headers,
    MessageCallback messageCallback, StateCallback stateCallback, std::string& error)
{
    Disconnect();
    error.clear();
    m_bStopping.store(false);
    m_bHeartbeatStop.store(false);
    const bool secure = server.scheme == "wss";
    const std::string name = NextClientName("mt-interface-web-");
    CSocketClient* created = secure ? CreateWssClientInstanceByName(name.c_str()) :
        CreateWebClientInstanceByName(name.c_str());
    if (created == nullptr) { error = "创建 SocketClient WebSocket 实例失败"; return false; }
    if (secure && !ConfigureSocketClientTls(created, server.tlsCaFile.c_str(), server.tlsVerify ? 1 : 0))
    {
        error = "WSS 证书链配置失败: " + server.tlsCaFile;
        DelWssClientInstance(created);
        return false;
    }
    {
        std::lock_guard<std::mutex> calls(m_clClientCallMutex);
        std::lock_guard<std::mutex> state(m_clMutex);
        m_pClient = created; m_bSecure = secure; m_bTransportConnected = false; m_bWebOpened = false;
        m_bClosed = false; m_iErrorCode = 0; m_clMessageCallback = std::move(messageCallback);
        m_clStateCallback = std::move(stateCallback);
    }
    {
        std::lock_guard<std::mutex> calls(m_clClientCallMutex);
        if (!created->Connect(server.host.c_str(), server.port, OnSocketEvent, this))
        { error = "SocketClient WebSocket Connect 调用失败"; }
    }
    if (!error.empty()) { Disconnect(); return false; }
    {
        std::unique_lock<std::mutex> state(m_clMutex);
        const bool ready = m_clChanged.wait_for(state, std::chrono::milliseconds(server.connectTimeoutMs),
            [&]() { return m_bTransportConnected || m_bClosed || m_bStopping.load(); });
        if (!ready || !m_bTransportConnected)
            error = m_bStopping.load() ? "WebSocket 连接已取消" :
                "WebSocket 连接超时或断开，SocketClient 错误码=" + std::to_string(m_iErrorCode);
    }
    if (!error.empty()) { Disconnect(); return false; }

    std::vector<std::string> names, values;
    std::vector<const char*> namePointers, valuePointers;
    for (const auto& header : headers) { names.push_back(header.first); values.push_back(header.second); }
    for (size_t index = 0; index < names.size(); ++index)
    { namePointers.push_back(names[index].c_str()); valuePointers.push_back(values[index].c_str()); }
    const bool defaultPort = (secure && server.port == 443) || (!secure && server.port == 80);
    const std::string host = defaultPort ? server.host : server.host + ":" + std::to_string(server.port);
    {
        std::lock_guard<std::mutex> calls(m_clClientCallMutex);
        {
            std::lock_guard<std::mutex> state(m_clMutex);
            if (m_pClient != created || m_bStopping.load()) error = "WebSocket 连接已取消";
        }
        if (error.empty() && !created->UpgradeWithHeaders(server.webSocketPath.c_str(), host.c_str(),
            namePointers.empty() ? nullptr : namePointers.data(),
            valuePointers.empty() ? nullptr : valuePointers.data(), static_cast<int>(namePointers.size())))
            error = "WebSocket Upgrade 调用失败";
    }
    if (!error.empty()) { Disconnect(); return false; }
    {
        std::unique_lock<std::mutex> state(m_clMutex);
        const bool ready = m_clChanged.wait_for(state, std::chrono::milliseconds(server.requestTimeoutMs),
            [&]() { return m_bWebOpened || m_bClosed || m_bStopping.load(); });
        if (!ready || !m_bWebOpened)
            error = m_bStopping.load() ? "WebSocket 握手已取消" :
                "WebSocket 握手失败或超时，SocketClient 错误码=" + std::to_string(m_iErrorCode);
    }
    if (!error.empty()) { Disconnect(); return false; }
    return true;
}

bool SocketWebTransport::Send(const std::string& json, std::string& error)
{
    error.clear();
    MessageCallback callback;
    {
        std::lock_guard<std::mutex> calls(m_clClientCallMutex);
        CSocketClient* target = nullptr;
        {
            std::lock_guard<std::mutex> state(m_clMutex);
            if (m_pClient == nullptr || !m_bWebOpened || m_bStopping.load())
            { error = "WebSocket 尚未连接"; return false; }
            target = m_pClient;
            callback = m_clMessageCallback;
        }
        if (!target->SendFrame(1, json.data(), static_cast<int>(json.size())))
        { error = "SocketClient SendFrame 调用失败"; return false; }
    }
    if (callback) callback(MakeWebMessage("发送", json));
    return true;
}

void SocketWebTransport::SetHeartbeat(const std::string& json, uint32_t intervalMs)
{
    m_bHeartbeatStop.store(true);
    m_clChanged.notify_all();
    if (m_clHeartbeatThread.joinable() && m_clHeartbeatThread.get_id() != std::this_thread::get_id())
        m_clHeartbeatThread.join();
    {
        std::lock_guard<std::mutex> state(m_clMutex);
        m_strHeartbeatJson = json; m_u32HeartbeatIntervalMs = intervalMs;
    }
    m_bHeartbeatStop.store(false);
    if (!json.empty() && intervalMs != 0 && !m_bStopping.load())
        m_clHeartbeatThread = std::thread(&SocketWebTransport::HeartbeatLoop, this);
}

void SocketWebTransport::HeartbeatLoop()
{
    while (!m_bStopping.load() && !m_bHeartbeatStop.load())
    {
        std::string payload;
        uint32_t interval = 0;
        {
            std::unique_lock<std::mutex> state(m_clMutex);
            payload = m_strHeartbeatJson; interval = m_u32HeartbeatIntervalMs;
            if (m_clChanged.wait_for(state, std::chrono::milliseconds(interval),
                [&]() { return m_bStopping.load() || m_bHeartbeatStop.load(); })) return;
        }
        std::string error;
        if (!Send(payload, error))
        {
            StateCallback callback;
            { std::lock_guard<std::mutex> state(m_clMutex); callback = m_clStateCallback; }
            if (callback && !m_bStopping.load()) callback(false, "WebSocket 心跳发送失败: " + error);
            return;
        }
    }
}

void SocketWebTransport::Disconnect()
{
    m_bStopping.store(true);
    m_bHeartbeatStop.store(true);
    m_clChanged.notify_all();
    if (m_clHeartbeatThread.joinable() && m_clHeartbeatThread.get_id() != std::this_thread::get_id())
        m_clHeartbeatThread.join();
    std::lock_guard<std::mutex> calls(m_clClientCallMutex);
    CSocketClient* client = nullptr;
    bool secure = false;
    {
        std::lock_guard<std::mutex> state(m_clMutex);
        client = m_pClient; secure = m_bSecure;
    }
    if (client != nullptr) client->Stop();
    {
        std::lock_guard<std::mutex> state(m_clMutex);
        if (m_pClient == client) m_pClient = nullptr;
        m_bTransportConnected = false; m_bWebOpened = false; m_bClosed = true;
        m_clMessageCallback = {}; m_clStateCallback = {}; m_strHeartbeatJson.clear(); m_u32HeartbeatIntervalMs = 0;
    }
    if (client != nullptr)
    {
        if (secure) DelWssClientInstance(client); else DelWebClientInstance(client);
    }
}

bool SocketWebTransport::IsConnected() const
{
    std::lock_guard<std::mutex> state(m_clMutex);
    return m_pClient != nullptr && m_bWebOpened && !m_bClosed && !m_bStopping.load();
}

void SocketWebTransport::OnSocketEvent(CSocketClient* client,
    const ST_SOCKET_CLIENT_EVENT* event, void* context)
{
    if (event != nullptr && context != nullptr)
        static_cast<SocketWebTransport*>(context)->HandleSocketEvent(client, *event);
}

void SocketWebTransport::HandleSocketEvent(CSocketClient* source, const ST_SOCKET_CLIENT_EVENT& event)
{
    MessageCallback messageCallback;
    StateCallback stateCallback;
    WebMessage message;
    bool hasMessage = false;
    bool notifyState = false;
    bool stateValue = false;
    std::string stateText;
    if (event.enEvent == EN_SOCKET_CLIENT_WEB_FRAME && event.byOpcode == 9)
    {
        // 回调执行期间 source 的生命周期由 SocketClient 保证。这里不能获取
        // m_clClientCallMutex，否则 Disconnect 持锁调用 Stop 等待本回调时会死锁。
        if (source != nullptr) source->SendFrame(10, event.pData, event.iDataLength);
        return;
    }
    {
        std::lock_guard<std::mutex> state(m_clMutex);
        switch (event.enEvent)
        {
        case EN_SOCKET_CLIENT_CONNECTED: m_bTransportConnected = true; break;
        case EN_SOCKET_CLIENT_WEB_OPEN:
            m_bWebOpened = true; m_bClosed = false; notifyState = true; stateValue = true;
            stateText = "WebSocket 已连接"; break;
        case EN_SOCKET_CLIENT_WEB_FRAME:
            if (event.byOpcode == 1 && event.pData != nullptr && event.iDataLength >= 0)
            {
                message = MakeWebMessage("接收", std::string(static_cast<const char*>(event.pData),
                    static_cast<size_t>(event.iDataLength)));
                messageCallback = m_clMessageCallback; hasMessage = true;
            }
            else if (event.byOpcode == 8)
            {
                m_bClosed = true; m_bWebOpened = false; notifyState = true;
                stateText = "服务端关闭 WebSocket";
            }
            break;
        case EN_SOCKET_CLIENT_CLOSED:
            m_bClosed = true; m_bTransportConnected = false; m_bWebOpened = false;
            m_iErrorCode = event.iErrorCode; notifyState = !m_bStopping.load();
            stateText = "WebSocket 已断开，错误码=" + std::to_string(m_iErrorCode); break;
        case EN_SOCKET_CLIENT_HTTP_ERROR:
            m_bClosed = true; m_iErrorCode = event.iErrorCode; notifyState = !m_bStopping.load();
            stateText = "WebSocket 协议错误，错误码=" + std::to_string(m_iErrorCode); break;
        default: break;
        }
        if (notifyState) stateCallback = m_clStateCallback;
        m_clChanged.notify_all();
    }
    if (hasMessage && messageCallback) messageCallback(message);
    if (notifyState && stateCallback) stateCallback(stateValue, stateText);
}
} // namespace interface_test
