#ifndef _NSDK_DLL_H_
#define _NSDK_DLL_H_

#include "nsdk_define.h"

#if defined(OS_IS_WINDOWS)
#if defined(NSDK_STATIC)
#define NSDK_API
#elif defined(NSDK_EXPORTS)
#define NSDK_API __declspec(dllexport)
#else
#define NSDK_API __declspec(dllimport)
#endif
#else
#define NSDK_API __attribute__((visibility("default")))
#endif

#include <cstdio>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

BGN_NAMESPACE_NSDK

#define F_OK 0 // 文件是否存在
#define X_OK 1 // 执行权限
#define W_OK 2 // 写权限
#define R_OK 4 // 读权限
#define RW_OK 6 // 读写权限

#define NSDK_MAX_PATH 260
#define NSDK_BUF_PATH 260
#define NSDK_MAX_BUF 1024

#ifdef __cplusplus
extern "C"
{
#endif
	/******************** 文件夹处理 ********************/
	// 判断文件夹是否存在。
	NSDK_API int FolderExists(const char* p_szFolderPath);
	// 创建文件夹。
	NSDK_API int CreateFolder(const char* p_szFolderPath);
	// 统一路径分隔符。
	NSDK_API bool RegularPath(std::string& p_strPath);
	// 获取当前程序根目录。
	NSDK_API const char* GetRootPath(void);
	// 获取目录下的全部文件。
	NSDK_API int GetAllFiles(std::string p_strDir, std::vector<std::string>& p_vecFiles);
	// 获取目录下的全部子目录。
	NSDK_API int GetSubDirs(std::string p_strDir, std::vector<std::string>& p_vecFiles);
	// 递归删除目录，可通过排除路径保留指定内容。
	NSDK_API bool DeleteDirectory(const char* p_szFilePath, const char* p_szExpath = "");

	/******************** 文件处理 ********************/
	// 判断文件是否存在。
	NSDK_API int FileExists(const char* p_szFilePath);
	// 获取文件属性。
	NSDK_API int GetFileAttr(const char* p_szFilePath, int p_iMode);
	// 获取文件大小。
	NSDK_API unsigned long FileLength(FILE* p_pFile);
	// 以二进制追加方式打开文件，失败时返回 nullptr。
	NSDK_API FILE* CreateAppendFile(const char* p_pszFile);

	/******************** 字符串处理 ********************/
	// 按指定分隔符拆分字符串。
	NSDK_API unsigned int StringSplit(const std::string p_strSrc,
		const std::string p_strSep, std::vector<std::string>& p_vecObj);
	// 将 UTF-8 字符串转换为本地窄字符编码。
	NSDK_API int UTF82ASC(const char* p_szSrcbuf, char* p_szOutbuf, int p_iOutlen);
	// 原地移除字符数组两端空格。
	NSDK_API void TrimCharArraySelf(char* p_pzStr);
	// 移除字符数组两端空格并写入 string。
	NSDK_API void TrimCharArray(std::string& p_refStr, const char* p_pzStr, int p_iLen);
	// 原地移除字符串两端空格。
	NSDK_API void TrimStr(std::string& p_refStr);
	// 移除字符串两端指定字符。
	NSDK_API void TrimStrByChar(std::string& p_refStr, char p_ch);
	// 按 printf 格式生成字符串。
	NSDK_API std::string FormatString(const char* p_szFormat, ...);
	// 将整数追加到字符串。
	NSDK_API std::string Add2String(const std::string& p_strSrc, int p_iData);
	// 将字符数组转换为小写。
	NSDK_API char* CharArrayLower(char* p_pszStr);
	// 将字符数组转换为大写。
	NSDK_API char* CharArrayUpper(char* p_pszStr);
	// 将字符串转换为小写。
	NSDK_API const char* StringLower(std::string& p_strSrc);
	// 将字符串转换为大写。
	NSDK_API const char* StringUpper(std::string& p_strSrc);
	// 安全复制 C 字符串，目标始终以 '\0' 结尾。
	NSDK_API void SafeCopyCString(char* p_szDst, size_t p_dwDstLen, const char* p_szSrc);

	/******************** 日期时间处理 ********************/
	// 获取指定日期若干天后的日期。
	NSDK_API unsigned long GetNextDate(unsigned long p_ulDate, unsigned int p_uiDays);
	// 获取当前系统日期或时间。
	NSDK_API unsigned int GetCurDate(bool p_bDate = true);
	// 获取当前时间。iType=0 返回 yyyyMMdd hh:mm ss:zzz，iType=1 返回 yyyy-MM-dd hh:mm:ss。
	NSDK_API int GetCurDateTime(char* p_pDateTime, int p_iBufLen, int p_iType = 0);
	// 分别返回本地日期和时间。
	NSDK_API void GetLocalDateTime(int& p_iDate, int& p_iTime);
	// 获取当前 UTC 毫秒时间戳。
	NSDK_API long long GetCurrentTimeMillis();
	// 按指定格式生成当前时间字符串。
	NSDK_API std::string GetTimes(const std::string& p_strFmt);
	// 获取指定日期所在周的星期五。
	NSDK_API unsigned long GetFriday(unsigned long p_ulDate);
	// 判断 yyyyMMdd 日期是否为周末。
	NSDK_API bool IsInWeekend(int p_iDate);
	// 线程安全地获取本地时间。
	NSDK_API std::tm SafeLocalTime(std::time_t p_ttNow);

	/******************** 数值处理 ********************/
	// 按指定小数位比较两个浮点数。
	NSDK_API bool IsEquals(double p_dData1, double p_dData2, int p_iXsFlag);
	// 判断浮点数是否为零。
	NSDK_API bool IsEqualsZero(double p_dData);
	// 按指定小数位四舍五入。
	NSDK_API double Round(double p_dData, short p_sPlaces = 2);
	// 将浮点数转换为整数。
	NSDK_API int Double2Int(double p_dData);

	/******************** 崩溃转储处理 ********************/
	// 初始化崩溃转储；空路径默认使用 GetRootPath() + CrashDumps。
	NSDK_API int InitBreakpad(const char* p_szDumpPath = nullptr);

	/******************** 操作系统处理 ********************/
	// 获取最后一个系统错误码。
	NSDK_API unsigned long GetSysError();
	// 获取 CPU 核心数量；p_bUsable=true 时仅统计可用核心。
	NSDK_API unsigned long GetNumberOfCores(bool p_bUsable = false);

#ifdef __cplusplus
}
#endif

// 将数值转换为字符串。p_iPrecision 为兼容旧接口保留。
template <class T>
std::string NumToString(T& p_anyNum, int p_iPrecision)
{
	(void)p_iPrecision;
	std::ostringstream strStream;
	strStream << p_anyNum;
	return strStream.str();
}

// 将字符串转换为数值。p_iPrecision 为兼容旧接口保留。
template <class T>
T StringToNum(std::string& p_strValue, int p_iPrecision = 5)
{
	(void)p_iPrecision;
	std::istringstream strStream(p_strValue);
	T anyNum;
	strStream >> anyNum;
	return anyNum;
}

END_NAMESPACE_NSDK

#endif // _NSDK_DLL_H_
