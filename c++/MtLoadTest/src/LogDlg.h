#pragma once

// 非模态日志窗口。仅展示 CAppLog 的不可变副本，不执行压测请求或指标计算。

#include "AppLog.h"

class CLogDlg final : public CDialogEx
{
public:
    // 借用应用日志对象；调用方必须保证 log 生命周期覆盖本窗口。
    explicit CLogDlg(CAppLog& log, CWnd* parent = nullptr);

protected:
    // 初始化列表列宽和刷新定时器。
    BOOL OnInitDialog() override;
    // 绑定日志列表控件。
    void DoDataExchange(CDataExchange* exchange) override;
    // 将取消操作统一转换为关闭非模态窗口。
    void OnCancel() override;
    // 定时拉取日志快照；用户滚动离开底部时不强制跳回。
    afx_msg void OnTimer(UINT_PTR eventId);
    // 停止定时器并销毁窗口。
    afx_msg void OnClose();
    // 窗口尺寸变化时重新分配日志列宽。
    afx_msg void OnSize(UINT type, int cx, int cy);
    // 使用系统关联程序打开当前日志文件。
    afx_msg void OnOpenLog();
    // 只清空内存显示，不删除磁盘日志。
    afx_msg void OnClearLog();
    // 按日志等级设置行颜色，不改变日志内容。
    afx_msg void OnCustomDraw(NMHDR* header, LRESULT* result);
    DECLARE_MESSAGE_MAP()

private:
    static constexpr UINT_PTR kRefreshTimer = 1; // 日志增量刷新定时器 ID。
    // 追加尚未显示的日志，并保留用户当前选择和滚动意图。
    void RefreshRows();
    // 根据列表客户区宽度铺满时间、等级和消息列。
    void ResizeColumns();

    CAppLog& log_;                 // 借用的线程安全日志存储。
    CListCtrl list_;              // 日志列表控件包装对象。
    uint64_t lastSequence_ = 0;   // 已显示的最大日志序号。
};
