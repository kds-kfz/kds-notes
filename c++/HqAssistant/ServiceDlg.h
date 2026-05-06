
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
	CServiceDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SERVICE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


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
	bool m_bStatus;
	CFont m_Font;
	CListCtrl m_ServiceList;//服务列表
	CEdit m_AppPathEdit;	//服务路径
	CEdit m_AppParamEdit;	//服务命令参数
	CComboBox m_WeekCombo;	//星期配置
	CDateTimeCtrl m_StartTime;	//开始时间(重启)
	CDateTimeCtrl m_EndTime;	//结束时间(重启)
	CListCtrl m_TimeList;		//时间列表

	int GetSelectedItemIndex(CListCtrl& p_listCtrl);

	afx_msg void OnNMClickServiceList(NMHDR *pNMHDR, LRESULT *pResult);//所在行获取
	afx_msg void OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult);//复选框勾选
	afx_msg void OnBnClickedTimeAddButton();
	afx_msg void OnBnClickedTimeDelButton();
	afx_msg void OnBnClickedLookButton();
	afx_msg void OnBnClickedAddButton();
	afx_msg void OnBnClickedDelButton();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedLogButton();
	afx_msg void OnCbnSelchangeWeekCombo();
	afx_msg void OnClose();
};
