#ifndef _NSDK_MUTEX_H_
#define _NSDK_MUTEX_H_

#include <string>
#include <sstream>

#include "nsdk_define.h"

#if defined(OS_IS_WINDOWS)
#include <windows.h>
#else
#include <pthread.h>
#include <errno.h>
#endif

BGN_NAMESPACE_NSDK

class CMutex
{
public:
	CMutex(void);
	~CMutex();

	int Create(const char* p_pszName = NULL);
	int Close(void);

	int Lock(unsigned int p_uiMilliseconds = INFINITE);
	int Unlock(void);

	int GetLastError(void) { return m_iLastError; }
	const char* GetLastErrorMsg(void) { return m_strLastErrorMsg.c_str(); }

protected:
	int SetLastError(const char* p_pszHostFunc, const char* p_pszCallFunc);

protected:
#if defined(OS_IS_WINDOWS)
	HANDLE m_hMutex;
#else
	bool m_bNamed;
	std::string m_strName;
	std::string m_strLockFile;
	pthread_mutex_t m_stMutex;
	pthread_mutex_t* m_pstMutex;
	void* m_pMappedAddr;
	int m_iShmFd;
	int m_iLockFd;
	bool m_bInit;
#endif

	int m_iLastError;
	std::string m_strLastErrorMsg;
};

inline CMutex::CMutex(void)
#if defined(OS_IS_WINDOWS)
	: m_hMutex(NULL), m_iLastError(0)
#else
	: m_bNamed(false), m_pstMutex(nullptr), m_pMappedAddr(nullptr), m_iShmFd(-1), m_iLockFd(-1), m_bInit(false), m_iLastError(0)
#endif
{
}

inline CMutex::~CMutex()
{
	Close();
}

inline int CMutex::Create(const char* p_pszName)
{
	(void)p_pszName;
#if defined(OS_IS_WINDOWS)
	m_hMutex = reinterpret_cast<HANDLE>(1);
#else
	m_bInit = true;
#endif
	m_iLastError = 0;
	m_strLastErrorMsg.clear();
	return 0;
}

inline int CMutex::Close(void)
{
#if defined(OS_IS_WINDOWS)
	m_hMutex = NULL;
#else
	m_bInit = false;
#endif
	return 0;
}

inline int CMutex::Lock(unsigned int p_uiMilliseconds)
{
	(void)p_uiMilliseconds;
	return 0;
}

inline int CMutex::Unlock(void)
{
	return 0;
}

inline int CMutex::SetLastError(const char* p_pszHostFunc, const char* p_pszCallFunc)
{
	m_iLastError = -1;
	m_strLastErrorMsg.clear();
	if (p_pszHostFunc != NULL)
	{
		m_strLastErrorMsg.append(p_pszHostFunc);
	}
	if (p_pszCallFunc != NULL)
	{
		if (!m_strLastErrorMsg.empty())
		{
			m_strLastErrorMsg.append(":");
		}
		m_strLastErrorMsg.append(p_pszCallFunc);
	}
	return m_iLastError;
}

END_NAMESPACE_NSDK

#endif  // _NSDK_MUTEX_H_