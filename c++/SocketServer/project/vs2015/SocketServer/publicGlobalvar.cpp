#include "publicGlobalvar.h"

volatile bool g_bServerStatus = false;

CHPThreadPoolPtr g_CTcpHPThreadPool;

CTcpSockServerObj *g_CTcpSockServerObj = nullptr;
CTcpServerListerNet *g_CTcpServerListerNet = nullptr;
ITcpServer *g_CTcpPackServer = nullptr;
TCP_NOTIFY_PROC g_pTcpHandle = nullptr;
CONNID g_ullTaskID = 0;

pthread_mutex_t g_mutexConnet;
std::map<CONNID, ClientData> g_mapClient;
std::set<CONNID> g_setTcpLocalClosing;

pthread_mutex_t g_mutexReq;
std::map<CONNID, ReqCacheData *> g_mapQueue;

pthread_mutex_t g_mutexTask;
std::map<CONNID, NotifyTask *> g_mapTask;

//********** WEB服务模块 **********/
CHPThreadPoolPtr g_CWebHPThreadPool;
volatile bool g_bWebServerStatus = false;
CONNID g_ullWebTaskID = 0;
WEB_NOTIFY_PROC g_pWebHandle = nullptr;
IHttpServer  *g_CWebPackServer = nullptr;
CWebSockServerObj *g_CWebSockServerObj = nullptr;
CWebServerListerNet *g_CWebServerListerNet = nullptr;

pthread_mutex_t g_mutexWebConnet;
std::map<CONNID, ClientData> g_mapWebClient;
std::set<CONNID> g_setWebLocalClosing;

pthread_mutex_t g_mutexWebReq;
std::map<CONNID, ReqCacheData *> g_mapWebQueue;

pthread_mutex_t g_mutexWebTask;
std::map<CONNID, NotifyTask *> g_mapWebTask;

//********** HTTP服务模块 **********/
CHPThreadPoolPtr g_CHttpHPThreadPool;
volatile bool g_bHttpServerStatus = false;
unsigned long long g_ullHttpAsynReqID = 0;
HTTP_NOTIFY_PROC g_pHttpHandle = nullptr;
IHttpServer* g_CHttpPackServer = nullptr;
CHttpSockServerObj* g_CHttpSockServerObj = nullptr;
CHttpServerListerNet* g_CHttpServerListerNet = nullptr;

pthread_mutex_t g_mutexHttpReq;
std::map<unsigned long long, CHttpAsynReqObj*> g_mapHttpReq;
std::map<CONNID, unsigned long long> g_mapHttpConnReq;
std::map<CONNID, unsigned long long> g_mapHttpConnActiveReq;

