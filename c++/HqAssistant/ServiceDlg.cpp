
// ServiceDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "Service.h"
#include "ServiceDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "ServiceDataMng.h"

std::map<int, ServiceInfo> g_mapServiceInfo;
std::map<int, bool> g_mapServiceStatus;
int g_iServiceCount = -1;//服务个数
int g_iServiceRow = -1;//选中服务，所在行
string g_strNewName = "";//新添加的服务
string g_strNewPath = "";//新添加的服务路径
string g_strCfg = "";
CListCtrl *g_ServiceList;
bool g_bCheck = false;


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CServiceDlg 对话框



CServiceDlg::CServiceDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_SERVICE_DIALOG, pParent)
{
	m_bStatus = false;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CServiceDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SERVICE_LIST, m_ServiceList);
	DDX_Control(pDX, IDC_EXE_EDIT, m_AppPathEdit);
	DDX_Control(pDX, IDC_PARAM_EDIT, m_AppParamEdit);
	DDX_Control(pDX, IDC_WEEK_COMBO, m_WeekCombo);
	DDX_Control(pDX, IDC_START_DATETIMEPICKER, m_StartTime);
	DDX_Control(pDX, IDC_END_DATETIMEPICKER, m_EndTime);
	DDX_Control(pDX, IDC_TIME_LIST, m_TimeList);
}

BEGIN_MESSAGE_MAP(CServiceDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(NM_CLICK, IDC_SERVICE_LIST, &CServiceDlg::OnNMClickServiceList)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_SERVICE_LIST, &CServiceDlg::OnItemchangedList)
	ON_BN_CLICKED(IDC_TIME_ADD_BUTTON, &CServiceDlg::OnBnClickedTimeAddButton)
	ON_BN_CLICKED(IDC_TIME_DEL_BUTTON, &CServiceDlg::OnBnClickedTimeDelButton)
	ON_BN_CLICKED(IDC_LOOK_BUTTON, &CServiceDlg::OnBnClickedLookButton)
	ON_BN_CLICKED(IDC_ADD_BUTTON, &CServiceDlg::OnBnClickedAddButton)
	ON_BN_CLICKED(IDC_DEL_BUTTON, &CServiceDlg::OnBnClickedDelButton)

	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_LOG_BUTTON, &CServiceDlg::OnBnClickedLogButton)
	ON_CBN_SELCHANGE(IDC_WEEK_COMBO, &CServiceDlg::OnCbnSelchangeWeekCombo)
	ON_WM_CLOSE()
END_MESSAGE_MAP()

DWORD WINAPI InitLogDlg(LPVOID Parameter)
{
	//new  一个窗口类对象
	//Create函数创建一个窗口
	//ShowWindow展示窗口
	g_pLogDlg = new CLogDlg();
	g_pLogDlg->Create(IDD_LOG_DIALOG);
	//g_pLogDlg->ShowWindow(SW_SHOW);
	g_pLogDlg->ShowWindow(SW_HIDE);
	g_pLogDlg->SetWindowPos(&g_pLogDlg->wndTop, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	g_pLogDlg->SetActiveWindow();
	//MT_INFO("*************提示信息:*************");
	//MT_INFO("%s", GetCommandLine());
	//在这三步之后： 自己添加消息循环
	MSG msg = { 0 };
	while (GetMessage(&msg, 0, 0, 0))    //得到消息
	{
		TranslateMessage(&msg);      //转换消息
		DispatchMessage(&msg);       //分发消息
	}
	return 0;
}

//定时线程，获取服务状态，并刷新状态列表
DWORD WINAPI UpdateServiceStatus(LPVOID Parameter)
{
	while (true)
	{
		if (g_bCheck)
			break;

		CServiceDataMng::GetInstance()->GetAllServiceStatus(g_mapServiceStatus);
		for (int i = 0; i < g_iServiceCount; i++)
		{
			if (g_mapServiceStatus.find(i) != g_mapServiceStatus.end())
			{
				g_ServiceList->SetItemText(i, 2, g_mapServiceStatus[i] ? "运行" : "停止");
			}
		}
		Sleep(1000);
	}

	return 0;
}

// CServiceDlg 消息处理程序

BOOL CServiceDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	//初始化前调用，注意这里不要加进程号，防止下次启动助手找不到窗口
	CString strProcessID;
	CString	strVersion;
	char szProcessName[256] = { 0 };
	GetModuleFileName(NULL, szProcessName, 256);
	GetFileVersion(szProcessName, strVersion);
	strProcessID.Format("--服务助手--( Ver:%s )", strVersion);

	HWND hwnd = ::FindWindow(NULL, strProcessID);
	if (hwnd)
	{
		BOOL bFlag = CenterAndActivateWindow(hwnd);
		if (bFlag)//激活成功则退出
		{
			ExitProcess(0);
		}
		else
		{
			::MessageBox(NULL, _T("无法激活助手窗口"), _T("错误"), MB_ICONERROR); // 激活失败时显示错误消息框
			ExitProcess(0);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	//初始化日志窗口
	CreateThread(NULL, 0, InitLogDlg, this, 0, 0);
	//Sleep(1000);

	m_Font.CreatePointFont(90, _T("宋体"));

	m_ServiceList.SetFont(&m_Font);
	m_ServiceList.ModifyStyle(0, LVS_REPORT);
	m_ServiceList.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	m_ServiceList.InsertColumn(0, "程序名称");
	m_ServiceList.InsertColumn(1, "启动目录");
	m_ServiceList.InsertColumn(2, "状态");

	m_ServiceList.SetColumnWidth(0, 120); //设置列宽
	m_ServiceList.SetColumnWidth(1, 300);
	m_ServiceList.SetColumnWidth(2, 80);
	g_ServiceList = &m_ServiceList;

	m_TimeList.SetFont(&m_Font);
	m_TimeList.ModifyStyle(0, LVS_REPORT);
	m_TimeList.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	m_TimeList.InsertColumn(0, "开始时间");
	m_TimeList.InsertColumn(1, "结束时间");

	m_TimeList.SetColumnWidth(0, 130); //设置列宽
	m_TimeList.SetColumnWidth(1, 130);

	m_WeekCombo.SetFont(&m_Font);
	m_WeekCombo.SetWindowText(_T("星期一"));//默认显示
	m_WeekCombo.AddString(_T("星期一"));
	m_WeekCombo.AddString(_T("星期二"));
	m_WeekCombo.AddString(_T("星期三"));
	m_WeekCombo.AddString(_T("星期四"));
	m_WeekCombo.AddString(_T("星期五"));
	m_WeekCombo.AddString(_T("星期六"));
	m_WeekCombo.AddString(_T("星期天"));

	m_ServiceList.ModifyStyle(0, LVS_REPORT);
	m_ServiceList.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);

	
	SetWindowText(strProcessID);

	//初始化服务数据
	CServiceDataMng::GetInstance()->Init();

	//获取配置
	g_strCfg = CServiceDataMng::GetInstance()->GetCfgPath();

	//初始化全局变量
	m_ServiceList.DeleteAllItems();
	g_iServiceCount = CServiceDataMng::GetInstance()->GetAllServiceInfo(g_mapServiceInfo);

	//数据填充展示
	for (int i = 0; i < g_iServiceCount; i++)
	{

		m_ServiceList.InsertItem(i, "", 1);
		m_ServiceList.SetItemText(i, 0, g_mapServiceInfo[i].strName.c_str());
		m_ServiceList.SetItemText(i, 1, g_mapServiceInfo[i].strPath.c_str());
		m_ServiceList.SetItemText(i, 2, "停止");

		if (1 == g_mapServiceInfo[i].iEnable)
		{
			m_ServiceList.SetCheck(i, true);
		}
	}

	//初始化完成
	m_bStatus = true;

	if (g_pLogDlg)
	{
		g_pLogDlg->Show(true);
	}

	CreateThread(NULL, 0, UpdateServiceStatus, this, 0, 0);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CServiceDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CServiceDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CServiceDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//单击处理事件，切换显示配置,所在行
void CServiceDlg::OnNMClickServiceList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码

	//首先得到点击的位置
	POSITION pos = m_ServiceList.GetFirstSelectedItemPosition();
	if (pos == NULL)
	{
		//没有选中行
		g_iServiceRow = -1;
		//清空路径
		m_AppPathEdit.SetWindowText("");
		//清空命令行
		m_AppParamEdit.SetWindowText("");
		//清空时间显示
		m_TimeList.DeleteAllItems();
		return;
	}

	//得到行号，通过POSITION转化
	g_iServiceRow = (int)m_ServiceList.GetNextSelectedItem(pos);

	//根据行号获取对应服务配置、刷新显示

	if (g_mapServiceInfo.find(g_iServiceRow) == g_mapServiceInfo.end())
	{
		g_iServiceRow = -1;
		return;
	}

	ServiceInfo refServiceInfo = g_mapServiceInfo[g_iServiceRow];

	//填充路径，参数
	string strFullAppPath = refServiceInfo.strPath + refServiceInfo.strName;
	m_AppPathEdit.SetWindowText(strFullAppPath.c_str());
	//m_AppParamEdit.SetWindowText(refServiceInfo.strCmdParam.c_str());

	//填充右边时间数据
	m_TimeList.DeleteAllItems();
	int j = m_WeekCombo.GetCurSel();
	WeekInfo enWeek = j == 0 ? MON : j == 1 ? TUE : j == 2 ? WED : j == 3 ? THU : j == 4 ? FRI : j == 5 ? SAT : j == 6 ? SUN : MON;

	vector<TimeInfo> vecTimeInfo = refServiceInfo.mapTimeConf[enWeek];
	int iTimeCount = vecTimeInfo.size();
	for (int j = 0; j < iTimeCount; j++)
	{
		m_TimeList.InsertItem(j, "", 1);
		m_TimeList.SetItemText(j, 0, vecTimeInfo[j].StartTime.toString().c_str());
		m_TimeList.SetItemText(j, 1, vecTimeInfo[j].EndTime.toString().c_str());
	}

	*pResult = 0;
}

//勾选复选框
void CServiceDlg::OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	if (m_bStatus)
	{
		NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
		CString c_str;
		if (m_ServiceList.GetCheck(pNMListView->iItem) == 1)
		{
			//AfxMessageBox("选上");
			g_iServiceRow = pNMListView->iItem;
			CServiceDataMng::GetInstance()->UpdateEnable(g_iServiceRow, 1);
			g_iServiceCount = CServiceDataMng::GetInstance()->GetAllServiceInfo(g_mapServiceInfo);

			//添加守护线程
			CServiceDataMng::GetInstance()->CreateDaemon(g_iServiceRow, &g_mapServiceInfo[g_iServiceRow]);
		}
		else
		{
			//AfxMessageBox("没选上");
			g_iServiceRow = pNMListView->iItem;
			CServiceDataMng::GetInstance()->UpdateEnable(g_iServiceRow, 0);
			g_iServiceCount = CServiceDataMng::GetInstance()->GetAllServiceInfo(g_mapServiceInfo);

			//状态更改
			if (g_mapServiceStatus.find(g_iServiceRow) != g_mapServiceStatus.end())
			{
				g_mapServiceStatus[g_iServiceRow] = false;
				g_ServiceList->SetItemText(g_iServiceRow, 2, "停止");
			}

			//删除守护线程
			CServiceDataMng::GetInstance()->DestroyDaemon(g_iServiceRow);

			g_iServiceRow = -1;
		}
	}
	*pResult = 0;
}

int CServiceDlg::GetSelectedItemIndex(CListCtrl& p_listCtrl)
{
	int nSelectedItem = -1;
	int nItemCount = p_listCtrl.GetItemCount();
	for (int i = 0; i < nItemCount; ++i)
	{
		if (p_listCtrl.GetItemState(i, LVNI_SELECTED) == LVNI_SELECTED)
		{
			nSelectedItem = i;
			break;
		}
	}
	return nSelectedItem;
}

void CServiceDlg::OnBnClickedTimeAddButton()
{
	// TODO: 在此添加控件通知处理程序代码
	//获取服务所在行
	//int iRow = GetSelectedItemIndex(m_ServiceList);

	if (-1 == g_iServiceRow)
		return;

	//获取周所在行
	int j = m_WeekCombo.GetCurSel();
	WeekInfo enWeek = j == 0 ? MON : j == 1 ? TUE : j == 2 ? WED : j == 3 ? THU : j == 4 ? FRI : j == 5 ? SAT : j == 6 ? SUN : MON;

	//获取起止时间
	CTime stStartTime, stEndTime;
	m_StartTime.GetTime(stStartTime);
	m_EndTime.GetTime(stEndTime);

	SYSTEMTIME stTime;
	stStartTime.GetAsSystemTime(stTime);

	TimeInfo stTimeInfo;
	stTimeInfo.StartTime.fromSystemTime(stTime);

	stEndTime.GetAsSystemTime(stTime);
	stTimeInfo.EndTime.fromSystemTime(stTime);

	//获取已有的时间总条数
	int iTimeTotal = m_TimeList.GetItemCount();
	m_TimeList.InsertItem(iTimeTotal, "", 1);
	m_TimeList.SetItemText(iTimeTotal, 0, stTimeInfo.StartTime.toString().c_str());
	m_TimeList.SetItemText(iTimeTotal, 1, stTimeInfo.EndTime.toString().c_str());

	CServiceDataMng::GetInstance()->UpdateTimeInfo(g_iServiceRow, enWeek, 0, stTimeInfo, ADD);
	g_iServiceCount = CServiceDataMng::GetInstance()->GetAllServiceInfo(g_mapServiceInfo);
}


void CServiceDlg::OnBnClickedTimeDelButton()
{
	// TODO: 在此添加控件通知处理程序代码
	//获取服务所在行
	//int iRow = GetSelectedItemIndex(m_ServiceList);

	if (-1 == g_iServiceRow)
		return;

	//获取周所在行
	int j = m_WeekCombo.GetCurSel();
	WeekInfo enWeek = j == 0 ? MON : j == 1 ? TUE : j == 2 ? WED : j == 3 ? THU : j == 4 ? FRI : j == 5 ? SAT : j == 6 ? SUN : MON;

	//获取起止时间
	TimeInfo stTimeInfo;

	//删除选中的时间
	//所在行
	int iTimeRow = GetSelectedItemIndex(m_TimeList);

	m_TimeList.DeleteItem(iTimeRow);

	CServiceDataMng::GetInstance()->UpdateTimeInfo(g_iServiceRow, enWeek, iTimeRow, stTimeInfo, DEL);
	g_iServiceCount = CServiceDataMng::GetInstance()->GetAllServiceInfo(g_mapServiceInfo);

	//更新配置到守护线程
}


void CServiceDlg::OnBnClickedLookButton()
{
	// TODO: 在此添加控件通知处理程序代码
	//打开程序路径
	// 创建文件对话框
	CFileDialog fileDlg(TRUE, NULL, NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("All Files (*.*)|*.*||"), NULL);

	// 显示对话框
	if (fileDlg.DoModal() == IDOK)
	{
		CString objFilePath = fileDlg.GetPathName();
		// 使用selectedFilePath，例如显示在编辑框中或进行其他处理

		std::string strAppPath(objFilePath.GetBuffer(), objFilePath.GetLength());
		g_strNewName = strAppPath.substr(strAppPath.find_last_of("\\") + 1);
		g_strNewPath = strAppPath;

		int iStrNameLen = g_strNewName.length();
		if (g_strNewName.find("exe", iStrNameLen - 4) == string::npos &&
			g_strNewName.find("EXE", iStrNameLen - 4) == string::npos &&
			g_strNewName.find("bat", iStrNameLen - 4) == string::npos &&
			g_strNewName.find("BAT", iStrNameLen - 4) == string::npos)
		{
			MT_WARN("[服务助手] 选择的应用格式不支持=%s", g_strNewPath.c_str());
			g_strNewName.clear();
			g_strNewPath.clear();
		}
		

		m_AppPathEdit.SetWindowText(strAppPath.c_str());
	}
}


void CServiceDlg::OnBnClickedAddButton()
{
	// TODO: 在此添加控件通知处理程序代码
	if (g_strNewName.empty() || g_strNewPath.empty())
		return;

	int iServiceTotal = m_ServiceList.GetItemCount();

	char strLabel[100] = { 0 };
	sprintf(strLabel, "Info_%d", iServiceTotal);

	ServiceInfo refServiceInfo;
	refServiceInfo.strName = g_strNewName;
	refServiceInfo.strPath = g_strNewPath;
	refServiceInfo.strCfg = g_strCfg;
	refServiceInfo.strLabel = strLabel;

	m_ServiceList.InsertItem(iServiceTotal, "", 1);
	m_ServiceList.SetItemText(iServiceTotal, 0, refServiceInfo.strName.c_str());
	m_ServiceList.SetItemText(iServiceTotal, 1, refServiceInfo.strPath.c_str());
	m_ServiceList.SetItemText(iServiceTotal, 2, "停止");

	//更新到服务数据
	g_mapServiceInfo[iServiceTotal] = refServiceInfo;
	CServiceDataMng::GetInstance()->UpdateAllServiceInfo(g_mapServiceInfo, refServiceInfo, ADD);

	MT_INFO("[服务助手] 新添加服务,名称=%s,路径=%s", g_strNewName.c_str(), g_strNewPath.c_str());

	g_strNewName.clear();
	g_strNewPath.clear();
}


void CServiceDlg::OnBnClickedDelButton()
{
	// TODO: 在此添加控件通知处理程序代码
	if (-1 == g_iServiceRow)
		return;

	if (g_mapServiceInfo.find(g_iServiceRow) == g_mapServiceInfo.end())
	{
		g_iServiceRow = -1;
		return;
	}

	//删除服务信息
	m_ServiceList.DeleteItem(g_iServiceRow);
	ServiceInfo &refServiceInfo = g_mapServiceInfo[g_iServiceRow];
	MT_INFO("[服务助手] 删除服务,名称=%s,路径=%s", refServiceInfo.strName.c_str(), refServiceInfo.strPath.c_str());
	g_mapServiceInfo.erase(g_iServiceRow);

	//清空路径
	m_AppPathEdit.SetWindowText("");

	//清空命令行
	m_AppParamEdit.SetWindowText("");

	//清空时间显示
	m_TimeList.DeleteAllItems();

	//删除服务数据
	CServiceDataMng::GetInstance()->UpdateAllServiceInfo(g_mapServiceInfo, refServiceInfo, DEL);

	g_iServiceRow = -1;
}

void CServiceDlg::OnDestroy()
{
	CDialogEx::OnDestroy();
	g_bCheck = true;

	// TODO: 在此处添加消息处理程序代码
	if (g_pLogDlg)
	{
		g_pLogDlg->Show(false);
	}

	CServiceDataMng::Release();
	CLog::Release();
}


void CServiceDlg::OnBnClickedLogButton()
{
	// TODO: 在此添加控件通知处理程序代码
	if (g_pLogDlg)
	{
		if (g_pLogDlg->IsIconic())
		{
			g_pLogDlg->ShowWindow(SW_SHOWNOACTIVATE);
		}
		else
		{
			g_pLogDlg->ShowWindow(SW_SHOW);
		}
		g_pLogDlg->SetWindowPos(&g_pLogDlg->wndTop, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	}
}


void CServiceDlg::OnCbnSelchangeWeekCombo()
{
	// TODO: 在此添加控件通知处理程序代码

	//得到行号，通过POSITION转化
	g_iServiceRow = GetSelectedItemIndex(m_ServiceList);

	//根据行号获取对应服务配置、刷新显示

	if (g_mapServiceInfo.find(g_iServiceRow) == g_mapServiceInfo.end())
	{
		g_iServiceRow = -1;
		return;
	}

	ServiceInfo refServiceInfo = g_mapServiceInfo[g_iServiceRow];

	//填充右边时间数据
	m_TimeList.DeleteAllItems();
	int j = m_WeekCombo.GetCurSel();
	WeekInfo enWeek = j == 0 ? MON : j == 1 ? TUE : j == 2 ? WED : j == 3 ? THU : j == 4 ? FRI : j == 5 ? SAT : j == 6 ? SUN : MON;

	vector<TimeInfo> vecTimeInfo = refServiceInfo.mapTimeConf[enWeek];
	int iTimeCount = vecTimeInfo.size();
	for (int j = 0; j < iTimeCount; j++)
	{
		m_TimeList.InsertItem(j, "", 1);
		m_TimeList.SetItemText(j, 0, vecTimeInfo[j].StartTime.toString().c_str());
		m_TimeList.SetItemText(j, 1, vecTimeInfo[j].EndTime.toString().c_str());
	}
}


void CServiceDlg::OnClose()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (MessageBox("你确定强制退出本系统吗？", "服务助手", MB_ICONQUESTION | MB_OKCANCEL) == IDCANCEL)
		return;
	HANDLE hself = GetCurrentProcess();
	TerminateProcess(hself, 0);

	CDialogEx::OnClose();
}
