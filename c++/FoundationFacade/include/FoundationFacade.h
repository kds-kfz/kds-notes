#ifndef KDS_FOUNDATION_FACADE_H
#define KDS_FOUNDATION_FACADE_H

#include "FacadeAbi.h"

#if defined(_WIN32)
#if defined(FOUNDATION_FACADE_STATIC)
#define FOUNDATION_FACADE_API
#elif defined(FOUNDATION_FACADE_EXPORTS)
#define FOUNDATION_FACADE_API __declspec(dllexport)
#else
#define FOUNDATION_FACADE_API __declspec(dllimport)
#endif
#else
#define FOUNDATION_FACADE_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C"
{
#endif

// 平台 Provider 的不透明句柄；只能由 FoundationFacade_DestroyPlatform 释放。
typedef void* FOUNDATION_PLATFORM_HANDLE;

// 取得 ABI、实现版本、私有依赖版本和构建标识；线程安全且不创建资源。
FOUNDATION_FACADE_API int FoundationFacade_GetAbiInfo(ST_FACADE_ABI_INFO* p_pInfo);

// 按当前编译平台创建 Provider；成功后句柄可供多个线程并发执行只读平台操作。
FOUNDATION_FACADE_API int FoundationFacade_CreatePlatform(
	FOUNDATION_PLATFORM_HANDLE* p_pPlatform);

// 释放本 DLL 创建的平台句柄；空句柄安全返回，调用方必须保证没有并发调用仍在使用它。
FOUNDATION_FACADE_API void FoundationFacade_DestroyPlatform(
	FOUNDATION_PLATFORM_HANDLE p_pPlatform);

// 返回 YYYYMMDD 格式的当前本地日期；输出范围为 19700101..99991231。
FOUNDATION_FACADE_API int FoundationFacade_GetLocalDate(
	FOUNDATION_PLATFORM_HANDLE p_pPlatform, uint32_t* p_puiDate);

// 让当前线程休眠指定毫秒；允许 0，函数不持有内部锁。
FOUNDATION_FACADE_API int FoundationFacade_SleepMs(
	FOUNDATION_PLATFORM_HANDLE p_pPlatform, uint32_t p_uiMilliseconds);

// 判断 UTF-8 路径是否为普通文件；成功时 p_piExists 输出 0 或 1。
FOUNDATION_FACADE_API int FoundationFacade_FileExists(
	FOUNDATION_PLATFORM_HANDLE p_pPlatform, const ST_FACADE_STRING_VIEW* p_pPath,
	int32_t* p_piExists);

// 判断 UTF-8 路径是否为目录；成功时 p_piExists 输出 0 或 1。
FOUNDATION_FACADE_API int FoundationFacade_DirectoryExists(
	FOUNDATION_PLATFORM_HANDLE p_pPlatform, const ST_FACADE_STRING_VIEW* p_pPath,
	int32_t* p_piExists);

// 递归创建 UTF-8 目录；目录已经存在也视为成功。
FOUNDATION_FACADE_API int FoundationFacade_CreateDirectories(
	FOUNDATION_PLATFORM_HANDLE p_pPlatform, const ST_FACADE_STRING_VIEW* p_pPath);

// 返回当前平台路径分隔符；Windows 为反斜杠，Linux 为正斜杠。
FOUNDATION_FACADE_API int FoundationFacade_GetPathSeparator(
	FOUNDATION_PLATFORM_HANDLE p_pPlatform, char* p_pchSeparator);

#ifdef __cplusplus
}
#endif

#endif // KDS_FOUNDATION_FACADE_H
