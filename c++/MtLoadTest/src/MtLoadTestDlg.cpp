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
    DDX_Control(exchange, IDC_PLAN_PATH, m_clPlanPathEdit);
    DDX_Control(exchange, IDC_ENVIRONMENT, m_clEnvironmentCombo);
    DDX_Control(exchange, IDC_SCENARIO, m_clScenarioCombo);
    DDX_Control(exchange, IDC_USER, m_clUserCombo);
    DDX_Control(exchange, IDC_RUN_STATE, m_clRunStateText);
    DDX_Control(exchange, IDC_STATUS_TEXT, m_clStatusText);
    DDX_Control(exchange, IDC_METRIC_LIST, m_clMetricList);
    DDX_Control(exchange, IDC_TRANSACTION_LIST, m_clTransactionList);
}

// 关键实现：初始化日志、指标列和动态库客户端，并启动只读快照定时器。
BOOL CMtLoadTestDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    SetWindowText(L"MtLoadTest - HTTP/JSON 压测工具");
    m_hMainIcon = AfxGetApp()->LoadIcon(IDI_MTLOADTEST_ICON);
    if (m_hMainIcon != nullptr)
    {
        SetIcon(m_hMainIcon, TRUE);
        SetIcon(m_hMainIcon, FALSE);
    }

    std::wstring logError;
    const bool logReady = m_clAppLog.Initialize(ExecutableDirectory(), logError);
    m_clAppLog.Add(L"INFO", L"MtLoadTest 启动，日志文件=" + m_clAppLog.FilePath());
    m_clMetricList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    m_clTransactionList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    const struct
    {
        const wchar_t* title;
        int width;
    } transactionColumns[] = {
        {L"业务事务", 180}, {L"启动", 70}, {L"完成", 70}, {L"成功", 70}, {L"失败", 70},
        {L"丢弃", 70}, {L"平均(ms)", 90}, {L"P50(ms)", 85}, {L"P90(ms)", 85},
        {L"P95(ms)", 85}, {L"P99(ms)", 85}, {L"事务吞吐(tx/s)", 120}
    };
    for (int i = 0; i < static_cast<int>(_countof(transactionColumns)); ++i)
        m_clTransactionList.InsertColumn(i, transactionColumns[i].title, LVCFMT_LEFT,
            transactionColumns[i].width);
    ResizeTransactionColumns();
    const struct
    {
        const wchar_t* title; // 列标题。
        int width;            // 在 100% DPI 下的基准宽度，单位为像素。
    } columns[] = {
        {L"场景", 110}, {L"步骤", 100}, {L"接口", 205}, {L"已请求/总请求", 125},
        {L"进度", 70}, {L"成功", 65}, {L"失败", 65}, {L"平均(ms)", 85},
        {L"中位数(ms)", 90}, {L"P90(ms)", 80}, {L"P95(ms)", 80}, {L"P99(ms)", 80},
        {L"最小(ms)", 80}, {L"最大(ms)", 80}, {L"异常%", 75}, {L"接口吞吐(全程 req/s)", 145},
        {L"接收(KB/s)", 95}, {L"发送(KB/s)", 95}, {L"请求大小", 105}, {L"应答大小", 105}
    };
    for (int i = 0; i < static_cast<int>(_countof(columns)); ++i)
        m_clMetricList.InsertColumn(i, columns[i].title, LVCFMT_LEFT, columns[i].width);
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
    m_bEngineLoaded = m_clEngine.Load(enginePath.wstring(), error);
    if (m_bEngineLoaded)
    {
        SetStatus(logReady ? L"压测引擎已加载，请选择 XML 计划" : (L"压测引擎已加载；" + logError).c_str());
#if defined(MTLOADTEST_STATIC_STACK)
        m_clAppLog.Add(L"INFO", L"MtLoadEngine 静态库初始化成功，ABI=" + std::to_wstring(MTLOAD_ABI_VERSION));
#else
        m_clAppLog.Add(L"INFO", L"MtLoadEngine.dll 加载成功，ABI=" + std::to_wstring(MTLOAD_ABI_VERSION));
#endif
    }
    else
    {
        SetStatus((L"引擎加载失败: " + error).c_str());
#if defined(MTLOADTEST_STATIC_STACK)
        m_clAppLog.Add(L"ERROR", L"MtLoadEngine 静态库初始化失败: " + error);
#else
        m_clAppLog.Add(L"ERROR", L"MtLoadEngine.dll 加载失败: " + error);
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
    m_clStatusText.SetWindowText(text);
}

// 关键实现：以引擎状态机为唯一依据批量更新按钮文字和可用状态。
void CMtLoadTestDlg::UpdateControls(int state)
{
    m_iCurrentState = state;
    const bool active = state == MTLOAD_STATE_STARTING || state == MTLOAD_STATE_RUNNING ||
        state == MTLOAD_STATE_PAUSING || state == MTLOAD_STATE_PAUSED ||
        state == MTLOAD_STATE_STOPPING;
    const bool stopping = state == MTLOAD_STATE_STOPPING;
    const bool paused = state == MTLOAD_STATE_PAUSING || state == MTLOAD_STATE_PAUSED;

    EnableWindowIfChanged(GetDlgItem(IDC_BROWSE_PLAN), m_bEngineLoaded && !active);
    EnableWindowIfChanged(GetDlgItem(IDC_VALIDATE_PLAN), m_bEngineLoaded && !active);
    EnableWindowIfChanged(GetDlgItem(IDC_ENVIRONMENT),
        m_bPlanValidated && !active && m_vecEnvironments.size() > 1);
    EnableWindowIfChanged(GetDlgItem(IDC_SCENARIO),
        m_bPlanValidated && !active && !m_vecWorkloads.empty());
    EnableWindowIfChanged(GetDlgItem(IDC_USER),
        m_bPlanValidated && !active && !m_vecUsers.empty());
    SetWindowTextIfChanged(GetDlgItem(IDC_START_RUN), active ? L"终止" : L"开始");
    EnableWindowIfChanged(GetDlgItem(IDC_START_RUN), active ? !stopping : m_bPlanValidated);
    SetWindowTextIfChanged(GetDlgItem(IDC_PAUSE_RUN), paused ? L"继续" : L"暂停");
    EnableWindowIfChanged(GetDlgItem(IDC_PAUSE_RUN),
        state == MTLOAD_STATE_STARTING || state == MTLOAD_STATE_RUNNING || paused);
    EnableWindowIfChanged(GetDlgItem(IDC_CLEAR_RESULTS), m_bEngineLoaded && !active);
    EnableWindowIfChanged(GetDlgItem(IDC_EXPORT_RESULTS),
        m_bEngineLoaded && !active && m_bHasExportableResults);
}

// 选择新的 XML 计划并清除旧计划派生的环境、场景和用户选项。
void CMtLoadTestDlg::OnBrowsePlan()
{
    CFileDialog dialog(TRUE, L"xml", nullptr, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
        L"压测计划 (*.xml)|*.xml|所有文件 (*.*)|*.*||", this);
    if (dialog.DoModal() != IDOK) return;
    m_clPlanPathEdit.SetWindowText(dialog.GetPathName());
    m_bPlanValidated = false;
    m_bHasExportableResults = false;
    m_vecEnvironments.clear();
    m_vecWorkloads.clear();
    m_vecUsers.clear();
    m_clEnvironmentCombo.ResetContent();
    m_clScenarioCombo.ResetContent();
    m_clUserCombo.ResetContent();
    UpdateControls(m_iCurrentState);
    SetStatus(L"已选择计划，等待校验");
    m_clAppLog.Add(L"INFO", L"选择压测计划: " + std::wstring(dialog.GetPathName().GetString()));
}

// 关键实现：委托引擎重新读取并校验 XML，成功后一次性刷新所有可选配置。
void CMtLoadTestDlg::OnValidatePlan()
{
    CString path;
    m_clPlanPathEdit.GetWindowText(path);
    if (path.IsEmpty())
    {
        SetStatus(L"请先选择 XML 计划文件");
        m_clAppLog.Add(L"WARN", L"计划校验被拒绝：尚未选择 XML 文件");
        return;
    }
    std::wstring error;
    m_bPlanValidated = m_clEngine.LoadAndValidatePlan(path.GetString(), error);
    SetStatus(m_bPlanValidated ? L"计划校验通过" : (L"计划校验失败: " + error).c_str());
    if (m_bPlanValidated)
    {
        RefreshEnvironmentChoices();
        RefreshScenarioChoices();
        RefreshUserChoices();
        m_clAppLog.Add(L"INFO", L"计划校验通过: " + std::wstring(path.GetString()));
    }
    else
    {
        m_vecEnvironments.clear();
        m_vecWorkloads.clear();
        m_vecUsers.clear();
        m_clEnvironmentCombo.ResetContent();
        m_clScenarioCombo.ResetContent();
        m_clUserCombo.ResetContent();
        m_clAppLog.Add(L"ERROR", L"计划校验失败: " + SanitizeDiagnostic(error));
    }
    RefreshSnapshot();
}

// 从引擎查询工作负载摘要；场景步骤不因负载曲线变化而复制。
void CMtLoadTestDlg::RefreshScenarioChoices()
{
    m_vecWorkloads = m_clEngine.GetWorkloads();
    m_clScenarioCombo.ResetContent();
    int selected = 0;
    for (int index = 0; index < static_cast<int>(m_vecWorkloads.size()); ++index)
    {
        const auto& workload = m_vecWorkloads[index];
        CString text;
        if (std::string(workload.mode) == "arrival")
            text.Format(L"%s [%s]  arrival %.2f tx/s，用户=%u..%u，阶段=%u，时长=%llums",
                Utf8Text(workload.name).GetString(), Utf8Text(workload.id).GetString(),
                workload.ratePerSec, workload.users, workload.maxUsers, workload.stageCount,
                workload.durationMs);
        else
            text.Format(L"%s [%s]  closed 用户=%u，场景=%u%s",
                Utf8Text(workload.name).GetString(), Utf8Text(workload.id).GetString(),
                workload.users, workload.scenarioCount, workload.implicit ? L"，兼容" : L"");
        m_clScenarioCombo.AddString(text);
        if (workload.selected != 0) selected = index;
    }
    m_clScenarioCombo.SetCurSel(selected);
    UpdateControls(m_iCurrentState);
}

// 从引擎查询环境定义；统一展示 scheme://host[:port]/basePath，省略默认端口。
void CMtLoadTestDlg::RefreshEnvironmentChoices()
{
    m_vecEnvironments = m_clEngine.GetEnvironments();
    m_clEnvironmentCombo.ResetContent();
    if (m_vecEnvironments.empty())
    {
        m_clEnvironmentCombo.AddString(L"计划内固定地址");
        m_clEnvironmentCombo.SetCurSel(0);
        UpdateControls(m_iCurrentState);
        return;
    }

    int selected = 0;
    for (int index = 0; index < static_cast<int>(m_vecEnvironments.size()); ++index)
    {
        const auto& environment = m_vecEnvironments[index];
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
        m_clEnvironmentCombo.AddString(text);
        if (environment.selected != 0) selected = index;
    }
    m_clEnvironmentCombo.SetCurSel(selected);
    UpdateControls(m_iCurrentState);
}

// 将界面环境选择提交给引擎，并仅记录不含凭据的目标地址。
void CMtLoadTestDlg::OnEnvironmentChanged()
{
    const int index = m_clEnvironmentCombo.GetCurSel();
    if (index < 0 || index >= static_cast<int>(m_vecEnvironments.size())) return;
    const auto& environment = m_vecEnvironments[index];
    std::wstring error;
    if (!m_clEngine.SelectEnvironment(environment.id, error))
    {
        SetStatus((L"环境选择失败: " + error).c_str());
        m_clAppLog.Add(L"ERROR", L"环境选择失败: " + SanitizeDiagnostic(error));
        RefreshEnvironmentChoices();
        return;
    }
    for (auto& item : m_vecEnvironments) item.selected = 0;
    m_vecEnvironments[index].selected = 1;
    const CString environmentName = Utf8Text(environment.name);
    SetStatus((L"已选择环境: " + std::wstring(environmentName.GetString())).c_str());
    std::wstring address = Utf8Text(environment.scheme).GetString();
    address += L"://";
    address += Utf8Text(environment.host).GetString();
    if (environment.port != 0) address += L":" + std::to_wstring(environment.port);
    address += Utf8Text(environment.basePath).GetString();
    m_clAppLog.Add(L"INFO", L"选择运行环境: " + std::wstring(environmentName.GetString()) +
        L"，地址=" + address);
}

// 从引擎查询可选用户；密码始终不进入下拉文本和日志。
void CMtLoadTestDlg::RefreshUserChoices()
{
    m_vecUsers = m_clEngine.GetUsers();
    m_clUserCombo.ResetContent();
    m_clUserCombo.AddString(L"按场景配置（使用 UserRefs）");
    int selected = 0;
    for (int index = 0; index < static_cast<int>(m_vecUsers.size()); ++index)
    {
        const auto& user = m_vecUsers[index];
        CString text;
        text.Format(L"%s [%s]  MT%s / 节点%s / Login=%s",
            Utf8Text(user.name).GetString(), Utf8Text(user.id).GetString(),
            Utf8Text(user.version).GetString(), Utf8Text(user.no).GetString(),
            Utf8Text(user.login).GetString());
        m_clUserCombo.AddString(text);
        if (user.selected != 0) selected = index + 1;
    }
    m_clUserCombo.SetCurSel(selected);
    UpdateControls(m_iCurrentState);
}

// 将工作负载选择提交给引擎；负载曲线和场景权重由 DLL 统一解释。
void CMtLoadTestDlg::OnScenarioChanged()
{
    const int index = m_clScenarioCombo.GetCurSel();
    if (index < 0 || index >= static_cast<int>(m_vecWorkloads.size())) return;
    const std::string workloadId = m_vecWorkloads[index].id;
    std::wstring error;
    if (!m_clEngine.SelectWorkload(workloadId, error))
    {
        SetStatus((L"工作负载选择失败: " + error).c_str());
        m_clAppLog.Add(L"ERROR", L"工作负载选择失败: " + SanitizeDiagnostic(error));
        RefreshScenarioChoices();
        return;
    }
    for (auto& item : m_vecWorkloads) item.selected = 0;
    m_vecWorkloads[index].selected = 1;
    const CString name = Utf8Text(m_vecWorkloads[index].name);
    SetStatus((L"已选择工作负载: " + std::wstring(name.GetString())).c_str());
    m_clAppLog.Add(L"INFO", L"选择工作负载: " + std::wstring(name.GetString()));
    RefreshSnapshot();
}

// 将用户覆盖选择提交给引擎；首项空 ID 表示恢复场景 UserRefs 分配规则。
void CMtLoadTestDlg::OnUserChanged()
{
    const int index = m_clUserCombo.GetCurSel();
    if (index < 0) return;
    const std::string userId = index == 0 ? "" : m_vecUsers[index - 1].id;
    std::wstring error;
    if (!m_clEngine.SelectUser(userId, error))
    {
        SetStatus((L"用户选择失败: " + error).c_str());
        m_clAppLog.Add(L"ERROR", L"用户选择失败: " + SanitizeDiagnostic(error));
        RefreshUserChoices();
        return;
    }
    for (auto& item : m_vecUsers) item.selected = 0;
    if (index > 0) m_vecUsers[index - 1].selected = 1;
    if (index == 0)
    {
        SetStatus(L"已恢复按场景 UserRefs 分配用户");
        m_clAppLog.Add(L"INFO", L"压测用户: 按场景 UserRefs 分配");
        return;
    }
    const auto& user = m_vecUsers[index - 1];
    CString message;
    message.Format(L"已选择压测用户: %s，MT%s/%s，Login=%s",
        Utf8Text(user.name).GetString(), Utf8Text(user.version).GetString(),
        Utf8Text(user.no).GetString(), Utf8Text(user.login).GetString());
    SetStatus(message);
    m_clAppLog.Add(L"INFO", message.GetString());
}

// 关键实现：同一按钮根据状态执行异步启动或请求终止，不在界面线程等待请求完成。
void CMtLoadTestDlg::OnStartStopRun()
{
    if (m_iCurrentState == MTLOAD_STATE_STARTING || m_iCurrentState == MTLOAD_STATE_RUNNING ||
        m_iCurrentState == MTLOAD_STATE_PAUSING || m_iCurrentState == MTLOAD_STATE_PAUSED)
    {
        m_clEngine.Stop();
        SetStatus(L"已请求终止，等待在途请求结束");
        m_clAppLog.Add(L"WARN", L"已请求终止压测；结束后可修改 XML 并重新加载");
        UpdateControls(MTLOAD_STATE_STOPPING);
        return;
    }

    std::wstring error;
    if (!m_clEngine.Start("mfc", error))
    {
        SetStatus((L"启动失败: " + error).c_str());
        m_clAppLog.Add(L"ERROR", L"压测启动失败: " + SanitizeDiagnostic(error));
        return;
    }
    UpdateControls(MTLOAD_STATE_STARTING);
    SetStatus(L"压测已异步启动");
    m_clAppLog.Add(L"INFO", L"已提交异步压测启动请求");
    RefreshSnapshot();
}

// 根据当前暂停状态调用 Pause 或 Resume，并立即反映过渡态避免重复点击。
void CMtLoadTestDlg::OnPauseResumeRun()
{
    std::wstring error;
    const bool resume = m_iCurrentState == MTLOAD_STATE_PAUSING || m_iCurrentState == MTLOAD_STATE_PAUSED;
    const bool succeeded = resume ? m_clEngine.Resume(error) : m_clEngine.Pause(error);
    if (!succeeded)
    {
        SetStatus(((resume ? L"继续失败: " : L"暂停失败: ") + error).c_str());
        m_clAppLog.Add(L"ERROR", (resume ? L"继续压测失败: " : L"暂停压测失败: ") +
            SanitizeDiagnostic(error));
        return;
    }
    SetStatus(resume ? L"已继续当前压测" : L"正在暂停；等待在途请求完成");
    m_clAppLog.Add(L"INFO", resume ? L"已继续当前压测" : L"已请求暂停压测");
    UpdateControls(resume ? MTLOAD_STATE_RUNNING : MTLOAD_STATE_PAUSING);
    RefreshSnapshot();
}

// 清空引擎结果和界面行，同时重置错误样本游标及导出资格。
void CMtLoadTestDlg::OnClearResults()
{
    std::wstring error;
    if (!m_clEngine.ClearResults(error))
    {
        SetStatus((L"清空失败: " + error).c_str());
        m_clAppLog.Add(L"ERROR", L"清空压测结果失败: " + SanitizeDiagnostic(error));
        return;
    }
    m_clMetricList.DeleteAllItems();
    m_clTransactionList.DeleteAllItems();
    m_bHasExportableResults = false;
    m_szObservedErrorCount = 0;
    m_clAppLog.Add(L"INFO", L"已清空指标和错误样本");
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
    if (!m_clEngine.ExportConfiguredReport(format, outputPath, error))
    {
        SetStatus((L"导出失败: " + error).c_str());
        m_clAppLog.Add(L"ERROR", L"导出压测结果失败: " + SanitizeDiagnostic(error));
        return;
    }
    m_bHasExportableResults = false;
    SetStatus((L"导出完成: " + outputPath).c_str());
    m_clAppLog.Add(L"INFO", L"压测结果已导出: " + outputPath);
}

// 延迟创建非模态日志窗口；重复点击时复用现有窗口并置前。
void CMtLoadTestDlg::OnShowLog()
{
    if (!m_pLogDialog)
    {
        m_pLogDialog = std::make_unique<CLogDlg>(m_clAppLog, this);
        if (!m_pLogDialog->Create(IDD_LOG_DIALOG, this))
        {
            m_pLogDialog.reset();
            SetStatus(L"无法创建日志窗口");
            m_clAppLog.Add(L"ERROR", L"创建日志窗口失败");
            return;
        }
    }
    m_pLogDialog->ShowWindow(SW_SHOWNORMAL);
    m_pLogDialog->SetForegroundWindow();
}

// 每秒读取一次引擎快照；其他定时器事件仍交还基类处理。
void CMtLoadTestDlg::OnTimer(UINT_PTR eventId)
{
    if (eventId == kSnapshotTimer) RefreshSnapshot();
    CDialogEx::OnTimer(eventId);
}

// 窗口尺寸变化后重新铺满事务和指标列，避免列表右侧出现无标题空列。
void CMtLoadTestDlg::OnSize(UINT type, int cx, int cy)
{
    CDialogEx::OnSize(type, cx, cy);
    ResizeTransactionColumns();
    ResizeMetricColumns();
}

// 按基准宽度同比放大事务列；空间不足时保留横向滚动，空间充足时铺满列表。
void CMtLoadTestDlg::ResizeTransactionColumns()
{
    if (!::IsWindow(m_clTransactionList.GetSafeHwnd())) return;
    const CHeaderCtrl* header = m_clTransactionList.GetHeaderCtrl();
    if (header == nullptr || header->GetItemCount() != 12) return;

    // 业务事务名称优先保留阅读宽度，其余数值列保证标题不被截断。
    static constexpr std::array<int, 12> baseWidths = {
        260, 70, 70, 70, 70, 70, 90, 85, 85, 85, 85, 140
    };
    constexpr int baseTotal = 1180;

    CRect client;
    m_clTransactionList.GetClientRect(&client);
    const int available = client.Width();
    if (available <= 0) return;

    if (available <= baseTotal)
    {
        for (int index = 0; index < static_cast<int>(baseWidths.size()); ++index)
            m_clTransactionList.SetColumnWidth(index, baseWidths[index]);
        return;
    }

    int assigned = 0;
    for (int index = 0; index < static_cast<int>(baseWidths.size()) - 1; ++index)
    {
        const int width = MulDiv(baseWidths[index], available, baseTotal);
        m_clTransactionList.SetColumnWidth(index, width);
        assigned += width;
    }
    m_clTransactionList.SetColumnWidth(static_cast<int>(baseWidths.size()) - 1,
        available - assigned);
}

// 按基准宽度同比放大各列，并把舍入余量全部交给最后一列。
void CMtLoadTestDlg::ResizeMetricColumns()
{
    if (!::IsWindow(m_clMetricList.GetSafeHwnd())) return;
    const CHeaderCtrl* header = m_clMetricList.GetHeaderCtrl();
    if (header == nullptr || header->GetItemCount() != 20) return;

    // 这些宽度保证标题完整；可视区域更宽时按相同比例铺满，不留下无标题空白区。
    static constexpr std::array<int, 20> baseWidths = {
        110, 100, 205, 125, 70, 65, 65, 85, 90, 80,
        80, 80, 80, 80, 75, 105, 95, 95, 105, 105
    };
    constexpr int baseTotal = 1895;

    CRect client;
    m_clMetricList.GetClientRect(&client);
    const int available = client.Width();
    if (available <= 0) return;

    if (available <= baseTotal)
    {
        for (int index = 0; index < static_cast<int>(baseWidths.size()); ++index)
            m_clMetricList.SetColumnWidth(index, baseWidths[index]);
        return;
    }

    int assigned = 0;
    for (int index = 0; index < static_cast<int>(baseWidths.size()) - 1; ++index)
    {
        const int width = MulDiv(baseWidths[index], available, baseTotal);
        m_clMetricList.SetColumnWidth(index, width);
        assigned += width;
    }
    m_clMetricList.SetColumnWidth(static_cast<int>(baseWidths.size()) - 1, available - assigned);
}

// 关键实现：读取状态、指标和错误快照，并以增量方式更新列表以保留用户选择及滚动位置。
void CMtLoadTestDlg::RefreshSnapshot()
{
    if (!m_bEngineLoaded) return;
    MTLOAD_RUN_STATE state{};
    if (!m_clEngine.GetState(state)) return;
    m_bHasExportableResults = state.totalSamples > 0 &&
        (state.state == MTLOAD_STATE_COMPLETED || state.state == MTLOAD_STATE_FAILED);
    UpdateControls(state.state);
    if (state.runId != m_u64ObservedRunId)
    {
        m_u64ObservedRunId = state.runId;
        m_szObservedErrorCount = 0;
        m_iLastLoggedState = -1;
    }
    CString stateLabel;
    CString requestProgress;
    if (state.plannedRequests == 0)
        requestProgress.Format(L"%llu/?", state.totalSamples);
    else
        requestProgress.Format(L"%llu/%llu (%.2f%%)", state.totalSamples, state.plannedRequests,
            100.0 * static_cast<double>(state.totalSamples) / state.plannedRequests);
    stateLabel.Format(L"状态: %s  请求: %s  吞吐: %.2f req/s  用户: %u/%u  在途: %u/%u  事务: %llu/%llu  丢弃: %llu",
        StateText(state.state).GetString(), requestProgress.GetString(), state.totalThroughputPerSec,
        state.activeUsers, state.peakActiveUsers, state.activeRequests, state.peakActiveRequests,
        state.completedIterations, state.scheduledIterations, state.droppedIterations);
    SetWindowTextIfChanged(&m_clRunStateText, stateLabel);

    const auto metrics = m_clEngine.GetMetrics();
    bool listChanged = false;
    // 首次检测到变化时才暂停重绘，整批更新结束后一次性恢复显示。
    const auto beginListUpdate = [&]()
    {
        if (listChanged) return;
        m_clMetricList.SetRedraw(FALSE);
        listChanged = true;
    };
    // 单元格值不变时不调用 SetItemText，避免列表控件清除当前选择高亮。
    const auto setItemTextIfChanged = [&](int item, int subItem, const CString& text)
    {
        if (m_clMetricList.GetItemText(item, subItem) == text) return;
        beginListUpdate();
        m_clMetricList.SetItemText(item, subItem, text);
    };

    // 保留现有 ListView 行对象，避免一秒快照刷新销毁用户的选中行和滚动位置。
    while (m_clMetricList.GetItemCount() > static_cast<int>(metrics.size()))
    {
        beginListUpdate();
        m_clMetricList.DeleteItem(m_clMetricList.GetItemCount() - 1);
    }
    for (int index = 0; index < static_cast<int>(metrics.size()); ++index)
    {
        const auto& row = metrics[index];
        const CString scenario = Utf8Text(row.scenario);
        if (index >= m_clMetricList.GetItemCount())
        {
            beginListUpdate();
            m_clMetricList.InsertItem(index, scenario);
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
        m_clMetricList.SetRedraw(TRUE);
        m_clMetricList.Invalidate(FALSE);
    }

    const auto transactions = m_clEngine.GetScenarioMetrics();
    bool transactionListChanged = false;
    // 事务列表与接口列表使用相同的增量更新边界；禁止定时刷新时清空重建，
    // 否则 ListView 会丢失选中行并把纵向、横向滚动位置重置到起点。
    const auto beginTransactionListUpdate = [&]()
    {
        if (transactionListChanged) return;
        m_clTransactionList.SetRedraw(FALSE);
        transactionListChanged = true;
    };
    const auto setTransactionItemTextIfChanged =
        [&](int item, int subItem, const CString& text)
    {
        if (m_clTransactionList.GetItemText(item, subItem) == text) return;
        beginTransactionListUpdate();
        m_clTransactionList.SetItemText(item, subItem, text);
    };

    // 只删除不再存在的尾部行；现有行对象保持不变以保留用户浏览状态。
    while (m_clTransactionList.GetItemCount() > static_cast<int>(transactions.size()))
    {
        beginTransactionListUpdate();
        m_clTransactionList.DeleteItem(m_clTransactionList.GetItemCount() - 1);
    }
    for (int index = 0; index < static_cast<int>(transactions.size()); ++index)
    {
        const auto& row = transactions[index];
        const CString scenario = Utf8Text(row.scenario);
        if (index >= m_clTransactionList.GetItemCount())
        {
            beginTransactionListUpdate();
            m_clTransactionList.InsertItem(index, scenario);
        }
        else
        {
            setTransactionItemTextIfChanged(index, 0, scenario);
        }
        CString value;
        value.Format(L"%llu", row.started); setTransactionItemTextIfChanged(index, 1, value);
        value.Format(L"%llu", row.completed); setTransactionItemTextIfChanged(index, 2, value);
        value.Format(L"%llu", row.succeeded); setTransactionItemTextIfChanged(index, 3, value);
        value.Format(L"%llu", row.failed); setTransactionItemTextIfChanged(index, 4, value);
        value.Format(L"%llu", row.dropped); setTransactionItemTextIfChanged(index, 5, value);
        value.Format(L"%.2f", row.averageMs); setTransactionItemTextIfChanged(index, 6, value);
        value.Format(L"%.2f", row.medianMs); setTransactionItemTextIfChanged(index, 7, value);
        value.Format(L"%.2f", row.p90Ms); setTransactionItemTextIfChanged(index, 8, value);
        value.Format(L"%.2f", row.p95Ms); setTransactionItemTextIfChanged(index, 9, value);
        value.Format(L"%.2f", row.p99Ms); setTransactionItemTextIfChanged(index, 10, value);
        value.Format(L"%.2f", row.throughputPerSec);
        setTransactionItemTextIfChanged(index, 11, value);
    }
    if (transactionListChanged)
    {
        m_clTransactionList.SetRedraw(TRUE);
        m_clTransactionList.Invalidate(FALSE);
    }

    LogEngineErrors();
    if (state.state != m_iLastLoggedState)
    {
        m_clAppLog.Add(L"INFO", L"运行状态变更: " + std::wstring(StateText(state.state).GetString()) +
            L"，runId=" + std::to_wstring(state.runId) +
            L"，samples=" + std::to_wstring(state.totalSamples));
        m_iLastLoggedState = state.state;
    }

    const bool terminal = state.state == MTLOAD_STATE_COMPLETED || state.state == MTLOAD_STATE_FAILED;
    if (terminal)
    {
        if (state.state == MTLOAD_STATE_FAILED)
        {
            const std::wstring error = m_clEngine.LastError();
            SetStatus((L"运行失败: " + error).c_str());
            m_clAppLog.Add(L"ERROR", L"本轮压测失败: " + SanitizeDiagnostic(error));
        }
        else if (state.lastResult == MTLOAD_E_THRESHOLD_FAILED)
            SetStatus(L"本轮压测已完成，但验收阈值未通过");
        else SetStatus(L"本轮压测已完成");
        LogTerminalSummary(state, metrics, transactions, m_clEngine.GetThresholdResults());
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
    const auto errors = m_clEngine.GetErrorSamples();
    if (errors.size() < m_szObservedErrorCount) m_szObservedErrorCount = 0;
    for (size_t index = m_szObservedErrorCount; index < errors.size(); ++index)
    {
        const auto& sample = errors[index];
        CString message;
        const std::wstring summary = SanitizeDiagnostic(Utf8Text(sample.summary).GetString());
        message.Format(L"接口失败: 场景=%s，步骤=%s，接口=%s，分类=%s，result=%d，http=%d，耗时=%.2fms，原因=%s",
            Utf8Text(sample.scenario).GetString(), Utf8Text(sample.step).GetString(),
            Utf8Text(sample.label).GetString(), Utf8Text(sample.category).GetString(),
            sample.result, sample.httpStatus, sample.elapsedMs, summary.c_str());
        m_clAppLog.Add(L"ERROR", message.GetString());
    }
    m_szObservedErrorCount = errors.size();
}

// 每个 runId 只记录一次终态摘要，避免定时器持续重复输出相同汇总。
void CMtLoadTestDlg::LogTerminalSummary(const MTLOAD_RUN_STATE& state,
    const std::vector<MTLOAD_METRIC_ROW>& metrics,
    const std::vector<MTLOAD_SCENARIO_METRIC_ROW>& transactions,
    const std::vector<MTLOAD_THRESHOLD_RESULT>& thresholds)
{
    if (state.runId == 0 || m_u64SummarizedRunId == state.runId) return;
    m_u64SummarizedRunId = state.runId;

    m_clAppLog.Add(state.state == MTLOAD_STATE_COMPLETED ? L"INFO" : L"ERROR",
        L"本轮结束: runId=" + std::to_wstring(state.runId) +
        L"，state=" + std::wstring(StateText(state.state).GetString()) +
        L"，result=" + std::to_wstring(state.lastResult) +
        L"，samples=" + std::to_wstring(state.totalSamples) +
        L"，throughput=" + std::to_wstring(state.totalThroughputPerSec) + L" req/s" +
        L"，transactions=" + std::to_wstring(state.completedIterations) +
        L"，dropped=" + std::to_wstring(state.droppedIterations));
    for (const auto& row : metrics)
    {
        CString message;
        message.Format(L"接口汇总: %s %s，场景=%s，步骤=%s，样本=%llu，成功=%llu，失败=%llu，平均=%.2fms，P95=%.2fms，吞吐=%.2freq/s",
            Utf8Text(row.method).GetString(), Utf8Text(row.path).GetString(),
            Utf8Text(row.scenario).GetString(), Utf8Text(row.step).GetString(),
            row.samples, row.succeeded, row.failed, row.averageMs, row.p95Ms, row.throughputPerSec);
        m_clAppLog.Add(row.failed == 0 ? L"INFO" : L"WARN", message.GetString());
    }
    for (const auto& row : transactions)
    {
        CString message;
        message.Format(L"事务汇总: 场景=%s，启动=%llu，完成=%llu，成功=%llu，失败=%llu，丢弃=%llu，平均=%.2fms，P95=%.2fms，吞吐=%.2ftx/s",
            Utf8Text(row.scenario).GetString(), row.started, row.completed, row.succeeded,
            row.failed, row.dropped, row.averageMs, row.p95Ms, row.throughputPerSec);
        m_clAppLog.Add(row.failed == 0 && row.dropped == 0 ? L"INFO" : L"WARN", message.GetString());
    }
    for (const auto& row : thresholds)
    {
        CString message;
        message.Format(L"阈值结果: %s，scope=%s，metric=%s，target=%s，operator=%s，expected=%.4f，actual=%.4f，passed=%s",
            Utf8Text(row.name).GetString(), Utf8Text(row.scope).GetString(), Utf8Text(row.metric).GetString(),
            Utf8Text(row.target).GetString(), Utf8Text(row.comparison).GetString(), row.expected,
            row.actual, row.passed == 1 ? L"true" : L"false");
        m_clAppLog.Add(row.passed == 1 ? L"INFO" : L"ERROR", message.GetString());
    }
}

// 关键实现：停止快照定时器、销毁日志窗并卸载引擎，待后台线程回收后再退出对话框。
void CMtLoadTestDlg::OnCancel()
{
    if (m_bClosing) return;
    m_bClosing = true;
    KillTimer(kSnapshotTimer);
    m_clAppLog.Add(L"INFO", L"MtLoadTest 正在退出，停止并回收压测线程");
    if (m_pLogDialog && ::IsWindow(m_pLogDialog->GetSafeHwnd())) m_pLogDialog->DestroyWindow();
    m_pLogDialog.reset();
    m_clEngine.Unload();
    m_clAppLog.Add(L"INFO", L"MtLoadTest 压测线程已回收，进程退出");
    CDialogEx::OnCancel();
}

