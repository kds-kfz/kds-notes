#ifndef _THREAD_H_
#define _THREAD_H_

#include "nsdk_define.h"

#include <pthread.h>
//#include <atomic>
//#include <mutex>
#include <string>
//#include <condition_variable>


//通知类型
enum NotifyType
{
	NT_FINISH = 1,	// 完成
	NT_COLLECT		// 采集
};

class CDataNotify
{
public:
	// 通知回调接口
	virtual void OnDataNotify(unsigned long long p_lluThreadId, NotifyType p_enType, void *p_pData, int p_iDataLen) = 0;
};

class CThread
{
public:
	CThread();
	virtual ~CThread();

	enum ThreadState
	{
		Stoped,     ///<停止状态，包括从未启动过和启动后被停止
		Running,    ///<运行状态
		Paused      ///<暂停状态
	};

	ThreadState State() const;

	void Run();
	int Start(unsigned int p_uiId);
	int Stop();
	int Pause();
	int Resume();
	bool GetFinish() { return m_bFinishFlag; };
	unsigned long long GetThreadId() { return m_ulThreadId; }



protected:
	virtual int Work() = 0;
	virtual bool AddNotify(CDataNotify* p_pNotify) = 0;
	virtual bool DelNotify(CDataNotify* p_pNotify) = 0;
	virtual void SendNotify(unsigned long long p_lluThreadId, NotifyType p_enNotifyType, void *p_pData, int p_iDataLen) = 0;//通知异常数据位置

	bool m_bFinishFlag;   ///<完成标识
	unsigned int m_ulThreadId;//线程号

private:
	unsigned long long ThreadId2uLong(unsigned long p_ulThreadId);

private:
	pthread_t *m_ptrThread;
	pthread_mutex_t m_mutex;
	pthread_cond_t m_condition;
	bool m_bPauseFlag;   ///<暂停标识
	bool m_bStopFlag;   ///<停止标识
	ThreadState m_enState;
};

#endif // THREAD_H

