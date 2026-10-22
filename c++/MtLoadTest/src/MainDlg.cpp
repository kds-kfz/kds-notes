#include "pch.h"
#include "MainDlg.h"

#include "MtLoadTestDlg.h"
#include "InterfaceTestDlg.h"
#include "ReservedFeatureDlg.h"
#include "resource.h"

BEGIN_MESSAGE_MAP(CMainDlg, CDialogEx)
    ON_COMMAND(ID_MENU_LOAD_TEST, &CMainDlg::OnOpenLoadTest)
    ON_COMMAND(ID_MENU_INTERFACE_TEST, &CMainDlg::OnOpenInterfaceTest)
    ON_COMMAND(ID_MENU_DATA_VIEWER, &CMainDlg::OnOpenDataViewer)
    ON_BN_CLICKED(IDC_OPEN_LOAD_TEST, &CMainDlg::OnOpenLoadTest)
    ON_BN_CLICKED(IDC_OPEN_INTERFACE_TEST, &CMainDlg::OnOpenInterfaceTest)
    ON_BN_CLICKED(IDC_OPEN_DATA_VIEWER, &CMainDlg::OnOpenDataViewer)
END_MESSAGE_MAP()

// 关键实现：仅绑定主窗口资源，子工具按用户选择临时创建。
CMainDlg::CMainDlg() : CDialogEx(IDD_MAIN_DIALOG)
{
}

// 关键实现：安装大小图标并保持主窗口不持有任何工具运行状态。
BOOL CMainDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    m_hMainIcon = AfxGetApp()->LoadIcon(IDI_MTLOADTEST_ICON);
    SetIcon(m_hMainIcon, TRUE);
    SetIcon(m_hMainIcon, FALSE);
    return TRUE;
}

// 关键实现：以栈对象模态运行压力测试窗口，返回后自动释放其资源。
void CMainDlg::OnOpenLoadTest()
{
    GetDlgItem(IDC_MAIN_STATUS)->SetWindowTextW(L"正在打开压力测试...");
    CMtLoadTestDlg dialog;
    dialog.DoModal();
    GetDlgItem(IDC_MAIN_STATUS)->SetWindowTextW(L"压力测试已关闭");
}

// 关键实现：以独立对话框打开接口测试，不复用压力测试配置或引擎。
void CMainDlg::OnOpenInterfaceTest()
{
    GetDlgItem(IDC_MAIN_STATUS)->SetWindowTextW(L"正在打开通用接口测试...");
    CInterfaceTestDlg dialog;
    dialog.DoModal();
    GetDlgItem(IDC_MAIN_STATUS)->SetWindowTextW(L"接口测试已关闭");
}

// 关键实现：当前只打开统一预留窗口，避免把未实现逻辑放入主窗口。
void CMainDlg::OnOpenDataViewer()
{
    GetDlgItem(IDC_MAIN_STATUS)->SetWindowTextW(L"正在打开数据查看预留窗口...");
    CReservedFeatureDlg dialog(IDD_DATA_VIEWER_DIALOG, L"数据查看",
        L"数据查看入口已预留。后续将支持 symbols、历史 K 线和双槽位文件的解析、编辑及按“原文件名_时间戳”另存。当前不会修改任何数据文件。");
    dialog.DoModal();
    GetDlgItem(IDC_MAIN_STATUS)->SetWindowTextW(L"数据查看预留窗口已关闭");
}
