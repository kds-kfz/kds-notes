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
	std::map<WeekInfo, map<int,bool>> m_mapFinish;// wyl 2026-05-06：记录每个重启时间段是否已执行，避免同一时间段重复重启。
	std::mutex m_WorkLock;// wyl 2026-05-06：保护守护线程读取和更新服务配置。
	int m_iLastDate;// wyl 2026-05-06：记录上次工作日期，跨天后复位重启时间段标志。
private:
	void ResetTimeFlag(long p_lCurSec, ServiceInfo *p_stServiceInfo = NULL);//复位各时间段启动时间标志
	bool WaitForExit(int p_iMilliseconds);// wyl 2026-05-06：支持可中断等待，避免守护线程退出时卡住。
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
