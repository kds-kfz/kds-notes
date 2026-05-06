#pragma once
#include "afxwin.h"
#include "resource.h"
#include "afxdialogex.h"
#include <afxmt.h>
#include <map>
#include <atomic>
// CLogDlg 对话框

class CLogDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CLogDlg)

public:
	CLogDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CLogDlg();

// 对话框数据
	enum { IDD = IDD_LOG_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	DECLARE_MESSAGE_MAP()
private:
	CFont m_oFont;
	CRect m_wndRect;		//原始窗口
	map<int, CRect> m_rect;
	BOOL m_bMainShow;				// 是否显示主界面
	CListCtrl m_LogList;
	vector<pair<string,string>> m_vecBuf;
	CMutex Mutex;
	HANDLE m_hPrintThread;
	std::atomic<bool> m_bPrintThreadExit;

	void Print();
	DWORD PrintThread();
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnClose();
	afx_msg void OnNMCustomdrawList(NMHDR *pNMHDR, LRESULT *pResult);
	virtual BOOL OnInitDialog();

	void OutInfo(const string& p_strTime, const string& p_strInfo);
	void Show(BOOL p_bShow);
	static DWORD WINAPI s_PrintThread(void * pv);
};

extern CLogDlg* g_pLogDlg;
