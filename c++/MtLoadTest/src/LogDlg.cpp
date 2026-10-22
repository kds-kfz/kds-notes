#include "pch.h"
#include "MtLoadTest.h"
#include "LogDlg.h"

#include <algorithm>
#include <shellapi.h>

BEGIN_MESSAGE_MAP(CLogDlg, CDialogEx)
    ON_WM_TIMER()
    ON_WM_CLOSE()
    ON_WM_SIZE()
    ON_BN_CLICKED(IDC_OPEN_LOG, &CLogDlg::OnOpenLog)
    ON_BN_CLICKED(IDC_CLEAR_LOG, &CLogDlg::OnClearLog)
    ON_NOTIFY(NM_CUSTOMDRAW, IDC_LOG_LIST, &CLogDlg::OnCustomDraw)
END_MESSAGE_MAP()

// 关键实现：借用线程安全日志对象，窗口不接管其生命周期。
CLogDlg::CLogDlg(CAppLog& log, CWnd* parent)
    : CDialogEx(IDD_LOG_DIALOG, parent), m_refLog(log)
{
}

// 关键实现：只绑定日志列表控件，不复制日志数据。
void CLogDlg::DoDataExchange(CDataExchange* exchange)
{
    CDialogEx::DoDataExchange(exchange);
    DDX_Control(exchange, IDC_LOG_LIST, m_clList);
}

// 关键实现：创建三列并启动低频增量刷新定时器。
BOOL CLogDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    SetWindowText(L"MtLoadTest 运行日志");
    m_clList.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
    m_clList.InsertColumn(0, L"时间", LVCFMT_LEFT, 210);
    m_clList.InsertColumn(1, L"级别", LVCFMT_LEFT, 70);
    m_clList.InsertColumn(2, L"消息", LVCFMT_LEFT, 300);
    ResizeColumns();
    SetTimer(kRefreshTimer, 300, nullptr);
    RefreshRows();
    return TRUE;
}

// 关键实现：仅追加新序号条目，并仅在用户原本位于底部时自动滚动。
void CLogDlg::RefreshRows()
{
    const auto entries = m_refLog.Snapshot();
    const int itemCountBefore = m_clList.GetItemCount();
    const int firstVisibleBefore = m_clList.GetTopIndex();
    const int visibleCount = m_clList.GetCountPerPage();
    const bool followTail = itemCountBefore == 0 ||
        firstVisibleBefore + visibleCount >= itemCountBefore;
    bool appended = false;
    for (const auto& entry : entries)
    {
        if (entry.sequence <= m_u64LastSequence) continue;
        if (m_clList.GetItemCount() >= 1000) m_clList.DeleteItem(0);
        const int index = m_clList.InsertItem(m_clList.GetItemCount(), entry.timestamp.c_str());
        m_clList.SetItemText(index, 1, entry.level.c_str());
        m_clList.SetItemText(index, 2, entry.message.c_str());
        m_u64LastSequence = entry.sequence;
        appended = true;
    }
    // 只有用户原本就在底部时才跟随新日志；手动向上滚动后保持阅读位置。
    if (appended && followTail && m_clList.GetItemCount() > 0)
        m_clList.EnsureVisible(m_clList.GetItemCount() - 1, FALSE);
}

// 关键实现：只处理本窗口刷新定时器，其余事件交还基类。
void CLogDlg::OnTimer(UINT_PTR eventId)
{
    if (eventId == kRefreshTimer) RefreshRows();
    CDialogEx::OnTimer(eventId);
}

// 关键实现：先停止定时器再销毁非模态窗口，避免销毁后继续刷新。
void CLogDlg::OnClose()
{
    ShowWindow(SW_HIDE);
}

// 关键实现：Esc 与标题栏关闭复用同一清理路径。
void CLogDlg::OnCancel()
{
    ShowWindow(SW_HIDE);
}

// 关键实现：先让基类完成布局，再按新的客户区宽度重排列。
void CLogDlg::OnSize(UINT type, int cx, int cy)
{
    CDialogEx::OnSize(type, cx, cy);
    if (!::IsWindow(m_clList.GetSafeHwnd())) return;
    m_clList.MoveWindow(8, 8, (std::max)(120, cx - 16), (std::max)(80, cy - 48));
    if (CWnd* clearButton = GetDlgItem(IDC_CLEAR_LOG))
        clearButton->MoveWindow((std::max)(8, cx - 178), (std::max)(8, cy - 32), 75, 22);
    if (CWnd* openButton = GetDlgItem(IDC_OPEN_LOG))
        openButton->MoveWindow((std::max)(88, cx - 95), (std::max)(8, cy - 32), 87, 22);
    ResizeColumns();
}

// 关键实现：固定时间和等级最小宽度，消息列占用全部剩余空间。
void CLogDlg::ResizeColumns()
{
    if (!::IsWindow(m_clList.GetSafeHwnd())) return;
    const CHeaderCtrl* header = m_clList.GetHeaderCtrl();
    if (header == nullptr || header->GetItemCount() != 3) return;

    constexpr int timeWidth = 210;
    constexpr int levelWidth = 70;
    constexpr int messageMinWidth = 300;
    CRect client;
    m_clList.GetClientRect(&client);
    const int available = client.Width();
    if (available <= 0) return;

    m_clList.SetColumnWidth(0, timeWidth);
    m_clList.SetColumnWidth(1, levelWidth);
    m_clList.SetColumnWidth(2,
        (std::max)(messageMinWidth, available - timeWidth - levelWidth));
}

// 关键实现：把日志路径交给系统关联程序，文件不存在时只显示提示。
void CLogDlg::OnOpenLog()
{
    const std::wstring path = m_refLog.FilePath();
    if (!path.empty()) ShellExecuteW(GetSafeHwnd(), L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

// 关键实现：清空内存和列表后重置增量序号，磁盘文件保持不变。
void CLogDlg::OnClearLog()
{
    const auto entries = m_refLog.Snapshot();
    if (!entries.empty()) m_u64LastSequence = entries.back().sequence;
    m_refLog.ClearMemory();
    m_clList.DeleteAllItems();
}

// 关键实现：在子项绘制阶段按等级设置文本颜色，不改变选择和滚动状态。
void CLogDlg::OnCustomDraw(NMHDR* header, LRESULT* result)
{
    auto* custom = reinterpret_cast<NMLVCUSTOMDRAW*>(header);
    *result = CDRF_DODEFAULT;
    if (custom->nmcd.dwDrawStage == CDDS_PREPAINT)
    {
        *result = CDRF_NOTIFYITEMDRAW;
        return;
    }
    if (custom->nmcd.dwDrawStage != CDDS_ITEMPREPAINT) return;

    const CString level = m_clList.GetItemText(static_cast<int>(custom->nmcd.dwItemSpec), 1);
    if (level == L"ERROR") custom->clrText = RGB(210, 35, 35);
    else if (level == L"WARN") custom->clrText = RGB(210, 120, 0);
    else if (level == L"DEBUG") custom->clrText = RGB(0, 128, 70);
}
