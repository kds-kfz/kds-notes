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
	if (nullptr == g_CHttpSockServerObj)
	{
		g_CHttpSockServerObj = new CHttpSockServerObj();
	}

	return g_CHttpSockServerObj;
}

void DelHttpSockInstance(CSocketServer *&pIns)
{
	if (g_CHttpSockServerObj == pIns)
	{
		delete g_CHttpSockServerObj;
		g_CHttpSockServerObj = nullptr;
		pIns = nullptr;
	}
}

/********** WEBSOCKET服务模块 **********/
CSocketServer* CreateWebSockInstance()
{
	if (nullptr == g_CWebSockServerObj)
	{
		g_CWebSockServerObj = new CWebSockServerObj();
	}

	return g_CWebSockServerObj;
}

void DelWebSockInstance(CSocketServer *&pIns)
{
	if (g_CWebSockServerObj == pIns)
	{
		delete g_CWebSockServerObj;
		g_CWebSockServerObj = nullptr;
		pIns = nullptr;
	}
}


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
	if (nullptr == g_CWebSockServerObj)
	{
		g_CWebSockServerObj = new CWebSockServerObj();
	}

	return g_CWebSockServerObj;
}

void DelWebSockInstance(CSocketServer *&pIns)
{
	if (g_CWebSockServerObj == pIns)
	{
		delete g_CWebSockServerObj;
		g_CWebSockServerObj = nullptr;
		pIns = nullptr;
	}
}

