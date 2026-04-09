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
	bool m_bNamed;                // 是否为命名锁。
	std::string m_strName;        // 命名锁对应的共享内存名称。
	std::string m_strLockFile;    // 创建和关闭命名锁时使用的协调锁文件。

	pthread_mutex_t m_stMutex;    // 无名锁使用的进程内互斥量实体。
	pthread_mutex_t* m_pstMutex;  // 当前实际使用的互斥量，可能指向本地或共享内存。
	void* m_pMappedAddr;          // 命名锁共享内存的映射首地址。
	int m_iShmFd;                 // 命名锁 shm_open 返回的句柄。
	int m_iLockFd;                // 协调锁文件句柄。
	bool m_bInit;                 // 当前锁对象是否已经初始化完成。
#endif

	int m_iLastError;
	std::string m_strLastErrorMsg;
};

END_NAMESPACE_NSDK

#endif  // __xsdk_mutex_h__
