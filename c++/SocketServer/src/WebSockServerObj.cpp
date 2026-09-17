#include "WebSockServerObj.h"
#include "publicGlobalvar.h"
#include "WebSocketSendCoordinator.h"
#include "ServerRuntimeContext.h"
#include "publicfunc.h"
#include "Log.h"
#include <windows.h>
#include <condition_variable>
#include <memory>
#include <mutex>

namespace
{
	// 注册单次发送调用；停服关闭接纳闸门后不再产生新票据。
	bool RegisterWebSendCall(ST_WEB_SERVER_RUNTIME* p_pRuntime)
	{
		if (p_pRuntime == nullptr)
		{
			return false;
		}
		std::lock_guard<std::mutex> lock(
			p_pRuntime->clSendLifecycleMutex);
		if (!p_pRuntime->bSendAccepting)
		{
			return false;
		}
		++p_pRuntime->szActiveSendCount;
		return true;
	}

	// 归还活动发送计数；最后一个调用退出时唤醒停服线程。
	void UnregisterWebSendCall(ST_WEB_SERVER_RUNTIME* p_pRuntime)
	{
		if (p_pRuntime == nullptr)
		{
			return;
		}
		std::lock_guard<std::mutex> lock(
			p_pRuntime->clSendLifecycleMutex);
		if (p_pRuntime->szActiveSendCount > 0)
		{
			--p_pRuntime->szActiveSendCount;
		}
		if (p_pRuntime->szActiveSendCount == 0)
		{
			p_pRuntime->clSendLifecycleCondition.notify_all();
		}
	}

	// 发送票据使用 RAII 归还，异常和提前返回都不会阻塞同连接后续发送。
	class CWebSendTurnGuard
	{
	public:
		explicit CWebSendTurnGuard(ST_WEB_SERVER_RUNTIME* p_pRuntime)
			: m_pRuntime(p_pRuntime), m_ullConnID(0), m_ullTicket(0),
			m_bRegistered(false), m_bAcquired(false)
		{
		}

		~CWebSendTurnGuard()
		{
			Release();
		}

		bool Acquire(CONNID p_ullConnID, bool p_bClosing)
		{
			if (!RegisterWebSendCall(m_pRuntime))
			{
				return false;
			}
			m_bRegistered = true;
			m_ullConnID = p_ullConnID;
			try
			{
				// map 和 ticket 在同一临界区内取得，避免空闲状态删除后生成第二套发送序列。
				std::lock_guard<std::mutex> mapLock(
					m_pRuntime->clSendOrdersMutex);
				std::shared_ptr<ST_WEB_SEND_ORDER>& ptrOrder =
					m_pRuntime->mapSendOrders[p_ullConnID];
				if (nullptr == ptrOrder)
				{
					ptrOrder = std::make_shared<ST_WEB_SEND_ORDER>();
				}
				std::lock_guard<std::mutex> orderLock(ptrOrder->clMutex);
				if (ptrOrder->bClosing)
				{
					return false;
				}
				if (p_bClosing)
				{
					ptrOrder->bClosing = true;
				}
				m_ptrOrder = ptrOrder;
				m_ullTicket = ptrOrder->ullNextTicket++;
			}
			catch (...)
			{
				WEB_ERROR("ConnID=%llu,AcquireWebSendTurnAllocFail", (unsigned long long)p_ullConnID);
				return false;
			}

			std::unique_lock<std::mutex> orderLock(m_ptrOrder->clMutex);
			m_ptrOrder->clCondition.wait(orderLock, [this]() {
				return m_ptrOrder->ullServingTicket == m_ullTicket;
			});
			m_bAcquired = true;
			return true;
		}

	private:
		void Release()
		{
			if (m_bAcquired && nullptr != m_ptrOrder)
			{
				{
					std::lock_guard<std::mutex> mapLock(
						m_pRuntime->clSendOrdersMutex);
					auto itOrder = m_pRuntime->mapSendOrders.find(m_ullConnID);
					std::lock_guard<std::mutex> orderLock(m_ptrOrder->clMutex);
					++m_ptrOrder->ullServingTicket;
					const bool bEraseOrder = m_ptrOrder->ullServingTicket == m_ptrOrder->ullNextTicket
						&& (!m_ptrOrder->bClosing || m_ptrOrder->bConnectionClosed);
					if (bEraseOrder &&
						itOrder != m_pRuntime->mapSendOrders.end() &&
						itOrder->second == m_ptrOrder)
					{
						m_pRuntime->mapSendOrders.erase(itOrder);
					}
				}
				m_ptrOrder->clCondition.notify_all();
				m_bAcquired = false;
			}
			if (m_bRegistered)
			{
				UnregisterWebSendCall(m_pRuntime);
				m_bRegistered = false;
			}
		}

	private:
		ST_WEB_SERVER_RUNTIME* m_pRuntime;
		CONNID m_ullConnID;
		unsigned long long m_ullTicket;
		bool m_bRegistered;
		bool m_bAcquired;
		std::shared_ptr<ST_WEB_SEND_ORDER> m_ptrOrder;
	};

	// wyl 2026-03-30：统一清空 Web 连接、任务和缓存状态，避免旧状态残留到下一次启动。
	void ClearWebRuntimeData(ST_WEB_SERVER_RUNTIME* p_pRuntime)
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
		foreach(p_pRuntime->mapNotifyQueue, it_queue)
		{
			for (NotifyTask* pTask : it_queue->second.deqTasks)
			{
				delete pTask;
			}
		}
		p_pRuntime->mapNotifyQueue.clear();
		pthread_mutex_unlock(&p_pRuntime->mutexTask);

		pthread_mutex_lock(&p_pRuntime->mutexRequest);
		foreach(p_pRuntime->mapRequest, it_queue)
		{
			delete it_queue->second;
		}
		p_pRuntime->mapRequest.clear();
		pthread_mutex_unlock(&p_pRuntime->mutexRequest);
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

	bool IsWebSocketSendable(ST_WEB_SERVER_RUNTIME* p_pRuntime,
		IHttpServer *pSender, CONNID dwConnID)
	{
		if (p_pRuntime == nullptr || pSender == nullptr)
		{
			return false;
		}

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

		return bMapAlive && HpSocketIsConnectedNoThrow(pSender, dwConnID);
	}

	bool GetWebSocketPendingDataLengthNoThrow(ST_WEB_SERVER_RUNTIME* p_pRuntime,
		IHttpServer *pSender, CONNID dwConnID, int& p_refIPendingBytes)
	{
		p_refIPendingBytes = 0;
		if (!IsWebSocketSendable(p_pRuntime, pSender, dwConnID))
			return false;

		BOOL bQueryOK = FALSE;
		int iPendingBytes = 0;
		DWORD dwExceptionCode = 0;
		__try
		{
			bQueryOK = pSender->GetPendingDataLength(dwConnID, iPendingBytes);
		}
		__except (dwExceptionCode = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
		{
			// 第三方网络库状态查询异常必须被截断，避免慢连接诊断打穿业务进程。
			WEB_ERROR("ConnID=%llu,WebSockGetPendingDataLengthException=0x%08X",
				(unsigned long long)dwConnID, dwExceptionCode);
			return false;
		}
		if (!bQueryOK || iPendingBytes < 0)
			return false;

		p_refIPendingBytes = iPendingBytes;
		return true;
	}

	bool SendWSMessageNoThrow(IHttpServer *pSender, CONNID dwConnID, BYTE iOperationCode,		const BYTE *pData, int iLength, ULONGLONG ullBodyLen, const char *p_szAction)
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

	bool SendWebSocketDataOrdered(ST_WEB_SERVER_RUNTIME* p_pRuntime,
		IHttpServer *pSender, CONNID dwConnID, BYTE p_byOperationCode,
		const char *p_szData, int p_iDataLen, const char *p_szAction)
	{
		CWebSendTurnGuard clTurnGuard(p_pRuntime);
		if (!clTurnGuard.Acquire(dwConnID, false))
		{
			return false;
		}
		bool bSendOK = false;
		if (IsWebSocketSendable(p_pRuntime, pSender, dwConnID))
		{
			bSendOK = SendWSMessageNoThrow(pSender, dwConnID, p_byOperationCode,
				(const BYTE *)p_szData, p_iDataLen, p_iDataLen, p_szAction);
		}
		else
		{
			WEB_WARN("ConnID=%llu,SkipSendWebSocketClosed,Opcode=%u,SendDataLen=%d",
				(unsigned long long)dwConnID, (unsigned int)p_byOperationCode, p_iDataLen);
		}
		return bSendOK;
	}

	void CloseWebSocketOrdered(ST_WEB_SERVER_RUNTIME* p_pRuntime,
		IHttpServer *pSender, CONNID dwConnID, BYTE p_byDataOperationCode,
		const char *p_szData, int p_iDataLen)
	{
		CWebSendTurnGuard clTurnGuard(p_pRuntime);
		if (!clTurnGuard.Acquire(dwConnID, true))
		{
			return;
		}
		const bool bCanClose = IsWebSocketSendable(p_pRuntime, pSender, dwConnID);
		if (p_pRuntime != nullptr && bCanClose)
		{
			pthread_mutex_lock(&p_pRuntime->mutexConnection);
			std::map<CONNID, ClientData>::iterator itClient =
				p_pRuntime->mapClient.find(dwConnID);
			if (itClient != p_pRuntime->mapClient.end())
			{
				itClient->second.bConnected = false;
			}
			p_pRuntime->setLocalClosing.insert(dwConnID);
			pthread_mutex_unlock(&p_pRuntime->mutexConnection);
		}

		if (!bCanClose)
		{
			WEB_WARN("ConnID=%llu,SkipCloseWebSocketClosed", (unsigned long long)dwConnID);
			return;
		}
		if (nullptr != p_szData && p_iDataLen > 0 &&
			!SendWSMessageNoThrow(pSender, dwConnID, p_byDataOperationCode,
				(const BYTE *)p_szData, p_iDataLen, p_iDataLen, "SendCloseData"))
		{
			WEB_ERROR("ConnID=%llu,SendCloseDataFail,err=%d", (unsigned long long)dwConnID, SYS_GetLastError());
		}
		if (!SendWSMessageNoThrow(pSender, dwConnID, 8, nullptr, 0, 0, "SendCloseFrame"))
		{
			WEB_WARN("ConnID=%llu,SendCloseFrameFail,err=%d", (unsigned long long)dwConnID, SYS_GetLastError());
		}
		if (!pSender->Disconnect(dwConnID, false) && p_pRuntime != nullptr)
		{
			pthread_mutex_lock(&p_pRuntime->mutexConnection);
			p_pRuntime->setLocalClosing.erase(dwConnID);
			p_pRuntime->mapClient.erase(dwConnID);
			pthread_mutex_unlock(&p_pRuntime->mutexConnection);

			pthread_mutex_lock(&p_pRuntime->mutexRequest);
			auto itQueue = p_pRuntime->mapRequest.find(dwConnID);
			if (itQueue != p_pRuntime->mapRequest.end())
			{
				delete itQueue->second;
				p_pRuntime->mapRequest.erase(itQueue);
			}
			pthread_mutex_unlock(&p_pRuntime->mutexRequest);
			MarkWebSocketSendConnectionClosed(p_pRuntime, dwConnID);
		}
	}
}

void StartWebSocketSendCoordinator(ST_WEB_SERVER_RUNTIME* p_pRuntime)
{
	if (p_pRuntime == nullptr)
	{
		return;
	}
	std::lock_guard<std::mutex> lifecycleLock(
		p_pRuntime->clSendLifecycleMutex);
	p_pRuntime->bSendAccepting = true;
}

void StopWebSocketSendCoordinator(ST_WEB_SERVER_RUNTIME* p_pRuntime)
{
	if (p_pRuntime == nullptr)
	{
		return;
	}
	{
		std::unique_lock<std::mutex> lifecycleLock(
			p_pRuntime->clSendLifecycleMutex);
		p_pRuntime->bSendAccepting = false;
		p_pRuntime->clSendLifecycleCondition.wait(lifecycleLock,
			[p_pRuntime]() {
			return p_pRuntime->szActiveSendCount == 0;
		});
	}
	std::lock_guard<std::mutex> sendLock(p_pRuntime->clSendOrdersMutex);
	p_pRuntime->mapSendOrders.clear();
}

void MarkWebSocketSendConnectionClosed(ST_WEB_SERVER_RUNTIME* p_pRuntime,
	CONNID p_ullConnID)
{
	if (p_pRuntime == nullptr)
	{
		return;
	}
	std::lock_guard<std::mutex> mapLock(p_pRuntime->clSendOrdersMutex);
	auto itOrder = p_pRuntime->mapSendOrders.find(p_ullConnID);
	if (itOrder == p_pRuntime->mapSendOrders.end() || nullptr == itOrder->second)
	{
		return;
	}
	std::lock_guard<std::mutex> orderLock(itOrder->second->clMutex);
	itOrder->second->bClosing = true;
	itOrder->second->bConnectionClosed = true;
	if (itOrder->second->ullServingTicket == itOrder->second->ullNextTicket)
	{
		p_pRuntime->mapSendOrders.erase(itOrder);
	}
}

bool SendWebSocketControlFrameOrdered(ST_WEB_SERVER_RUNTIME* p_pRuntime,
	IHttpServer* p_pSender, CONNID p_ullConnID,
	BYTE p_byOperationCode, const BYTE* p_pData, int p_iDataLen, const char* p_szAction)
{
	CWebSendTurnGuard clTurnGuard(p_pRuntime);
	if (!clTurnGuard.Acquire(p_ullConnID, false) ||
		!IsWebSocketSendable(p_pRuntime, p_pSender, p_ullConnID))
	{
		return false;
	}
	return SendWSMessageNoThrow(p_pSender, p_ullConnID, p_byOperationCode,
		p_pData, p_iDataLen, p_iDataLen, p_szAction);
}

bool CloseWebSocketFromPeerOrdered(ST_WEB_SERVER_RUNTIME* p_pRuntime,
	IHttpServer* p_pSender, CONNID p_ullConnID)
{
	CWebSendTurnGuard clTurnGuard(p_pRuntime);
	if (!clTurnGuard.Acquire(p_ullConnID, true))
	{
		return false;
	}
	const bool bCanClose = IsWebSocketSendable(
		p_pRuntime, p_pSender, p_ullConnID);
	if (bCanClose && !SendWSMessageNoThrow(p_pSender, p_ullConnID, 8, nullptr, 0, 0, "ReplyCloseFrame"))
	{
		WEB_WARN("ConnID=%llu,ReplyCloseFrameFail,err=%d", (unsigned long long)p_ullConnID, SYS_GetLastError());
	}
	if (nullptr != p_pSender)
	{
		p_pSender->Disconnect(p_ullConnID, false);
	}
	return bCanClose;
}

// 停服只释放指定 WebSocket 实例，其他网络实例继续独立运行。
void DeleteWebObj(ST_WEB_SERVER_RUNTIME* p_pRuntime)
{
	if (p_pRuntime == nullptr)
	{
		return;
	}
	// wyl 2026-03-30：停服时先拉低 Web 运行状态，再停止服务和线程池，避免回调继续进入无效状态。
	p_pRuntime->bServerStatus.store(false);
	p_pRuntime->clIdentity.MarkStopped();
	StopWebSocketSendCoordinator(p_pRuntime);

	if (p_pRuntime->pPackServer != nullptr)
	{
		p_pRuntime->pPackServer->Stop();
		p_pRuntime->pPackServer->Wait(INFINITE);
	}

	// wyl 2026-04-15：先等待线程池中已排队的 Web 回调自然退出，再销毁底层 server，
	// 避免同进程并行服务或停服边界下，上层回调拿到已经失效的 p_refServerHandle。
	p_pRuntime->clThreadPool->Stop();

	ClearWebRuntimeData(p_pRuntime);

	if (p_pRuntime->pPackServer != nullptr)
	{
		HP_Destroy_HttpServer(p_pRuntime->pPackServer);
		p_pRuntime->pPackServer = nullptr;
	}

	if (p_pRuntime->pListener != nullptr)
	{
		delete p_pRuntime->pListener;
		p_pRuntime->pListener = nullptr;
	}
	p_pRuntime->pNotifyHandler = nullptr;
	p_pRuntime->ullTaskId = 0;
}

int CWebSockServerObj::WebSockCompare(void* p_refSrcClient, void* p_refObjClient)
{
	if (nullptr == p_refSrcClient || nullptr == p_refObjClient)
		return -1;

	CONNID dwSrcConnID = (CONNID)p_refSrcClient;
	CONNID dwObjConnID = (CONNID)p_refObjClient;
	return dwSrcConnID == dwObjConnID ? 0 : 1;
}

CWebSockServerObj::CWebSockServerObj(const std::string& p_refServiceName,
	std::uint64_t p_ullInstanceId)
	: m_ptrRuntime(new ST_WEB_SERVER_RUNTIME(
		p_refServiceName, p_ullInstanceId))
{
}

CWebSockServerObj::~CWebSockServerObj()
{
	DeleteWebObj(m_ptrRuntime.get());
}

bool CWebSockServerObj::FillRuntimeInfo(
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

bool CWebSockServerObj::SetSocketListenQueue(
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

bool CWebSockServerObj::CreateWebSock(const char *p_szIp, unsigned short p_unPort, unsigned int p_uiRBufLen, unsigned int p_uiMaxConnectNum, unsigned int p_uiMaxAcceptNum,
	WEB_NOTIFY_PROC p_webHandle, unsigned int p_uiThreadNum, unsigned int p_uiQueueNum, char *p_szErr, const char *p_szLogFold)
{
	std::lock_guard<std::mutex> clLifecycleLock(
		m_ptrRuntime->clLifecycleMutex);

	if (nullptr == p_szErr)
	{
		return false;
	}

	if (nullptr == p_szIp || 7 > strlen(p_szIp))
	{
		_snprintf(p_szErr, 1024, "code=-1,msg=init param err");
		return false;
	}

	// wyl 2026-03-30：启动前先清理旧实例残留，避免重复启动时复用脏状态。
	DeleteWebObj(m_ptrRuntime.get());

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
			DeleteWebObj(m_ptrRuntime.get());
			return false;
		}
		// 设置日志等级
		CWebLog::GetInstance()->SetLogLevel((char *)"info");
	}

	// 设置任务回调
	m_ptrRuntime->pNotifyHandler = p_webHandle;

	// wyl 2026-03-30：先初始化发送协调器和线程池，再启动网络监听，减少启动窗口期竞态。
	StartWebSocketSendCoordinator(m_ptrRuntime.get());

	//2.设置线程池
	m_ptrRuntime->clThreadPool->AdjustThreadCount(p_uiThreadNum);
	if (!m_ptrRuntime->clThreadPool->Start(p_uiThreadNum,
		p_uiQueueNum, TRP_CALL_FAIL, 0))
	{
		_snprintf(p_szErr, 1024, "code=%d,msg=thread pool start fail", SYS_GetLastError());
		DeleteWebObj(m_ptrRuntime.get());
		return false;
	}

	//3.创建服务监听器
	if (m_ptrRuntime->pListener == nullptr)
	{
		m_ptrRuntime->pListener = new (std::nothrow)
			CWebServerListerNet(m_ptrRuntime.get());
	}

	if (m_ptrRuntime->pListener == nullptr)
	{
		iRet = SYS_GetLastError();
		_snprintf(p_szErr, 1024, "code=%d,msg=create web server lister fail", iRet);
		DeleteWebObj(m_ptrRuntime.get());
		return false;
	}

	//4.创建服务
	if (m_ptrRuntime->pPackServer == nullptr)
	{
		m_ptrRuntime->pPackServer = HP_Create_HttpServer(
			m_ptrRuntime->pListener);
	}

	if (m_ptrRuntime->pPackServer == nullptr)
	{
		_snprintf(p_szErr, 1024, "code=%d,msg=create web server fail", SYS_GetLastError());
		DeleteWebObj(m_ptrRuntime.get());
		return false;
	}

	//5.设置超时心跳
	m_ptrRuntime->pPackServer->SetKeepAliveTime(2000);
	m_ptrRuntime->pPackServer->SetKeepAliveInterval(1000);

	//6.设置缓存大小
	m_ptrRuntime->pPackServer->SetSocketBufferSize(p_uiRBufLen);

	//7.设置最大连接数
	m_ptrRuntime->pPackServer->SetMaxConnectionCount(p_uiMaxConnectNum);

	// wyl 2026-03-30：这里设置的是底层 Accept 预分配数量，不是“同一 IP 最大连接数”限流。
	//8.设置Accept大小
	m_ptrRuntime->pPackServer->SetAcceptSocketCount(p_uiMaxAcceptNum);
	// TCP listen 队列与 Accept 预投递数量语义独立；未显式设置时保留 HPSocket 默认值。
	if (m_ptrRuntime->uiSocketListenQueue != 0)
	{
		m_ptrRuntime->pPackServer->SetSocketListenQueue(
			m_ptrRuntime->uiSocketListenQueue);
	}

	// wyl 2026-03-30：资源准备完成后再标记 Web 服务可运行，供回调路径做状态保护。
	m_ptrRuntime->bServerStatus.store(true);

	//9.启动服务
	if (!m_ptrRuntime->pPackServer->Start(p_szIp, p_unPort))
	{
		char szErrDesc[256] = { 0 };
		SafeCopyCString(szErrDesc, sizeof(szErrDesc),
			m_ptrRuntime->pPackServer->GetLastErrorDesc());
		_snprintf(p_szErr, 1024, "code=%d,msg=%s",
			m_ptrRuntime->pPackServer->GetLastError(), szErrDesc);
		DeleteWebObj(m_ptrRuntime.get());
		return false;
	}

	m_ptrRuntime->clIdentity.MarkStarted(p_szIp, p_unPort);
	WEB_INFO("server started");
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
	std::lock_guard<std::mutex> clLifecycleLock(
		m_ptrRuntime->clLifecycleMutex);
	DeleteWebObj(m_ptrRuntime.get());
}

void CWebSockServerObj::WebSockClose(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen)
{
	if (nullptr == p_refServer || nullptr == p_refClient)
		return;
	CloseWebSocketOrdered(m_ptrRuntime.get(), (IHttpServer *)p_refServer,
		(CONNID)p_refClient, 2, p_szData, p_iDataLen);
}

void CWebSockServerObj::WebSockCloseText(void *p_refServer, void *p_refClient,
	const char *p_szData, int p_iDataLen)
{
	if (nullptr == p_refServer || nullptr == p_refClient)
		return;
	CloseWebSocketOrdered(m_ptrRuntime.get(), (IHttpServer *)p_refServer,
		(CONNID)p_refClient, 1, p_szData, p_iDataLen);
}

bool CWebSockServerObj::WebSockSend(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen)
{
	if (nullptr == p_refServer || nullptr == p_refClient || nullptr == p_szData || 0 >= p_iDataLen)
		return false;

	return SendWebSocketDataOrdered(m_ptrRuntime.get(),
		(IHttpServer *)p_refServer, (CONNID)p_refClient,
		2, p_szData, p_iDataLen, "SendWSBinaryMessage");
}

bool CWebSockServerObj::WebSockSendText(void *p_refServer, void *p_refClient,
	const char *p_szData, int p_iDataLen)
{
	if (nullptr == p_refServer || nullptr == p_refClient || nullptr == p_szData || 0 >= p_iDataLen)
		return false;
	return SendWebSocketDataOrdered(m_ptrRuntime.get(),
		(IHttpServer *)p_refServer, (CONNID)p_refClient,
		1, p_szData, p_iDataLen, "SendWSTextMessage");
}

bool CWebSockServerObj::WebSockIsAlive(void *p_refServer, void *p_refClient)
{
	if (nullptr == p_refServer || nullptr == p_refClient)
		return false;

	IHttpServer *pSender = (IHttpServer *)p_refServer;
	CONNID dwConnID = (CONNID)p_refClient;
	return IsWebSocketSendable(m_ptrRuntime.get(), pSender, dwConnID);
}
bool CWebSockServerObj::WebSockGetPendingDataLength(void *p_refServer, void *p_refClient,
	int& p_refIPendingBytes)
{
	p_refIPendingBytes = 0;
	if (nullptr == p_refServer || nullptr == p_refClient)
		return false;

	return GetWebSocketPendingDataLengthNoThrow(m_ptrRuntime.get(),
		(IHttpServer *)p_refServer, (CONNID)p_refClient, p_refIPendingBytes);
}
