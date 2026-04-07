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
#include <sstream>
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
	// 判断文件夹存在否
	NSDK_API int FolderExists(const char* p_szFolderPath);
	// 创建文件夹
	NSDK_API int CreateFolder(const char* p_szFolderPath);
	// 路径斜杠转换
	NSDK_API bool RegularPath(std::string& p_strPath);
	// 获取当前路径
	NSDK_API const char* GetRootPath(void);
	// 获取路径下所有文件
	NSDK_API int GetAllFiles(std::string p_strDir, std::vector<std::string>& p_vecFiles);
	// 删除文件夹下所有内容
	NSDK_API bool DeleteDirectory(const char* p_szFilePath, const char* p_szExpath = "");

	/******************** 文件处理 ********************/
	// 文件是否存在
	NSDK_API int FileExists(const char* p_szFilePath);
	// 获取文件属性
	NSDK_API int GetFileAttr(const char* p_szFilePath, int p_iMode);
	// 获取文件大小
	NSDK_API unsigned long FileLength(FILE* p_pFile);

	/******************** 字符串处理 ********************/
	// 字符串按分隔符获取
	NSDK_API unsigned int StringSplit(const std::string p_strSrc, const std::string p_strSep, std::vector<std::string>& p_vecObj);
	// UTF8转ASC
	NSDK_API const char* UTF82ASC(const char* p_szSrcbuf, char* p_szOutbuf, int p_iOutlen);
	// 字符串除空格(直接操作自身)
	NSDK_API void TrimCharArraySelf(char* p_pzStr);
	// 字符串除空格(以string返回Trim结果)
	NSDK_API void TrimCharArray(std::string& p_refStr, const char* p_pzStr, int p_iLen);
	// 字符串除空格(直接操作自身)
	NSDK_API void TrimStr(std::string& p_refStr);
	// 字符串格式化
	NSDK_API std::string FormatString(const char* p_szFormat, ...);
	// 字符串 append int
	NSDK_API std::string Add2String(const std::string& p_strSrc, int p_iData);
	// 字符串小写格式化
	NSDK_API char* CharArrayLower(char* p_pszStr);
	// 字符串大写格式化
	NSDK_API char* CharArrayUpper(char* p_pszStr);
	// 字符串小写格式化
	NSDK_API const char* StringLower(std::string& p_strSrc);
	// 字符串大写格式化
	NSDK_API const char* StringUpper(std::string& p_strSrc);

	/******************** 日期时间处理 ********************/
	// 获取前后日期
	NSDK_API unsigned long GetNextDate(unsigned long p_ulDate, unsigned int p_uiDays);
	// 获取当前系统日期
	NSDK_API unsigned int GetCurDate(bool p_bDate = true);
	// 获取当前时间精确到毫秒
	NSDK_API int GetCurDateTime(char* p_pDateTime, int p_iBufLen);
	// 获取当前时间精确到毫秒
	NSDK_API void GetLocalDateTime(int& p_iDate, int& p_iTime);
	// 获取当前时间精确到毫秒
	NSDK_API long long GetCurrentTimeMillis();
	// 支持传入格式
	NSDK_API std::string GetTimes(const std::string& p_strFmt);
	// 获取某年某月某日所在的周五日期
	NSDK_API unsigned long GetFriday(unsigned long p_ulDate);
	// 判断日期是否是周末 p_iDate=yyyymmdd
	NSDK_API bool IsInWeekend(int p_iDate);

	/******************** 数字处理 ********************/
	// 判断彼此是否相等
	NSDK_API bool IsEquals(double p_dData1, double p_dData2, int p_iXsFlag);
	// 判断是否为零
	NSDK_API bool IsEqualsZero(double p_dData);
	// 浮点数四舍五入
	NSDK_API double Round(double p_dData, short p_sPlaces = 2);
	// 浮点数转整形
	NSDK_API int Double2Int(double p_dData);
	
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
