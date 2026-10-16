#pragma once

// MtLoadEngine 公共导出接口。除 MtLoad_Wait 和报告导出外，接口均为短时调用。
// 句柄由 MtLoad_Create 创建，并且只能由 MtLoad_Destroy 释放。

#include "MtLoadTypes.h"

#if defined(_WIN32)
#if defined(MTLOADENGINE_STATIC)
#define MTLOAD_API
#elif defined(MTLOADENGINE_EXPORTS)
#define MTLOAD_API __declspec(dllexport)
#else
#define MTLOAD_API __declspec(dllimport)
#endif
#define MTLOAD_CALL __cdecl
#else
#define MTLOAD_API __attribute__((visibility("default")))
#define MTLOAD_CALL
#endif

#ifdef __cplusplus
extern "C" {
#endif

// 返回当前 DLL 的 ABI 版本，不阻塞且不产生状态变化。
MTLOAD_API uint32_t MTLOAD_CALL MtLoad_GetAbiVersion(void);

// 填充推荐默认值。调用方先设置 structSize；函数会写入 abiVersion 及其他字段。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_InitOptions(MTLOAD_ENGINE_OPTIONS* options);

// 创建实例。options 仅在调用期间借用，成功后 outHandle 归调用方持有。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_Create(const MTLOAD_ENGINE_OPTIONS* options,
    MTLOAD_HANDLE* outHandle);

// 同步读取 UTF-8 XML 计划；成功后进入 PLAN_LOADED，不发起网络请求。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_LoadPlan(MTLOAD_HANDLE handle,
    const char* absolutePlanPathUtf8);

// 校验计划引用、范围和首版协议支持，成功后进入 VALIDATED。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_ValidatePlan(MTLOAD_HANDLE handle);

// 两次调用取得计划中的环境列表。环境信息不包含账号或密码。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_GetEnvironments(MTLOAD_HANDLE handle,
    MTLOAD_ENVIRONMENT* rows, uint32_t* inoutCount);

// 选择本轮使用的环境。只能在非运行状态调用；id 必须来自环境列表。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_SelectEnvironment(MTLOAD_HANDLE handle,
    const char* environmentIdUtf8);

// 两次调用取得场景列表；场景名称为 UTF-8，支持中文。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_GetScenarios(MTLOAD_HANDLE handle,
    MTLOAD_SCENARIO* rows, uint32_t* inoutCount);

// 选择本轮唯一运行的场景；传空字符串表示运行全部场景。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_SelectScenario(MTLOAD_HANDLE handle,
    const char* scenarioIdUtf8);

// 两次调用取得工作负载列表。工作负载独立定义场景组合、负载曲线和阈值。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_GetWorkloads(MTLOAD_HANDLE handle,
    MTLOAD_WORKLOAD* rows, uint32_t* inoutCount);

// 选择本轮工作负载。旧配置会把 Scenario.Load 转换为可选择的隐式工作负载。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_SelectWorkload(MTLOAD_HANDLE handle,
    const char* workloadIdUtf8);

// 两次调用取得 Users 中的用户摘要；不返回密码和扩展参数。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_GetUsers(MTLOAD_HANDLE handle,
    MTLOAD_USER* rows, uint32_t* inoutCount);

// 选择本轮统一使用的用户；传空字符串恢复按各 Scenario.UserRefs 分配。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_SelectUser(MTLOAD_HANDLE handle,
    const char* userIdUtf8);

// 异步启动当前计划。runNameUtf8 可为空，函数不等待网络连接。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_Start(MTLOAD_HANDLE handle,
    const char* runNameUtf8);

// 请求暂停调度。已经发出的请求继续完成，尚未发出的请求停在调度边界。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_Pause(MTLOAD_HANDLE handle);

// 从当前场景、步骤和迭代位置继续执行，不重新开始本轮。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_Resume(MTLOAD_HANDLE handle);

// 幂等请求停止，不等待工作线程退出。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_Stop(MTLOAD_HANDLE handle);

// 清空指标、错误样本和本轮时间。运行、暂停或停止过程中不允许调用。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_ClearResults(MTLOAD_HANDLE handle);

// 等待终态；timeoutMs 为毫秒，UINT32_MAX 表示无限等待。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_Wait(MTLOAD_HANDLE handle, uint32_t timeoutMs);

// 复制当前运行状态。调用前必须填写 structSize 和 abiVersion。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_GetRunState(MTLOAD_HANDLE handle,
    MTLOAD_RUN_STATE* outState);

// 两次调用取得指标。rows 为空时 *inoutCount 返回所需行数。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_GetMetricRows(MTLOAD_HANDLE handle,
    MTLOAD_METRIC_ROW* rows, uint32_t* inoutCount);

// 两次调用取得业务事务指标。事务统计不混入接口请求指标。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_GetScenarioMetricRows(MTLOAD_HANDLE handle,
    MTLOAD_SCENARIO_METRIC_ROW* rows, uint32_t* inoutCount);

// 两次调用取得本轮阈值判定明细；运行前或尚未评估时返回 0 行。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_GetThresholdResults(MTLOAD_HANDLE handle,
    MTLOAD_THRESHOLD_RESULT* rows, uint32_t* inoutCount);

// 两次调用取得错误样本，数组由调用方持有。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_GetErrorSamples(MTLOAD_HANDLE handle,
    MTLOAD_ERROR_SAMPLE* rows, uint32_t* inoutCount);

// 两次调用取得 UTF-8 JSON 快照；字节数包含结尾零字符。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_GetSnapshotJson(MTLOAD_HANDLE handle,
    char* buffer, uint32_t* inoutBytes);
// 两次调用取得最近错误的 UTF-8 文本；字节数包含结尾零字符。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_GetLastError(MTLOAD_HANDLE handle,
    char* buffer, uint32_t* inoutBytes);

// 从当前不可变快照向目录导出固定文件名报告；formats 可组合。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_ExportReport(MTLOAD_HANDLE handle,
    const char* outputDirectoryUtf8, uint32_t formats);

// 从当前不可变快照导出一个文件；format 只能指定一个格式，文件扩展名由调用方选择。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_ExportReportFile(MTLOAD_HANDLE handle,
    const char* absoluteFilePathUtf8, uint32_t format);

// 按 XML 的 Reports.directory 导出时间戳文件；目录为空时使用 XML 所在目录。
// outputPathUtf8 接收实际输出路径，outputPathBytes 是调用方缓冲区字节数。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_ExportConfiguredReport(MTLOAD_HANDLE handle,
    uint32_t format, char* outputPathUtf8, uint32_t outputPathBytes);

// 停止并释放实例。不得从 SocketClient 回调线程调用。
MTLOAD_API int32_t MTLOAD_CALL MtLoad_Destroy(MTLOAD_HANDLE handle);

#ifdef __cplusplus
}
#endif

