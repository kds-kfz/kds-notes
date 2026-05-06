#include "stdafx.h"
#include "ThreadWoker.h"
#include <io.h>
namespace
{
    // wyl 2026-05-06：bat运行后实际进程通常是cmd，无法按脚本完整路径反查，只能保留旧PID兜底。
    bool IsBatchServiceName(const string& p_refName)
    {
        if (p_refName.length() < 4)
            return false;

        string strExt = p_refName.substr(p_refName.length() - 4);
        return _stricmp(strExt.c_str(), ".bat") == 0;
    }
    // wyl 2026-05-06：启动前优先按完整路径绑定已存在服务，避免同一路径重复拉起新进程。
    long BindExistingServiceProcess(ServiceInfo& p_refServiceInfo)
    {
        long lPid = FindProcessIdByPath(p_refServiceInfo.strName.c_str(), p_refServiceInfo.strPath.c_str());
        if (lPid <= 0 && IsBatchServiceName(p_refServiceInfo.strName))
            lPid = IsProcessIdExists(p_refServiceInfo.lPid);

        if (lPid <= 0)
        {
            p_refServiceInfo.lPid = -1;
            return -1;
        }

        if (p_refServiceInfo.lPid != lPid)
        {
            p_refServiceInfo.lPid = lPid;
            if (!p_refServiceInfo.strCfg.empty() && !p_refServiceInfo.strLabel.empty())
            {
                // wyl 2026-05-06：绑定到旧进程后立即写回配置，后续停止、重启都管理这个旧实例。
                WritePrivateProfileString(p_refServiceInfo.strLabel.c_str(), "Pid", std::to_string(p_refServiceInfo.lPid).c_str(), p_refServiceInfo.strCfg.c_str());
            }
            MT_INFO("[Worker] name=%s,path=%s,pid=%ld,service already started",
                p_refServiceInfo.strName.c_str(), p_refServiceInfo.strPath.c_str(), p_refServiceInfo.lPid);
        }

        return p_refServiceInfo.lPid;
    }
}
CThreadWork::CThreadWork(ServiceInfo *p_stServiceInfo) {
	m_bExit.store(false);
	m_bStatus.store(false);
	m_bThreadWorkFinished.store(false);
	m_stServiceInfo = *p_stServiceInfo;
	//获取当前时间
	time_t now_t = time(NULL);	//保存日期
	tm   now_time;
	localtime_s(&now_time, &now_t);
	long lCurSec = (now_time.tm_hour * 60 + now_time.tm_min) * 60 + now_time.tm_sec;
	m_iLastDate = (now_time.tm_year + 1900) * 10000 + (now_time.tm_mon + 1) * 100 + now_time.tm_mday;

	ResetTimeFlag(lCurSec);
	
	std::thread(std::bind(&CThreadWork::Work, this)).detach();
};

// wyl 2026-05-06：分段等待退出信号，避免线程长时间睡眠导致关闭助手反应慢。
bool CThreadWork::WaitForExit(int p_iMilliseconds)
{
	int iWaited = 0;
	while (iWaited < p_iMilliseconds)
	{
		if (m_bExit.load())
			return true;

		int iSleep = p_iMilliseconds - iWaited;
		if (iSleep > 50)
			iSleep = 50;

		std::this_thread::sleep_for(std::chrono::milliseconds(iSleep));
		iWaited += iSleep;
	}

	return m_bExit.load();
}
void CThreadWork::ResetTimeFlag(long p_lCurSec, ServiceInfo *p_stServiceInfo)
{
	if (p_stServiceInfo)
	{
		// wyl 2026-05-06：同步最新服务配置，包含启用状态、PID和重启时间表。
		m_stServiceInfo = *p_stServiceInfo;
	}

	m_mapFinish.clear();

	for (auto it = m_stServiceInfo.mapTimeConf.begin(); it != m_stServiceInfo.mapTimeConf.end(); it++)
	{
		map<int, bool> &mapFinish = m_mapFinish[it->first];

		for (int i = 0; i < it->second.size(); i++)//首次启动在时间范围内检测进程是否存在
		{
			if (it->second[i].StartTime.getSec() <= p_lCurSec &&
				p_lCurSec >= it->second[i].EndTime.getSec())
			{
				m_stServiceInfo.lPid = BindExistingServiceProcess(m_stServiceInfo);
				mapFinish[i] = m_stServiceInfo.lPid > 0;
			}
			else
			{
				mapFinish[i] = false;
			}
		}
	}
}

void CThreadWork::UpdateTime(ServiceInfo *p_stServiceInfo)
{
	// wyl 2026-05-06：界面添加或删除重启时间后，通过这里同步到守护线程。
	//获取当前时间
	time_t now_t = time(NULL);	//保存日期
	tm   now_time;
	localtime_s(&now_time, &now_t);
	long lCurSec = (now_time.tm_hour * 60 + now_time.tm_min) * 60 + now_time.tm_sec;

	m_WorkLock.lock();
	ResetTimeFlag(lCurSec, p_stServiceInfo);
	m_WorkLock.unlock();
}

void CThreadWork::Work()
{
	//m_bThreadWorkFinished = false;
	string strName = m_stServiceInfo.strName;
	string strPath = m_stServiceInfo.strPath;

	MT_INFO("[Worker] thread=%lu,name=%s,path=%s,start",
		GetCurrentThreadId(), strName.c_str(), strPath.c_str());
	while (true)
	{
		if (m_bExit)
			break;

		m_WorkLock.lock();
		if (1 == m_stServiceInfo.iEnable)
		{
			bool bStart = true;//是否需要启动, 没有配置时间的保持24小时运行
			bool bExist = false;//进程是否已经存在
			bool bReStartTime = false;//是否在启动时间重启

			//获取当前时间
			time_t now_t = time(NULL);	//保存日期
			tm   now_time;
			localtime_s(&now_time, &now_t);
			long lCurSec = (now_time.tm_hour * 60 + now_time.tm_min) * 60 + now_time.tm_sec;
			int iCurDate = (now_time.tm_year + 1900) * 10000 + (now_time.tm_mon + 1) * 100 + now_time.tm_mday;
			if (iCurDate > m_iLastDate)//隔天
			{
				CLog::GetInstance()->InitLog();//隔天日志初始化,TODO 隔天多各线程切换,可以考虑优化日志
				MT_INFO("[Worker] name=%s,path=%s,lastDate=%d,today=%d,reset",
					m_stServiceInfo.strName.c_str(), m_stServiceInfo.strPath.c_str(), m_iLastDate, iCurDate);
				m_iLastDate = iCurDate;
				ResetTimeFlag(lCurSec);
			}

			//重启时间
			CTime t(now_time.tm_year + 1900, now_time.tm_mon + 1, now_time.tm_mday, 0, 0, 0);
			int idx = t.GetDayOfWeek() - 1;
			//转换成星期行
			idx = idx == 0 ? 6 : idx - 1;
			WeekInfo enWeek = (WeekInfo)idx;
			vector<TimeInfo> &vecTimeInfo = m_stServiceInfo.mapTimeConf[enWeek];
			int iTimePos = 0;//当前时间列位置
			for (int i = 0; i < vecTimeInfo.size(); i++)
			{
				int lStartSec = vecTimeInfo[i].StartTime.getSec();
				int lEndSec = vecTimeInfo[i].EndTime.getSec();
				if (lStartSec <= lCurSec && lEndSec >= lCurSec)//重启时间
				{
					bReStartTime = true;
					if (!m_mapFinish[enWeek][i])
					{
						m_stServiceInfo.lPid = BindExistingServiceProcess(m_stServiceInfo);
						m_mapFinish[enWeek][i] = m_stServiceInfo.lPid > 0;
						bStart = true;
					}
					else
					{
						m_stServiceInfo.lPid = BindExistingServiceProcess(m_stServiceInfo);
						bStart = m_stServiceInfo.lPid <= 0;
						break;
					}
					iTimePos = i;
					// wyl 2026-05-06：重启时间命中时停止完整进程树，最长等待5分钟优雅退出。
					int iStopRet = StopProcessTree(m_stServiceInfo.lPid,
						m_stServiceInfo.strName.c_str(),
						m_stServiceInfo.strPath.c_str(),
						m_stServiceInfo.strTitle.c_str(),
						5 * 60 * 1000,
						10 * 1000);
					if (0 == iStopRet)
					{
						MT_INFO("[Worker] name=%s,path=%s,now=%d,start=%d,end=%d,stop tree ok",
							m_stServiceInfo.strName.c_str(), m_stServiceInfo.strPath.c_str(),
							vecTimeInfo[i].StartTime.getSec(), vecTimeInfo[i].EndTime.getSec(), lCurSec);
					}
					else
					{
						MT_WARN("[Worker] name=%s,path=%s,now=%d,start=%d,end=%d,stop tree failed=%d",
							m_stServiceInfo.strName.c_str(), m_stServiceInfo.strPath.c_str(),
							vecTimeInfo[i].StartTime.getSec(), vecTimeInfo[i].EndTime.getSec(), lCurSec, iStopRet);
					}
					m_stServiceInfo.lPid = -1;
					bStart = true;
					break;
				}
				else
				{
					bStart = 1 == m_stServiceInfo.iCheck;
				}
			}

			//启动进程
			m_stServiceInfo.lPid = BindExistingServiceProcess(m_stServiceInfo);
			// wyl 2026-05-06：如果同路径旧进程已存在，直接接管旧PID，不再启动第二个实例。
			bExist = m_stServiceInfo.lPid > 0;
			if (bStart && !bExist)
			{
				m_bStatus.store(false);
				string strWorkPath = m_stServiceInfo.strPath.substr(0, m_stServiceInfo.strPath.find_last_of("\\"));
				m_stServiceInfo.lPid = StartProcess(strWorkPath.c_str(), m_stServiceInfo.strPath.c_str());

				//成功，失败都同步配置
				WritePrivateProfileString(m_stServiceInfo.strLabel.c_str(), "Pid", std::to_string(m_stServiceInfo.lPid).c_str(), m_stServiceInfo.strCfg.c_str());

				if (-2 == m_stServiceInfo.lPid)
				{
					MT_WARN("[Worker] name=%s,path=%s,file missing",
						m_stServiceInfo.strName.c_str(), m_stServiceInfo.strPath.c_str());
				}
				else if (-1 == m_stServiceInfo.lPid)
				{
					MT_WARN("[Worker] name=%s,path=%s,start failed=%d",
						m_stServiceInfo.strName.c_str(), m_stServiceInfo.strPath.c_str(), GetLastError());
				}
				else
				{
					//启动成功：两种情况：1、守护任何时间拉起 2、在重启时间拉起
					if (bReStartTime)
					{
						m_mapFinish[enWeek][iTimePos] = true;
					}
					MT_INFO("[Worker] name=%s,path=%s,pid=%ld,start ok",
						m_stServiceInfo.strName.c_str(), m_stServiceInfo.strPath.c_str(), m_stServiceInfo.lPid);
				}
				if (WaitForExit(2000))// wyl 2026-05-06：等待新进程稳定，同时允许助手退出时快速打断。
					break;
			}
			else
			{
				if (bExist)
				{
					m_bStatus.store(true);
				}
				else
				{
					m_bStatus.store(false);
				}
			}
		}
		m_WorkLock.unlock();

		// wyl 2026-05-06：守护循环间隔等待同样可被退出信号打断。
		if (WaitForExit(1000))
			break;
	}
	m_bThreadWorkFinished = true;
	MT_INFO("[Worker] thread=%lu,name=%s,path=%s,stop",GetCurrentThreadId(), strName.c_str(), strPath.c_str());
}
