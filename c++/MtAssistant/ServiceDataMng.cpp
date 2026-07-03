#include "stdafx.h"
#include "ServiceDataMng.h"
#include "ServiceConfigXml.h"

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

CServiceDataMng::CServiceDataMng() : m_iCount(0), m_strLogLevel("info")
{
}

CServiceDataMng::~CServiceDataMng()
{
	std::vector<int> vecDaemonRows;
	m_DaemonLock.lock();
	for (auto it = m_mapDaemon.begin(); it != m_mapDaemon.end(); ++it)
		vecDaemonRows.push_back(it->first);
	m_DaemonLock.unlock();

	for (size_t i = 0; i < vecDaemonRows.size(); ++i)
		DestroyDaemon(vecDaemonRows[i]);
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
			ServiceInfo stServiceInfo = it->second;
			CreateDaemon(it->first, &stServiceInfo);
		}
	}
}

namespace
{
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
		if (!IsAbsoluteProgramPath(p_refServiceInfo.strPath))
			return false;

		string strExt = p_refServiceInfo.strName.substr(p_refServiceInfo.strName.length() - 4);
		return _stricmp(strExt.c_str(), ".exe") == 0 || _stricmp(strExt.c_str(), ".bat") == 0;
	}

	string BuildServiceLabel(int p_iRow)
	{
		char szLabel[64] = { 0 };
		sprintf_s(szLabel, "Info_%d", p_iRow);
		return szLabel;
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
}

void CServiceDataMng::NormalizeServiceRows(std::map<int, ServiceInfo>& p_refMapServiceInfo)
{
	std::map<int, ServiceInfo> mapNormalized;
	int iRow = 0;
	for (auto it = p_refMapServiceInfo.begin(); it != p_refMapServiceInfo.end(); ++it)
	{
		ServiceInfo stServiceInfo = it->second;
		stServiceInfo.iRow = iRow;
		stServiceInfo.strLabel = BuildServiceLabel(iRow);
		stServiceInfo.strCfg = m_strCfg;
		mapNormalized[iRow] = stServiceInfo;
		++iRow;
	}
	p_refMapServiceInfo.swap(mapNormalized);
}

void CServiceDataMng::SaveCfg(const std::map<int, ServiceInfo>& p_refMapServiceInfo)
{
	ServiceConfigXmlData stData;
	stData.strLogLevel = m_strLogLevel.empty() ? "info" : m_strLogLevel;
	stData.mapServiceInfo = p_refMapServiceInfo;
	if (!SaveServiceConfigXml(m_strCfg, stData))
	{
		MT_WARN("[MtAssistant] save config failed, path=%s", m_strCfg.c_str());
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

	ServiceConfigXmlData stConfigData;
	if (_access(m_strCfg.c_str(), 0) == -1)
	{
		MT_WARN("[MtAssistant] xml config file missing, create empty config, path=%s", m_strCfg.c_str());
		m_ServiceInfoLock.lock();
		m_mapServiceInfo.clear();
		m_iCount = 0;
		m_ServiceInfoLock.unlock();
		SaveCfg(stConfigData.mapServiceInfo);
		CLog::GetInstance()->SetLogLevel((char*)stConfigData.strLogLevel.c_str());
		return;
	}

	MT_INFO("[MtAssistant] load xml config start");
	if (!LoadServiceConfigXml(m_strCfg, stConfigData))
	{
		m_ServiceInfoLock.lock();
		m_mapServiceInfo.clear();
		m_iCount = 0;
		m_ServiceInfoLock.unlock();
		CLog::GetInstance()->SetLogLevel((char*)m_strLogLevel.c_str());
		return;
	}

	m_strLogLevel = stConfigData.strLogLevel.empty() ? "info" : stConfigData.strLogLevel;
	MT_INFO("[MtAssistant] log level: %s", m_strLogLevel.c_str());
	CLog::GetInstance()->SetLogLevel((char*)m_strLogLevel.c_str());

	std::map<int, ServiceInfo> mapServiceInfo = stConfigData.mapServiceInfo;
	NormalizeServiceRows(mapServiceInfo);

	m_ServiceInfoLock.lock();
	m_mapServiceInfo = mapServiceInfo;
	m_iCount = (int)m_mapServiceInfo.size();
	m_ServiceInfoLock.unlock();

	MT_INFO("[MtAssistant] load xml config done");
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
	return (int)p_mapServiceInfo.size();
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
			UpdateRuntimePid(it->first, lPid);
			MT_INFO("[MtAssistant] service already started, name=%s,path=%s,pid=%ld",
				refServiceInfo.strName.c_str(), refServiceInfo.strPath.c_str(), lPid);
		}
		else if (lPid <= 0 && refServiceInfo.lPid > 0)
		{
			UpdateRuntimePid(it->first, -1);
		}

		p_mapServiceStatus[it->first] = bRunning;
	}
}

void CServiceDataMng::UpdateAllServiceInfo(std::map<int, ServiceInfo> &p_mapServiceInfo, ServiceInfo p_refServiceInfo, OperationType p_enType)
{
	bool bAllDaemonDestroyed = true;
	if (DEL == p_enType)
	{
		std::vector<int> vecDaemonRows;
		m_DaemonLock.lock();
		for (auto it = m_mapDaemon.begin(); it != m_mapDaemon.end(); ++it)
			vecDaemonRows.push_back(it->first);
		m_DaemonLock.unlock();

		for (size_t i = 0; i < vecDaemonRows.size(); ++i)
		{
			if (!DestroyDaemon(vecDaemonRows[i]))
				bAllDaemonDestroyed = false;
		}
	}

	std::map<int, ServiceInfo> mapServiceInfo = p_mapServiceInfo;
	NormalizeServiceRows(mapServiceInfo);

	m_ServiceInfoLock.lock();
	m_mapServiceInfo = mapServiceInfo;
	m_iCount = (int)m_mapServiceInfo.size();
	std::map<int, ServiceInfo> mapSnapshot = m_mapServiceInfo;
	m_ServiceInfoLock.unlock();

	p_mapServiceInfo = mapSnapshot;
	SaveCfg(mapSnapshot);

	if (DEL == p_enType)
	{
		if (!bAllDaemonDestroyed)
		{
			MT_WARN("[Worker] skip daemon recreate because old daemon is still exiting");
			return;
		}

		for (auto it = mapSnapshot.begin(); it != mapSnapshot.end(); ++it)
		{
			if (1 == it->second.iEnable)
			{
				ServiceInfo stServiceInfo = it->second;
				CreateDaemon(it->first, &stServiceInfo);
			}
		}
	}
}

void CServiceDataMng::UpdateEnable(int p_iServiceRow, int p_iEnable)
{
	bool bFind = false;
	std::map<int, ServiceInfo> mapSnapshot;
	m_ServiceInfoLock.lock();
	if (m_mapServiceInfo.find(p_iServiceRow) != m_mapServiceInfo.end())
	{
		bFind = true;
		m_mapServiceInfo[p_iServiceRow].iEnable = p_iEnable;
		mapSnapshot = m_mapServiceInfo;
	}
	m_ServiceInfoLock.unlock();

	if (!bFind)
		return;

	// wyl 2026-05-06：勾选状态只落XML配置；守护线程由界面勾选创建、取消勾选销毁。
	SaveCfg(mapSnapshot);
}

void CServiceDataMng::UpdateRuntimePid(int p_iServiceRow, long p_lPid)
{
	bool bFind = false;
	std::map<int, ServiceInfo> mapSnapshot;
	m_ServiceInfoLock.lock();
	if (m_mapServiceInfo.find(p_iServiceRow) != m_mapServiceInfo.end())
	{
		bFind = true;
		m_mapServiceInfo[p_iServiceRow].lPid = p_lPid;
		mapSnapshot = m_mapServiceInfo;
	}
	m_ServiceInfoLock.unlock();

	if (bFind)
		SaveCfg(mapSnapshot);
}

void CServiceDataMng::UpdateTimeInfo(int p_iServiceRow, WeekInfo p_enWeek, int p_iTimeRow, TimeInfo p_stTimeInfo, OperationType p_enType)
{
	ServiceInfo objServiceInfo;
	bool bFind = false;
	std::map<int, ServiceInfo> mapSnapshot;

	if (MODIFY == p_enType)// 改， 暂不支持
	{
		return;
	}
	else if (ADD == p_enType)// 增
	{
		m_ServiceInfoLock.lock();
		if (m_mapServiceInfo.find(p_iServiceRow) != m_mapServiceInfo.end())
		{
			ServiceInfo &refServiceInfo = m_mapServiceInfo[p_iServiceRow];
			refServiceInfo.AddTime(p_enWeek, p_stTimeInfo);
			objServiceInfo = refServiceInfo;
			mapSnapshot = m_mapServiceInfo;
			bFind = true;
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
			mapSnapshot = m_mapServiceInfo;
			bFind = true;
		}
		m_ServiceInfoLock.unlock();
	}
	else
	{
		return;
	}

	if (!bFind)
		return;

	SaveCfg(mapSnapshot);
	MT_INFO("[Worker] schedule config changed, row=%d,week=%s,start=%s,end=%s,op=%d,name=%s,path=%s",
		p_iServiceRow, WeekToXmlName(p_enWeek),
		p_stTimeInfo.StartTime.toString().c_str(), p_stTimeInfo.EndTime.toString().c_str(),
		(int)p_enType, objServiceInfo.strName.c_str(), objServiceInfo.strPath.c_str());
	UpdateTimeDaemon(p_iServiceRow, &objServiceInfo);
}

int CServiceDataMng::StopService(int p_iServiceRow)
{
	ServiceInfo stServiceInfo = GetServiceInfo(p_iServiceRow);
	if (stServiceInfo.strName.empty() || stServiceInfo.strPath.empty())
		return -1;

	UpdateEnable(p_iServiceRow, 0);
	DestroyDaemon(p_iServiceRow);

	if (IsCurrentProcessServicePath(stServiceInfo.strPath))
	{
		MT_WARN("[MtAssistant] skip stop current assistant process, name=%s,path=%s",
			stServiceInfo.strName.c_str(), stServiceInfo.strPath.c_str());
		return -2;
	}

	long lPid = ProbeRunningServicePid(stServiceInfo);
	int iStopRet = StopProcessTree(lPid > 0 ? lPid : -1,
		stServiceInfo.strName.c_str(),
		stServiceInfo.strPath.c_str(),
		stServiceInfo.strTitle.c_str(),
		0,
		10 * 1000);

	if (0 == iStopRet)
	{
		UpdateRuntimePid(p_iServiceRow, -1);
		MT_INFO("[MtAssistant] stop service, name=%s,path=%s",
			stServiceInfo.strName.c_str(), stServiceInfo.strPath.c_str());
	}
	else
	{
		MT_WARN("[MtAssistant] stop service failed=%d, name=%s,path=%s",
			iStopRet, stServiceInfo.strName.c_str(), stServiceInfo.strPath.c_str());
	}

	return iStopRet;
}

void CServiceDataMng::CreateDaemon(int p_iServiceRow, ServiceInfo *p_refServiceInfo)
{
	if (NULL == p_refServiceInfo || !IsValidServiceInfo(*p_refServiceInfo))
		return;

	ServiceInfo stServiceInfo = *p_refServiceInfo;
	stServiceInfo.iRow = p_iServiceRow;
	stServiceInfo.strLabel = BuildServiceLabel(p_iServiceRow);
	stServiceInfo.strCfg = m_strCfg;

	bool bCreated = false;
	bool bUpdated = false;
	CThreadWork* pStaleWork = NULL;

	m_DaemonLock.lock();
	auto it = m_mapDaemon.find(p_iServiceRow);
	if (it != m_mapDaemon.end() && it->second->m_bThreadWorkFinished.load())
	{
		pStaleWork = it->second;
		m_mapDaemon.erase(it);
		it = m_mapDaemon.end();
	}

	if (it == m_mapDaemon.end())
	{
		CThreadWork *work = new CThreadWork(&stServiceInfo);
		m_mapDaemon[p_iServiceRow] = work;
		bCreated = true;
	}
	else
	{
		// wyl 2026-05-06：守护线程已存在时只同步最新配置，不重复创建线程。
		it->second->UpdateTime(&stServiceInfo);
		bUpdated = true;
	}
	m_DaemonLock.unlock();

	if (pStaleWork != NULL)
	{
		delete pStaleWork;
		MT_WARN("[Worker] stale daemon replaced, row=%d,name=%s,path=%s",
			p_iServiceRow, stServiceInfo.strName.c_str(), stServiceInfo.strPath.c_str());
	}

	if (bCreated)
	{
		MT_INFO("[Worker] daemon created, name=%s,path=%s",
			stServiceInfo.strName.c_str(), stServiceInfo.strPath.c_str());
	}
	else if (bUpdated)
	{
		MT_INFO("[Worker] daemon updated, name=%s,path=%s",
			stServiceInfo.strName.c_str(), stServiceInfo.strPath.c_str());
	}

	Sleep(50);
}

bool CServiceDataMng::DestroyDaemon(int p_iServiceRow)
{
	CThreadWork* work = NULL;
	string strName = "", strPath = "";

	m_DaemonLock.lock();
	auto it = m_mapDaemon.find(p_iServiceRow);
	if (it != m_mapDaemon.end())
	{
		work = it->second;
		work->m_bExit = true;
		strName = work->m_stServiceInfo.strName;
		strPath = work->m_stServiceInfo.strPath;
	}
	m_DaemonLock.unlock();

	if (work == NULL)
		return true;

	const int iMaxWaitMs = 15000;
	int iWaitedMs = 0;
	while (!work->m_bThreadWorkFinished.load() && iWaitedMs < iMaxWaitMs)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
		iWaitedMs += 20;
	}

	if (!work->m_bThreadWorkFinished.load())
	{
		MT_WARN("[Worker] daemon destroy timeout, row=%d,name=%s,path=%s",
			p_iServiceRow, strName.c_str(), strPath.c_str());
		return false;
	}

	m_DaemonLock.lock();
	auto itErase = m_mapDaemon.find(p_iServiceRow);
	if (itErase != m_mapDaemon.end() && itErase->second == work)
	{
		m_mapDaemon.erase(itErase);
	}
	m_DaemonLock.unlock();

	delete work;
	MT_INFO("[Worker] daemon destroyed, name=%s,path=%s",
		strName.c_str(), strPath.c_str());
	return true;
}

void CServiceDataMng::UpdateTimeDaemon(int p_iServiceRow, ServiceInfo *p_refServiceInfo)
{
	if (NULL == p_refServiceInfo)
		return;

	ServiceInfo stServiceInfo = *p_refServiceInfo;
	bool bSynced = false;
	bool bNeedCreate = false;
	CThreadWork* pStaleWork = NULL;

	m_DaemonLock.lock();
	auto it = m_mapDaemon.find(p_iServiceRow);
	if (it != m_mapDaemon.end() && it->second->m_bThreadWorkFinished.load())
	{
		pStaleWork = it->second;
		m_mapDaemon.erase(it);
		it = m_mapDaemon.end();
	}

	if (it != m_mapDaemon.end())
	{
		CThreadWork *work = it->second;
		work->UpdateTime(&stServiceInfo);
		bSynced = true;
	}
	else if (1 == stServiceInfo.iEnable)
	{
		bNeedCreate = true;
	}
	m_DaemonLock.unlock();

	if (pStaleWork != NULL)
	{
		delete pStaleWork;
		MT_WARN("[Worker] stale daemon removed before schedule sync, row=%d,name=%s,path=%s",
			p_iServiceRow, stServiceInfo.strName.c_str(), stServiceInfo.strPath.c_str());
	}

	if (bSynced)
	{
		MT_INFO("[Worker] schedule synced, row=%d,name=%s,path=%s",
			p_iServiceRow, stServiceInfo.strName.c_str(), stServiceInfo.strPath.c_str());
	}
	else if (bNeedCreate)
	{
		MT_WARN("[Worker] schedule changed but daemon missing, recreate, row=%d,name=%s,path=%s",
			p_iServiceRow, stServiceInfo.strName.c_str(), stServiceInfo.strPath.c_str());
		CreateDaemon(p_iServiceRow, &stServiceInfo);
	}
	else
	{
		MT_INFO("[Worker] schedule changed while service disabled, row=%d,name=%s,path=%s",
			p_iServiceRow, stServiceInfo.strName.c_str(), stServiceInfo.strPath.c_str());
	}
}
