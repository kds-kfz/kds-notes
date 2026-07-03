
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
#include "ServiceConfigXml.h"
#include <algorithm>
#include <thread>

#include "nsdk.h"
#include "nsdk_atomic.h"


std::map<int, ServiceInfo> g_mapServiceInfo;
std::map<int, bool> g_mapServiceStatus;
int g_iServiceCount = -1;// wyl 2026-05-06：当前配置中的服务数量，刷新列表时用来限制遍历范围。
int g_iServiceRow = -1;// wyl 2026-05-06：当前选中的服务行号，启停和时间配置都依赖该索引。
string g_strNewName = "";// wyl 2026-05-06：文件选择后缓存待添加服务名。
string g_strNewPath = "";// wyl 2026-05-06：文件选择后缓存待添加服务完整路径。
string g_strCfg = "";
static const UINT_PTR SERVICE_STATUS_TIMER_ID = 1;// wyl 2026-05-06：服务状态刷新定时器ID，只在UI线程更新列表。
static const UINT WM_STOP_SERVICE_DONE = WM_APP + 101;// wyl 2026-07-03：后台Stop完成后通知UI线程刷新状态。
namespace
{
	struct LocalPickItem
	{
		string strText;
		string strPath;
		bool bDirectory;
	};

	struct LocalPickData
	{
		string strCurrentDir;
		string strSelectedPath;
		vector<LocalPickItem> vecItems;
	};

	// wyl 2026-05-06：只允许选择服务助手支持守护的exe/bat文件。
	bool IsSupportedProgramFile(const string& p_refName)
	{
		if (p_refName.length() < 4)
			return false;

		string strExt = p_refName.substr(p_refName.length() - 4);
		return _stricmp(strExt.c_str(), ".exe") == 0 || _stricmp(strExt.c_str(), ".bat") == 0;
	}

	WeekInfo WeekFromComboIndex(int p_iIndex)
	{
		if (p_iIndex == 0) return MON;
		if (p_iIndex == 1) return TUE;
		if (p_iIndex == 2) return WED;
		if (p_iIndex == 3) return THU;
		if (p_iIndex == 4) return FRI;
		if (p_iIndex == 5) return SAT;
		if (p_iIndex == 6) return SUN;
		return MON;
	}

	WeekInfo GetSelectedWeekInfo(CComboBox& p_refWeekCombo)
	{
		int iSel = p_refWeekCombo.GetCurSel();
		if (iSel >= 0)
			return WeekFromComboIndex(iSel);

		CString strWeekText;
		p_refWeekCombo.GetWindowText(strWeekText);
		if (strWeekText.CompareNoCase(_T("Tue")) == 0) return TUE;
		if (strWeekText.CompareNoCase(_T("Wed")) == 0) return WED;
		if (strWeekText.CompareNoCase(_T("Thu")) == 0) return THU;
		if (strWeekText.CompareNoCase(_T("Fri")) == 0) return FRI;
		if (strWeekText.CompareNoCase(_T("Sat")) == 0) return SAT;
		if (strWeekText.CompareNoCase(_T("Sun")) == 0) return SUN;
		return MON;
	}

	bool IsLocalPathDelimiter(char p_chValue)
	{
#if defined(OS_IS_WINDOWS)
		return p_chValue == NSDK_PATH_DELIMETER[0] || p_chValue == '/';
#else
		return p_chValue == NSDK_PATH_DELIMETER[0];
#endif
	}

	size_t FindLastLocalPathDelimiter(const string& p_refPath)
	{
		size_t nPos = p_refPath.find_last_of(NSDK_PATH_DELIMETER);
#if defined(OS_IS_WINDOWS)
		size_t nAltPos = p_refPath.find_last_of('/');
		if (nAltPos != string::npos && (nPos == string::npos || nAltPos > nPos))
			nPos = nAltPos;
#endif
		return nPos;
	}

	// wyl 2026-05-06：判断路径是否为盘符根目录，避免向上目录越界。
	bool IsDriveRootPath(const string& p_refPath)
	{
		return p_refPath.length() == 3 && p_refPath[1] == ':' && IsLocalPathDelimiter(p_refPath[2]);
	}

	// wyl 2026-05-06：拼接本地路径，统一补充分隔符。
	string JoinLocalPath(const string& p_refDir, const string& p_refName)
	{
		if (p_refDir.empty())
			return p_refName;

		char chLast = p_refDir[p_refDir.length() - 1];
		if (IsLocalPathDelimiter(chLast))
			return p_refDir + p_refName;

		return p_refDir + NSDK_PATH_DELIMETER + p_refName;
	}

	// wyl 2026-05-06：获取上一级目录，根目录保持不变。
	string GetParentLocalDir(string p_strDir)
	{
		if (p_strDir.empty() || IsDriveRootPath(p_strDir))
			return p_strDir;

		while (p_strDir.length() > 3 && IsLocalPathDelimiter(p_strDir[p_strDir.length() - 1]))
			p_strDir.erase(p_strDir.length() - 1);

		size_t pos = FindLastLocalPathDelimiter(p_strDir);
		if (pos == string::npos)
			return p_strDir;
		if (pos <= 2)
			return p_strDir.substr(0, pos + 1);

		return p_strDir.substr(0, pos);
	}

	// wyl 2026-05-06：向列表追加一项，并用列表项数据保存vecItems下标。
	void AddPickListItem(HWND p_hList, LocalPickData* p_pData, const string& p_refText, const string& p_refPath, bool p_bDirectory)
	{
		LocalPickItem stItem;
		stItem.strText = p_refText;
		stItem.strPath = p_refPath;
		stItem.bDirectory = p_bDirectory;
		p_pData->vecItems.push_back(stItem);

		int iIndex = (int)SendMessageA(p_hList, LB_ADDSTRING, 0, (LPARAM)p_refText.c_str());
		if (iIndex >= 0)
			SendMessageA(p_hList, LB_SETITEMDATA, iIndex, (LPARAM)(p_pData->vecItems.size() - 1));
	}

	// wyl 2026-05-06：刷新自定义文件选择列表，只用FindFirstFile枚举，不触发Shell文件框。
	void RefreshLocalProgramList(HWND p_hDlg, LocalPickData* p_pData)
	{
		HWND hList = GetDlgItem(p_hDlg, IDC_PICK_FILE_LIST);
		SendMessageA(hList, LB_RESETCONTENT, 0, 0);
		p_pData->vecItems.clear();
		SetDlgItemTextA(p_hDlg, IDC_PICK_PATH_EDIT, p_pData->strCurrentDir.c_str());

		char szDrives[512] = { 0 };
		DWORD dwDriveLen = GetLogicalDriveStringsA(sizeof(szDrives) - 1, szDrives);
		for (char* pDrive = szDrives; dwDriveLen > 0 && pDrive[0] != '\0'; pDrive += strlen(pDrive) + 1)
		{
			string strText = "[D] ";
			strText += pDrive;
			AddPickListItem(hList, p_pData, strText, pDrive, true);
		}

		string strParent = GetParentLocalDir(p_pData->strCurrentDir);
		if (!strParent.empty() && _stricmp(strParent.c_str(), p_pData->strCurrentDir.c_str()) != 0)
			AddPickListItem(hList, p_pData, "[..]", strParent, true);

		vector<LocalPickItem> vecDirs;
		vector<LocalPickItem> vecFiles;
		string strFindPath = JoinLocalPath(p_pData->strCurrentDir, "*");
		WIN32_FIND_DATAA stFindData;
		HANDLE hFind = FindFirstFileA(strFindPath.c_str(), &stFindData);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				string strName = stFindData.cFileName;
				if (strName == "." || strName == "..")
					continue;

				LocalPickItem stItem;
				stItem.strPath = JoinLocalPath(p_pData->strCurrentDir, strName);
				if ((stFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
				{
					stItem.strText = "[D] " + strName;
					stItem.bDirectory = true;
					vecDirs.push_back(stItem);
				}
				else if (IsSupportedProgramFile(strName))
				{
					stItem.strText = strName;
					stItem.bDirectory = false;
					vecFiles.push_back(stItem);
				}
			} while (FindNextFileA(hFind, &stFindData));
			FindClose(hFind);
		}

		auto fnSortByText = [](const LocalPickItem& p_refLeft, const LocalPickItem& p_refRight) {
			return _stricmp(p_refLeft.strText.c_str(), p_refRight.strText.c_str()) < 0;
		};
		sort(vecDirs.begin(), vecDirs.end(), fnSortByText);
		sort(vecFiles.begin(), vecFiles.end(), fnSortByText);

		for (size_t i = 0; i < vecDirs.size(); ++i)
			AddPickListItem(hList, p_pData, vecDirs[i].strText, vecDirs[i].strPath, true);
		for (size_t i = 0; i < vecFiles.size(); ++i)
			AddPickListItem(hList, p_pData, vecFiles[i].strText, vecFiles[i].strPath, false);
	}

	// wyl 2026-05-06：处理列表当前项，目录进入下一级，文件作为选择结果。
	bool AcceptLocalPickSelection(HWND p_hDlg, LocalPickData* p_pData)
	{
		HWND hList = GetDlgItem(p_hDlg, IDC_PICK_FILE_LIST);
		int iSel = (int)SendMessageA(hList, LB_GETCURSEL, 0, 0);
		if (iSel == LB_ERR)
			return false;

		int iItem = (int)SendMessageA(hList, LB_GETITEMDATA, iSel, 0);
		if (iItem < 0 || iItem >= (int)p_pData->vecItems.size())
			return false;

		LocalPickItem& refItem = p_pData->vecItems[iItem];
		if (refItem.bDirectory)
		{
			p_pData->strCurrentDir = refItem.strPath;
			RefreshLocalProgramList(p_hDlg, p_pData);
			return false;
		}

		p_pData->strSelectedPath = refItem.strPath;
		EndDialog(p_hDlg, IDOK);
		return true;
	}

	// wyl 2026-05-06：自定义文件选择窗口消息处理，不使用系统Shell文件选择框。
	INT_PTR CALLBACK LocalProgramPickerProc(HWND p_hDlg, UINT p_uMsg, WPARAM p_wParam, LPARAM p_lParam)
	{
		LocalPickData* pData = (LocalPickData*)GetWindowLongPtr(p_hDlg, DWLP_USER);
		switch (p_uMsg)
		{
		case WM_INITDIALOG:
			pData = (LocalPickData*)p_lParam;
			SetWindowLongPtr(p_hDlg, DWLP_USER, (LONG_PTR)pData);
			RefreshLocalProgramList(p_hDlg, pData);
			return TRUE;
		case WM_COMMAND:
			if (LOWORD(p_wParam) == IDCANCEL)
			{
				EndDialog(p_hDlg, IDCANCEL);
				return TRUE;
			}
			if (LOWORD(p_wParam) == IDOK)
			{
				AcceptLocalPickSelection(p_hDlg, pData);
				return TRUE;
			}
			if (LOWORD(p_wParam) == IDC_PICK_UP_BUTTON)
			{
				pData->strCurrentDir = GetParentLocalDir(pData->strCurrentDir);
				RefreshLocalProgramList(p_hDlg, pData);
				return TRUE;
			}
			if (LOWORD(p_wParam) == IDC_PICK_FILE_LIST && HIWORD(p_wParam) == LBN_DBLCLK)
			{
				AcceptLocalPickSelection(p_hDlg, pData);
				return TRUE;
			}
			break;
		}
		return FALSE;
	}

	// wyl 2026-05-06：弹出工程内文件选择窗口，彻底避开系统文件框创建log目录的问题。
	bool ShowLocalProgramPicker(HWND p_hOwner, string& p_refSelectedPath)
	{
		LocalPickData stData;
		stData.strCurrentDir = nsdk::GetRootPath();
		INT_PTR iRet = DialogBoxParamA(AfxGetInstanceHandle(), MAKEINTRESOURCEA(IDD_FILE_PICK_DIALOG), p_hOwner, LocalProgramPickerProc, (LPARAM)&stData);
		if (iRet != IDOK || stData.strSelectedPath.empty())
			return false;

		p_refSelectedPath = stData.strSelectedPath;
		return true;
	}
}



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
	m_bStopPending = false;
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
	ON_WM_TIMER()
	ON_WM_HELPINFO()
	ON_COMMAND(ID_HELP, &CServiceDlg::OnHelp)
	ON_BN_CLICKED(IDC_STOP_BUTTON, &CServiceDlg::OnBnClickedStopButton)
	ON_MESSAGE(WM_STOP_SERVICE_DONE, &CServiceDlg::OnStopServiceDone)
END_MESSAGE_MAP()

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


	// wyl 2026-05-06：初始化前固定主窗口标题，避免标题带进程号导致下次启动无法找到旧窗口。
	CString strProcessID;
	CString	strVersion;
	char szProcessName[256] = { 0 };
	GetModuleFileName(NULL, szProcessName, 256);
	GetFileVersion(szProcessName, strVersion);
	strProcessID.Format("MtAssistant(Ver:%s)", strVersion);

	HWND hwnd = ::FindWindow(NULL, strProcessID);
	if (hwnd)
	{
		BOOL bFlag = CenterAndActivateWindow(hwnd);
		if (bFlag)// wyl 2026-05-06：找到旧窗口并激活成功后退出当前实例。
		{
			ExitProcess(0);
		}
		else
		{
			::MessageBox(NULL, _T("Cannot activate assistant window"), _T("Error"), MB_ICONERROR); // wyl 2026-05-06：弹窗文本保持英文，兼容无中文语言包环境。
			ExitProcess(0);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	// wyl 2026-07-03：日志窗口必须在主UI线程创建，避免跨线程创建MFC窗口导致随机弹错或假死。
	if (g_pLogDlg == NULL)
	{
		g_pLogDlg = new CLogDlg(this);
		if (!g_pLogDlg->Create(IDD_LOG_DIALOG, this))
		{
			delete g_pLogDlg;
			g_pLogDlg = NULL;
		}
		else
		{
			g_pLogDlg->ShowWindow(SW_HIDE);
		}
	}

	m_Font.CreatePointFont(90, _T("Segoe UI"));

	m_ServiceList.SetFont(&m_Font);
	m_ServiceList.ModifyStyle(0, LVS_REPORT);
	m_ServiceList.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	m_ServiceList.InsertColumn(0, "Program");
	m_ServiceList.InsertColumn(1, "Path");
	m_ServiceList.InsertColumn(2, "Status");

	m_ServiceList.SetColumnWidth(0, 120); //设置列宽
	m_ServiceList.SetColumnWidth(1, 300);
	m_ServiceList.SetColumnWidth(2, 80);

	m_TimeList.SetFont(&m_Font);
	m_TimeList.ModifyStyle(0, LVS_REPORT);
	m_TimeList.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT);

	m_TimeList.InsertColumn(0, "Start");
	m_TimeList.InsertColumn(1, "End");

	m_TimeList.SetColumnWidth(0, 130); //设置列宽
	m_TimeList.SetColumnWidth(1, 130);

	m_WeekCombo.SetFont(&m_Font);
	m_WeekCombo.AddString(_T("Mon"));
	m_WeekCombo.AddString(_T("Tue"));
	m_WeekCombo.AddString(_T("Wed"));
	m_WeekCombo.AddString(_T("Thu"));
	m_WeekCombo.AddString(_T("Fri"));
	m_WeekCombo.AddString(_T("Sat"));
	m_WeekCombo.AddString(_T("Sun"));
	m_WeekCombo.SetCurSel(0);// wyl 2026-07-03：设置真实选中项，避免只显示文本导致新增时间保存到错误星期。

	m_ServiceList.ModifyStyle(0, LVS_REPORT);
	m_ServiceList.SetExtendedStyle(LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);

	
	SetWindowText(strProcessID);

	//初始化服务数据
	CServiceDataMng::GetInstance()->Init();

	//获取配置
	g_strCfg = CServiceDataMng::GetInstance()->GetCfgPath();

	//初始化全局变量并填充展示
	ReloadServiceList();

	//初始化完成
	m_bStatus = true;

	if (g_pLogDlg)
	{
		g_pLogDlg->Show(true);
	}

	SetTimer(SERVICE_STATUS_TIMER_ID, 1000, NULL);// wyl 2026-05-06：每秒刷新状态，避免工作线程频繁触碰UI控件。

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
	m_AppPathEdit.SetWindowText(refServiceInfo.strPath.c_str());
	//m_AppParamEdit.SetWindowText(refServiceInfo.strCmdParam.c_str());

	//填充右边时间数据
	m_TimeList.DeleteAllItems();
	WeekInfo enWeek = GetSelectedWeekInfo(m_WeekCombo);

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

// wyl 2026-05-06：勾选复选框时只处理状态位变化，避免快速点击导致重复逻辑。
void CServiceDlg::OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_LISTVIEW* pNMListView = reinterpret_cast<NM_LISTVIEW*>(pNMHDR);
	*pResult = 0;

	if (!m_bStatus || pNMListView == NULL || pNMListView->iItem < 0)
		return;

	if ((pNMListView->uChanged & LVIF_STATE) == 0)
		return;

	// wyl 2026-05-06：只比较复选框状态位，过滤选中行、焦点变化等无关通知。
	UINT uOldCheckState = pNMListView->uOldState & LVIS_STATEIMAGEMASK;
	UINT uNewCheckState = pNMListView->uNewState & LVIS_STATEIMAGEMASK;
	if (uOldCheckState == uNewCheckState)
		return;

	int iServiceRow = pNMListView->iItem;
	if (g_mapServiceInfo.find(iServiceRow) == g_mapServiceInfo.end())
		return;

	bool bEnable = m_ServiceList.GetCheck(iServiceRow) == TRUE;
	g_iServiceRow = iServiceRow;

	// wyl 2026-05-06：先落配置再更新内存状态，确保界面和守护线程最终一致。
	CServiceDataMng::GetInstance()->UpdateEnable(iServiceRow, bEnable ? 1 : 0);
	g_mapServiceInfo[iServiceRow].iEnable = bEnable ? 1 : 0;

	if (bEnable)
	{
		ServiceInfo stServiceInfo = CServiceDataMng::GetInstance()->GetServiceInfo(iServiceRow);
		if (stServiceInfo.strName.empty() && stServiceInfo.strPath.empty())
			return;

		// wyl 2026-05-06：勾选启用时创建守护线程，由线程负责启动和监控服务。
		CServiceDataMng::GetInstance()->CreateDaemon(iServiceRow, &stServiceInfo);
	}
	else
	{
		// wyl 2026-05-06：取消勾选只停止守护线程，当前服务状态显示保持不变。
		CServiceDataMng::GetInstance()->DestroyDaemon(iServiceRow);
		g_iServiceRow = -1;
	}
}
int CServiceDlg::GetSelectedItemIndex(CListCtrl& p_listCtrl)
{
	return p_listCtrl.GetNextItem(-1, LVNI_SELECTED);
}

void CServiceDlg::ClearServiceDetail()
{
	g_iServiceRow = -1;
	m_AppPathEdit.SetWindowText("");
	m_AppParamEdit.SetWindowText("");
	m_TimeList.DeleteAllItems();
}

void CServiceDlg::ReloadServiceList()
{
	bool bOldStatus = m_bStatus;
	m_bStatus = false;

	m_ServiceList.DeleteAllItems();
	g_iServiceCount = CServiceDataMng::GetInstance()->GetAllServiceInfo(g_mapServiceInfo);
	CServiceDataMng::GetInstance()->GetAllServiceStatus(g_mapServiceStatus);

	for (int i = 0; i < g_iServiceCount; i++)
	{
		auto itService = g_mapServiceInfo.find(i);
		if (itService == g_mapServiceInfo.end())
			continue;

		ServiceInfo& refServiceInfo = itService->second;
		m_ServiceList.InsertItem(i, "", 1);
		m_ServiceList.SetItemText(i, 0, refServiceInfo.strName.c_str());
		m_ServiceList.SetItemText(i, 1, refServiceInfo.strPath.c_str());

		auto itStatus = g_mapServiceStatus.find(i);
		bool bRunning = itStatus != g_mapServiceStatus.end() && itStatus->second;
		m_ServiceList.SetItemText(i, 2, bRunning ? "Running" : "Stopped");
		m_ServiceList.SetCheck(i, 1 == refServiceInfo.iEnable);
	}

	m_bStatus = bOldStatus;
}

void CServiceDlg::OnBnClickedTimeAddButton()
{
	// TODO: 在此添加控件通知处理程序代码
	//获取服务所在行
	//int iRow = GetSelectedItemIndex(m_ServiceList);

	if (-1 == g_iServiceRow)
		return;

	//获取周所在行
	WeekInfo enWeek = GetSelectedWeekInfo(m_WeekCombo);

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
	WeekInfo enWeek = GetSelectedWeekInfo(m_WeekCombo);

	//获取起止时间
	TimeInfo stTimeInfo;

	//删除选中的时间
	//所在行
	int iTimeRow = GetSelectedItemIndex(m_TimeList);
	if (-1 == iTimeRow)
		return;

	m_TimeList.DeleteItem(iTimeRow);

	CServiceDataMng::GetInstance()->UpdateTimeInfo(g_iServiceRow, enWeek, iTimeRow, stTimeInfo, DEL);
	g_iServiceCount = CServiceDataMng::GetInstance()->GetAllServiceInfo(g_mapServiceInfo);

	//更新配置到守护线程
}


void CServiceDlg::OnBnClickedLookButton()
{
	// TODO: 在此添加控件通知处理程序代码
	//打开程序路径
    // wyl 2026-05-06：旧写法保留备用，暂时注释掉；该写法会调用Windows系统/Shell文件选择框。
    // wyl 2026-05-06：当前运行环境中，Shell文件框会触发组件按相对路径创建程序同级log目录，所以先试用工程内文件选择窗口。
    // CFileDialog fileDlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
    //     _T("Program Files (*.exe;*.bat)|*.exe;*.bat|All Files (*.*)|*.*||"), this);
    // if (fileDlg.DoModal() != IDOK)
    //     return;
    // CString strOldAppPath = fileDlg.GetPathName();
    // std::string strAppPath = strOldAppPath.GetBuffer(0);

	// wyl 2026-05-06：使用工程内文件选择窗口，不调用系统Shell文件框，避免点击Browse创建同级log目录。
	std::string strAppPath;
	if (!ShowLocalProgramPicker(GetSafeHwnd(), strAppPath))
		return;

	std::string strFullPath;
	if (!NormalizeFullProgramPath(strAppPath, strFullPath))
	{
		MT_WARN("[MtAssistant] invalid selected app path=%s", strAppPath.c_str());
		g_strNewName.clear();
		g_strNewPath.clear();
		return;
	}

	g_strNewName = GetProgramFileName(strFullPath);
	g_strNewPath = strFullPath;

	// wyl 2026-05-06：复用统一后缀校验，避免短文件名触发越界判断。
	if (!IsSupportedProgramFile(g_strNewName))
	{
		MT_WARN("[MtAssistant] unsupported app file=%s", g_strNewPath.c_str());
		g_strNewName.clear();
		g_strNewPath.clear();
	}

	m_AppPathEdit.SetWindowText(g_strNewPath.c_str());
}


void CServiceDlg::OnBnClickedAddButton()
{
	// TODO: 在此添加控件通知处理程序代码
	if (g_strNewName.empty() || g_strNewPath.empty())
		return;

	std::string strFullPath;
	if (!NormalizeFullProgramPath(g_strNewPath, strFullPath))
	{
		MT_WARN("[MtAssistant] invalid app path=%s", g_strNewPath.c_str());
		return;
	}
	g_strNewPath = strFullPath;
	g_strNewName = GetProgramFileName(g_strNewPath);

	int iServiceTotal = m_ServiceList.GetItemCount();

	char strLabel[100] = { 0 };
	sprintf(strLabel, "Info_%d", iServiceTotal);

	ServiceInfo refServiceInfo;
	refServiceInfo.strName = g_strNewName;
	refServiceInfo.strPath = g_strNewPath;
	refServiceInfo.strCfg = g_strCfg;
	refServiceInfo.strLabel = strLabel;
	refServiceInfo.iRow = iServiceTotal;
	// wyl 2026-05-06：添加服务时先按完整路径查找旧进程，命中则写入旧PID，后续直接接管。
	refServiceInfo.lPid = FindProcessIdByPath(refServiceInfo.strName.c_str(), refServiceInfo.strPath.c_str());
	if (refServiceInfo.lPid > 0)
	{
		MT_INFO("[MtAssistant] service already started, name=%s,path=%s,pid=%ld",
			refServiceInfo.strName.c_str(), refServiceInfo.strPath.c_str(), refServiceInfo.lPid);
	}
	m_ServiceList.InsertItem(iServiceTotal, "", 1);
	m_ServiceList.SetItemText(iServiceTotal, 0, refServiceInfo.strName.c_str());
	m_ServiceList.SetItemText(iServiceTotal, 1, refServiceInfo.strPath.c_str());
	m_ServiceList.SetItemText(iServiceTotal, 2, refServiceInfo.lPid > 0 ? "Running" : "Stopped");
	if (refServiceInfo.lPid > 0)
		g_mapServiceStatus[iServiceTotal] = true;

	//更新到服务数据
	g_mapServiceInfo[iServiceTotal] = refServiceInfo;
	// wyl 2026-05-06：新增服务后立即更新服务数量，定时刷新才能覆盖新行状态。
	g_iServiceCount = iServiceTotal + 1;
	CServiceDataMng::GetInstance()->UpdateAllServiceInfo(g_mapServiceInfo, refServiceInfo, ADD);

	MT_INFO("[MtAssistant] add service, name=%s,path=%s", g_strNewName.c_str(), g_strNewPath.c_str());

	g_strNewName.clear();
	g_strNewPath.clear();
}


void CServiceDlg::OnBnClickedDelButton()
{
	int iServiceRow = GetSelectedItemIndex(m_ServiceList);
	if (-1 == iServiceRow)
		iServiceRow = g_iServiceRow;

	if (-1 == iServiceRow)
		return;

	if (g_mapServiceInfo.find(iServiceRow) == g_mapServiceInfo.end())
	{
		ClearServiceDetail();
		return;
	}

	ServiceInfo stDeletedServiceInfo = g_mapServiceInfo[iServiceRow];
	MT_INFO("[MtAssistant] delete service start, row=%d,name=%s,path=%s",
		iServiceRow, stDeletedServiceInfo.strName.c_str(), stDeletedServiceInfo.strPath.c_str());

	bool bOldStatus = m_bStatus;
	m_bStatus = false;
	g_mapServiceInfo.erase(iServiceRow);

	CServiceDataMng::GetInstance()->UpdateAllServiceInfo(g_mapServiceInfo, stDeletedServiceInfo, DEL);
	ReloadServiceList();
	ClearServiceDetail();
	m_bStatus = bOldStatus;

	MT_INFO("[MtAssistant] delete service done, row=%d,name=%s,path=%s",
		iServiceRow, stDeletedServiceInfo.strName.c_str(), stDeletedServiceInfo.strPath.c_str());
}

void CServiceDlg::OnDestroy()
{
	m_bStatus = false;
	KillTimer(SERVICE_STATUS_TIMER_ID);// wyl 2026-05-06：窗口销毁时停止状态刷新定时器。
	CDialogEx::OnDestroy();

	// TODO: 在此处添加消息处理程序代码
	if (g_pLogDlg)
	{
		CLogDlg* pLogDlg = g_pLogDlg;
		g_pLogDlg = NULL;
		pLogDlg->Show(false);
		pLogDlg->DestroyWindow();
		delete pLogDlg;
	}

	CServiceDataMng::Release();
	CLog::Release();
}


// wyl 2026-05-06：在UI线程定时刷新状态，避免工作线程直接操作列表控件。
void CServiceDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == SERVICE_STATUS_TIMER_ID)
	{
		if (!m_bStatus)
			return;

		// wyl 2026-05-06：状态采集和列表刷新都放在UI线程，避免跨线程操作控件。
		CServiceDataMng::GetInstance()->GetAllServiceStatus(g_mapServiceStatus);

		// wyl 2026-05-06：按配置数量和列表行数取较小值，防止刷新越界。
		int iItemCount = m_ServiceList.GetItemCount();
		int iServiceCount = g_iServiceCount < iItemCount ? g_iServiceCount : iItemCount;
		for (int i = 0; i < iServiceCount; i++)
		{
			auto it = g_mapServiceStatus.find(i);
			if (it != g_mapServiceStatus.end())
			{
				m_ServiceList.SetItemText(i, 2, it->second ? "Running" : "Stopped");
			}
		}
		return;
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CServiceDlg::OnBnClickedLogButton()
{
	if (g_pLogDlg == NULL)
	{
		g_pLogDlg = new CLogDlg(this);
		if (!g_pLogDlg->Create(IDD_LOG_DIALOG, this))
		{
			delete g_pLogDlg;
			g_pLogDlg = NULL;
			return;
		}
		g_pLogDlg->Show(true);
	}

	if (g_pLogDlg->IsIconic())
		g_pLogDlg->ShowWindow(SW_RESTORE);
	else
		g_pLogDlg->ShowWindow(SW_SHOW);
	g_pLogDlg->SetWindowPos(&g_pLogDlg->wndTop, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
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
	WeekInfo enWeek = GetSelectedWeekInfo(m_WeekCombo);

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
	if (MessageBox("Force exit this app?", "MtAssistant", MB_ICONQUESTION | MB_OKCANCEL) == IDCANCEL)
		return;
	HANDLE hself = GetCurrentProcess();
	TerminateProcess(hself, 0);

	CDialogEx::OnClose();
}

void CServiceDlg::OnBnClickedStopButton()
{
	if (m_bStopPending)
		return;

	int iServiceRow = GetSelectedItemIndex(m_ServiceList);
	if (-1 == iServiceRow)
		iServiceRow = g_iServiceRow;

	if (-1 == iServiceRow)
		return;

	if (g_mapServiceInfo.find(iServiceRow) == g_mapServiceInfo.end())
	{
		g_iServiceRow = -1;
		return;
	}

	m_bStopPending = true;
	CWnd* pStopButton = GetDlgItem(IDC_STOP_BUTTON);
	if (pStopButton != NULL)
		pStopButton->EnableWindow(FALSE);

	bool bOldStatus = m_bStatus;
	m_bStatus = false;
	m_ServiceList.SetCheck(iServiceRow, FALSE);
	m_bStatus = bOldStatus;

	if (g_mapServiceInfo.find(iServiceRow) != g_mapServiceInfo.end())
		g_mapServiceInfo[iServiceRow].iEnable = 0;
	g_mapServiceStatus[iServiceRow] = true;
	m_ServiceList.SetItemText(iServiceRow, 2, "Stopping");

	HWND hWnd = GetSafeHwnd();
	std::thread([hWnd, iServiceRow]() {
		int iStopRet = CServiceDataMng::GetInstance()->StopService(iServiceRow);
		if (::IsWindow(hWnd))
			::PostMessage(hWnd, WM_STOP_SERVICE_DONE, (WPARAM)iServiceRow, (LPARAM)iStopRet);
	}).detach();
}

LRESULT CServiceDlg::OnStopServiceDone(WPARAM wParam, LPARAM lParam)
{
	int iServiceRow = (int)wParam;
	int iStopRet = (int)lParam;

	m_bStopPending = false;
	CWnd* pStopButton = GetDlgItem(IDC_STOP_BUTTON);
	if (pStopButton != NULL)
		pStopButton->EnableWindow(TRUE);

	g_iServiceCount = CServiceDataMng::GetInstance()->GetAllServiceInfo(g_mapServiceInfo);
	CServiceDataMng::GetInstance()->GetAllServiceStatus(g_mapServiceStatus);

	if (iServiceRow >= 0 && iServiceRow < m_ServiceList.GetItemCount())
	{
		bool bOldStatus = m_bStatus;
		m_bStatus = false;
		m_ServiceList.SetCheck(iServiceRow, FALSE);
		m_bStatus = bOldStatus;
	}

	if (g_mapServiceInfo.find(iServiceRow) != g_mapServiceInfo.end())
		g_mapServiceInfo[iServiceRow].iEnable = 0;

	auto itStatus = g_mapServiceStatus.find(iServiceRow);
	bool bRunning = itStatus != g_mapServiceStatus.end() && itStatus->second;
	if (iServiceRow >= 0 && iServiceRow < m_ServiceList.GetItemCount())
		m_ServiceList.SetItemText(iServiceRow, 2, bRunning ? "Running" : "Stopped");
	if (0 == iStopRet && !bRunning)
		g_mapServiceStatus[iServiceRow] = false;

	return 0;
}

BOOL CServiceDlg::OnHelpInfo(HELPINFO* pHelpInfo)
{
	return TRUE;
}

void CServiceDlg::OnHelp()
{
}

BOOL CServiceDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg != NULL && pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_F1)
		return TRUE;

	return CDialogEx::PreTranslateMessage(pMsg);
}
