#ifndef __SERVICE_DATA_H__
#define __SERVICE_DATA_H__

#include "ThreadWoker.h"
#include<thread>
#include<mutex>
using namespace std;

#define SERVICE_CFG_NAME "HqAssistant.ini"


//服务数据管理
class CServiceDataMng
{
private:
	static CServiceDataMng *m_pThis;
private:
	int m_iCount;//服务个数
	string m_strHome;//根路径
	string m_strCfg;//配置路径
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
	void DestroyDaemon(int p_iServiceRow);//销毁守护线程
private:
	void UpdateEnableToCfg(int p_iServiceRow, int p_iEnable);
	void UpdateTimeToCfg(int p_iServiceRow, ServiceInfo *p_refServiceInfo, int p_iCount, OperationType p_enType = ADD);
	void UpdateServiceToCfg(int p_iServiceRow, ServiceInfo *p_refServiceInfo, int p_iCount, OperationType p_enType = ADD);
	void UpdateTimeDaemon(int p_iServiceRow, ServiceInfo *p_refServiceInfo);
public:
	CServiceDataMng();
	~CServiceDataMng();
	static void Release();
	static CServiceDataMng *GetInstance();
};

#endif