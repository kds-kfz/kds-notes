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
      httpTransport_(std::make_unique<interface_test::SocketHttpTransport>()),
      webTransport_(std::make_unique<interface_test::SocketWebTransport>())
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
    DDX_Control(exchange, IDC_INTERFACE_ENV, environmentCombo_);
    DDX_Control(exchange, IDC_INTERFACE_TRANSPORT, transportCombo_);
    DDX_Control(exchange, IDC_INTERFACE_SERVER, serverCombo_);
    DDX_Control(exchange, IDC_INTERFACE_PROTOCOL, payloadCombo_);
    DDX_Control(exchange, IDC_INTERFACE_VARIABLE_SET, variableSetCombo_);
    DDX_Control(exchange, IDC_INTERFACE_CATEGORY, categoryCombo_);
    DDX_Control(exchange, IDC_INTERFACE_CASE, caseCombo_);
    DDX_Control(exchange, IDC_INTERFACE_FIELD_TYPE, fieldTypeCombo_);
    DDX_Control(exchange, IDC_INTERFACE_FIELD_LIST, fieldList_);
    DDX_Control(exchange, IDC_INTERFACE_WEB_LIST, webList_);
}

// 关键实现：初始化列表后从 EXE/conf 加载独立配置，失败仅提示而不退出工具。
BOOL CInterfaceTestDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    SetIcon(AfxGetApp()->LoadIcon(IDI_MTLOADTEST_ICON), TRUE);
    SetIcon(AfxGetApp()->LoadIcon(IDI_MTLOADTEST_ICON), FALSE);
    std::wstring logError;
    if (appLog_.Initialize(ExecutableDirectory(), logError))
        appLog_.Add(L"INFO", L"接口测试窗口启动，日志文件=" + appLog_.FilePath());
    else
        appLog_.Add(L"ERROR", L"接口测试日志初始化失败: " + logError);
    InitializeLists();
    configPath_ = ExecutableDirectory() + L"\\conf\\interface-test.xml";
    SetDlgItemText(IDC_INTERFACE_CONFIG_PATH, configPath_.c_str());
    if (!LoadConfiguration(configPath_))
        SetStatus(L"请选择并加载 interface-test.xml");
    return TRUE;
}

// 关键实现：按业务阅读顺序创建字段列和 WebSocket 消息列。
void CInterfaceTestDlg::InitializeLists()
{
    fieldList_.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);
    fieldList_.InsertColumn(0, L"启用", LVCFMT_CENTER, 48);
    fieldList_.InsertColumn(1, L"JSON Pointer", LVCFMT_LEFT, 145);
    fieldList_.InsertColumn(2, L"类型", LVCFMT_LEFT, 70);
    fieldList_.InsertColumn(3, L"必填", LVCFMT_CENTER, 48);
    fieldList_.InsertColumn(4, L"值", LVCFMT_LEFT, 145);
    fieldList_.InsertColumn(5, L"说明", LVCFMT_LEFT, 220);
    ResizeFieldColumns();

    webList_.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    webList_.InsertColumn(0, L"时间", LVCFMT_LEFT, 90);
    webList_.InsertColumn(1, L"方向", LVCFMT_CENTER, 48);
    webList_.InsertColumn(2, L"FuncId", LVCFMT_LEFT, 58);
    webList_.InsertColumn(3, L"Topic", LVCFMT_LEFT, 52);
    webList_.InsertColumn(4, L"EventType", LVCFMT_LEFT, 70);
    webList_.InsertColumn(5, L"大小", LVCFMT_RIGHT, 70);
    webList_.InsertColumn(6, L"摘要", LVCFMT_LEFT, 380);
    ResizeWebColumns();

    const wchar_t* types[] = {L"string", L"integer", L"unsigned", L"number", L"boolean", L"object", L"array", L"null"};
    for (const wchar_t* type : types)
        fieldTypeCombo_.AddString(type);
}

// 字段列按当前列表客户区分配，避免类型和值截断，并让说明列吸收全部剩余宽度。
// 窄窗口保留 900 像素的可读基线和水平滚动，防止为铺满而把字段压缩到不可读。
void CInterfaceTestDlg::ResizeFieldColumns()
{
    if (fieldList_.GetSafeHwnd() == nullptr || fieldList_.GetHeaderCtrl() == nullptr ||
        fieldList_.GetHeaderCtrl()->GetItemCount() != 6)
        return;

    CRect client;
    fieldList_.GetClientRect(&client);
    const int available = std::max(client.Width() - ::GetSystemMetrics(SM_CXVSCROLL) - 2, 900);
    const int weights[] = {6, 18, 11, 7, 23};
    int used = 0;
    for (int column = 0; column < static_cast<int>(_countof(weights)); ++column)
    {
        const int width = available * weights[column] / 100;
        fieldList_.SetColumnWidth(column, width);
        used += width;
    }
    fieldList_.SetColumnWidth(5, std::max(available - used, 1));
}

// Web 列宽按当前列表客户区分配，避免高 DPI 下标题截断或末尾留下空白列区域。
// 窄窗口使用 800 像素的可读基线并保留横向滚动，宽窗口则完整铺满。
void CInterfaceTestDlg::ResizeWebColumns()
{
    if (webList_.GetSafeHwnd() == nullptr || webList_.GetHeaderCtrl() == nullptr ||
        webList_.GetHeaderCtrl()->GetItemCount() != 7)
        return;

    CRect client;
    webList_.GetClientRect(&client);
    const int available = std::max(client.Width() - ::GetSystemMetrics(SM_CXVSCROLL) - 2, 800);
    const int weights[] = {15, 8, 10, 12, 14, 10};
    int used = 0;
    for (int column = 0; column < static_cast<int>(_countof(weights)); ++column)
    {
        const int width = available * weights[column] / 100;
        webList_.SetColumnWidth(column, width);
        used += width;
    }
    webList_.SetColumnWidth(6, std::max(available - used, 1));
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
    config_ = std::move(loaded);
    configPath_ = path;
    selectedCaseId_.clear();
    dirty_ = false;
    rawJsonDirty_ = false;
    session_ = {};
    SetDlgItemText(IDC_INTERFACE_CONFIG_PATH, configPath_.c_str());
    refreshing_ = true;
    PopulateEnvironments();
    PopulateTransports();
    PopulateServers();
    PopulatePayloads();
    PopulateVariableSets();
    PopulateCategories();
    PopulateCases();
    refreshing_ = false;
    LoadSelectedCase();
    SetStatus(L"配置已加载；请选择服务、协议和接口后发送请求");
    return true;
}

// 关键实现：重建显示项和稳定 ID 并行数组，优先选择配置默认环境。
void CInterfaceTestDlg::PopulateEnvironments()
{
    environmentCombo_.ResetContent();
    environmentIds_.clear();
    for (const auto& environment : config_.environments)
    {
        environmentCombo_.AddString(mttool::Utf8ToWide(environment.name).c_str());
        environmentIds_.push_back(environment.id);
    }
    SelectComboId(environmentCombo_, environmentIds_, config_.defaultEnvironmentId);
    if (environmentCombo_.GetCurSel() == CB_ERR && environmentCombo_.GetCount() > 0)
        environmentCombo_.SetCurSel(0);
}

// 关键实现：根据当前环境实际服务器去重服务类型，避免展示无可选服务器的入口。
void CInterfaceTestDlg::PopulateTransports()
{
    const int previous = transportCombo_.GetCurSel();
    interface_test::TransportKind previousValue = interface_test::TransportKind::Http;
    if (previous >= 0 && previous < static_cast<int>(transportValues_.size()))
        previousValue = transportValues_[previous];
    transportCombo_.ResetContent();
    transportValues_.clear();
    const int environmentIndex = environmentCombo_.GetCurSel();
    const std::string environmentId = environmentIndex >= 0 && environmentIndex < static_cast<int>(environmentIds_.size()) ?
        environmentIds_[environmentIndex] : std::string();
    const interface_test::TransportKind values[] = {interface_test::TransportKind::Tcp,
        interface_test::TransportKind::Http, interface_test::TransportKind::Https,
        interface_test::TransportKind::WebSocket};
    for (auto value : values)
    {
        const bool configured = std::any_of(config_.servers.begin(), config_.servers.end(),
            [value, &environmentId](const auto& server) {
                return server.environmentId == environmentId && server.transport == value;
            });
        if (!configured && value != interface_test::TransportKind::Tcp)
            continue;
        transportCombo_.AddString(mttool::Utf8ToWide(interface_test::ToDisplayName(value)).c_str());
        transportValues_.push_back(value);
    }
    auto it = std::find(transportValues_.begin(), transportValues_.end(), previousValue);
    if (it == transportValues_.end())
        it = std::find(transportValues_.begin(), transportValues_.end(), interface_test::TransportKind::Http);
    transportCombo_.SetCurSel(it == transportValues_.end() ? 0 : static_cast<int>(it - transportValues_.begin()));
}

// 关键实现：同时按环境和服务类型筛选服务器，并重建 ID 映射。
void CInterfaceTestDlg::PopulateServers()
{
    serverCombo_.ResetContent();
    serverIds_.clear();
    const int environmentIndex = environmentCombo_.GetCurSel();
    if (environmentIndex < 0 || environmentIndex >= static_cast<int>(environmentIds_.size())) return;
    const std::string& environmentId = environmentIds_[environmentIndex];
    const auto transport = SelectedTransport();
    for (const auto& server : config_.servers)
    {
        if (server.environmentId != environmentId || server.transport != transport) continue;
        std::string label = server.name + "  " + server.scheme + "://" + server.host;
        if (server.hasExplicitPort) label += ":" + std::to_string(server.port);
        serverCombo_.AddString(mttool::Utf8ToWide(label).c_str());
        serverIds_.push_back(server.id);
    }
    if (serverCombo_.GetCount() > 0) serverCombo_.SetCurSel(0);
}

// 关键实现：合并服务器声明和能力矩阵，非法组合不进入下拉，待开发组合明确标注。
void CInterfaceTestDlg::PopulatePayloads()
{
    payloadCombo_.ResetContent();
    payloadValues_.clear();
    const interface_test::ServerProfile* server = SelectedServer();
    std::vector<interface_test::PayloadKind> values;
    if (server != nullptr) values = server->payloads;
    if (values.empty()) values = {interface_test::PayloadKind::Binary, interface_test::PayloadKind::Json,
        interface_test::PayloadKind::Protobuf};
    for (auto value : values)
    {
        const auto support = interface_test::QueryTransportPayloadSupport(SelectedTransport(), value);
        if (!support.allowed) continue;
        payloadCombo_.AddString(mttool::Utf8ToWide(support.label).c_str());
        payloadValues_.push_back(value);
    }
    auto json = std::find(payloadValues_.begin(), payloadValues_.end(), interface_test::PayloadKind::Json);
    payloadCombo_.SetCurSel(json == payloadValues_.end() ? 0 : static_cast<int>(json - payloadValues_.begin()));
}

// 关键实现：重建变量组列表并选择配置默认值，变量内容不显示在下拉文本中。
void CInterfaceTestDlg::PopulateVariableSets()
{
    variableSetCombo_.ResetContent();
    variableSetIds_.clear();
    for (const auto& set : config_.variableSets)
    {
        variableSetCombo_.AddString(mttool::Utf8ToWide(set.name).c_str());
        variableSetIds_.push_back(set.id);
    }
    SelectComboId(variableSetCombo_, variableSetIds_, config_.defaultVariableSetId);
    if (variableSetCombo_.GetCurSel() == CB_ERR && variableSetCombo_.GetCount() > 0)
        variableSetCombo_.SetCurSel(0);
    ResetSessionVariables();
}

// 关键实现：只汇总当前服务器组和报文协议可用用例的分类并去重。
void CInterfaceTestDlg::PopulateCategories()
{
    categoryCombo_.ResetContent();
    categoryValues_.clear();
    const auto* server = SelectedServer();
    const auto payload = SelectedPayload();
    std::set<std::string> categories;
    if (server != nullptr)
    {
        for (const auto& item : config_.cases)
            if (item.serverGroup == server->group && item.payload == payload)
                categories.insert(item.category);
    }
    categoryCombo_.AddString(L"全部分类");
    categoryValues_.push_back({});
    for (const auto& category : categories)
    {
        categoryCombo_.AddString(mttool::Utf8ToWide(category).c_str());
        categoryValues_.push_back(category);
    }
    categoryCombo_.SetCurSel(0);
}

// 关键实现：按服务器组、协议和分类筛选用例，并尽量保持原 selectedCaseId_。
void CInterfaceTestDlg::PopulateCases()
{
    const std::string previousId = selectedCaseId_;
    caseCombo_.ResetContent();
    caseIds_.clear();
    const auto* server = SelectedServer();
    const auto payload = SelectedPayload();
    const int categoryIndex = categoryCombo_.GetCurSel();
    const std::string category = categoryIndex >= 0 && categoryIndex < static_cast<int>(categoryValues_.size()) ?
        categoryValues_[categoryIndex] : std::string();
    if (server != nullptr)
    {
        for (const auto& item : config_.cases)
        {
            if (item.serverGroup != server->group || item.payload != payload ||
                (!category.empty() && item.category != category)) continue;
            caseCombo_.AddString(mttool::Utf8ToWide(item.name).c_str());
            caseIds_.push_back(item.id);
        }
    }
    SelectComboId(caseCombo_, caseIds_, previousId);
    if (caseCombo_.GetCurSel() == CB_ERR && caseCombo_.GetCount() > 0) caseCombo_.SetCurSel(0);
}

// 关键实现：切换时复制配置用例形成工作副本，后续编辑不直接污染 config_。
void CInterfaceTestDlg::LoadSelectedCase()
{
    const int selection = caseCombo_.GetCurSel();
    if (selection < 0 || selection >= static_cast<int>(caseIds_.size()))
    {
        selectedCaseId_.clear();
        workingCase_ = {};
        RefreshCaseDisplay();
        return;
    }
    selectedCaseId_ = caseIds_[selection];
    if (const auto* item = config_.FindCase(selectedCaseId_)) workingCase_ = *item;
    dirty_ = false;
    RefreshCaseDisplay();
}

// 关键实现：在 refreshing_ 门禁内批量更新说明、方法、路径、字段和 JSON。
void CInterfaceTestDlg::RefreshCaseDisplay()
{
    refreshing_ = true;
    SetDlgItemText(IDC_INTERFACE_METHOD, mttool::Utf8ToWide(workingCase_.method).c_str());
    SetDlgItemText(IDC_INTERFACE_PATH, mttool::Utf8ToWide(workingCase_.path).c_str());
    const std::string description = "功能：" + workingCase_.functionDescription + "\r\n使用场景：" +
        workingCase_.usageDescription + "\r\n鉴权：" + workingCase_.authDescription +
        "    副作用：" + workingCase_.sideEffectDescription;
    SetDlgItemText(IDC_INTERFACE_DESCRIPTION, mttool::Utf8ToWide(description).c_str());
    std::string headers;
    for (const auto& header : workingCase_.headers)
        headers += header.name + ": " + header.value + "\r\n";
    SetDlgItemText(IDC_INTERFACE_HEADERS, mttool::Utf8ToWide(headers).c_str());
    RefreshFieldList();
    RefreshRequestPreview();
    SetDlgItemText(IDC_INTERFACE_RESPONSE_STATUS, L"");
    SetDlgItemText(IDC_INTERFACE_RESPONSE_HEADERS, L"");
    SetDlgItemText(IDC_INTERFACE_RESPONSE, L"");
    refreshing_ = false;
    RefreshControlState();
}

// 关键实现：字段表完整重建后恢复复选状态，必填字段仍显示但不能关闭。
void CInterfaceTestDlg::RefreshFieldList()
{
    refreshing_ = true;
    fieldList_.DeleteAllItems();
    for (size_t i = 0; i < workingCase_.fields.size(); ++i)
    {
        const auto& field = workingCase_.fields[i];
        const int row = fieldList_.InsertItem(static_cast<int>(i), L"");
        fieldList_.SetCheck(row, field.required || field.enabled);
        fieldList_.SetItemText(row, 1, mttool::Utf8ToWide(field.pointer).c_str());
        fieldList_.SetItemText(row, 2, mttool::Utf8ToWide(field.type).c_str());
        fieldList_.SetItemText(row, 3, field.required ? L"是" : L"否");
        fieldList_.SetItemText(row, 4, field.sensitive ? L"******" : mttool::Utf8ToWide(field.value).c_str());
        fieldList_.SetItemText(row, 5, mttool::Utf8ToWide(field.description).c_str());
        fieldList_.SetItemData(row, static_cast<DWORD_PTR>(i));
    }
    SetDlgItemText(IDC_INTERFACE_FIELD_PATH, L"");
    SetDlgItemText(IDC_INTERFACE_FIELD_VALUE, L"");
    fieldTypeCombo_.SetCurSel(CB_ERR);
    refreshing_ = false;
}

// 关键实现：使用 SessionVariables 副本生成预览，避免界面刷新消耗正式 sequence。
void CInterfaceTestDlg::RefreshRequestPreview()
{
    const bool previousRefreshing = refreshing_;
    refreshing_ = true;
    if (selectedCaseId_.empty())
    {
        SetDlgItemText(IDC_INTERFACE_REQUEST, L"");
        SetDlgItemText(IDC_INTERFACE_ACTUAL_REQUEST, L"");
        rawJsonDirty_ = false;
        refreshing_ = previousRefreshing;
        return;
    }
    auto previewSession = session_;
    std::string body;
    std::string redacted;
    std::string error;
    if (!BuildCurrentBody(previewSession, body, redacted, error))
    {
        SetDlgItemText(IDC_INTERFACE_REQUEST, mttool::Utf8ToWide("JSON 预览失败: " + error).c_str());
        rawJsonDirty_ = false;
        refreshing_ = previousRefreshing;
        return;
    }
    SetDlgItemText(IDC_INTERFACE_REQUEST, mttool::Utf8ToWide(redacted).c_str());
    const auto* server = SelectedServer();
    if (server != nullptr)
    {
        std::vector<std::pair<std::string, std::string>> headers;
        std::string redactedHeaders;
        auto headerSession = session_;
        BuildCurrentHeaders(headerSession, headers, redactedHeaders, error);
        const std::string actual = workingCase_.method + " " + interface_test::JoinUrl(*server,
            server->transport == interface_test::TransportKind::WebSocket ? server->webSocketPath : workingCase_.path) +
            "\r\n" + redactedHeaders + "\r\n" + redacted;
        SetDlgItemText(IDC_INTERFACE_ACTUAL_REQUEST, mttool::Utf8ToWide(actual).c_str());
    }
    rawJsonDirty_ = false;
    refreshing_ = previousRefreshing;
}

// 关键实现：把协议实现状态、工作线程状态和 WebSocket 连接状态统一映射到按钮。
void CInterfaceTestDlg::RefreshControlState()
{
    const auto support = interface_test::QueryTransportPayloadSupport(SelectedTransport(), SelectedPayload());
    const bool hasCase = !selectedCaseId_.empty();
    const bool web = SelectedTransport() == interface_test::TransportKind::WebSocket;
    const bool busy = workerRunning_.load();
    GetDlgItem(IDC_INTERFACE_SEND)->EnableWindow(hasCase && support.implemented && !busy &&
        (!web || webTransport_->IsConnected()) && !workingCase_.receiveOnly);
    GetDlgItem(IDC_INTERFACE_CANCEL)->EnableWindow(busy && !web);
    GetDlgItem(IDC_INTERFACE_CONNECT)->EnableWindow(web && support.implemented && !busy && !webTransport_->IsConnected());
    GetDlgItem(IDC_INTERFACE_DISCONNECT)->EnableWindow(web && webTransport_->IsConnected());
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
    session_ = {};
    const int selection = variableSetCombo_.GetCurSel();
    if (selection < 0 || selection >= static_cast<int>(variableSetIds_.size())) return;
    const auto* set = config_.FindVariableSet(variableSetIds_[selection]);
    if (set == nullptr) return;
    for (const auto& variable : set->variables)
    {
        session_.values[variable.name] = variable.value;
        if (variable.sensitive) session_.sensitiveNames.insert(variable.name);
    }
}

// 关键实现：委托 JsonMessage 从字段表同时构造真实正文和脱敏正文。
bool CInterfaceTestDlg::BuildCurrentBody(interface_test::SessionVariables& session, std::string& body,
    std::string& redactedBody, std::string& error) const
{
    if (workingCase_.fields.empty())
    {
        body.clear();
        redactedBody.clear();
        return true;
    }
    return interface_test::BuildJsonFromFields(workingCase_.fields, workingCase_.rootType,
        session, body, redactedBody, error);
}

// 关键实现：逐个展开请求头，必填空值立即失败，敏感值只进入脱敏预览。
bool CInterfaceTestDlg::BuildCurrentHeaders(interface_test::SessionVariables& session,
    std::vector<std::pair<std::string, std::string>>& headers,
    std::string& redactedHeaders, std::string& error) const
{
    headers.clear();
    redactedHeaders.clear();
    for (const auto& header : workingCase_.headers)
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
    workingCase_.method = mttool::WideToUtf8(Trim(std::wstring(ControlText(this, IDC_INTERFACE_METHOD).GetString())));
    workingCase_.path = mttool::WideToUtf8(Trim(std::wstring(ControlText(this, IDC_INTERFACE_PATH).GetString())));
    if (workingCase_.method.empty())
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
            auto old = std::find_if(workingCase_.headers.begin(), workingCase_.headers.end(), [&header](const auto& item) {
                return EqualAsciiNoCase(item.name, header.name);
            });
            if (old != workingCase_.headers.end())
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
    for (const auto& old : workingCase_.headers)
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
    workingCase_.headers = std::move(parsedHeaders);
    return true;
}

// 关键实现：脏工作副本提供保存、放弃、取消三路选择，取消时阻止当前切换。
bool CInterfaceTestDlg::ConfirmPendingChanges()
{
    if (!dirty_) return true;
    const int answer = AfxMessageBox(L"当前接口配置尚未保存。\r\n\r\n是：保存后继续\r\n否：放弃修改\r\n取消：留在当前接口",
        MB_YESNOCANCEL | MB_ICONQUESTION);
    if (answer == IDCANCEL) return false;
    if (answer == IDYES)
    {
        OnSave();
        return !dirty_;
    }
    dirty_ = false;
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
        configPath_ = dialog.GetPathName().GetString();
        SetDlgItemText(IDC_INTERFACE_CONFIG_PATH, configPath_.c_str());
        LoadConfiguration(configPath_);
    }
}

// 关键实现：先确认当前未保存内容，再按路径执行完整配置替换。
void CInterfaceTestDlg::OnLoad()
{
    if (!ConfirmPendingChanges()) return;
    const std::wstring path = ControlText(this, IDC_INTERFACE_CONFIG_PATH).GetString();
    LoadConfiguration(path);
}

// 关键实现：先提交工作副本到 config_，再通过 SaveAtomic 持久化并清除脏标记。
void CInterfaceTestDlg::OnSave()
{
    if (selectedCaseId_.empty()) return;
    if (rawJsonDirty_)
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
    if (auto* target = config_.FindCase(selectedCaseId_)) *target = workingCase_;
    if (!config_.SaveAtomic(mttool::WideToUtf8(configPath_), error))
    {
        SetStatus(mttool::Utf8ToWide(error));
        AfxMessageBox(mttool::Utf8ToWide(error).c_str(), MB_OK | MB_ICONERROR);
        return;
    }
    dirty_ = false;
    SetStatus(L"接口测试配置已原子保存");
}

// 关键实现：环境变化按服务器、协议、分类、用例顺序级联刷新。
void CInterfaceTestDlg::OnEnvironmentChanged()
{
    if (refreshing_ || !ConfirmPendingChanges()) return;
    refreshing_ = true;
    PopulateTransports(); PopulateServers(); PopulatePayloads(); PopulateCategories(); PopulateCases();
    refreshing_ = false;
    ResetSessionVariables();
    LoadSelectedCase();
}

// 关键实现：服务类型变化后重新建立合法服务器和报文组合。
void CInterfaceTestDlg::OnTransportChanged()
{
    if (refreshing_ || !ConfirmPendingChanges()) return;
    refreshing_ = true;
    PopulateServers(); PopulatePayloads(); PopulateCategories(); PopulateCases();
    refreshing_ = false;
    ResetSessionVariables();
    LoadSelectedCase();
}

// 关键实现：服务器变化后更新报文能力和适用用例，不复用旧工作副本。
void CInterfaceTestDlg::OnServerChanged()
{
    if (refreshing_ || !ConfirmPendingChanges()) return;
    refreshing_ = true;
    PopulatePayloads(); PopulateCategories(); PopulateCases();
    refreshing_ = false;
    ResetSessionVariables();
    LoadSelectedCase();
}

// 关键实现：报文变化只刷新匹配分类和用例，并重新计算控件可用性。
void CInterfaceTestDlg::OnPayloadChanged()
{
    if (refreshing_ || !ConfirmPendingChanges()) return;
    refreshing_ = true;
    PopulateCategories(); PopulateCases();
    refreshing_ = false;
    LoadSelectedCase();
}

// 关键实现：分类筛选只影响用例列表，不修改配置模型。
void CInterfaceTestDlg::OnCategoryChanged()
{
    if (refreshing_ || !ConfirmPendingChanges()) return;
    refreshing_ = true; PopulateCases(); refreshing_ = false; LoadSelectedCase();
}

// 关键实现：切换前处理未保存内容；用户取消时恢复原用例选择。
void CInterfaceTestDlg::OnCaseChanged()
{
    if (refreshing_) return;
    const int selection = caseCombo_.GetCurSel();
    if (selection < 0 || selection >= static_cast<int>(caseIds_.size())) return;
    const std::string requested = caseIds_[selection];
    if (requested != selectedCaseId_ && !ConfirmPendingChanges())
    {
        SelectComboId(caseCombo_, caseIds_, selectedCaseId_);
        return;
    }
    LoadSelectedCase();
}

// 关键实现：切换账号后清空运行时提取值并重新生成脱敏请求预览。
void CInterfaceTestDlg::OnVariableSetChanged()
{
    if (refreshing_) return;
    ResetSessionVariables();
    RefreshRequestPreview();
    SetStatus(L"已切换变量组；运行时 Token 已清除");
}

// 关键实现：忽略程序刷新通知，仅把真实用户编辑标记为 dirty_。
void CInterfaceTestDlg::OnEditorChanged()
{
    if (refreshing_) return;
    const UINT controlId = static_cast<UINT>(GetFocus() == nullptr ? 0 : GetFocus()->GetDlgCtrlID());
    if (controlId == IDC_INTERFACE_REQUEST)
        rawJsonDirty_ = true;
    dirty_ = true;
}

// 关键实现：处理复选变化前校验行索引，必填字段被关闭时立即恢复选中。
void CInterfaceTestDlg::OnFieldChanged(NMHDR* header, LRESULT* result)
{
    *result = 0;
    if (refreshing_) return;
    const auto* change = reinterpret_cast<NMLISTVIEW*>(header);
    if ((change->uNewState & LVIS_SELECTED) != 0)
        OnFieldSelectionChanged(header, result);
    if ((change->uChanged & LVIF_STATE) == 0 || change->iItem < 0) return;
    const size_t index = static_cast<size_t>(fieldList_.GetItemData(change->iItem));
    if (index >= workingCase_.fields.size()) return;
    auto& field = workingCase_.fields[index];
    const bool checked = fieldList_.GetCheck(change->iItem) != FALSE;
    if (field.required && !checked)
    {
        refreshing_ = true;
        fieldList_.SetCheck(change->iItem, TRUE);
        refreshing_ = false;
        SetStatus(L"必填字段不能关闭");
        return;
    }
    if (field.enabled != checked)
    {
        field.enabled = checked;
        dirty_ = true;
        RefreshRequestPreview();
    }
}

// 关键实现：只在新选中行有效时回填字段编辑器，避免空选择覆盖输入。
void CInterfaceTestDlg::OnFieldSelectionChanged(NMHDR* header, LRESULT* result)
{
    *result = 0;
    if (refreshing_) return;
    const auto* change = reinterpret_cast<NMLISTVIEW*>(header);
    if ((change->uNewState & LVIS_SELECTED) == 0 || change->iItem < 0) return;
    const size_t index = static_cast<size_t>(fieldList_.GetItemData(change->iItem));
    if (index >= workingCase_.fields.size()) return;
    const auto& field = workingCase_.fields[index];
    SetDlgItemText(IDC_INTERFACE_FIELD_PATH, mttool::Utf8ToWide(field.pointer).c_str());
    SetDlgItemText(IDC_INTERFACE_FIELD_VALUE, mttool::Utf8ToWide(field.value).c_str());
    const int typeIndex = FindComboText(fieldTypeCombo_, mttool::Utf8ToWide(field.type));
    fieldTypeCombo_.SetCurSel(typeIndex);
}

// 关键实现：校验 Pointer 唯一性和类型后更新选中字段，再同步原始 JSON。
void CInterfaceTestDlg::OnApplyField()
{
    POSITION position = fieldList_.GetFirstSelectedItemPosition();
    if (position == nullptr) return;
    const int row = fieldList_.GetNextSelectedItem(position);
    const size_t index = static_cast<size_t>(fieldList_.GetItemData(row));
    if (index >= workingCase_.fields.size()) return;
    auto& field = workingCase_.fields[index];
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
    const int typeIndex = fieldTypeCombo_.GetCurSel();
    if (typeIndex != CB_ERR) fieldTypeCombo_.GetLBText(typeIndex, type);
    if (!type.IsEmpty()) field.type = mttool::WideToUtf8(type.GetString());
    field.value = mttool::WideToUtf8(ControlText(this, IDC_INTERFACE_FIELD_VALUE).GetString());
    field.enabled = true;
    dirty_ = true;
    rawJsonDirty_ = false;
    RefreshFieldList();
    RefreshRequestPreview();
}

// 关键实现：生成不冲突的默认 Pointer，新字段始终为可选且启用。
void CInterfaceTestDlg::OnAddField()
{
    std::set<std::string> paths;
    for (const auto& field : workingCase_.fields) paths.insert(field.pointer);
    unsigned int number = 1;
    std::string pointer;
    do { pointer = "/NewField" + std::to_string(number++); } while (paths.count(pointer) != 0);
    interface_test::FieldDefinition field;
    field.pointer = pointer;
    field.description = "界面新增可选字段";
    field.enabled = true;
    workingCase_.fields.push_back(std::move(field));
    dirty_ = true;
    RefreshFieldList();
    const int row = static_cast<int>(workingCase_.fields.size() - 1);
    fieldList_.SetItemState(row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    fieldList_.EnsureVisible(row, FALSE);
    RefreshRequestPreview();
}

// 关键实现：仅允许删除当前可选字段，必填字段保留契约门禁。
void CInterfaceTestDlg::OnDeleteField()
{
    POSITION position = fieldList_.GetFirstSelectedItemPosition();
    if (position == nullptr) return;
    const int row = fieldList_.GetNextSelectedItem(position);
    const size_t index = static_cast<size_t>(fieldList_.GetItemData(row));
    if (index >= workingCase_.fields.size()) return;
    if (workingCase_.fields[index].required)
    {
        AfxMessageBox(L"必填字段不能删除。", MB_OK | MB_ICONWARNING);
        return;
    }
    workingCase_.fields.erase(workingCase_.fields.begin() + index);
    dirty_ = true;
    RefreshFieldList();
    RefreshRequestPreview();
}

// 关键实现：完整解析原始 JSON 后才替换字段状态，失败保留用户文本供修正。
void CInterfaceTestDlg::OnApplyJson()
{
    const std::string raw = mttool::WideToUtf8(ControlText(this, IDC_INTERFACE_REQUEST).GetString());
    const auto oldFields = workingCase_.fields;
    std::string error;
    if (!interface_test::SynchronizeFieldsFromJson(raw, workingCase_.fields, workingCase_.rootType, error))
    {
        AfxMessageBox(mttool::Utf8ToWide(error).c_str(), MB_OK | MB_ICONERROR);
        return;
    }
    for (const auto& old : oldFields)
    {
        if (!old.sensitive) continue;
        auto current = std::find_if(workingCase_.fields.begin(), workingCase_.fields.end(), [&old](const auto& item) {
            return item.pointer == old.pointer;
        });
        if (current != workingCase_.fields.end() && current->value == "******")
        {
            current->value = old.value;
            current->type = old.type;
            current->sensitive = true;
        }
    }
    dirty_ = true;
    rawJsonDirty_ = false;
    RefreshFieldList();
    RefreshRequestPreview();
}

// 关键实现：只对非 read 风险用例弹出包含 URL 和脱敏摘要的确认框。
bool CInterfaceTestDlg::ConfirmWriteRequest(const std::string& url, const std::string& redactedBody) const
{
    if (workingCase_.risk == "read") return true;
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
    if (workerRunning_.load() || selectedCaseId_.empty()) return;
    if (rawJsonDirty_)
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
    interface_test::SessionVariables requestSession = session_;
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
    session_.sequence = requestSession.sequence;

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
        if (workingCase_.receiveOnly)
        {
            SetStatus(L"该 Web 消息为只读服务端通知，不能主动发送");
            return;
        }
        StartWorker([this, body] {
            std::string sendError;
            if (!webTransport_->Send(body, sendError) && !closing_.load())
                PostMessage(kWebStateMessage, 0, reinterpret_cast<LPARAM>(new WebState{false, sendError}));
        });
        SetStatus(L"正在发送 WebSocket JSON 消息...");
        RefreshControlState();
        return;
    }

    const std::string url = interface_test::JoinUrl(*selectedServer, workingCase_.path);
    if (!ConfirmWriteRequest(url, redactedBody)) return;
    const std::string actual = workingCase_.method + " " + url + "\r\n" + redactedHeaders + "\r\n" +
        redactedBody;
    SetDlgItemText(IDC_INTERFACE_ACTUAL_REQUEST, mttool::Utf8ToWide(actual).c_str());
    interface_test::HttpRequest request;
    request.server = *selectedServer;
    request.method = workingCase_.method;
    request.path = workingCase_.path;
    request.headers = std::move(headers);
    request.body = std::move(body);
    const std::string caseId = selectedCaseId_;
    StartWorker([this, request = std::move(request), caseId] {
        auto* completion = new HttpCompletion;
        completion->caseId = caseId;
        completion->response = httpTransport_->Execute(request);
        if (!closing_.load()) PostMessage(kHttpCompletedMessage, 0, reinterpret_cast<LPARAM>(completion));
        else delete completion;
    });
    SetStatus(L"HTTP 请求执行中...");
    RefreshControlState();
}

// 关键实现：只通知 SocketClient 传输层停止当前请求，后台线程负责形成取消结果并退出。
void CInterfaceTestDlg::OnCancelRequest()
{
    httpTransport_->Cancel();
    SetStatus(L"正在取消 HTTP 请求...");
}

// Web 连接回调只分配深拷贝消息并投递窗口消息，绝不从接收线程直接操作 MFC 控件。
void CInterfaceTestDlg::OnConnectWeb()
{
    if (workerRunning_.load() || webTransport_->IsConnected()) return;
    const auto* server = SelectedServer();
    if (server == nullptr || server->transport != interface_test::TransportKind::WebSocket) return;
    std::vector<std::pair<std::string, std::string>> headers;
    std::string redactedHeaders, error;
    auto connectSession = session_;
    if (!BuildCurrentHeaders(connectSession, headers, redactedHeaders, error))
    {
        AfxMessageBox(mttool::Utf8ToWide(error).c_str(), MB_OK | MB_ICONERROR);
        return;
    }
    std::string heartbeat;
    const auto heartbeatCase = std::find_if(config_.cases.begin(), config_.cases.end(), [server](const auto& item) {
        return item.serverGroup == server->group && item.id == "web-heartbeat";
    });
    if (heartbeatCase != config_.cases.end())
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
        const bool connected = webTransport_->Connect(serverCopy, headers,
            [this](const interface_test::WebMessage& message) {
                if (!closing_.load()) PostMessage(kWebMessage, 0,
                    reinterpret_cast<LPARAM>(new interface_test::WebMessage(message)));
            },
            [this](bool state, const std::string& text) {
                if (!closing_.load()) PostMessage(kWebStateMessage, 0,
                    reinterpret_cast<LPARAM>(new WebState{state, text}));
            }, connectError);
        if (!connected)
        {
            if (!closing_.load()) PostMessage(kWebStateMessage, 0,
                reinterpret_cast<LPARAM>(new WebState{false, connectError}));
            return;
        }
        if (!heartbeat.empty())
        {
            webTransport_->SetHeartbeat(heartbeat, serverCopy.heartbeatMs);
            std::string sendError;
            if (!webTransport_->Send(heartbeat, sendError) && !closing_.load())
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
    webTransport_->Disconnect();
    SetStatus(L"WebSocket 已由客户端断开；不会自动重连");
    RefreshControlState();
}

// 关键实现：同时清空消息模型、列表行和详情文本，避免视图与内存记录不一致。
void CInterfaceTestDlg::OnClearWeb()
{
    webMessages_.clear();
    webList_.DeleteAllItems();
    SetDlgItemText(IDC_INTERFACE_WEB_DETAIL, L"");
    SetStatus(L"WebSocket 消息已清空");
}

// 关键实现：直接复用压力测试的 CLogDlg；重复点击只激活同一个窗口。
void CInterfaceTestDlg::OnShowLog()
{
    if (!logDialog_)
    {
        logDialog_ = std::make_unique<CLogDlg>(appLog_, this);
        if (!logDialog_->Create(IDD_LOG_DIALOG, this))
        {
            logDialog_.reset();
            SetStatus(L"无法创建日志窗口");
            return;
        }
    }
    logDialog_->ShowWindow(SW_SHOWNORMAL);
    logDialog_->SetForegroundWindow();
}

// 该处理函数运行在 UI 线程，负责接管工作线程分配的结果并更新会话提取变量。
LRESULT CInterfaceTestDlg::OnHttpCompleted(WPARAM, LPARAM parameter)
{
    std::unique_ptr<HttpCompletion> completion(reinterpret_cast<HttpCompletion*>(parameter));
    workerRunning_.store(false);
    if (worker_.joinable()) worker_.join();
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
    appLog_.Add(L"INFO", L"接口完成: " + status.str());
    SetDlgItemText(IDC_INTERFACE_RESPONSE_HEADERS,
        mttool::Utf8ToWide(completion->response.responseHeaders).c_str());
    std::string formatted;
    std::string formatError;
    if (!interface_test::FormatJson(completion->response.responseBody, formatted, formatError))
        formatted = completion->response.responseBody;
    SetDlgItemText(IDC_INTERFACE_RESPONSE, mttool::Utf8ToWide(formatted).c_str());

    if (const auto* item = config_.FindCase(completion->caseId))
    {
        std::vector<std::string> extracted;
        std::string extractError;
        if (!interface_test::ApplyExtractRules(completion->response.responseBody, item->extracts,
            session_, extracted, extractError))
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
    if (webMessages_.size() >= limit && !webMessages_.empty())
    {
        webMessages_.erase(webMessages_.begin());
        webList_.DeleteItem(0);
        for (int row = 0; row < webList_.GetItemCount(); ++row)
            webList_.SetItemData(row, static_cast<DWORD_PTR>(row));
    }
    webMessages_.push_back(*message);
    const int row = webList_.InsertItem(webList_.GetItemCount(), mttool::Utf8ToWide(message->time).c_str());
    webList_.SetItemText(row, 1, mttool::Utf8ToWide(message->direction).c_str());
    webList_.SetItemText(row, 2, mttool::Utf8ToWide(message->funcId).c_str());
    webList_.SetItemText(row, 3, mttool::Utf8ToWide(message->topic).c_str());
    webList_.SetItemText(row, 4, mttool::Utf8ToWide(message->eventType).c_str());
    webList_.SetItemText(row, 5, FormatBytes(message->bytes).c_str());
    webList_.SetItemText(row, 6, mttool::Utf8ToWide(message->summary).c_str());
    webList_.SetItemData(row, static_cast<DWORD_PTR>(webMessages_.size() - 1));
    return 0;
}

// 关键实现：接管后台线程分配的状态对象，并在连接结束后回收已退出的工作线程。
LRESULT CInterfaceTestDlg::OnWebState(WPARAM, LPARAM parameter)
{
    std::unique_ptr<WebState> state(reinterpret_cast<WebState*>(parameter));
    workerRunning_.store(false);
    if (worker_.joinable() && !webTransport_->IsConnected()) worker_.join();
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
    const size_t index = static_cast<size_t>(webList_.GetItemData(change->iItem));
    if (index >= webMessages_.size()) return;
    std::string formatted;
    std::string error;
    if (!interface_test::FormatJson(webMessages_[index].json, formatted, error)) formatted = webMessages_[index].json;
    SetDlgItemText(IDC_INTERFACE_WEB_DETAIL, mttool::Utf8ToWide(formatted).c_str());
}

// 对话框同一时间只拥有一个短任务线程；启动下一任务前回收上一个已结束线程。
void CInterfaceTestDlg::StartWorker(std::function<void()> work)
{
    if (worker_.joinable()) worker_.join();
    workerRunning_.store(true);
    worker_ = std::thread([this, work = std::move(work)] {
        work();
        workerRunning_.store(false);
    });
}

// 关闭顺序固定为：阻止新回调、取消 HTTP、断开 Web、等待工作线程。
// OnDestroy、OnCancel 和析构都可重复调用该函数，因此各底层停止操作必须幂等。
void CInterfaceTestDlg::StopWorkers()
{
    closing_.store(true);
    httpTransport_->Cancel();
    webTransport_->Disconnect();
    if (worker_.joinable()) worker_.join();
    workerRunning_.store(false);
}

// 关键实现：窗口销毁前停止 HTTP、WebSocket 和心跳线程，确保没有回调访问失效 HWND。
void CInterfaceTestDlg::OnDestroy()
{
    StopWorkers();
    appLog_.Add(L"INFO", L"接口测试窗口关闭，网络线程已回收");
    if (logDialog_ && ::IsWindow(logDialog_->GetSafeHwnd())) logDialog_->DestroyWindow();
    logDialog_.reset();
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
    appLog_.Add(failed ? L"ERROR" : (warning ? L"WARN" : L"INFO"), text);
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
    const int selection = serverCombo_.GetCurSel();
    if (selection < 0 || selection >= static_cast<int>(serverIds_.size())) return nullptr;
    return config_.FindServer(serverIds_[selection]);
}

// 关键实现：无有效选择时回退到 HTTP，保证刷新过程中的临时空选项不会产生非法枚举。
interface_test::TransportKind CInterfaceTestDlg::SelectedTransport() const
{
    const int selection = transportCombo_.GetCurSel();
    if (selection < 0 || selection >= static_cast<int>(transportValues_.size()))
        return interface_test::TransportKind::Http;
    return transportValues_[selection];
}

// 关键实现：无有效选择时回退到 JSON，保持当前已实现功能的安全默认值。
interface_test::PayloadKind CInterfaceTestDlg::SelectedPayload() const
{
    const int selection = payloadCombo_.GetCurSel();
    if (selection < 0 || selection >= static_cast<int>(payloadValues_.size()))
        return interface_test::PayloadKind::Json;
    return payloadValues_[selection];
}
