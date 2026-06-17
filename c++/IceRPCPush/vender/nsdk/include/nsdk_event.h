#ifndef __NSDK_EVENT_H__
#define __NSDK_EVENT_H__

#include <string>
#include <sstream>

#include "nsdk_define.h"

#if defined(OS_IS_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>
#else
#include <pthread.h>
#include <errno.h>
#endif

BGN_NAMESPACE_NSDK

class CEvent
{
public:
	CEvent(void);
	~CEvent();

	int Create(bool p_bManualReset = false, bool p_bInitialState = false, const char* p_pszName = NULL);
	int Close(void);

	int SetEvent(void);
	int ResetEvent(void);
	int WaitEvent(unsigned int p_uiMilliseconds = INFINITE);

	int GetLastError(void) { return m_iLastError; }
	const char* GetLastErrorMsg(void) { return m_strLastErrorMsg.c_str(); }

protected:
	int SetLastError(const char* p_pszHostFunc, const char* p_pszCallFunc);

protected:
#if defined(OS_IS_WINDOWS)
	HANDLE m_hEvent;
#else
	bool m_bNamed;                // 是否为命名事件。
	std::string m_strName;        // 命名事件对应的共享内存名称。
	std::string m_strLockFile;    // 创建和关闭命名事件时使用的协调锁文件。

	pthread_mutex_t m_stMutex;    // 无名事件使用的本地互斥量。
	pthread_cond_t m_stCond;      // 无名事件使用的本地条件变量。
	pthread_mutex_t* m_pstMutex;  // 当前实际使用的互斥量，可能指向本地或共享内存。
	pthread_cond_t* m_pstCond;    // 当前实际使用的条件变量，可能指向本地或共享内存。
	bool m_bSignaled;             // 无名事件的信号状态。
	bool* m_pbSignaled;           // 当前实际使用的信号状态，可能指向本地或共享内存。
	void* m_pMappedAddr;          // 命名事件共享内存的映射首地址。
	int m_iShmFd;                 // 命名事件 shm_open 返回的句柄。
	int m_iLockFd;                // 协调锁文件句柄。
	bool m_bManualReset;          // 事件是否为手动复位。
	bool m_bInit;                 // 当前事件对象是否已经初始化完成。
#endif

	int m_iLastError;
	std::string m_strLastErrorMsg;
};

END_NAMESPACE_NSDK

#endif  // __xsdk_event_h__
