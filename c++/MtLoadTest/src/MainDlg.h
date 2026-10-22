#pragma once

// 应用主窗口只负责三个工具入口，不持有压测、网络连接或文件解析状态。
class CMainDlg final : public CDialogEx
{
public:
    // 绑定主对话框资源，不创建子工具窗口。
    CMainDlg();

protected:
    // 初始化窗口图标和状态文本；仅在 UI 线程调用。
    BOOL OnInitDialog() override;
    // 模态打开压力测试窗口，窗口返回后恢复主界面。
    afx_msg void OnOpenLoadTest();
    // 模态打开通用接口测试窗口。
    afx_msg void OnOpenInterfaceTest();
    // 打开数据查看占位窗口，后续实现不影响主窗口职责。
    afx_msg void OnOpenDataViewer();
    DECLARE_MESSAGE_MAP()

private:
    HICON m_hMainIcon = nullptr;   // 从应用资源借用的主图标句柄，不由本类销毁。
};
