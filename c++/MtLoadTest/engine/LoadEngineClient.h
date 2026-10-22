#pragma once

// MFC 与 MtLoadEngine 公共 C ABI 之间的唯一适配层。
// 动态配置拥有 DLL 模块，静态配置直接绑定符号；两种配置都拥有 MTLOAD_HANDLE。

#include <Windows.h>

#include <string>
#include <vector>

#include <MtLoadEngine.h>

class CLoadEngineClient final
{
public:
    // 构造空适配器，不加载 DLL 或创建引擎句柄。
    CLoadEngineClient() = default;
    // 先停止并销毁引擎句柄；动态配置随后释放 DLL 模块。
    ~CLoadEngineClient();
    CLoadEngineClient(const CLoadEngineClient&) = delete;
    CLoadEngineClient& operator=(const CLoadEngineClient&) = delete;

    // 动态配置加载指定 DLL，静态配置忽略路径并直接绑定 ABI；随后创建引擎实例。
    bool Load(const std::wstring& dllPath, std::wstring& error);
    // 幂等停止并释放句柄和模块；返回后所有函数指针均失效。
    void Unload();
    // 依次加载和校验 XML 计划，失败时返回 DLL 的详细错误。
    bool LoadAndValidatePlan(const std::wstring& planPath, std::wstring& error);
    // 使用 ABI 两次调用模式取得环境摘要副本。
    std::vector<MTLOAD_ENVIRONMENT> GetEnvironments() const;
    // 选择运行环境；仅转发稳定 ID，不缓存宿主侧副本。
    bool SelectEnvironment(const std::string& environmentId, std::wstring& error);
    // 使用 ABI 两次调用模式取得场景摘要副本。
    std::vector<MTLOAD_SCENARIO> GetScenarios() const;
    // 选择唯一场景；空 ID 表示全部场景。
    bool SelectScenario(const std::string& scenarioId, std::wstring& error);
    // 取得独立工作负载列表并选择其中一个。
    std::vector<MTLOAD_WORKLOAD> GetWorkloads() const;
    bool SelectWorkload(const std::string& workloadId, std::wstring& error);
    // 取得不含密码的用户摘要副本。
    std::vector<MTLOAD_USER> GetUsers() const;
    // 选择统一用户；空 ID 恢复场景 UserRefs。
    bool SelectUser(const std::string& userId, std::wstring& error);
    // 异步启动新一轮压测；函数仅等待 DLL 接受启动请求。
    bool Start(const std::string& runName, std::wstring& error);
    // 请求暂停新任务调度，保留当前运行位置。
    bool Pause(std::wstring& error);
    // 从暂停位置恢复调度。
    bool Resume(std::wstring& error);
    // 幂等请求停止；不在 UI 线程等待运行结束。
    void Stop();
    // 清空可导出结果；运行或暂停状态下由 DLL 拒绝。
    bool ClearResults(std::wstring& error);
    // 复制轻量运行状态；未加载 DLL 时返回 false。
    bool GetState(MTLOAD_RUN_STATE& state) const;
    // 取得指标行副本，失败时返回空集合。
    std::vector<MTLOAD_METRIC_ROW> GetMetrics() const;
    // 取得独立业务事务汇总和运行终点阈值结果。
    std::vector<MTLOAD_SCENARIO_METRIC_ROW> GetScenarioMetrics() const;
    std::vector<MTLOAD_THRESHOLD_RESULT> GetThresholdResults() const;
    // 取得脱敏错误样本副本，失败时返回空集合。
    std::vector<MTLOAD_ERROR_SAMPLE> GetErrorSamples() const;
    // 按 XML 报告目录导出单一格式并返回实际路径。
    bool ExportConfiguredReport(uint32_t format, std::wstring& outputPath,
        std::wstring& error);
    // 读取 DLL 最近错误并转换为 Windows 宽字符串。
    std::wstring LastError() const;

private:
#if !defined(MTLOADTEST_STATIC_STACK)
    // 从已加载模块解析指定导出函数；缺失符号时返回带函数名的错误。
    template<class T> bool Resolve(T& target, const char* name, std::wstring& error);
#else
    // 静态配置把函数表绑定到链接进 EXE 的 MtLoadEngine C ABI 符号。
    void BindStaticApi();
#endif
    // 宽字符串转换为 DLL 所需 UTF-8。
    static std::string ToUtf8(const std::wstring& value);
    // DLL 返回的 UTF-8 转换为界面宽字符串。
    static std::wstring FromUtf8(const std::string& value);

    HMODULE m_hModule = nullptr;    // 动态配置的模块句柄；静态配置始终为空。
    MTLOAD_HANDLE m_hEngine = nullptr; // DLL 引擎实例句柄，由 m_pfnDestroy 释放。
    decltype(&MtLoad_GetAbiVersion) m_pfnGetAbiVersion = nullptr; // ABI 版本查询入口。
    decltype(&MtLoad_InitOptions) m_pfnInitOptions = nullptr; // 选项默认值初始化入口。
    decltype(&MtLoad_Create) m_pfnCreate = nullptr; // 引擎实例创建入口。
    decltype(&MtLoad_LoadPlan) m_pfnLoadPlan = nullptr; // 计划加载入口。
    decltype(&MtLoad_ValidatePlan) m_pfnValidatePlan = nullptr; // 计划校验入口。
    decltype(&MtLoad_GetEnvironments) m_pfnGetEnvironments = nullptr; // 环境查询入口。
    decltype(&MtLoad_SelectEnvironment) m_pfnSelectEnvironment = nullptr; // 环境选择入口。
    decltype(&MtLoad_GetScenarios) m_pfnGetScenarios = nullptr; // 场景查询入口。
    decltype(&MtLoad_SelectScenario) m_pfnSelectScenario = nullptr; // 场景选择入口。
    decltype(&MtLoad_GetWorkloads) m_pfnGetWorkloads = nullptr; // 工作负载查询入口。
    decltype(&MtLoad_SelectWorkload) m_pfnSelectWorkload = nullptr; // 工作负载选择入口。
    decltype(&MtLoad_GetUsers) getUsers_ = nullptr; // 用户摘要查询入口。
    decltype(&MtLoad_SelectUser) m_pfnSelectUser = nullptr; // 用户选择入口。
    decltype(&MtLoad_Start) m_pfnStart = nullptr; // 异步启动入口。
    decltype(&MtLoad_Pause) m_pfnPause = nullptr; // 暂停入口。
    decltype(&MtLoad_Resume) m_pfnResume = nullptr; // 继续入口。
    decltype(&MtLoad_Stop) m_pfnStop = nullptr; // 停止入口。
    decltype(&MtLoad_ClearResults) m_pfnClearResults = nullptr; // 结果清理入口。
    decltype(&MtLoad_Wait) m_pfnWait = nullptr; // 终态等待入口。
    decltype(&MtLoad_GetRunState) m_pfnGetRunState = nullptr; // 状态快照入口。
    decltype(&MtLoad_GetMetricRows) m_pfnGetMetricRows = nullptr; // 指标查询入口。
    decltype(&MtLoad_GetScenarioMetricRows) m_pfnGetScenarioMetricRows = nullptr; // 事务指标入口。
    decltype(&MtLoad_GetThresholdResults) m_pfnGetThresholdResults = nullptr; // 阈值结果入口。
    decltype(&MtLoad_GetErrorSamples) m_pfnGetErrorSamples = nullptr; // 错误样本查询入口。
    decltype(&MtLoad_ExportConfiguredReport) m_pfnExportConfiguredReport = nullptr; // 配置化导出入口。
    decltype(&MtLoad_GetLastError) m_pfnGetLastError = nullptr; // 最近错误查询入口。
    decltype(&MtLoad_Destroy) m_pfnDestroy = nullptr; // 引擎销毁入口。
};
