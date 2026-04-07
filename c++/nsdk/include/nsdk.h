#ifndef _NSDK_DLL_H_
#define _NSDK_DLL_H_

#include "nsdk_define.h"

#if defined(OS_IS_WINDOWS)
#define NSDK_API       __declspec(dllexport)
#else
#define NSDK_API
#endif  // defined(OS_IS_WINDOWS)

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

BGN_NAMESPACE_NSDK

#define F_OK	0 // 是否存在
#define X_OK	1 // 执行权限
#define W_OK	2 // 写入权限
#define R_OK	4 // 读取权限
#define RW_OK	6 // 读写权限

#ifdef __cplusplus
extern "C"
{
#endif
	/******************** 文件夹处理 ********************/
	class NSDK_API CFolder
	{
		CFolder();
		~CFolder() {}
		// 判断文件夹存在否
		static int FolderExists(const char* p_szFolderPath);
		// 创建文件夹
		static int CreateFolder(const char* p_szFolderPath);
		// 路径斜杠转换
		static bool RegularPath(std::string& p_strPath);
		// 获取当前路径
		static const char* GetRootPath(void);
		// 获取路径下所有文件
		static int GetAllFiles(std::string p_strDir, std::vector<std::string>& p_vecFiles);
		// 删除文件夹下所有内容
		static bool DeleteDirectory(const char* p_szFilePath, const char* p_szExpath = "");
	};

	/******************** 文件处理 ********************/
	class NSDK_API CFile
	{
		CFile() {}
		~CFile() {}
		// 文件是否存在
		static int FileExists(const char* p_szFilePath);
		// 获取文件属性
		static int GetFileAttr(const char* p_szFilePath, int p_iMode);
		// 获取文件大小
		static unsigned long FileLength(FILE* p_pFile);
	};

	/******************** 字符串处理 ********************/
	class NSDK_API CString
	{
		CString() {}
		~CString() {}
		// 字符串按分隔符获取
		static unsigned int StringSplit(const std::string p_strSrc, const std::string p_strSep, std::vector<std::string>& p_vecObj);
	};
	
	/******************** 日期时间处理 ********************/
	class NSDK_API CDateTime
	{
		CDateTime() {}
		~CDateTime() {}
		// 获取前后日期
		static unsigned long GetNextDate(unsigned long p_ulDate, unsigned int p_uiDays);
		// 获取当前系统日期
		static unsigned int GetCurDate(bool p_bDate = true);
		// 获取当前时间精确到毫秒
		static int GetCurDateTime(char* p_pDateTime, int p_iBufLen);
		// 获取当前时间精确到毫秒
		static void GetLocalDateTime(int& p_iDate, int& p_iTime);
		// 支持传入格式
		static std::string GetTimes(const std::string& p_strFmt);
		// 获取某年某月某日所在的周五日期
		static unsigned long GetFriday(unsigned long p_ulDate);
	};

	/******************** 数字处理 ********************/
	class NSDK_API CDigit
	{
		CDigit() {}
		~CDigit() {}
		// 判断彼此是否相等
		static bool IsEquals(double p_dData1, double p_dData2, int p_iXsFlag);
	};

	
#ifdef __cplusplus
}
#endif

//num转string
template <class T>
std::string NumToString(T& p_anyNum, int p_iPrecision)
{
	std::ostringstream strStream;
	//strStream << std::fixed << std::setprecision(p_iPrecision) << p_anyNum;
	strStream << p_anyNum;
	return strStream.str();
}

//string转num
template <class T>
T StringToNum(std::string& p_strValue, int p_iPrecision = 5)
{
	std::istringstream strStream(p_strValue);
	T anyNum;
	//strStream >> std::fixed >> std::setprecision(p_iPrecision) >> anyNum;
	strStream >> anyNum;
	return anyNum;
}

END_NAMESPACE_NSDK


#endif // _NSDK_DLL_H_
