#include "nsdk.h"
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <chrono>
#include <vector>
#include <thread>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <time.h>
#include <assert.h>
#include <math.h>

#if defined( OS_IS_WINDOWS )
#include <io.h>
#include <atltime.h>
#else
#include <unistd.h>
#include <ctime>
#include <chrono>
#include <sys/time.h>
#include <iostream>
#include <dirent.h>
#include <stdarg.h>
#include <errno.h>
#include <arpa/inet.h>
#include <iconv.h>
#include <dirent.h>
#endif

#include "nsdk_atomic.h"
#include "aes.h"
#if defined(OS_IS_WINDOWS) && defined(NSDK_ENABLE_BREAKPAD)
#include "exception_handler.h"
#endif

//内部使用的宏
#define NSDK_MAX_PATH 260
#define COMPPREC		0.001 // Round使用

//加密部分
//注意编译给客户端时需要把 Aes Deaes Base64Encode Base64Decode 去掉
//#define SERVER_AUTH_FLAG //服务端认证宏 客户端授权控制
#define AUTH_DAY	(0)//授权天数 0当天有效

std::string g_strAuthKey = "tjzt!@#$%^&*()_+<>?;";
std::string g_strAuthIV = "gfdertfghjkuyrtg";   //ECB MODE不需要关心chain，可以填空
std::string g_strSignature = "TJZT";//签名

static const std::string g_strBase64Chars =
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz"
"0123456789+/";

BGN_NAMESPACE_NSDK

#define NSDK_BREAKPAD_DUMP_FOLDER_NAME "CrashDumps"
#define NSDK_BREAKPAD_CRASH_MARKER_FILE_NAME L"last_crash.txt"

namespace
{
	// 按固定8位日期字段解析授权日期，避免substr产生临时字符串。
	int ParseAuthDate8(const char* p_szDate)
	{
		if (p_szDate == nullptr)
		{
			return 0;
		}

		char szDate[9] = { 0 };
		memcpy(szDate, p_szDate, 8);
		return atoi(szDate);
	}

#if defined(OS_IS_WINDOWS) && defined(NSDK_ENABLE_BREAKPAD)
	google_breakpad::ExceptionHandler* g_pBreakpad = nullptr;

	std::string WideToAsciiText(const wchar_t* p_wszText)
	{
		std::string strText;
		if (p_wszText == nullptr)
			return strText;

		while (*p_wszText != 0)
		{
			wchar_t wchValue = *p_wszText++;
			strText += (wchValue >= 0 && wchValue <= 0x7F) ? static_cast<char>(wchValue) : '?';
		}

		return strText;
	}

	bool DumpCallback(const wchar_t* p_wszDumpPath,
		const wchar_t* p_wszDumpId,
		void* p_pContext,
		EXCEPTION_POINTERS* p_pExInfo,
		MDRawAssertionInfo* p_pAssertion,
		bool p_bSucceeded)
	{
		(void)p_pContext;
		(void)p_pAssertion;

		// wyl 2026-05-07：崩溃现场只写极简标记，避免调用业务日志、弹窗、网络等复杂逻辑导致二次异常。
		if (p_wszDumpPath != nullptr && p_wszDumpPath[0] != L'\0')
		{
			std::wstring strMarkerPath = p_wszDumpPath;
			if (!strMarkerPath.empty() && strMarkerPath[strMarkerPath.length() - 1] != L'\\' && strMarkerPath[strMarkerPath.length() - 1] != L'/')
				strMarkerPath += L"\\";
			strMarkerPath += NSDK_BREAKPAD_CRASH_MARKER_FILE_NAME;

			std::wofstream ofsCrashFile(strMarkerPath.c_str(), std::ios::out | std::ios::app);
			if (ofsCrashFile.is_open())
			{
				auto tpNow = std::chrono::system_clock::now();
				std::time_t tNow = std::chrono::system_clock::to_time_t(tpNow);
				std::tm tmNow = SafeLocalTime(tNow);
				auto nMillisecond = std::chrono::duration_cast<std::chrono::milliseconds>(tpNow.time_since_epoch()).count() % 1000;

				unsigned long dwExceptionCode = 0;
				void* pExceptionAddress = nullptr;
				if (p_pExInfo != nullptr && p_pExInfo->ExceptionRecord != nullptr)
				{
					dwExceptionCode = p_pExInfo->ExceptionRecord->ExceptionCode;
					pExceptionAddress = p_pExInfo->ExceptionRecord->ExceptionAddress;
				}

				// wyl 2026-05-07：last_crash.txt 每行字段说明：时间、dump生成结果、当前线程ID、异常码、异常地址、dump_id。
				// succeeded=1 表示 Breakpad 写 dump 成功；exception 为异常码；address 为异常发生地址；dump_id 对应生成的 dmp 文件名。
				std::ostringstream ossCrashLine;
				ossCrashLine << "[" << std::put_time(&tmNow, "%Y-%m-%d %H:%M:%S")
					<< "." << std::setw(3) << std::setfill('0') << nMillisecond << std::setfill(' ')
					<< "] succeeded=" << (p_bSucceeded ? 1 : 0)
					<< " tid=" << std::this_thread::get_id()
					<< " exception=0x" << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << dwExceptionCode
					<< std::nouppercase << std::dec << std::setfill(' ')
					<< " address=" << pExceptionAddress
					<< " dump_id=" << WideToAsciiText(p_wszDumpId)
					<< "\n";
				ofsCrashFile << ossCrashLine.str().c_str();
			}
		}

		return p_bSucceeded;
	}
#endif
}

/******************** 文件夹处理 ********************/
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
				if (mkdir(curPath.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH) == -1)
				{
					return -1;
				}
			}
		}
	}
#endif

	return 0;
}

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
		if (pos && last != p_strPath.npos)
		{
			p_strPath.erase(last, pos - last + 3);
		}
	}
#else
	TranslatePath(p_strPath);
	/*
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
	*/
#endif
	return true;
}

const char* GetRootPath(void)
{
	static char s_szMoundlePath[NSDK_MAX_PATH] = { 0 };
#if defined( OS_IS_WINDOWS )
	GetCurrentDirectoryA(NSDK_MAX_PATH, s_szMoundlePath);
#else
	getcwd(s_szMoundlePath, NSDK_MAX_PATH);
#endif
	return s_szMoundlePath;
}

int InitBreakpad(const char* p_szDumpPath)
{
#if defined(OS_IS_WINDOWS) && defined(NSDK_ENABLE_BREAKPAD)
	if (g_pBreakpad != nullptr)
		return NSDK_OK;

	std::string strDumpPath;
	if (p_szDumpPath != nullptr && p_szDumpPath[0] != '\0')
	{
		strDumpPath = p_szDumpPath;
	}
	else
	{
		strDumpPath = GetRootPath();
		if (!strDumpPath.empty() && strDumpPath.compare(strDumpPath.length() - 1, 1, NSDK_PATH_DELIMETER) != 0)
			strDumpPath += NSDK_PATH_DELIMETER;
		strDumpPath += NSDK_BREAKPAD_DUMP_FOLDER_NAME;
	}

	RegularPath(strDumpPath);
	if (FolderExists(strDumpPath.c_str()) != NSDK_OK)
	{
		if (CreateFolder(strDumpPath.c_str()) != NSDK_OK)
			return NSDK_KO;
	}

	int iWideLen = MultiByteToWideChar(CP_ACP, 0, strDumpPath.c_str(), -1, NULL, 0);
	if (iWideLen <= 0)
		return NSDK_KO;

	std::vector<wchar_t> vecDumpPath(iWideLen);
	if (MultiByteToWideChar(CP_ACP, 0, strDumpPath.c_str(), -1, &vecDumpPath[0], iWideLen) <= 0)
		return NSDK_KO;

	std::wstring strDumpPathW(&vecDumpPath[0]);
	g_pBreakpad = new google_breakpad::ExceptionHandler(
		strDumpPathW,
		nullptr,
		DumpCallback,
		nullptr,
		google_breakpad::ExceptionHandler::HANDLER_ALL
	);

	return g_pBreakpad != nullptr ? NSDK_OK : NSDK_KO;
#else
	(void)p_szDumpPath;
	return NSDK_KO;
#endif
}

int GetAllFiles(std::string p_strDir, std::vector<std::string>& p_vecFiles)
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
		if (0 == strncmp(dp->d_name, ".", 1) || 0 == strcmp(dp->d_name, ".."))
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

int GetSubDirs(std::string p_strDir, std::vector<std::string>& p_vecFiles)
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
			p_vecFiles.push_back(p_strDir + "\\" + std::string(findData.cFileName));
		}
		//else continue;

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
		else if (10 == dp->d_type || 4 == dp->d_type)//链接文件 //dir 是目录则递归调用
		{
			memset(szBasePath, '\0', NSDK_MAX_PATH);
			strcpy(szBasePath, p_strDir.c_str());
			strcat(szBasePath, "/");
			strcat(szBasePath, dp->d_name);
			p_vecFiles.push_back(szBasePath);
		}
	}
	closedir(dir);
#endif
	return 0;
}

bool DeleteDirectory(const char* p_szFilePath, const char* p_szExpath)
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
	DIR* dir = opendir(p_szFilePath);
	if (dir == nullptr)
	{
		// 打开目录失败
		return false;
	}

	dirent* entry = nullptr;
	while ((entry = readdir(dir)) != nullptr) {
		if (strncmp(entry->d_name, ".", 1) == 0 || strcmp(entry->d_name, "..") == 0)
		{
			// 跳过当前目录和父目录
			continue;
		}

		if (entry->d_type == DT_DIR)
		{
			// 如果是文件夹，递归删除
			if (strcmp(entry->d_name, p_szExpath) != 0)
			{
				char szFile[NSDK_MAX_PATH] = { 0 };
				snprintf(szFile, sizeof(szFile) - 1, "%s/%s", p_szFilePath, entry->d_name);
				if (!DeleteDirectory(szFile))
				{
					closedir(dir);
					return false;
				}
			}
		}
		else
		{
			// 如果是文件，直接删除
			char szFile[NSDK_MAX_PATH] = { 0 };
			snprintf(szFile, sizeof(szFile) - 1, "%s/%s", p_szFilePath, entry->d_name);
			if (remove(szFile) != 0)
			{
				closedir(dir);
				return false;
			}
		}
	}

	// 关闭目录流
	closedir(dir);

	// 删除当前目录
	rmdir(p_szFilePath);//最后一层路径不删除,只删除非p_szExpath的目录

#endif
	return true;
}

/******************** 文件处理 ********************/
int FileExists(const char* p_szFilePath)
{
	if (p_szFilePath == nullptr || strlen(p_szFilePath) == 0)
		return -1;

	struct stat st;
	if (stat(p_szFilePath, &st) != 0)
		return -2;

	return st.st_mode & S_IFREG ? 0 : -3;
}

int GetFileAttr(const char* p_szFilePath, int p_iMode)
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

unsigned long FileLength(FILE* p_pFile)
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

FILE* CreateAppendFile(const char* p_pszFile)
{
	FILE* fp = fopen(p_pszFile, "r+b");

	if (!fp)
	{
		fp = fopen(p_pszFile, "wb");
		if (!fp)
		{
			return nullptr;
		}
	}

	return fp;
}

/******************** 字符串处理 ********************/
int unsigned StringSplit(const std::string p_strSrc, const std::string p_strSep, std::vector<std::string>& p_vecObj)
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

int UTF82ASC(const char* p_szSrcbuf, char* p_szOutbuf, int p_iOutlen)
{
#if defined( OS_IS_WINDOWS )
	if (NULL == p_szSrcbuf || NULL == p_szOutbuf || 0 >= strlen(p_szSrcbuf) || 0 >= p_iOutlen)
		return -1;

	INT32 len = MultiByteToWideChar(CP_UTF8, 0, p_szSrcbuf, -1, NULL, NULL);
	if (len <= 0)
		return -2;

	unsigned short* wszASC = new unsigned short[len + 1];
	memset(wszASC, 0, len * 2 + 2);
	MultiByteToWideChar(CP_UTF8, 0, p_szSrcbuf, -1, (wchar_t*)wszASC, len);

	len = WideCharToMultiByte(CP_ACP, 0, (wchar_t*)wszASC, -1, NULL, 0, NULL, NULL);
	char* szASC = new char[len + 1];
	memset(szASC, 0, len + 1);
	WideCharToMultiByte(CP_ACP, 0, (wchar_t*)wszASC, -1, szASC, len, NULL, NULL);

	strncpy(p_szOutbuf, szASC, p_iOutlen - 1);

	delete[] wszASC;
	delete[] szASC;
#else
	if (NULL == p_szSrcbuf || NULL == p_szOutbuf || 0 >= strlen(p_szSrcbuf) || 0 >= p_iOutlen)
		return -1;

	iconv_t cd;
	char* pSrc = (char*)p_szSrcbuf;
	char** pin = &(pSrc);

	size_t iLenin = strlen(p_szSrcbuf);
	size_t iLenout = p_iOutlen;

	char szOut[255] = { 0 };
	char* pTmp = &(szOut[0]);
	char** pout = &(pTmp);

	//
	cd = iconv_open("GBK", "UTF-8");
	if (cd == 0)
	{
		//printf("iconv_open() ERR\n");
		return -2;
	}

	if (iconv(cd, pin, &iLenin, pout, &iLenout) == -1)
	{
		//printf("iconv() ERR\n");
		iconv_close(cd);
		switch (errno)
		{
		case  E2BIG:
			return -3;
		case  EILSEQ:
			return -4;
		case  EINVAL:
			return -5;
		default:
			return -8;
		}
	}
	iLenout = nsdk_min(iLenout, p_iOutlen);
	strncpy(p_szOutbuf, szOut, iLenout);

	iconv_close(cd);
#endif
	return 0;
}

void TrimCharArraySelf(char* p_pzStr)
{
	char* src = p_pzStr;
	char* end;
	//去除开头空白字符
	while (isspace(*p_pzStr))
	{
		p_pzStr++;
	}
	//如果字符串全是空白，则直接返回空字符串
	if (*p_pzStr == '\0')
	{
		strcpy(src, p_pzStr);
		return;
	}
	//去除字符串结尾的空白字符
	end = p_pzStr + strlen(p_pzStr) - 1;
	while (end > p_pzStr && isspace(*end))
	{
		end--;
	}

	//字符串结尾添加结束符
	*(end + 1) = '\0';
	strcpy(src, p_pzStr);
}

void TrimCharArray(std::string& p_refStr, const char* p_pzStr, int p_iLen)
{
	p_refStr = std::string(p_pzStr);
	TrimStr(p_refStr);
}

void TrimStr(std::string& p_refStr)
{
	int s = p_refStr.find_first_not_of(" ");
	int e = p_refStr.find_last_not_of(" ");

	//
	if (s >= 0 && e >= 0 && e >= s)
	{
		p_refStr = p_refStr.substr(s, e - s + 1);
	}
}

void TrimStrByChar(std::string& p_refStr, char p_ch)
{
	//去除字符串前后的字符c
	int s = p_refStr.find_first_not_of(p_ch);
	int e = p_refStr.find_last_not_of(p_ch);

	if (s >= 0 && e >= 0 && e >= s)
	{
		p_refStr = p_refStr.substr(s, e - s + 1);
	}
	if (s == -1 && e == -1)
	{
		p_refStr = "";
	}
}

std::string FormatString(const char* p_szFormat, ...)
{
	char szBuffer[260]; // 最大路径长度
	va_list args;
	va_start(args, p_szFormat);
	vsprintf(szBuffer, p_szFormat, args); //TODO 动态参数比format少时会导致程序报错
	va_end(args);
	std::string strRtn(szBuffer);
	return strRtn;
}

std::string Add2String(const std::string& p_strSrc, int p_iData)
{
	std::ostringstream oss;
	oss << p_strSrc;
	oss << p_iData;
	return oss.str();
}

char* CharArrayLower(char* p_pszStr)
{
	int i, len = strlen(p_pszStr);
	for (i = 0; i < len; i++)
		p_pszStr[i] = tolower(p_pszStr[i]);
	return p_pszStr;
}
char* CharArrayUpper(char* p_pszStr)
{
	int i, len = strlen(p_pszStr);
	for (i = 0; i < len; i++)
		p_pszStr[i] = toupper(p_pszStr[i]);
	return p_pszStr;
}

const char* StringLower(std::string& p_strSrc)
{
	int i, len = strlen(p_strSrc.c_str());
	for (i = 0; i < len; i++)
		p_strSrc[i] = tolower(p_strSrc[i]);
	return p_strSrc.c_str();
}

const char* StringUpper(std::string& p_strSrc)
{
	int i, len = strlen(p_strSrc.c_str());
	for (i = 0; i < len; i++)
		p_strSrc[i] = toupper(p_strSrc[i]);
	return p_strSrc.c_str();
}

void SafeCopyCString(char* p_szDst, size_t p_dwDstLen, const char* p_szSrc)
{
	if (nullptr == p_szDst || 0 == p_dwDstLen)
		return;

	p_szDst[0] = '\0';
	if (nullptr == p_szSrc)
		return;

	size_t dwCopyLen = strlen(p_szSrc);
	if (dwCopyLen >= p_dwDstLen)
		dwCopyLen = p_dwDstLen - 1;

	if (dwCopyLen > 0)
	{
		memcpy(p_szDst, p_szSrc, dwCopyLen);
	}
	p_szDst[dwCopyLen] = '\0';
}

/******************** 日期时间处理 ********************/

// 函数用于将指定日期按照偏移量进行偏移计算
std::tm OffsetDays(std::tm& p_tmSpecified_date, int p_iOffset)
{
	// 对日期进行偏移
	p_tmSpecified_date.tm_mday += p_iOffset;
	// 规范化日期，确保日期的合理性
	mktime(&p_tmSpecified_date);
	return p_tmSpecified_date;
}

unsigned long GetNextDate(unsigned long p_ulDate, unsigned int p_uiDays)
{
	unsigned long lDate = 0;
#if defined(OS_IS_WINDOWS )
	CTime cBack(p_ulDate / 10000, p_ulDate % 10000 / 100, p_ulDate % 100, 0, 0, 0);
	cBack += CTimeSpan(p_uiDays, 0, 0, 0);
	lDate = cBack.GetYear() * 10000 + cBack.GetMonth() * 100 + cBack.GetDay();

#else
	// 设置偏移后日期
	tm tmSpecifiedDate;
	tmSpecifiedDate.tm_year = (p_ulDate / 10000) - 1900;
	tmSpecifiedDate.tm_mon = (p_ulDate % 10000 / 100) - 1;
	tmSpecifiedDate.tm_mday = p_ulDate % 100;
	tmSpecifiedDate.tm_hour = tmSpecifiedDate.tm_min = tmSpecifiedDate.tm_sec = 0;

	// 格式化本地日期
	time_t tmSpecifiedTime = mktime(&tmSpecifiedDate);
	tm* ptmSpecifiedDate = localtime(&tmSpecifiedTime);

	tm tmNewdDate = OffsetDays(*ptmSpecifiedDate, p_uiDays);
	lDate = (tmNewdDate.tm_year + 1900) * 10000 + (tmNewdDate.tm_mon + 1) * 100 + tmNewdDate.tm_mday;
#endif
	return lDate;
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

int GetCurDateTime(char* p_pDateTime, int p_iBufLen, int p_iType)
{
	if (NULL == p_pDateTime || p_iBufLen <= 0)
		return -1;

	static char s_szDateTime[NSDK_MAX_PATH] = { 0 };
#if defined( OS_IS_WINDOWS )
	// 时间
	SYSTEMTIME stCurrTime = { 0 };
	GetLocalTime(&stCurrTime);

	if (0 == p_iType)
	{
		_snprintf(s_szDateTime, NSDK_MAX_PATH - 1, "%04d%02d%02d %02d:%02d %02d:%03d",
			stCurrTime.wYear, stCurrTime.wMonth, stCurrTime.wDay, stCurrTime.wHour,
			stCurrTime.wMinute, stCurrTime.wSecond, stCurrTime.wMilliseconds);
	}
	else
	{
		_snprintf(s_szDateTime, NSDK_MAX_PATH - 1, "%04d-%02d-%02d %02d:%02d:%02d",
			stCurrTime.wYear, stCurrTime.wMonth, stCurrTime.wDay, stCurrTime.wHour,
			stCurrTime.wMinute, stCurrTime.wSecond);
	}
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	std::tm* tm_ptr = localtime(&tv.tv_sec);

	if (0 == p_iType)
	{
		snprintf(s_szDateTime, NSDK_MAX_PATH - 1, "%04d%02d%02d %02d:%02d %02d:%03ld",
			tm_ptr->tm_year + 1900, tm_ptr->tm_mon + 1, tm_ptr->tm_mday, tm_ptr->tm_hour,
			tm_ptr->tm_min, tm_ptr->tm_sec, tv.tv_usec);
	}
	else
	{
		snprintf(s_szDateTime, NSDK_MAX_PATH - 1, "%04d-%02d-%02d %02d:%02d:%02d",
			tm_ptr->tm_year + 1900, tm_ptr->tm_mon + 1, tm_ptr->tm_mday, tm_ptr->tm_hour,
			tm_ptr->tm_min, tm_ptr->tm_sec);
	}
#endif
	int iBufLen = nsdk_min(strlen(s_szDateTime), p_iBufLen);
	memcpy(p_pDateTime, s_szDateTime, iBufLen);
	return 0;
}

void GetLocalDateTime(int& p_iDate, int& p_iTime)
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

long long GetCurrentTimeMillis()
{
	int iDate, iTime;
	GetLocalDateTime(iDate, iTime);
	return (long long)iDate * 1000000000 + (long long)iTime;
}

std::string GetTimes(const std::string& p_strFmt)
{
	time_t tmt = time(NULL);
	struct tm* pTime = NULL;
	char szTimeBuf[NSDK_MAX_PATH] = { 0 };
	memset(szTimeBuf, 0, NSDK_MAX_PATH);
	std::string strTime = "";

	pTime = localtime(&tmt);
	strftime(szTimeBuf, NSDK_MAX_PATH, p_strFmt.c_str(), pTime);
	strTime = szTimeBuf;
	return strTime;
}

unsigned long GetFriday(unsigned long p_ulDate) // 得到某日的星期五
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

bool IsInWeekend(int p_iDate)
{
	std::tm tmDate;
	tmDate.tm_year = (p_iDate / 10000) - 1900;
	tmDate.tm_mon = ((p_iDate % 10000) / 100) - 1;
	tmDate.tm_mday = p_iDate % 100;
	tmDate.tm_hour = 0;
	tmDate.tm_min = 0;
	tmDate.tm_sec = 0;
	// 调用mktime函数来标准化日期结构，这一步很关键，它会自动更新tm_wday等成员
	std::mktime(&tmDate);

	return tmDate.tm_wday == 0 || tmDate.tm_wday == 6;
}

std::tm SafeLocalTime(std::time_t p_ttNow)
{
	std::tm stLocalTime = {};
#if defined(_WIN32)
	localtime_s(&stLocalTime, &p_ttNow);
#else
	localtime_r(&p_ttNow, &stLocalTime);
#endif
	return stLocalTime;
}


/******************** 数字处理 ********************/
bool IsEquals(double p_dData1, double p_dData2, int p_iXsFlag)
{
	if ((p_dData1 >= 0.0f && p_dData2 < 0.0f) || (p_dData1 <= 0.0f && p_dData2 > 0.0f))
		return false;
	return fabs(p_dData1 - p_dData2) < pow(.1, p_iXsFlag);
}

bool IsEqualsZero(double p_dData)
{
	return IsEquals(p_dData, 0, 0);
}

double Round(double p_dData, short p_sPlaces)
{
	double dRetval;
	double dMod = COMPPREC;
	if (p_dData < 0.0) dMod = -dMod;
	dRetval = p_dData;
	dRetval += (5.0 / pow(10.0, p_sPlaces + 1.0));
	dRetval *= pow(10.0, p_sPlaces);
	dRetval = floor(dRetval + dMod);
	dRetval /= pow(10.0, p_sPlaces);
	return dRetval;
}

int Double2Int(double p_dData)
{
	static double _dMagic = 6755399441055744.0;
	p_dData += _dMagic;
	return *(int*)&p_dData;
}

/******************** 操作系统 ********************/

unsigned long GetSysError()
{
#if defined( OS_IS_WINDOWS )
	// 时间
	return GetLastError();
#else
	return errno;
#endif
}

unsigned long GetNumberOfCores(bool p_bUsable)
{
#if defined( OS_IS_WINDOWS )
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	return sysInfo.dwNumberOfProcessors;
#else
	return p_bUsable ? sysconf(_SC_NPROCESSORS_ONLN) : sysconf(_SC_NPROCESSORS_CONF);
	//sysconf(_SC_NPROCESSORS_CONF);//返回系统所有的CPU核数，这个值也包括系统中禁止用户使用的CPU个数
	//sysconf(_SC_NPROCESSORS_ONLN);//返回系统中可用的CPU核数
#endif
}

/******************** 加密认证处理 ********************/
static bool IsBase64(unsigned char p_ch) {
	return (isalnum(p_ch) || (p_ch == '+') || (p_ch == '/'));
}

std::string Base64Encode(unsigned char const* p_ucBytes, unsigned int p_uiLen)
{
	std::string strResult = "";
	unsigned char ucCharArray3[3] = { 0 };
	unsigned char ucCharArray4[4] = { 0 };

	int i = 0, j = 0;

	while (p_uiLen--)
	{
		ucCharArray3[i++] = *(p_ucBytes++);
		if (i == 3)
		{
			ucCharArray4[0] = (ucCharArray3[0] & 0xfc) >> 2;
			ucCharArray4[1] = ((ucCharArray3[0] & 0x03) << 4) + ((ucCharArray3[1] & 0xf0) >> 4);
			ucCharArray4[2] = ((ucCharArray3[1] & 0x0f) << 2) + ((ucCharArray3[2] & 0xc0) >> 6);
			ucCharArray4[3] = ucCharArray3[2] & 0x3f;

			for (i = 0; (i < 4); i++)
			{
				strResult += g_strBase64Chars[ucCharArray4[i]];
			}
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 3; j++)
		{
			ucCharArray3[j] = '\0';
		}

		ucCharArray4[0] = (ucCharArray3[0] & 0xfc) >> 2;
		ucCharArray4[1] = ((ucCharArray3[0] & 0x03) << 4) + ((ucCharArray3[1] & 0xf0) >> 4);
		ucCharArray4[2] = ((ucCharArray3[1] & 0x0f) << 2) + ((ucCharArray3[2] & 0xc0) >> 6);
		ucCharArray4[3] = ucCharArray3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
		{
			strResult += g_strBase64Chars[ucCharArray4[j]];
		}

		while ((i++ < 3))
		{
			strResult += '=';
		}
	}

	return strResult;
}

std::string Base64Decode(std::string const& p_strEncoded)
{
	std::string strResult = "";
	unsigned char ucCharArray3[3] = { 0 };
	unsigned char ucCharArray4[4] = { 0 };

	int i = 0, j = 0, iInPos = 0;
	int iLen = p_strEncoded.size();

	while (iLen-- && (p_strEncoded[iInPos] != '=') && IsBase64(p_strEncoded[iInPos]))
	{
		ucCharArray4[i++] = p_strEncoded[iInPos];
		iInPos++;
		if (i == 4)
		{
			for (i = 0; i < 4; i++)
			{
				ucCharArray4[i] = g_strBase64Chars.find(ucCharArray4[i]);
			}

			ucCharArray3[0] = (ucCharArray4[0] << 2) + ((ucCharArray4[1] & 0x30) >> 4);
			ucCharArray3[1] = ((ucCharArray4[1] & 0xf) << 4) + ((ucCharArray4[2] & 0x3c) >> 2);
			ucCharArray3[2] = ((ucCharArray4[2] & 0x3) << 6) + ucCharArray4[3];

			for (i = 0; (i < 3); i++)
			{
				strResult += ucCharArray3[i];
			}
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 4; j++)
		{
			ucCharArray4[j] = 0;
		}

		for (j = 0; j < 4; j++)
		{
			ucCharArray4[j] = g_strBase64Chars.find(ucCharArray4[j]);
		}

		ucCharArray3[0] = (ucCharArray4[0] << 2) + ((ucCharArray4[1] & 0x30) >> 4);
		ucCharArray3[1] = ((ucCharArray4[1] & 0xf) << 4) + ((ucCharArray4[2] & 0x3c) >> 2);
		ucCharArray3[2] = ((ucCharArray4[2] & 0x3) << 6) + ucCharArray4[3];

		for (j = 0; (j < i - 1); j++)
		{
			strResult += ucCharArray3[j];
		}
	}

	return strResult;
}

std::string Aes(const std::string& p_strSrc) //AES加密
{
	size_t ullLength = p_strSrc.length();
	int iBlockNum = ullLength / BLOCK_SIZE + 1;
	//明文
	char* pszDataIn = new char[iBlockNum * BLOCK_SIZE + 1];
	memset(pszDataIn, 0x00, iBlockNum * BLOCK_SIZE + 1);
	strcpy(pszDataIn, p_strSrc.c_str());

	//进行PKCS7Padding填充。
	int k = ullLength % BLOCK_SIZE;
	int j = ullLength / BLOCK_SIZE;
	int iPadding = BLOCK_SIZE - k;
	for (int i = 0; i < iPadding; i++)
	{
		pszDataIn[j * BLOCK_SIZE + k + i] = iPadding;
	}
	pszDataIn[iBlockNum * BLOCK_SIZE] = '\0';

	//加密后的密文
	char* pszDataOut = new char[iBlockNum * BLOCK_SIZE + 1];
	memset(pszDataOut, 0, iBlockNum * BLOCK_SIZE + 1);

	//进行进行AES的CBC模式加密
	AES Aes;
	Aes.MakeKey(g_strAuthKey.c_str(), g_strAuthIV.c_str(), 16, 16);
	Aes.Encrypt(pszDataIn, pszDataOut, iBlockNum * BLOCK_SIZE, AES::CBC);
	std::string str = Base64Encode((unsigned char*)pszDataOut,
		iBlockNum * BLOCK_SIZE);

	delete[] pszDataIn;
	delete[] pszDataOut;
	return str;
}

std::string Deaes(const std::string& p_strSrc) //AES解密
{
	string strData = Base64Decode(p_strSrc);
	size_t ullLength = strData.length();

	//密文
	char* pszDataIn = new char[ullLength + 1];
	memcpy(pszDataIn, strData.c_str(), ullLength + 1);

	//明文
	char* pszDataOut = new char[ullLength + 1];
	memcpy(pszDataOut, strData.c_str(), ullLength + 1);

	//进行AES的CBC模式解密
	AES Aes;
	Aes.MakeKey(g_strAuthKey.c_str(), g_strAuthIV.c_str(), 16, 16);
	Aes.Decrypt(pszDataIn, pszDataOut, ullLength, AES::CBC);

	//去PKCS7Padding填充
	if (0x00 < pszDataOut[ullLength - 1] <= 0x16)
	{
		int tmp = pszDataOut[ullLength - 1];
		for (unsigned int i = ullLength - 1; i >= ullLength - tmp; i--)
		{
			if (pszDataOut[i] != tmp)
			{
				memset(pszDataOut, 0, ullLength);
				//cout << "去填充失败！解密出错！！" << endl;
				break;
			}
			else
				pszDataOut[i] = 0;
		}
	}
	string strDest(pszDataOut);

	delete[] pszDataIn;
	delete[] pszDataOut;

	return strDest;
}

std::string MakeFeatrue(const std::string p_strClientInfo)
{
	//p_strClientInfo：客户信息，例如：userId
	//返回 客户信息密文。生成方式：公钥+TJZT+开始日期 --> AES加密 --> BASE64加密
	string strClientInfo = "";//明文
	strClientInfo.append(p_strClientInfo);
	strClientInfo.append(g_strAuthKey);
	strClientInfo.append(g_strSignature);
	strClientInfo.append(std::to_string(GetCurDate()));

	return Aes(strClientInfo);
}

#ifndef SERVER_AUTH_FLAG
int BuildFeatrueByPlainClientInfo(const std::string& p_strClientInfo, int p_iAuthDay, std::string& p_strFeatrue)
{
	// 服务端已经拿到明文客户信息时，直接构造最终授权明文，减少一次AES加密和一次AES解密。
	p_strFeatrue.clear();
	if (p_strClientInfo.empty())
	{
		return -1;
	}

	const int iDate = GetCurDate();
	int iAuthDay = p_iAuthDay;
	iAuthDay = nsdk_min(iAuthDay, 30);
	iAuthDay = nsdk_max(iAuthDay, -30);
	const int iNextDate = GetNextDate(iDate, iAuthDay);

	std::string strNewFeatrue = p_strClientInfo;
	strNewFeatrue.append(g_strAuthKey);
	strNewFeatrue.append(g_strSignature);
	strNewFeatrue.append(std::to_string(iDate));
	strNewFeatrue.append(std::to_string(iNextDate));
	p_strFeatrue = Aes(strNewFeatrue);
	return p_strFeatrue.empty() ? -2 : 0;
}

int AuthFeatrueByPlainClientInfo(const std::string& p_strClientInfo, const std::string& p_strFeatrue, std::string& p_strFeatrueInfo)
{
	// 返回码保持与AuthFeatrue一致，仅减少校验过程中的临时字符串。
	if (p_strFeatrue.empty())
	{
		return -1;
	}
	p_strFeatrueInfo = Deaes(p_strFeatrue);
	if (p_strFeatrueInfo.empty())
	{
		return -2;
	}

	const size_t szClientInfoLen = p_strClientInfo.length();
	const size_t szAuthKeyLength = g_strAuthKey.length();
	const size_t szSignatureLength = g_strSignature.length();
	const size_t szDateLength = 8 + 8;

	if (p_strFeatrueInfo.length() != szClientInfoLen + szAuthKeyLength + szSignatureLength + szDateLength)
	{
		return -3;
	}

	if (p_strFeatrueInfo.compare(0, szClientInfoLen, p_strClientInfo) != 0)
	{
		return -4;
	}

	const size_t szAuthKeyOffset = szClientInfoLen;
	if (p_strFeatrueInfo.compare(szAuthKeyOffset, szAuthKeyLength, g_strAuthKey) != 0)
	{
		return -5;
	}

	const size_t szSignatureOffset = szAuthKeyOffset + szAuthKeyLength;
	if (p_strFeatrueInfo.compare(szSignatureOffset, szSignatureLength, g_strSignature) != 0)
	{
		return -6;
	}

	const size_t szDateOffset = szSignatureOffset + szSignatureLength;
	const char* p_szDate = p_strFeatrueInfo.c_str() + szDateOffset;
	const int iStartDate = ParseAuthDate8(p_szDate);
	const int iEndDate = ParseAuthDate8(p_szDate + 8);
	const int iCurDate = GetCurDate();

	if (iCurDate > iEndDate)
	{
		return -7;
	}

	if (iStartDate > iEndDate)
	{
		return -8;
	}

	const int iNextDate = GetNextDate(iStartDate, AUTH_DAY);
	if (iNextDate != iEndDate)
	{
		return -9;
	}

	return 0;
}

int BuildFeatrue(std::string p_strClientInfo, unsigned int p_uiClientInfoLen, int p_iAuthDay, std::string& p_strFeatrue)
{
	//p_iAuthDay 授权天数
	//p_usClientInfoLen 客户信息长度，例如：userId，长度是6
	//p_strClientInfo 客户信息密文。生成方式：公钥+TJZT+开始日期 --> AES加密 --> BASE64加密
	//p_strFeatrue 服务器授权密文。生成方式：公钥+TJZT+开始日期+结束日期 --> AES加密 --> BASE64加密

	//1、解密：BASE64解密 --> AES解密 --> 公钥+TJZT+开始日期
	if (p_strClientInfo.empty())
	{
		//长度是0
		return -1;
	}

	std::string strAesData = Deaes(p_strClientInfo);
	if (strAesData.empty())
	{
		//AES解密失败
		return -2;
	}

	//2、校验：公钥+TJZT
	unsigned int uiClientInfoLen = p_uiClientInfoLen;
	int iAuthKeyLength = g_strAuthKey.length();
	int iSignatureLength = g_strSignature.length();
	int iDateLength = 8;//年月日 20241219

	if (strAesData.length() != uiClientInfoLen + iAuthKeyLength + iSignatureLength + iDateLength)
	{
		//长度匹配失败 公钥+TJZT+开始日期
		return -3;
	}

	std::string strAuthKey = strAesData.substr(uiClientInfoLen, iAuthKeyLength);
	if (strAuthKey != g_strAuthKey)
	{
		//公钥匹配失败
		return -4;
	}

	std::string strSignature = strAesData.substr(uiClientInfoLen + iAuthKeyLength, iSignatureLength);
	if (strSignature != g_strSignature)
	{
		//签名匹配失败
		return -5;
	}

	//3、授权：服务器授权密文
	std::string strDate = strAesData.substr(uiClientInfoLen +iAuthKeyLength + iSignatureLength, iDateLength);
	int iDate = atoi(strDate.c_str());
	int iCurDate = GetCurDate();
	int iAuthDay = p_iAuthDay;
	//授权天数在-30到30天之间
	iAuthDay = nsdk_min(iAuthDay, 30);
	iAuthDay = nsdk_max(iAuthDay, -30);
	int iNextDate = GetNextDate(iDate, iAuthDay);
	std::string strNewFeatrue = strAesData + std::to_string(iNextDate);
	p_strFeatrue = Aes(strNewFeatrue);

	return 0;
}

int AuthFeatrue(const std::string p_strClientInfo, std::string p_strFeatrue, std::string& p_strFeatrueInfo)
{
	//p_strFeatrue 服务器授权密文。生成方式：公钥+TJZT+开始日期+结束日期 --> AES加密 --> BASE64加密
	//p_strFeatrueInfo 解密明文

	//1、解密：BASE64解密 --> AES解密 --> 公钥+TJZT+开始日期+结束日期
	if (p_strFeatrue.empty())
	{
		//长度是0
		return -1;
	}
	p_strFeatrueInfo = Deaes(p_strFeatrue);
	if (p_strFeatrueInfo.empty())
	{
		//AES解密失败
		return -2;
	}

	//2、校验：公钥+TJZT
	unsigned int uiClientInfoLen = p_strClientInfo.length();
	int iAuthKeyLength = g_strAuthKey.length();
	int iSignatureLength = g_strSignature.length();
	int iDateLength = 8 + 8;//开始日期 20241219 结束日期 20241220

	if (p_strFeatrueInfo.length() != uiClientInfoLen + iAuthKeyLength + iSignatureLength + iDateLength)
	{
		//长度匹配失败 公钥+TJZT+开始日期+结束日期
		return -3;
	}

	std::string strClientInfo = p_strFeatrueInfo.substr(0, uiClientInfoLen);
	if (strClientInfo != p_strClientInfo)
	{
		//客户信息匹配失败
		return -4;
	}

	std::string strAuthKey = p_strFeatrueInfo.substr(uiClientInfoLen, iAuthKeyLength);
	if (strAuthKey != g_strAuthKey)
	{
		//公钥匹配失败
		return -5;
	}

	std::string strSignature = p_strFeatrueInfo.substr(uiClientInfoLen + iAuthKeyLength, iSignatureLength);
	if (strSignature != g_strSignature)
	{
		//签名匹配失败
		return -6;
	}

	//3、检验结束日期时间
	std::string strStartEndDate = p_strFeatrueInfo.substr(uiClientInfoLen + iAuthKeyLength + iSignatureLength, iDateLength);
	int iStartDate = atoi(strStartEndDate.substr(0, 8).c_str());
	int iEndDate = atoi(strStartEndDate.substr(8).c_str());
	int iCurDate = GetCurDate();

	if (iCurDate > iEndDate)
	{
		//结束日期失效
		return -7;
	}

	if (iStartDate > iEndDate)
	{
		//开始结束日期时序错误
		return -8;
	}

	int iNextDate = GetNextDate(iStartDate, AUTH_DAY);
	if (iNextDate != iEndDate)
	{
		//授权结束日期错误 未按照制定天数授权
		return -9;
	}

	return 0;
}
#else
int BuildFeatrueByPlainClientInfo(const std::string& p_strClientInfo, int p_iAuthDay, std::string& p_strFeatrue)
{
	return -99;
}
int AuthFeatrueByPlainClientInfo(const std::string& p_strClientInfo, const std::string& p_strFeatrue, std::string& p_strFeatrueInfo)
{
	return -99;
}
int BuildFeatrue(std::string p_strClientInfo, unsigned int p_uiClientInfoLen, int p_iAuthDay, std::string& p_strFeatrue)
{
	return -99;
}
int AuthFeatrue(const std::string p_strClientInfo, std::string p_strFeatrue, std::string& p_strFeatrueInfo)
{
	return -99;
}
#endif


END_NAMESPACE_NSDK

