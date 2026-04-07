#ifndef _NSDK_DLL_H_
#define _NSDK_DLL_H_

#ifdef NSDK_EXPORTS
#define NSDK_API __declspec(dllexport)
#else
#define NSDK_API __declspec(dllimport)
#endif

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>

#define F_OK 0	// 是否存在
#define X_OK 1	// 执行权限
#define W_OK 2	// 写入权限
#define R_OK 4	// 读取权限
#define RW_OK 6 // 读写权限

#ifdef __cplusplus
extern "C"
{
#endif
	// 判断文件夹存在否
	NSDK_API int FolderExists(const char *p_szFolderPath);
	// 创建文件夹
	NSDK_API int CreateFolder(const char *p_szFolderPath);
	// 获取文件属性
	NSDK_API int GetFileAttr(const char *p_szFilePath, int p_iMode);
	// 创建文件夹
	NSDK_API int CreatePath(const char* p_szFolderPath);
	// 路径双斜杠校验
	NSDK_API bool RegularPath(std::string &sPath);
	// 获取当前路径
	NSDK_API const char* GetRootPath(void);
	// 字符串按分隔符获取
	NSDK_API unsigned int StringSplit(const std::string p_strSrc, const std::string p_strSep, std::vector<std::string> &p_vecObj);
	// 获取文件大小
	NSDK_API unsigned long FileLength(FILE * p_pFile);
	// 获取前后日期
	NSDK_API unsigned long GetNextDate(unsigned long p_ulDate, unsigned int p_uiDays);
	// 获取当前系统日期
	NSDK_API unsigned int GetCurDate(bool p_bDate = true);
	// 获取某年某月某日所在的周五日期
	NSDK_API unsigned long GetFriday(unsigned long p_ulDate);
	// 获取路径下所有文件
	NSDK_API int GetAllFiles(std::string strDir, std::vector<std::string>& filesVec);
	// 删除文件加下所有内容
	NSDK_API bool DeleteDirectory(const char* p_szFilePath, const char* p_szExpath = "");
	// 判断彼此是否相等
	NSDK_API bool IsEquals(double p_dData1, double p_dData2, int p_iXsFlag);


#ifdef __cplusplus
}
#endif

//num转string
template <class T>
std::string NumToString(T& p_anyNum, int p_iPrecision)
{
	std::ostringstream strStream;
	strStream << std::fixed << std::setprecision(p_iPrecision) << p_anyNum;
	return strStream.str();
}

//string转num
template <class T>
T StringToNum(std::string& p_strValue, int p_iPrecision = 5)
{
	std::istringstream strStream(p_strValue);
	T anyNum;
	strStream >> std::fixed >> std::setprecision(p_iPrecision) >> anyNum;
	return anyNum;
}

#endif // _NSDK_DLL_H_
