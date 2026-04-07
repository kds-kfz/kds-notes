#include "nsdk.h"
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
#else
#include <unistd.h>
#include <sys/time.h>
#include <iostream>
#include <dirent.h>
#endif

#ifdef OS_IS_WINDOWS
#define PATH_DELIMETER "\\"
#elif defined(__GNUC__)
#define PATH_DELIMETER "/"
#else
#error compiler not supported!
#endif

#define NSDK_MAX_PATH 260

#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))

BGN_NAMESPACE_NSDK

/******************** 文件夹处理 ********************/
int CFolder::FolderExists(const char* p_szFolderPath)
{
	if (p_szFolderPath == nullptr || strlen(p_szFolderPath) == 0)
		return -1;

	struct stat st;
	if (stat(p_szFolderPath, &st) != 0)
		return -2;

	return st.st_mode & S_IFDIR ? 0 : -3;
}

int CFolder::CreateFolder(const char* p_szFolderPath)
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
				if (mkdir(curPath.c_str(), S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH) == -1)
				{
					return -1;
				}
			}
		}
	}
#endif

	return 0;
}

bool CFolder::RegularPath(std::string& p_strPath)
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
	std::replace(p_strPath.begin(), p_strPath.end(), '\\', '/');
	//	if (sPath.find(TEXT(':')) == sPath.npos)
	//		sPath.insert(0, theApp.HomePath());

	for (size_t pos = p_strPath.find("//"); pos != p_strPath.npos; pos = p_strPath.find("//", pos))
	{
		p_strPath.erase(pos, 1);
	}
	for (size_t pos = p_strPath.find("/./"); pos != p_strPath.npos; pos = p_strPath.find("/./", pos))
	{
		p_strPath.erase(pos, 2);
	}
	for (size_t pos = p_strPath.find("/../"); pos != p_strPath.npos; pos = p_strPath.find("/../", pos))
	{
		size_t last = p_strPath.rfind(" / ", pos - 1);
		assert(pos && last != p_strPath.npos);
		p_strPath.erase(last, pos - last + 3);
	}
#endif
	return true;
}

const char* CFolder::GetRootPath(void)
{
	static char s_szMoundlePath[NSDK_MAX_PATH] = { 0 };
#if defined( OS_IS_WINDOWS )
	GetCurrentDirectoryA(NSDK_MAX_PATH, s_szMoundlePath);
#else
	getcwd(s_szMoundlePath, NSDK_MAX_PATH);
#endif
	return s_szMoundlePath;
}

int CFolder::GetAllFiles(std::string p_strDir, std::vector<std::string>& p_vecFiles)
{
#if defined( OS_IS_WINDOWS )
	WIN32_FIND_DATA findData;
	std::string strFindFile = p_strDir + "\\*";
	HANDLE hFind = FindFirstFile(strFindFile.c_str(), &findData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return -1;
	}
	do
	{
		if (std::string(findData.cFileName).compare(".") == 0 ||
			std::string(findData.cFileName).compare("..") == 0)continue;
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			//如果是目录
			std::string strDir1 = p_strDir + "\\";
			strDir1.append(std::string(findData.cFileName).c_str());
			GetAllFiles(strDir1, p_vecFiles);
		}
		else
		{
			p_vecFiles.push_back(p_strDir + "\\" + std::string(findData.cFileName));
		}
	} while (FindNextFile(hFind, &findData));
	FindClose(hFind);
#else
	DIR* dir = NULL;
	dir = opendir(p_strDir.c_str());
	if (NULL == dir) {
		return -1;
	}

	struct dirent* dp = NULL;
	char szBasePath[NSDK_MAX_PATH] = { 0 };        //基目录
	while ((dp = readdir(dir)) != nullptr) {
		if (0 == strcmp(dp->d_name, ".") && 0 == strcmp(dp->d_name, ".."))
		{
			continue;
		}
		else if (8 == dp->d_type)
		{
			memset(szBasePath, 0, NSDK_MAX_PATH);
			strcpy(szBasePath, p_strDir.c_str());
			strcat(szBasePath, "/");
			strcat(szBasePath, dp->d_name);
			p_vecFiles.push_back(szBasePath);
		}
		else if (10 == dp->d_type || 4 == dp->d_type)//链接文件 //dir 是目录则递归调用
		{
			memset(szBasePath, '\0', NSDK_MAX_PATH);
			strcpy(szBasePath, p_strDir.c_str());
			strcat(szBasePath, "/");
			strcat(szBasePath, dp->d_name);
			GetAllFiles(szBasePath, p_vecFiles);
		}
	}
	closedir(dir);
#endif
	return 0;
}

bool CFolder::DeleteDirectory(const char* p_szFilePath, const char* p_szExpath)
{
#if defined( OS_IS_WINDOWS )
	char szFind[NSDK_MAX_PATH] = { 0 };
	WIN32_FIND_DATAA FindFileData;
	strcpy(szFind, p_szFilePath);
	strcat(szFind, "\\*.*");
	HANDLE hFind = ::FindFirstFileA(szFind, &FindFileData);
	if (INVALID_HANDLE_VALUE == hFind)
		return false;

	while (TRUE)
	{
		if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (FindFileData.cFileName[0] != '.' && strncmp(FindFileData.cFileName, "..", 2) != 0 && strcmp(FindFileData.cFileName, p_szExpath) != 0)
			{
				char szFile[NSDK_MAX_PATH] = { 0 };
				_snprintf(szFile, sizeof(szFile) - 1, "%s\\%s", p_szFilePath, FindFileData.cFileName);
				DeleteDirectory(szFile);
			}
		}
		else
		{
			char szFile[NSDK_MAX_PATH] = { 0 };
			_snprintf(szFile, sizeof(szFile) - 1, "%s\\%s", p_szFilePath, FindFileData.cFileName);
			DeleteFileA(szFile);
		}
		if (!FindNextFileA(hFind, &FindFileData))    break;
	}
	FindClose(hFind);
	RemoveDirectoryA(p_szFilePath);
#else
	std::vector<std::string> vecFiles;
	GetAllFiles(p_szFilePath, vecFiles);
	for (auto it_path = vecFiles.begin(); it_path != vecFiles.end(); it_path++)
	{
		std::string strCmd = "rm -f " + *it_path;
		system(strCmd.c_str());
	}
#endif
	return true;
}

/******************** 文件处理 ********************/
int CFile::FileExists(const char* p_szFilePath)
{
	if (p_szFilePath == nullptr || strlen(p_szFilePath) == 0)
		return -1;

	struct stat st;
	if (stat(p_szFilePath, &st) != 0)
		return -2;

	return st.st_mode & S_IFREG ? 0 : -3;
}

int CFile::GetFileAttr(const char* p_szFilePath, int p_iMode)
{
	if (p_szFilePath == nullptr || strlen(p_szFilePath) == 0)
		return -1;

	if (p_iMode != F_OK && p_iMode != X_OK && p_iMode != W_OK &&
		p_iMode != R_OK && p_iMode != RW_OK)
		return -2;//属性不支持
#if defined( OS_IS_WINDOWS )
	if (_access(p_szFilePath, p_iMode) == 0)
		return 0;
#else
	if (access(p_szFilePath, p_iMode) == 0)
		return 0;
#endif
	return -3;//对应属性不存在
}

unsigned long CFile::FileLength(FILE* p_pFile)
{
	if (NULL == p_pFile)
		return 0L;

	unsigned long ulOldPos = 0, ulFileLen = 0;
	ulOldPos = ftell(p_pFile);
	fseek(p_pFile, 0L, SEEK_END);
	ulFileLen = ftell(p_pFile);
	fseek(p_pFile, ulOldPos, SEEK_SET);
	return ulFileLen;
}

/******************** 字符串处理 ********************/
int unsigned CString::StringSplit(const std::string p_strSrc, const std::string p_strSep, std::vector<std::string>& p_vecObj)
{
	std::string::size_type begin, end;
	p_vecObj.clear();
	if (p_strSrc.length() == 0)
	{
		return 0;
	}

	begin = 0;
	while (1)
	{
		//end = str.find_first_of( sep, begin );
		end = p_strSrc.find(p_strSep, begin);
		if (end == std::string::npos)
		{
			break;
		}
		p_vecObj.push_back(p_strSrc.substr(begin, end - begin));
		begin = end + p_strSep.length();
	}
	p_vecObj.push_back(p_strSrc.substr(begin));

	return p_vecObj.size();
}

/******************** 日期时间处理 ********************/
unsigned long CDateTime::GetNextDate(unsigned long p_ulDate, unsigned int p_uiDays)
{
	unsigned long lDate= 0;
#if defined(OS_IS_WINDOWS )
	CTime cBack(p_ulDate / 10000, p_ulDate % 10000 / 100, p_ulDate % 100, 0, 0, 0);
	cBack += CTimeSpan(p_uiDays, 0, 0, 0);
	lDate = cBack.GetYear() * 10000 + cBack.GetMonth() * 100 + cBack.GetDay();
#else
	tm now_time;
	now_time.tm_year = (p_ulDate / 10000) - 1900;
	now_time.tm_mon = (p_ulDate % 10000 / 100) - 1;
	now_time.tm_wday = p_ulDate % 100 + p_uiDays;
	now_time.tm_hour = now_time.tm_min = now_time.tm_sec = 0;
	mktime(&now_time);
	lDate = (now_time.tm_year + 1900) * 10000 + (now_time.tm_mon + 1) * 100 + now_time.tm_wday;
#endif
	return lDate;
}

unsigned int CDateTime::GetCurDate(bool p_bDate)
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

int CDateTime::GetCurDateTime(char *p_pDateTime, int p_iBufLen)
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

void CDateTime::GetLocalDateTime(int &p_iDate, int &p_iTime)
{
#if defined( OS_IS_WINDOWS )
	// 时间
	SYSTEMTIME stCurrTime = { 0 };
	GetLocalTime(&stCurrTime);

	p_iDate = stCurrTime.wYear * 10000 + stCurrTime.wMonth * 100 + stCurrTime.wDay;
	p_iTime = (stCurrTime.wHour * 10000 + stCurrTime.wMinute * 100 + stCurrTime.wSecond) * 1000 + stCurrTime.wMilliseconds;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	std::tm* tm_ptr = localtime(&tv.tv_sec);

	p_iDate = (tm_ptr->tm_year + 1900) * 10000 + (tm_ptr->tm_mon + 1) * 100 + tm_ptr->tm_mday;
	p_iTime = (tm_ptr->tm_hour * 10000 + tm_ptr->tm_min * 100 + tm_ptr->tm_sec) * 1000 + tv.tv_usec;
#endif
}

std::string CDateTime::GetTimes(const std::string& p_strFmt)
{
	time_t tmt = time(NULL);
	struct tm *pTime = NULL;
	char szTimeBuf[NSDK_MAX_PATH] = { 0 };
	memset(szTimeBuf, 0, NSDK_MAX_PATH);
	std::string strTime = "";

	pTime = localtime(&tmt);
	strftime(szTimeBuf, NSDK_MAX_PATH, p_strFmt.c_str(), pTime);
	strTime = szTimeBuf;
	return strTime;
}

unsigned long CDateTime::GetFriday(unsigned long p_ulDate) // 得到某日的星期五
{
	const static char aDaysOfMon[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	short i = 0, iYear = 0, iMon = 0, iDay = 0;
	unsigned long ulDays = 0;

	iYear = p_ulDate / 10000;
	iMon = (p_ulDate % 10000) / 100;
	iDay = (p_ulDate % 10000) % 100;

	for (i = 1980; i < iYear; i++)
	{
		ulDays += 365;
		if (i % 400 == 0 || (i % 4 == 0 && i % 100 != 0)) // 闰年
		{
			ulDays++;
		}
	}

	for (i = 1; i < iMon; i++)
	{
		ulDays += aDaysOfMon[i - 1];
		if (i == 2 && (iYear % 400 == 0 || (iYear % 4 == 0 && iYear % 100 != 0)))
		{
			ulDays++;
		}
	}

	ulDays += iDay - 1; // 从1980年至当日共有多少天
	ulDays -= 3; // 1980年1月1日为星期二，则离星期五的天数还要减去3
	i = ulDays % 7;

	if (i > 2) // 得到当日所在星期的星期五。若为星期五、六、日，则为当日
	{
		iDay += (7 - i) % 7;
	}

	i = aDaysOfMon[iMon - 1];
	if (iMon == 2 && (iYear % 400 == 0 || (iYear % 4 == 0 && iYear % 100 != 0)))
	{
		i++;
	}
	if (iDay > i)
	{
		iDay -= i;
		iMon++;
	}
	if (iMon > 12)
	{
		iMon = 1;
		iYear++;
	}

	return (long)iYear * 10000L + (long)iMon * 100L + (long)iDay;
}

/******************** 数字处理 ********************/
bool CDigit::IsEquals(double p_dData1, double p_dData2, int p_iXsFlag)
{
	if ((p_dData1 >= 0.0f && p_dData2 < 0.0f) || (p_dData1 <= 0.0f && p_dData2 > 0.0f))
		return false;
	return fabs(p_dData1 - p_dData2) < pow(.1, p_iXsFlag);
}

END_NAMESPACE_NSDK

