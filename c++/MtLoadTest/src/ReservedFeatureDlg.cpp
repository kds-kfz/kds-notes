#include "pch.h"
#include "ReservedFeatureDlg.h"

#include "resource.h"

// 关键实现：复制调用方文本，避免窗口显示期间依赖临时 CString 生命周期。
CReservedFeatureDlg::CReservedFeatureDlg(UINT templateId, const CString& title, const CString& message)
    : CDialogEx(templateId), title_(title), message_(message)
{
}

// 关键实现：初始化完成后一次性设置标题和只读说明控件。
BOOL CReservedFeatureDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    SetWindowTextW(title_);
    SetIcon(AfxGetApp()->LoadIcon(IDI_MTLOADTEST_ICON), TRUE);
    SetIcon(AfxGetApp()->LoadIcon(IDI_MTLOADTEST_ICON), FALSE);
    GetDlgItem(IDC_RESERVED_TITLE)->SetWindowTextW(title_);
    GetDlgItem(IDC_RESERVED_MESSAGE)->SetWindowTextW(message_);
    return TRUE;
}
