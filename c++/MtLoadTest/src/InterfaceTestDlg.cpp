#include "pch.h"
#include "InterfaceTestDlg.h"

#include "common/Utf8Text.h"
#include "interface_test/JsonMessage.h"
#include "LogDlg.h"
#include "resource.h"

#include <algorithm>
#include <iomanip>
#include <set>
#include <sstream>

namespace
{
constexpr UINT kHttpCompletedMessage = WM_APP + 301; // HTTP 后台任务完成消息。
constexpr UINT kWebMessage = WM_APP + 302;           // WebSocket 收发记录消息。
constexpr UINT kWebStateMessage = WM_APP + 303;      // WebSocket 连接状态消息。

// HTTP 工作线程向 UI 线程转移的完成数据。
struct HttpCompletion
{
    interface_test::HttpResponse response; // 完整传输结果，消息处理函数接管所有权。
    std::string caseId;                    // 发起请求时的用例 ID，防止切换后误应用提取规则。
};

// WebSocket 后台线程向 UI 线程转移的状态数据。
struct WebState
{
    bool connected = false;    // 回调发生时的连接状态。
    std::string text;          // 面向用户的 UTF-8 状态或错误说明。
};

// 安全读取指定控件文本；控件不存在时返回空字符串。
CString ControlText(const CWnd* window, int id)
{
    CString value;
    const_cast<CWnd*>(window)->GetDlgItemText(id, value);
    return value;
}

// 去除宽字符串首尾空白，用于 Pointer、路径和方法输入校验。
std::wstring Trim(const std::wstring& value)
{
    const size_t first = value.find_first_not_of(L" \t\r\n");
    if (first == std::wstring::npos) return {};
    const size_t last = value.find_last_not_of(L" \t\r\n");
    return value.substr(first, last - first + 1);
}

// 按 1024 进制格式化字节数并保留两位小数。
std::wstring FormatBytes(uint64_t bytes)
{
    static const wchar_t* units[] = {L"B", L"KB", L"MB", L"GB", L"TB"};
    double value = static_cast<double>(bytes);
    size_t unit = 0;
    while (value >= 1024.0 && unit + 1 < _countof(units))
    {
        value /= 1024.0;
        ++unit;
    }
    std::wostringstream stream;
    stream << std::fixed << std::setprecision(2) << value << L" " << units[unit];
    return stream.str();
}

// 返回当前 EXE 目录，供默认配置路径定位使用。
std::wstring ExecutableDirectory()
{
    std::vector<wchar_t> buffer(32768);
    const DWORD length = ::GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    std::wstring path(buffer.data(), length);
    const size_t separator = path.find_last_of(L"\\/");
    return separator == std::wstring::npos ? L"." : path.substr(0, separator);
}

// 按显示文本查找组合框项，找不到时返回 CB_ERR。
int FindComboText(const CComboBox& combo, const std::wstring& text)
{
    for (int i = 0; i < combo.GetCount(); ++i)
    {
        CString value;
        combo.GetLBText(i, value);
        if (std::wstring(value.GetString()) == text)
            return i;
    }
    return CB_ERR;
}

// 请求头名称按 HTTP 规则执行 ASCII 大小写无关比较，不依赖平台区域设置。
bool EqualAsciiNoCase(const std::string& left, const std::string& right)
{
    if (left.size() != right.size()) return false;
    for (size_t index = 0; index < left.size(); ++index)
    {
        const auto fold = [](unsigned char value) {
            return value >= 'A' && value <= 'Z' ? static_cast<unsigned char>(value + ('a' - 'A')) : value;
        };
        if (fold(static_cast<unsigned char>(left[index])) !=
            fold(static_cast<unsigned char>(right[index])))
            return false;
    }
    return true;
}
} // namespace

BEGIN_MESSAGE_MAP(CInterfaceTestDlg, CDialogEx)
    ON_WM_DESTROY()
    ON_WM_SIZE()
    ON_BN_CLICKED(IDC_INTERFACE_BROWSE, &CInterfaceTestDlg::OnBrowse)
    ON_BN_CLICKED(IDC_INTERFACE_LOAD, &CInterfaceTestDlg::OnLoad)
    ON_BN_CLICKED(IDC_INTERFACE_SAVE, &CInterfaceTestDlg::OnSave)
    ON_CBN_SELCHANGE(IDC_INTERFACE_ENV, &CInterfaceTestDlg::OnEnvironmentChanged)
    ON_CBN_SELCHANGE(IDC_INTERFACE_TRANSPORT, &CInterfaceTestDlg::OnTransportChanged)
    ON_CBN_SELCHANGE(IDC_INTERFACE_SERVER, &CInterfaceTestDlg::OnServerChanged)
    ON_CBN_SELCHANGE(IDC_INTERFACE_PAYLOAD, &CInterfaceTestDlg::OnPayloadChanged)
    ON_CBN_SELCHANGE(IDC_INTERFACE_CATEGORY, &CInterfaceTestDlg::OnCategoryChanged)
    ON_CBN_SELCHANGE(IDC_INTERFACE_CASE, &CInterfaceTestDlg::OnCaseChanged)
    ON_CBN_SELCHANGE(IDC_INTERFACE_VARIABLE_SET, &CInterfaceTestDlg::OnVariableSetChanged)
    ON_EN_CHANGE(IDC_INTERFACE_METHOD, &CInterfaceTestDlg::OnEditorChanged)
    ON_EN_CHANGE(IDC_INTERFACE_PATH, &CInterfaceTestDlg::OnEditorChanged)
    ON_EN_CHANGE(IDC_INTERFACE_HEADERS, &CInterfaceTestDlg::OnEditorChanged)
    ON_EN_CHANGE(IDC_INTERFACE_REQUEST, &CInterfaceTestDlg::OnEditorChanged)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_INTERFACE_FIELD_LIST, &CInterfaceTestDlg::OnFieldChanged)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_INTERFACE_WEB_LIST, &CInterfaceTestDlg::OnWebSelectionChanged)
    ON_BN_CLICKED(IDC_INTERFACE_FIELD_APPLY, &CInterfaceTestDlg::OnApplyField)
    ON_BN_CLICKED(IDC_INTERFACE_FIELD_ADD, &CInterfaceTestDlg::OnAddField)
    ON_BN_CLICKED(IDC_INTERFACE_FIELD_DELETE, &CInterfaceTestDlg::OnDeleteField)
    ON_BN_CLICKED(IDC_INTERFACE_JSON_APPLY, &CInterfaceTestDlg::OnApplyJson)
    ON_BN_CLICKED(IDC_INTERFACE_SEND, &CInterfaceTestDlg::OnSend)
    ON_BN_CLICKED(IDC_INTERFACE_CANCEL, &CInterfaceTestDlg::OnCancelRequest)
    ON_BN_CLICKED(IDC_INTERFACE_CONNECT, &CInterfaceTestDlg::OnConnectWeb)
    ON_BN_CLICKED(IDC_INTERFACE_DISCONNECT, &CInterfaceTestDlg::OnDisconnectWeb)
    ON_BN_CLICKED(IDC_INTERFACE_WEB_CLEAR, &CInterfaceTestDlg::OnClearWeb)
    ON_BN_CLICKED(IDC_INTERFACE_LOG, &CInterfaceTestDlg::OnShowLog)
    ON_MESSAGE(kHttpCompletedMessage, &CInterfaceTestDlg::OnHttpCompleted)
    ON_MESSAGE(kWebMessage, &CInterfaceTestDlg::OnWebMessage)
    ON_MESSAGE(kWebStateMessage, &CInterfaceTestDlg::OnWebState)
END_MESSAGE_MAP()

// 关键实现：创建彼此独立的 HTTP 和 WebSocket 传输对象，不在构造期访问窗口句柄。
CInterfaceTestDlg::CInterfaceTestDlg()
    : CDialogEx(IDD_INTERFACE_TEST_DIALOG),
      m_pHttpTransport(std::make_unique<interface_test::SocketHttpTransport>()),
      m_pWebTransport(std::make_unique<interface_test::SocketWebTransport>())
{
}

// 关键实现：复用 StopWorkers，保证析构返回时不再有后台线程访问本对象。
CInterfaceTestDlg::~CInterfaceTestDlg()
{
    StopWorkers();
}

// 关键实现：集中绑定下拉、字段列表和 WebSocket 列表控件。
void CInterfaceTestDlg::DoDataExchange(CDataExchange* exchange)
{
    CDialogEx::DoDataExchange(exchange);
    DDX_Control(exchange, IDC_INTERFACE_ENV, m_clEnvironmentCombo);
    DDX_Control(exchange, IDC_INTERFACE_TRANSPORT, m_clTransportCombo);
    DDX_Control(exchange, IDC_INTERFACE_SERVER, m_clServerCombo);
    DDX_Control(exchange, IDC_INTERFACE_PROTOCOL, m_clPayloadCombo);
    DDX_Control(exchange, IDC_INTERFACE_VARIABLE_SET, m_clVariableSetCombo);
    DDX_Control(exchange, IDC_INTERFACE_CATEGORY, m_clCategoryCombo);
    DDX_Control(exchange, IDC_INTERFACE_CASE, m_clCaseCombo);
    DDX_Control(exchange, IDC_INTERFACE_FIELD_TYPE, m_clFieldTypeCombo);
    DDX_Control(exchange, IDC_INTERFACE_FIELD_LIST, m_clFieldList);
    DDX_Control(exchange, IDC_INTERFACE_WEB_LIST, m_clWebList);
}

// 关键实现：初始化列表后从 EXE/conf 加载独立配置，失败仅提示而不退出工具。
BOOL CInterfaceTestDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    SetIcon(AfxGetApp()->LoadIcon(IDI_MTLOADTEST_ICON), TRUE);
    SetIcon(AfxGetApp()->LoadIcon(IDI_MTLOADTEST_ICON), FALSE);
    std::wstring logError;
    if (m_clAppLog.Initialize(ExecutableDirectory(), logError))
        m_clAppLog.Add(L"INFO", L"接口测试窗口启动，日志文件=" + m_clAppLog.FilePath());
    else
        m_clAppLog.Add(L"ERROR", L"接口测试日志初始化失败: " + logError);
    InitializeLists();
    m_wstrConfigPath = ExecutableDirectory() + L"\\conf\\interface-test.xml";
    SetDlgItemText(IDC_INTERFACE_CONFIG_PATH, m_wstrConfigPath.c_str());
    if (!LoadConfiguration(m_wstrConfigPath))
        SetStatus(L"请选择并加载 interface-test.xml");
    return TRUE;
}

// 关键实现：按业务阅读顺序创建字段列和 WebSocket 消息列。
void CInterfaceTestDlg::InitializeLists()
{
    m_clFieldList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);
    m_clFieldList.InsertColumn(0, L"启用", LVCFMT_CENTER, 48);
    m_clFieldList.InsertColumn(1, L"JSON Pointer", LVCFMT_LEFT, 145);
    m_clFieldList.InsertColumn(2, L"类型", LVCFMT_LEFT, 70);
    m_clFieldList.InsertColumn(3, L"必填", LVCFMT_CENTER, 48);
    m_clFieldList.InsertColumn(4, L"值", LVCFMT_LEFT, 145);
    m_clFieldList.InsertColumn(5, L"说明", LVCFMT_LEFT, 220);
    ResizeFieldColumns();

    m_clWebList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    m_clWebList.InsertColumn(0, L"时间", LVCFMT_LEFT, 90);
    m_clWebList.InsertColumn(1, L"方向", LVCFMT_CENTER, 48);
    m_clWebList.InsertColumn(2, L"FuncId", LVCFMT_LEFT, 58);
    m_clWebList.InsertColumn(3, L"Topic", LVCFMT_LEFT, 52);
    m_clWebList.InsertColumn(4, L"EventType", LVCFMT_LEFT, 70);
    m_clWebList.InsertColumn(5, L"大小", LVCFMT_RIGHT, 70);
    m_clWebList.InsertColumn(6, L"摘要", LVCFMT_LEFT, 380);
    ResizeWebColumns();

    const wchar_t* types[] = {L"string", L"integer", L"unsigned", L"number", L"boolean", L"object", L"array", L"null"};
    for (const wchar_t* type : types)
        m_clFieldTypeCombo.AddString(type);
}

// 字段列按当前列表客户区分配，避免类型和值截断，并让说明列吸收全部剩余宽度。
// 窄窗口保留 900 像素的可读基线和水平滚动，防止为铺满而把字段压缩到不可读。
void CInterfaceTestDlg::ResizeFieldColumns()
{
    if (m_clFieldList.GetSafeHwnd() == nullptr || m_clFieldList.GetHeaderCtrl() == nullptr ||
        m_clFieldList.GetHeaderCtrl()->GetItemCount() != 6)
        return;

    CRect client;
    m_clFieldList.GetClientRect(&client);
    const int available = std::max(client.Width() - ::GetSystemMetrics(SM_CXVSCROLL) - 2, 900);
    const int weights[] = {6, 18, 11, 7, 23};
    int used = 0;
    for (int column = 0; column < static_cast<int>(_countof(weights)); ++column)
    {
        const int width = available * weights[column] / 100;
        m_clFieldList.SetColumnWidth(column, width);
        used += width;
    }
    m_clFieldList.SetColumnWidth(5, std::max(available - used, 1));
}

// Web 列宽按当前列表客户区分配，避免高 DPI 下标题截断或末尾留下空白列区域。
// 窄窗口使用 800 像素的可读基线并保留横向滚动，宽窗口则完整铺满。
void CInterfaceTestDlg::ResizeWebColumns()
{
    if (m_clWebList.GetSafeHwnd() == nullptr || m_clWebList.GetHeaderCtrl() == nullptr ||
        m_clWebList.GetHeaderCtrl()->GetItemCount() != 7)
        return;

    CRect client;
    m_clWebList.GetClientRect(&client);
    const int available = std::max(client.Width() - ::GetSystemMetrics(SM_CXVSCROLL) - 2, 800);
    const int weights[] = {15, 8, 10, 12, 14, 10};
    int used = 0;
    for (int column = 0; column < static_cast<int>(_countof(weights)); ++column)
    {
        const int width = available * weights[column] / 100;
        m_clWebList.SetColumnWidth(column, width);
        used += width;
    }
    m_clWebList.SetColumnWidth(6, std::max(available - used, 1));
}

// 关键实现：窗口创建完成后才调整 WebSocket 列，避免访问尚未绑定的控件。
void CInterfaceTestDlg::OnSize(UINT type, int width, int height)
{
    CDialogEx::OnSize(type, width, height);
    ResizeFieldColumns();
    ResizeWebColumns();
}

// 关键实现：先停止网络任务，再加载完整模型并按依赖顺序刷新下拉与工作副本。
bool CInterfaceTestDlg::LoadConfiguration(const std::wstring& path)
{
    interface_test::InterfaceTestConfig loaded;
    std::string error;
    if (!loaded.Load(mttool::WideToUtf8(path), error))
    {
        SetStatus(mttool::Utf8ToWide(error));
        return false;
    }
    m_stConfig = std::move(loaded);
    m_wstrConfigPath = path;
    m_strSelectedCaseId.clear();
    m_bDirty = false;
    m_bRawJsonDirty = false;
    m_stSession = {};
    SetDlgItemText(IDC_INTERFACE_CONFIG_PATH, m_wstrConfigPath.c_str());
    m_bRefreshing = true;
    PopulateEnvironments();
    PopulateTransports();
    PopulateServers();
    PopulatePayloads();
    PopulateVariableSets();
    PopulateCategories();
    PopulateCases();
    m_bRefreshing = false;
    LoadSelectedCase();
    SetStatus(L"配置已加载；请选择服务、协议和接口后发送请求");
    return true;
}

// 关键实现：重建显示项和稳定 ID 并行数组，优先选择配置默认环境。
void CInterfaceTestDlg::PopulateEnvironments()
{
    m_clEnvironmentCombo.ResetContent();
    m_vecEnvironmentIds.clear();
    for (const auto& environment : m_stConfig.environments)
    {
        m_clEnvironmentCombo.AddString(mttool::Utf8ToWide(environment.name).c_str());
        m_vecEnvironmentIds.push_back(environment.id);
    }
    SelectComboId(m_clEnvironmentCombo, m_vecEnvironmentIds, m_stConfig.defaultEnvironmentId);
    if (m_clEnvironmentCombo.GetCurSel() == CB_ERR && m_clEnvironmentCombo.GetCount() > 0)
        m_clEnvironmentCombo.SetCurSel(0);
}

// 关键实现：根据当前环境实际服务器去重服务类型，避免展示无可选服务器的入口。
void CInterfaceTestDlg::PopulateTransports()
{
    const int previous = m_clTransportCombo.GetCurSel();
    interface_test::TransportKind previousValue = interface_test::TransportKind::Http;
    if (previous >= 0 && previous < static_cast<int>(m_vecTransportValues.size()))
        previousValue = m_vecTransportValues[previous];
    m_clTransportCombo.ResetContent();
    m_vecTransportValues.clear();
    const int environmentIndex = m_clEnvironmentCombo.GetCurSel();
    const std::string environmentId = environmentIndex >= 0 && environmentIndex < static_cast<int>(m_vecEnvironmentIds.size()) ?
        m_vecEnvironmentIds[environmentIndex] : std::string();
    const interface_test::TransportKind values[] = {interface_test::TransportKind::Tcp,
        interface_test::TransportKind::Http, interface_test::TransportKind::Https,
        interface_test::TransportKind::WebSocket};
    for (auto value : values)
    {
        const bool configured = std::any_of(m_stConfig.servers.begin(), m_stConfig.servers.end(),
            [value, &environmentId](const auto& server) {
                return server.environmentId == environmentId && server.transport == value;
            });
        if (!configured && value != interface_test::TransportKind::Tcp)
            continue;
        m_clTransportCombo.AddString(mttool::Utf8ToWide(interface_test::ToDisplayName(value)).c_str());
        m_vecTransportValues.push_back(value);
    }
    auto it = std::find(m_vecTransportValues.begin(), m_vecTransportValues.end(), previousValue);
    if (it == m_vecTransportValues.end())
        it = std::find(m_vecTransportValues.begin(), m_vecTransportValues.end(), interface_test::TransportKind::Http);
    m_clTransportCombo.SetCurSel(it == m_vecTransportValues.end() ? 0 : static_cast<int>(it - m_vecTransportValues.begin()));
}

// 关键实现：同时按环境和服务类型筛选服务器，并重建 ID 映射。
void CInterfaceTestDlg::PopulateServers()
{
    m_clServerCombo.ResetContent();
    m_vecServerIds.clear();
    const int environmentIndex = m_clEnvironmentCombo.GetCurSel();
    if (environmentIndex < 0 || environmentIndex >= static_cast<int>(m_vecEnvironmentIds.size())) return;
    const std::string& environmentId = m_vecEnvironmentIds[environmentIndex];
    const auto transport = SelectedTransport();
    for (const auto& server : m_stConfig.servers)
    {
        if (server.environmentId != environmentId || server.transport != transport) continue;
        std::string label = server.name + "  " + server.scheme + "://" + server.host;
        if (server.hasExplicitPort) label += ":" + std::to_string(server.port);
        m_clServerCombo.AddString(mttool::Utf8ToWide(label).c_str());
        m_vecServerIds.push_back(server.id);
    }
    if (m_clServerCombo.GetCount() > 0) m_clServerCombo.SetCurSel(0);
}

// 关键实现：合并服务器声明和能力矩阵，非法组合不进入下拉，待开发组合明确标注。
void CInterfaceTestDlg::PopulatePayloads()
{
    m_clPayloadCombo.ResetContent();
    m_vecPayloadValues.clear();
    const interface_test::ServerProfile* server = SelectedServer();
    std::vector<interface_test::PayloadKind> values;
    if (server != nullptr) values = server->payloads;
    if (values.empty()) values = {interface_test::PayloadKind::Binary, interface_test::PayloadKind::Json,
        interface_test::PayloadKind::Protobuf};
    for (auto value : values)
    {
        const auto support = interface_test::QueryTransportPayloadSupport(SelectedTransport(), value);
        if (!support.allowed) continue;
        m_clPayloadCombo.AddString(mttool::Utf8ToWide(support.label).c_str());
        m_vecPayloadValues.push_back(value);
    }
    auto json = std::find(m_vecPayloadValues.begin(), m_vecPayloadValues.end(), interface_test::PayloadKind::Json);
    m_clPayloadCombo.SetCurSel(json == m_vecPayloadValues.end() ? 0 : static_cast<int>(json - m_vecPayloadValues.begin()));
}

// 关键实现：重建变量组列表并选择配置默认值，变量内容不显示在下拉文本中。
void CInterfaceTestDlg::PopulateVariableSets()
{
    m_clVariableSetCombo.ResetContent();
    m_vecVariableSetIds.clear();
    for (const auto& set : m_stConfig.variableSets)
    {
        m_clVariableSetCombo.AddString(mttool::Utf8ToWide(set.name).c_str());
        m_vecVariableSetIds.push_back(set.id);
    }
    SelectComboId(m_clVariableSetCombo, m_vecVariableSetIds, m_stConfig.defaultVariableSetId);
    if (m_clVariableSetCombo.GetCurSel() == CB_ERR && m_clVariableSetCombo.GetCount() > 0)
        m_clVariableSetCombo.SetCurSel(0);
    ResetSessionVariables();
}

// 关键实现：只汇总当前服务器组和报文协议可用用例的分类并去重。
void CInterfaceTestDlg::PopulateCategories()
{
    m_clCategoryCombo.ResetContent();
    m_vecCategoryValues.clear();
    const auto* server = SelectedServer();
    const auto payload = SelectedPayload();
    std::set<std::string> categories;
    if (server != nullptr)
    {
        for (const auto& item : m_stConfig.cases)
            if (item.serverGroup == server->group && item.payload == payload)
                categories.insert(item.category);
    }
    m_clCategoryCombo.AddString(L"全部分类");
    m_vecCategoryValues.push_back({});
    for (const auto& category : categories)
    {
        m_clCategoryCombo.AddString(mttool::Utf8ToWide(category).c_str());
        m_vecCategoryValues.push_back(category);
    }
    m_clCategoryCombo.SetCurSel(0);
}

// 关键实现：按服务器组、协议和分类筛选用例，并尽量保持原 m_strSelectedCaseId。
void CInterfaceTestDlg::PopulateCases()
{
    const std::string previousId = m_strSelectedCaseId;
    m_clCaseCombo.ResetContent();
    m_vecCaseIds.clear();
    const auto* server = SelectedServer();
    const auto payload = SelectedPayload();
    const int categoryIndex = m_clCategoryCombo.GetCurSel();
    const std::string category = categoryIndex >= 0 && categoryIndex < static_cast<int>(m_vecCategoryValues.size()) ?
        m_vecCategoryValues[categoryIndex] : std::string();
    if (server != nullptr)
    {
        for (const auto& item : m_stConfig.cases)
        {
            if (item.serverGroup != server->group || item.payload != payload ||
                (!category.empty() && item.category != category)) continue;
            m_clCaseCombo.AddString(mttool::Utf8ToWide(item.name).c_str());
            m_vecCaseIds.push_back(item.id);
        }
    }
    SelectComboId(m_clCaseCombo, m_vecCaseIds, previousId);
    if (m_clCaseCombo.GetCurSel() == CB_ERR && m_clCaseCombo.GetCount() > 0) m_clCaseCombo.SetCurSel(0);
}

// 关键实现：切换时复制配置用例形成工作副本，后续编辑不直接污染 m_stConfig。
void CInterfaceTestDlg::LoadSelectedCase()
{
    const int selection = m_clCaseCombo.GetCurSel();
    if (selection < 0 || selection >= static_cast<int>(m_vecCaseIds.size()))
    {
        m_strSelectedCaseId.clear();
        m_stWorkingCase = {};
        RefreshCaseDisplay();
        return;
    }
    m_strSelectedCaseId = m_vecCaseIds[selection];
    if (const auto* item = m_stConfig.FindCase(m_strSelectedCaseId)) m_stWorkingCase = *item;
    m_bDirty = false;
    RefreshCaseDisplay();
}

// 关键实现：在 m_bRefreshing 门禁内批量更新说明、方法、路径、字段和 JSON。
void CInterfaceTestDlg::RefreshCaseDisplay()
{
    m_bRefreshing = true;
    SetDlgItemText(IDC_INTERFACE_METHOD, mttool::Utf8ToWide(m_stWorkingCase.method).c_str());
    SetDlgItemText(IDC_INTERFACE_PATH, mttool::Utf8ToWide(m_stWorkingCase.path).c_str());
    const std::string description = "功能：" + m_stWorkingCase.functionDescription + "\r\n使用场景：" +
        m_stWorkingCase.usageDescription + "\r\n鉴权：" + m_stWorkingCase.authDescription +
        "    副作用：" + m_stWorkingCase.sideEffectDescription;
    SetDlgItemText(IDC_INTERFACE_DESCRIPTION, mttool::Utf8ToWide(description).c_str());
    std::string headers;
    for (const auto& header : m_stWorkingCase.headers)
        headers += header.name + ": " + header.value + "\r\n";
    SetDlgItemText(IDC_INTERFACE_HEADERS, mttool::Utf8ToWide(headers).c_str());
    RefreshFieldList();
    RefreshRequestPreview();
    SetDlgItemText(IDC_INTERFACE_RESPONSE_STATUS, L"");
    SetDlgItemText(IDC_INTERFACE_RESPONSE_HEADERS, L"");
    SetDlgItemText(IDC_INTERFACE_RESPONSE, L"");
    m_bRefreshing = false;
    RefreshControlState();
}

// 关键实现：字段表完整重建后恢复复选状态，必填字段仍显示但不能关闭。
void CInterfaceTestDlg::RefreshFieldList()
{
    m_bRefreshing = true;
    m_clFieldList.DeleteAllItems();
    for (size_t i = 0; i < m_stWorkingCase.fields.size(); ++i)
    {
        const auto& field = m_stWorkingCase.fields[i];
        const int row = m_clFieldList.InsertItem(static_cast<int>(i), L"");
        m_clFieldList.SetCheck(row, field.required || field.enabled);
        m_clFieldList.SetItemText(row, 1, mttool::Utf8ToWide(field.pointer).c_str());
        m_clFieldList.SetItemText(row, 2, mttool::Utf8ToWide(field.type).c_str());
        m_clFieldList.SetItemText(row, 3, field.required ? L"是" : L"否");
        m_clFieldList.SetItemText(row, 4, field.sensitive ? L"******" : mttool::Utf8ToWide(field.value).c_str());
        m_clFieldList.SetItemText(row, 5, mttool::Utf8ToWide(field.description).c_str());
        m_clFieldList.SetItemData(row, static_cast<DWORD_PTR>(i));
    }
    SetDlgItemText(IDC_INTERFACE_FIELD_PATH, L"");
    SetDlgItemText(IDC_INTERFACE_FIELD_VALUE, L"");
    m_clFieldTypeCombo.SetCurSel(CB_ERR);
    m_bRefreshing = false;
}

// 关键实现：使用 SessionVariables 副本生成预览，避免界面刷新消耗正式 sequence。
void CInterfaceTestDlg::RefreshRequestPreview()
{
    const bool previousRefreshing = m_bRefreshing;
    m_bRefreshing = true;
    if (m_strSelectedCaseId.empty())
    {
        SetDlgItemText(IDC_INTERFACE_REQUEST, L"");
        SetDlgItemText(IDC_INTERFACE_ACTUAL_REQUEST, L"");
        m_bRawJsonDirty = false;
        m_bRefreshing = previousRefreshing;
        return;
    }
    auto previewSession = m_stSession;
    std::string body;
    std::string redacted;
    std::string error;
    if (!BuildCurrentBody(previewSession, body, redacted, error))
    {
        SetDlgItemText(IDC_INTERFACE_REQUEST, mttool::Utf8ToWide("JSON 预览失败: " + error).c_str());
        m_bRawJsonDirty = false;
        m_bRefreshing = previousRefreshing;
        return;
    }
    SetDlgItemText(IDC_INTERFACE_REQUEST, mttool::Utf8ToWide(redacted).c_str());
    const auto* server = SelectedServer();
    if (server != nullptr)
    {
        std::vector<std::pair<std::string, std::string>> headers;
        std::string redactedHeaders;
        auto headerSession = m_stSession;
        BuildCurrentHeaders(headerSession, headers, redactedHeaders, error);
        const std::string actual = m_stWorkingCase.method + " " + interface_test::JoinUrl(*server,
            server->transport == interface_test::TransportKind::WebSocket ? server->webSocketPath : m_stWorkingCase.path) +
            "\r\n" + redactedHeaders + "\r\n" + redacted;
        SetDlgItemText(IDC_INTERFACE_ACTUAL_REQUEST, mttool::Utf8ToWide(actual).c_str());
    }
    m_bRawJsonDirty = false;
    m_bRefreshing = previousRefreshing;
}

// 关键实现：把协议实现状态、工作线程状态和 WebSocket 连接状态统一映射到按钮。
void CInterfaceTestDlg::RefreshControlState()
{
    const auto support = interface_test::QueryTransportPayloadSupport(SelectedTransport(), SelectedPayload());
    const bool hasCase = !m_strSelectedCaseId.empty();
    const bool web = SelectedTransport() == interface_test::TransportKind::WebSocket;
    const bool busy = m_bWorkerRunning.load();
    GetDlgItem(IDC_INTERFACE_SEND)->EnableWindow(hasCase && support.implemented && !busy &&
        (!web || m_pWebTransport->IsConnected()) && !m_stWorkingCase.receiveOnly);
    GetDlgItem(IDC_INTERFACE_CANCEL)->EnableWindow(busy && !web);
    GetDlgItem(IDC_INTERFACE_CONNECT)->EnableWindow(web && support.implemented && !busy && !m_pWebTransport->IsConnected());
    GetDlgItem(IDC_INTERFACE_DISCONNECT)->EnableWindow(web && m_pWebTransport->IsConnected());
    GetDlgItem(IDC_INTERFACE_WEB_CLEAR)->EnableWindow(web);
    GetDlgItem(IDC_INTERFACE_FIELD_ADD)->EnableWindow(hasCase && support.implemented);
    GetDlgItem(IDC_INTERFACE_FIELD_DELETE)->EnableWindow(hasCase && support.implemented);
    GetDlgItem(IDC_INTERFACE_FIELD_APPLY)->EnableWindow(hasCase && support.implemented);
    GetDlgItem(IDC_INTERFACE_JSON_APPLY)->EnableWindow(hasCase && support.implemented);
    if (!support.implemented && hasCase)
        SetStatus(L"当前服务/报文组合已预留，待开发");
}

// 关键实现：从变量组重建内存会话并单独登记敏感名称，运行时 Token 不写回 XML。
void CInterfaceTestDlg::ResetSessionVariables()
{
    m_stSession = {};
    const int selection = m_clVariableSetCombo.GetCurSel();
    if (selection < 0 || selection >= static_cast<int>(m_vecVariableSetIds.size())) return;
    const auto* set = m_stConfig.FindVariableSet(m_vecVariableSetIds[selection]);
    if (set == nullptr) return;
    for (const auto& variable : set->variables)
    {
        m_stSession.values[variable.name] = variable.value;
        if (variable.sensitive) m_stSession.sensitiveNames.insert(variable.name);
    }
}

// 关键实现：委托 JsonMessage 从字段表同时构造真实正文和脱敏正文。
bool CInterfaceTestDlg::BuildCurrentBody(interface_test::SessionVariables& session, std::string& body,
    std::string& redactedBody, std::string& error) const
{
    if (m_stWorkingCase.fields.empty())
    {
        body.clear();
        redactedBody.clear();
        return true;
    }
    return interface_test::BuildJsonFromFields(m_stWorkingCase.fields, m_stWorkingCase.rootType,
        session, body, redactedBody, error);
}

// 关键实现：逐个展开请求头，必填空值立即失败，敏感值只进入脱敏预览。
bool CInterfaceTestDlg::BuildCurrentHeaders(interface_test::SessionVariables& session,
    std::vector<std::pair<std::string, std::string>>& headers,
    std::string& redactedHeaders, std::string& error) const
{
    headers.clear();
    redactedHeaders.clear();
    for (const auto& header : m_stWorkingCase.headers)
    {
        const std::string value = interface_test::ExpandVariables(header.value, session, error);
        if (!error.empty()) return false;
        if (header.required && value.empty())
        {
            error = "必填请求头不能为空: " + header.name;
            return false;
        }
        if (!header.required && value.empty()) continue;
        headers.emplace_back(header.name, value);
        const bool sensitive = header.sensitive ||
            EqualAsciiNoCase(header.name, "Authorization") ||
            header.value.find("${Token}") != std::string::npos ||
            header.value.find("${Password}") != std::string::npos;
        redactedHeaders += header.name + ": " + (sensitive ? "******" : value) + "\r\n";
    }
    return true;
}

// 关键实现：原始 JSON 领先时先同步字段表，再提交方法/路径等控件并刷新规范 JSON。
bool CInterfaceTestDlg::CommitEditors(std::string& error)
{
    m_stWorkingCase.method = mttool::WideToUtf8(Trim(std::wstring(ControlText(this, IDC_INTERFACE_METHOD).GetString())));
    m_stWorkingCase.path = mttool::WideToUtf8(Trim(std::wstring(ControlText(this, IDC_INTERFACE_PATH).GetString())));
    if (m_stWorkingCase.method.empty())
    {
        error = "请求方法不能为空";
        return false;
    }

    const std::wstring headerText = ControlText(this, IDC_INTERFACE_HEADERS).GetString();
    std::vector<interface_test::HeaderDefinition> parsedHeaders;
    size_t position = 0;
    while (position <= headerText.size())
    {
        const size_t end = headerText.find_first_of(L"\r\n", position);
        const std::wstring line = Trim(headerText.substr(position,
            end == std::wstring::npos ? std::wstring::npos : end - position));
        if (!line.empty())
        {
            const size_t colon = line.find(L':');
            if (colon == std::wstring::npos)
            {
                error = "请求头格式应为 Name: Value";
                return false;
            }
            interface_test::HeaderDefinition header;
            header.name = mttool::WideToUtf8(Trim(line.substr(0, colon)));
            header.value = mttool::WideToUtf8(Trim(line.substr(colon + 1)));
            auto old = std::find_if(m_stWorkingCase.headers.begin(), m_stWorkingCase.headers.end(), [&header](const auto& item) {
                return EqualAsciiNoCase(item.name, header.name);
            });
            if (old != m_stWorkingCase.headers.end())
            {
                header.required = old->required;
                header.sensitive = old->sensitive;
                header.description = old->description;
            }
            parsedHeaders.push_back(std::move(header));
        }
        if (end == std::wstring::npos) break;
        position = end + 1;
        if (position < headerText.size() && headerText[end] == L'\r' && headerText[position] == L'\n') ++position;
    }
    for (const auto& old : m_stWorkingCase.headers)
    {
        if (!old.required) continue;
        const bool found = std::any_of(parsedHeaders.begin(), parsedHeaders.end(), [&old](const auto& item) {
            return EqualAsciiNoCase(item.name, old.name);
        });
        if (!found)
        {
            error = "必填请求头不能删除: " + old.name;
            return false;
        }
    }
    m_stWorkingCase.headers = std::move(parsedHeaders);
    return true;
}

// 关键实现：脏工作副本提供保存、放弃、取消三路选择，取消时阻止当前切换。
bool CInterfaceTestDlg::ConfirmPendingChanges()
{
    if (!m_bDirty) return true;
    const int answer = AfxMessageBox(L"当前接口配置尚未保存。\r\n\r\n是：保存后继续\r\n否：放弃修改\r\n取消：留在当前接口",
        MB_YESNOCANCEL | MB_ICONQUESTION);
    if (answer == IDCANCEL) return false;
    if (answer == IDYES)
    {
        OnSave();
        return !m_bDirty;
    }
    m_bDirty = false;
    return true;
}

// 关键实现：文件对话框只更新配置路径，实际解析由用户点击加载触发。
void CInterfaceTestDlg::OnBrowse()
{
    if (!ConfirmPendingChanges()) return;
    CFileDialog dialog(TRUE, L"xml", nullptr, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
        L"接口测试配置 (*.xml)|*.xml|所有文件 (*.*)|*.*||", this);
    if (dialog.DoModal() == IDOK)
    {
        m_wstrConfigPath = dialog.GetPathName().GetString();
        SetDlgItemText(IDC_INTERFACE_CONFIG_PATH, m_wstrConfigPath.c_str());
        LoadConfiguration(m_wstrConfigPath);
    }
}

// 关键实现：先确认当前未保存内容，再按路径执行完整配置替换。
void CInterfaceTestDlg::OnLoad()
{
    if (!ConfirmPendingChanges()) return;
    const std::wstring path = ControlText(this, IDC_INTERFACE_CONFIG_PATH).GetString();
    LoadConfiguration(path);
}

// 关键实现：先提交工作副本到 m_stConfig，再通过 SaveAtomic 持久化并清除脏标记。
void CInterfaceTestDlg::OnSave()
{
    if (m_strSelectedCaseId.empty()) return;
    if (m_bRawJsonDirty)
    {
        AfxMessageBox(L"原始 JSON 已修改，请先点击“同步到字段表”，确认字段类型和必填规则后再保存。",
            MB_OK | MB_ICONWARNING);
        return;
    }
    std::string error;
    if (!CommitEditors(error))
    {
        AfxMessageBox(mttool::Utf8ToWide(error).c_str(), MB_OK | MB_ICONERROR);
        return;
    }
    if (auto* target = m_stConfig.FindCase(m_strSelectedCaseId)) *target = m_stWorkingCase;
    if (!m_stConfig.SaveAtomic(mttool::WideToUtf8(m_wstrConfigPath), error))
    {
        SetStatus(mttool::Utf8ToWide(error));
        AfxMessageBox(mttool::Utf8ToWide(error).c_str(), MB_OK | MB_ICONERROR);
        return;
    }
    m_bDirty = false;
    SetStatus(L"接口测试配置已原子保存");
}

// 关键实现：环境变化按服务器、协议、分类、用例顺序级联刷新。
void CInterfaceTestDlg::OnEnvironmentChanged()
{
    if (m_bRefreshing || !ConfirmPendingChanges()) return;
    m_bRefreshing = true;
    PopulateTransports(); PopulateServers(); PopulatePayloads(); PopulateCategories(); PopulateCases();
    m_bRefreshing = false;
    ResetSessionVariables();
    LoadSelectedCase();
}

// 关键实现：服务类型变化后重新建立合法服务器和报文组合。
void CInterfaceTestDlg::OnTransportChanged()
{
    if (m_bRefreshing || !ConfirmPendingChanges()) return;
    m_bRefreshing = true;
    PopulateServers(); PopulatePayloads(); PopulateCategories(); PopulateCases();
    m_bRefreshing = false;
    ResetSessionVariables();
    LoadSelectedCase();
}

// 关键实现：服务器变化后更新报文能力和适用用例，不复用旧工作副本。
void CInterfaceTestDlg::OnServerChanged()
{
    if (m_bRefreshing || !ConfirmPendingChanges()) return;
    m_bRefreshing = true;
    PopulatePayloads(); PopulateCategories(); PopulateCases();
    m_bRefreshing = false;
    ResetSessionVariables();
    LoadSelectedCase();
}

// 关键实现：报文变化只刷新匹配分类和用例，并重新计算控件可用性。
void CInterfaceTestDlg::OnPayloadChanged()
{
    if (m_bRefreshing || !ConfirmPendingChanges()) return;
    m_bRefreshing = true;
    PopulateCategories(); PopulateCases();
    m_bRefreshing = false;
    LoadSelectedCase();
}

// 关键实现：分类筛选只影响用例列表，不修改配置模型。
void CInterfaceTestDlg::OnCategoryChanged()
{
    if (m_bRefreshing || !ConfirmPendingChanges()) return;
    m_bRefreshing = true; PopulateCases(); m_bRefreshing = false; LoadSelectedCase();
}

// 关键实现：切换前处理未保存内容；用户取消时恢复原用例选择。
void CInterfaceTestDlg::OnCaseChanged()
{
    if (m_bRefreshing) return;
    const int selection = m_clCaseCombo.GetCurSel();
    if (selection < 0 || selection >= static_cast<int>(m_vecCaseIds.size())) return;
    const std::string requested = m_vecCaseIds[selection];
    if (requested != m_strSelectedCaseId && !ConfirmPendingChanges())
    {
        SelectComboId(m_clCaseCombo, m_vecCaseIds, m_strSelectedCaseId);
        return;
    }
    LoadSelectedCase();
}

// 关键实现：切换账号后清空运行时提取值并重新生成脱敏请求预览。
void CInterfaceTestDlg::OnVariableSetChanged()
{
    if (m_bRefreshing) return;
    ResetSessionVariables();
    RefreshRequestPreview();
    SetStatus(L"已切换变量组；运行时 Token 已清除");
}

// 关键实现：忽略程序刷新通知，仅把真实用户编辑标记为 m_bDirty。
void CInterfaceTestDlg::OnEditorChanged()
{
    if (m_bRefreshing) return;
    const UINT controlId = static_cast<UINT>(GetFocus() == nullptr ? 0 : GetFocus()->GetDlgCtrlID());
    if (controlId == IDC_INTERFACE_REQUEST)
        m_bRawJsonDirty = true;
    m_bDirty = true;
}

// 关键实现：处理复选变化前校验行索引，必填字段被关闭时立即恢复选中。
void CInterfaceTestDlg::OnFieldChanged(NMHDR* header, LRESULT* result)
{
    *result = 0;
    if (m_bRefreshing) return;
    const auto* change = reinterpret_cast<NMLISTVIEW*>(header);
    if ((change->uNewState & LVIS_SELECTED) != 0)
        OnFieldSelectionChanged(header, result);
    if ((change->uChanged & LVIF_STATE) == 0 || change->iItem < 0) return;
    const size_t index = static_cast<size_t>(m_clFieldList.GetItemData(change->iItem));
    if (index >= m_stWorkingCase.fields.size()) return;
    auto& field = m_stWorkingCase.fields[index];
    const bool checked = m_clFieldList.GetCheck(change->iItem) != FALSE;
    if (field.required && !checked)
    {
        m_bRefreshing = true;
        m_clFieldList.SetCheck(change->iItem, TRUE);
        m_bRefreshing = false;
        SetStatus(L"必填字段不能关闭");
        return;
    }
    if (field.enabled != checked)
    {
        field.enabled = checked;
        m_bDirty = true;
        RefreshRequestPreview();
    }
}

// 关键实现：只在新选中行有效时回填字段编辑器，避免空选择覆盖输入。
void CInterfaceTestDlg::OnFieldSelectionChanged(NMHDR* header, LRESULT* result)
{
    *result = 0;
    if (m_bRefreshing) return;
    const auto* change = reinterpret_cast<NMLISTVIEW*>(header);
    if ((change->uNewState & LVIS_SELECTED) == 0 || change->iItem < 0) return;
    const size_t index = static_cast<size_t>(m_clFieldList.GetItemData(change->iItem));
    if (index >= m_stWorkingCase.fields.size()) return;
    const auto& field = m_stWorkingCase.fields[index];
    SetDlgItemText(IDC_INTERFACE_FIELD_PATH, mttool::Utf8ToWide(field.pointer).c_str());
    SetDlgItemText(IDC_INTERFACE_FIELD_VALUE, mttool::Utf8ToWide(field.value).c_str());
    const int typeIndex = FindComboText(m_clFieldTypeCombo, mttool::Utf8ToWide(field.type));
    m_clFieldTypeCombo.SetCurSel(typeIndex);
}

// 关键实现：校验 Pointer 唯一性和类型后更新选中字段，再同步原始 JSON。
void CInterfaceTestDlg::OnApplyField()
{
    POSITION position = m_clFieldList.GetFirstSelectedItemPosition();
    if (position == nullptr) return;
    const int row = m_clFieldList.GetNextSelectedItem(position);
    const size_t index = static_cast<size_t>(m_clFieldList.GetItemData(row));
    if (index >= m_stWorkingCase.fields.size()) return;
    auto& field = m_stWorkingCase.fields[index];
    const std::string pointer = mttool::WideToUtf8(Trim(ControlText(this, IDC_INTERFACE_FIELD_PATH).GetString()));
    if (pointer.empty() || pointer.front() != '/')
    {
        AfxMessageBox(L"JSON Pointer 必须以 / 开头。", MB_OK | MB_ICONERROR);
        return;
    }
    if (field.required && pointer != field.pointer)
    {
        AfxMessageBox(L"必填字段的 JSON Pointer 不能修改。", MB_OK | MB_ICONERROR);
        return;
    }
    field.pointer = pointer;
    CString type;
    const int typeIndex = m_clFieldTypeCombo.GetCurSel();
    if (typeIndex != CB_ERR) m_clFieldTypeCombo.GetLBText(typeIndex, type);
    if (!type.IsEmpty()) field.type = mttool::WideToUtf8(type.GetString());
    field.value = mttool::WideToUtf8(ControlText(this, IDC_INTERFACE_FIELD_VALUE).GetString());
    field.enabled = true;
    m_bDirty = true;
    m_bRawJsonDirty = false;
    RefreshFieldList();
    RefreshRequestPreview();
}

// 关键实现：生成不冲突的默认 Pointer，新字段始终为可选且启用。
void CInterfaceTestDlg::OnAddField()
{
    std::set<std::string> paths;
    for (const auto& field : m_stWorkingCase.fields) paths.insert(field.pointer);
    unsigned int number = 1;
    std::string pointer;
    do { pointer = "/NewField" + std::to_string(number++); } while (paths.count(pointer) != 0);
    interface_test::FieldDefinition field;
    field.pointer = pointer;
    field.description = "界面新增可选字段";
    field.enabled = true;
    m_stWorkingCase.fields.push_back(std::move(field));
    m_bDirty = true;
    RefreshFieldList();
    const int row = static_cast<int>(m_stWorkingCase.fields.size() - 1);
    m_clFieldList.SetItemState(row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    m_clFieldList.EnsureVisible(row, FALSE);
    RefreshRequestPreview();
}

// 关键实现：仅允许删除当前可选字段，必填字段保留契约门禁。
void CInterfaceTestDlg::OnDeleteField()
{
    POSITION position = m_clFieldList.GetFirstSelectedItemPosition();
    if (position == nullptr) return;
    const int row = m_clFieldList.GetNextSelectedItem(position);
    const size_t index = static_cast<size_t>(m_clFieldList.GetItemData(row));
    if (index >= m_stWorkingCase.fields.size()) return;
    if (m_stWorkingCase.fields[index].required)
    {
        AfxMessageBox(L"必填字段不能删除。", MB_OK | MB_ICONWARNING);
        return;
    }
    m_stWorkingCase.fields.erase(m_stWorkingCase.fields.begin() + index);
    m_bDirty = true;
    RefreshFieldList();
    RefreshRequestPreview();
}

// 关键实现：完整解析原始 JSON 后才替换字段状态，失败保留用户文本供修正。
void CInterfaceTestDlg::OnApplyJson()
{
    const std::string raw = mttool::WideToUtf8(ControlText(this, IDC_INTERFACE_REQUEST).GetString());
    const auto oldFields = m_stWorkingCase.fields;
    std::string error;
    if (!interface_test::SynchronizeFieldsFromJson(raw, m_stWorkingCase.fields, m_stWorkingCase.rootType, error))
    {
        AfxMessageBox(mttool::Utf8ToWide(error).c_str(), MB_OK | MB_ICONERROR);
        return;
    }
    for (const auto& old : oldFields)
    {
        if (!old.sensitive) continue;
        auto current = std::find_if(m_stWorkingCase.fields.begin(), m_stWorkingCase.fields.end(), [&old](const auto& item) {
            return item.pointer == old.pointer;
        });
        if (current != m_stWorkingCase.fields.end() && current->value == "******")
        {
            current->value = old.value;
            current->type = old.type;
            current->sensitive = true;
        }
    }
    m_bDirty = true;
    m_bRawJsonDirty = false;
    RefreshFieldList();
    RefreshRequestPreview();
}

// 关键实现：只对非 read 风险用例弹出包含 URL 和脱敏摘要的确认框。
bool CInterfaceTestDlg::ConfirmWriteRequest(const std::string& url, const std::string& redactedBody) const
{
    if (m_stWorkingCase.risk == "read") return true;
    std::wstring summary = L"该请求可能修改服务器数据，请确认发送。\r\n\r\n服务器：";
    if (const auto* server = SelectedServer()) summary += mttool::Utf8ToWide(server->name);
    summary += L"\r\nURL：" + mttool::Utf8ToWide(url) + L"\r\n请求摘要：" +
        mttool::Utf8ToWide(redactedBody.substr(0, 500));
    return AfxMessageBox(summary.c_str(), MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES;
}

// UI 线程只完成编辑器提交、协议门禁和请求快照构造；实际 I/O 始终交给工作线程。
// 请求快照按值捕获，防止发送期间切换下拉框改变正在执行的请求。
void CInterfaceTestDlg::OnSend()
{
    if (m_bWorkerRunning.load() || m_strSelectedCaseId.empty()) return;
    if (m_bRawJsonDirty)
    {
        AfxMessageBox(L"原始 JSON 已修改，请先点击“同步到字段表”，确认字段类型和必填规则后再发送。",
            MB_OK | MB_ICONWARNING);
        return;
    }
    std::string error;
    if (!CommitEditors(error))
    {
        AfxMessageBox(mttool::Utf8ToWide(error).c_str(), MB_OK | MB_ICONERROR);
        return;
    }
    const auto support = interface_test::QueryTransportPayloadSupport(SelectedTransport(), SelectedPayload());
    if (!support.implemented)
    {
        SetStatus(L"当前服务/报文组合待开发");
        return;
    }
    const auto* selectedServer = SelectedServer();
    if (selectedServer == nullptr) return;
    interface_test::SessionVariables requestSession = m_stSession;
    std::string body, redactedBody;
    if (!BuildCurrentBody(requestSession, body, redactedBody, error))
    {
        AfxMessageBox(mttool::Utf8ToWide(error).c_str(), MB_OK | MB_ICONERROR);
        return;
    }
    std::vector<std::pair<std::string, std::string>> headers;
    std::string redactedHeaders;
    if (!BuildCurrentHeaders(requestSession, headers, redactedHeaders, error))
    {
        AfxMessageBox(mttool::Utf8ToWide(error).c_str(), MB_OK | MB_ICONERROR);
        return;
    }
    m_stSession.sequence = requestSession.sequence;

    // 服务器级压缩配置属于实际请求的一部分，预览必须与传输层最终发送内容保持一致。
    const bool hasAcceptEncoding = std::any_of(headers.begin(), headers.end(), [](const auto& header) {
        return _stricmp(header.first.c_str(), "Accept-Encoding") == 0;
    });
    if (!selectedServer->acceptEncoding.empty() && !hasAcceptEncoding)
    {
        headers.emplace_back("Accept-Encoding", selectedServer->acceptEncoding);
        redactedHeaders += "Accept-Encoding: " + selectedServer->acceptEncoding + "\r\n";
    }

    if (selectedServer->transport == interface_test::TransportKind::WebSocket)
    {
        if (m_stWorkingCase.receiveOnly)
        {
            SetStatus(L"该 Web 消息为只读服务端通知，不能主动发送");
            return;
        }
        StartWorker([this, body] {
            std::string sendError;
            if (!m_pWebTransport->Send(body, sendError) && !m_bClosing.load())
                PostMessage(kWebStateMessage, 0, reinterpret_cast<LPARAM>(new WebState{false, sendError}));
        });
        SetStatus(L"正在发送 WebSocket JSON 消息...");
        RefreshControlState();
        return;
    }

    const std::string url = interface_test::JoinUrl(*selectedServer, m_stWorkingCase.path);
    if (!ConfirmWriteRequest(url, redactedBody)) return;
    const std::string actual = m_stWorkingCase.method + " " + url + "\r\n" + redactedHeaders + "\r\n" +
        redactedBody;
    SetDlgItemText(IDC_INTERFACE_ACTUAL_REQUEST, mttool::Utf8ToWide(actual).c_str());
    interface_test::HttpRequest request;
    request.server = *selectedServer;
    request.method = m_stWorkingCase.method;
    request.path = m_stWorkingCase.path;
    request.headers = std::move(headers);
    request.body = std::move(body);
    const std::string caseId = m_strSelectedCaseId;
    StartWorker([this, request = std::move(request), caseId] {
        auto* completion = new HttpCompletion;
        completion->caseId = caseId;
        completion->response = m_pHttpTransport->Execute(request);
        if (!m_bClosing.load()) PostMessage(kHttpCompletedMessage, 0, reinterpret_cast<LPARAM>(completion));
        else delete completion;
    });
    SetStatus(L"HTTP 请求执行中...");
    RefreshControlState();
}

// 关键实现：只通知 SocketClient 传输层停止当前请求，后台线程负责形成取消结果并退出。
void CInterfaceTestDlg::OnCancelRequest()
{
    m_pHttpTransport->Cancel();
    SetStatus(L"正在取消 HTTP 请求...");
}

// Web 连接回调只分配深拷贝消息并投递窗口消息，绝不从接收线程直接操作 MFC 控件。
void CInterfaceTestDlg::OnConnectWeb()
{
    if (m_bWorkerRunning.load() || m_pWebTransport->IsConnected()) return;
    const auto* server = SelectedServer();
    if (server == nullptr || server->transport != interface_test::TransportKind::WebSocket) return;
    std::vector<std::pair<std::string, std::string>> headers;
    std::string redactedHeaders, error;
    auto connectSession = m_stSession;
    if (!BuildCurrentHeaders(connectSession, headers, redactedHeaders, error))
    {
        AfxMessageBox(mttool::Utf8ToWide(error).c_str(), MB_OK | MB_ICONERROR);
        return;
    }
    std::string heartbeat;
    const auto heartbeatCase = std::find_if(m_stConfig.cases.begin(), m_stConfig.cases.end(), [server](const auto& item) {
        return item.serverGroup == server->group && item.id == "web-heartbeat";
    });
    if (heartbeatCase != m_stConfig.cases.end())
    {
        std::string redacted;
        if (!interface_test::BuildJsonFromFields(heartbeatCase->fields, heartbeatCase->rootType,
            connectSession, heartbeat, redacted, error))
        {
            AfxMessageBox(mttool::Utf8ToWide(error).c_str(), MB_OK | MB_ICONERROR);
            return;
        }
    }
    const auto serverCopy = *server;
    StartWorker([this, serverCopy, headers = std::move(headers), heartbeat = std::move(heartbeat)] {
        std::string connectError;
        const bool connected = m_pWebTransport->Connect(serverCopy, headers,
            [this](const interface_test::WebMessage& message) {
                if (!m_bClosing.load()) PostMessage(kWebMessage, 0,
                    reinterpret_cast<LPARAM>(new interface_test::WebMessage(message)));
            },
            [this](bool state, const std::string& text) {
                if (!m_bClosing.load()) PostMessage(kWebStateMessage, 0,
                    reinterpret_cast<LPARAM>(new WebState{state, text}));
            }, connectError);
        if (!connected)
        {
            if (!m_bClosing.load()) PostMessage(kWebStateMessage, 0,
                reinterpret_cast<LPARAM>(new WebState{false, connectError}));
            return;
        }
        if (!heartbeat.empty())
        {
            m_pWebTransport->SetHeartbeat(heartbeat, serverCopy.heartbeatMs);
            std::string sendError;
            if (!m_pWebTransport->Send(heartbeat, sendError) && !m_bClosing.load())
                PostMessage(kWebStateMessage, 0, reinterpret_cast<LPARAM>(new WebState{false,
                    "首次心跳/鉴权发送失败: " + sendError}));
        }
    });
    SetStatus(L"正在连接 WebSocket...");
    RefreshControlState();
}

// 关键实现：同步执行传输层的幂等断开，并立即刷新连接相关按钮状态。
void CInterfaceTestDlg::OnDisconnectWeb()
{
    m_pWebTransport->Disconnect();
    SetStatus(L"WebSocket 已由客户端断开；不会自动重连");
    RefreshControlState();
}

// 关键实现：同时清空消息模型、列表行和详情文本，避免视图与内存记录不一致。
void CInterfaceTestDlg::OnClearWeb()
{
    m_vecWebMessages.clear();
    m_clWebList.DeleteAllItems();
    SetDlgItemText(IDC_INTERFACE_WEB_DETAIL, L"");
    SetStatus(L"WebSocket 消息已清空");
}

// 关键实现：直接复用压力测试的 CLogDlg；重复点击只激活同一个窗口。
void CInterfaceTestDlg::OnShowLog()
{
    if (!m_pLogDialog)
    {
        m_pLogDialog = std::make_unique<CLogDlg>(m_clAppLog, this);
        if (!m_pLogDialog->Create(IDD_LOG_DIALOG, this))
        {
            m_pLogDialog.reset();
            SetStatus(L"无法创建日志窗口");
            return;
        }
    }
    m_pLogDialog->ShowWindow(SW_SHOWNORMAL);
    m_pLogDialog->SetForegroundWindow();
}

// 该处理函数运行在 UI 线程，负责接管工作线程分配的结果并更新会话提取变量。
LRESULT CInterfaceTestDlg::OnHttpCompleted(WPARAM, LPARAM parameter)
{
    std::unique_ptr<HttpCompletion> completion(reinterpret_cast<HttpCompletion*>(parameter));
    m_bWorkerRunning.store(false);
    if (m_clWorker.joinable()) m_clWorker.join();
    if (!completion->response.transportSucceeded)
    {
        SetDlgItemText(IDC_INTERFACE_RESPONSE_STATUS, L"传输失败");
        SetDlgItemText(IDC_INTERFACE_RESPONSE, mttool::Utf8ToWide(completion->response.error).c_str());
        SetStatus(L"HTTP 请求失败: " + mttool::Utf8ToWide(completion->response.error));
        RefreshControlState();
        return 0;
    }
    std::wostringstream status;
    status << L"HTTP " << completion->response.statusCode << L"    耗时 " << completion->response.elapsedMs
        << L" ms    请求 " << FormatBytes(completion->response.requestBytes) << L"    响应 "
        << FormatBytes(completion->response.responseBytes);
    SetDlgItemText(IDC_INTERFACE_RESPONSE_STATUS, status.str().c_str());
    m_clAppLog.Add(L"INFO", L"接口完成: " + status.str());
    SetDlgItemText(IDC_INTERFACE_RESPONSE_HEADERS,
        mttool::Utf8ToWide(completion->response.responseHeaders).c_str());
    std::string formatted;
    std::string formatError;
    if (!interface_test::FormatJson(completion->response.responseBody, formatted, formatError))
        formatted = completion->response.responseBody;
    SetDlgItemText(IDC_INTERFACE_RESPONSE, mttool::Utf8ToWide(formatted).c_str());

    if (const auto* item = m_stConfig.FindCase(completion->caseId))
    {
        std::vector<std::string> extracted;
        std::string extractError;
        if (!interface_test::ApplyExtractRules(completion->response.responseBody, item->extracts,
            m_stSession, extracted, extractError))
            SetStatus(L"请求完成，但变量提取失败: " + mttool::Utf8ToWide(extractError));
        else if (!extracted.empty())
            SetStatus(L"请求完成；已提取会话变量 " + mttool::Utf8ToWide(extracted.front()));
        else
            SetStatus(L"HTTP 请求完成");
    }
    RefreshRequestPreview();
    RefreshControlState();
    return 0;
}

// 消息列表只保留配置的上限；删除最旧项后同步重建行索引，避免详情视图越界。
LRESULT CInterfaceTestDlg::OnWebMessage(WPARAM, LPARAM parameter)
{
    std::unique_ptr<interface_test::WebMessage> message(reinterpret_cast<interface_test::WebMessage*>(parameter));
    const auto* server = SelectedServer();
    const size_t limit = server == nullptr ? 5000 : server->maxMessages;
    if (m_vecWebMessages.size() >= limit && !m_vecWebMessages.empty())
    {
        m_vecWebMessages.erase(m_vecWebMessages.begin());
        m_clWebList.DeleteItem(0);
        for (int row = 0; row < m_clWebList.GetItemCount(); ++row)
            m_clWebList.SetItemData(row, static_cast<DWORD_PTR>(row));
    }
    m_vecWebMessages.push_back(*message);
    const int row = m_clWebList.InsertItem(m_clWebList.GetItemCount(), mttool::Utf8ToWide(message->time).c_str());
    m_clWebList.SetItemText(row, 1, mttool::Utf8ToWide(message->direction).c_str());
    m_clWebList.SetItemText(row, 2, mttool::Utf8ToWide(message->funcId).c_str());
    m_clWebList.SetItemText(row, 3, mttool::Utf8ToWide(message->topic).c_str());
    m_clWebList.SetItemText(row, 4, mttool::Utf8ToWide(message->eventType).c_str());
    m_clWebList.SetItemText(row, 5, FormatBytes(message->bytes).c_str());
    m_clWebList.SetItemText(row, 6, mttool::Utf8ToWide(message->summary).c_str());
    m_clWebList.SetItemData(row, static_cast<DWORD_PTR>(m_vecWebMessages.size() - 1));
    return 0;
}

// 关键实现：接管后台线程分配的状态对象，并在连接结束后回收已退出的工作线程。
LRESULT CInterfaceTestDlg::OnWebState(WPARAM, LPARAM parameter)
{
    std::unique_ptr<WebState> state(reinterpret_cast<WebState*>(parameter));
    m_bWorkerRunning.store(false);
    if (m_clWorker.joinable() && !m_pWebTransport->IsConnected()) m_clWorker.join();
    SetStatus(mttool::Utf8ToWide(state->text));
    RefreshControlState();
    return 0;
}

// 关键实现：校验选中行索引后再读取消息模型，防止清空列表期间访问越界。
void CInterfaceTestDlg::OnWebSelectionChanged(NMHDR* header, LRESULT* result)
{
    *result = 0;
    const auto* change = reinterpret_cast<NMLISTVIEW*>(header);
    if ((change->uNewState & LVIS_SELECTED) == 0 || change->iItem < 0) return;
    const size_t index = static_cast<size_t>(m_clWebList.GetItemData(change->iItem));
    if (index >= m_vecWebMessages.size()) return;
    std::string formatted;
    std::string error;
    if (!interface_test::FormatJson(m_vecWebMessages[index].json, formatted, error)) formatted = m_vecWebMessages[index].json;
    SetDlgItemText(IDC_INTERFACE_WEB_DETAIL, mttool::Utf8ToWide(formatted).c_str());
}

// 对话框同一时间只拥有一个短任务线程；启动下一任务前回收上一个已结束线程。
void CInterfaceTestDlg::StartWorker(std::function<void()> work)
{
    if (m_clWorker.joinable()) m_clWorker.join();
    m_bWorkerRunning.store(true);
    m_clWorker = std::thread([this, work = std::move(work)] {
        work();
        m_bWorkerRunning.store(false);
    });
}

// 关闭顺序固定为：阻止新回调、取消 HTTP、断开 Web、等待工作线程。
// OnDestroy、OnCancel 和析构都可重复调用该函数，因此各底层停止操作必须幂等。
void CInterfaceTestDlg::StopWorkers()
{
    m_bClosing.store(true);
    m_pHttpTransport->Cancel();
    m_pWebTransport->Disconnect();
    if (m_clWorker.joinable()) m_clWorker.join();
    m_bWorkerRunning.store(false);
}

// 关键实现：窗口销毁前停止 HTTP、WebSocket 和心跳线程，确保没有回调访问失效 HWND。
void CInterfaceTestDlg::OnDestroy()
{
    StopWorkers();
    m_clAppLog.Add(L"INFO", L"接口测试窗口关闭，网络线程已回收");
    if (m_pLogDialog && ::IsWindow(m_pLogDialog->GetSafeHwnd())) m_pLogDialog->DestroyWindow();
    m_pLogDialog.reset();
    CDialogEx::OnDestroy();
}

// 关键实现：只有在未保存修改得到处理后才停止后台任务并关闭对话框。
void CInterfaceTestDlg::OnCancel()
{
    if (!ConfirmPendingChanges()) return;
    StopWorkers();
    CDialogEx::OnCancel();
}

// 关键实现：状态文本只由界面线程写入，后台线程必须通过窗口消息转交更新。
void CInterfaceTestDlg::SetStatus(const std::wstring& text)
{
    SetDlgItemText(IDC_INTERFACE_STATUS, text.c_str());
    const bool failed = text.find(L"失败") != std::wstring::npos ||
        text.find(L"错误") != std::wstring::npos || text.find(L"无法") != std::wstring::npos;
    const bool warning = !failed && (text.find(L"取消") != std::wstring::npos ||
        text.find(L"待开发") != std::wstring::npos || text.find(L"不能") != std::wstring::npos);
    m_clAppLog.Add(failed ? L"ERROR" : (warning ? L"WARN" : L"INFO"), text);
}

// 关键实现：通过与下拉项并行的 ID 数组定位选项，避免把显示文本当成稳定主键。
void CInterfaceTestDlg::SelectComboId(CComboBox& combo, const std::vector<std::string>& ids, const std::string& id)
{
    const auto it = std::find(ids.begin(), ids.end(), id);
    combo.SetCurSel(it == ids.end() ? CB_ERR : static_cast<int>(it - ids.begin()));
}

// 关键实现：先校验界面索引，再由配置模型按稳定 ID 返回服务器定义。
const interface_test::ServerProfile* CInterfaceTestDlg::SelectedServer() const
{
    const int selection = m_clServerCombo.GetCurSel();
    if (selection < 0 || selection >= static_cast<int>(m_vecServerIds.size())) return nullptr;
    return m_stConfig.FindServer(m_vecServerIds[selection]);
}

// 关键实现：无有效选择时回退到 HTTP，保证刷新过程中的临时空选项不会产生非法枚举。
interface_test::TransportKind CInterfaceTestDlg::SelectedTransport() const
{
    const int selection = m_clTransportCombo.GetCurSel();
    if (selection < 0 || selection >= static_cast<int>(m_vecTransportValues.size()))
        return interface_test::TransportKind::Http;
    return m_vecTransportValues[selection];
}

// 关键实现：无有效选择时回退到 JSON，保持当前已实现功能的安全默认值。
interface_test::PayloadKind CInterfaceTestDlg::SelectedPayload() const
{
    const int selection = m_clPayloadCombo.GetCurSel();
    if (selection < 0 || selection >= static_cast<int>(m_vecPayloadValues.size()))
        return interface_test::PayloadKind::Json;
    return m_vecPayloadValues[selection];
}
