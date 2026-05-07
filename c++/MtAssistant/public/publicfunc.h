#pragma once

#include <afxstr.h>
#include <windows.h>

BOOL GetFileVersion(LPCTSTR strFile, CString& strVersion);
BOOL CenterAndActivateWindow(HWND hWnd);

int KillProcess(long p_lProcessID);
// wyl 2026-05-06：停止完整进程树：先优雅退出，超时后强杀，并确认进程树持续清空。
int StopProcessTree(long p_lProcessID, const char* p_szProcessName, const char* p_szProcessFullPath,
	const char* p_szWindowTitle = NULL, int p_iGracefulWaitMs = 5 * 60 * 1000, int p_iForceWaitMs = 10 * 1000);
long FindProcessid(const char* p_szProcessName);
// wyl 2026-05-06：按完整程序路径查找已运行进程，避免服务已存在时重复拉起。
long FindProcessIdByPath(const char* p_szProcessName, const char* p_szProcessFullPath);
long IsProcessIdExists(long p_iProcessId);
long StartProcess(const char* p_szProcessWorkPath, const char* p_szProcessFullPath);
