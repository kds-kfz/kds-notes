// LogDlg.cpp : 实现文件
//

#include "stdafx.h"
//#include "CloudCalcMoney.h"
#include "LogDlg.h"
#include "afxdialogex.h"

#define MAXINFO 1000
// CLogDlg 对话框

IMPLEMENT_DYNAMIC(CLogDlg, CDialogEx)

CLogDlg::CLogDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CLogDlg::IDD, pParent)
{
	m_bMainShow = false;
	m_hPrintThread = NULL;
}

CLogDlg::~CLogDlg()
{
	TerminateThread(m_hPrintThread, 1);
	CloseHandle(m_hPrintThread);
	m_hPrintThread = NULL;
}

void CLogDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_LOG, m_LogList);
}


BEGIN_MESSAGE_MAP(CLogDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &CLogDlg::OnBnClickedOk)
	ON_WM_SIZE()
	ON_WM_CLOSE()
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LIST_LOG, &CLogDlg::OnNMCustomdrawList)
END_MESSAGE_MAP()


// CLogDlg 消息处理程序



void CLogDlg::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	int iDate = GetCurDate();
	char szBuf[128];
	sprintf(szBuf,"explorer .\\Day_Logs\\%d.log",iDate);

	WinExec(szBuf, SW_HIDE);
	//CDialogEx::OnOK();
}

void CLogDlg::OutInfo(const string& p_strTime, const string& p_strInfo)
{
	Mutex.Lock();
	m_vecBuf.push_back(make_pair(p_strTime, p_strInfo));
	Mutex.Unlock();
}


void CLogDlg::Print() 
{
	vector<pair<string, string>> vecTmp;
	Mutex.Lock();
	if (m_vecBuf.empty()) 
	{ 
		Mutex.Unlock(); 
		return; 
	}
	vecTmp = m_vecBuf;
	m_vecBuf.clear();
	Mutex.Unlock();
	if (vecTmp.size() > MAXINFO)
	{
		vecTmp.erase(vecTmp.begin(), vecTmp.end() - MAXINFO);
	}
	string strTime;
	string strMsg;
	for (int i = 0; i < vecTmp.size(); i++) 
	{
		strTime = vecTmp[i].first;
		strMsg = vecTmp[i].second;

		int iLeft = strMsg.find('[') + 1;
		int iRight = strMsg.find(']');

		int iNum = m_LogList.GetItemCount();
		if (iNum > MAXINFO)
		{
			m_LogList.DeleteAllItems();
		}
		m_LogList.InsertItem(iNum, "");
		m_LogList.SetItemText(iNum, 0, strTime.c_str());
		m_LogList.SetItemText(iNum, 1, strMsg.substr(iLeft, iRight - iLeft).c_str());
		m_LogList.SetItemText(iNum, 2, strMsg.substr(iRight + 1).c_str());

		if (m_LogList.GetTopIndex() + m_LogList.GetCountPerPage() + 3 >= iNum)
		{ //如果 滚动到最后  自动滚动到最后					3条为容错空间
			m_LogList.EnsureVisible(iNum, FALSE);
		}
	}
}

BOOL CLogDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_hPrintThread = CreateThread(NULL, 0, s_PrintThread, this, 0, NULL);

	m_oFont.CreatePointFont(90, _T("宋体"));
	m_LogList.SetFont(&m_oFont);

	m_LogList.ModifyStyle(0, LVS_REPORT);
	m_LogList.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	m_LogList.InsertColumn(0, "Time");
	m_LogList.InsertColumn(1, "Tag");
	m_LogList.InsertColumn(2, "Msg");

	m_LogList.SetColumnWidth(0, 200); //设置列宽
	m_LogList.SetColumnWidth(1, 55);
	m_LogList.SetColumnWidth(2, 1500);

	// TODO:  在此添加额外的初始化
	GetClientRect(&m_wndRect);		//获取窗口表花钱的大小

	HWND  hwndChild = ::GetWindow(m_hWnd, GW_CHILD);

	CWnd *pWnd = NULL;
	CRect szRect;

	while (hwndChild)
	{
		int id = ::GetDlgCtrlID(hwndChild);//取得ID
		pWnd = GetDlgItem(id);
		pWnd->GetWindowRect(&szRect); //获取按钮变化前的大小
		ScreenToClient(&szRect);
		m_rect.insert(make_pair(id, szRect));
		hwndChild = ::GetWindow(hwndChild, GW_HWNDNEXT);
	}

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CLogDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码

	if (nType == 1) return; //最小化则什么都不做

	int id = 0;
	CWnd *pWnd;
	//按钮控件
	id = IDOK;
	pWnd = GetDlgItem(id);
	if (pWnd != NULL) 
	{
		CRect rect;
		rect.left = cx - (m_rect[id].right - m_rect[id].left) - 12;
		rect.top = cy - (m_rect[id].bottom - m_rect[id].top) - 14;
		rect.right = cx - 12;
		rect.bottom = cy - 14;
		pWnd->MoveWindow(rect);//设置控件大小
	}	
	//列表控件
	id = IDC_LIST_LOG;
	pWnd = GetDlgItem(id);
	if (pWnd != NULL)
	{
		CRect rect;
		rect.left = m_rect[id].left;
		rect.top = m_rect[id].top;
		rect.right = cx - 12;
		rect.bottom = cy - 48;
		pWnd->MoveWindow(rect);//设置控件大小
	}
}

void CLogDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (!m_bMainShow)
	{
		if (MessageBox("你确定强制退出本系统吗？", "行情插件", MB_ICONQUESTION | MB_OKCANCEL) == IDCANCEL)
			return;
		HANDLE hself = GetCurrentProcess();
		TerminateProcess(hself, 0);
	}
	else
	{
		ShowWindow(SW_HIDE);
	}
	//CDialogEx::OnClose();
}

void CLogDlg::Show(BOOL p_bShow)
{
	if (p_bShow)
	{		
		if(!m_bMainShow)
			ShowWindow(SW_HIDE);	
	}
	else 
	{
		// 关闭时不显示日志窗口，释放内容也正常释放
		ShowWindow(SW_HIDE);
		/*ShowWindow(SW_SHOW);
		SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);*/
	}
	m_bMainShow = p_bShow;
}


void CLogDlg::OnNMCustomdrawList(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = CDRF_DODEFAULT;


	if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYSUBITEMDRAW;
	}//这上面的都是默认的不用理
	else if ((CDDS_ITEMPREPAINT | CDDS_SUBITEM) == pLVCD->nmcd.dwDrawStage)
	{
		//COLORREF clrNewTextColor, clrNewBkColor;//当前单元格的文本颜色以及背景颜色

		int nItem = static_cast<int>(pLVCD->nmcd.dwItemSpec);//当前行
		CString strTemp = m_LogList.GetItemText(nItem, 1);//当前行的某一列的内容
		if (strTemp == "ERROR")
		{
			pLVCD->clrText = RGB(255, 0, 0);
		}
		else if (strTemp == "WARN")
		{
			pLVCD->clrText = RGB(255, 145, 0);
		}
		else if (strTemp == "DEBUG")
		{
			pLVCD->clrText = RGB(12, 153, 0);
		}
		else if (strTemp == "INFO")
		{
			pLVCD->clrText = RGB(0, 0, 0);
		}
		*pResult = CDRF_DODEFAULT;
	}
}


DWORD WINAPI CLogDlg::s_PrintThread(void * pv)
{
	CLogDlg * pThis = (CLogDlg*)pv;
	return pThis->PrintThread();
}

DWORD CLogDlg::PrintThread()
{
	while (1)
	{
		Print();
		Sleep(10);
	}
}