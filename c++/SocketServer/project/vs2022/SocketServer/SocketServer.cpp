#include "publicGlobalvar.h"

/********** TCP服务模块 **********/
CSocketServer* CreateTcpSockInstance()
{
	if (nullptr == g_CTcpSockServerObj)
	{
		g_CTcpSockServerObj = new CTcpSockServerObj();
	}

	return g_CTcpSockServerObj;
}

void DelTcpSockInstance(CSocketServer *&pIns)
{
	if (g_CTcpSockServerObj == pIns)
	{
		delete g_CTcpSockServerObj;
		g_CTcpSockServerObj = nullptr;
		pIns = nullptr;
	}
}

/********** HTTP服务模块 **********/
CSocketServer* CreateHttpSockInstance()
{
	return nullptr;
}

void DelHttpSockInstance(CSocketServer *&pIns)
{

}

/********** WEBSOCKET服务模块 **********/
CSocketServer* CreateWebSockInstance()
{
	return nullptr;
}

void DelWebSockInstance(CSocketServer *&pIns)
{

}

