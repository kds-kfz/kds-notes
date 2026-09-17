#include "TcpSockServerObj.h"
#include "publicGlobalvar.h"
#include "publicfunc.h"
#include "ServerRuntimeContext.h"
#include "Log.h"
#include <windows.h>

namespace
{
	// wyl 2026-03-30：统一清空连接、任务和请求缓存，避免旧状态残留到下一次启动。
	void ClearTcpRuntimeData(ST_TCP_SERVER_RUNTIME* p_pRuntime)
	{
		if (p_pRuntime == nullptr)
		{
			return;
		}

		pthread_mutex_lock(&p_pRuntime->mutexConnection);
		p_pRuntime->mapClient.clear();
		p_pRuntime->setLocalClosing.clear();
		pthread_mutex_unlock(&p_pRuntime->mutexConnection);

		pthread_mutex_lock(&p_pRuntime->mutexTask);
		foreach(p_pRuntime->mapTask, it_task)
		{
			delete it_task->second;
		}
		p_pRuntime->mapTask.clear();
		pthread_mutex_unlock(&p_pRuntime->mutexTask);

		pthread_mutex_lock(&p_pRuntime->mutexRequest);
		foreach(p_pRuntime->mapRequest, it_queue)
		{
			delete it_queue->second;
		}
		p_pRuntime->mapRequest.clear();
		pthread_mutex_unlock(&p_pRuntime->mutexRequest);
	}

	bool TcpSocketIsConnectedNoThrow(ITcpServer *pSender, CONNID dwConnID)
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
			TCP_ERROR("ConnID=%llu,TcpSockIsAliveException=0x%08X", (unsigned long long)dwConnID, dwExceptionCode);
			return false;
		}
		return bAlive ? true : false;
	}

	bool IsTcpSocketSendable(ST_TCP_SERVER_RUNTIME* p_pRuntime,
		ITcpServer *pSender, CONNID dwConnID)
	{
		if (p_pRuntime == nullptr || pSender == nullptr)
			return false;

		bool bMapAlive = false;
		pthread_mutex_lock(&p_pRuntime->mutexConnection);
		std::map<CONNID, ClientData>::iterator itClient =
			p_pRuntime->mapClient.find(dwConnID);
		// wyl 2026-05-19：发送前同时校验本地连接表和 HP-Socket 状态，避免关闭通知排队期间继续推送旧 ConnID。
		bMapAlive = p_pRuntime->bServerStatus.load()
			&& pSender == p_pRuntime->pPackServer
			&& itClient != p_pRuntime->mapClient.end()
			&& itClient->second.bConnected
			&& p_pRuntime->setLocalClosing.find(dwConnID) ==
				p_pRuntime->setLocalClosing.end();
		pthread_mutex_unlock(&p_pRuntime->mutexConnection);

		return bMapAlive && TcpSocketIsConnectedNoThrow(pSender, dwConnID);
	}

	bool SendTcpDataNoThrow(ITcpServer *pSender, CONNID dwConnID, const BYTE *pData, int iLength, const char *p_szAction)
	{
		if (nullptr == pSender || nullptr == pData || iLength <= 0)
			return false;

		BOOL bSendOK = FALSE;
		DWORD dwExceptionCode = 0;
		__try
		{
			bSendOK = pSender->Send(dwConnID, pData, iLength);
		}
		__except (dwExceptionCode = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
		{
			// wyl 2026-05-19：TCP 底层发送异常不能让进程无日志退出，至少落下 ConnID、动作和异常码。
			TCP_ERROR("ConnID=%llu,%sException=0x%08X,SendDataLen=%d",
				(unsigned long long)dwConnID, nullptr != p_szAction ? p_szAction : "TcpSend",
				dwExceptionCode, iLength);
			return false;
		}
		return bSendOK ? true : false;
	}

	bool DisconnectTcpNoThrow(ITcpServer *pSender, CONNID dwConnID, BOOL bForce, const char *p_szAction)
	{
		if (nullptr == pSender)
			return false;

		BOOL bOK = FALSE;
		DWORD dwExceptionCode = 0;
		__try
		{
			bOK = pSender->Disconnect(dwConnID, bForce);
		}
		__except (dwExceptionCode = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
		{
			TCP_ERROR("ConnID=%llu,%sException=0x%08X",
				(unsigned long long)dwConnID, nullptr != p_szAction ? p_szAction : "TcpDisconnect", dwExceptionCode);
			return false;
		}
		return bOK ? true : false;
	}

	// 停服只释放指定 TCP 实例，其他同协议实例继续独立运行。
	void DeleteObj(ST_TCP_SERVER_RUNTIME* p_pRuntime)
	{
		if (p_pRuntime == nullptr)
		{
			return;
		}
		p_pRuntime->bServerStatus.store(false);
		p_pRuntime->clIdentity.MarkStopped();
		if (p_pRuntime->pPackServer != nullptr)
		{
			p_pRuntime->pPackServer->Stop();
			HP_Destroy_TcpServer(p_pRuntime->pPackServer);
			p_pRuntime->pPackServer = nullptr;
		}
		p_pRuntime->clThreadPool->Stop();
		if (p_pRuntime->pListener != nullptr)
		{
			delete p_pRuntime->pListener;
			p_pRuntime->pListener = nullptr;
		}
		ClearTcpRuntimeData(p_pRuntime);
		p_pRuntime->pNotifyHandler = nullptr;
		p_pRuntime->ullTaskId = 0;
	}
}

CTcpSockServerObj::CTcpSockServerObj(const std::string& p_refServiceName,
	std::uint64_t p_ullInstanceId)
	: m_ptrRuntime(new ST_TCP_SERVER_RUNTIME(
		p_refServiceName, p_ullInstanceId))
{
}

CTcpSockServerObj::~CTcpSockServerObj()
{
	DeleteObj(m_ptrRuntime.get());
}

bool CTcpSockServerObj::FillRuntimeInfo(
	ST_SOCKET_SERVER_RUNTIME_INFO& p_refInfo) const
{
	if (m_ptrRuntime == nullptr)
	{
		return false;
	}
	std::lock_guard<std::mutex> clLifecycleLock(
		m_ptrRuntime->clLifecycleMutex);
	if (!m_ptrRuntime->clIdentity.Snapshot(p_refInfo))
	{
		return false;
	}
	if (m_ptrRuntime->pPackServer != nullptr)
	{
		p_refInfo.uiMaxConnectionCount =
			m_ptrRuntime->pPackServer->GetMaxConnectionCount();
		p_refInfo.uiAcceptSocketCount =
			m_ptrRuntime->pPackServer->GetAcceptSocketCount();
		p_refInfo.uiSocketListenQueue =
			m_ptrRuntime->pPackServer->GetSocketListenQueue();
	}
	return true;
}

bool CTcpSockServerObj::SetSocketListenQueue(
	unsigned int p_uiSocketListenQueue)
{
	if (m_ptrRuntime == nullptr || p_uiSocketListenQueue == 0 ||
		p_uiSocketListenQueue > 65535U)
	{
		return false;
	}
	std::lock_guard<std::mutex> clLifecycleLock(
		m_ptrRuntime->clLifecycleMutex);
	if (m_ptrRuntime->bServerStatus.load() ||
		m_ptrRuntime->pPackServer != nullptr)
	{
		return false;
	}
	m_ptrRuntime->uiSocketListenQueue = p_uiSocketListenQueue;
	return true;
}

// 以下别名只在 Server 成员函数内展开为当前实例字段。
#define g_bTcpMutexInit (m_ptrRuntime != nullptr)
#define g_bServerStatus (m_ptrRuntime->bServerStatus)
#define g_CTcpHPThreadPool (m_ptrRuntime->clThreadPool)
#define g_CTcpPackServer (m_ptrRuntime->pPackServer)
#define g_CTcpServerListerNet (m_ptrRuntime->pListener)
#define g_pTcpHandle (m_ptrRuntime->pNotifyHandler)
#define g_ullTaskID (m_ptrRuntime->ullTaskId)
#define g_mutexConnet (m_ptrRuntime->mutexConnection)
#define g_mapClient (m_ptrRuntime->mapClient)
#define g_setTcpLocalClosing (m_ptrRuntime->setLocalClosing)
#define g_mutexReq (m_ptrRuntime->mutexRequest)
#define g_mapQueue (m_ptrRuntime->mapRequest)

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
	std::lock_guard<std::mutex> clLifecycleLock(
		m_ptrRuntime->clLifecycleMutex);
	if (nullptr == p_szErr)
		return false;

	if (nullptr == p_szIp || 7 > strlen(p_szIp))
	{
		_snprintf(p_szErr, 1024, "code=-1,msg=init param err");
		return false;
	}

	// wyl 2026-03-30：启动前先清理旧实例残留，避免重复启动时复用脏状态。
	DeleteObj(m_ptrRuntime.get());

	int iRet = 0;

	//1.初始化日志
	if (nullptr != p_szLogFold && strlen(p_szLogFold) > 0)
	{
		// wyl 2026-03-30：TCP 服务使用独立日志单例，避免与 Web 服务共享路径和生命周期。
		if (MA_OK == CTcpLog::GetInstance()->InitLog(p_szLogFold))
		{
			CTcpLog::GetInstance()->Resume();//恢复工作
			TCP_INFO("log started");
		}
		else
		{
			_snprintf(p_szErr, 1024, "code=-2,msg=log init fail");
			DeleteObj(m_ptrRuntime.get());
			return false;
		}
		// 设置日志等级
		CTcpLog::GetInstance()->SetLogLevel((char *)"info");
	}

	// 设置任务回调
	g_pTcpHandle = p_tcpHandle;

	//2.设置线程池
	g_CTcpHPThreadPool->AdjustThreadCount(p_uiThreadNum);
	if (!g_CTcpHPThreadPool->Start(p_uiThreadNum, p_uiQueueNum, TRP_CALL_FAIL, 0))
	{
		_snprintf(p_szErr, 1024, "code=%d,msg=thread pool start fail", SYS_GetLastError());
		DeleteObj(m_ptrRuntime.get());
		return false;
	}

	//3.创建服务监听器
	if (nullptr == g_CTcpServerListerNet)
	{
		g_CTcpServerListerNet = new (std::nothrow)
			CTcpServerListerNet(m_ptrRuntime.get());
	}

	if (nullptr == g_CTcpServerListerNet)
	{
		iRet = SYS_GetLastError();
		_snprintf(p_szErr, 1024, "code=%d,msg=create tcp server lister fail", iRet);
		DeleteObj(m_ptrRuntime.get());
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
		DeleteObj(m_ptrRuntime.get());
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
	// TCP listen 队列与 Accept 预投递数量语义独立；未显式设置时保留 HPSocket 默认值。
	if (m_ptrRuntime->uiSocketListenQueue != 0)
	{
		g_CTcpPackServer->SetSocketListenQueue(
			m_ptrRuntime->uiSocketListenQueue);
	}

	// wyl 2026-03-30：资源准备完成后再标记服务可运行，供回调路径做状态保护。
	g_bServerStatus = true;

	//9.启动服务
	if (!g_CTcpPackServer->Start(p_szIp, p_unPort))
	{
		char szErrDesc[256] = { 0 };
		SafeCopyCString(szErrDesc, sizeof(szErrDesc), g_CTcpPackServer->GetLastErrorDesc());
		_snprintf(p_szErr, 1024, "code=%d,msg=%s",
			g_CTcpPackServer->GetLastError(), szErrDesc);
		DeleteObj(m_ptrRuntime.get());
		return false;
	}

	m_ptrRuntime->clIdentity.MarkStarted(p_szIp, p_unPort);
	TCP_INFO("server started");
	return true;
}

void CTcpSockServerObj::StopTcpSock()
{
	std::lock_guard<std::mutex> clLifecycleLock(
		m_ptrRuntime->clLifecycleMutex);
	DeleteObj(m_ptrRuntime.get());
}

bool CTcpSockServerObj::TcpSockSend(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen)
{
	if (nullptr == p_refServer || nullptr == p_refClient || nullptr == p_szData || 0 >= p_iDataLen)
		return false;

	ITcpServer *pSender = (ITcpServer *)p_refServer;
	CONNID dwConnID = (CONNID)p_refClient;

	if (!IsTcpSocketSendable(m_ptrRuntime.get(), pSender, dwConnID))
	{
		TCP_WARN("ConnID=%llu,SkipSendTcpClosed,SendDataLen=%d", (unsigned long long)dwConnID, p_iDataLen);
		return false;
	}

	bool bSendOK = SendTcpDataNoThrow(pSender, dwConnID, (const BYTE *)p_szData, p_iDataLen, "TcpSend");
	if (bSendOK)
	{
		TCP_INFO("ConnID=%llu,SendDataLen=%d", (unsigned long long)dwConnID, p_iDataLen);
	}
	else
	{
		TCP_ERROR("ConnID=%llu,SendDataLen=%d,err=%d", (unsigned long long)dwConnID, p_iDataLen, SYS_GetLastError());
	}
	return bSendOK;
}

void CTcpSockServerObj::TcpSockClose(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen)
{
	if (nullptr == p_refServer || nullptr == p_refClient)
		return;

	ITcpServer *pSender = (ITcpServer *)p_refServer;
	CONNID dwConnID = (CONNID)p_refClient;

	bool bCanClose = IsTcpSocketSendable(
		m_ptrRuntime.get(), pSender, dwConnID);
	if (g_bTcpMutexInit && bCanClose)
	{
		pthread_mutex_lock(&g_mutexConnet);
		std::map<CONNID, ClientData>::iterator itClient = g_mapClient.find(dwConnID);
		if (itClient != g_mapClient.end())
		{
			// wyl 2026-05-19：本端主动关闭开始时立即撤销可发送状态，阻止其它业务线程继续推送同一连接。
			itClient->second.bConnected = false;
		}
		// wyl 2026-03-30：显式标记“本端主动断开”，不要再依赖 OnClose 里的系统错误码猜测关闭来源。
		g_setTcpLocalClosing.insert(dwConnID);
		pthread_mutex_unlock(&g_mutexConnet);
	}

	if (!bCanClose)
	{
		TCP_WARN("ConnID=%llu,SkipCloseTcpClosed", (unsigned long long)dwConnID);
		return;
	}

	// wyl 2026-03-30：允许无数据直接断连，发送应答和关闭连接不再强耦合。
	if (nullptr != p_szData && p_iDataLen > 0)
	{
		if (!SendTcpDataNoThrow(pSender, dwConnID, (const BYTE *)p_szData, p_iDataLen, "TcpCloseSend"))
		{
			TCP_WARN("ConnID=%llu,SendCloseDataFail,err=%d", (unsigned long long)dwConnID, SYS_GetLastError());
		}
	}

	// wyl 2026-03-30：改为优雅断开，让关闭前已经排队的最后一包数据有机会真正发出。
	if (!DisconnectTcpNoThrow(pSender, dwConnID, false, "TcpDisconnect") && g_bTcpMutexInit)
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

bool CTcpSockServerObj::TcpSockIsAlive(void *p_refServer, void *p_refClient)
{
	if (nullptr == p_refServer || nullptr == p_refClient)
		return false;

	ITcpServer *pSender = (ITcpServer *)p_refServer;
	CONNID dwConnID = (CONNID)p_refClient;
	return IsTcpSocketSendable(m_ptrRuntime.get(), pSender, dwConnID);
}

