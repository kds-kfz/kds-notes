#include "HttpTransport.h"

#include <SocketClient.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <thread>

#include "../../include/MtLoadTypes.h"

namespace mtload
{
namespace
{
// 单次 SocketClient 交换的回调状态；所有非原子字段均由 mutex 保护。
struct CallbackState
{
    std::mutex mutex;                  // 串行化回调写入和请求线程读取。
    std::condition_variable changed;   // 任一连接/响应事件发生后唤醒等待线程。
    bool connected = false;            // 是否已经收到连接成功事件。
    bool closed = false;               // 连接是否已经关闭或发生 HTTP 错误。
    bool complete = false;             // 是否已经收到 HTTP 完成事件。
    bool tooLarge = false;             // 响应正文是否超过配置上限。
    int status = 0;                    // 最近收到的 HTTP 状态码。
    int errorCode = 0;                 // SocketClient 或内部错误码。
    uint32_t maxResponseBytes = 0;     // 本请求允许累计的响应正文上限。
    uint64_t wireBodyBytes = 0;        // 压缩响应按解压前的线上字节统计接收速率。
    std::string body;                  // 按事件顺序拼接的原始响应正文。
};

// 将 SocketClient 异步事件深拷贝进调用栈状态，并在锁内更新完成谓词。
void OnSocketEvent(CSocketClient*, const ST_SOCKET_CLIENT_EVENT* event, void* context)
{
    if (event == nullptr || context == nullptr) return;
    auto& state = *static_cast<CallbackState*>(context);
    std::lock_guard<std::mutex> guard(state.mutex);
    switch (event->enEvent)
    {
    case EN_SOCKET_CLIENT_CONNECTED:
        state.connected = true;
        break;
    case EN_SOCKET_CLIENT_CLOSED:
        state.closed = true;
        state.errorCode = event->iErrorCode;
        break;
    case EN_SOCKET_CLIENT_HTTP_STATUS:
        state.status = event->usStatus;
        break;
    case EN_SOCKET_CLIENT_HTTP_BODY:
        if (event->pData != nullptr && event->iDataLength > 0)
        {
            if (state.body.size() + static_cast<size_t>(event->iDataLength) > state.maxResponseBytes)
                state.tooLarge = true;
            else
                state.body.append(static_cast<const char*>(event->pData), static_cast<size_t>(event->iDataLength));
        }
        break;
    case EN_SOCKET_CLIENT_HTTP_COMPLETE:
        state.wireBodyBytes = event->ullWireBodyBytes;
        state.complete = true;
        break;
    case EN_SOCKET_CLIENT_HTTP_ERROR:
        state.errorCode = event->iErrorCode == 0 ? MTLOAD_E_INTERNAL : event->iErrorCode;
        state.closed = true;
        break;
    default:
        break;
    }
    state.changed.notify_all();
}

// 为每个短连接生成进程内唯一名称，避免 SocketClient 实例注册冲突。
std::string NextClientName()
{
    static std::atomic<unsigned long long> sequence{ 1 };
    return "mtload-http-" + std::to_string(sequence.fetch_add(1));
}

// 以短周期检查停止标记的方式等待事件，保证停止请求不会被长超时阻塞。
template<class Predicate>
bool WaitForEvent(std::condition_variable& changed, std::unique_lock<std::mutex>& lock,
    uint32_t timeoutMs, const std::atomic<bool>& stopping, Predicate predicate)
{
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (!predicate() && !stopping.load())
    {
        const auto now = std::chrono::steady_clock::now();
        if (now >= deadline) return false;
        changed.wait_until(lock, (std::min)(deadline, now + std::chrono::milliseconds(50)));
    }
    return predicate();
}
}

// 关键实现：单次调用独占 SocketClient 与 CallbackState，完成连接、发送、接收和确定性清理。
HttpResponse ExecuteHttp(const Target& target, const Request& request,
    const std::vector<Header>& globalHeaders, const std::string& expandedPath,
    const std::string& expandedBody, uint32_t maxResponseBytes,
    const std::atomic<bool>& stopping)
{
    HttpResponse response;
    response.result = MTLOAD_E_INTERNAL;
    const auto started = std::chrono::steady_clock::now();
    CallbackState callback;
    callback.maxResponseBytes = maxResponseBytes;

    // 阶段一：按目标协议创建本请求独占客户端；HTTPS 由 SocketClient 完成 TLS 与证书校验。
    const bool secure = target.transport == "https";
    const std::string clientName = NextClientName();
    CSocketClient* client = secure ? CreateHttpsClientInstanceByName(clientName.c_str()) :
        CreateHttpClientInstanceByName(clientName.c_str());
    if (client == nullptr)
    {
        response.error = secure ? "创建 SocketClient HTTPS 实例失败" :
            "创建 SocketClient HTTP 实例失败";
        return response;
    }

    // HTTPS 在建连前加载配置指定的可信证书链；失败立即返回，避免伪装成响应超时。
    if (secure && !ConfigureSocketClientTls(client, target.tlsCaFile.c_str(),
        target.tlsVerify ? 1 : 0))
    {
        response.error = "SocketClient HTTPS 证书链加载失败: " + target.tlsCaFile;
        DelHttpsClientInstance(client);
        return response;
    }

    auto finish = [&]()
    {
        client->Stop();
        if (secure) DelHttpsClientInstance(client);
        else DelHttpClientInstance(client);
        response.elapsedMs = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - started).count();
    };

    // 阶段二：连接并等待异步回调确认；等待时不持有 SocketClient 内部锁。
    if (!client->Connect(target.host.c_str(), target.port, OnSocketEvent, &callback))
    {
        response.error = "SocketClient Connect 调用失败，HPSocket 错误码=" +
            std::to_string(callback.errorCode);
        finish();
        return response;
    }
    {
        std::unique_lock<std::mutex> lock(callback.mutex);
        const bool signaled = WaitForEvent(callback.changed, lock, target.connectTimeoutMs, stopping,
            [&]() { return callback.connected || callback.closed; });
        if (stopping.load())
        {
            response.result = MTLOAD_E_INVALID_STATE;
            response.error = "运行已停止";
            lock.unlock();
            finish();
            return response;
        }
        if (!signaled || !callback.connected)
        {
            response.result = MTLOAD_E_TIMEOUT;
            response.error = "HTTP 连接超时或断开，SocketClient 错误码=" +
                std::to_string(callback.errorCode);
            // Stop 会等待回调退出，调用前必须释放回调状态锁，避免关闭回调反向等待。
            lock.unlock();
            finish();
            return response;
        }
    }

    // 阶段三：请求级头覆盖全局头，并保持字符串存储到 Request 返回后仍有效。
    std::map<std::string, std::string> merged;
    for (const auto& header : globalHeaders) merged[header.name] = header.value;
    for (const auto& header : request.headers) merged[header.name] = header.value;
    // HTTP/1.1 必须携带 Host；显式非默认端口也必须进入 Host，供 Nginx/CloudFront 正确路由。
    if (merged.find("Host") == merged.end())
    {
        const bool defaultPort = (secure && target.port == 443) || (!secure && target.port == 80);
        merged["Host"] = defaultPort ? target.host : target.host + ":" + std::to_string(target.port);
    }
    if (!expandedBody.empty() && merged.find("Content-Type") == merged.end())
        merged["Content-Type"] = "application/json";

    std::vector<std::string> names;
    std::vector<std::string> values;
    std::vector<const char*> namePointers;
    std::vector<const char*> valuePointers;
    for (const auto& pair : merged)
    {
        names.push_back(pair.first);
        values.push_back(pair.second);
    }
    for (size_t i = 0; i < names.size(); ++i)
    {
        namePointers.push_back(names[i].c_str());
        valuePointers.push_back(values[i].c_str());
    }
    // 字节口径只统计实际请求正文，与响应正文字节保持一致。
    response.sentBytes = expandedBody.size();
    if (!client->Request(request.method.c_str(), expandedPath.c_str(),
        namePointers.empty() ? nullptr : namePointers.data(),
        valuePointers.empty() ? nullptr : valuePointers.data(),
        static_cast<int>(namePointers.size()), expandedBody.data(),
        static_cast<int>(expandedBody.size())))
    {
        response.error = "SocketClient Request 调用失败";
        finish();
        return response;
    }
    // 阶段四：等待完成、断开、超限、超时或停止中的第一个条件，并形成唯一结果。
    {
        std::unique_lock<std::mutex> lock(callback.mutex);
        const bool signaled = WaitForEvent(callback.changed, lock, target.requestTimeoutMs, stopping,
            [&]() { return callback.complete || callback.closed || callback.tooLarge; });
        response.status = callback.status;
        response.body = callback.body;
        response.receivedBytes = callback.complete ? callback.wireBodyBytes : callback.body.size();
        if (stopping.load())
        {
            response.result = MTLOAD_E_INVALID_STATE;
            response.error = "运行已停止";
        }
        else if (!signaled)
        {
            response.result = MTLOAD_E_TIMEOUT;
            response.error = "HTTP 响应超时";
        }
        else if (callback.tooLarge)
        {
            response.result = MTLOAD_E_INTERNAL;
            response.error = "HTTP 响应超过 maxResponseBytes";
        }
        else if (!callback.complete)
        {
            response.result = MTLOAD_E_INTERNAL;
            response.error = "HTTP 响应完成前连接断开，错误码=" + std::to_string(callback.errorCode);
        }
        else
        {
            response.transportOk = true;
            response.result = MTLOAD_OK;
        }
    }
    finish();
    return response;
}
}
