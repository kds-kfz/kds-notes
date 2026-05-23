#include "WebSockServerObj.h"
#include "publicGlobalvar.h"
#include "publicfunc.h"
#include "Log.h"
#include <windows.h>

namespace
{
	bool g_bWebMutexInit = false;

	// wyl 2026-03-30：集中管理 Web 服务运行时锁，避免重复启停时出现未初始化或重复销毁。
	void InitWebMutexes()
	{
		if (g_bWebMutexInit)
			return;

		pthread_mutex_init(&g_mutexWebConnet, nullptr);
		pthread_mutex_init(&g_mutexWebTask, nullptr);
		pthread_mutex_init(&g_mutexWebReq, nullptr);
		g_bWebMutexInit = true;
	}

	void DestroyWebMutexes()
	{
		if (!g_bWebMutexInit)
			return;

		pthread_mutex_destroy(&g_mutexWebConnet);
		pthread_mutex_destroy(&g_mutexWebTask);
		pthread_mutex_destroy(&g_mutexWebReq);
		g_bWebMutexInit = false;
	}

	// wyl 2026-03-30：统一清空 Web 连接、任务和缓存状态，避免旧状态残留到下一次启动。
	void ClearWebRuntimeData()
	{
		if (!g_bWebMutexInit)
			return;

		pthread_mutex_lock(&g_mutexWebConnet);
		g_mapWebClient.clear();
		g_setWebLocalClosing.clear();
		pthread_mutex_unlock(&g_mutexWebConnet);

		pthread_mutex_lock(&g_mutexWebTask);
		foreach(g_mapWebTask, it_task)
		{
			delete it_task->second;
		}
		g_mapWebTask.clear();
		pthread_mutex_unlock(&g_mutexWebTask);

		pthread_mutex_lock(&g_mutexWebReq);
		foreach(g_mapWebQueue, it_queue)
		{
			delete it_queue->second;
		}
		g_mapWebQueue.clear();
		pthread_mutex_unlock(&g_mutexWebReq);
	}

	bool HpSocketIsConnectedNoThrow(IHttpServer *pSender, CONNID dwConnID)
	{
		if (nullptr == pSender)
			return false;

		BOOL bAlive = FALSE;
		DWORD dwExceptionCode = 0;
		__try
		{
			bAlive = pSender->HasStarted() && pSender->IsConnected(dwConnID);
		}
		__except (dwExceptionCode = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
		{
			WEB_ERROR("ConnID=%llu,WebSockIsAliveException=0x%08X", (unsigned long long)dwConnID, dwExceptionCode);
			return false;
		}
		return bAlive ? true : false;
	}

	bool IsWebSocketSendable(IHttpServer *pSender, CONNID dwConnID)
	{
		if (nullptr == pSender || !g_bWebMutexInit)
			return false;

		bool bMapAlive = false;
		pthread_mutex_lock(&g_mutexWebConnet);
		std::map<CONNID, ClientData>::iterator itClient = g_mapWebClient.find(dwConnID);
		// wyl 2026-05-19：发送前同时校验本地连接表和 HP-Socket 状态，避免关闭通知排队期间继续推送旧 ConnID。
		bMapAlive = g_bWebServerStatus
			&& pSender == g_CWebPackServer
			&& itClient != g_mapWebClient.end()
			&& itClient->second.bConnected
			&& g_setWebLocalClosing.find(dwConnID) == g_setWebLocalClosing.end();
		pthread_mutex_unlock(&g_mutexWebConnet);

		return bMapAlive && HpSocketIsConnectedNoThrow(pSender, dwConnID);
	}

	bool SendWSMessageNoThrow(IHttpServer *pSender, CONNID dwConnID, BYTE iOperationCode,
		const BYTE *pData, int iLength, ULONGLONG ullBodyLen, const char *p_szAction)
	{
		if (nullptr == pSender)
			return false;

		BOOL bSendOK = FALSE;
		DWORD dwExceptionCode = 0;
		__try
		{
			bSendOK = pSender->SendWSMessage(dwConnID, true, 0, iOperationCode, pData, iLength, ullBodyLen);
		}
		__except (dwExceptionCode = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
		{
			// wyl 2026-05-19：第三方网络库发送异常不能让进程无日志退出，至少落下 ConnID、动作和异常码。
			WEB_ERROR("ConnID=%llu,%sException=0x%08X,Opcode=%u,SendDataLen=%d",
				(unsigned long long)dwConnID, nullptr != p_szAction ? p_szAction : "SendWSMessage",
				dwExceptionCode, (unsigned int)iOperationCode, iLength);
			return false;
		}
		return bSendOK ? true : false;
	}
}

void DeleteWebObj(void)
{
	// wyl 2026-03-30：停服时先拉低 Web 运行状态，再停止服务和线程池，避免回调继续进入无效状态。
	g_bWebServerStatus = false;

	if (nullptr != g_CWebPackServer)
	{
		g_CWebPackServer->Stop();
		g_CWebPackServer->Wait(INFINITE);
	}

	// wyl 2026-04-15：先等待线程池中已排队的 Web 回调自然退出，再销毁底层 server，
	// 避免同进程并行服务或停服边界下，上层回调拿到已经失效的 p_refServerHandle。
	if (nullptr != g_CWebHPThreadPool)
	{
		g_CWebHPThreadPool->Stop();
	}

	ClearWebRuntimeData();

	if (nullptr != g_CWebPackServer)
	{
		HP_Destroy_HttpServer(g_CWebPackServer);
		g_CWebPackServer = nullptr;
	}

	if (nullptr != g_CWebServerListerNet)
	{
		delete g_CWebServerListerNet;
		g_CWebServerListerNet = nullptr;
	}
	DestroyWebMutexes();

	g_pWebHandle = nullptr;
	g_ullWebTaskID = 0;

	CWebLog::Release();
}

int CWebSockServerObj::WebSockCompare(void* p_refSrcClient, void* p_refObjClient)
{
	if (nullptr == p_refSrcClient || nullptr == p_refObjClient)
		return -1;

	CONNID dwSrcConnID = (CONNID)p_refSrcClient;
	CONNID dwObjConnID = (CONNID)p_refObjClient;
	return dwSrcConnID == dwObjConnID ? 0 : 1;
}

CWebSockServerObj::CWebSockServerObj()
{

}

CWebSockServerObj::~CWebSockServerObj()
{
	DeleteWebObj();
}

bool CWebSockServerObj::CreateWebSock(const char *p_szIp, unsigned short p_unPort, unsigned int p_uiRBufLen, unsigned int p_uiMaxConnectNum, unsigned int p_uiMaxAcceptNum,
	WEB_NOTIFY_PROC p_webHandle, unsigned int p_uiThreadNum, unsigned int p_uiQueueNum, char *p_szErr, const char *p_szLogFold)
{
	pthread_mutex_lock(&g_mutexServiceLifecycle);

	if (nullptr == p_szErr)
	{
		pthread_mutex_unlock(&g_mutexServiceLifecycle);
		return false;
	}

	if (nullptr == p_szIp || 7 > strlen(p_szIp))
	{
		_snprintf(p_szErr, 1024, "code=-1,msg=init param err");
		pthread_mutex_unlock(&g_mutexServiceLifecycle);
		return false;
	}

	// wyl 2026-03-30：启动前先清理旧实例残留，避免重复启动时复用脏状态。
	DeleteWebObj();

	int iRet = 0;

	//1.初始化日志
	if (nullptr != p_szLogFold && strlen(p_szLogFold) > 0)
	{
		// wyl 2026-03-30：Web 服务使用独立日志单例，避免与 TCP 服务共享日志路径和生命周期。
		if (MA_OK == CWebLog::GetInstance()->InitLog(p_szLogFold))
		{
			CWebLog::GetInstance()->Resume();//恢复工作
			WEB_INFO("log started");
		}
		else
		{
			_snprintf(p_szErr, 1024, "code=-2,msg=log init fail");
			DeleteWebObj();
			pthread_mutex_unlock(&g_mutexServiceLifecycle);
			return false;
		}
		// 设置日志等级
		CWebLog::GetInstance()->SetLogLevel((char *)"info");
	}

	// 设置任务回调
	g_pWebHandle = p_webHandle;

	// wyl 2026-03-30：先初始化锁和线程池，再启动网络监听，减少启动窗口期竞态。
	InitWebMutexes();

	//2.设置线程池
	g_CWebHPThreadPool->AdjustThreadCount(p_uiThreadNum);
	if (!g_CWebHPThreadPool->Start(p_uiThreadNum, p_uiQueueNum, TRP_CALL_FAIL, 0))
	{
		_snprintf(p_szErr, 1024, "code=%d,msg=thread pool start fail", SYS_GetLastError());
		DeleteWebObj();
		pthread_mutex_unlock(&g_mutexServiceLifecycle);
		return false;
	}

	//3.创建服务监听器
	if (nullptr == g_CWebServerListerNet)
	{
		g_CWebServerListerNet = new CWebServerListerNet();
	}

	if (nullptr == g_CWebServerListerNet)
	{
		iRet = SYS_GetLastError();
		_snprintf(p_szErr, 1024, "code=%d,msg=create web server lister fail", iRet);
		DeleteWebObj();
		pthread_mutex_unlock(&g_mutexServiceLifecycle);
		return false;
	}

	//4.创建服务
	if (nullptr == g_CWebPackServer)
	{
		g_CWebPackServer = HP_Create_HttpServer(g_CWebServerListerNet);
	}

	if (nullptr == g_CWebPackServer)
	{
		_snprintf(p_szErr, 1024, "code=%d,msg=create web server fail", SYS_GetLastError());
		DeleteWebObj();
		pthread_mutex_unlock(&g_mutexServiceLifecycle);
		return false;
	}

	//5.设置超时心跳
	g_CWebPackServer->SetKeepAliveTime(2000);
	g_CWebPackServer->SetKeepAliveInterval(1000);

	//6.设置缓存大小
	g_CWebPackServer->SetSocketBufferSize(p_uiRBufLen);

	//7.设置最大连接数
	g_CWebPackServer->SetMaxConnectionCount(p_uiMaxConnectNum);

	// wyl 2026-03-30：这里设置的是底层 Accept 预分配数量，不是“同一 IP 最大连接数”限流。
	//8.设置Accept大小
	g_CWebPackServer->SetAcceptSocketCount(p_uiMaxAcceptNum);

	// wyl 2026-03-30：资源准备完成后再标记 Web 服务可运行，供回调路径做状态保护。
	g_bWebServerStatus = true;

	//9.启动服务
	if (!g_CWebPackServer->Start(p_szIp, p_unPort))
	{
		char szErrDesc[256] = { 0 };
		SafeCopyCString(szErrDesc, sizeof(szErrDesc), g_CWebPackServer->GetLastErrorDesc());
		_snprintf(p_szErr, 1024, "code=%d,msg=%s",
			g_CWebPackServer->GetLastError(), szErrDesc);
		DeleteWebObj();
		pthread_mutex_unlock(&g_mutexServiceLifecycle);
		return false;
	}

	WEB_INFO("server started");
	pthread_mutex_unlock(&g_mutexServiceLifecycle);
	return true;
}

bool CWebSockServerObj::CreateWssSock(const char*, unsigned short, unsigned int, unsigned int, unsigned int,
	WEB_NOTIFY_PROC, unsigned int, unsigned int, char* p_szErr,
	const char*, const char*,
	const char*, const char*,
	const char*)
{
	// wyl 2026-03-30：当前版本尚未实现 WSS 建链和证书装载，必须明确返回失败，避免上层误判服务已启动成功。
	if (nullptr != p_szErr)
	{
		_snprintf(p_szErr, 1024, "code=-3,msg=wss not implement");
	}
	return false;
}

void CWebSockServerObj::StopWebSock()
{
	pthread_mutex_lock(&g_mutexServiceLifecycle);
	DeleteWebObj();
	pthread_mutex_unlock(&g_mutexServiceLifecycle);
}

void CWebSockServerObj::WebSockClose(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen)
{
	if (nullptr == p_refServer || nullptr == p_refClient)
		return;

	IHttpServer *pSender = (IHttpServer *)p_refServer;
	CONNID dwConnID = (CONNID)p_refClient;

	bool bCanClose = IsWebSocketSendable(pSender, dwConnID);
	if (g_bWebMutexInit && bCanClose)
	{
		pthread_mutex_lock(&g_mutexWebConnet);
		std::map<CONNID, ClientData>::iterator itClient = g_mapWebClient.find(dwConnID);
		if (itClient != g_mapWebClient.end())
		{
			// wyl 2026-05-19：本端主动关闭开始时立即撤销可发送状态，阻止其它业务线程继续推送同一连接。
			itClient->second.bConnected = false;
		}
		// wyl 2026-03-30：显式标记“本端主动断开”，不要再依赖 OnClose 里的操作类型猜测关闭来源。
		g_setWebLocalClosing.insert(dwConnID);
		pthread_mutex_unlock(&g_mutexWebConnet);
	}

	if (!bCanClose)
	{
		WEB_WARN("ConnID=%llu,SkipCloseWebSocketClosed", (unsigned long long)dwConnID);
		return;
	}

	// wyl 2026-03-30：WebSocket 关闭前如果有业务数据，按二进制消息帧发送，不再错误地回 HTTP 响应。
	if (nullptr != p_szData && p_iDataLen > 0)
	{
		if (!SendWSMessageNoThrow(pSender, dwConnID, 2, (const BYTE *)p_szData, p_iDataLen, p_iDataLen, "SendCloseData"))
		{
			WEB_ERROR("ConnID=%llu,SendCloseDataFail,err=%d", (unsigned long long)dwConnID, SYS_GetLastError());
		}
	}

	// wyl 2026-03-30：主动关闭时补发 WebSocket Close 帧，避免客户端把关闭过程识别成协议错误。
	if (!SendWSMessageNoThrow(pSender, dwConnID, 8, nullptr, 0, 0, "SendCloseFrame"))
	{
		WEB_WARN("ConnID=%llu,SendCloseFrameFail,err=%d", (unsigned long long)dwConnID, SYS_GetLastError());
	}

	// wyl 2026-03-30：改为优雅断开，让前面已经排队的 WebSocket 数据帧和 Close 帧有机会发出。
	if (!pSender->Disconnect(dwConnID, false) && g_bWebMutexInit)
	{
		// wyl 2026-03-30：如果主动断开失败，主动回收本次标记和缓存，避免状态残留。
		pthread_mutex_lock(&g_mutexWebConnet);
		g_setWebLocalClosing.erase(dwConnID);
		g_mapWebClient.erase(dwConnID);
		pthread_mutex_unlock(&g_mutexWebConnet);

		pthread_mutex_lock(&g_mutexWebReq);
		if (g_mapWebQueue.find(dwConnID) != g_mapWebQueue.end())
		{
			delete g_mapWebQueue[dwConnID];
			g_mapWebQueue.erase(dwConnID);
		}
		pthread_mutex_unlock(&g_mutexWebReq);
	}
}

bool CWebSockServerObj::WebSockSend(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen)
{
	if (nullptr == p_refServer || nullptr == p_refClient || nullptr == p_szData || 0 >= p_iDataLen)
		return false;

	IHttpServer *pSender = (IHttpServer *)p_refServer;
	CONNID dwConnID = (CONNID)p_refClient;

	if (!IsWebSocketSendable(pSender, dwConnID))
	{
		WEB_WARN("ConnID=%llu,SkipSendWebSocketClosed,SendDataLen=%d", (unsigned long long)dwConnID, p_iDataLen);
		return false;
	}

	// wyl 2026-03-30：WebSocket 对外发送统一走消息帧接口，当前按二进制帧发送以匹配上层“原始字节块”语义。
	bool bSendOK = SendWSMessageNoThrow(pSender, dwConnID, 2, (const BYTE *)p_szData, p_iDataLen, p_iDataLen, "SendWSMessage");

	if (bSendOK)
	{
		WEB_INFO("ConnID=%llu,SendDataLen=%d", (unsigned long long)dwConnID, p_iDataLen);
	}
	else
	{
		WEB_ERROR("ConnID=%llu,SendDataLen=%d,err=%d", (unsigned long long)dwConnID, p_iDataLen, SYS_GetLastError());
	}
	return bSendOK;
}

bool CWebSockServerObj::WebSockIsAlive(void *p_refServer, void *p_refClient)
{
	if (nullptr == p_refServer || nullptr == p_refClient)
		return false;

	IHttpServer *pSender = (IHttpServer *)p_refServer;
	CONNID dwConnID = (CONNID)p_refClient;
	return IsWebSocketSendable(pSender, dwConnID);
}

