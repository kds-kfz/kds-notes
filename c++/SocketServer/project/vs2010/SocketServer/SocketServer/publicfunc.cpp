#include "publicfunc.h"

#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <time.h>
#include <assert.h>
#include <math.h>

#if defined( OS_IS_WINDOWS )
#include <io.h>
#include <atltime.h>
#include <windows.h>
#else
#include <unistd.h>
#include <ctime>
#include <chrono>
#include <sys/time.h>
#include <iostream>
#include <dirent.h>
#include <stdarg.h>
#endif

#ifdef OS_IS_WINDOWS
#define PATH_DELIMETER "\\"
#elif defined(__GNUC__)
#define PATH_DELIMETER "/"
#else
#error compiler not supported!
#endif

#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))

bool IsLeftDeli(int i)
{
	return '\\' == i;
}

void TranslatePath(std::string& p_str)
{
	std::replace_if(p_str.begin(), p_str.end(), IsLeftDeli, '/');
}

bool RegularPath(std::string& p_strPath)
{
#if defined( OS_IS_WINDOWS )
	std::replace(p_strPath.begin(), p_strPath.end(), TEXT('/'), TEXT('\\'));
	//	if (sPath.find(TEXT(':')) == sPath.npos)
	//		sPath.insert(0, theApp.HomePath());

	for (size_t pos = p_strPath.find(TEXT("\\\\")); pos != p_strPath.npos; pos = p_strPath.find(TEXT("\\\\"), pos))
	{
		p_strPath.erase(pos, 1);
	}
	for (size_t pos = p_strPath.find(TEXT("\\.\\")); pos != p_strPath.npos; pos = p_strPath.find(TEXT("\\.\\"), pos))
	{
		p_strPath.erase(pos, 2);
	}
	for (size_t pos = p_strPath.find(TEXT("\\..\\")); pos != p_strPath.npos; pos = p_strPath.find(TEXT("\\..\\"), pos))
	{
		size_t last = p_strPath.rfind(TEXT('\\'), pos - 1);
		assert(pos && last != p_strPath.npos);
		p_strPath.erase(last, pos - last + 3);
	}
#else
	TranslatePath(p_strPath);
#endif
	return true;
}

int FolderExists(const char* p_szFolderPath)
{
	if (p_szFolderPath == nullptr || strlen(p_szFolderPath) == 0)
		return -1;

	struct stat st;
	if (stat(p_szFolderPath, &st) != 0)
		return -2;

	return st.st_mode & S_IFDIR ? 0 : -3;
}

int CreateFolder(const char* p_szFolderPath)
{
	if (NULL == p_szFolderPath)
		return -1;

	std::string strPath(p_szFolderPath);
#if defined( OS_IS_WINDOWS )
	RegularPath(strPath);
	for (size_t pos = strPath.find(TEXT('\\')); pos != strPath.npos; pos = strPath.find(TEXT('\\'), pos + 1))
	{
		strPath[pos] = 0;
		DWORD attr = ::GetFileAttributes(strPath.c_str());
		if (attr == -1 || !(attr & FILE_ATTRIBUTE_DIRECTORY))
		{
			if (!::CreateDirectory(strPath.c_str(), NULL))
				return -1;
		}
		strPath[pos] = TEXT('\\');
	}
	DWORD attr = ::GetFileAttributes(strPath.c_str());
	if (attr == -1 || !(attr & FILE_ATTRIBUTE_DIRECTORY))
	{
		if (!::CreateDirectory(strPath.c_str(), NULL))
			return -2;
	}
#else
	unsigned int iBeginCmpPath = 0;
	unsigned int iEndCmpPath = 0;

	std::string fullPath = "";

	if ('/' != strPath[0])
	{
		fullPath = getcwd(nullptr, 0);
		iBeginCmpPath = fullPath.size();
		fullPath = fullPath + "/" + strPath;
	}
	else
	{
		fullPath = strPath;
		iBeginCmpPath = 1;
	}

	if (fullPath[fullPath.size() - 1] != '/')
	{
		fullPath += "/";
	}

	iEndCmpPath = fullPath.size();

	for (uint32_t i = iBeginCmpPath; i < iEndCmpPath; i++)
	{
		if ('/' == fullPath[i])
		{
			std::string curPath = fullPath.substr(0, i);
			if (access(curPath.c_str(), F_OK) != 0)
			{
				//S_IRUSR 用户读权限 S_IWUSR 用户写权限 S_IRGRP 用户组读权限 S_IWGRP 用户组写权限 S_IROTH 其他组读权限 S_IWOTH 其他组写权限
				//至少764才能进入操作读写
				if (mkdir(curPath.c_str(),  S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH) == -1)
				{
					return -1;
				}
			}
		}
	}
#endif

	return 0;
}

int GetCurDateTime(char* p_pDateTime, int p_iBufLen)
{
	if (NULL == p_pDateTime || p_iBufLen <= 0)
		return -1;

	static char s_szDateTime[NSDK_MAX_PATH] = { 0 };
#if defined( OS_IS_WINDOWS )
	// 时间
	SYSTEMTIME stCurrTime = { 0 };
	GetLocalTime(&stCurrTime);

	_snprintf(s_szDateTime, NSDK_MAX_PATH - 1, "%04d%02d%02d %02d:%02d %02d:%03d",
		stCurrTime.wYear, stCurrTime.wMonth, stCurrTime.wDay, stCurrTime.wHour,
		stCurrTime.wMinute, stCurrTime.wSecond, stCurrTime.wMilliseconds);
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	std::tm* tm_ptr = localtime(&tv.tv_sec);

	snprintf(s_szDateTime, NSDK_MAX_PATH - 1, "%04d%02d%02d %02d:%02d %02d:%03ld",
		tm_ptr->tm_year + 1900, tm_ptr->tm_mon + 1, tm_ptr->tm_mday, tm_ptr->tm_hour,
		tm_ptr->tm_min, tm_ptr->tm_sec, tv.tv_usec);
#endif
	int iBufLen = MIN(strlen(s_szDateTime), p_iBufLen);
	memcpy(p_pDateTime, s_szDateTime, iBufLen);
	return 0;
}

unsigned int GetCurDate(bool p_bDate)
{
	time_t now_t = time(NULL);
	tm now_time;
#if defined(OS_IS_WINDOWS )
	localtime_s(&now_time, &now_t);
#else
	now_time = *localtime(&now_t);
#endif
	if (p_bDate)
		return (now_time.tm_year + 1900) * 10000 + (now_time.tm_mon + 1) * 100 + now_time.tm_mday;
	else
		return now_time.tm_hour * 10000 + now_time.tm_min * 100 + now_time.tm_sec;
}
