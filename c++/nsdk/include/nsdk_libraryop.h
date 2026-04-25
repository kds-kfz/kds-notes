#ifndef _NSDK_LIBRARYOP_H_
#define _NSDK_LIBRARYOP_H_

#include "nsdk_define.h"

#if defined(OS_IS_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#endif  // defined(OS_IS_WINDOWS)

#if defined(OS_IS_LINUX) || defined(OS_IS_AIX)
#include <unistd.h>
#include <dlfcn.h>
#endif  // defined(OS_IS_LINUX) || defined(OS_IS_AIX)

#if defined(OS_IS_LINUX) || defined(OS_IS_AIX)
typedef void* HMODULE;
#endif  // defined(OS_IS_LINUX) || defined(OS_IS_AIX)


class CLibraryOp
{
public:
	HMODULE m_Hmod;

	CLibraryOp()
	{
		HMODULE hMod = NULL;
		m_Hmod = hMod;
	}

	CLibraryOp(const char* p_pszFileName) : m_Hmod(NULL)
	{
		Load(p_pszFileName);
	}

	~CLibraryOp()
	{
		Free();
	}

	//--------------------------------------------------------------------------------------
	// 函数名称：Load
	// 函数描述：用于打开指定名字(p_pszFileName)的动态链接库，并返回操作句柄
	// 其他说明：(空)
	// 入参说明：[in]   const char*    p_pszFileName    无    动态链接库文件名
	// 返回结果：bool  调用结果
	//                 false    表示返回句柄失败
	//                 true     表示返回句柄成功
	//--------------------------------------------------------------------------------------
	bool Load(const char* p_pszFileName)
	{
		bool bRetVal = false;
		if (!p_pszFileName)
		{
			goto __end;
		}
		Free();

#if defined OS_IS_WINDOWS
		m_Hmod = ::LoadLibrary(p_pszFileName);
#else
		m_Hmod = dlopen(p_pszFileName, RTLD_NOW);
#endif

		bRetVal = (m_Hmod != NULL);

	__end:
		return bRetVal;
	}

	//--------------------------------------------------------------------------------------
	// 函数名称：Free
	// 函数描述：用于关闭指定句柄的动态链接库
	// 其他说明：(空)
	// 入参说明：(空)
	// 返回结果：(空)
	//--------------------------------------------------------------------------------------
	void Free()
	{
		if (IsLoaded())
		{
#if defined OS_IS_WINDOWS
			::FreeLibrary(m_Hmod);
#else
			dlclose(m_Hmod);
#endif
			m_Hmod = NULL;
		}
		return;
	}

	//--------------------------------------------------------------------------------------
	// 函数名称：IsLoaded
	// 函数描述：用于确定句柄是否为NULL
	// 其他说明：(空)
	// 入参说明：(空)
	// 返回结果：bool  调用结果
	//                 false    表示句柄为NULL
	//                 true     表示句柄不为NULL
	//--------------------------------------------------------------------------------------
	bool IsLoaded() const
	{
		return m_Hmod != NULL;
	}

	//--------------------------------------------------------------------------------------
	// 函数名称：GetFuncAddress
	// 函数描述：判断是否取到函数执行地址, 并保存地址
	// 其他说明：(空)
	// 入参说明：[in]   const char*    p_pszFileName    无    函数名
	//                  void **       p_pvdAddress     无    函数地址
	// 返回结果：bool  调用结果
	//                 false    表示获取地址失败
	//                 true     表示获取地址成功
	//--------------------------------------------------------------------------------------
	bool GetFuncAddress(void** p_pvdAddress, const char* p_pszFuncName) const
	{
		bool bRetVal = false;
		if (p_pszFuncName == NULL || !IsLoaded())
		{
			goto __end;
		}

#if defined OS_IS_WINDOWS
		* p_pvdAddress = (void*)::GetProcAddress(m_Hmod, p_pszFuncName);
#endif

#if defined(OS_IS_LINUX) || defined(OS_IS_AIX)
		*p_pvdAddress = dlsym(m_Hmod, p_pszFuncName);
#endif

		bRetVal = (*p_pvdAddress != NULL);

	__end:
		return bRetVal;
	}

};

#endif
