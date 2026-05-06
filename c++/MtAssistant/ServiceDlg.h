// ServiceDlg.h : 头文件
//

#pragma once
#include "afxcmn.h"
#include "afxwin.h"
#include "afxdtctl.h"
#include "afxfontcombobox.h"


// CServiceDlg 对话框
class CServiceDlg : public CDialogEx
{
// 构造
public:
	CServiceDlg(CWnd* pParent = NULL); // 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SERVICE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX); // DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	bool m_bStatus;// wyl 2026-05-06：标记主窗口是否已进入可刷新状态，避免销毁期间继续更新UI。
	CFont m_Font;
	CListCtrl m_ServiceList;// wyl 2026-05-06：服务列表，状态列统一由UI线程刷新。
	CEdit m_AppPathEdit;
	CEdit m_AppParamEdit;
	CComboBox m_WeekCombo;
	CDateTimeCtrl m_StartTime;
	CDateTimeCtrl m_EndTime;
	CListCtrl m_TimeList;

	int GetSelectedItemIndex(CListCtrl& p_listCtrl);

	afx_msg void OnNMClickServiceList(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult);// wyl 2026-05-06：处理复选框启停，过滤无关状态变化。
	afx_msg void OnBnClickedTimeAddButton();
	afx_msg void OnBnClickedTimeDelButton();
	afx_msg void OnBnClickedLookButton();
	afx_msg void OnBnClickedAddButton();
	afx_msg void OnBnClickedDelButton();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedLogButton();
	afx_msg void OnCbnSelchangeWeekCombo();
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT_PTR nIDEvent);// wyl 2026-05-06：UI线程定时刷新服务运行状态。
};