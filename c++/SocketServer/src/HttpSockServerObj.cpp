#include "HttpSockServerObj.h"
#include "publicGlobalvar.h"
#include "ServerRuntimeContext.h"
#include "publicfunc.h"
#include "Log.h"

namespace
{
	// wyl 2026-04-07：统一清空 HTTP 异步请求对象和当前连接解析状态，避免旧状态残留到下一次启动。
	void ClearHttpRuntimeData(ST_HTTP_SERVER_RUNTIME* p_pRuntime)
	{
		if (p_pRuntime == nullptr)
		{
			return;
		}
		pthread_mutex_lock(&p_pRuntime->mutexRequest);
		foreach(p_pRuntime->mapRequest, it_req)
		{
			if (nullptr == it_req->second)
				continue;

			if (it_req->second->IsDispatched())
			{
				// 业务回调已拿到请求对象后，其生命周期由上层负责；停服时只断开底层发送能力，避免悬空指针。
				it_req->second->DetachTransport();
			}
			else
			{
				delete it_req->second;
			}
		}
		p_pRuntime->mapRequest.clear();
		p_pRuntime->mapParsingRequest.clear();
		p_pRuntime->mapActiveRequest.clear();
		pthread_mutex_unlock(&p_pRuntime->mutexRequest);
	}

	// 停服只释放指定 HTTP 实例，其他同协议实例继续独立运行。
	void DeleteHttpObj(ST_HTTP_SERVER_RUNTIME* p_pRuntime)
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
		}
		p_pRuntime->clThreadPool->Stop();
		ClearHttpRuntimeData(p_pRuntime);
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
		p_pRuntime->ullAsyncRequestId = 0;
	}
}

CHttpSockServerObj::CHttpSockServerObj(const std::string& p_refServiceName,
	std::uint64_t p_ullInstanceId)
	: m_ptrRuntime(new ST_HTTP_SERVER_RUNTIME(
		p_refServiceName, p_ullInstanceId))
{
	m_ptrRuntime->pOwner = this;
}

CHttpSockServerObj::~CHttpSockServerObj()
{
	DeleteHttpObj(m_ptrRuntime.get());
}

bool CHttpSockServerObj::FillRuntimeInfo(
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

bool CHttpSockServerObj::SetSocketListenQueue(
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

bool CHttpSockServerObj::CreateHttpSock(const char* p_szIp, unsigned short p_unPort, unsigned int p_uiRBufLen, unsigned int p_uiMaxConnectNum, unsigned int p_uiMaxAcceptNum,
	HTTP_NOTIFY_PROC p_httpHandle, unsigned int p_uiThreadNum, unsigned int p_uiQueueNum, char* p_szErr, const char* p_szLogFold)
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

	// wyl 2026-04-07：启动前先清理旧实例残留，避免重复启动时复用脏状态。
	DeleteHttpObj(m_ptrRuntime.get());

	int iRet = 0;

	//1.初始化日志
	if (nullptr != p_szLogFold && strlen(p_szLogFold) > 0)
	{
		// wyl 2026-04-07：Http 服务使用独立日志单例，避免与 TCP 服务共享日志路径和生命周期。
		if (MA_OK == CHttpLog::GetInstance()->InitLog(p_szLogFold))
		{
			CHttpLog::GetInstance()->Resume();//恢复工作
			HTTP_INFO("log started");
		}
		else
		{
			_snprintf(p_szErr, 1024, "code=-2,msg=log init fail");
			DeleteHttpObj(m_ptrRuntime.get());
			return false;
		}
		// 设置日志等级
		CHttpLog::GetInstance()->SetLogLevel((char *)"info");
	}

	// 设置任务回调
	m_ptrRuntime->pNotifyHandler = p_httpHandle;

	//2.设置线程池
	m_ptrRuntime->clThreadPool->AdjustThreadCount(p_uiThreadNum);
	if (!m_ptrRuntime->clThreadPool->Start(p_uiThreadNum,
		p_uiQueueNum, TRP_CALL_FAIL, 0))
	{
		_snprintf(p_szErr, 1024, "code=%d,msg=thread pool start fail", SYS_GetLastError());
		DeleteHttpObj(m_ptrRuntime.get());
		return false;
	}

	//3.创建服务监听器
	if (nullptr == m_ptrRuntime->pListener)
	{
		m_ptrRuntime->pListener = new (std::nothrow)
			CHttpServerListerNet(m_ptrRuntime.get());
	}

	if (nullptr == m_ptrRuntime->pListener)
	{
		iRet = SYS_GetLastError();
		_snprintf(p_szErr, 1024, "code=%d,msg=create http server listener fail", iRet);
		DeleteHttpObj(m_ptrRuntime.get());
		return false;
	}

	//4.创建服务
	if (nullptr == m_ptrRuntime->pPackServer)
	{
		m_ptrRuntime->pPackServer = HP_Create_HttpServer(
			m_ptrRuntime->pListener);
	}

	if (nullptr == m_ptrRuntime->pPackServer)
	{
		_snprintf(p_szErr, 1024, "code=%d,msg=create http server fail", SYS_GetLastError());
		DeleteHttpObj(m_ptrRuntime.get());
		return false;
	}

	//5.设置超时心跳
	m_ptrRuntime->pPackServer->SetKeepAliveTime(2000);
	m_ptrRuntime->pPackServer->SetKeepAliveInterval(1000);

	//6.设置缓存大小
	m_ptrRuntime->pPackServer->SetSocketBufferSize(p_uiRBufLen);

	//7.设置最大连接数
	m_ptrRuntime->pPackServer->SetMaxConnectionCount(p_uiMaxConnectNum);

	// wyl 2026-04-07：这里设置的是底层 Accept 预分配数量，不是“同一 IP 最大连接数”限流。
	//8.设置Accept大小
	m_ptrRuntime->pPackServer->SetAcceptSocketCount(p_uiMaxAcceptNum);
	// TCP listen 队列与 Accept 预投递数量语义独立；未显式设置时保留 HPSocket 默认值。
	if (m_ptrRuntime->uiSocketListenQueue != 0)
	{
		m_ptrRuntime->pPackServer->SetSocketListenQueue(
			m_ptrRuntime->uiSocketListenQueue);
	}

	// wyl 2026-04-07：资源准备完成后再标记 Http 服务可运行，供回调路径做状态保护。
	m_ptrRuntime->bServerStatus.store(true);

	//9.启动服务
	if (!m_ptrRuntime->pPackServer->Start(p_szIp, p_unPort))
	{
		char szErrDesc[256] = { 0 };
		SafeCopyCString(szErrDesc, sizeof(szErrDesc),
			m_ptrRuntime->pPackServer->GetLastErrorDesc());
		_snprintf(p_szErr, 1024, "code=%d,msg=%s",
			m_ptrRuntime->pPackServer->GetLastError(), szErrDesc);
		DeleteHttpObj(m_ptrRuntime.get());
		return false;
	}

	m_ptrRuntime->clIdentity.MarkStarted(p_szIp, p_unPort);
	HTTP_INFO("server started");
	return true;
}

bool CHttpSockServerObj::CreateHttpsSock(const char*, unsigned short, unsigned int, unsigned int, unsigned int,
	HTTP_NOTIFY_PROC, unsigned int, unsigned int, char* p_szErr,
	const char*, const char*,
	const char*, const char*,
	const char*)
{
	// wyl 2026-04-07：当前版本尚未实现 WSS 建链和证书装载，必须明确返回失败，避免上层误判服务已启动成功。
	if (nullptr != p_szErr)
	{
		_snprintf(p_szErr, 1024, "code=-3,msg=https not implement");
	}
	return false;
}

void CHttpSockServerObj::StopHttpSock()
{
	std::lock_guard<std::mutex> clLifecycleLock(
		m_ptrRuntime->clLifecycleMutex);
	DeleteHttpObj(m_ptrRuntime.get());
}

bool CHttpSockServerObj::DelHttpAsynReq(unsigned long long p_lluReqId)
{
	if (m_ptrRuntime == nullptr)
		return false;

	CHttpAsynReqObj* pReqObj = nullptr;
	pthread_mutex_lock(&m_ptrRuntime->mutexRequest);
	std::map<unsigned long long, CHttpAsynReqObj*>::iterator itReq =
		m_ptrRuntime->mapRequest.find(p_lluReqId);
	if (itReq == m_ptrRuntime->mapRequest.end())
	{
		pthread_mutex_unlock(&m_ptrRuntime->mutexRequest);
		return false;
	}

	pReqObj = itReq->second;
	std::map<CONNID, unsigned long long>::iterator itConnReq =
		m_ptrRuntime->mapParsingRequest.find((CONNID)pReqObj->GetConnId());
	if (itConnReq != m_ptrRuntime->mapParsingRequest.end() &&
		itConnReq->second == p_lluReqId)
	{
		m_ptrRuntime->mapParsingRequest.erase(itConnReq);
	}

	std::map<CONNID, unsigned long long>::iterator itActiveReq =
		m_ptrRuntime->mapActiveRequest.find((CONNID)pReqObj->GetConnId());
	if (itActiveReq != m_ptrRuntime->mapActiveRequest.end() &&
		itActiveReq->second == p_lluReqId)
	{
		m_ptrRuntime->mapActiveRequest.erase(itActiveReq);
	}

	m_ptrRuntime->mapRequest.erase(itReq);
	pthread_mutex_unlock(&m_ptrRuntime->mutexRequest);

	if (nullptr != pReqObj)
	{
		if (!pReqObj->HasSentResponse())
		{
			// 业务层放弃本次请求时，库内主动结束连接，避免 keep-alive 连接悬挂在“永远等不到响应”的状态。
			pReqObj->AbortRequest();
		}
		else
		{
			pReqObj->DetachTransport();
		}
		delete pReqObj;
	}
	return true;
}


