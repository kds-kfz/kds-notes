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
#if defined(OS_IS_WINDOWS) && defined(NSDK_ENABLE_BREAKPAD)
#include "exception_handler.h"
#endif

//�ڲ�ʹ�õĺ�
#define NSDK_MAX_PATH 260
#define COMPPREC		0.001 // Roundʹ��

BGN_NAMESPACE_NSDK

#define NSDK_BREAKPAD_DUMP_FOLDER_NAME "CrashDumps"
#define NSDK_BREAKPAD_CRASH_MARKER_FILE_NAME L"last_crash.txt"

namespace
{
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

		// wyl 2026-05-07�������ֳ�ֻд�����ǣ��������ҵ����־������������ȸ����߼����¶����쳣��
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

				// wyl 2026-05-07��last_crash.txt ÿ���ֶ�˵����ʱ�䡢dump���ɽ������ǰ�߳�ID���쳣�롢�쳣��ַ��dump_id��
				// succeeded=1 ��ʾ Breakpad д dump �ɹ���exception Ϊ�쳣�룻address Ϊ�쳣������ַ��dump_id ��Ӧ���ɵ� dmp �ļ�����
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

/******************** �ļ��д��� ********************/
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
				//S_IRUSR �û���Ȩ�� S_IWUSR �û�дȨ�� S_IRGRP �û����Ȩ�� S_IWGRP �û���дȨ�� S_IROTH �������Ȩ�� S_IWOTH ������дȨ��
				//����764���ܽ��������д
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
			//�����Ŀ¼
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
	char szBasePath[NSDK_MAX_PATH] = { 0 };        //��Ŀ¼
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
		else if (10 == dp->d_type || 4 == dp->d_type)//�����ļ� //dir ��Ŀ¼��ݹ����
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
			//�����Ŀ¼
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
	char szBasePath[NSDK_MAX_PATH] = { 0 };        //��Ŀ¼
	while ((dp = readdir(dir)) != nullptr) {
		if (0 == strcmp(dp->d_name, ".") && 0 == strcmp(dp->d_name, ".."))
		{
			continue;
		}
		else if (10 == dp->d_type || 4 == dp->d_type)//�����ļ� //dir ��Ŀ¼��ݹ����
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
		// ��Ŀ¼ʧ��
		return false;
	}

	dirent* entry = nullptr;
	while ((entry = readdir(dir)) != nullptr) {
		if (strncmp(entry->d_name, ".", 1) == 0 || strcmp(entry->d_name, "..") == 0)
		{
			// ������ǰĿ¼�͸�Ŀ¼
			continue;
		}

		if (entry->d_type == DT_DIR)
		{
			// ������ļ��У��ݹ�ɾ��
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
			// ������ļ���ֱ��ɾ��
			char szFile[NSDK_MAX_PATH] = { 0 };
			snprintf(szFile, sizeof(szFile) - 1, "%s/%s", p_szFilePath, entry->d_name);
			if (remove(szFile) != 0)
			{
				closedir(dir);
				return false;
			}
		}
	}

	// �ر�Ŀ¼��
	closedir(dir);

	// ɾ����ǰĿ¼
	rmdir(p_szFilePath);//���һ��·����ɾ��,ֻɾ����p_szExpath��Ŀ¼

#endif
	return true;
}

/******************** �ļ����� ********************/
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
		return -2;//���Բ�֧��
#if defined( OS_IS_WINDOWS )
	if (_access(p_szFilePath, p_iMode) == 0)
		return 0;
#else
	if (access(p_szFilePath, p_iMode) == 0)
		return 0;
#endif
	return -3;//��Ӧ���Բ�����
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

/******************** �ַ������� ********************/
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
	//ȥ����ͷ�հ��ַ�
	while (isspace(*p_pzStr))
	{
		p_pzStr++;
	}
	//����ַ���ȫ�ǿհף���ֱ�ӷ��ؿ��ַ���
	if (*p_pzStr == '\0')
	{
		strcpy(src, p_pzStr);
		return;
	}
	//ȥ���ַ�����β�Ŀհ��ַ�
	end = p_pzStr + strlen(p_pzStr) - 1;
	while (end > p_pzStr && isspace(*end))
	{
		end--;
	}

	//�ַ�����β���ӽ�����
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
	//ȥ���ַ���ǰ����ַ�c
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
	char szBuffer[260]; // ���·������
	va_list args;
	va_start(args, p_szFormat);
	vsprintf(szBuffer, p_szFormat, args); //TODO ��̬������format��ʱ�ᵼ�³��򱨴�
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

/******************** ����ʱ�䴦�� ********************/

// �������ڽ�ָ�����ڰ���ƫ��������ƫ�Ƽ���
std::tm OffsetDays(std::tm& p_tmSpecified_date, int p_iOffset)
{
	// �����ڽ���ƫ��
	p_tmSpecified_date.tm_mday += p_iOffset;
	// �淶�����ڣ�ȷ�����ڵĺ�����
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
	// ����ƫ�ƺ�����
	tm tmSpecifiedDate;
	tmSpecifiedDate.tm_year = (p_ulDate / 10000) - 1900;
	tmSpecifiedDate.tm_mon = (p_ulDate % 10000 / 100) - 1;
	tmSpecifiedDate.tm_mday = p_ulDate % 100;
	tmSpecifiedDate.tm_hour = tmSpecifiedDate.tm_min = tmSpecifiedDate.tm_sec = 0;

	// ��ʽ����������
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
	// ʱ��
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
	// ʱ��
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

unsigned long GetFriday(unsigned long p_ulDate) // �õ�ĳ�յ�������
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
		if (i % 400 == 0 || (i % 4 == 0 && i % 100 != 0)) // ����
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

	ulDays += iDay - 1; // ��1980�������չ��ж�����
	ulDays -= 3; // 1980��1��1��Ϊ���ڶ��������������������Ҫ��ȥ3
	i = ulDays % 7;

	if (i > 2) // �õ������������ڵ������塣��Ϊ�����塢�����գ���Ϊ����
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
	// ����mktime��������׼�����ڽṹ����һ���ܹؼ��������Զ�����tm_wday�ȳ�Ա
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


/******************** ���ִ��� ********************/
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

/******************** ����ϵͳ ********************/

unsigned long GetSysError()
{
#if defined( OS_IS_WINDOWS )
	// ʱ��
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
	//sysconf(_SC_NPROCESSORS_CONF);//����ϵͳ���е�CPU���������ֵҲ����ϵͳ�н�ֹ�û�ʹ�õ�CPU����
	//sysconf(_SC_NPROCESSORS_ONLN);//����ϵͳ�п��õ�CPU����
#endif
}

END_NAMESPACE_NSDK
