#include "pch.h"
#include "LoadEngineClient.h"

#include <algorithm>

// 关键实现：复用 Unload 的停止、销毁和 FreeLibrary 顺序。
CLoadEngineClient::~CLoadEngineClient()
{
    Unload();
}

#if !defined(MTLOADTEST_STATIC_STACK)
// 关键实现：动态配置中每个导出符号解析失败立即返回具体函数名，禁止保留半初始化函数表。
template<class T> bool CLoadEngineClient::Resolve(T& target, const char* name, std::wstring& error)
{
    target = reinterpret_cast<T>(GetProcAddress(m_hModule, name));
    if (target != nullptr) return true;
    error = L"MtLoadEngine.dll 缺少导出函数: " + FromUtf8(name);
    return false;
}
#else
// 关键实现：静态配置直接取得已链接符号地址，复用后续统一的 ABI 校验和生命周期代码。
void CLoadEngineClient::BindStaticApi()
{
    m_pfnGetAbiVersion = &MtLoad_GetAbiVersion;
    m_pfnInitOptions = &MtLoad_InitOptions;
    m_pfnCreate = &MtLoad_Create;
    m_pfnLoadPlan = &MtLoad_LoadPlan;
    m_pfnValidatePlan = &MtLoad_ValidatePlan;
    m_pfnGetEnvironments = &MtLoad_GetEnvironments;
    m_pfnSelectEnvironment = &MtLoad_SelectEnvironment;
    m_pfnGetScenarios = &MtLoad_GetScenarios;
    m_pfnSelectScenario = &MtLoad_SelectScenario;
    m_pfnGetWorkloads = &MtLoad_GetWorkloads;
    m_pfnSelectWorkload = &MtLoad_SelectWorkload;
    getUsers_ = &MtLoad_GetUsers;
    m_pfnSelectUser = &MtLoad_SelectUser;
    m_pfnStart = &MtLoad_Start;
    m_pfnPause = &MtLoad_Pause;
    m_pfnResume = &MtLoad_Resume;
    m_pfnStop = &MtLoad_Stop;
    m_pfnClearResults = &MtLoad_ClearResults;
    m_pfnWait = &MtLoad_Wait;
    m_pfnGetRunState = &MtLoad_GetRunState;
    m_pfnGetMetricRows = &MtLoad_GetMetricRows;
    m_pfnGetScenarioMetricRows = &MtLoad_GetScenarioMetricRows;
    m_pfnGetThresholdResults = &MtLoad_GetThresholdResults;
    m_pfnGetErrorSamples = &MtLoad_GetErrorSamples;
    m_pfnExportConfiguredReport = &MtLoad_ExportConfiguredReport;
    m_pfnGetLastError = &MtLoad_GetLastError;
    m_pfnDestroy = &MtLoad_Destroy;
}
#endif

// 关键实现：按编译模式加载 DLL 或绑定静态符号，再统一核对 ABI 并创建句柄。
bool CLoadEngineClient::Load(const std::wstring& dllPath, std::wstring& error)
{
    Unload();
#if defined(MTLOADTEST_STATIC_STACK)
    (void)dllPath;
    BindStaticApi();
#else
    m_hModule = LoadLibraryW(dllPath.c_str());
    if (m_hModule == nullptr)
    {
        error = L"无法加载 MtLoadEngine.dll，Windows 错误码=" + std::to_wstring(GetLastError());
        return false;
    }
    if (!Resolve(m_pfnGetAbiVersion, "MtLoad_GetAbiVersion", error) ||
        !Resolve(m_pfnInitOptions, "MtLoad_InitOptions", error) || !Resolve(m_pfnCreate, "MtLoad_Create", error) ||
        !Resolve(m_pfnLoadPlan, "MtLoad_LoadPlan", error) || !Resolve(m_pfnValidatePlan, "MtLoad_ValidatePlan", error) ||
        !Resolve(m_pfnGetEnvironments, "MtLoad_GetEnvironments", error) ||
        !Resolve(m_pfnSelectEnvironment, "MtLoad_SelectEnvironment", error) ||
        !Resolve(m_pfnGetScenarios, "MtLoad_GetScenarios", error) ||
        !Resolve(m_pfnSelectScenario, "MtLoad_SelectScenario", error) ||
        !Resolve(m_pfnGetWorkloads, "MtLoad_GetWorkloads", error) ||
        !Resolve(m_pfnSelectWorkload, "MtLoad_SelectWorkload", error) ||
        !Resolve(getUsers_, "MtLoad_GetUsers", error) ||
        !Resolve(m_pfnSelectUser, "MtLoad_SelectUser", error) ||
        !Resolve(m_pfnStart, "MtLoad_Start", error) || !Resolve(m_pfnPause, "MtLoad_Pause", error) ||
        !Resolve(m_pfnResume, "MtLoad_Resume", error) || !Resolve(m_pfnStop, "MtLoad_Stop", error) ||
        !Resolve(m_pfnClearResults, "MtLoad_ClearResults", error) ||
        !Resolve(m_pfnWait, "MtLoad_Wait", error) || !Resolve(m_pfnGetRunState, "MtLoad_GetRunState", error) ||
        !Resolve(m_pfnGetMetricRows, "MtLoad_GetMetricRows", error) ||
        !Resolve(m_pfnGetScenarioMetricRows, "MtLoad_GetScenarioMetricRows", error) ||
        !Resolve(m_pfnGetThresholdResults, "MtLoad_GetThresholdResults", error) ||
        !Resolve(m_pfnGetErrorSamples, "MtLoad_GetErrorSamples", error) ||
        !Resolve(m_pfnExportConfiguredReport, "MtLoad_ExportConfiguredReport", error) ||
        !Resolve(m_pfnGetLastError, "MtLoad_GetLastError", error) || !Resolve(m_pfnDestroy, "MtLoad_Destroy", error))
    {
        Unload();
        return false;
    }
#endif
    if (m_pfnGetAbiVersion() != MTLOAD_ABI_VERSION)
    {
        error = L"MtLoadEngine ABI 版本不匹配";
        Unload();
        return false;
    }
    MTLOAD_ENGINE_OPTIONS options{};
    options.structSize = sizeof(options);
    if (m_pfnInitOptions(&options) != MTLOAD_OK || m_pfnCreate(&options, &m_hEngine) != MTLOAD_OK)
    {
        error = L"创建 MtLoadEngine 实例失败";
        Unload();
        return false;
    }
    return true;
}

// 关键实现：先请求停止和等待终态，再销毁实例；只有动态配置需要释放模块。
void CLoadEngineClient::Unload()
{
    if (m_hEngine != nullptr && m_pfnDestroy != nullptr)
    {
        if (m_pfnStop != nullptr) m_pfnStop(m_hEngine);
        if (m_pfnWait != nullptr) m_pfnWait(m_hEngine, UINT32_MAX);
        m_pfnDestroy(m_hEngine);
        m_hEngine = nullptr;
    }
    if (m_hModule != nullptr)
    {
        FreeLibrary(m_hModule);
        m_hModule = nullptr;
    }
}

// 关键实现：宽路径只转换一次 UTF-8，加载或校验失败均读取 DLL 最近错误。
bool CLoadEngineClient::LoadAndValidatePlan(const std::wstring& planPath, std::wstring& error)
{
    if (m_hEngine == nullptr)
    {
        error = L"压测引擎尚未加载";
        return false;
    }
    const std::string path = ToUtf8(planPath);
    int result = m_pfnLoadPlan(m_hEngine, path.c_str());
    if (result == MTLOAD_OK) result = m_pfnValidatePlan(m_hEngine);
    if (result != MTLOAD_OK)
    {
        error = LastError();
        if (error.empty()) error = L"计划加载或校验失败，错误码=" + std::to_wstring(result);
        return false;
    }
    return true;
}

// 关键实现：只提交异步启动命令，UI 后续通过定时快照观察 RUNNING 和终态。
bool CLoadEngineClient::Start(const std::string& runName, std::wstring& error)
{
    if (m_hEngine == nullptr)
    {
        error = L"压测引擎尚未加载";
        return false;
    }
    const int result = m_pfnStart(m_hEngine, runName.c_str());
    if (result == MTLOAD_OK) return true;
    error = LastError();
    if (error.empty()) error = L"启动压测失败，错误码=" + std::to_wstring(result);
    return false;
}

// 关键实现：先查询所需数量，再初始化每行 ABI 头并执行第二次复制。
std::vector<MTLOAD_ENVIRONMENT> CLoadEngineClient::GetEnvironments() const
{
    if (m_hEngine == nullptr || m_pfnGetEnvironments == nullptr) return {};
    uint32_t count = 0;
    int result = m_pfnGetEnvironments(m_hEngine, nullptr, &count);
    if (result != MTLOAD_E_BUFFER_TOO_SMALL && result != MTLOAD_OK) return {};
    std::vector<MTLOAD_ENVIRONMENT> rows(count);
    if (count == 0) return rows;
    if (m_pfnGetEnvironments(m_hEngine, rows.data(), &count) != MTLOAD_OK) return {};
    rows.resize(count);
    return rows;
}

// 关键实现：转发稳定环境 ID，错误统一从 DLL 转换为宽字符串。
bool CLoadEngineClient::SelectEnvironment(const std::string& environmentId,
    std::wstring& error)
{
    if (m_hEngine == nullptr || m_pfnSelectEnvironment == nullptr)
    {
        error = L"压测引擎尚未加载";
        return false;
    }
    const int result = m_pfnSelectEnvironment(m_hEngine, environmentId.c_str());
    if (result == MTLOAD_OK) return true;
    error = LastError();
    if (error.empty()) error = L"选择运行环境失败，错误码=" + std::to_wstring(result);
    return false;
}

// 关键实现：遵循 ABI 两次调用模式取得完整场景摘要数组。
std::vector<MTLOAD_SCENARIO> CLoadEngineClient::GetScenarios() const
{
    if (m_hEngine == nullptr || m_pfnGetScenarios == nullptr) return {};
    uint32_t count = 0;
    int result = m_pfnGetScenarios(m_hEngine, nullptr, &count);
    if (result != MTLOAD_E_BUFFER_TOO_SMALL && result != MTLOAD_OK) return {};
    std::vector<MTLOAD_SCENARIO> rows(count);
    if (count == 0) return rows;
    if (m_pfnGetScenarios(m_hEngine, rows.data(), &count) != MTLOAD_OK) return {};
    rows.resize(count);
    return rows;
}

// 关键实现：空字符串保留“全部场景”语义，不在宿主层改写。
bool CLoadEngineClient::SelectScenario(const std::string& scenarioId,
    std::wstring& error)
{
    if (m_hEngine == nullptr || m_pfnSelectScenario == nullptr)
    {
        error = L"压测引擎尚未加载";
        return false;
    }
    const int result = m_pfnSelectScenario(m_hEngine, scenarioId.c_str());
    if (result == MTLOAD_OK) return true;
    error = LastError();
    if (error.empty()) error = L"选择压测场景失败，错误码=" + std::to_wstring(result);
    return false;
}

// 关键实现：通过统一两次调用协议取得工作负载，兼容项和显式项由 DLL 标记。
std::vector<MTLOAD_WORKLOAD> CLoadEngineClient::GetWorkloads() const
{
    if (m_hEngine == nullptr || m_pfnGetWorkloads == nullptr) return {};
    uint32_t count = 0;
    int result = m_pfnGetWorkloads(m_hEngine, nullptr, &count);
    if (result != MTLOAD_E_BUFFER_TOO_SMALL && result != MTLOAD_OK) return {};
    std::vector<MTLOAD_WORKLOAD> rows(count);
    if (count == 0) return rows;
    if (m_pfnGetWorkloads(m_hEngine, rows.data(), &count) != MTLOAD_OK) return {};
    rows.resize(count);
    return rows;
}

// 关键实现：工作负载 ID 直接传给 V4 ABI，界面不自行解释负载曲线。
bool CLoadEngineClient::SelectWorkload(const std::string& workloadId,
    std::wstring& error)
{
    if (m_hEngine == nullptr || m_pfnSelectWorkload == nullptr)
    {
        error = L"压测引擎尚未加载";
        return false;
    }
    const int result = m_pfnSelectWorkload(m_hEngine, workloadId.c_str());
    if (result == MTLOAD_OK) return true;
    error = LastError();
    if (error.empty()) error = L"选择工作负载失败，错误码=" + std::to_wstring(result);
    return false;
}

// 关键实现：只复制 DLL 提供的脱敏用户摘要，宿主不读取 XML 密码。
std::vector<MTLOAD_USER> CLoadEngineClient::GetUsers() const
{
    if (m_hEngine == nullptr || getUsers_ == nullptr) return {};
    uint32_t count = 0;
    int result = getUsers_(m_hEngine, nullptr, &count);
    if (result != MTLOAD_E_BUFFER_TOO_SMALL && result != MTLOAD_OK) return {};
    std::vector<MTLOAD_USER> rows(count);
    if (count == 0) return rows;
    if (getUsers_(m_hEngine, rows.data(), &count) != MTLOAD_OK) return {};
    rows.resize(count);
    return rows;
}

// 关键实现：转发统一用户覆盖 ID，空字符串恢复场景配置。
bool CLoadEngineClient::SelectUser(const std::string& userId, std::wstring& error)
{
    if (m_hEngine == nullptr || m_pfnSelectUser == nullptr)
    {
        error = L"压测引擎尚未加载";
        return false;
    }
    const int result = m_pfnSelectUser(m_hEngine, userId.c_str());
    if (result == MTLOAD_OK) return true;
    error = LastError();
    if (error.empty()) error = L"选择压测用户失败，错误码=" + std::to_wstring(result);
    return false;
}

// 关键实现：提交暂停命令但不等待在途请求结束。
bool CLoadEngineClient::Pause(std::wstring& error)
{
    if (m_hEngine == nullptr || m_pfnPause == nullptr) { error = L"压测引擎尚未加载"; return false; }
    const int result = m_pfnPause(m_hEngine);
    if (result == MTLOAD_OK) return true;
    error = LastError();
    if (error.empty()) error = L"暂停压测失败，错误码=" + std::to_wstring(result);
    return false;
}

// 关键实现：提交继续命令并保留当前 runId 和场景位置。
bool CLoadEngineClient::Resume(std::wstring& error)
{
    if (m_hEngine == nullptr || m_pfnResume == nullptr) { error = L"压测引擎尚未加载"; return false; }
    const int result = m_pfnResume(m_hEngine);
    if (result == MTLOAD_OK) return true;
    error = LastError();
    if (error.empty()) error = L"继续压测失败，错误码=" + std::to_wstring(result);
    return false;
}

// 关键实现：由 DLL 状态机拒绝运行中清理，宿主只转换错误文本。
bool CLoadEngineClient::ClearResults(std::wstring& error)
{
    if (m_hEngine == nullptr || m_pfnClearResults == nullptr) { error = L"压测引擎尚未加载"; return false; }
    const int result = m_pfnClearResults(m_hEngine);
    if (result == MTLOAD_OK) return true;
    error = LastError();
    if (error.empty()) error = L"清空结果失败，错误码=" + std::to_wstring(result);
    return false;
}

// 关键实现：句柄和函数入口有效时幂等提交停止，不阻塞 UI。
void CLoadEngineClient::Stop()
{
    if (m_hEngine != nullptr && m_pfnStop != nullptr) m_pfnStop(m_hEngine);
}

// 关键实现：每次调用重新填写结构大小和 ABI 版本，防止复用旧快照头部。
bool CLoadEngineClient::GetState(MTLOAD_RUN_STATE& state) const
{
    if (m_hEngine == nullptr) return false;
    state = {};
    state.structSize = sizeof(state);
    state.abiVersion = MTLOAD_ABI_VERSION;
    return m_pfnGetRunState(m_hEngine, &state) == MTLOAD_OK;
}

// 关键实现：两次调用间指标可能增长，容量不足时返回空集合并等待下一轮刷新。
std::vector<MTLOAD_METRIC_ROW> CLoadEngineClient::GetMetrics() const
{
    if (m_hEngine == nullptr) return {};
    uint32_t count = 0;
    int result = m_pfnGetMetricRows(m_hEngine, nullptr, &count);
    if (result != MTLOAD_E_BUFFER_TOO_SMALL && result != MTLOAD_OK) return {};
    std::vector<MTLOAD_METRIC_ROW> rows(count);
    if (count == 0) return rows;
    if (m_pfnGetMetricRows(m_hEngine, rows.data(), &count) != MTLOAD_OK) return {};
    rows.resize(count);
    return rows;
}

// 关键实现：事务指标与接口指标使用不同 ABI 结构，防止宿主混淆吞吐口径。
std::vector<MTLOAD_SCENARIO_METRIC_ROW> CLoadEngineClient::GetScenarioMetrics() const
{
    if (m_hEngine == nullptr || m_pfnGetScenarioMetricRows == nullptr) return {};
    uint32_t count = 0;
    int result = m_pfnGetScenarioMetricRows(m_hEngine, nullptr, &count);
    if (result != MTLOAD_E_BUFFER_TOO_SMALL && result != MTLOAD_OK) return {};
    std::vector<MTLOAD_SCENARIO_METRIC_ROW> rows(count);
    if (count == 0) return rows;
    if (m_pfnGetScenarioMetricRows(m_hEngine, rows.data(), &count) != MTLOAD_OK) return {};
    rows.resize(count);
    return rows;
}

// 关键实现：运行未结束时通常返回空集合，终态后返回完整阈值明细。
std::vector<MTLOAD_THRESHOLD_RESULT> CLoadEngineClient::GetThresholdResults() const
{
    if (m_hEngine == nullptr || m_pfnGetThresholdResults == nullptr) return {};
    uint32_t count = 0;
    int result = m_pfnGetThresholdResults(m_hEngine, nullptr, &count);
    if (result != MTLOAD_E_BUFFER_TOO_SMALL && result != MTLOAD_OK) return {};
    std::vector<MTLOAD_THRESHOLD_RESULT> rows(count);
    if (count == 0) return rows;
    if (m_pfnGetThresholdResults(m_hEngine, rows.data(), &count) != MTLOAD_OK) return {};
    rows.resize(count);
    return rows;
}

// 关键实现：按 ABI 头初始化每个错误样本，并把数组所有权留在宿主容器。
std::vector<MTLOAD_ERROR_SAMPLE> CLoadEngineClient::GetErrorSamples() const
{
    if (m_hEngine == nullptr || m_pfnGetErrorSamples == nullptr) return {};
    uint32_t count = 0;
    int result = m_pfnGetErrorSamples(m_hEngine, nullptr, &count);
    if (result != MTLOAD_E_BUFFER_TOO_SMALL && result != MTLOAD_OK) return {};
    std::vector<MTLOAD_ERROR_SAMPLE> rows(count);
    if (count == 0) return rows;
    if (m_pfnGetErrorSamples(m_hEngine, rows.data(), &count) != MTLOAD_OK) return {};
    rows.resize(count);
    return rows;
}

// 关键实现：使用固定最大路径缓冲区接收 DLL 生成的实际导出路径。
bool CLoadEngineClient::ExportConfiguredReport(uint32_t format,
    std::wstring& outputPath, std::wstring& error)
{
    if (m_hEngine == nullptr || m_pfnExportConfiguredReport == nullptr)
    {
        error = L"压测引擎尚未加载";
        return false;
    }
    std::vector<char> path(32768);
    const int result = m_pfnExportConfiguredReport(m_hEngine, format, path.data(),
        static_cast<uint32_t>(path.size()));
    if (result == MTLOAD_OK)
    {
        outputPath = FromUtf8(path.data());
        return true;
    }
    error = LastError();
    if (error.empty()) error = L"导出压测报告失败，错误码=" + std::to_wstring(result);
    return false;
}

// 关键实现：先查询所需字节数，再读取完整 UTF-8 错误，避免截断中文诊断。
std::wstring CLoadEngineClient::LastError() const
{
    if (m_hEngine == nullptr || m_pfnGetLastError == nullptr) return {};
    uint32_t bytes = 0;
    m_pfnGetLastError(m_hEngine, nullptr, &bytes);
    std::vector<char> buffer(bytes == 0 ? 1 : bytes);
    if (m_pfnGetLastError(m_hEngine, buffer.data(), &bytes) != MTLOAD_OK) return {};
    return FromUtf8(buffer.data());
}

// 关键实现：按 Windows 严格 Unicode 转换两次计算容量。
std::string CLoadEngineClient::ToUtf8(const std::wstring& value)
{
    if (value.empty()) return {};
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), size, nullptr, nullptr);
    return result;
}

// 关键实现：拒绝非法 UTF-8 并按精确容量生成宽字符串。
std::wstring CLoadEngineClient::FromUtf8(const std::string& value)
{
    if (value.empty()) return {};
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), result.data(), size);
    return result;
}
