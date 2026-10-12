#include "pch.h"
#include "MtLoadTest.h"
#include "MtLoadTestDlg.h"
#include "LogDlg.h"

#include <algorithm>
#include <array>
#include <cwctype>
#include <filesystem>
#include <iomanip>
#include <sstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
// 仅在启用状态确实变化时更新控件，避免定时快照导致按钮重复重绘和闪烁。
void EnableWindowIfChanged(CWnd* window, bool enabled)
{
    if (window != nullptr && (window->IsWindowEnabled() != FALSE) != enabled)
    {
        window->EnableWindow(enabled ? TRUE : FALSE);
    }
}

// 仅在文本确实变化时更新控件，保留焦点和选择状态并减少无效重绘。
void SetWindowTextIfChanged(CWnd* window, const wchar_t* text)
{
    if (window == nullptr) return;
    CString currentText;
    window->GetWindowText(currentText);
    if (currentText != text)
    {
        window->SetWindowText(text);
    }
}

// 按 1024 进位格式化累计字节数，所有单位统一保留两位小数。
CString FormatByteSize(uint64_t bytes)
{
    static constexpr const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB", L"PB", L"EB" };
    if (bytes < 1024)
    {
        CString value;
        value.Format(L"%.2f B", static_cast<double>(bytes));
        return value;
    }

    double converted = static_cast<double>(bytes);
    int unitIndex = 0;
    while (converted >= 1024.0 && unitIndex < static_cast<int>(_countof(units)) - 1)
    {
        converted /= 1024.0;
        ++unitIndex;
    }

    CString value;
    value.Format(L"%.2f %s", converted, units[unitIndex]);
    return value;
}
} // namespace

BEGIN_MESSAGE_MAP(CMtLoadTestDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BROWSE_PLAN, &CMtLoadTestDlg::OnBrowsePlan)
    ON_BN_CLICKED(IDC_VALIDATE_PLAN, &CMtLoadTestDlg::OnValidatePlan)
    ON_CBN_SELCHANGE(IDC_ENVIRONMENT, &CMtLoadTestDlg::OnEnvironmentChanged)
    ON_CBN_SELCHANGE(IDC_SCENARIO, &CMtLoadTestDlg::OnScenarioChanged)
    ON_CBN_SELCHANGE(IDC_USER, &CMtLoadTestDlg::OnUserChanged)
    ON_BN_CLICKED(IDC_START_RUN, &CMtLoadTestDlg::OnStartStopRun)
    ON_BN_CLICKED(IDC_PAUSE_RUN, &CMtLoadTestDlg::OnPauseResumeRun)
    ON_BN_CLICKED(IDC_CLEAR_RESULTS, &CMtLoadTestDlg::OnClearResults)
    ON_BN_CLICKED(IDC_EXPORT_RESULTS, &CMtLoadTestDlg::OnExportResults)
    ON_BN_CLICKED(IDC_LOG_BUTTON, &CMtLoadTestDlg::OnShowLog)
    ON_WM_CLOSE()
    ON_WM_SIZE()
    ON_WM_TIMER()
END_MESSAGE_MAP()

// 初始化压力测试对话框并绑定对应资源模板。
CMtLoadTestDlg::CMtLoadTestDlg() : CDialogEx(IDD_MTLOADTEST_DIALOG) {}

// 智能指针和客户端包装类负责释放其持有资源，无需额外析构逻辑。
CMtLoadTestDlg::~CMtLoadTestDlg() = default;

// 将资源控件与成员变量绑定，后续业务逻辑只通过成员访问界面状态。
void CMtLoadTestDlg::DoDataExchange(CDataExchange* exchange)
{
    CDialogEx::DoDataExchange(exchange);
    DDX_Control(exchange, IDC_PLAN_PATH, planPathEdit_);
    DDX_Control(exchange, IDC_ENVIRONMENT, environmentCombo_);
    DDX_Control(exchange, IDC_SCENARIO, scenarioCombo_);
    DDX_Control(exchange, IDC_USER, userCombo_);
    DDX_Control(exchange, IDC_RUN_STATE, runStateText_);
    DDX_Control(exchange, IDC_STATUS_TEXT, statusText_);
    DDX_Control(exchange, IDC_METRIC_LIST, metricList_);
}

// 关键实现：初始化日志、指标列和动态库客户端，并启动只读快照定时器。
BOOL CMtLoadTestDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    SetWindowText(L"MtLoadTest - HTTP/JSON 压测工具");
    mainIcon_ = AfxGetApp()->LoadIcon(IDI_MTLOADTEST_ICON);
    if (mainIcon_ != nullptr)
    {
        SetIcon(mainIcon_, TRUE);
        SetIcon(mainIcon_, FALSE);
    }

    std::wstring logError;
    const bool logReady = appLog_.Initialize(ExecutableDirectory(), logError);
    appLog_.Add(L"INFO", L"MtLoadTest 启动，日志文件=" + appLog_.FilePath());
    metricList_.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    const struct
    {
        const wchar_t* title; // 列标题。
        int width;            // 在 100% DPI 下的基准宽度，单位为像素。
    } columns[] = {
        {L"场景", 110}, {L"步骤", 100}, {L"接口", 205}, {L"已请求/总请求", 125},
        {L"进度", 70}, {L"成功", 65}, {L"失败", 65}, {L"平均(ms)", 85},
        {L"中位数(ms)", 90}, {L"P90(ms)", 80}, {L"P95(ms)", 80}, {L"P99(ms)", 80},
        {L"最小(ms)", 80}, {L"最大(ms)", 80}, {L"异常%", 75}, {L"吞吐(req/s)", 105},
        {L"接收(KB/s)", 95}, {L"发送(KB/s)", 95}, {L"请求大小", 105}, {L"应答大小", 105}
    };
    for (int i = 0; i < static_cast<int>(_countof(columns)); ++i)
        metricList_.InsertColumn(i, columns[i].title, LVCFMT_LEFT, columns[i].width);
    ResizeMetricColumns();

    std::wstring error;
    std::filesystem::path enginePath;
#if !defined(MTLOADTEST_STATIC_STACK)
    enginePath = std::filesystem::path(ExecutableDirectory()) /
#ifdef _DEBUG
        L"MtLoadEngine_d.dll";
#else
        L"MtLoadEngine.dll";
#endif
#endif
    engineLoaded_ = engine_.Load(enginePath.wstring(), error);
    if (engineLoaded_)
    {
        SetStatus(logReady ? L"压测引擎已加载，请选择 XML 计划" : (L"压测引擎已加载；" + logError).c_str());
#if defined(MTLOADTEST_STATIC_STACK)
        appLog_.Add(L"INFO", L"MtLoadEngine 静态库初始化成功，ABI=" + std::to_wstring(MTLOAD_ABI_VERSION));
#else
        appLog_.Add(L"INFO", L"MtLoadEngine.dll 加载成功，ABI=" + std::to_wstring(MTLOAD_ABI_VERSION));
#endif
    }
    else
    {
        SetStatus((L"引擎加载失败: " + error).c_str());
#if defined(MTLOADTEST_STATIC_STACK)
        appLog_.Add(L"ERROR", L"MtLoadEngine 静态库初始化失败: " + error);
#else
        appLog_.Add(L"ERROR", L"MtLoadEngine.dll 加载失败: " + error);
#endif
    }
    UpdateControls(MTLOAD_STATE_CREATED);
    SetTimer(kSnapshotTimer, 1000, nullptr);
    return TRUE;
}

// 通过当前可执行文件路径推导运行目录，配置、日志和动态库均以此为基准定位。
std::wstring CMtLoadTestDlg::ExecutableDirectory()
{
    std::wstring path(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    path.resize(length);
    return std::filesystem::path(path).parent_path().wstring();
}

// 对可能包含密码、Token 等敏感信息的诊断进行整句替换，防止写入普通日志。
std::wstring CMtLoadTestDlg::SanitizeDiagnostic(const std::wstring& value)
{
    std::wstring lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](wchar_t character) { return static_cast<wchar_t>(towlower(character)); });
    const wchar_t* sensitiveNames[] = {L"token", L"password", L"passwd", L"authorization", L"secret"};
    for (const wchar_t* name : sensitiveNames)
    {
        if (lower.find(name) != std::wstring::npos)
            return L"敏感字段相关诊断，详细值已隐藏";
    }
    return value;
}

// 将稳定的 C ABI 状态码转换为界面和日志共用的英文状态名称。
CString CMtLoadTestDlg::StateText(int state)
{
    switch (state)
    {
    case MTLOAD_STATE_CREATED: return L"CREATED";
    case MTLOAD_STATE_PLAN_LOADED: return L"PLAN_LOADED";
    case MTLOAD_STATE_VALIDATED: return L"VALIDATED";
    case MTLOAD_STATE_STARTING: return L"STARTING";
    case MTLOAD_STATE_RUNNING: return L"RUNNING";
    case MTLOAD_STATE_STOPPING: return L"STOPPING";
    case MTLOAD_STATE_COMPLETED: return L"COMPLETED";
    case MTLOAD_STATE_FAILED: return L"FAILED";
    case MTLOAD_STATE_PAUSING: return L"PAUSING";
    case MTLOAD_STATE_PAUSED: return L"PAUSED";
    default: return L"UNKNOWN";
    }
}

// 将引擎返回的 UTF-8 文本转换为 MFC 使用的 UTF-16 字符串。
CString CMtLoadTestDlg::Utf8Text(const char* value)
{
    if (value == nullptr || *value == '\0') return {};
    const int size = MultiByteToWideChar(CP_UTF8, 0, value, -1, nullptr, 0);
    CString result;
    wchar_t* buffer = result.GetBuffer(size);
    MultiByteToWideChar(CP_UTF8, 0, value, -1, buffer, size);
    result.ReleaseBuffer();
    return result;
}

// 更新窗口底部的用户可见状态说明。
void CMtLoadTestDlg::SetStatus(const CString& text)
{
    statusText_.SetWindowText(text);
}

// 关键实现：以引擎状态机为唯一依据批量更新按钮文字和可用状态。
void CMtLoadTestDlg::UpdateControls(int state)
{
    currentState_ = state;
    const bool active = state == MTLOAD_STATE_STARTING || state == MTLOAD_STATE_RUNNING ||
        state == MTLOAD_STATE_PAUSING || state == MTLOAD_STATE_PAUSED ||
        state == MTLOAD_STATE_STOPPING;
    const bool stopping = state == MTLOAD_STATE_STOPPING;
    const bool paused = state == MTLOAD_STATE_PAUSING || state == MTLOAD_STATE_PAUSED;

    EnableWindowIfChanged(GetDlgItem(IDC_BROWSE_PLAN), engineLoaded_ && !active);
    EnableWindowIfChanged(GetDlgItem(IDC_VALIDATE_PLAN), engineLoaded_ && !active);
    EnableWindowIfChanged(GetDlgItem(IDC_ENVIRONMENT),
        planValidated_ && !active && environments_.size() > 1);
    EnableWindowIfChanged(GetDlgItem(IDC_SCENARIO),
        planValidated_ && !active && !scenarios_.empty());
    EnableWindowIfChanged(GetDlgItem(IDC_USER),
        planValidated_ && !active && !users_.empty());
    SetWindowTextIfChanged(GetDlgItem(IDC_START_RUN), active ? L"终止" : L"开始");
    EnableWindowIfChanged(GetDlgItem(IDC_START_RUN), active ? !stopping : planValidated_);
    SetWindowTextIfChanged(GetDlgItem(IDC_PAUSE_RUN), paused ? L"继续" : L"暂停");
    EnableWindowIfChanged(GetDlgItem(IDC_PAUSE_RUN),
        state == MTLOAD_STATE_STARTING || state == MTLOAD_STATE_RUNNING || paused);
    EnableWindowIfChanged(GetDlgItem(IDC_CLEAR_RESULTS), engineLoaded_ && !active);
    EnableWindowIfChanged(GetDlgItem(IDC_EXPORT_RESULTS),
        engineLoaded_ && !active && hasExportableResults_);
}

// 选择新的 XML 计划并清除旧计划派生的环境、场景和用户选项。
void CMtLoadTestDlg::OnBrowsePlan()
{
    CFileDialog dialog(TRUE, L"xml", nullptr, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
        L"压测计划 (*.xml)|*.xml|所有文件 (*.*)|*.*||", this);
    if (dialog.DoModal() != IDOK) return;
    planPathEdit_.SetWindowText(dialog.GetPathName());
    planValidated_ = false;
    hasExportableResults_ = false;
    environments_.clear();
    scenarios_.clear();
    users_.clear();
    environmentCombo_.ResetContent();
    scenarioCombo_.ResetContent();
    userCombo_.ResetContent();
    UpdateControls(currentState_);
    SetStatus(L"已选择计划，等待校验");
    appLog_.Add(L"INFO", L"选择压测计划: " + std::wstring(dialog.GetPathName().GetString()));
}

// 关键实现：委托引擎重新读取并校验 XML，成功后一次性刷新所有可选配置。
void CMtLoadTestDlg::OnValidatePlan()
{
    CString path;
    planPathEdit_.GetWindowText(path);
    if (path.IsEmpty())
    {
        SetStatus(L"请先选择 XML 计划文件");
        appLog_.Add(L"WARN", L"计划校验被拒绝：尚未选择 XML 文件");
        return;
    }
    std::wstring error;
    planValidated_ = engine_.LoadAndValidatePlan(path.GetString(), error);
    SetStatus(planValidated_ ? L"计划校验通过" : (L"计划校验失败: " + error).c_str());
    if (planValidated_)
    {
        RefreshEnvironmentChoices();
        RefreshScenarioChoices();
        RefreshUserChoices();
        appLog_.Add(L"INFO", L"计划校验通过: " + std::wstring(path.GetString()));
    }
    else
    {
        environments_.clear();
        scenarios_.clear();
        users_.clear();
        environmentCombo_.ResetContent();
        scenarioCombo_.ResetContent();
        userCombo_.ResetContent();
        appLog_.Add(L"ERROR", L"计划校验失败: " + SanitizeDiagnostic(error));
    }
    RefreshSnapshot();
}

// 从引擎查询场景摘要，并在显示文本中给出线程、循环和预计请求数。
void CMtLoadTestDlg::RefreshScenarioChoices()
{
    scenarios_ = engine_.GetScenarios();
    scenarioCombo_.ResetContent();
    scenarioCombo_.AddString(L"全部场景");
    int selected = 0;
    for (int index = 0; index < static_cast<int>(scenarios_.size()); ++index)
    {
        const auto& scenario = scenarios_[index];
        CString text;
        if (scenario.plannedRequests == 0)
            text.Format(L"%s [%s]  线程=%u，循环=持续时间，单轮请求=%llu",
                Utf8Text(scenario.name).GetString(), Utf8Text(scenario.id).GetString(),
                scenario.threads, scenario.requestsPerIteration);
        else
            text.Format(L"%s [%s]  线程=%u，循环=%u，总请求=%llu",
                Utf8Text(scenario.name).GetString(), Utf8Text(scenario.id).GetString(),
                scenario.threads, scenario.iterations, scenario.plannedRequests);
        scenarioCombo_.AddString(text);
        if (scenario.selected != 0) selected = index + 1;
    }
    scenarioCombo_.SetCurSel(selected);
    UpdateControls(currentState_);
}

// 从引擎查询环境定义；统一展示 scheme://host[:port]/basePath，省略默认端口。
void CMtLoadTestDlg::RefreshEnvironmentChoices()
{
    environments_ = engine_.GetEnvironments();
    environmentCombo_.ResetContent();
    if (environments_.empty())
    {
        environmentCombo_.AddString(L"计划内固定地址");
        environmentCombo_.SetCurSel(0);
        UpdateControls(currentState_);
        return;
    }

    int selected = 0;
    for (int index = 0; index < static_cast<int>(environments_.size()); ++index)
    {
        const auto& environment = environments_[index];
        CString text;
        if (environment.port == 0)
            text.Format(L"%s [%s]  %s://%s%s", Utf8Text(environment.name).GetString(),
                Utf8Text(environment.id).GetString(), Utf8Text(environment.scheme).GetString(),
                Utf8Text(environment.host).GetString(),
                Utf8Text(environment.basePath).GetString());
        else
            text.Format(L"%s [%s]  %s://%s:%u%s", Utf8Text(environment.name).GetString(),
                Utf8Text(environment.id).GetString(), Utf8Text(environment.scheme).GetString(),
                Utf8Text(environment.host).GetString(),
                environment.port, Utf8Text(environment.basePath).GetString());
        environmentCombo_.AddString(text);
        if (environment.selected != 0) selected = index;
    }
    environmentCombo_.SetCurSel(selected);
    UpdateControls(currentState_);
}

// 将界面环境选择提交给引擎，并仅记录不含凭据的目标地址。
void CMtLoadTestDlg::OnEnvironmentChanged()
{
    const int index = environmentCombo_.GetCurSel();
    if (index < 0 || index >= static_cast<int>(environments_.size())) return;
    const auto& environment = environments_[index];
    std::wstring error;
    if (!engine_.SelectEnvironment(environment.id, error))
    {
        SetStatus((L"环境选择失败: " + error).c_str());
        appLog_.Add(L"ERROR", L"环境选择失败: " + SanitizeDiagnostic(error));
        RefreshEnvironmentChoices();
        return;
    }
    for (auto& item : environments_) item.selected = 0;
    environments_[index].selected = 1;
    const CString environmentName = Utf8Text(environment.name);
    SetStatus((L"已选择环境: " + std::wstring(environmentName.GetString())).c_str());
    std::wstring address = Utf8Text(environment.scheme).GetString();
    address += L"://";
    address += Utf8Text(environment.host).GetString();
    if (environment.port != 0) address += L":" + std::to_wstring(environment.port);
    address += Utf8Text(environment.basePath).GetString();
    appLog_.Add(L"INFO", L"选择运行环境: " + std::wstring(environmentName.GetString()) +
        L"，地址=" + address);
}

// 从引擎查询可选用户；密码始终不进入下拉文本和日志。
void CMtLoadTestDlg::RefreshUserChoices()
{
    users_ = engine_.GetUsers();
    userCombo_.ResetContent();
    userCombo_.AddString(L"按场景配置（使用 UserRefs）");
    int selected = 0;
    for (int index = 0; index < static_cast<int>(users_.size()); ++index)
    {
        const auto& user = users_[index];
        CString text;
        text.Format(L"%s [%s]  MT%s / 节点%s / Login=%s",
            Utf8Text(user.name).GetString(), Utf8Text(user.id).GetString(),
            Utf8Text(user.version).GetString(), Utf8Text(user.no).GetString(),
            Utf8Text(user.login).GetString());
        userCombo_.AddString(text);
        if (user.selected != 0) selected = index + 1;
    }
    userCombo_.SetCurSel(selected);
    UpdateControls(currentState_);
}

// 将场景筛选提交给引擎；首项空 ID 表示执行全部场景。
void CMtLoadTestDlg::OnScenarioChanged()
{
    const int index = scenarioCombo_.GetCurSel();
    if (index < 0) return;
    const std::string scenarioId = index == 0 ? "" : scenarios_[index - 1].id;
    std::wstring error;
    if (!engine_.SelectScenario(scenarioId, error))
    {
        SetStatus((L"场景选择失败: " + error).c_str());
        appLog_.Add(L"ERROR", L"场景选择失败: " + SanitizeDiagnostic(error));
        RefreshScenarioChoices();
        return;
    }
    for (auto& item : scenarios_) item.selected = 0;
    if (index > 0) scenarios_[index - 1].selected = 1;
    const CString name = index == 0 ? CString(L"全部场景") : Utf8Text(scenarios_[index - 1].name);
    SetStatus((L"已选择压测场景: " + std::wstring(name.GetString())).c_str());
    appLog_.Add(L"INFO", L"选择压测场景: " + std::wstring(name.GetString()));
    RefreshSnapshot();
}

// 将用户覆盖选择提交给引擎；首项空 ID 表示恢复场景 UserRefs 分配规则。
void CMtLoadTestDlg::OnUserChanged()
{
    const int index = userCombo_.GetCurSel();
    if (index < 0) return;
    const std::string userId = index == 0 ? "" : users_[index - 1].id;
    std::wstring error;
    if (!engine_.SelectUser(userId, error))
    {
        SetStatus((L"用户选择失败: " + error).c_str());
        appLog_.Add(L"ERROR", L"用户选择失败: " + SanitizeDiagnostic(error));
        RefreshUserChoices();
        return;
    }
    for (auto& item : users_) item.selected = 0;
    if (index > 0) users_[index - 1].selected = 1;
    if (index == 0)
    {
        SetStatus(L"已恢复按场景 UserRefs 分配用户");
        appLog_.Add(L"INFO", L"压测用户: 按场景 UserRefs 分配");
        return;
    }
    const auto& user = users_[index - 1];
    CString message;
    message.Format(L"已选择压测用户: %s，MT%s/%s，Login=%s",
        Utf8Text(user.name).GetString(), Utf8Text(user.version).GetString(),
        Utf8Text(user.no).GetString(), Utf8Text(user.login).GetString());
    SetStatus(message);
    appLog_.Add(L"INFO", message.GetString());
}

// 关键实现：同一按钮根据状态执行异步启动或请求终止，不在界面线程等待请求完成。
void CMtLoadTestDlg::OnStartStopRun()
{
    if (currentState_ == MTLOAD_STATE_STARTING || currentState_ == MTLOAD_STATE_RUNNING ||
        currentState_ == MTLOAD_STATE_PAUSING || currentState_ == MTLOAD_STATE_PAUSED)
    {
        engine_.Stop();
        SetStatus(L"已请求终止，等待在途请求结束");
        appLog_.Add(L"WARN", L"已请求终止压测；结束后可修改 XML 并重新加载");
        UpdateControls(MTLOAD_STATE_STOPPING);
        return;
    }

    std::wstring error;
    if (!engine_.Start("mfc", error))
    {
        SetStatus((L"启动失败: " + error).c_str());
        appLog_.Add(L"ERROR", L"压测启动失败: " + SanitizeDiagnostic(error));
        return;
    }
    UpdateControls(MTLOAD_STATE_STARTING);
    SetStatus(L"压测已异步启动");
    appLog_.Add(L"INFO", L"已提交异步压测启动请求");
    RefreshSnapshot();
}

// 根据当前暂停状态调用 Pause 或 Resume，并立即反映过渡态避免重复点击。
void CMtLoadTestDlg::OnPauseResumeRun()
{
    std::wstring error;
    const bool resume = currentState_ == MTLOAD_STATE_PAUSING || currentState_ == MTLOAD_STATE_PAUSED;
    const bool succeeded = resume ? engine_.Resume(error) : engine_.Pause(error);
    if (!succeeded)
    {
        SetStatus(((resume ? L"继续失败: " : L"暂停失败: ") + error).c_str());
        appLog_.Add(L"ERROR", (resume ? L"继续压测失败: " : L"暂停压测失败: ") +
            SanitizeDiagnostic(error));
        return;
    }
    SetStatus(resume ? L"已继续当前压测" : L"正在暂停；等待在途请求完成");
    appLog_.Add(L"INFO", resume ? L"已继续当前压测" : L"已请求暂停压测");
    UpdateControls(resume ? MTLOAD_STATE_RUNNING : MTLOAD_STATE_PAUSING);
    RefreshSnapshot();
}

// 清空引擎结果和界面行，同时重置错误样本游标及导出资格。
void CMtLoadTestDlg::OnClearResults()
{
    std::wstring error;
    if (!engine_.ClearResults(error))
    {
        SetStatus((L"清空失败: " + error).c_str());
        appLog_.Add(L"ERROR", L"清空压测结果失败: " + SanitizeDiagnostic(error));
        return;
    }
    metricList_.DeleteAllItems();
    hasExportableResults_ = false;
    observedErrorCount_ = 0;
    appLog_.Add(L"INFO", L"已清空指标和错误样本");
    RefreshSnapshot();
    SetStatus(L"压测结果已清空");
}

// 通过弹出菜单选择 XML 或 XLSX，再委托引擎按计划中的目录规则生成报告。
void CMtLoadTestDlg::OnExportResults()
{
    CMenu menu;
    if (!menu.CreatePopupMenu()) return;
    menu.AppendMenuW(MF_STRING, ID_EXPORT_XLSX, L"导出 Excel 表格 (*.xlsx)");
    menu.AppendMenuW(MF_STRING, ID_EXPORT_XML, L"导出 XML 列表 (*.xml)");
    CRect rectangle;
    GetDlgItem(IDC_EXPORT_RESULTS)->GetWindowRect(&rectangle);
    const UINT command = menu.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN,
        rectangle.left, rectangle.bottom, this);
    if (command != ID_EXPORT_XLSX && command != ID_EXPORT_XML) return;

    const uint32_t format = command == ID_EXPORT_XLSX ? MTLOAD_REPORT_XLSX : MTLOAD_REPORT_XML;
    std::wstring outputPath;
    std::wstring error;
    if (!engine_.ExportConfiguredReport(format, outputPath, error))
    {
        SetStatus((L"导出失败: " + error).c_str());
        appLog_.Add(L"ERROR", L"导出压测结果失败: " + SanitizeDiagnostic(error));
        return;
    }
    hasExportableResults_ = false;
    SetStatus((L"导出完成: " + outputPath).c_str());
    appLog_.Add(L"INFO", L"压测结果已导出: " + outputPath);
}

// 延迟创建非模态日志窗口；重复点击时复用现有窗口并置前。
void CMtLoadTestDlg::OnShowLog()
{
    if (!logDialog_)
    {
        logDialog_ = std::make_unique<CLogDlg>(appLog_, this);
        if (!logDialog_->Create(IDD_LOG_DIALOG, this))
        {
            logDialog_.reset();
            SetStatus(L"无法创建日志窗口");
            appLog_.Add(L"ERROR", L"创建日志窗口失败");
            return;
        }
    }
    logDialog_->ShowWindow(SW_SHOWNORMAL);
    logDialog_->SetForegroundWindow();
}

// 每秒读取一次引擎快照；其他定时器事件仍交还基类处理。
void CMtLoadTestDlg::OnTimer(UINT_PTR eventId)
{
    if (eventId == kSnapshotTimer) RefreshSnapshot();
    CDialogEx::OnTimer(eventId);
}

// 窗口尺寸变化后重新铺满指标列，避免列表右侧出现无标题空列。
void CMtLoadTestDlg::OnSize(UINT type, int cx, int cy)
{
    CDialogEx::OnSize(type, cx, cy);
    ResizeMetricColumns();
}

// 按基准宽度同比放大各列，并把舍入余量全部交给最后一列。
void CMtLoadTestDlg::ResizeMetricColumns()
{
    if (!::IsWindow(metricList_.GetSafeHwnd())) return;
    const CHeaderCtrl* header = metricList_.GetHeaderCtrl();
    if (header == nullptr || header->GetItemCount() != 20) return;

    // 这些宽度保证标题完整；可视区域更宽时按相同比例铺满，不留下无标题空白区。
    static constexpr std::array<int, 20> baseWidths = {
        110, 100, 205, 125, 70, 65, 65, 85, 90, 80,
        80, 80, 80, 80, 75, 105, 95, 95, 105, 105
    };
    constexpr int baseTotal = 1895;

    CRect client;
    metricList_.GetClientRect(&client);
    const int available = client.Width();
    if (available <= 0) return;

    if (available <= baseTotal)
    {
        for (int index = 0; index < static_cast<int>(baseWidths.size()); ++index)
            metricList_.SetColumnWidth(index, baseWidths[index]);
        return;
    }

    int assigned = 0;
    for (int index = 0; index < static_cast<int>(baseWidths.size()) - 1; ++index)
    {
        const int width = MulDiv(baseWidths[index], available, baseTotal);
        metricList_.SetColumnWidth(index, width);
        assigned += width;
    }
    metricList_.SetColumnWidth(static_cast<int>(baseWidths.size()) - 1, available - assigned);
}

// 关键实现：读取状态、指标和错误快照，并以增量方式更新列表以保留用户选择及滚动位置。
void CMtLoadTestDlg::RefreshSnapshot()
{
    if (!engineLoaded_) return;
    MTLOAD_RUN_STATE state{};
    if (!engine_.GetState(state)) return;
    hasExportableResults_ = state.totalSamples > 0 &&
        (state.state == MTLOAD_STATE_COMPLETED || state.state == MTLOAD_STATE_FAILED);
    UpdateControls(state.state);
    if (state.runId != observedRunId_)
    {
        observedRunId_ = state.runId;
        observedErrorCount_ = 0;
        lastLoggedState_ = -1;
    }
    CString stateLabel;
    CString requestProgress;
    if (state.plannedRequests == 0)
        requestProgress.Format(L"%llu/?", state.totalSamples);
    else
        requestProgress.Format(L"%llu/%llu (%.2f%%)", state.totalSamples, state.plannedRequests,
            100.0 * static_cast<double>(state.totalSamples) / state.plannedRequests);
    stateLabel.Format(L"状态: %s    请求: %s    活动用户: %u    在途请求: %u",
        StateText(state.state).GetString(), requestProgress.GetString(), state.activeUsers, state.activeRequests);
    SetWindowTextIfChanged(&runStateText_, stateLabel);

    const auto metrics = engine_.GetMetrics();
    bool listChanged = false;
    // 首次检测到变化时才暂停重绘，整批更新结束后一次性恢复显示。
    const auto beginListUpdate = [&]()
    {
        if (listChanged) return;
        metricList_.SetRedraw(FALSE);
        listChanged = true;
    };
    // 单元格值不变时不调用 SetItemText，避免列表控件清除当前选择高亮。
    const auto setItemTextIfChanged = [&](int item, int subItem, const CString& text)
    {
        if (metricList_.GetItemText(item, subItem) == text) return;
        beginListUpdate();
        metricList_.SetItemText(item, subItem, text);
    };

    // 保留现有 ListView 行对象，避免一秒快照刷新销毁用户的选中行和滚动位置。
    while (metricList_.GetItemCount() > static_cast<int>(metrics.size()))
    {
        beginListUpdate();
        metricList_.DeleteItem(metricList_.GetItemCount() - 1);
    }
    for (int index = 0; index < static_cast<int>(metrics.size()); ++index)
    {
        const auto& row = metrics[index];
        const CString scenario = Utf8Text(row.scenario);
        if (index >= metricList_.GetItemCount())
        {
            beginListUpdate();
            metricList_.InsertItem(index, scenario);
        }
        else
        {
            setItemTextIfChanged(index, 0, scenario);
        }
        setItemTextIfChanged(index, 1, Utf8Text(row.step));
        setItemTextIfChanged(index, 2, Utf8Text(row.label));
        CString value;
        if (row.plannedRequests == 0) value.Format(L"%llu/?", row.samples);
        else value.Format(L"%llu/%llu", row.samples, row.plannedRequests);
        setItemTextIfChanged(index, 3, value);
        if (row.plannedRequests == 0) value = L"-";
        else value.Format(L"%.2f%%", 100.0 * static_cast<double>(row.samples) / row.plannedRequests);
        setItemTextIfChanged(index, 4, value);
        value.Format(L"%llu", row.succeeded); setItemTextIfChanged(index, 5, value);
        value.Format(L"%llu", row.failed); setItemTextIfChanged(index, 6, value);
        value.Format(L"%.2f", row.averageMs); setItemTextIfChanged(index, 7, value);
        value.Format(L"%.2f", row.medianMs); setItemTextIfChanged(index, 8, value);
        value.Format(L"%.2f", row.p90Ms); setItemTextIfChanged(index, 9, value);
        value.Format(L"%.2f", row.p95Ms); setItemTextIfChanged(index, 10, value);
        value.Format(L"%.2f", row.p99Ms); setItemTextIfChanged(index, 11, value);
        value.Format(L"%.2f", row.minMs); setItemTextIfChanged(index, 12, value);
        value.Format(L"%.2f", row.maxMs); setItemTextIfChanged(index, 13, value);
        value.Format(L"%.2f%%", row.errorRate * 100.0); setItemTextIfChanged(index, 14, value);
        value.Format(L"%.2f", row.throughputPerSec); setItemTextIfChanged(index, 15, value);
        value.Format(L"%.2f", row.receivedKiBPerSec); setItemTextIfChanged(index, 16, value);
        value.Format(L"%.2f", row.sentKiBPerSec); setItemTextIfChanged(index, 17, value);
        setItemTextIfChanged(index, 18, FormatByteSize(row.sentBytes));
        setItemTextIfChanged(index, 19, FormatByteSize(row.receivedBytes));
    }
    if (listChanged)
    {
        metricList_.SetRedraw(TRUE);
        metricList_.Invalidate(FALSE);
    }

    LogEngineErrors();
    if (state.state != lastLoggedState_)
    {
        appLog_.Add(L"INFO", L"运行状态变更: " + std::wstring(StateText(state.state).GetString()) +
            L"，runId=" + std::to_wstring(state.runId) +
            L"，samples=" + std::to_wstring(state.totalSamples));
        lastLoggedState_ = state.state;
    }

    const bool terminal = state.state == MTLOAD_STATE_COMPLETED || state.state == MTLOAD_STATE_FAILED;
    if (terminal)
    {
        if (state.state == MTLOAD_STATE_FAILED)
        {
            const std::wstring error = engine_.LastError();
            SetStatus((L"运行失败: " + error).c_str());
            appLog_.Add(L"ERROR", L"本轮压测失败: " + SanitizeDiagnostic(error));
        }
        else SetStatus(L"本轮压测已完成");
        LogTerminalSummary(state, metrics);
    }
}

// 统一复用取消流程，确保标题栏关闭和取消键具有相同清理语义。
void CMtLoadTestDlg::OnClose()
{
    OnCancel();
}

// 只追加本轮尚未记录的错误样本，并在写日志前统一执行敏感信息脱敏。
void CMtLoadTestDlg::LogEngineErrors()
{
    const auto errors = engine_.GetErrorSamples();
    if (errors.size() < observedErrorCount_) observedErrorCount_ = 0;
    for (size_t index = observedErrorCount_; index < errors.size(); ++index)
    {
        const auto& sample = errors[index];
        CString message;
        const std::wstring summary = SanitizeDiagnostic(Utf8Text(sample.summary).GetString());
        message.Format(L"接口失败: 场景=%s，步骤=%s，接口=%s，分类=%s，result=%d，http=%d，耗时=%.2fms，原因=%s",
            Utf8Text(sample.scenario).GetString(), Utf8Text(sample.step).GetString(),
            Utf8Text(sample.label).GetString(), Utf8Text(sample.category).GetString(),
            sample.result, sample.httpStatus, sample.elapsedMs, summary.c_str());
        appLog_.Add(L"ERROR", message.GetString());
    }
    observedErrorCount_ = errors.size();
}

// 每个 runId 只记录一次终态摘要，避免定时器持续重复输出相同汇总。
void CMtLoadTestDlg::LogTerminalSummary(const MTLOAD_RUN_STATE& state,
    const std::vector<MTLOAD_METRIC_ROW>& metrics)
{
    if (state.runId == 0 || summarizedRunId_ == state.runId) return;
    summarizedRunId_ = state.runId;

    appLog_.Add(state.state == MTLOAD_STATE_COMPLETED ? L"INFO" : L"ERROR",
        L"本轮结束: runId=" + std::to_wstring(state.runId) +
        L"，state=" + std::wstring(StateText(state.state).GetString()) +
        L"，samples=" + std::to_wstring(state.totalSamples));
    for (const auto& row : metrics)
    {
        CString message;
        message.Format(L"接口汇总: %s %s，场景=%s，步骤=%s，样本=%llu，成功=%llu，失败=%llu，平均=%.2fms，P95=%.2fms，吞吐=%.2freq/s",
            Utf8Text(row.method).GetString(), Utf8Text(row.path).GetString(),
            Utf8Text(row.scenario).GetString(), Utf8Text(row.step).GetString(),
            row.samples, row.succeeded, row.failed, row.averageMs, row.p95Ms, row.throughputPerSec);
        appLog_.Add(row.failed == 0 ? L"INFO" : L"WARN", message.GetString());
    }
}

// 关键实现：停止快照定时器、销毁日志窗并卸载引擎，待后台线程回收后再退出对话框。
void CMtLoadTestDlg::OnCancel()
{
    if (closing_) return;
    closing_ = true;
    KillTimer(kSnapshotTimer);
    appLog_.Add(L"INFO", L"MtLoadTest 正在退出，停止并回收压测线程");
    if (logDialog_ && ::IsWindow(logDialog_->GetSafeHwnd())) logDialog_->DestroyWindow();
    logDialog_.reset();
    engine_.Unload();
    appLog_.Add(L"INFO", L"MtLoadTest 压测线程已回收，进程退出");
    CDialogEx::OnCancel();
}

