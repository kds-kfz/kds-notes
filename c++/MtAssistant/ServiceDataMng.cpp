#include "stdafx.h"
#include "ServiceDataMng.h"
#include <io.h>

#include "nsdk.h"
#include "nsdk_atomic.h"

CServiceDataMng *CServiceDataMng::m_pThis = NULL;
extern int g_iBreakpadInitRet;

CServiceDataMng *CServiceDataMng::GetInstance()
{
	if (NULL == m_pThis)
	{
		m_pThis = new CServiceDataMng;
	}
	return m_pThis;
}

void CServiceDataMng::Release()
{
	if (NULL == m_pThis)
		return;

	delete m_pThis;
	m_pThis = NULL;
}

CServiceDataMng::CServiceDataMng() : m_iCount(0)
{

}

CServiceDataMng::~CServiceDataMng()
{
	//销毁所有守护线程
	m_ServiceInfoLock.lock();
	for (auto it = m_mapServiceInfo.begin(); it != m_mapServiceInfo.end(); it++)
	{
		DestroyDaemon(it->first);
	}
	m_ServiceInfoLock.unlock();
}

void CServiceDataMng::Init()
{
	ReadCfg();

	//测试崩溃
	//*(int*)0 = 0;

	std::map<int, ServiceInfo> mapServiceInfo;
	m_ServiceInfoLock.lock();
	mapServiceInfo = m_mapServiceInfo;
	m_ServiceInfoLock.unlock();

	for (auto it = mapServiceInfo.begin(); it != mapServiceInfo.end(); it++)
	{
		if (1 == it->second.iEnable)
		{
			// wyl 2026-05-06：助手启动时只恢复已勾选启用服务的守护线程。
			CreateDaemon(it->first, &it->second);
		}
	}
}


// wyl 2026-05-06：保留交易终端固定窗口标题，用于停止流程补充定位目标进程。
string GetTitle(char *p_szName)
{
	if (0 == strcmp("TradingTerminal.exe", p_szName))
		return "TradingTerminal.exe";
	
	return "";
}

namespace
{
	const int MAX_SERVICE_CFG_COUNT = 256;// wyl 2026-05-06：限制服务配置数量，避免异常ini导致循环和内存压力失控。
	const int MAX_TIME_CFG_COUNT = 128;// wyl 2026-05-06：限制单服务时间段数量，避免异常配置拖慢界面和守护线程。

	// wyl 2026-05-06：bat运行后实际进程通常是cmd，无法按脚本完整路径反查，只能使用旧PID兜底。
	bool IsBatchServiceName(const string& p_refName)
	{
		if (p_refName.length() < 4)
			return false;

		string strExt = p_refName.substr(p_refName.length() - 4);
		return _stricmp(strExt.c_str(), ".bat") == 0;
	}

	// wyl 2026-05-06：探测服务是否已经运行，exe按完整路径精确匹配，bat保留旧PID兜底。
	long ProbeRunningServicePid(const ServiceInfo& p_refServiceInfo)
	{
		long lPid = FindProcessIdByPath(p_refServiceInfo.strName.c_str(), p_refServiceInfo.strPath.c_str());
		if (lPid <= 0 && IsBatchServiceName(p_refServiceInfo.strName))
			lPid = IsProcessIdExists(p_refServiceInfo.lPid);

		return lPid;
	}

	// wyl 2026-05-06：创建守护线程前校验服务名称和完整路径，只接受exe/bat。
	bool IsValidServiceInfo(const ServiceInfo& p_refServiceInfo)
	{
		if (p_refServiceInfo.strName.length() < 4 || p_refServiceInfo.strPath.empty())
			return false;

		string strExt = p_refServiceInfo.strName.substr(p_refServiceInfo.strName.length() - 4);
		return _stricmp(strExt.c_str(), ".exe") == 0 || _stricmp(strExt.c_str(), ".bat") == 0;
	}

	// wyl 2026-05-06：校验ini中的数量字段，负数按0处理，超上限则截断。
	int NormalizeCfgCount(const char *p_szSection, const char *p_szKey, int p_iValue, int p_iMaxValue)
	{
		if (p_iValue < 0)
		{
			MT_WARN("[MtAssistant] invalid config %s/%s=%d, use 0", p_szSection, p_szKey, p_iValue);
			return 0;
		}
		if (p_iValue > p_iMaxValue)
		{
			MT_WARN("[MtAssistant] config %s/%s=%d exceeds max %d, clamped", p_szSection, p_szKey, p_iValue, p_iMaxValue);
			return p_iMaxValue;
		}
		return p_iValue;
	}

	// wyl 2026-05-06：严格解析HH:mm:ss，避免非法时间进入守护调度。
	bool ParseRunTime(const char *p_szValue, tagRunTime &p_refTime)
	{
		if (p_szValue == NULL)
			return false;

		int hour = -1, min = -1, sec = -1, pos = 0;
		if (sscanf_s(p_szValue, " %d:%d:%d %n", &hour, &min, &sec, &pos) != 3 || p_szValue[pos] != '\0')
			return false;

		if (hour < 0 || hour > 23 || min < 0 || min > 59 || sec < 0 || sec > 59)
			return false;

		p_refTime.hour = hour;
		p_refTime.min = min;
		p_refTime.sec = sec;
		return true;
	}
}

void CServiceDataMng::ReadCfg()
{
	m_strHome = nsdk::GetRootPath();

	string strLogFolder = m_strHome;
	strLogFolder.append(NSDK_PATH_DELIMETER "run" NSDK_PATH_DELIMETER "log");
	// wyl 2026-05-06：这里只传日志目录，完整日志文件路径统一由CLog::m_strLogPath维护。
	int iRet = CLog::GetInstance()->InitLog(strLogFolder.c_str());
	if (MA_OK != iRet)
	{
		return;
	}

	//记得恢复工作线程
	CLog::GetInstance()->Resume();

	MT_INFO("Log started ****************************");
	MT_INFO("[MtAssistant] InitBreakpad ret=%d", g_iBreakpadInitRet);

	m_strCfg = m_strHome;
	m_strCfg.append(NSDK_PATH_DELIMETER);
	m_strCfg.append(SERVICE_CFG_NAME);

	if (_access(m_strCfg.c_str(), 0) == -1)
	{
		MT_WARN("[MtAssistant] config file missing, path=%s", m_strCfg.c_str());
		return;
	}

	// 设置日志级别
	char strLogLevel[64] = { 0 };
	GetPrivateProfileString("LOG", "LogLevel", "info", strLogLevel, sizeof(strLogLevel), m_strCfg.c_str());
	MT_INFO("[MtAssistant] log level: %s", strLogLevel);
	CLog::GetInstance()->SetLogLevel(strLogLevel);

	char szTemp[_MAX_PATH] = { 0 };

	MT_INFO("[MtAssistant] load config start");
	m_iCount = NormalizeCfgCount("Info", "Count", GetPrivateProfileInt("Info", "Count", 0, m_strCfg.c_str()), MAX_SERVICE_CFG_COUNT);

	char strLabel[100] = { 0 };
	char strStartTime[100] = { 0 };
	char strEndTime[100] = { 0 };
	char strTimeCount[100] = { 0 };

	for (int i = 0; i < m_iCount; i++)
	{
		ServiceInfo pstServiceInfo;

		sprintf(strLabel, "Info_%d", i);
		GetPrivateProfileString(strLabel, "Name", "", szTemp, _MAX_PATH, m_strCfg.c_str());
		pstServiceInfo.strLabel = strLabel;
		pstServiceInfo.strName = szTemp;

		//ini中文读不了
		//GetPrivateProfileString(strLabel, "Title", "", szTemp, _MAX_PATH, m_strCfg.c_str());
		//pstServiceInfo.strTitle = szTemp;

		pstServiceInfo.strTitle = GetTitle(szTemp);

		GetPrivateProfileString(strLabel, "Path", "", szTemp, _MAX_PATH, m_strCfg.c_str());
		pstServiceInfo.strPath = szTemp;

		pstServiceInfo.iEnable = GetPrivateProfileInt(strLabel, "Enable", 0, m_strCfg.c_str());
		pstServiceInfo.lPid = GetPrivateProfileInt(strLabel, "Pid", -1, m_strCfg.c_str());
		pstServiceInfo.iCheck = GetPrivateProfileInt(strLabel, "Check", 0, m_strCfg.c_str());
		pstServiceInfo.strCfg = m_strCfg;
		pstServiceInfo.strLabel = strLabel;

		for (int j = 0; j < WEEK_NUM; j++)
		{
			WeekInfo enWeek = j == 0 ? MON : j == 1 ? TUE : j == 2 ? WED : j == 3 ? THU : j == 4 ? FRI : j == 5 ? SAT : j == 6 ? SUN : MON;
			vector<TimeInfo> &listTimeInfo = pstServiceInfo.mapTimeConf[enWeek];
			
			sprintf(strTimeCount, "TimeCount_%d", j);
			int iTimeCount = NormalizeCfgCount(strLabel, strTimeCount, GetPrivateProfileInt(strLabel, strTimeCount, 0, m_strCfg.c_str()), MAX_TIME_CFG_COUNT);

			for (int k = 0; k < iTimeCount; k++)//时间下标
			{
				TimeInfo stTimeInfo;
				tagRunTime stStartTime;
				tagRunTime stEndTime;

				sprintf(strStartTime, "StartTime_%d_%d", j, k);
				GetPrivateProfileString(strLabel, strStartTime, "0:0:0", szTemp, _MAX_PATH, m_strCfg.c_str());
				if (!ParseRunTime(szTemp, stStartTime))
				{
					MT_WARN("[MtAssistant] invalid config %s/%s=%s, skip schedule", strLabel, strStartTime, szTemp);
					continue;
				}
				stTimeInfo.StartTime = stStartTime;

				sprintf(strEndTime, "EndTime_%d_%d", j, k);
				GetPrivateProfileString(strLabel, strEndTime, "0:0:0", szTemp, _MAX_PATH, m_strCfg.c_str());
				if (!ParseRunTime(szTemp, stEndTime))
				{
					MT_WARN("[MtAssistant] invalid config %s/%s=%s, skip schedule", strLabel, strEndTime, szTemp);
					continue;
				}
				stTimeInfo.EndTime = stEndTime;

				listTimeInfo.push_back(stTimeInfo);
			}
		}

		m_ServiceInfoLock.lock();
		m_mapServiceInfo[i] = pstServiceInfo;
		m_ServiceInfoLock.unlock();
	}

	MT_INFO("[MtAssistant] load config done");
}

ServiceInfo CServiceDataMng::GetServiceInfo(int p_iServiceRow)
{
	ServiceInfo stServiceInfo;
	m_ServiceInfoLock.lock();
	if (m_mapServiceInfo.find(p_iServiceRow) != m_mapServiceInfo.end())
	{
		stServiceInfo = m_mapServiceInfo[p_iServiceRow];
	}
	m_ServiceInfoLock.unlock();
	return stServiceInfo;
}

int CServiceDataMng::GetAllServiceInfo(std::map<int, ServiceInfo> &p_mapServiceInfo)
{
	p_mapServiceInfo.clear();
	m_ServiceInfoLock.lock();
	p_mapServiceInfo = m_mapServiceInfo;
	m_ServiceInfoLock.unlock();

	for (auto it = p_mapServiceInfo.begin(); it != p_mapServiceInfo.end(); it++)
	{
		it->second.lPid = GetPrivateProfileInt(it->second.strLabel.c_str(), "Pid", -1, m_strCfg.c_str());
	}

	return p_mapServiceInfo.size();
}

// wyl 2026-05-06：每次重新生成状态表，并按完整路径探测真实进程，避免旧服务已运行但界面显示停止。
void CServiceDataMng::GetAllServiceStatus(std::map<int, bool> &p_mapServiceStatus)
{
	p_mapServiceStatus.clear();

	std::map<int, ServiceInfo> mapServiceInfo;
	m_ServiceInfoLock.lock();
	mapServiceInfo = m_mapServiceInfo;
	m_ServiceInfoLock.unlock();

	std::map<int, bool> mapDaemonStatus;
	m_DaemonLock.lock();
	for (auto it = m_mapDaemon.begin(); it != m_mapDaemon.end(); it++)
	{
		mapDaemonStatus[it->first] = it->second->GetStatus();
	}
	m_DaemonLock.unlock();

	for (auto it = mapServiceInfo.begin(); it != mapServiceInfo.end(); it++)
	{
		ServiceInfo &refServiceInfo = it->second;
		long lPid = ProbeRunningServicePid(refServiceInfo);
		bool bRunning = lPid > 0;

		auto itDaemon = mapDaemonStatus.find(it->first);
		if (itDaemon != mapDaemonStatus.end())
			bRunning = bRunning || itDaemon->second;

		if (lPid > 0 && refServiceInfo.lPid != lPid)
		{
			// wyl 2026-05-06：状态刷新发现旧服务已运行时，立即写回PID并打印已启动日志。
			WritePrivateProfileString(refServiceInfo.strLabel.c_str(), "Pid", std::to_string(lPid).c_str(), m_strCfg.c_str());
			m_ServiceInfoLock.lock();
			if (m_mapServiceInfo.find(it->first) != m_mapServiceInfo.end())
				m_mapServiceInfo[it->first].lPid = lPid;
			m_ServiceInfoLock.unlock();

			MT_INFO("[MtAssistant] service already started, name=%s,path=%s,pid=%ld",
				refServiceInfo.strName.c_str(), refServiceInfo.strPath.c_str(), lPid);
		}

		p_mapServiceStatus[it->first] = bRunning;
	}
}

void CServiceDataMng::UpdateAllServiceInfo(std::map<int, ServiceInfo> &p_mapServiceInfo, ServiceInfo p_refServiceInfo, OperationType p_enType)
{
	int iServiceCount = 0;
	m_ServiceInfoLock.lock();
	m_mapServiceInfo = p_mapServiceInfo;
	iServiceCount = (int)m_mapServiceInfo.size();
	m_ServiceInfoLock.unlock();

	// wyl 2026-05-06：添加服务只保存配置和PID，守护线程仍由界面勾选时创建。
	UpdateServiceToCfg(iServiceCount, &p_refServiceInfo, iServiceCount, p_enType);
}
void CServiceDataMng::UpdateEnable(int p_iServiceRow, int p_iEnable)
{
	bool bFind = false;
	m_ServiceInfoLock.lock();
	if (m_mapServiceInfo.find(p_iServiceRow) != m_mapServiceInfo.end())
	{
		bFind = true;
		m_mapServiceInfo[p_iServiceRow].iEnable = p_iEnable;
	}
	m_ServiceInfoLock.unlock();

	if (!bFind)
		return;

	// wyl 2026-05-06：勾选状态只落配置；守护线程由界面勾选创建、取消勾选销毁。
	UpdateEnableToCfg(p_iServiceRow, p_iEnable);
}
void CServiceDataMng::UpdateTimeInfo(int p_iServiceRow, WeekInfo p_enWeek, int p_iTimeRow, TimeInfo p_stTimeInfo, OperationType p_enType)
{
	ServiceInfo objServiceInfo;

	if (MODIFY == p_enType)// 改， 暂不支持
	{
		m_ServiceInfoLock.lock();
		if (m_mapServiceInfo.find(p_iServiceRow) != m_mapServiceInfo.end())
		{
			vector<TimeInfo> &vecTimeInfo = m_mapServiceInfo[p_iServiceRow].mapTimeConf[p_enWeek];
		}
		m_ServiceInfoLock.unlock();
	}
	else if (ADD == p_enType)// 增
	{
		m_ServiceInfoLock.lock();
		if (m_mapServiceInfo.find(p_iServiceRow) != m_mapServiceInfo.end())
		{
			ServiceInfo &refServiceInfo = m_mapServiceInfo[p_iServiceRow];
			refServiceInfo.AddTime(p_enWeek, p_stTimeInfo);
			objServiceInfo = refServiceInfo;
		}
		m_ServiceInfoLock.unlock();
	}
	else if (DEL == p_enType)// 删
	{
		m_ServiceInfoLock.lock();
		if (m_mapServiceInfo.find(p_iServiceRow) != m_mapServiceInfo.end())
		{
			ServiceInfo &refServiceInfo = m_mapServiceInfo[p_iServiceRow];
			refServiceInfo.DelTime(p_enWeek, p_iTimeRow);
			objServiceInfo = refServiceInfo;
		}
		m_ServiceInfoLock.unlock();
	}
	else
	{
		return;
	}

	//把数据写入配置文件
	UpdateTimeToCfg(p_iServiceRow, &objServiceInfo, m_iCount, p_enType);

	//同步到守护线程
	UpdateTimeDaemon(p_iServiceRow, &objServiceInfo);
}

void CServiceDataMng::UpdateEnableToCfg(int p_iServiceRow, int p_iEnable)
{
	char strLabel[100] = { 0 };
	sprintf(strLabel, "Info_%d", p_iServiceRow);
	WritePrivateProfileString(strLabel, "Enable", std::to_string(p_iEnable).c_str(), m_strCfg.c_str());
}

void CServiceDataMng::UpdateTimeToCfg(int p_iServiceRow, ServiceInfo *p_refServiceInfo, int p_iCount, OperationType p_enType)
{
	char strLabel[100] = { 0 };
	char strStartTime[100] = { 0 };
	char strEndTime[100] = { 0 };
	char strTimeCount[100] = { 0 };


	if (NULL == p_refServiceInfo)
		return;

	//先删除后写入
	sprintf(strLabel, "Info_%d", p_iServiceRow);
	p_refServiceInfo->lPid = GetPrivateProfileInt(strLabel, "Pid", p_refServiceInfo->lPid, m_strCfg.c_str());
	//p_refServiceInfo->iCheck = GetPrivateProfileInt(strLabel, "Check", 0, m_strCfg.c_str());
	WritePrivateProfileSection(strLabel, "", m_strCfg.c_str());

	if (ADD == p_enType || DEL == p_enType)
	{

		WritePrivateProfileString(strLabel, "Name", p_refServiceInfo->strName.c_str(), m_strCfg.c_str());
		WritePrivateProfileString(strLabel, "Path", p_refServiceInfo->strPath.c_str(), m_strCfg.c_str());
		//WritePrivateProfileString(strLabel, "Cmd", p_refServiceInfo->strCmdParam.c_str(), m_strCfg.c_str());
		WritePrivateProfileString(strLabel, "Enable", std::to_string(p_refServiceInfo->iEnable).c_str(), m_strCfg.c_str());
		WritePrivateProfileString(strLabel, "Pid", std::to_string(p_refServiceInfo->lPid).c_str(), m_strCfg.c_str());
		WritePrivateProfileString(strLabel, "Check", std::to_string(p_refServiceInfo->iCheck).c_str(), m_strCfg.c_str());

		for (int j = 0; j < WEEK_NUM; j++)
		{
			WeekInfo enWeek = j == 0 ? MON : j == 1 ? TUE : j == 2 ? WED : j == 3 ? THU : j == 4 ? FRI : j == 5 ? SAT : j == 6 ? SUN : MON;
			vector<TimeInfo> listTimeInfo = p_refServiceInfo->mapTimeConf[enWeek];

			sprintf(strTimeCount, "TimeCount_%d", j);
			WritePrivateProfileString(strLabel, strTimeCount, std::to_string(listTimeInfo.size()).c_str(), m_strCfg.c_str());

			for (int k = 0; k < listTimeInfo.size(); k++)//时间下标
			{
				TimeInfo stTimeInfo = listTimeInfo[k];

				sprintf(strStartTime, "StartTime_%d_%d", j, k);
				WritePrivateProfileString(strLabel, strStartTime, stTimeInfo.StartTime.toString().c_str(), m_strCfg.c_str());

				sprintf(strEndTime, "EndTime_%d_%d", j, k);
				WritePrivateProfileString(strLabel, strEndTime, stTimeInfo.EndTime.toString().c_str(), m_strCfg.c_str());
			}
		}
	}
	else if (MODIFY == p_enType)
	{
	}
}

void CServiceDataMng::UpdateServiceToCfg(int p_iServiceRow, ServiceInfo *p_refServiceInfo, int p_iCount, OperationType p_enType)
{
	char strLabel[100] = { 0 };
	char strStartTime[100] = { 0 };
	char strEndTime[100] = { 0 };
	char strTimeCount[100] = { 0 };

	//服务数量
	WritePrivateProfileString("Info", "Count", std::to_string(p_iCount).c_str(), m_strCfg.c_str());

	if (NULL == p_refServiceInfo)
		return;

	if (ADD == p_enType)
	{
		sprintf(strLabel, "Info_%d", p_iCount - 1);
		p_refServiceInfo->lPid = GetPrivateProfileInt(strLabel, "Pid", p_refServiceInfo->lPid, m_strCfg.c_str());
		//p_refServiceInfo->iCheck = GetPrivateProfileInt(strLabel, "Check", 0, m_strCfg.c_str());
		WritePrivateProfileSection(strLabel, "", m_strCfg.c_str());

		WritePrivateProfileString(strLabel, "Name", p_refServiceInfo->strName.c_str(), m_strCfg.c_str());
		WritePrivateProfileString(strLabel, "Path", p_refServiceInfo->strPath.c_str(), m_strCfg.c_str());
		//WritePrivateProfileString(strLabel, "Cmd", p_refServiceInfo->strCmdParam.c_str(), m_strCfg.c_str());
		WritePrivateProfileString(strLabel, "Enable", std::to_string(p_refServiceInfo->iEnable).c_str(), m_strCfg.c_str());
		WritePrivateProfileString(strLabel, "Pid", std::to_string(p_refServiceInfo->lPid).c_str(), m_strCfg.c_str());
		WritePrivateProfileString(strLabel, "Check", std::to_string(p_refServiceInfo->iCheck).c_str(), m_strCfg.c_str());

		for (int j = 0; j < WEEK_NUM; j++)
		{
			WeekInfo enWeek = j == 0 ? MON : j == 1 ? TUE : j == 2 ? WED : j == 3 ? THU : j == 4 ? FRI : j == 5 ? SAT : j == 6 ? SUN : MON;
			vector<TimeInfo> listTimeInfo = p_refServiceInfo->mapTimeConf[enWeek];

			sprintf(strTimeCount, "TimeCount_%d", j);
			WritePrivateProfileString(strLabel, strTimeCount, std::to_string(listTimeInfo.size()).c_str(), m_strCfg.c_str());

			for (int k = 0; k < listTimeInfo.size(); k++)//时间下标
			{
				TimeInfo stTimeInfo = listTimeInfo[k];

				sprintf(strStartTime, "StartTime_%d_%d", j, k);
				WritePrivateProfileString(strLabel, strStartTime, stTimeInfo.StartTime.toString().c_str(), m_strCfg.c_str());

				sprintf(strEndTime, "EndTime_%d_%d", j, k);
				WritePrivateProfileString(strLabel, strEndTime, stTimeInfo.EndTime.toString().c_str(), m_strCfg.c_str());
			}
		}
	}
	else if (DEL == p_enType)
	{
		//先删除后写入
		sprintf(strLabel, "Info_%d", p_iServiceRow);
		WritePrivateProfileSection(strLabel, NULL, m_strCfg.c_str());
		DestroyDaemon(p_iServiceRow);
	}
	else if (MODIFY == p_enType)
	{
	}
}

void CServiceDataMng::CreateDaemon(int p_iServiceRow, ServiceInfo *p_refServiceInfo)
{
	if (NULL == p_refServiceInfo || !IsValidServiceInfo(*p_refServiceInfo))
		return;

	bool bCreated = false;
	bool bUpdated = false;

	m_DaemonLock.lock();
	auto it = m_mapDaemon.find(p_iServiceRow);
	if (it == m_mapDaemon.end())
	{
		CThreadWork *work = new CThreadWork(p_refServiceInfo);
		m_mapDaemon[p_iServiceRow] = work;
		bCreated = true;
	}
	else
	{
		// wyl 2026-05-06：守护线程已存在时只同步最新配置，不重复创建线程。
		it->second->UpdateTime(p_refServiceInfo);
		bUpdated = true;
	}
	m_DaemonLock.unlock();

	if (bCreated)
	{
		MT_INFO("[Worker] daemon created, name=%s,path=%s",
			p_refServiceInfo->strName.c_str(), p_refServiceInfo->strPath.c_str());
	}
	else if (bUpdated)
	{
		MT_INFO("[Worker] daemon updated, name=%s,path=%s",
			p_refServiceInfo->strName.c_str(), p_refServiceInfo->strPath.c_str());
	}

	Sleep(50);
}

void CServiceDataMng::DestroyDaemon(int p_iServiceRow)
{
	bool bFlag = false;
	string strName = "", strPath = "";
	m_DaemonLock.lock();
	if (m_mapDaemon.find(p_iServiceRow) != m_mapDaemon.end())
	{
		CThreadWork *work = m_mapDaemon[p_iServiceRow];
		bFlag = true;

		work->m_bExit = true;
		while (work->m_bThreadWorkFinished == false) {
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}

		strName = work->m_stServiceInfo.strName;
		strPath = work->m_stServiceInfo.strPath;
		
		delete work;
		work = NULL;
		m_mapDaemon.erase(p_iServiceRow);
	}
	m_DaemonLock.unlock();
	if (bFlag)
	{
		MT_INFO("[Worker] service deleted, name=%s,path=%s",
			strName.c_str(), strPath.c_str());
	}
}

void CServiceDataMng::UpdateTimeDaemon(int p_iServiceRow, ServiceInfo *p_refServiceInfo)
{
	ServiceInfo stServiceInfo;
	m_DaemonLock.lock();
	if (m_mapDaemon.find(p_iServiceRow) != m_mapDaemon.end())
	{
		CThreadWork *work = m_mapDaemon[p_iServiceRow];
		stServiceInfo = work->m_stServiceInfo;
		work->UpdateTime(p_refServiceInfo);
	}
	m_DaemonLock.unlock();

	MT_INFO("[Worker] schedule synced, name=%s,path=%s",
		stServiceInfo.strName.c_str(), stServiceInfo.strPath.c_str());
}
