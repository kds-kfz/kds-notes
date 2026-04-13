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

	sem_t m_stSem;
	sem_t* m_pstSem;

	pthread_mutex_t m_stMutex;
	bool m_bInit;
#endif

	int m_iLastError;
	std::string m_strLastErrorMsg;
};

END_NAMESPACE_NSDK

#endif  // __xsdk_mutex_h__
