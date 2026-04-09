#include "nsdk_thread.h"

#include <functional>

#if defined(OS_IS_WINDOWS)
#include <process.h>
#endif

namespace
{
#if !defined(OS_IS_WINDOWS)
	unsigned int GetThreadIdValue(pthread_t p_hThread)
	{
		return static_cast<unsigned int>(std::hash<pthread_t>()(p_hThread));
	}
#endif
}

BGN_NAMESPACE_NSDK

#if defined(OS_IS_WINDOWS)
unsigned __stdcall ThreadProxy(void* p_pvdParam)
#else
void* ThreadProxy(void* p_pvdParam)
#endif
{
	CBaseThread* pThread = reinterpret_cast<CBaseThread*>(p_pvdParam);
	if (NULL != pThread)
	{
		pThread->Run();
	}

#if defined(OS_IS_WINDOWS)
	return 0;
#else
	return NULL;
#endif
}

CBaseThread::CBaseThread(void)
{
#if defined(OS_IS_WINDOWS)
	m_hThread = NULL;
#else
	m_hThread = 0;
#endif
	m_uiThreadID = 0;
	m_pvdParam = NULL;
}

CBaseThread::~CBaseThread()
{
	DeleteThread();
}

int CBaseThread::CreateThread(void* p_pvdParam)
{
#if defined(OS_IS_WINDOWS)
	if (NULL != m_hThread)
#else
	if (0 != m_hThread)
#endif
	{
		return NSDK_KO;
	}

	if (NSDK_OK != m_clEventIdle.Create(true, true) ||
		NSDK_OK != m_clEventWork.Create(true, false) ||
		NSDK_OK != m_clEventKill.Create(true, false) ||
		NSDK_OK != m_clEventDead.Create(true, false))
	{
		m_clEventIdle.Close();
		m_clEventWork.Close();
		m_clEventKill.Close();
		m_clEventDead.Close();
		return NSDK_KO;
	}

	m_pvdParam = p_pvdParam;
	m_uiThreadID = 0;
	// 线程创建后默认处于暂停态，只有 Resume 后才会进入初始化和工作循环。

#if defined(OS_IS_WINDOWS)
	unsigned uiThreadID = 0;
	uintptr_t hThread = _beginthreadex(NULL, 0, ThreadProxy, this, 0, &uiThreadID);
	if (0 == hThread)
	{
		m_clEventIdle.Close();
		m_clEventWork.Close();
		m_clEventKill.Close();
		m_clEventDead.Close();
		m_pvdParam = NULL;
		return NSDK_KO;
	}

	m_hThread = reinterpret_cast<HANDLE>(hThread);
	m_uiThreadID = uiThreadID;
#else
	if (0 != pthread_create(&m_hThread, NULL, ThreadProxy, this))
	{
		m_hThread = 0;
		m_clEventIdle.Close();
		m_clEventWork.Close();
		m_clEventKill.Close();
		m_clEventDead.Close();
		m_pvdParam = NULL;
		return NSDK_KO;
	}

	m_uiThreadID = GetThreadIdValue(m_hThread);
#endif

	return NSDK_OK;
}

int CBaseThread::DeleteThread(void)
{
#if defined(OS_IS_WINDOWS)
	if (NULL == m_hThread)
#else
	if (0 == m_hThread)
#endif
	{
		return NSDK_OK;
	}

#if defined(OS_IS_WINDOWS)
	if (::GetCurrentThreadId() == m_uiThreadID)
#else
	if (pthread_equal(pthread_self(), m_hThread))
#endif
	{
		// 避免线程自己等待自己退出；这里只发出退出请求，由外部拥有者稍后回收线程对象。
		m_clEventKill.SetEvent();
		m_clEventWork.SetEvent();
		return NSDK_KO;
	}

	m_clEventKill.SetEvent();
	m_clEventWork.SetEvent();
	// 如果线程当前处于暂停等待状态，这里顺带唤醒它去感知退出请求。
	m_clEventDead.WaitEvent(INFINITE);

#if defined(OS_IS_WINDOWS)
	::WaitForSingleObject(m_hThread, INFINITE);
	::CloseHandle(m_hThread);
	m_hThread = NULL;
#else
	pthread_join(m_hThread, NULL);
	m_hThread = 0;
#endif

	m_uiThreadID = 0;
	m_pvdParam = NULL;

	m_clEventIdle.Close();
	m_clEventWork.Close();
	m_clEventKill.Close();
	m_clEventDead.Close();

	return NSDK_OK;
}

int CBaseThread::Run(void)
{
	bool bInitialized = false;

	while (WAIT_OBJECT_0 != m_clEventKill.WaitEvent(0))
	{
		// 没有收到 Resume 前一直维持空闲态，不提前执行 Initialize。
		int iWaitRet = m_clEventWork.WaitEvent(100);
		if (WAIT_OBJECT_0 != iWaitRet)
		{
			m_clEventIdle.SetEvent();
			if (WAIT_TIMEOUT == iWaitRet)
			{
				continue;
			}

			break;
		}

		if (WAIT_OBJECT_0 == m_clEventKill.WaitEvent(0))
		{
			break;
		}

		if (!bInitialized)
		{
			m_clEventIdle.ResetEvent();

			// 延迟到首次 Resume 后再初始化，和旧版 Thread.cpp 的语义保持一致。
			int iInitRet = Initialize();
			if (NSDK_OK != iInitRet)
			{
				m_clEventIdle.SetEvent();
				m_clEventDead.SetEvent();
				return iInitRet;
			}

			bInitialized = true;
		}

		m_clEventIdle.ResetEvent();
		if (NSDK_OK != Work())
		{
			break;
		}
	}

	m_clEventIdle.SetEvent();
	if (bInitialized)
	{
		Uninitialize();
	}
	m_clEventDead.SetEvent();
	return NSDK_OK;
}

int CBaseThread::Pause(void)
{
#if defined(OS_IS_WINDOWS)
	if (NULL == m_hThread)
#else
	if (0 == m_hThread)
#endif
	{
		return NSDK_KO;
	}

#if defined(OS_IS_WINDOWS)
	if (::GetCurrentThreadId() == m_uiThreadID)
#else
	if (pthread_equal(pthread_self(), m_hThread))
#endif
	{
		// 线程自身无法同步等待自己进入 idle，直接拒绝这种用法以避免自锁。
		return NSDK_KO;
	}

	m_clEventWork.ResetEvent();

	int iWaitRet = m_clEventIdle.WaitEvent(INFINITE);
	if (WAIT_OBJECT_0 != iWaitRet)
	{
		return NSDK_KO;
	}

	return OnPause();
}

int CBaseThread::Resume(void)
{
#if defined(OS_IS_WINDOWS)
	if (NULL == m_hThread)
#else
	if (0 == m_hThread)
#endif
	{
		return NSDK_KO;
	}

	int iRet = OnResume();
	if (NSDK_OK != iRet)
	{
		return iRet;
	}

	return m_clEventWork.SetEvent();
}

bool CBaseThread::IsRunning(void)
{
#if defined(OS_IS_WINDOWS)
	if (NULL == m_hThread)
#else
	if (0 == m_hThread)
#endif
	{
		return false;
	}

	return (WAIT_OBJECT_0 != m_clEventDead.WaitEvent(0)) &&
		(WAIT_OBJECT_0 != m_clEventIdle.WaitEvent(0));
}

END_NAMESPACE_NSDK
