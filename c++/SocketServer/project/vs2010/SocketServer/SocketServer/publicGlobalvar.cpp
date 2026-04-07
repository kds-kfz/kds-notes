#include "publicGlobalvar.h"

std::string g_strServerName = "[TCP·þÎñ]";
volatile bool g_bServerStatus = false;

CHPThreadPoolPtr g_CHPThreadPool;

CTcpSockServerObj *g_CTcpSockServerObj = nullptr;
CTcpServerListerNet *g_CTcpServerListerNet = nullptr;
ITcpServer *g_CTcpPackServer = nullptr;
TCP_NOTIFY_PROC g_pTcpHandle = nullptr;
CONNID g_ullTaskID = 0;

pthread_mutex_t g_mutexConnet;
std::map<CONNID, ClientData> g_mapClient;

pthread_mutex_t g_mutexReq;
std::map<CONNID, ReqCacheData *> g_mapQueue;

pthread_mutex_t g_mutexTask;
std::map<CONNID, NotifyTask *> g_mapTask;
