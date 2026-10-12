#pragma once

// SocketClient 的单次 HTTP/1.1 交换适配器。
// 当前里程碑每个逻辑请求创建独立连接；连接池复用在实施步骤 6 后续完成。

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "../config/LoadPlan.h"

namespace mtload
{
struct HttpResponse
{
    bool transportOk = false;       // 是否完成 HTTP 交换；不代表业务断言成功。
    int result = 0;                 // MTLOAD_RESULT 传输结果码。
    int status = 0;                 // HTTP 状态码；未收到响应头时为 0。
    uint64_t sentBytes = 0;         // 实际提交的请求正文大小，单位：字节。
    uint64_t receivedBytes = 0;     // 实际接收的响应正文大小，单位：字节。
    double elapsedMs = 0.0;         // 从发起连接到完成/失败的端到端耗时。
    std::string body;               // 响应正文副本，所有权随返回值交给调用方。
    std::string error;              // 已脱敏的 UTF-8 传输诊断。
};

// 执行一次同步 HTTP 交换。函数可由多个虚拟用户并发调用，每次调用独占连接状态。
// stopping 在连接和接收阶段用于协作取消；返回值始终携带计时和已知字节统计。
HttpResponse ExecuteHttp(const Target& target, const Request& request,
    const std::vector<Header>& globalHeaders, const std::string& expandedPath,
    const std::string& expandedBody, uint32_t maxResponseBytes,
    const std::atomic<bool>& stopping);
}

