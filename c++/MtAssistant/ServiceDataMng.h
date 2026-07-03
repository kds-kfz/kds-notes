#ifndef __SERVICE_DATA_H__
#define __SERVICE_DATA_H__

#include "ThreadWoker.h"
#include<thread>
#include<mutex>
using namespace std;

#define SERVICE_CFG_NAME "MtAssistant.xml"


//服务数据管理
class CServiceDataMng
{
private:
	static CServiceDataMng *m_pThis;
private:
	int m_iCount;//服务个数
	string m_strHome;//根路径
	string m_strCfg;//配置路径
	string m_strLogLevel;// wyl 2026-07-03：XML日志级别配置，保存配置时原样写回。
	std::mutex m_ServiceInfoLock;//服务信息锁
	std::mutex m_DaemonLock;//守护线程锁
	std::map<int, ServiceInfo>m_mapServiceInfo;//行数,服务信息
	std::map<int, CThreadWork *>m_mapDaemon;//行数,守护线程
public:
	void Init();//初始化
	void ReadCfg();//读取配置
	string GetCfgPath() { return m_strCfg; }

	//服务tab显示列表
	ServiceInfo GetServiceInfo(int p_iServiceRow);////获取服务信息
	//时间tab显示列表
	//设置服务信息，流程：选择服务列表，勾选使能->选择星期->设置起止时间->添加保存
	void UpdateTimeInfo(int p_iServiceRow, WeekInfo p_enWeek, int p_iTimeRow, TimeInfo p_stTimeInfo, OperationType p_enType = ADD);//设置服务信息
	void UpdateEnable(int p_iServiceRow, int p_iEnable);//设置服务信息
	int GetAllServiceInfo(std::map<int, ServiceInfo> &p_mapServiceInfo);
	void GetAllServiceStatus(std::map<int, bool> &p_mapServiceStatus);
	void UpdateAllServiceInfo(std::map<int, ServiceInfo> &p_mapServiceInfo, ServiceInfo p_refServiceInfo, OperationType p_enType = ADD);
	void CreateDaemon(int p_iServiceRow, ServiceInfo *p_refServiceInfo);//创建守护线程
	bool DestroyDaemon(int p_iServiceRow);//销毁守护线程
	void UpdateRuntimePid(int p_iServiceRow, long p_lPid);// wyl 2026-07-03：运行时PID统一写回XML。
	int StopService(int p_iServiceRow);// wyl 2026-07-03：手动停止服务并强制退出进程树。
private:
	void SaveCfg(const std::map<int, ServiceInfo>& p_refMapServiceInfo);
	void NormalizeServiceRows(std::map<int, ServiceInfo>& p_refMapServiceInfo);
	void UpdateTimeDaemon(int p_iServiceRow, ServiceInfo *p_refServiceInfo);
public:
	CServiceDataMng();
	~CServiceDataMng();
	static void Release();
	static CServiceDataMng *GetInstance();
};

#endif
