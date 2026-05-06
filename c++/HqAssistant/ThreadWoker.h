#ifndef __THREAD_WORKEWR_H__
#define __THREAD_WORKEWR_H__

#include <iostream>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <functional>
#include <atomic>

#include "Struct.h"
#include<thread>
#include<mutex>
using namespace std;

#include "Log.h"

class CThreadWork
{
private:
	std::atomic<bool> m_bStatus;
	std::map<WeekInfo, map<int,bool>> m_mapFinish;//每个时间段是否已启动成功
	std::mutex m_WorkLock;//工作线程锁
	int m_iLastDate;//上次启动日期,隔天各启动时间段标志位复位
private:
	void ResetTimeFlag(long p_lCurSec, ServiceInfo *p_stServiceInfo = NULL);//复位各时间段启动时间标志
public:
	std::atomic<bool> m_bExit;
	std::atomic<bool> m_bThreadWorkFinished;
	ServiceInfo m_stServiceInfo;
	CThreadWork(ServiceInfo *p_stServiceInfo);
	void UpdateTime(ServiceInfo *p_stServiceInfo);
	void Work();
	~CThreadWork() {
		m_bExit.store(true);
	};
	bool GetStatus() {
		return m_bStatus;
	}
};

#endif
