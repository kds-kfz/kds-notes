#include "TcpSockServerObj.h"
#include "publicGlobalvar.h"
#include "publicfunc.h"
#include "Log.h"

void DeleteObj(void)
{
	g_CHPThreadPool->Stop(0);

	if (nullptr != g_CTcpPackServer)
	{
		g_CTcpPackServer->Stop();
		delete g_CTcpPackServer;
		g_CTcpPackServer = NULL;
	}

	if (nullptr != g_CTcpServerListerNet)
	{
		delete g_CTcpServerListerNet;
		g_CTcpServerListerNet = nullptr;
	}

	CLog::Release();

	if(!g_bServerStatus)
		return;

	pthread_mutex_lock(&g_mutexConnet);
	g_mapClient.clear();
	pthread_mutex_unlock(&g_mutexConnet);

	pthread_mutex_lock(&g_mutexTask);
	foreach(g_mapTask, it_task)
	{
		delete it_task->second;
	}
	pthread_mutex_unlock(&g_mutexTask);

	pthread_mutex_lock(&g_mutexReq);
	foreach(g_mapQueue, it_queue)
	{
		delete it_queue->second;
	}
	pthread_mutex_unlock(&g_mutexReq);

	pthread_mutex_destroy(&g_mutexConnet);
	pthread_mutex_destroy(&g_mutexTask);
	pthread_mutex_destroy(&g_mutexReq);

	//INFO("%s 释放完成", g_strServerName.c_str());
	//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	//Sleep(1000);
	//CLog::Release();
}

CTcpSockServerObj::CTcpSockServerObj()
{

}

CTcpSockServerObj::~CTcpSockServerObj()
{
	DeleteObj();
}

int CTcpSockServerObj::TcpSockCompare(void *p_refSrcClient, void *p_refObjClient)
{
	if(nullptr == p_refSrcClient || nullptr == p_refObjClient)
		return -1;

	CONNID dwSrcConnID = (CONNID)p_refSrcClient;
	CONNID dwObjConnID = (CONNID)p_refObjClient;
	return dwSrcConnID == dwObjConnID ? 0 : 1;
}

bool CTcpSockServerObj::CreateTcpSock(const char *p_szIp, unsigned short p_unPort, unsigned int p_uiRBufLen, unsigned int p_uiMaxConnectNum, unsigned int p_uiMaxAcceptNum,
	TCP_NOTIFY_PROC p_tcpHandle, unsigned int p_uiThreadNum, unsigned int p_uiQueueNum, char *p_szErr, const char *p_szLogFold)
{
	if (nullptr == p_szIp || 7 > strlen(p_szIp) || nullptr == p_szErr)
	{
#if defined( OS_IS_WINDOWS )
		_snprintf(p_szErr, 1024, "code=-1,msg=init param err");
#else
		snprintf(p_szErr, 1024, "code=-1,msg=init param err");
#endif
		DeleteObj();
		return false;
	}

	int iRet = 0;

	//1.初始化日志
	if (nullptr != p_szLogFold && strlen(p_szLogFold) > 0)
	{
		if (MA_OK == CLog::GetInstance()->InitLog(p_szLogFold))
		{
			CLog::GetInstance()->Resume();//恢复工作
			INFO("启动LOG  ****************************");
		}
		else
		{
#if defined( OS_IS_WINDOWS )
			_snprintf(p_szErr, 1024, "code=-2,msg=log init fail");
#else
			snprintf(p_szErr, 1024, "code=-2,msg=log init fail");
#endif
			DeleteObj();
			return false;
		}
		// 设置日志等级
		CLog::GetInstance()->SetLogLevel((char *)"info");
	}

	// 设置任务回调
	g_pTcpHandle = p_tcpHandle;

	//2.设置线程池
	g_CHPThreadPool->AdjustThreadCount(p_uiThreadNum);

	//3.创建服务监听器
	if (nullptr == g_CTcpServerListerNet)
	{
		g_CTcpServerListerNet = new CTcpServerListerNet();
	}

	if (nullptr == g_CTcpServerListerNet)
	{
		iRet = SYS_GetLastError();
#if defined( OS_IS_WINDOWS )
		_snprintf(p_szErr, 1024, "code=%d,msg=create tcp server lister fail", iRet);
#else
		snprintf(p_szErr, 1024, "code=%d,msg=create tcp server lister fail", iRet);
#endif
		DeleteObj();
		return false;
	}

	//4.创建服务
	if (nullptr == g_CTcpPackServer)
	{
		g_CTcpPackServer = HP_Create_TcpServer(g_CTcpServerListerNet);
		//g_CTcpPackServer = HP_Create_TcpServer(g_CTcpServerListerNet);
	}

	if (nullptr == g_CTcpPackServer)
	{
#if defined( OS_IS_WINDOWS )
		_snprintf(p_szErr, 1024, "code=%d,msg=create tcp server fail", SYS_GetLastError());
#else
		snprintf(p_szErr, 1024, "code=%d,msg=create tcp server fail", SYS_GetLastError());
#endif
		DeleteObj();
		return false;
	}

	//5.设置超时心跳
	g_CTcpPackServer->SetKeepAliveTime(2000);
	g_CTcpPackServer->SetKeepAliveInterval(1000);

	//6.设置缓存大小
	g_CTcpPackServer->SetSocketBufferSize(p_uiRBufLen);

	//7.设置最大连接数
	g_CTcpPackServer->SetMaxConnectionCount(p_uiMaxConnectNum);

	//8.设置Accept大小
	g_CTcpPackServer->SetAcceptSocketCount(p_uiMaxAcceptNum);

	//9.启动服务
	if (!g_CTcpPackServer->Start(p_szIp, p_unPort))
	{
#if defined( OS_IS_WINDOWS )
		_snprintf(p_szErr, 1024, "code=%d,msg=%s",
			g_CTcpPackServer->GetLastError(), g_CTcpPackServer->GetLastErrorDesc());
#else
		snprintf(p_szErr, 1024, "code=%d,msg=%s",
			g_CTcpPackServer->GetLastError(), g_CTcpPackServer->GetLastErrorDesc());
#endif
		DeleteObj();
		return false;
	}

	//10.启动服务 要先启动线程池
	if (!g_CHPThreadPool->Start(p_uiThreadNum, p_uiQueueNum, TRP_CALL_FAIL, 0))
	{
#if defined( OS_IS_WINDOWS )
		_snprintf(p_szErr, 1024, "code=%d,msg=thread pool start fail", SYS_GetLastError());
#else
		snprintf(p_szErr, 1024, "code=%d,msg=thread pool start fail", SYS_GetLastError());
#endif
		DeleteObj();
		return false;
	}

	pthread_mutex_init(&g_mutexConnet, nullptr);
	pthread_mutex_init(&g_mutexTask, nullptr);
	pthread_mutex_init(&g_mutexReq, nullptr);

	INFO("%s 启动完成", g_strServerName.c_str());
	return true;
}

void CTcpSockServerObj::StopTcpSock()
{
	DeleteObj();
}

void CTcpSockServerObj::TcpSockSend(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen)
{
	if (nullptr == p_refServer || nullptr == p_refClient || nullptr == p_szData || 0 >= p_iDataLen)
		return;

	ITcpServer *pSender = (ITcpServer *)p_refServer;
	CONNID dwConnID = (CONNID)p_refClient;

	printf("%lu,SendDataLen=%d\n", dwConnID, p_iDataLen);
	if(pSender->Send(dwConnID, (BYTE *)p_szData, p_iDataLen))
	{
		printf("%lu,SendDataLen=%d,ok\n", dwConnID, p_iDataLen);
	}
	else
	{
		printf("%lu,SendDataLen=%d,err=%d\n", dwConnID, p_iDataLen, SYS_GetLastError());
	}
}

void CTcpSockServerObj::TcpSockClose(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen)
{
	if (nullptr == p_refServer || nullptr == p_refClient || nullptr == p_szData || 0 >= p_iDataLen)
		return;

	ITcpServer *pSender = (ITcpServer *)p_refServer;
	CONNID dwConnID = (CONNID)p_refClient;
	pSender->Send(dwConnID, (BYTE *)p_szData, p_iDataLen);

	pthread_mutex_lock(&g_mutexConnet);
	if (g_mapClient.find(dwConnID) != g_mapClient.end())
	{
		g_mapClient.erase(dwConnID);
	}
	pthread_mutex_unlock(&g_mutexConnet);

	pthread_mutex_lock(&g_mutexReq);
	if (g_mapQueue.find(dwConnID) != g_mapQueue.end())
	{
		delete g_mapQueue[dwConnID];
		g_mapQueue.erase(dwConnID);
	}
	pthread_mutex_unlock(&g_mutexReq);

	pSender->Disconnect(dwConnID);
}
