#if !defined(__xsdk_io_h__)
#define __xsdk_io_h__

#include "xsdk_define.h"

BGN_NAMESPACE_XSDK

#ifdef  __cplusplus
extern "C"
{
#endif

//------------------------------------------------------------------------------
// 功能描述：
//     检测给定目录是否存在。
// 参数说明：
//     p_pszPathName[in]
//         待检测的目录。目录层次分隔支持'/'和'\'，目录尾部允许以或不以'/'或'\'结束
// 返回说明：
//     true
//         待检测的目录已存在
//     false
//         待检测的目录不存在
bool CheckDirectory(
  const char *p_pszPathName
);

//------------------------------------------------------------------------------
// 功能描述：
//     创建给定目录，中间目录如果不存在也将被创建
// 参数说明：
//     p_pszPathName[in]
//         待创建的目录。目录层次分隔支持'/'和'\'，目录尾部允许以或不以'/'或'\'结束
// 返回说明：
//     XSDK_OK
//         创建目录成功（或，待创建的目录已存在）
//     XSDK_KO
//         创建目录失败
int CreateDirectory(
  const char *p_pszPathName
);

//------------------------------------------------------------------------------
// 功能描述：
//     删除给定目录。待删除的目录必须是空的，且调用调用进程必须拥有待删除目录的删除权限。
// 参数说明：
//     p_pszPathName[in]
//         待删除的目录。目录层次分隔支持'/'和'\'，目录尾部允许以或不以'/'或'\'结束
// 返回说明：
//     XSDK_OK
//         删除目录成功（或，待删除的目录不存在）
//     XSDK_KO
//         删除目录失败
int RemoveDirectory(
  const char *p_pszPathName
);

//------------------------------------------------------------------------------
// 功能描述：
//     获取当前进程运行的目录及文件名称。
// 参数说明：
//     p_pszPathName[out]
//         存储当前进程运行目录的内存空间指针。
//         Windows操作系统环境，目录层次以'\'分隔；UNIX系操作系统环境，目录层次以'/'分隔。
//     p_iPathNameSize[in]
//         p_pszPathName的字节大小
//     p_pszFileName[out]
//         存储当前进程文件名称的内存空间指针。
//     p_iFileNameSize[in]
//         p_pszFileName的字节大小
// 返回说明：
//     XSDK_OK
//         函数执行成功
//     XSDK_KO
//         函数执行失败
int GetCurrentFilePath(
  char *p_pszPathName,
  int p_iPathNameSize,
  char *p_pszFileName,
  int p_iFileNameSize
);

#ifdef __cplusplus
}
#endif

END_NAMESPACE_XSDK

#endif  // __xsdk_io_h__
