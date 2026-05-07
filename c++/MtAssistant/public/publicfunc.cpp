#include "StdAfx.h"
#include "publicfunc.h"

#include "exception_handler.h"
#include "nsdk.h"
#include "nsdk_atomic.h"

#include <algorithm>
#include <ctime>
#include <cstring>
#include <io.h>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <tlhelp32.h>
#include <vector>


#define BREAKPAD_DUMP_FOLDER_NAME "CrashDumps"

namespace
{
	google_breakpad::ExceptionHandler* g_pBreakpad = nullptr;

	bool DumpCallback(const wchar_t* dump_path,
		const wchar_t* dump_id,
		void* context,
		EXCEPTION_POINTERS* exinfo,
		MDRawAssertionInfo* assertion,
		bool succeeded)
	{
		return succeeded;
	}

	// wyl 2026-05-06：统一进程路径格式，避免斜杠、引号和相对路径导致匹配误判。
	std::string NormalizeProcessPath(std::string strPath)
	{
		if (strPath.size() >= 2 && strPath.front() == '"' && strPath.back() == '"')
			strPath = strPath.substr(1, strPath.size() - 2);

		std::replace(strPath.begin(), strPath.end(), '/', '\\');

		char szFullPath[4096] = { 0 };
		DWORD dwLen = GetFullPathNameA(strPath.c_str(), sizeof(szFullPath), szFullPath, NULL);
		if (dwLen > 0 && dwLen < sizeof(szFullPath))
			strPath = szFullPath;

		return strPath;
	}

	// wyl 2026-05-06：读取进程真实可执行文件路径，用于和配置路径精确比对。
	bool QueryProcessPath(DWORD dwPid, std::string& strPath)
	{
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, dwPid);
		if (hProcess == NULL)
			return false;

		char szPath[4096] = { 0 };
		DWORD dwSize = sizeof(szPath);
		BOOL bOk = QueryFullProcessImageNameA(hProcess, 0, szPath, &dwSize);
		CloseHandle(hProcess);
		if (!bOk || dwSize == 0)
			return false;

		strPath.assign(szPath, dwSize);
		return true;
	}

	// wyl 2026-05-06：快速判断指定PID是否仍处于运行状态。
	bool IsProcessAlive(DWORD dwPid)
	{
		if (dwPid == 0)
			return false;

		HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, dwPid);
		if (hProcess == NULL)
			return false;

		DWORD dwWait = WaitForSingleObject(hProcess, 0);
		CloseHandle(hProcess);
		return dwWait == WAIT_TIMEOUT;
	}

	// wyl 2026-05-06：获取当前系统进程快照，后续统一基于快照分析父子关系。
	bool SnapshotProcesses(std::vector<PROCESSENTRY32>& vecProcess)
	{
		vecProcess.clear();
		HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hProcessSnap == NULL || hProcessSnap == INVALID_HANDLE_VALUE)
			return false;

		PROCESSENTRY32 stPe;
		stPe.dwSize = sizeof(stPe);
		BOOL bMore = Process32First(hProcessSnap, &stPe);
		while (bMore)
		{
			vecProcess.push_back(stPe);
			bMore = Process32Next(hProcessSnap, &stPe);
		}

		CloseHandle(hProcessSnap);
		return true;
	}

	// wyl 2026-05-06：按进程名和完整路径确认目标服务，避免误杀同名程序。
	bool IsMatchedServiceProcess(const PROCESSENTRY32& stPe, const char* p_szProcessName, const char* p_szProcessFullPath)
	{
		if (p_szProcessName == NULL || p_szProcessName[0] == '\0')
			return false;
		if (_stricmp(p_szProcessName, stPe.szExeFile) != 0)
			return false;

		if (p_szProcessFullPath == NULL || p_szProcessFullPath[0] == '\0')
			return true;

		std::string strProcessPath;
		if (!QueryProcessPath(stPe.th32ProcessID, strProcessPath))
			return false;

		const std::string strCfgPath = NormalizeProcessPath(p_szProcessFullPath);
		return _stricmp(strCfgPath.c_str(), NormalizeProcessPath(strProcessPath).c_str()) == 0;
	}

	struct WindowTitleFindData
	{
		const char* pszWindowTitle;
		std::set<DWORD>* psetPids;
	};

	// wyl 2026-05-06：枚举窗口标题，补充只能通过窗口标题定位的目标进程。
	BOOL CALLBACK EnumWindowTitleProc(HWND hWnd, LPARAM lParam)
	{
		WindowTitleFindData* pData = reinterpret_cast<WindowTitleFindData*>(lParam);
		char szTitle[512] = { 0 };
		GetWindowTextA(hWnd, szTitle, sizeof(szTitle));
		if (szTitle[0] != '\0' && strcmp(szTitle, pData->pszWindowTitle) == 0)
		{
			DWORD dwPid = 0;
			GetWindowThreadProcessId(hWnd, &dwPid);
			if (dwPid != 0)
				pData->psetPids->insert(dwPid);
		}
		return TRUE;
	}

	// wyl 2026-05-06：汇总PID、进程名路径、窗口标题匹配到的根进程。
	std::set<DWORD> FindRootPids(long lProcessID, const char* p_szProcessName, const char* p_szProcessFullPath, const char* p_szWindowTitle)
	{
		std::set<DWORD> setRoots;
		if (lProcessID > 0 && IsProcessAlive(static_cast<DWORD>(lProcessID)))
			setRoots.insert(static_cast<DWORD>(lProcessID));

		std::vector<PROCESSENTRY32> vecProcess;
		if (SnapshotProcesses(vecProcess))
		{
			for (size_t i = 0; i < vecProcess.size(); ++i)
			{
				if (IsMatchedServiceProcess(vecProcess[i], p_szProcessName, p_szProcessFullPath))
					setRoots.insert(vecProcess[i].th32ProcessID);
			}
		}

		if (p_szWindowTitle != NULL && p_szWindowTitle[0] != '\0')
		{
			WindowTitleFindData data;
			data.pszWindowTitle = p_szWindowTitle;
			data.psetPids = &setRoots;
			EnumWindows(EnumWindowTitleProc, reinterpret_cast<LPARAM>(&data));
		}

		return setRoots;
	}

	// wyl 2026-05-06：从根进程向下收集完整进程树，覆盖父子进程场景。
	std::vector<DWORD> CollectProcessTree(const std::set<DWORD>& setRoots)
	{
		std::vector<PROCESSENTRY32> vecProcess;
		std::vector<DWORD> vecTree;
		if (!SnapshotProcesses(vecProcess))
			return vecTree;

		std::set<DWORD> setAlive;
		std::map<DWORD, std::vector<DWORD> > mapChildren;
		for (size_t i = 0; i < vecProcess.size(); ++i)
		{
			setAlive.insert(vecProcess[i].th32ProcessID);
			mapChildren[vecProcess[i].th32ParentProcessID].push_back(vecProcess[i].th32ProcessID);
		}

		std::set<DWORD> setVisited;
		std::queue<DWORD> queuePids;
		for (std::set<DWORD>::const_iterator it = setRoots.begin(); it != setRoots.end(); ++it)
			queuePids.push(*it);

		while (!queuePids.empty())
		{
			DWORD dwPid = queuePids.front();
			queuePids.pop();
			if (!setVisited.insert(dwPid).second)
				continue;

			if (setAlive.find(dwPid) != setAlive.end())
				vecTree.push_back(dwPid);

			std::vector<DWORD>& vecChildren = mapChildren[dwPid];
			for (size_t i = 0; i < vecChildren.size(); ++i)
				queuePids.push(vecChildren[i]);
		}

		return vecTree;
	}

	// wyl 2026-05-06：重新扫描当前目标进程树，捕获关闭过程中重新拉起的进程。
	std::vector<DWORD> CollectCurrentTargets(const std::set<DWORD>& setOriginalRoots, const char* p_szProcessName,
		const char* p_szProcessFullPath, const char* p_szWindowTitle)
	{
		std::set<DWORD> setRoots = setOriginalRoots;
		std::set<DWORD> setCurrentRoots = FindRootPids(-1, p_szProcessName, p_szProcessFullPath, p_szWindowTitle);
		setRoots.insert(setCurrentRoots.begin(), setCurrentRoots.end());
		return CollectProcessTree(setRoots);
	}

	struct CloseWindowData
	{
		const std::set<DWORD>* psetPids;
	};

	// wyl 2026-05-06：向目标进程拥有的窗口发送关闭消息，实现优雅退出的第一步。
	BOOL CALLBACK EnumCloseWindowProc(HWND hWnd, LPARAM lParam)
	{
		CloseWindowData* pData = reinterpret_cast<CloseWindowData*>(lParam);
		DWORD dwPid = 0;
		GetWindowThreadProcessId(hWnd, &dwPid);
		if (dwPid != 0 && pData->psetPids->find(dwPid) != pData->psetPids->end())
			PostMessage(hWnd, WM_CLOSE, 0, 0);
		return TRUE;
	}

	// wyl 2026-05-06：批量通知目标进程窗口关闭。
	void RequestCloseWindows(const std::vector<DWORD>& vecPids)
	{
		std::set<DWORD> setPids(vecPids.begin(), vecPids.end());
		CloseWindowData data;
		data.psetPids = &setPids;
		EnumWindows(EnumCloseWindowProc, reinterpret_cast<LPARAM>(&data));
	}

	// wyl 2026-05-06：向控制台进程发送CTRL_BREAK，尽量让无窗口程序自行退出。
	void RequestConsoleBreak(const std::vector<DWORD>& vecPids)
	{
		std::set<DWORD> setSent;
		for (size_t i = 0; i < vecPids.size(); ++i)
		{
			DWORD dwPid = vecPids[i];
			if (dwPid == GetCurrentProcessId() || !setSent.insert(dwPid).second)
				continue;

			if (AttachConsole(dwPid))
			{
				SetConsoleCtrlHandler(NULL, TRUE);
				GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, 0);
				Sleep(100);
				FreeConsole();
				SetConsoleCtrlHandler(NULL, FALSE);
			}
		}
	}

	// wyl 2026-05-06：在指定时间内轮询进程树是否已经退出。
	bool WaitTargetsExit(const std::set<DWORD>& setOriginalRoots, const char* p_szProcessName,
		const char* p_szProcessFullPath, const char* p_szWindowTitle, int iWaitMs)
	{
		DWORD dwStart = GetTickCount();
		DWORD dwWait = iWaitMs > 0 ? static_cast<DWORD>(iWaitMs) : 0;
		while (true)
		{
			if (CollectCurrentTargets(setOriginalRoots, p_szProcessName, p_szProcessFullPath, p_szWindowTitle).empty())
				return true;

			if (GetTickCount() - dwStart >= dwWait)
				break;
			Sleep(100);
		}

		return CollectCurrentTargets(setOriginalRoots, p_szProcessName, p_szProcessFullPath, p_szWindowTitle).empty();
	}

	// wyl 2026-05-06：进程树清空后继续确认一段时间，防止子进程或守护进程短暂复活。
	bool ConfirmTargetsEmpty(const std::set<DWORD>& setOriginalRoots, const char* p_szProcessName,
		const char* p_szProcessFullPath, const char* p_szWindowTitle, int iConfirmMs)
	{
		DWORD dwStart = GetTickCount();
		DWORD dwWait = iConfirmMs > 0 ? static_cast<DWORD>(iConfirmMs) : 0;
		while (true)
		{
			if (!CollectCurrentTargets(setOriginalRoots, p_szProcessName, p_szProcessFullPath, p_szWindowTitle).empty())
				return false;

			if (GetTickCount() - dwStart >= dwWait)
				return true;
			Sleep(100);
		}
	}

	// wyl 2026-05-06：按子进程优先的顺序强制结束进程树。
	int ForceTerminateProcessTree(const std::vector<DWORD>& vecPids)
	{
		int iCount = 0;
		for (std::vector<DWORD>::const_reverse_iterator it = vecPids.rbegin(); it != vecPids.rend(); ++it)
		{
			DWORD dwPid = *it;
			if (dwPid == GetCurrentProcessId())
				continue;

			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwPid);
			if (hProcess == NULL)
				continue;

			if (TerminateProcess(hProcess, 4))
				++iCount;
			CloseHandle(hProcess);
		}
		return iCount;
	}
}

BOOL GetFileVersion(LPCTSTR strFile, CString& strVersion)
{
	DWORD dwHandle = 0;
	DWORD dwVerSize = GetFileVersionInfoSize(strFile, &dwHandle);
	if (dwVerSize == 0)
		return FALSE;

	std::vector<BYTE> vecVersion(dwVerSize);
	if (!GetFileVersionInfo(strFile, 0, dwVerSize, vecVersion.data()))
		return FALSE;

	VS_FIXEDFILEINFO* pInfo = NULL;
	UINT nInfoLen = 0;
	if (!VerQueryValue(vecVersion.data(), _T("\\"), reinterpret_cast<void**>(&pInfo), &nInfoLen) || pInfo == NULL)
		return FALSE;

	strVersion.Format(_T("%d.%d.%d.%d"),
		HIWORD(pInfo->dwFileVersionMS), LOWORD(pInfo->dwFileVersionMS),
		HIWORD(pInfo->dwFileVersionLS), LOWORD(pInfo->dwFileVersionLS));
	return TRUE;
}

BOOL CenterAndActivateWindow(HWND hWnd)
{
	if (hWnd == NULL)
		return FALSE;

	if (!SetForegroundWindow(hWnd))
		return FALSE;

	RECT rect = { 0 };
	if (!GetWindowRect(hWnd, &rect))
		return FALSE;

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	int windowWidth = rect.right - rect.left;
	int windowHeight = rect.bottom - rect.top;
	int x = (screenWidth - windowWidth) / 2;
	int y = (screenHeight - windowHeight) / 2;

	return SetWindowPos(hWnd, HWND_TOP, x, y, windowWidth, windowHeight, SWP_SHOWWINDOW);
}

// 初始化函数（你想在哪里调用都可以）
void InitBreakpad()
{
	if (g_pBreakpad != nullptr)
		return;

	std::string strDumpPath = nsdk::GetRootPath();
	if (!strDumpPath.empty())
	{
		if (strDumpPath.compare(strDumpPath.length() - 1, 1, NSDK_PATH_DELIMETER) != 0)
			strDumpPath += NSDK_PATH_DELIMETER;
	}
	strDumpPath += BREAKPAD_DUMP_FOLDER_NAME;

	if (nsdk::FolderExists(strDumpPath.c_str()) != NSDK_OK)
	{
		if (nsdk::CreateFolder(strDumpPath.c_str()) != NSDK_OK)
			return;
	}

	std::wstring strDumpPathW;
	int iWideLen = MultiByteToWideChar(CP_ACP, 0, strDumpPath.c_str(), -1, NULL, 0);
	if (iWideLen > 0)
	{
		std::vector<wchar_t> vecDumpPath(iWideLen);
		MultiByteToWideChar(CP_ACP, 0, strDumpPath.c_str(), -1, &vecDumpPath[0], iWideLen);
		strDumpPathW.assign(&vecDumpPath[0]);
	}
	else
	{
		return;
	}

	// 初始化崩溃捕获 参1：崩溃 dump 文件保存的目录 参2：崩溃过滤函数（回调）填 nullptr 表示 不过滤，所有崩溃都捕获。一般不用，保持 nullptr 即可
	// 参3：崩溃发生后的回调函数，可以记录日志、弹窗提示、上传 dump、做一些收尾工作
	// 参4：传递给回调函数的自定义参数（上下文）。不需要就填 nullptr
	// 参5：要捕获哪些类型的崩溃。（空指针、除零、数组越界、C++ 异常、纯虚函数、堆栈溢出、非法指令）
	g_pBreakpad = new google_breakpad::ExceptionHandler(
		strDumpPathW,
		nullptr,
		DumpCallback,
		nullptr,
		google_breakpad::ExceptionHandler::HANDLER_ALL
	);
}
int KillProcess(long p_lProcessID)
{
	if (p_lProcessID <= 0)
		return -1;

	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, static_cast<DWORD>(p_lProcessID));
	if (hProcess == NULL)
		return -2;

	int iRet = TerminateProcess(hProcess, 4) ? 0 : -3;
	CloseHandle(hProcess);
	return iRet;
}

// wyl 2026-05-06：停止完整进程树：先优雅退出，超时后强杀，并确认进程树持续清空。
int StopProcessTree(long p_lProcessID, const char* p_szProcessName, const char* p_szProcessFullPath,
	const char* p_szWindowTitle, int p_iGracefulWaitMs, int p_iForceWaitMs)
{
	// wyl 2026-05-06：进程树清空后再观察10秒，确认没有子进程或守护进程复活。
	const int iConfirmEmptyWaitMs = 10 * 1000;
	// wyl 2026-05-06：根进程集合同时覆盖已知PID、同名同路径进程和标题命中的窗口进程。
	std::set<DWORD> setRoots = FindRootPids(p_lProcessID, p_szProcessName, p_szProcessFullPath, p_szWindowTitle);
	if (setRoots.empty())
		return 0;

	// wyl 2026-05-06：每次停止前重新收集当前进程树，避免使用过期快照。
	std::vector<DWORD> vecTargets = CollectCurrentTargets(setRoots, p_szProcessName, p_szProcessFullPath, p_szWindowTitle);
	if (vecTargets.empty())
		return 0;

	// wyl 2026-05-06：先发送窗口关闭和控制台中断，让服务有机会自行释放资源。
	RequestCloseWindows(vecTargets);
	RequestConsoleBreak(vecTargets);
	// wyl 2026-05-06：最多等待5分钟优雅退出，具体等待时长由调用方传入。
	bool bExited = WaitTargetsExit(setRoots, p_szProcessName, p_szProcessFullPath, p_szWindowTitle, p_iGracefulWaitMs);
	if (!bExited)
	{
		// wyl 2026-05-06：优雅等待超时后再次扫描，按最新进程树执行强杀兜底。
		vecTargets = CollectCurrentTargets(setRoots, p_szProcessName, p_szProcessFullPath, p_szWindowTitle);
		ForceTerminateProcessTree(vecTargets);
		bExited = WaitTargetsExit(setRoots, p_szProcessName, p_szProcessFullPath, p_szWindowTitle, p_iForceWaitMs);
	}

	if (!bExited)
		return -1;

	// wyl 2026-05-06：退出后继续确认10秒，确认期间发现进程复活则进入下一轮强杀。
	if (ConfirmTargetsEmpty(setRoots, p_szProcessName, p_szProcessFullPath, p_szWindowTitle, iConfirmEmptyWaitMs))
		return 0;

	// wyl 2026-05-06：确认窗口内发现进程复活，重新收集后再强制结束。
	vecTargets = CollectCurrentTargets(setRoots, p_szProcessName, p_szProcessFullPath, p_szWindowTitle);
	ForceTerminateProcessTree(vecTargets);
	if (!WaitTargetsExit(setRoots, p_szProcessName, p_szProcessFullPath, p_szWindowTitle, p_iForceWaitMs))
		return -1;

	return ConfirmTargetsEmpty(setRoots, p_szProcessName, p_szProcessFullPath, p_szWindowTitle, iConfirmEmptyWaitMs) ? 0 : -1;
}

long StartProcess(const char* p_szProcessWorkPath, const char* p_szProcessFullPath)
{
	if (p_szProcessFullPath == NULL || p_szProcessFullPath[0] == '\0')
		return -1;

	if (_access(p_szProcessFullPath, 0) == -1)
		return -2;

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	const char* pWorkPath = (p_szProcessWorkPath != NULL && p_szProcessWorkPath[0] != '\0') ? p_szProcessWorkPath : NULL;
	// wyl 2026-05-06：创建独立进程组，后续才能向控制台服务发送CTRL_BREAK。
	if (!CreateProcessA(p_szProcessFullPath, NULL, NULL, NULL, FALSE, CREATE_NEW_PROCESS_GROUP, NULL, pWorkPath, &si, &pi))
		return -1;

	long lPid = static_cast<long>(pi.dwProcessId);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return lPid;
}

long FindProcessid(const char* p_szProcessName)
{
	if (p_szProcessName == NULL || p_szProcessName[0] == '\0')
		return -99;

	PROCESSENTRY32 stPe;
	stPe.dwSize = sizeof(stPe);
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == NULL)
		return -1;
	if (hProcessSnap == INVALID_HANDLE_VALUE)
		return -2;

	long lPid = -3;
	BOOL bMore = Process32First(hProcessSnap, &stPe);
	while (bMore)
	{
		if (_stricmp(p_szProcessName, stPe.szExeFile) == 0)
		{
			lPid = static_cast<long>(stPe.th32ProcessID);
			break;
		}
		bMore = Process32Next(hProcessSnap, &stPe);
	}
	CloseHandle(hProcessSnap);
	return lPid;
}

// wyl 2026-05-06：按完整程序路径查找已运行进程，优先绑定旧服务实例，避免重复启动。
long FindProcessIdByPath(const char* p_szProcessName, const char* p_szProcessFullPath)
{
	if (p_szProcessName == NULL || p_szProcessName[0] == '\0' ||
		p_szProcessFullPath == NULL || p_szProcessFullPath[0] == '\0')
		return -99;

	std::vector<PROCESSENTRY32> vecProcess;
	if (!SnapshotProcesses(vecProcess))
		return -1;

	for (size_t i = 0; i < vecProcess.size(); ++i)
	{
		if (IsMatchedServiceProcess(vecProcess[i], p_szProcessName, p_szProcessFullPath))
			return static_cast<long>(vecProcess[i].th32ProcessID);
	}

	return -2;
}
long IsProcessIdExists(long p_iProcessId)
{
	if (p_iProcessId <= 0)
		return -1;

	PROCESSENTRY32 stPe;
	stPe.dwSize = sizeof(stPe);
	HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == NULL)
		return -2;
	if (hProcessSnap == INVALID_HANDLE_VALUE)
		return -3;

	long lPid = -4;
	BOOL bMore = Process32First(hProcessSnap, &stPe);
	while (bMore)
	{
		if (stPe.th32ProcessID == static_cast<DWORD>(p_iProcessId))
		{
			lPid = p_iProcessId;
			break;
		}
		bMore = Process32Next(hProcessSnap, &stPe);
	}
	CloseHandle(hProcessSnap);
	return lPid;
}
