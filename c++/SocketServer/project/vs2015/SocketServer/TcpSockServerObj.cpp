#include "TcpSockServerObj.h"
#include "publicGlobalvar.h"
#include "publicfunc.h"
#include "Log.h"

namespace
{
	bool g_bTcpMutexInit = false;

	// wyl 2026-03-30：集中管理 TCP 运行时锁，避免重复启停时出现未初始化或重复销毁。
	void InitTcpMutexes()
	{
		if (g_bTcpMutexInit)
			return;

		pthread_mutex_init(&g_mutexConnet, nullptr);
		pthread_mutex_init(&g_mutexTask, nullptr);
		pthread_mutex_init(&g_mutexReq, nullptr);
		g_bTcpMutexInit = true;
	}

	void DestroyTcpMutexes()
	{
		if (!g_bTcpMutexInit)
			return;

		pthread_mutex_destroy(&g_mutexConnet);
		pthread_mutex_destroy(&g_mutexTask);
		pthread_mutex_destroy(&g_mutexReq);
		g_bTcpMutexInit = false;
	}

	// wyl 2026-03-30：统一清空连接、任务和请求缓存，避免旧状态残留到下一次启动。
	void ClearTcpRuntimeData()
	{
		if (!g_bTcpMutexInit)
			return;

		pthread_mutex_lock(&g_mutexConnet);
		g_mapClient.clear();
		g_setTcpLocalClosing.clear();
		pthread_mutex_unlock(&g_mutexConnet);

		pthread_mutex_lock(&g_mutexTask);
		foreach(g_mapTask, it_task)
		{
			delete it_task->second;
		}
		g_mapTask.clear();
		pthread_mutex_unlock(&g_mutexTask);

		pthread_mutex_lock(&g_mutexReq);
		foreach(g_mapQueue, it_queue)
		{
			delete it_queue->second;
		}
		g_mapQueue.clear();
		pthread_mutex_unlock(&g_mutexReq);
	}
}

void DeleteObj(void)
{
	// wyl 2026-03-30：停服时先拉低运行状态，再停止服务和线程池，避免回调继续进入无效状态。
	g_bServerStatus = false;

	if (nullptr != g_CTcpPackServer)
	{
		g_CTcpPackServer->Stop();
		HP_Destroy_TcpServer(g_CTcpPackServer);
		g_CTcpPackServer = NULL;
	}

	// wyl 2026-03-30：线程池关闭改为等待已提交任务自然退出，避免强制停池后马上清理任务对象导致悬空指针。
	g_CTcpHPThreadPool->Stop();

	if (nullptr != g_CTcpServerListerNet)
	{
		delete g_CTcpServerListerNet;
		g_CTcpServerListerNet = nullptr;
	}

	ClearTcpRuntimeData();
	DestroyTcpMutexes();

	g_pTcpHandle = nullptr;
	g_ullTaskID = 0;

	CTcpLog::Release();
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
	if (nullptr == p_szErr)
		return false;

	if (nullptr == p_szIp || 7 > strlen(p_szIp))
	{
		_snprintf(p_szErr, 1024, "code=-1,msg=init param err");
		return false;
	}

	// wyl 2026-03-30：启动前先清理旧实例残留，避免重复启动时复用脏状态。
	DeleteObj();

	int iRet = 0;

	//1.初始化日志
	if (nullptr != p_szLogFold && strlen(p_szLogFold) > 0)
	{
		// wyl 2026-03-30：TCP 服务使用独立日志单例，避免与 Web 服务共享路径和生命周期。
		if (MA_OK == CTcpLog::GetInstance()->InitLog(p_szLogFold))
		{
			CTcpLog::GetInstance()->Resume();//恢复工作
			TCP_INFO("启动LOG  ****************************");
		}
		else
		{
			_snprintf(p_szErr, 1024, "code=-2,msg=log init fail");
			DeleteObj();
			return false;
		}
		// 设置日志等级
		CTcpLog::GetInstance()->SetLogLevel((char *)"info");
	}

	// 设置任务回调
	g_pTcpHandle = p_tcpHandle;

	// wyl 2026-03-30：先初始化锁和线程池，再启动网络监听，减少启动窗口期竞态。
	InitTcpMutexes();

	//2.设置线程池
	g_CTcpHPThreadPool->AdjustThreadCount(p_uiThreadNum);
	if (!g_CTcpHPThreadPool->Start(p_uiThreadNum, p_uiQueueNum, TRP_CALL_FAIL, 0))
	{
		_snprintf(p_szErr, 1024, "code=%d,msg=thread pool start fail", SYS_GetLastError());
		DeleteObj();
		return false;
	}

	//3.创建服务监听器
	if (nullptr == g_CTcpServerListerNet)
	{
		g_CTcpServerListerNet = new CTcpServerListerNet();
	}

	if (nullptr == g_CTcpServerListerNet)
	{
		iRet = SYS_GetLastError();
		_snprintf(p_szErr, 1024, "code=%d,msg=create tcp server lister fail", iRet);
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
		_snprintf(p_szErr, 1024, "code=%d,msg=create tcp server fail", SYS_GetLastError());
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

	// wyl 2026-03-30：这里设置的是底层 Accept 预分配数量，不是“同一 IP 最大连接数”限流。
	//8.设置Accept大小
	g_CTcpPackServer->SetAcceptSocketCount(p_uiMaxAcceptNum);

	// wyl 2026-03-30：资源准备完成后再标记服务可运行，供回调路径做状态保护。
	g_bServerStatus = true;

	//9.启动服务
	const std::basic_string<TCHAR> strBindAddress = MakeBindAddress(p_szIp);
	if (!g_CTcpPackServer->Start(strBindAddress.c_str(), p_unPort))
	{
		char szErrDesc[256] = { 0 };
		CopyTextToAnsi(szErrDesc, sizeof(szErrDesc), g_CTcpPackServer->GetLastErrorDesc());
		_snprintf(p_szErr, 1024, "code=%d,msg=%s",
			g_CTcpPackServer->GetLastError(), szErrDesc);
		DeleteObj();
		return false;
	}

	TCP_INFO("启动完成");
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

	if(pSender->Send(dwConnID, (BYTE *)p_szData, p_iDataLen))
	{
		TCP_INFO("ConnID=%llu,SendDataLen=%d", (unsigned long long)dwConnID, p_iDataLen);
	}
	else
	{
		TCP_ERROR("ConnID=%llu,SendDataLen=%d,err=%d", (unsigned long long)dwConnID, p_iDataLen, SYS_GetLastError());
	}
}

void CTcpSockServerObj::TcpSockClose(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen)
{
	if (nullptr == p_refServer || nullptr == p_refClient)
		return;

	ITcpServer *pSender = (ITcpServer *)p_refServer;
	CONNID dwConnID = (CONNID)p_refClient;
	// wyl 2026-03-30：允许无数据直接断连，发送应答和关闭连接不再强耦合。
	if (nullptr != p_szData && p_iDataLen > 0)
	{
		if (!pSender->Send(dwConnID, (BYTE *)p_szData, p_iDataLen))
		{
			TCP_WARN("ConnID=%llu,SendCloseDataFail,err=%d", (unsigned long long)dwConnID, SYS_GetLastError());
		}
	}

	if (g_bTcpMutexInit)
	{
		pthread_mutex_lock(&g_mutexConnet);
		// wyl 2026-03-30：显式标记“本端主动断开”，不要再依赖 OnClose 里的系统错误码猜测关闭来源。
		g_setTcpLocalClosing.insert(dwConnID);
		pthread_mutex_unlock(&g_mutexConnet);
	}

	// wyl 2026-03-30：改为优雅断开，让关闭前已经排队的最后一包数据有机会真正发出。
	if (!pSender->Disconnect(dwConnID, false) && g_bTcpMutexInit)
	{
		// wyl 2026-03-30：如果断开调用失败，主动回收本次标记和缓存，避免状态残留。
		pthread_mutex_lock(&g_mutexConnet);
		g_setTcpLocalClosing.erase(dwConnID);
		g_mapClient.erase(dwConnID);
		pthread_mutex_unlock(&g_mutexConnet);

		pthread_mutex_lock(&g_mutexReq);
		if (g_mapQueue.find(dwConnID) != g_mapQueue.end())
		{
			delete g_mapQueue[dwConnID];
			g_mapQueue.erase(dwConnID);
		}
		pthread_mutex_unlock(&g_mutexReq);
	}
}
