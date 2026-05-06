//------------------------------------------------------------------
// 版权声明：本程序模块属于金证券商综合业务系统(KBSS)的一部分
//           金证科技股份有限公司  版权所有
//
// 文件名称：xsdk_folder.h
// 模块名称：目录操作类
// 模块描述：本模块提供对目录操作的统一接口
//
// 开发作者：钟兆斌
// 创建日期：2012-02-21
// 模块版本：1.0.000.000
//-------------------------------------------------------------------
// 修改日期      版本              作者            备注
//-------------------------------------------------------------------
// 2012-02-21  1.0.000.000        钟兆斌          原创
//-------------------------------------------------------------------
#if !defined(__XSDK_FOLDER_H__)
#define __XSDK_FOLDER_H__

#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include "xsdk_define.h"

BGN_NAMESPACE_XSDK

#ifdef __cplusplus
extern "C"
{
#endif


#ifdef OS_IS_WINDOWS
//#define snprintf _snprintf
#define rmdir _rmdir
#define mkdir _mkdir
#else

#endif

//--------------------------------------------------------------------
//   类名称：CFolder
// 简要描述：该类提供对目录的基础操作接口，创建、删除、复制等
//--------------------------------------------------------------------
class CFolder
{
  public:
    CFolder() {};

    ~CFolder() {};

    //--------------------------------------------------------------------------------
    // 函数名称：CFolder::FolderExists
    // 简要描述：判断文件是否存在
    //
    // 其他说明：
    // 入参说明：[in]  string  p_pszFilePath    无   文件完整路径
    // 返回结果：int   调用结果
    //                  0   存在
    //                  -1  不存在
    //--------------------------------------------------------------------------------
    static int FolderExists(const char *p_pszFilePath);
    ///PathIsDirectory  winapi
    
    //--------------------------------------------------------------------------------
    // 函数名称：CFolder::GetCurFolder
    // 简要描述：获取当前目录
    //
    // 其他说明：
    // 入参说明：[out] string  p_pszBuffer  无   保存当前目录字符串
    //           [in]  int     p_iLength    无   字符串长度
    // 返回结果： int  调用结果
    //                  0   存在
    //                  -1  不存在
    //--------------------------------------------------------------------------------
    static int GetCurFolder(char *p_pszBuffer, int p_iLength);
    ///GetCurrentDirectory winapi
    
    
    //--------------------------------------------------------------------------------
    // 函数名称：CFolder::GetAllFileNames
    // 简要描述：取指定文件夹中的所有文件名
    //
    // 其他说明：
    // 入参说明：[out] array   p_aszFileNames   无   文件名字符串数组
    //           [in]  string  p_pszFolderPath  无   文件夹完整路径
    // 返回结果： int  调用结果
    //                  0    调用成功
    //                  非0  错误代码
    //--------------------------------------------------------------------------------
    static int GetAllFileNames(std::vector<std::string> &p_vecFileNames, const char *p_pszFolderPath, const char *p_pszFilter = NULL);
    
    //--------------------------------------------------------------------------------
    // 函数名称：CFolder::CreateFolder
    // 简要描述：创建文件夹
    //
    // 其他说明：
    // 入参说明：[in]  string  p_pszFolderPath  无   文件夹完整路径
    // 返回结果： int  调用结果
    //                  0    调用成功
    //                  非0  错误代码
    //--------------------------------------------------------------------------------
    static int CreateFolder(char *p_pszFolderPath);
    ///CreateDirectory  winapi
    
    //--------------------------------------------------------------------------------
    // 函数名称：CFolder::DelFolder
    // 简要描述：删除指定文件夹
    //
    // 其他说明：
    // 入参说明：[in]  string  p_pszPath  无   指定路径
    // 返回结果： int  调用结果
    //                  0    调用成功
    //                  非0  错误代码
    //--------------------------------------------------------------------------------
    static int DelFolder(const char *p_pszPath);
    ///RemoveDirectory  winapi
    
    
    //--------------------------------------------------------------------------------
    // 函数名称：CFolder::CopyFolder
    // 简要描述：复制文件夹
    //
    // 其他说明：
    // 入参说明：[in]  string  p_pszDestPath  无   拷贝目的文件夹路径
    //           [in]  string  p_pszSourPath  无   拷贝源文件夹路径
    // 返回结果： int  调用结果
    //                  0    调用成功
    //                  非0  错误代码
    //--------------------------------------------------------------------------------
    static int CopyFolder(char *p_pszDestPath, char *p_pszSourPath);
  
  
};

#ifdef __cplusplus
}
#endif  //__cplusplus

END_NAMESPACE_XSDK

#endif  //__XSDK_FOLDER_H__
