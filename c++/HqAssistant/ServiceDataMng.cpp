#include "stdafx.h"
#include "ServiceDataMng.h"
#include <io.h>

CServiceDataMng *CServiceDataMng::m_pThis = NULL;

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

CServiceDataMng::CServiceDataMng()
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
	m_ServiceInfoLock.lock();
	for (auto it = m_mapServiceInfo.begin(); it != m_mapServiceInfo.end(); it++)
	{
		int iStrNameLen = it->second.strName.length();
		if (1 == it->second.iEnable && (
			it->second.strName.find("exe", iStrNameLen - 4) != string::npos ||
			it->second.strName.find("EXE", iStrNameLen - 4) != string::npos ||
			it->second.strName.find("bat", iStrNameLen - 4) != string::npos ||
			it->second.strName.find("BAT", iStrNameLen - 4) != string::npos)
			)
		{
			CreateDaemon(it->first, &it->second);
		}
	}
	m_ServiceInfoLock.unlock();
}

string GetTitle(char *p_szName)
{
	if (0 == strcmp("TradingTerminal.exe", p_szName))
		return "--MT交易终端--7×24小时不间断";
	
	return "";
}

void CServiceDataMng::ReadCfg()
{
	m_strHome = GetRootPath();

	string  strLogPath = m_strHome;
	strLogPath.append("\\Logs\\");
	int iRet = CLog::GetInstance()->InitLog(strLogPath.c_str());
	if (MA_OK != iRet)
	{
		return;
	}

	//记得恢复工作线程
	CLog::GetInstance()->Resume();

	MT_INFO("启动LOG  ****************************");

	m_strCfg = m_strHome;
	m_strCfg.append("\\");
	m_strCfg.append(SERVICE_CFG_NAME);

	if (_access(m_strCfg.c_str(), 0) == -1)
	{
		MT_WARN("[服务助手] 配置文件路径=%s,不存在", m_strCfg.c_str());
		return;
	}

	// 设置日志级别
	char strLogLevel[64] = { 0 };
	GetPrivateProfileString("LOG", "LogLevel", "info", strLogLevel, sizeof(strLogLevel), m_strCfg.c_str());
	MT_INFO("[服务助手] 日志级别: %s", strLogLevel);
	CLog::GetInstance()->SetLogLevel(strLogLevel);

	char szTemp[_MAX_PATH] = { 0 };

	MT_INFO("[服务助手] 开始读取配置");
	m_iCount = GetPrivateProfileInt("Info", "Count", 0, m_strCfg.c_str());

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
			int iTimeCount = GetPrivateProfileInt(strLabel, strTimeCount, 0, m_strCfg.c_str());

			for (int k = 0; k < iTimeCount; k++)//时间下标
			{
				TimeInfo stTimeInfo;
				tagRunTime stStartTime;
				tagRunTime stEndTime;

				sprintf(strStartTime, "StartTime_%d_%d", j, k);
				GetPrivateProfileString(strLabel, strStartTime, "0:0:0", szTemp, _MAX_PATH, m_strCfg.c_str());
				sscanf(szTemp, "%d:%d:%d", &stStartTime.hour, &stStartTime.min, &stStartTime.sec);
				stTimeInfo.StartTime = stStartTime;

				sprintf(strEndTime, "EndTime_%d_%d", j, k);
				GetPrivateProfileString(strLabel, strEndTime, "0:0:0", szTemp, _MAX_PATH, m_strCfg.c_str());
				sscanf(szTemp, "%d:%d:%d", &stEndTime.hour, &stEndTime.min, &stEndTime.sec);
				stTimeInfo.EndTime = stEndTime;

				listTimeInfo.push_back(stTimeInfo);
			}
		}

		m_ServiceInfoLock.lock();
		m_mapServiceInfo[i] = pstServiceInfo;
		m_ServiceInfoLock.unlock();
	}

	MT_INFO("[服务助手] 结束读取配置");
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

void CServiceDataMng::GetAllServiceStatus(std::map<int, bool> &p_mapServiceStatus)
{
	m_DaemonLock.lock();
	for (auto it = m_mapDaemon.begin(); it != m_mapDaemon.end(); it++)
	{

		p_mapServiceStatus[it->first] = it->second->GetStatus();

	}
	m_DaemonLock.unlock();
}

void CServiceDataMng::UpdateAllServiceInfo(std::map<int, ServiceInfo> &p_mapServiceInfo, ServiceInfo p_refServiceInfo, OperationType p_enType)
{
	m_ServiceInfoLock.lock();
	m_mapServiceInfo = p_mapServiceInfo;
	m_ServiceInfoLock.unlock();

	
	UpdateServiceToCfg(m_mapServiceInfo.size(), &p_refServiceInfo, m_mapServiceInfo.size(), p_enType);
}

void CServiceDataMng::UpdateEnable(int p_iServiceRow, int p_iEnable)
{
	bool bFind = false;
	m_ServiceInfoLock.lock();
	if (m_mapServiceInfo.find(p_iServiceRow) != m_mapServiceInfo.end())
	{
		bFind = true;
		ServiceInfo &refServiceInfo = m_mapServiceInfo[p_iServiceRow];
		refServiceInfo.iEnable = p_iEnable;
	}
	m_ServiceInfoLock.unlock();

	if (!bFind)
		return;

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
	p_refServiceInfo->lPid = GetPrivateProfileInt(strLabel, "Pid", -1, m_strCfg.c_str());
	//p_refServiceInfo->iCheck = GetPrivateProfileInt(strLabel, "Check", 0, m_strCfg.c_str());
	WritePrivateProfileSection(strLabel, "", m_strCfg.c_str());

	if (ADD == p_enType || DEL == p_enType)
	{

		WritePrivateProfileString(strLabel, "Name", p_refServiceInfo->strName.c_str(), m_strCfg.c_str());
		WritePrivateProfileString(strLabel, "Path", p_refServiceInfo->strPath.c_str(), m_strCfg.c_str());
		//WritePrivateProfileString(strLabel, "Title", p_refServiceInfo->strTitle.c_str(), m_strCfg.c_str());
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
		p_refServiceInfo->lPid = GetPrivateProfileInt(strLabel, "Pid", -1, m_strCfg.c_str());
		//p_refServiceInfo->iCheck = GetPrivateProfileInt(strLabel, "Check", 0, m_strCfg.c_str());
		WritePrivateProfileSection(strLabel, "", m_strCfg.c_str());

		WritePrivateProfileString(strLabel, "Name", p_refServiceInfo->strName.c_str(), m_strCfg.c_str());
		WritePrivateProfileString(strLabel, "Path", p_refServiceInfo->strPath.c_str(), m_strCfg.c_str());
		//WritePrivateProfileString(strLabel, "Title", p_refServiceInfo->strTitle.c_str(), m_strCfg.c_str());
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
	bool bFlag = false;

	m_DaemonLock.lock();
	if (m_mapDaemon.find(p_iServiceRow) == m_mapDaemon.end())
	{
		CThreadWork *work = new CThreadWork(p_refServiceInfo);
		m_mapDaemon[p_iServiceRow] = work;
		bFlag = true;
	}
	m_DaemonLock.unlock();

	if (bFlag)
	{
		MT_INFO("[守护线程] 进程名=%s,路径=%s,添加完成",
			p_refServiceInfo->strName.c_str(), p_refServiceInfo->strPath.c_str());
	}
	else
	{
		MT_INFO("[守护线程] 进程名=%s,路径=%s,已添加",
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
		MT_INFO("[守护线程] 进程名=%s,路径=%s,删除完成",
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

	MT_INFO("[守护线程] 进程名=%s,路径=%s,同步时间配置",
		stServiceInfo.strName.c_str(), stServiceInfo.strPath.c_str());
}