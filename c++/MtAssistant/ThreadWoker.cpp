#include "stdafx.h"
#include "ThreadWoker.h"
#include <io.h>
#include "nsdk_atomic.h"
#include "ServiceDataMng.h"
#include "ServiceConfigXml.h"
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

    bool IsCurrentTimeInRange(TimeInfo& p_refTimeInfo, long p_lCurSec)
    {
        int iStartSec = p_refTimeInfo.StartTime.getSec();
        int iEndSec = p_refTimeInfo.EndTime.getSec();
        if (iStartSec <= iEndSec)
            return iStartSec <= p_lCurSec && p_lCurSec <= iEndSec;

        return p_lCurSec >= iStartSec || p_lCurSec <= iEndSec;
    }

    WeekInfo GetCurrentWeekInfo(const tm& p_refNowTime)
    {
        // localtime_s().tm_wday: 0=Sun, 1=Mon, ... 6=Sat.
        if (p_refNowTime.tm_wday == 0)
            return SUN;
        return (WeekInfo)(p_refNowTime.tm_wday - 1);
    }

    int GetRunTimeSeconds(const tagRunTime& p_refTime)
    {
        return (p_refTime.hour * 60 + p_refTime.min) * 60 + p_refTime.sec;
    }

    bool IsSameTimeInfo(const TimeInfo& p_refLeft, const TimeInfo& p_refRight)
    {
        return GetRunTimeSeconds(p_refLeft.StartTime) == GetRunTimeSeconds(p_refRight.StartTime) &&
            GetRunTimeSeconds(p_refLeft.EndTime) == GetRunTimeSeconds(p_refRight.EndTime);
    }

    bool IsSameTimeConf(const map<WeekInfo, vector<TimeInfo> >& p_refLeft, const map<WeekInfo, vector<TimeInfo> >& p_refRight)
    {
        for (int i = 0; i < WEEK_NUM; ++i)
        {
            WeekInfo enWeek = (WeekInfo)i;
            map<WeekInfo, vector<TimeInfo> >::const_iterator itLeft = p_refLeft.find(enWeek);
            map<WeekInfo, vector<TimeInfo> >::const_iterator itRight = p_refRight.find(enWeek);
            size_t nLeftSize = itLeft == p_refLeft.end() ? 0 : itLeft->second.size();
            size_t nRightSize = itRight == p_refRight.end() ? 0 : itRight->second.size();
            if (nLeftSize != nRightSize)
                return false;
            if (nLeftSize == 0)
                continue;

            const vector<TimeInfo>& vecLeft = itLeft->second;
            const vector<TimeInfo>& vecRight = itRight->second;
            for (size_t j = 0; j < vecLeft.size(); ++j)
            {
                if (!IsSameTimeInfo(vecLeft[j], vecRight[j]))
                    return false;
            }
        }
        return true;
    }

    bool IsCurrentProcessServicePath(const string& p_refServicePath)
    {
        char szCurrentPath[4096] = { 0 };
        DWORD dwLen = GetModuleFileNameA(NULL, szCurrentPath, sizeof(szCurrentPath));
        if (dwLen == 0 || dwLen >= sizeof(szCurrentPath))
            return false;

        string strCurrentFullPath;
        string strServiceFullPath;
        if (!NormalizeFullProgramPath(szCurrentPath, strCurrentFullPath))
            return false;
        if (!NormalizeFullProgramPath(p_refServicePath, strServiceFullPath))
            return false;

        return _stricmp(strCurrentFullPath.c_str(), strServiceFullPath.c_str()) == 0;
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
            if (p_refServiceInfo.iRow >= 0)
            {
                // wyl 2026-07-03：绑定到旧进程后立即写回XML配置，后续停止、重启都管理这个旧实例。
                CServiceDataMng::GetInstance()->UpdateRuntimePid(p_refServiceInfo.iRow, p_refServiceInfo.lPid);
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

		for (int i = 0; i < it->second.size(); i++)
		{
			// wyl 2026-07-03：重启完成标记只在真正执行过重启后置位，避免进程已存在时跳过本次时间段。
			mapFinish[i] = false;
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
		//获取当前时间
		time_t now_t = time(NULL);	//保存日期
		tm   now_time;
		localtime_s(&now_time, &now_t);
		long lCurSec = (now_time.tm_hour * 60 + now_time.tm_min) * 60 + now_time.tm_sec;
		int iCurDate = (now_time.tm_year + 1900) * 10000 + (now_time.tm_mon + 1) * 100 + now_time.tm_mday;

		if (m_stServiceInfo.iRow >= 0)
		{
			ServiceInfo stLatestServiceInfo = CServiceDataMng::GetInstance()->GetServiceInfo(m_stServiceInfo.iRow);
			if (!stLatestServiceInfo.strName.empty() && !stLatestServiceInfo.strPath.empty())
			{
				bool bTimeConfChanged = !IsSameTimeConf(m_stServiceInfo.mapTimeConf, stLatestServiceInfo.mapTimeConf);
				m_stServiceInfo = stLatestServiceInfo;
				if (bTimeConfChanged)
				{
					WeekInfo enCurWeek = GetCurrentWeekInfo(now_time);
					auto itCurWeek = m_stServiceInfo.mapTimeConf.find(enCurWeek);
					size_t nCurWeekCount = itCurWeek == m_stServiceInfo.mapTimeConf.end() ? 0 : itCurWeek->second.size();
					MT_INFO("[Worker] name=%s,path=%s,schedule refreshed,week=%s,count=%u",
						m_stServiceInfo.strName.c_str(), m_stServiceInfo.strPath.c_str(), WeekToXmlName(enCurWeek), (unsigned int)nCurWeekCount);
					ResetTimeFlag(lCurSec);
				}
			}
		}

		if (1 == m_stServiceInfo.iEnable)
		{
			bool bStart = true;//是否需要启动, 勾选后的服务在非重启时间也保持监控
			bool bExist = false;//进程是否已经存在
			bool bReStartTime = false;//是否在启动时间重启
			bool bRestartReadyToFinish = false;//本轮重启停止阶段是否已完成，启动成功后再置完成标记

			if (iCurDate > m_iLastDate)//隔天
			{
				CLog::GetInstance()->InitLog();//隔天日志初始化,TODO 隔天多各线程切换,可以考虑优化日志
				MT_INFO("[Worker] name=%s,path=%s,lastDate=%d,today=%d,reset",
					m_stServiceInfo.strName.c_str(), m_stServiceInfo.strPath.c_str(), m_iLastDate, iCurDate);
				m_iLastDate = iCurDate;
				ResetTimeFlag(lCurSec);
			}

			//重启时间
			WeekInfo enWeek = GetCurrentWeekInfo(now_time);
			vector<TimeInfo> &vecTimeInfo = m_stServiceInfo.mapTimeConf[enWeek];
			int iTimePos = -1;//当前时间列位置
			for (int i = 0; i < vecTimeInfo.size(); i++)
			{
				if (IsCurrentTimeInRange(vecTimeInfo[i], lCurSec))//重启时间
				{
					bReStartTime = true;
					iTimePos = i;
					if (!m_mapFinish[enWeek][i])
					{
						m_stServiceInfo.lPid = BindExistingServiceProcess(m_stServiceInfo);
						if (IsCurrentProcessServicePath(m_stServiceInfo.strPath))
						{
							// wyl 2026-07-03：配置误指向助手自身时绝不能关闭/强杀自己，只记录并保持运行状态。
							m_mapFinish[enWeek][i] = true;
							bStart = false;
							MT_WARN("[Worker] skip restart current assistant process, name=%s,path=%s,now=%d,start=%d,end=%d",
								m_stServiceInfo.strName.c_str(), m_stServiceInfo.strPath.c_str(), lCurSec,
								vecTimeInfo[i].StartTime.getSec(), vecTimeInfo[i].EndTime.getSec());
						}
						else
						{
							int iStopRet = 0;
							if (m_stServiceInfo.lPid > 0)
							{
								// wyl 2026-07-03：定时重启直接强制结束进程树，避免优雅等待跨过整个重启窗口。
								iStopRet = StopProcessTree(m_stServiceInfo.lPid,
									m_stServiceInfo.strName.c_str(),
									m_stServiceInfo.strPath.c_str(),
									m_stServiceInfo.strTitle.c_str(),
									0,
									10 * 1000);
							}

							if (0 == iStopRet)
							{
								MT_INFO("[Worker] name=%s,path=%s,now=%d,start=%d,end=%d,stop tree ok",
									m_stServiceInfo.strName.c_str(), m_stServiceInfo.strPath.c_str(), lCurSec,
									vecTimeInfo[i].StartTime.getSec(), vecTimeInfo[i].EndTime.getSec());
								m_stServiceInfo.lPid = -1;
								CServiceDataMng::GetInstance()->UpdateRuntimePid(m_stServiceInfo.iRow, -1);
								bStart = true;
								bRestartReadyToFinish = true;
							}
							else
							{
								MT_WARN("[Worker] name=%s,path=%s,now=%d,start=%d,end=%d,stop tree failed=%d",
									m_stServiceInfo.strName.c_str(), m_stServiceInfo.strPath.c_str(), lCurSec,
									vecTimeInfo[i].StartTime.getSec(), vecTimeInfo[i].EndTime.getSec(), iStopRet);
								bStart = false;
							}
						}
					}
					else
					{
						// wyl 2026-07-03：本时间段已经重启过，后续仍按勾选守护语义精确监控。
						bStart = true;
					}
					break;
				}
			}
			if (!bReStartTime)
			{
				// wyl 2026-07-03：左侧服务已勾选时，非重启窗口也持续监控，崩溃后自动拉起。
				bStart = true;
			}

			//启动进程
			m_stServiceInfo.lPid = BindExistingServiceProcess(m_stServiceInfo);
			// wyl 2026-05-06：如果同路径旧进程已存在，直接接管旧PID，不再启动第二个实例。
			bExist = m_stServiceInfo.lPid > 0;
			if (bStart && !bExist)
			{
				m_bStatus.store(false);
				string strWorkPath = m_stServiceInfo.strPath.substr(0, m_stServiceInfo.strPath.find_last_of(NSDK_PATH_DELIMETER));
				m_stServiceInfo.lPid = StartProcess(strWorkPath.c_str(), m_stServiceInfo.strPath.c_str());

				//成功，失败都同步XML配置
				CServiceDataMng::GetInstance()->UpdateRuntimePid(m_stServiceInfo.iRow, m_stServiceInfo.lPid);

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
					m_bStatus.store(true);
					//启动成功：两种情况：1、守护任何时间拉起 2、在重启时间拉起
					if (bReStartTime && bRestartReadyToFinish && iTimePos >= 0)
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
					if (bReStartTime && bRestartReadyToFinish && iTimePos >= 0)
					{
						m_mapFinish[enWeek][iTimePos] = true;
					}
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
	m_bStatus.store(false);
	m_bThreadWorkFinished = true;
	MT_INFO("[Worker] thread=%lu,name=%s,path=%s,stop",GetCurrentThreadId(), strName.c_str(), strPath.c_str());
}
