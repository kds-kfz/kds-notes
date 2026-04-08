#ifndef _GLOBALVAR_H_
#define _GLOBALVAR_H_

#include "TcpSockServerObj.h"
#include "TcpServerListerNet.h"
#include "WebSockServerObj.h"
#include "WebServerListerNet.h"
#include "CHttpAsynReqObj.h"
#include "HttpSockServerObj.h"
#include "HttpServerListerNet.h"

#include <pthread.h>
#include <map>
#include <set>
#include "Struct.h"

//全局变量

/********** TCP服务模块 **********/
extern CHPThreadPoolPtr g_CTcpHPThreadPool;
extern volatile bool g_bServerStatus;
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
extern std::map<CONNID, NotifyTask *> g_mapTask;//任务缓存

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
extern std::map<CONNID, NotifyTask *> g_mapWebTask;//任务缓存

//********** HTTP服务模块 **********/
extern CHPThreadPoolPtr g_CHttpHPThreadPool;
extern volatile bool g_bHttpServerStatus;
extern HTTP_NOTIFY_PROC g_pHttpHandle;
extern unsigned long long g_ullHttpAsynReqID;
extern IHttpServer* g_CHttpPackServer;
extern CHttpSockServerObj* g_CHttpSockServerObj;
extern CHttpServerListerNet* g_CHttpServerListerNet;

extern pthread_mutex_t g_mutexHttpReq;	//请求锁
extern std::map<unsigned long long, CHttpAsynReqObj*> g_mapHttpReq;//异步请求对象，生命周期以“异步请求号”为主键
extern std::map<CONNID, unsigned long long> g_mapHttpConnReq;//当前连接正在解析中的请求，只在 HTTP 解析阶段占用
extern std::map<CONNID, unsigned long long> g_mapHttpConnActiveReq;//当前连接已分发给上层、等待应答的请求，用于限制同连接在途请求数

#endif
