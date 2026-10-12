#include "../../include/MtLoadEngine.h"

// C ABI 适配层只负责参数、版本、缓冲区和异常边界；业务状态由 LoadEngine 管理。

#include <algorithm>
#include <cstring>
#include <new>
#include <string>

#include "../engine/LoadEngine.h"

namespace
{
// 将不透明 C 句柄还原为内部对象；本函数不取得或转移对象所有权。
mtload::LoadEngine* Engine(MTLOAD_HANDLE handle)
{
    return static_cast<mtload::LoadEngine*>(handle);
}

// 实现统一的“两次调用”字符串复制约定，并始终回写所需字节数。
int CopyString(const std::string& value, char* buffer, uint32_t* inoutBytes)
{
    if (inoutBytes == nullptr) return MTLOAD_E_INVALID_ARGUMENT;
    const uint32_t required = static_cast<uint32_t>(value.size() + 1);
    if (buffer == nullptr || *inoutBytes < required)
    {
        *inoutBytes = required;
        return MTLOAD_E_BUFFER_TOO_SMALL;
    }
    std::memcpy(buffer, value.c_str(), required);
    *inoutBytes = required;
    return MTLOAD_OK;
}

// 实现统一的“两次调用”POD 数组复制约定，避免把内部容器内存暴露给调用方。
template<class T> int CopyRows(const std::vector<T>& source, T* rows, uint32_t* inoutCount)
{
    if (inoutCount == nullptr) return MTLOAD_E_INVALID_ARGUMENT;
    const uint32_t required = static_cast<uint32_t>(source.size());
    if (rows == nullptr || *inoutCount < required)
    {
        *inoutCount = required;
        return required == 0 ? MTLOAD_OK : MTLOAD_E_BUFFER_TOO_SMALL;
    }
    std::copy(source.begin(), source.end(), rows);
    *inoutCount = required;
    return MTLOAD_OK;
}
}

extern "C"
{
// 关键实现：直接返回编译期 ABI 常量，不触碰任何实例状态。
uint32_t MTLOAD_CALL MtLoad_GetAbiVersion(void)
{
    return MTLOAD_ABI_VERSION;
}

// 关键实现：先验证调用方结构容量，再整体清零并写入受支持默认值。
int32_t MTLOAD_CALL MtLoad_InitOptions(MTLOAD_ENGINE_OPTIONS* options)
{
    if (options == nullptr || options->structSize < sizeof(MTLOAD_ENGINE_OPTIONS))
        return MTLOAD_E_INVALID_ARGUMENT;
    *options = {};
    options->structSize = sizeof(MTLOAD_ENGINE_OPTIONS);
    options->abiVersion = MTLOAD_ABI_VERSION;
    options->maxConnections = 1000;
    options->eventQueueCapacity = 65536;
    options->snapshotIntervalMs = 1000;
    options->maxResponseBytes = 8U * 1024U * 1024U;
    return MTLOAD_OK;
}

// 关键实现：在创建对象前完成 ABI 和范围校验，并把所有 C++ 异常封装为结果码。
int32_t MTLOAD_CALL MtLoad_Create(const MTLOAD_ENGINE_OPTIONS* options,
    MTLOAD_HANDLE* outHandle)
{
    if (outHandle == nullptr) return MTLOAD_E_INVALID_ARGUMENT;
    *outHandle = nullptr;
    if (options == nullptr || options->structSize < sizeof(MTLOAD_ENGINE_OPTIONS))
        return MTLOAD_E_INVALID_ARGUMENT;
    if (options->abiVersion != MTLOAD_ABI_VERSION) return MTLOAD_E_ABI_MISMATCH;
    if (options->maxConnections == 0 || options->maxConnections > 100000 ||
        options->snapshotIntervalMs < 100 || options->maxResponseBytes == 0)
        return MTLOAD_E_INVALID_ARGUMENT;
    try
    {
        *outHandle = new mtload::LoadEngine(*options);
        return MTLOAD_OK;
    }
    catch (const std::bad_alloc&) { return MTLOAD_E_START_FAILED; }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：校验句柄和 UTF-8 路径后转发给状态机，异常不得越过 C ABI。
int32_t MTLOAD_CALL MtLoad_LoadPlan(MTLOAD_HANDLE handle, const char* path)
{
    if (Engine(handle) == nullptr || path == nullptr || *path == '\0') return MTLOAD_E_INVALID_ARGUMENT;
    try { return Engine(handle)->LoadPlanFile(path); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：仅转发当前计划校验，保持 C ABI 异常边界。
int32_t MTLOAD_CALL MtLoad_ValidatePlan(MTLOAD_HANDLE handle)
{
    if (Engine(handle) == nullptr) return MTLOAD_E_INVALID_ARGUMENT;
    try { return Engine(handle)->Validate(); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：先获取环境快照，再通过统一数组复制协议交给调用方。
int32_t MTLOAD_CALL MtLoad_GetEnvironments(MTLOAD_HANDLE handle,
    MTLOAD_ENVIRONMENT* rows, uint32_t* inoutCount)
{
    if (Engine(handle) == nullptr) return MTLOAD_E_INVALID_ARGUMENT;
    try { return CopyRows(Engine(handle)->GetEnvironments(), rows, inoutCount); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：拒绝空环境 ID，并由内部状态机判断是否允许切换。
int32_t MTLOAD_CALL MtLoad_SelectEnvironment(MTLOAD_HANDLE handle,
    const char* environmentId)
{
    if (Engine(handle) == nullptr || environmentId == nullptr || *environmentId == '\0')
        return MTLOAD_E_INVALID_ARGUMENT;
    try { return Engine(handle)->SelectEnvironment(environmentId); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：场景容器在锁内复制完成后才写入调用方缓冲区。
int32_t MTLOAD_CALL MtLoad_GetScenarios(MTLOAD_HANDLE handle,
    MTLOAD_SCENARIO* rows, uint32_t* inoutCount)
{
    if (Engine(handle) == nullptr) return MTLOAD_E_INVALID_ARGUMENT;
    try { return CopyRows(Engine(handle)->GetScenarios(), rows, inoutCount); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：允许空字符串表达“全部场景”，但不接受空指针。
int32_t MTLOAD_CALL MtLoad_SelectScenario(MTLOAD_HANDLE handle,
    const char* scenarioId)
{
    if (Engine(handle) == nullptr || scenarioId == nullptr) return MTLOAD_E_INVALID_ARGUMENT;
    try { return Engine(handle)->SelectScenario(scenarioId); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：只复制不含密码的用户摘要 POD。
int32_t MTLOAD_CALL MtLoad_GetUsers(MTLOAD_HANDLE handle,
    MTLOAD_USER* rows, uint32_t* inoutCount)
{
    if (Engine(handle) == nullptr) return MTLOAD_E_INVALID_ARGUMENT;
    try { return CopyRows(Engine(handle)->GetUsers(), rows, inoutCount); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：空字符串恢复 UserRefs，非空 ID 由引擎验证存在性。
int32_t MTLOAD_CALL MtLoad_SelectUser(MTLOAD_HANDLE handle, const char* userId)
{
    if (Engine(handle) == nullptr || userId == nullptr) return MTLOAD_E_INVALID_ARGUMENT;
    try { return Engine(handle)->SelectUser(userId); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：空运行名规范化为空字符串，启动本身保持异步。
int32_t MTLOAD_CALL MtLoad_Start(MTLOAD_HANDLE handle, const char* runName)
{
    if (Engine(handle) == nullptr) return MTLOAD_E_INVALID_ARGUMENT;
    try { return Engine(handle)->Start(runName == nullptr ? "" : runName); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：转发暂停请求，不等待在途网络请求结束。
int32_t MTLOAD_CALL MtLoad_Pause(MTLOAD_HANDLE handle)
{
    if (Engine(handle) == nullptr) return MTLOAD_E_INVALID_ARGUMENT;
    try { return Engine(handle)->Pause(); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：转发继续请求，由内部暂停条件变量唤醒虚拟用户。
int32_t MTLOAD_CALL MtLoad_Resume(MTLOAD_HANDLE handle)
{
    if (Engine(handle) == nullptr) return MTLOAD_E_INVALID_ARGUMENT;
    try { return Engine(handle)->Resume(); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：幂等设置停止标记，调用线程不执行 join。
int32_t MTLOAD_CALL MtLoad_Stop(MTLOAD_HANDLE handle)
{
    if (Engine(handle) == nullptr) return MTLOAD_E_INVALID_ARGUMENT;
    try { return Engine(handle)->Stop(); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：由引擎状态机保证运行期间不能清理结果。
int32_t MTLOAD_CALL MtLoad_ClearResults(MTLOAD_HANDLE handle)
{
    if (Engine(handle) == nullptr) return MTLOAD_E_INVALID_ARGUMENT;
    try { return Engine(handle)->ClearResults(); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：把调用方超时原样传入内部条件等待。
int32_t MTLOAD_CALL MtLoad_Wait(MTLOAD_HANDLE handle, uint32_t timeoutMs)
{
    if (Engine(handle) == nullptr) return MTLOAD_E_INVALID_ARGUMENT;
    try { return Engine(handle)->Wait(timeoutMs); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：在读状态前验证结构大小和 ABI 版本，防止越界写入。
int32_t MTLOAD_CALL MtLoad_GetRunState(MTLOAD_HANDLE handle, MTLOAD_RUN_STATE* outState)
{
    if (Engine(handle) == nullptr || outState == nullptr ||
        outState->structSize < sizeof(MTLOAD_RUN_STATE)) return MTLOAD_E_INVALID_ARGUMENT;
    if (outState->abiVersion != MTLOAD_ABI_VERSION) return MTLOAD_E_ABI_MISMATCH;
    try { return Engine(handle)->GetState(*outState); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：使用快照和两次调用模式返回聚合指标。
int32_t MTLOAD_CALL MtLoad_GetMetricRows(MTLOAD_HANDLE handle,
    MTLOAD_METRIC_ROW* rows, uint32_t* inoutCount)
{
    if (Engine(handle) == nullptr) return MTLOAD_E_INVALID_ARGUMENT;
    try { return CopyRows(Engine(handle)->GetMetrics(), rows, inoutCount); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：使用快照和两次调用模式返回脱敏错误样本。
int32_t MTLOAD_CALL MtLoad_GetErrorSamples(MTLOAD_HANDLE handle,
    MTLOAD_ERROR_SAMPLE* rows, uint32_t* inoutCount)
{
    if (Engine(handle) == nullptr) return MTLOAD_E_INVALID_ARGUMENT;
    try { return CopyRows(Engine(handle)->GetErrors(), rows, inoutCount); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：先生成完整 JSON，再按含结尾零字符的容量规则复制。
int32_t MTLOAD_CALL MtLoad_GetSnapshotJson(MTLOAD_HANDLE handle,
    char* buffer, uint32_t* inoutBytes)
{
    if (Engine(handle) == nullptr) return MTLOAD_E_INVALID_ARGUMENT;
    try { return CopyString(Engine(handle)->GetSnapshotJson(), buffer, inoutBytes); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：复制最近错误文本，不返回内部 string 指针。
int32_t MTLOAD_CALL MtLoad_GetLastError(MTLOAD_HANDLE handle,
    char* buffer, uint32_t* inoutBytes)
{
    if (Engine(handle) == nullptr) return MTLOAD_E_INVALID_ARGUMENT;
    try { return CopyString(Engine(handle)->GetLastError(), buffer, inoutBytes); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：验证非空目录和格式位掩码后导出同一份指标快照。
int32_t MTLOAD_CALL MtLoad_ExportReport(MTLOAD_HANDLE handle,
    const char* outputDirectory, uint32_t formats)
{
    if (Engine(handle) == nullptr || outputDirectory == nullptr || *outputDirectory == '\0')
        return MTLOAD_E_INVALID_ARGUMENT;
    try { return Engine(handle)->Export(outputDirectory, formats); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：单文件导出要求明确路径，格式和扩展名匹配由引擎检查。
int32_t MTLOAD_CALL MtLoad_ExportReportFile(MTLOAD_HANDLE handle,
    const char* absoluteFilePath, uint32_t format)
{
    if (handle == nullptr || absoluteFilePath == nullptr || *absoluteFilePath == '\0')
        return MTLOAD_E_INVALID_ARGUMENT;
    try { return Engine(handle)->ExportFile(absoluteFilePath, format); }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：导出成功后再复制实际路径，缓冲区不足不会写入截断路径。
int32_t MTLOAD_CALL MtLoad_ExportConfiguredReport(MTLOAD_HANDLE handle,
    uint32_t format, char* outputPath, uint32_t outputPathBytes)
{
    if (handle == nullptr || outputPath == nullptr || outputPathBytes == 0)
        return MTLOAD_E_INVALID_ARGUMENT;
    try
    {
        std::string path;
        const int result = Engine(handle)->ExportConfigured(format, path);
        if (result != MTLOAD_OK) return result;
        if (path.size() + 1 > outputPathBytes) return MTLOAD_E_BUFFER_TOO_SMALL;
        std::memcpy(outputPath, path.c_str(), path.size() + 1);
        return MTLOAD_OK;
    }
    catch (...) { return MTLOAD_E_INTERNAL; }
}

// 关键实现：delete 触发停止、唤醒和线程联结，确保句柄释放后无后台任务。
int32_t MTLOAD_CALL MtLoad_Destroy(MTLOAD_HANDLE handle)
{
    if (Engine(handle) == nullptr) return MTLOAD_E_INVALID_ARGUMENT;
    try
    {
        delete Engine(handle);
        return MTLOAD_OK;
    }
    catch (...) { return MTLOAD_E_INTERNAL; }
}
}

