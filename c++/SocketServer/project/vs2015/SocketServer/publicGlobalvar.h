#ifndef _GLOBALVAR_H_
#define _GLOBALVAR_H_

#include "TcpSockServerObj.h"
#include "TcpServerListerNet.h"
#include "WebSockServerObj.h"
#include "WebServerListerNet.h"

#include <pthread.h>
#include <map>
#include <set>
#include "Struct.h"

//全局变量
extern CHPThreadPoolPtr g_CHPThreadPool;
extern volatile bool g_bServerStatus;

/********** TCP服务模块 **********/
extern CTcpSockServerObj *g_CTcpSockServerObj;
extern CTcpServerListerNet *g_CTcpServerListerNet;
extern ITcpServer *g_CTcpPackServer;
extern TCP_NOTIFY_PROC g_pTcpHandle;
extern CONNID g_ullTaskID;

extern pthread_mutex_t g_mutexConnet;	//连接锁
extern std::map<CONNID, ClientData> g_mapClient;//客户信息
extern std::set<CONNID> g_setTcpLocalClosing;//本端主动关闭中的连接

extern pthread_mutex_t g_mutexReq;	//请求锁
extern std::map<CONNID, ReqCacheData *> g_mapQueue;//请求缓存

extern pthread_mutex_t g_mutexTask;	//任务锁
extern std::map<CONNID, NotifyTask *> g_mapTask;//请求缓存

//********** WEB服务模块 **********/
extern CHPThreadPoolPtr g_CWebHPThreadPool;
extern volatile bool g_bWebServerStatus;
extern WEB_NOTIFY_PROC g_pWebHandle;
extern CONNID g_ullWebTaskID;
extern IHttpServer *g_CWebPackServer;
extern CWebSockServerObj *g_CWebSockServerObj;
extern CWebServerListerNet *g_CWebServerListerNet;

extern pthread_mutex_t g_mutexWebConnet;	//连接锁
extern std::map<CONNID, ClientData> g_mapWebClient;//客户信息
extern std::set<CONNID> g_setWebLocalClosing;//本端主动关闭中的连接

extern pthread_mutex_t g_mutexWebReq;	//请求锁
extern std::map<CONNID, ReqCacheData *> g_mapWebQueue;//请求缓存

extern pthread_mutex_t g_mutexWebTask;	//任务锁
extern std::map<CONNID, NotifyTask *> g_mapWebTask;//请求缓存

#endif
