#include "Thread.h"
#include <iostream>
#include <sstream>

USE_NAMESPACE_NSDK

using namespace std;

void *ThreadHandle(void *param)
{
	CThread* pThis = (CThread*)param;
	pThis->Run();
	return (void*)1;
}

CThread::CThread():
	m_ptrThread(nullptr),
	m_bPauseFlag(true),
	m_bStopFlag(false),
	m_enState(Stoped),
	m_bFinishFlag(false)
{
}

CThread::~CThread()
{
	Stop();
}

CThread::ThreadState CThread::State() const
{
	return m_enState;
}

int CThread::Start(unsigned int p_uiId)
{
	int iRet = -1;
	if (m_ptrThread == nullptr)
	{
		pthread_mutex_init(&m_mutex, nullptr);
		pthread_cond_init(&m_condition, nullptr);

		m_ptrThread = new pthread_t;
		pthread_create(m_ptrThread, nullptr, ThreadHandle, this);
		m_bPauseFlag = true;
		m_bStopFlag = false;
		m_enState = Paused;
		m_bFinishFlag = false;
		iRet = 0;
		//m_lluThreadId = ThreadId2uLong(std::this_thread::get_id());
		//pthread_t threadId = pthread_self(); // 获取当前线程的pthread_t结构
		//unsigned long ulThreadId = pthread_getw32threadid_np(threadId); // 获取Windows线程ID
		//m_ulThreadId = ThreadId2uLong(ulThreadId);
		m_ulThreadId = p_uiId;
	}
	return iRet;
}

int CThread::Stop()
{
	int iRet = -1;
	if (m_ptrThread != nullptr)
	{
		m_bPauseFlag = false;
		m_bStopFlag = true;
		pthread_cond_signal(&m_condition);  // Notify one waiting thread, if there is one.
		pthread_join(*m_ptrThread,nullptr); // wait for thread finished
		delete m_ptrThread;
		m_ptrThread = nullptr;
		m_enState = Stoped;
		iRet = 0;

		pthread_mutex_destroy(&m_mutex);
		pthread_cond_destroy(&m_condition);
	}
	return iRet;
}

int CThread::Pause()
{
	int iRet = -1;
	if (m_ptrThread != nullptr)
	{
		m_bPauseFlag = true;
		m_enState = Paused;
		iRet = 0;
	}
	return iRet;
}

int CThread::Resume()
{
	int iRet = -1;
	if (m_ptrThread != nullptr)
	{
		m_bPauseFlag = false;
		//pthread_mutex_lock(&m_mutex);
		pthread_cond_signal(&m_condition);
		m_enState = Running;
		//pthread_mutex_unlock(&m_mutex);
		iRet = 0;
	}
	return iRet;
}

void CThread::Run()
{
	while (!m_bStopFlag)
	{
		
		if (m_bPauseFlag)
		{
			pthread_mutex_lock(&m_mutex);
			while (m_bPauseFlag)
			{
				 pthread_cond_wait(&m_condition, &m_mutex); // Unlock m_mutex and wait to be notified
			}
			pthread_mutex_unlock(&m_mutex);
		}
		Work();
	}
	m_bPauseFlag = false;
	m_bStopFlag = false;
}

unsigned long long CThread::ThreadId2uLong(unsigned long p_ulThreadId)
{
	std::ostringstream oss;
	oss << p_ulThreadId;
	std::string stid = oss.str();
	return std::stoull(stid);
}

