#ifndef _GLOBALVAR_H_
#define _GLOBALVAR_H_

#include "TcpSockServerObj.h"
#include "TcpServerListerNet.h"

#include <pthread.h>
#include <map>
#include "Struct.h"

//全局变量
extern CHPThreadPoolPtr g_CHPThreadPool;
extern std::string g_strServerName;
extern volatile bool g_bServerStatus;

/********** TCP服务模块 **********/
extern CTcpSockServerObj *g_CTcpSockServerObj;
extern CTcpServerListerNet *g_CTcpServerListerNet;
extern ITcpServer *g_CTcpPackServer;
extern TCP_NOTIFY_PROC g_pTcpHandle;
extern CONNID g_ullTaskID;

extern pthread_mutex_t g_mutexConnet;	//连接锁
extern std::map<CONNID, ClientData> g_mapClient;//客户信息

extern pthread_mutex_t g_mutexReq;	//请求锁
extern std::map<CONNID, ReqCacheData *> g_mapQueue;//请求缓存

extern pthread_mutex_t g_mutexTask;	//任务锁
extern std::map<CONNID, NotifyTask *> g_mapTask;//请求缓存

#endif