#ifndef _NSDK_THREAD_H_
#define _NSDK_THREAD_H_

#include "nsdk_define.h"
#include "nsdk_event.h"

#if defined(OS_IS_WINDOWS)
#include <windows.h>
#else
#include <pthread.h>
#include <errno.h>
#endif

BGN_NAMESPACE_NSDK

#if defined(OS_IS_WINDOWS)
unsigned __stdcall ThreadProxy(void* p_pvdParam);
typedef unsigned(__stdcall* PFN_THREAD_FUNC)(void*);
#else
void* ThreadProxy(void* p_pvdParam);
typedef void* (*PFN_THREAD_FUNC)(void*);
#endif

class CBaseThread
{
public:
	CBaseThread(void);
	virtual ~CBaseThread();

public:
	virtual int CreateThread(void* p_pvdParam = NULL);
	virtual int DeleteThread(void);

	virtual int Initialize(void) { return NSDK_OK; }
	virtual int Uninitialize(void) { return NSDK_OK; }

	virtual int Run(void);
	virtual int Work(void) = 0;

	virtual int OnPause(void) { return NSDK_OK; }
	virtual int OnResume(void) { return NSDK_OK; }
	virtual int Pause(void);
	virtual int Resume(void);

	virtual bool IsRunning(void);
	virtual unsigned int GetThreadID(void) const { return m_uiThreadID; }

protected:
#if defined(OS_IS_WINDOWS)
	HANDLE m_hThread;
#else
	pthread_t m_hThread;
#endif
	unsigned int m_uiThreadID;    // 对外暴露的线程ID缓存。

	CEvent m_clEventIdle;         // 空闲事件，暂停或等待工作时置位。
	CEvent m_clEventWork;         // 工作事件，Resume 后允许线程进入初始化和工作循环。
	CEvent m_clEventKill;         // 结束事件，请求线程退出。
	CEvent m_clEventDead;         // 死亡事件，线程清理完成后置位。

	void* m_pvdParam;             // 创建线程时传入的用户参数。
};

END_NAMESPACE_NSDK

#endif  // __xsdk_thread_h__
