#include "publicGlobalvar.h"

/********** TCP服务模块 **********/
CSocketServer* CreateTcpSockInstance()
{
	pthread_mutex_lock(&g_mutexServiceLifecycle);
	if (nullptr == g_CTcpSockServerObj)
	{
		g_CTcpSockServerObj = new CTcpSockServerObj();
	}

	CSocketServer* pServer = g_CTcpSockServerObj;
	pthread_mutex_unlock(&g_mutexServiceLifecycle);
	return pServer;
}

void DelTcpSockInstance(CSocketServer *&pIns)
{
	pthread_mutex_lock(&g_mutexServiceLifecycle);
	if (g_CTcpSockServerObj == pIns)
	{
		delete g_CTcpSockServerObj;
		g_CTcpSockServerObj = nullptr;
		pIns = nullptr;
	}
	pthread_mutex_unlock(&g_mutexServiceLifecycle);
}

/********** HTTP服务模块 **********/
CSocketServer* CreateHttpSockInstance()
{
	pthread_mutex_lock(&g_mutexServiceLifecycle);
	if (nullptr == g_CHttpSockServerObj)
	{
		g_CHttpSockServerObj = new CHttpSockServerObj();
	}

	CSocketServer* pServer = g_CHttpSockServerObj;
	pthread_mutex_unlock(&g_mutexServiceLifecycle);
	return pServer;
}

void DelHttpSockInstance(CSocketServer *&pIns)
{
	pthread_mutex_lock(&g_mutexServiceLifecycle);
	if (g_CHttpSockServerObj == pIns)
	{
		delete g_CHttpSockServerObj;
		g_CHttpSockServerObj = nullptr;
		pIns = nullptr;
	}
	pthread_mutex_unlock(&g_mutexServiceLifecycle);
}

/********** WEBSOCKET服务模块 **********/
CSocketServer* CreateWebSockInstance()
{
	pthread_mutex_lock(&g_mutexServiceLifecycle);
	if (nullptr == g_CWebSockServerObj)
	{
		g_CWebSockServerObj = new CWebSockServerObj();
	}

	CSocketServer* pServer = g_CWebSockServerObj;
	pthread_mutex_unlock(&g_mutexServiceLifecycle);
	return pServer;
}

void DelWebSockInstance(CSocketServer *&pIns)
{
	pthread_mutex_lock(&g_mutexServiceLifecycle);
	if (g_CWebSockServerObj == pIns)
	{
		delete g_CWebSockServerObj;
		g_CWebSockServerObj = nullptr;
		pIns = nullptr;
	}
	pthread_mutex_unlock(&g_mutexServiceLifecycle);
}


