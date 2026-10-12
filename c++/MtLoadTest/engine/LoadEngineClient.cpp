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
    target = reinterpret_cast<T>(GetProcAddress(module_, name));
    if (target != nullptr) return true;
    error = L"MtLoadEngine.dll 缺少导出函数: " + FromUtf8(name);
    return false;
}
#else
// 关键实现：静态配置直接取得已链接符号地址，复用后续统一的 ABI 校验和生命周期代码。
void CLoadEngineClient::BindStaticApi()
{
    getAbiVersion_ = &MtLoad_GetAbiVersion;
    initOptions_ = &MtLoad_InitOptions;
    create_ = &MtLoad_Create;
    loadPlan_ = &MtLoad_LoadPlan;
    validatePlan_ = &MtLoad_ValidatePlan;
    getEnvironments_ = &MtLoad_GetEnvironments;
    selectEnvironment_ = &MtLoad_SelectEnvironment;
    getScenarios_ = &MtLoad_GetScenarios;
    selectScenario_ = &MtLoad_SelectScenario;
    getUsers_ = &MtLoad_GetUsers;
    selectUser_ = &MtLoad_SelectUser;
    start_ = &MtLoad_Start;
    pause_ = &MtLoad_Pause;
    resume_ = &MtLoad_Resume;
    stop_ = &MtLoad_Stop;
    clearResults_ = &MtLoad_ClearResults;
    wait_ = &MtLoad_Wait;
    getRunState_ = &MtLoad_GetRunState;
    getMetricRows_ = &MtLoad_GetMetricRows;
    getErrorSamples_ = &MtLoad_GetErrorSamples;
    exportConfiguredReport_ = &MtLoad_ExportConfiguredReport;
    getLastError_ = &MtLoad_GetLastError;
    destroy_ = &MtLoad_Destroy;
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
    module_ = LoadLibraryW(dllPath.c_str());
    if (module_ == nullptr)
    {
        error = L"无法加载 MtLoadEngine.dll，Windows 错误码=" + std::to_wstring(GetLastError());
        return false;
    }
    if (!Resolve(getAbiVersion_, "MtLoad_GetAbiVersion", error) ||
        !Resolve(initOptions_, "MtLoad_InitOptions", error) || !Resolve(create_, "MtLoad_Create", error) ||
        !Resolve(loadPlan_, "MtLoad_LoadPlan", error) || !Resolve(validatePlan_, "MtLoad_ValidatePlan", error) ||
        !Resolve(getEnvironments_, "MtLoad_GetEnvironments", error) ||
        !Resolve(selectEnvironment_, "MtLoad_SelectEnvironment", error) ||
        !Resolve(getScenarios_, "MtLoad_GetScenarios", error) ||
        !Resolve(selectScenario_, "MtLoad_SelectScenario", error) ||
        !Resolve(getUsers_, "MtLoad_GetUsers", error) ||
        !Resolve(selectUser_, "MtLoad_SelectUser", error) ||
        !Resolve(start_, "MtLoad_Start", error) || !Resolve(pause_, "MtLoad_Pause", error) ||
        !Resolve(resume_, "MtLoad_Resume", error) || !Resolve(stop_, "MtLoad_Stop", error) ||
        !Resolve(clearResults_, "MtLoad_ClearResults", error) ||
        !Resolve(wait_, "MtLoad_Wait", error) || !Resolve(getRunState_, "MtLoad_GetRunState", error) ||
        !Resolve(getMetricRows_, "MtLoad_GetMetricRows", error) ||
        !Resolve(getErrorSamples_, "MtLoad_GetErrorSamples", error) ||
        !Resolve(exportConfiguredReport_, "MtLoad_ExportConfiguredReport", error) ||
        !Resolve(getLastError_, "MtLoad_GetLastError", error) || !Resolve(destroy_, "MtLoad_Destroy", error))
    {
        Unload();
        return false;
    }
#endif
    if (getAbiVersion_() != MTLOAD_ABI_VERSION)
    {
        error = L"MtLoadEngine ABI 版本不匹配";
        Unload();
        return false;
    }
    MTLOAD_ENGINE_OPTIONS options{};
    options.structSize = sizeof(options);
    if (initOptions_(&options) != MTLOAD_OK || create_(&options, &handle_) != MTLOAD_OK)
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
    if (handle_ != nullptr && destroy_ != nullptr)
    {
        if (stop_ != nullptr) stop_(handle_);
        if (wait_ != nullptr) wait_(handle_, UINT32_MAX);
        destroy_(handle_);
        handle_ = nullptr;
    }
    if (module_ != nullptr)
    {
        FreeLibrary(module_);
        module_ = nullptr;
    }
}

// 关键实现：宽路径只转换一次 UTF-8，加载或校验失败均读取 DLL 最近错误。
bool CLoadEngineClient::LoadAndValidatePlan(const std::wstring& planPath, std::wstring& error)
{
    if (handle_ == nullptr)
    {
        error = L"压测引擎尚未加载";
        return false;
    }
    const std::string path = ToUtf8(planPath);
    int result = loadPlan_(handle_, path.c_str());
    if (result == MTLOAD_OK) result = validatePlan_(handle_);
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
    if (handle_ == nullptr)
    {
        error = L"压测引擎尚未加载";
        return false;
    }
    const int result = start_(handle_, runName.c_str());
    if (result == MTLOAD_OK) return true;
    error = LastError();
    if (error.empty()) error = L"启动压测失败，错误码=" + std::to_wstring(result);
    return false;
}

// 关键实现：先查询所需数量，再初始化每行 ABI 头并执行第二次复制。
std::vector<MTLOAD_ENVIRONMENT> CLoadEngineClient::GetEnvironments() const
{
    if (handle_ == nullptr || getEnvironments_ == nullptr) return {};
    uint32_t count = 0;
    int result = getEnvironments_(handle_, nullptr, &count);
    if (result != MTLOAD_E_BUFFER_TOO_SMALL && result != MTLOAD_OK) return {};
    std::vector<MTLOAD_ENVIRONMENT> rows(count);
    if (count == 0) return rows;
    if (getEnvironments_(handle_, rows.data(), &count) != MTLOAD_OK) return {};
    rows.resize(count);
    return rows;
}

// 关键实现：转发稳定环境 ID，错误统一从 DLL 转换为宽字符串。
bool CLoadEngineClient::SelectEnvironment(const std::string& environmentId,
    std::wstring& error)
{
    if (handle_ == nullptr || selectEnvironment_ == nullptr)
    {
        error = L"压测引擎尚未加载";
        return false;
    }
    const int result = selectEnvironment_(handle_, environmentId.c_str());
    if (result == MTLOAD_OK) return true;
    error = LastError();
    if (error.empty()) error = L"选择运行环境失败，错误码=" + std::to_wstring(result);
    return false;
}

// 关键实现：遵循 ABI 两次调用模式取得完整场景摘要数组。
std::vector<MTLOAD_SCENARIO> CLoadEngineClient::GetScenarios() const
{
    if (handle_ == nullptr || getScenarios_ == nullptr) return {};
    uint32_t count = 0;
    int result = getScenarios_(handle_, nullptr, &count);
    if (result != MTLOAD_E_BUFFER_TOO_SMALL && result != MTLOAD_OK) return {};
    std::vector<MTLOAD_SCENARIO> rows(count);
    if (count == 0) return rows;
    if (getScenarios_(handle_, rows.data(), &count) != MTLOAD_OK) return {};
    rows.resize(count);
    return rows;
}

// 关键实现：空字符串保留“全部场景”语义，不在宿主层改写。
bool CLoadEngineClient::SelectScenario(const std::string& scenarioId,
    std::wstring& error)
{
    if (handle_ == nullptr || selectScenario_ == nullptr)
    {
        error = L"压测引擎尚未加载";
        return false;
    }
    const int result = selectScenario_(handle_, scenarioId.c_str());
    if (result == MTLOAD_OK) return true;
    error = LastError();
    if (error.empty()) error = L"选择压测场景失败，错误码=" + std::to_wstring(result);
    return false;
}

// 关键实现：只复制 DLL 提供的脱敏用户摘要，宿主不读取 XML 密码。
std::vector<MTLOAD_USER> CLoadEngineClient::GetUsers() const
{
    if (handle_ == nullptr || getUsers_ == nullptr) return {};
    uint32_t count = 0;
    int result = getUsers_(handle_, nullptr, &count);
    if (result != MTLOAD_E_BUFFER_TOO_SMALL && result != MTLOAD_OK) return {};
    std::vector<MTLOAD_USER> rows(count);
    if (count == 0) return rows;
    if (getUsers_(handle_, rows.data(), &count) != MTLOAD_OK) return {};
    rows.resize(count);
    return rows;
}

// 关键实现：转发统一用户覆盖 ID，空字符串恢复场景配置。
bool CLoadEngineClient::SelectUser(const std::string& userId, std::wstring& error)
{
    if (handle_ == nullptr || selectUser_ == nullptr)
    {
        error = L"压测引擎尚未加载";
        return false;
    }
    const int result = selectUser_(handle_, userId.c_str());
    if (result == MTLOAD_OK) return true;
    error = LastError();
    if (error.empty()) error = L"选择压测用户失败，错误码=" + std::to_wstring(result);
    return false;
}

// 关键实现：提交暂停命令但不等待在途请求结束。
bool CLoadEngineClient::Pause(std::wstring& error)
{
    if (handle_ == nullptr || pause_ == nullptr) { error = L"压测引擎尚未加载"; return false; }
    const int result = pause_(handle_);
    if (result == MTLOAD_OK) return true;
    error = LastError();
    if (error.empty()) error = L"暂停压测失败，错误码=" + std::to_wstring(result);
    return false;
}

// 关键实现：提交继续命令并保留当前 runId 和场景位置。
bool CLoadEngineClient::Resume(std::wstring& error)
{
    if (handle_ == nullptr || resume_ == nullptr) { error = L"压测引擎尚未加载"; return false; }
    const int result = resume_(handle_);
    if (result == MTLOAD_OK) return true;
    error = LastError();
    if (error.empty()) error = L"继续压测失败，错误码=" + std::to_wstring(result);
    return false;
}

// 关键实现：由 DLL 状态机拒绝运行中清理，宿主只转换错误文本。
bool CLoadEngineClient::ClearResults(std::wstring& error)
{
    if (handle_ == nullptr || clearResults_ == nullptr) { error = L"压测引擎尚未加载"; return false; }
    const int result = clearResults_(handle_);
    if (result == MTLOAD_OK) return true;
    error = LastError();
    if (error.empty()) error = L"清空结果失败，错误码=" + std::to_wstring(result);
    return false;
}

// 关键实现：句柄和函数入口有效时幂等提交停止，不阻塞 UI。
void CLoadEngineClient::Stop()
{
    if (handle_ != nullptr && stop_ != nullptr) stop_(handle_);
}

// 关键实现：每次调用重新填写结构大小和 ABI 版本，防止复用旧快照头部。
bool CLoadEngineClient::GetState(MTLOAD_RUN_STATE& state) const
{
    if (handle_ == nullptr) return false;
    state = {};
    state.structSize = sizeof(state);
    state.abiVersion = MTLOAD_ABI_VERSION;
    return getRunState_(handle_, &state) == MTLOAD_OK;
}

// 关键实现：两次调用间指标可能增长，容量不足时返回空集合并等待下一轮刷新。
std::vector<MTLOAD_METRIC_ROW> CLoadEngineClient::GetMetrics() const
{
    if (handle_ == nullptr) return {};
    uint32_t count = 0;
    int result = getMetricRows_(handle_, nullptr, &count);
    if (result != MTLOAD_E_BUFFER_TOO_SMALL && result != MTLOAD_OK) return {};
    std::vector<MTLOAD_METRIC_ROW> rows(count);
    if (count == 0) return rows;
    if (getMetricRows_(handle_, rows.data(), &count) != MTLOAD_OK) return {};
    rows.resize(count);
    return rows;
}

// 关键实现：按 ABI 头初始化每个错误样本，并把数组所有权留在宿主容器。
std::vector<MTLOAD_ERROR_SAMPLE> CLoadEngineClient::GetErrorSamples() const
{
    if (handle_ == nullptr || getErrorSamples_ == nullptr) return {};
    uint32_t count = 0;
    int result = getErrorSamples_(handle_, nullptr, &count);
    if (result != MTLOAD_E_BUFFER_TOO_SMALL && result != MTLOAD_OK) return {};
    std::vector<MTLOAD_ERROR_SAMPLE> rows(count);
    if (count == 0) return rows;
    if (getErrorSamples_(handle_, rows.data(), &count) != MTLOAD_OK) return {};
    rows.resize(count);
    return rows;
}

// 关键实现：使用固定最大路径缓冲区接收 DLL 生成的实际导出路径。
bool CLoadEngineClient::ExportConfiguredReport(uint32_t format,
    std::wstring& outputPath, std::wstring& error)
{
    if (handle_ == nullptr || exportConfiguredReport_ == nullptr)
    {
        error = L"压测引擎尚未加载";
        return false;
    }
    std::vector<char> path(32768);
    const int result = exportConfiguredReport_(handle_, format, path.data(),
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
    if (handle_ == nullptr || getLastError_ == nullptr) return {};
    uint32_t bytes = 0;
    getLastError_(handle_, nullptr, &bytes);
    std::vector<char> buffer(bytes == 0 ? 1 : bytes);
    if (getLastError_(handle_, buffer.data(), &bytes) != MTLOAD_OK) return {};
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
