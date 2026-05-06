#include "stdafx.h"
#include "ThreadWoker.h"
#include <io.h>

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

void CThreadWork::ResetTimeFlag(long p_lCurSec, ServiceInfo *p_stServiceInfo)
{
	if (p_stServiceInfo)//同步时间配置
	{
		m_stServiceInfo.mapTimeConf = p_stServiceInfo->mapTimeConf;
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
				if (m_stServiceInfo.strName.find("exe") != string::npos ||
					m_stServiceInfo.strName.find("EXE") != string::npos)
				{
					m_stServiceInfo.lPid = FindProcessid(m_stServiceInfo.strName.c_str());//准确获取快照
				}
				else
				{
					m_stServiceInfo.lPid = IsProcessIdExists(m_stServiceInfo.lPid);//用于bat，助手启动的bat，手动启动则无法判断
				}
				mapFinish[i] = m_stServiceInfo.lPid >= 0;
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

	MT_INFO("[守护线程] 线程号=%lu,进程名=%s,路径=%s,开始工作",
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
				MT_INFO("[守护线程] 进程名=%s,路径=%s,昨天=%d,今天=%d,隔天复位",
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
						m_mapFinish[enWeek][i] = (m_stServiceInfo.lPid = IsProcessIdExists(m_stServiceInfo.lPid)) > 0;
						bStart = true;
					}
					else
					{
						bStart = (m_stServiceInfo.lPid = IsProcessIdExists(m_stServiceInfo.lPid)) < 0;
						break;
					}
					iTimePos = i;
					if (0 == KillProcess(m_stServiceInfo.lPid))//结束进程
					{
						//m_stServiceInfo.lPid = -1;
						MT_INFO("[守护线程] 进程名=%s,路径=%s,当前=%d,开始=%d,结束=%d,根据PID关闭进程成功",
							m_stServiceInfo.strName.c_str(), m_stServiceInfo.strPath.c_str(),
							vecTimeInfo[i].StartTime.getSec(), vecTimeInfo[i].EndTime.getSec(), lCurSec);
					}

					//根据名称再次结束进程
					if (!m_stServiceInfo.strTitle.empty())
					{
						//查找窗口 获取pid 再结束进程
						HWND hwnd = ::FindWindow(NULL, m_stServiceInfo.strTitle.c_str());
						if (hwnd)
						{
							DWORD ulProcessId;
							GetWindowThreadProcessId(hwnd, &ulProcessId);
							if (0 == KillProcess(ulProcessId))
							{
								MT_INFO("[守护线程] 进程名=%s,路径=%s,当前=%d,开始=%d,结束=%d,根据进程名已经关闭进程成功",
									m_stServiceInfo.strTitle.c_str(), m_stServiceInfo.strPath.c_str(),
									vecTimeInfo[i].StartTime.getSec(), vecTimeInfo[i].EndTime.getSec(), lCurSec);
							}
							else
							{
								MT_WARN("[守护线程] 进程名=%s,路径=%s,当前=%d,开始=%d,结束=%d,根据进程名已经关闭进程失败",
									m_stServiceInfo.strTitle.c_str(), m_stServiceInfo.strPath.c_str(),
									vecTimeInfo[i].StartTime.getSec(), vecTimeInfo[i].EndTime.getSec(), lCurSec);
							}
						}
						else
						{
							MT_INFO("[守护线程] 进程名=%s,路径=%s,当前=%d,开始=%d,结束=%d,根据PID已经关闭进程成功",
								m_stServiceInfo.strName.c_str(), m_stServiceInfo.strPath.c_str(),
								vecTimeInfo[i].StartTime.getSec(), vecTimeInfo[i].EndTime.getSec(), lCurSec);
						}
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
			bExist = 0 <= (m_stServiceInfo.lPid = IsProcessIdExists(m_stServiceInfo.lPid));
			if (bStart && !bExist)
			{
				m_bStatus.store(false);
				string strWorkPath = m_stServiceInfo.strPath.substr(0, m_stServiceInfo.strPath.find_last_of("\\"));
				m_stServiceInfo.lPid = StartProcess(strWorkPath.c_str(), m_stServiceInfo.strPath.c_str());

				//成功，失败都同步配置
				WritePrivateProfileString(m_stServiceInfo.strLabel.c_str(), "Pid", std::to_string(m_stServiceInfo.lPid).c_str(), m_stServiceInfo.strCfg.c_str());

				if (-2 == m_stServiceInfo.lPid)
				{
					MT_WARN("[守护线程] 进程名=%s,路径=%s,不存在",
						m_stServiceInfo.strName.c_str(), m_stServiceInfo.strPath.c_str());
				}
				else if (-1 == m_stServiceInfo.lPid)
				{
					MT_WARN("[守护线程] 进程名=%s,路径=%s,创建进程失败=%d",
						m_stServiceInfo.strName.c_str(), m_stServiceInfo.strPath.c_str(), GetLastError());
				}
				else
				{
					//启动成功：两种情况：1、守护任何时间拉起 2、在重启时间拉起
					if (bReStartTime)
					{
						m_mapFinish[enWeek][iTimePos] = true;
					}
					MT_INFO("[守护线程] 进程名=%s,路径=%s,进程号=%ld,启动成功",
						m_stServiceInfo.strName.c_str(), m_stServiceInfo.strPath.c_str(), m_stServiceInfo.lPid);
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(2000));//保证进程拉起，下次循环进程能检测到进程
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

		//检测守护进程
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
	m_bThreadWorkFinished = true;
	MT_INFO("[守护线程] 线程号=%lu,进程名=%s,路径=%s,结束工作",GetCurrentThreadId(), strName.c_str(), strPath.c_str());
}