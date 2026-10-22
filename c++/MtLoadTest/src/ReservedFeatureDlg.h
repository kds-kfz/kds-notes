#pragma once

// 尚未进入实现阶段的工具使用统一占位窗口，主菜单和窗口生命周期先固定下来。
class CReservedFeatureDlg final : public CDialogEx
{
public:
    // 保存资源 ID、标题和说明，实际控件赋值在 OnInitDialog 中完成。
    CReservedFeatureDlg(UINT templateId, const CString& title, const CString& message);

protected:
    // 将预留功能名称和当前状态写入只读控件。
    BOOL OnInitDialog() override;

private:
    CString m_strTitle;      // 窗口标题及功能名称。
    CString m_strMessage;    // 面向用户的预留状态说明。
};
