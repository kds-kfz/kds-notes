#include "publicGlobalvar.h"
#include "publicfunc.h"
#include "Log.h"
#include "Base64.h"
#include "USER_SHA1.h"
#include "WebSocketSendCoordinator.h"
#include "ServerRuntimeContext.h"
#include <exception>
#include <new>

void ThreadWebNotifyTask(LPTSocketTask p_pSocketTask);

namespace
{
	// wyl 2026-03-30：单连接允许排队的 WebSocket 数据通知任务上限，防止慢消费场景把线程池任务队列顶满。
	const unsigned int g_uiWebMaxPendingTaskPerConn = 256;
	// wyl 2026-03-30：单连接允许排队的 WebSocket 数据通知累计字节上限，限制慢消费时的内存占用。
	const unsigned long long g_ullWebMaxPendingBytesPerConn = 16ULL * 1024 * 1024;
	// wyl 2026-03-30：单条 WebSocket 业务消息允许累计的最大长度，超过后按协议错误处理。
	const unsigned long g_ulWebMessageMaxLen = 16 * 1024 * 1024;
	// wyl 2026-03-30：完整消息处理结束后仍保留的小块缓存阈值，低于该值继续复用，避免频繁申请释放。
	const unsigned long g_ulWebCacheKeepLen = 64 * 1024;
	// wyl 2026-03-30：WebSocket 控制帧载荷长度上限，ping/pong/close 都必须满足该限制。
	const unsigned long g_ulWebControlFrameMaxLen = 125;

	// wyl 2026-03-30：按 WebSocket 标准生成握手应答值，避免并发握手时使用静态缓冲产生串包。
	bool BuildWebSocketAcceptKey(const char* pSrcKey, char* pDstKey, size_t dwDstLen)
	{
		if (nullptr == pSrcKey || nullptr == pDstKey || dwDstLen == 0)
			return false;

		char szSourceKey[256] = { 0 };
		unsigned char bySha1Buf[20] = { 0 };
		sprintf_s(szSourceKey, sizeof(szSourceKey), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", pSrcKey);
		if (SHA1_String(reinterpret_cast<const unsigned char*>(szSourceKey), (unsigned long)strlen(szSourceKey), bySha1Buf) <= 0)
			return false;

		base64_encode(bySha1Buf, 20, pDstKey);
		pDstKey[dwDstLen - 1] = '\0';
		return true;
	}

	// wyl 2026-03-30：仅做 ASCII 范围内的不区分大小写比较，足够覆盖 WebSocket 标准头值。
	char ToLowerAscii(char ch)
	{
		return (ch >= 'A' && ch <= 'Z') ? (ch - 'A' + 'a') : ch;
	}

	bool EqualsIgnoreCaseAscii(const char* pLeft, const char* pRight)
	{
		if (nullptr == pLeft || nullptr == pRight)
			return false;

		while (*pLeft != '\0' && *pRight != '\0')
		{
			if (ToLowerAscii(*pLeft) != ToLowerAscii(*pRight))
				return false;
			++pLeft;
			++pRight;
		}
		return ('\0' == *pLeft && '\0' == *pRight);
	}

	bool HeaderContainsTokenIgnoreCase(const char* pHeaderValue, const char* pToken)
	{
		if (nullptr == pHeaderValue || nullptr == pToken || '\0' == *pToken)
			return false;

		const char* pCursor = pHeaderValue;
		while ('\0' != *pCursor)
		{
			while (' ' == *pCursor || '\t' == *pCursor || ',' == *pCursor)
			{
				++pCursor;
			}

			const char* pTokenBegin = pCursor;
			while ('\0' != *pCursor && ',' != *pCursor)
			{
				++pCursor;
			}

			const char* pTokenEnd = pCursor;
			while (pTokenEnd > pTokenBegin && (' ' == *(pTokenEnd - 1) || '\t' == *(pTokenEnd - 1)))
			{
				--pTokenEnd;
			}

			if (pTokenEnd > pTokenBegin)
			{
				size_t dwHeaderTokenLen = (size_t)(pTokenEnd - pTokenBegin);
				size_t dwMatchLen = strlen(pToken);
				if (dwHeaderTokenLen == dwMatchLen)
				{
					bool bMatch = true;
					for (size_t i = 0; i < dwMatchLen; ++i)
					{
						if (ToLowerAscii(pTokenBegin[i]) != ToLowerAscii(pToken[i]))
						{
							bMatch = false;
							break;
						}
					}
					if (bMatch)
						return true;
				}
			}

			if (',' == *pCursor)
			{
				++pCursor;
			}
		}

		return false;
	}

	// wyl 2026-03-30：为单连接的 WebSocket 数据通知做轻量配额控制，避免上层消费过慢时任务队列和内存持续顶满。
	bool ReserveWebPendingQuota(ST_WEB_SERVER_RUNTIME* p_pRuntime,
		CONNID dwConnID, unsigned int uiDataLen)
	{
		if (p_pRuntime == nullptr)
		{
			return false;
		}
		if (0 == uiDataLen)
		{
			return true;
		}

		bool bReserved = false;
		pthread_mutex_lock(&p_pRuntime->mutexRequest);
		ReqCacheData *&refReqCacheData = p_pRuntime->mapRequest[dwConnID];
		if (nullptr == refReqCacheData)
		{
			// wyl 2026-04-25：配额统计对象按连接懒创建；使用 nothrow，申请失败时拒绝本次通知，避免异常穿透线程池。
			refReqCacheData = new (std::nothrow) ReqCacheData();
			if (nullptr == refReqCacheData)
			{
				pthread_mutex_unlock(&p_pRuntime->mutexRequest);
				return false;
			}
			refReqCacheData->ullConnID = dwConnID;
		}

		if (uiDataLen <= g_ullWebMaxPendingBytesPerConn
			&& refReqCacheData->uiWebPendingTaskCount < g_uiWebMaxPendingTaskPerConn
			&& refReqCacheData->ullWebPendingBytes <= g_ullWebMaxPendingBytesPerConn - uiDataLen)
		{
			++refReqCacheData->uiWebPendingTaskCount;
			refReqCacheData->ullWebPendingBytes += uiDataLen;
			bReserved = true;
		}
		pthread_mutex_unlock(&p_pRuntime->mutexRequest);
		return bReserved;
	}

	// wyl 2026-03-30：Web 数据通知完成后归还配额，空闲时顺手移除统计对象，避免无效状态长期残留。
	void ReleaseWebPendingQuota(ST_WEB_SERVER_RUNTIME* p_pRuntime,
		CONNID dwConnID, unsigned int uiDataLen)
	{
		if (p_pRuntime == nullptr)
		{
			return;
		}
		pthread_mutex_lock(&p_pRuntime->mutexRequest);
		std::map<CONNID, ReqCacheData*>::iterator itReq =
			p_pRuntime->mapRequest.find(dwConnID);
		if (itReq != p_pRuntime->mapRequest.end() && nullptr != itReq->second)
		{
			ReqCacheData *pReqCacheData = itReq->second;
			if (pReqCacheData->uiWebPendingTaskCount > 0)
			{
				--pReqCacheData->uiWebPendingTaskCount;
			}
			if (pReqCacheData->ullWebPendingBytes >= uiDataLen)
			{
				pReqCacheData->ullWebPendingBytes -= uiDataLen;
			}
			else
			{
				pReqCacheData->ullWebPendingBytes = 0;
			}

			if (0 == pReqCacheData->uiWebPendingTaskCount
				&& 0 == pReqCacheData->ullWebPendingBytes
				&& 0 == pReqCacheData->ulLength
				&& 0 == pReqCacheData->ulPos
				&& 0 == pReqCacheData->ulCapacity
				&& 0 == pReqCacheData->ulWsFrameLength
				&& 0 == pReqCacheData->ulWsFramePos
				&& 0 == pReqCacheData->ulWsControlLength
				&& 0 == pReqCacheData->ulWsControlPos
				&& !pReqCacheData->bWsMessageActive)
			{
				delete pReqCacheData;
				p_pRuntime->mapRequest.erase(itReq);
			}
		}
		pthread_mutex_unlock(&p_pRuntime->mutexRequest);
	}
}

namespace
{
	// 单条通知处理保持原有回调内容，外层 drain 只负责同连接顺序。
	bool ProcessOneWebNotifyTask(ST_WEB_SERVER_RUNTIME* p_pRuntime,
		IHttpServer *pSender, NotifyTask *pTask)
	{
		if (p_pRuntime == nullptr || pSender == nullptr || pTask == nullptr)
		{
			return false;
		}

		ClientData stClientData;
		pthread_mutex_lock(&p_pRuntime->mutexConnection);
		auto itClient = p_pRuntime->mapClient.find(pTask->ullConnID);
		if (itClient != p_pRuntime->mapClient.end())
		{
			stClientData = itClient->second;
		}
		if (enWebClose == pTask->enWebNotifyType)
		{
			p_pRuntime->mapClient.erase(pTask->ullConnID);
		}
		pthread_mutex_unlock(&p_pRuntime->mutexConnection);

		if (enWebClose == pTask->enWebNotifyType)
		{
			pthread_mutex_lock(&p_pRuntime->mutexRequest);
			auto itQueue = p_pRuntime->mapRequest.find(pTask->ullConnID);
			if (itQueue != p_pRuntime->mapRequest.end())
			{
				delete itQueue->second;
				p_pRuntime->mapRequest.erase(itQueue);
			}
			pthread_mutex_unlock(&p_pRuntime->mutexRequest);
		}

		bool bCallbackOk = true;
		if (p_pRuntime->pNotifyHandler != nullptr &&
			p_pRuntime->bServerStatus.load())
		{
			try
			{
				switch (pTask->enWebNotifyType)
				{
				case enWebData:
					WEB_INFO("ip=%s,port=%d,type=%d,len=%u", stClientData.szIp,
						stClientData.unPort, pTask->enWebNotifyType, pTask->uiLen);
					p_pRuntime->pNotifyHandler((void*)pSender, (void*)pTask->ullConnID, pTask->enWebNotifyType,
						(void*)pTask->pBuf, pTask->uiLen, stClientData.szIp, stClientData.unPort, pTask->szErrMsg);
					break;
				case enWebClose:
					_snprintf(pTask->szErrMsg, sizeof(pTask->szErrMsg), "client close");
					p_pRuntime->pNotifyHandler((void*)pSender, (void*)pTask->ullConnID, pTask->enWebNotifyType,
						nullptr, 0, stClientData.szIp, stClientData.unPort, pTask->szErrMsg);
					break;
				case enWebConnect:
					_snprintf(pTask->szErrMsg, sizeof(pTask->szErrMsg), "client connect");
					p_pRuntime->pNotifyHandler((void*)pSender, (void*)pTask->ullConnID, pTask->enWebNotifyType,
						nullptr, 0, stClientData.szIp, stClientData.unPort, pTask->szErrMsg);
					break;
				case enWebError:
					p_pRuntime->pNotifyHandler((void*)pSender, (void*)pTask->ullConnID, pTask->enWebNotifyType,
						nullptr, 0, stClientData.szIp, stClientData.unPort, pTask->szErrMsg);
					break;
				default:
					break;
				}
			}
			catch (const std::exception& p_refEx)
			{
				WEB_ERROR("ConnID=%llu,WebNotifyCallbackException=%s", (unsigned long long)pTask->ullConnID, p_refEx.what());
				bCallbackOk = false;
			}
			catch (...)
			{
				WEB_ERROR("ConnID=%llu,WebNotifyCallbackUnknownException", (unsigned long long)pTask->ullConnID);
				bCallbackOk = false;
			}
		}

		if (enWebData == pTask->enWebNotifyType)
		{
			ReleaseWebPendingQuota(p_pRuntime, pTask->ullConnID, pTask->uiLen);
		}
		return bCallbackOk;
	}

	const unsigned int g_uiWebNotifyDrainBatchSize = 32;

	struct ST_WEB_DRAIN_TASK_DATA
	{
		ST_WEB_SERVER_RUNTIME* pRuntime; // 不拥有；停服先等待线程池退出再释放上下文。
		CONNID ullConnID;                // 本次排空的连接编号，仅在所属实例内有效。
	};

	bool SubmitWebDrainTaskNoLock(ST_WEB_SERVER_RUNTIME* p_pRuntime,
		IHttpServer* pSender, CONNID dwConnID)
	{
		if (p_pRuntime == nullptr)
		{
			return false;
		}
		ST_WEB_DRAIN_TASK_DATA stTaskData = { p_pRuntime, dwConnID };
		LPTSocketTask pSocketTask = HP_Create_SocketTaskObj((Fn_SocketTaskProc)ThreadWebNotifyTask,
			pSender, dwConnID, (const BYTE*)&stTaskData, sizeof(stTaskData));
		if (nullptr == pSocketTask)
		{
			return false;
		}
		if (!p_pRuntime->clThreadPool->Submit(pSocketTask, 1000 * 5))
		{
			HP_Destroy_SocketTaskObj(pSocketTask);
			return false;
		}
		return true;
	}
}

// 通知任务按连接 FIFO 排空，不同连接仍由线程池并行处理。
void ThreadWebNotifyTask(LPTSocketTask socketTask)
{
	if (nullptr == socketTask || nullptr == socketTask->buf)
		return;
	IHttpServer *pSender = (IHttpServer *)socketTask->sender;
	ST_WEB_DRAIN_TASK_DATA* pTaskData =
		(ST_WEB_DRAIN_TASK_DATA *)socketTask->buf;
	ST_WEB_SERVER_RUNTIME* pRuntime = pTaskData->pRuntime;
	const CONNID ullConnID = pTaskData->ullConnID;
	if (pRuntime == nullptr)
	{
		return;
	}

	unsigned int uiProcessedCount = 0;
	for (; uiProcessedCount < g_uiWebNotifyDrainBatchSize; ++uiProcessedCount)
	{
		NotifyTask *pTask = nullptr;
		pthread_mutex_lock(&pRuntime->mutexTask);
		auto itQueue = pRuntime->mapNotifyQueue.find(ullConnID);
		if (itQueue == pRuntime->mapNotifyQueue.end() || itQueue->second.deqTasks.empty())
		{
			if (itQueue != pRuntime->mapNotifyQueue.end())
			{
				pRuntime->mapNotifyQueue.erase(itQueue);
			}
			pthread_mutex_unlock(&pRuntime->mutexTask);
			break;
		}
		pTask = itQueue->second.deqTasks.front();
		itQueue->second.deqTasks.pop_front();
		pthread_mutex_unlock(&pRuntime->mutexTask);

		const bool bCallbackOk = ProcessOneWebNotifyTask(pRuntime, pSender, pTask);
		delete pTask;
		if (!bCallbackOk)
		{
			pSender->Disconnect(ullConnID, false);
		}
	}

	// 单连接每批最多处理固定数量，队列仍有任务时重新提交，给其它连接公平执行机会。
	pthread_mutex_lock(&pRuntime->mutexTask);
	auto itQueue = pRuntime->mapNotifyQueue.find(ullConnID);
	if (itQueue != pRuntime->mapNotifyQueue.end() && !itQueue->second.deqTasks.empty())
	{
		if (!SubmitWebDrainTaskNoLock(pRuntime, pSender, ullConnID))
		{
			for (NotifyTask* pQueuedTask : itQueue->second.deqTasks)
			{
				if (nullptr != pQueuedTask && enWebData == pQueuedTask->enWebNotifyType)
				{
					ReleaseWebPendingQuota(pRuntime, pQueuedTask->ullConnID, pQueuedTask->uiLen);
				}
				delete pQueuedTask;
			}
			pRuntime->mapNotifyQueue.erase(itQueue);
			WEB_ERROR("ConnID=%llu,ResubmitWebDrainTaskFail", (unsigned long long)ullConnID);
			pSender->Disconnect(ullConnID, false);
		}
	}
	else if (itQueue != pRuntime->mapNotifyQueue.end())
	{
		pRuntime->mapNotifyQueue.erase(itQueue);
	}
	pthread_mutex_unlock(&pRuntime->mutexTask);
}

namespace
{
	// wyl 2026-03-30：按需扩展 WebSocket 消息缓存，兼容一条消息被拆成多次 Body 回调的情况。
	bool EnsureWebCacheCapacity(ReqCacheData* pReqCacheData, unsigned long ulNeedLen)
	{
		if (nullptr == pReqCacheData)
			return false;

		if (0 == ulNeedLen)
			return true;

		if (ulNeedLen <= pReqCacheData->ulCapacity)
			return true;

		unsigned long ulNewCapacity = (0 == pReqCacheData->ulCapacity) ? ulNeedLen : pReqCacheData->ulCapacity;
		while (ulNewCapacity < ulNeedLen)
		{
			if (ulNewCapacity > g_ulWebMessageMaxLen / 2)
			{
				ulNewCapacity = ulNeedLen;
				break;
			}
			ulNewCapacity *= 2;
		}

		// wyl 2026-04-25：扩容采用 nothrow，失败时让调用方按协议错误关闭连接，而不是抛异常中断网络线程。
		char* pNewBuf = new (std::nothrow) char[ulNewCapacity];
		if (nullptr == pNewBuf)
			return false;

		if (nullptr != pReqCacheData->pBuf && pReqCacheData->ulPos > 0)
		{
			memcpy(pNewBuf, pReqCacheData->pBuf, pReqCacheData->ulPos);
		}

		delete[]pReqCacheData->pBuf;
		pReqCacheData->pBuf = pNewBuf;
		pReqCacheData->ulCapacity = ulNewCapacity;
		return true;
	}

	// wyl 2026-03-30：完整消息处理完后，仅保留小块缓存复用，大块缓存及时释放，避免长连接长期占住峰值内存。
	void ShrinkWebCacheIfNeeded(ReqCacheData* pReqCacheData)
	{
		if (nullptr == pReqCacheData)
			return;

		if (pReqCacheData->ulCapacity > g_ulWebCacheKeepLen)
		{
			delete[]pReqCacheData->pBuf;
			pReqCacheData->pBuf = nullptr;
			pReqCacheData->ulCapacity = 0;
		}
	}

	// wyl 2026-03-30：复位 WebSocket 当前帧状态；在完整消息结束后再一起清理消息累计状态。
	void ResetWebFrameState(ReqCacheData* pReqCacheData, bool bResetMessageState)
	{
		if (nullptr == pReqCacheData)
			return;

		pReqCacheData->ulWsFrameLength = 0;
		pReqCacheData->ulWsFramePos = 0;
		pReqCacheData->ulWsControlLength = 0;
		pReqCacheData->ulWsControlPos = 0;
		pReqCacheData->bWsFinalFrame = true;
		pReqCacheData->bWsIgnoreFrame = false;
		pReqCacheData->ucWsFrameOperationCode = 0;

		if (bResetMessageState)
		{
			pReqCacheData->ulLength = 0;
			pReqCacheData->ulPos = 0;
			pReqCacheData->bWsMessageActive = false;
			pReqCacheData->ucWsOperationCode = 0;
			ShrinkWebCacheIfNeeded(pReqCacheData);
		}
	}

	// wyl 2026-03-30：统一封装 Web 通知任务提交流程，避免重复代码和失败路径遗漏清理。
	bool SubmitWebNotifyTask(ST_WEB_SERVER_RUNTIME* p_pRuntime,
		IHttpServer* pSender, CONNID dwConnID, NotifyTask* pNotifyTask)
	{
		if (p_pRuntime == nullptr || pSender == nullptr || pNotifyTask == nullptr)
		{
			return false;
		}

		pthread_mutex_lock(&p_pRuntime->mutexTask);
		pNotifyTask->ullTaskID = ++p_pRuntime->ullTaskId;
		ST_WEB_NOTIFY_QUEUE_V2& refQueue = p_pRuntime->mapNotifyQueue[dwConnID];
		if (refQueue.bClosed)
		{
			pthread_mutex_unlock(&p_pRuntime->mutexTask);
			delete pNotifyTask;
			return false;
		}
		refQueue.deqTasks.push_back(pNotifyTask);
		if (pNotifyTask->enWebNotifyType == enWebClose)
		{
			refQueue.bClosed = true;
		}
		if (refQueue.bDraining)
		{
			pthread_mutex_unlock(&p_pRuntime->mutexTask);
			return true;
		}
		refQueue.bDraining = true;

		if (!SubmitWebDrainTaskNoLock(p_pRuntime, pSender, dwConnID))
		{
			for (NotifyTask *pQueuedTask : refQueue.deqTasks)
			{
				delete pQueuedTask;
			}
			p_pRuntime->mapNotifyQueue.erase(dwConnID);
			pthread_mutex_unlock(&p_pRuntime->mutexTask);
			return false;
		}

		pthread_mutex_unlock(&p_pRuntime->mutexTask);
		return true;
	}
}

EnHttpParseResult CWebServerListerNet::OnRequestLine(IHttpServer*, CONNID, LPCSTR lpszMethod, LPCSTR)
{
	return (nullptr != lpszMethod
		&& (0 == strcmp(lpszMethod, "POST") || 0 == strcmp(lpszMethod, "GET")))
		? HPR_OK : HPR_ERROR;
}

EnHttpParseResult CWebServerListerNet::OnHeader(IHttpServer*, CONNID, LPCSTR, LPCSTR)
{
	// wyl 2026-03-30：握手改到 OnHeadersComplete() 统一处理，避免单个请求头到达时就提前回包。
	return HPR_OK;
}

EnHttpParseResult CWebServerListerNet::OnHeadersComplete(IHttpServer* pSender, CONNID dwConnID)
{
	if (m_pRuntime == nullptr || !m_pRuntime->bServerStatus.load())
		return HPR_ERROR;

	const char* pMethod = pSender->GetMethod(dwConnID);
	const char* lpszWebSocketKey = nullptr;
	if (!pSender->GetHeader(dwConnID, "Sec-WebSocket-Key", &lpszWebSocketKey)
		|| nullptr == lpszWebSocketKey || 0 == strlen(lpszWebSocketKey))
	{
		return HPR_OK;
	}

	const char* lpszUpgrade = nullptr;
	const char* lpszConnection = nullptr;
	const char* lpszVersion = nullptr;
	const bool bGetMethod = (nullptr != pMethod) && EqualsIgnoreCaseAscii(pMethod, "GET");
	const bool bUpgradeHeader = pSender->GetHeader(dwConnID, "Upgrade", &lpszUpgrade)
		&& nullptr != lpszUpgrade && EqualsIgnoreCaseAscii(lpszUpgrade, "websocket");
	const bool bConnectionHeader = pSender->GetHeader(dwConnID, "Connection", &lpszConnection)
		&& nullptr != lpszConnection && HeaderContainsTokenIgnoreCase(lpszConnection, "Upgrade");
	const bool bVersionHeader = pSender->GetHeader(dwConnID, "Sec-WebSocket-Version", &lpszVersion)
		&& nullptr != lpszVersion && EqualsIgnoreCaseAscii(lpszVersion, "13");
	const bool bUpgradeTypeOk = pSender->IsUpgrade(dwConnID) && HUT_WEB_SOCKET == pSender->GetUpgradeType(dwConnID);

	// wyl 2026-03-30：只有标准的 WebSocket 升级请求才进入握手响应，避免异常头部把普通 HTTP 请求误升级。
	if (!bGetMethod || !bUpgradeHeader || !bConnectionHeader || !bVersionHeader || !bUpgradeTypeOk)
	{
		WEB_WARN("ConnID=%llu,InvalidWebSocketHandshake", (unsigned long long)dwConnID);
		return HPR_ERROR;
	}

	char szAcceptKey[128] = { 0 };
	if (!BuildWebSocketAcceptKey(lpszWebSocketKey, szAcceptKey, sizeof(szAcceptKey)))
	{
		WEB_ERROR("ConnID=%llu,BuildWebSocketAcceptKeyFail", (unsigned long long)dwConnID);
		return HPR_ERROR;
	}

	THeader lpHeaders[3];
	lpHeaders[0].name = "Upgrade";
	lpHeaders[0].value = "websocket";
	lpHeaders[1].name = "Connection";
	lpHeaders[1].value = "Upgrade";
	lpHeaders[2].name = "Sec-WebSocket-Accept";
	lpHeaders[2].value = szAcceptKey;

	// wyl 2026-03-30：在请求头解析完整后统一发送握手响应，避免并发下静态缓冲串包和头部数量错误。
	if (!pSender->SendResponse(dwConnID, HSC_SWITCHING_PROTOCOLS, nullptr, lpHeaders, 3, nullptr, 0))
	{
		WEB_ERROR("ConnID=%llu,SendWebSocketHandshakeFail,err=%d", (unsigned long long)dwConnID, SYS_GetLastError());
		return HPR_ERROR;
	}

	// wyl 2026-03-30：101 响应发出后必须返回 HPR_UPGRADE，通知 HP-Socket 后续切到 WebSocket 回调链路。
	return HPR_UPGRADE;
}

EnHttpParseResult CWebServerListerNet::OnUpgrade(IHttpServer* pSender, CONNID dwConnID, EnHttpUpgradeType enUpgradeType)
{
	if (m_pRuntime == nullptr || !m_pRuntime->bServerStatus.load())
		return HPR_ERROR;

	if (HUT_WEB_SOCKET != enUpgradeType)
	{
		WEB_WARN("ConnID=%llu,UnexpectedUpgradeType=%d", (unsigned long long)dwConnID, (int)enUpgradeType);
		return HPR_ERROR;
	}

	bool bFoundClient = false;
	bool bAlreadyConnected = false;
	pthread_mutex_lock(&m_pRuntime->mutexConnection);
	std::map<CONNID, ClientData>::iterator itClient =
		m_pRuntime->mapClient.find(dwConnID);
	if (itClient != m_pRuntime->mapClient.end())
	{
		bFoundClient = true;
		bAlreadyConnected = itClient->second.bConnected;
		if (!bAlreadyConnected)
		{
			itClient->second.bConnected = true;
		}
	}
	pthread_mutex_unlock(&m_pRuntime->mutexConnection);

	if (!bFoundClient)
	{
		WEB_ERROR("ConnID=%llu,UpgradeWithoutClientState", (unsigned long long)dwConnID);
		return HPR_ERROR;
	}

	if (bAlreadyConnected)
		return HPR_OK;

	// wyl 2026-03-30：只有升级真正完成后才向上层派发连接成功，避免业务过早按 WebSocket 已就绪处理。
	NotifyTask *pNotifyTask = new (std::nothrow) NotifyTask();
	if (nullptr == pNotifyTask)
	{
		WEB_ERROR("ConnID=%llu,WebConnectNotifyAllocFail", (unsigned long long)dwConnID);
		return HPR_ERROR;
	}
	pNotifyTask->enWebNotifyType = enWebConnect;
	pNotifyTask->ullConnID = dwConnID;

	if (!SubmitWebNotifyTask(m_pRuntime, pSender, dwConnID, pNotifyTask))
	{
		pthread_mutex_lock(&m_pRuntime->mutexConnection);
		itClient = m_pRuntime->mapClient.find(dwConnID);
		if (itClient != m_pRuntime->mapClient.end())
		{
			itClient->second.bConnected = false;
		}
		pthread_mutex_unlock(&m_pRuntime->mutexConnection);
		return HPR_ERROR;
	}

	return HPR_OK;
}

EnHandleResult CWebServerListerNet::OnWSMessageHeader(IHttpServer* pSender, CONNID dwConnID, BOOL bFinal, BYTE iReserved, BYTE iOperationCode, const BYTE[4], ULONGLONG ullBodyLen)
{
	if (m_pRuntime == nullptr || !m_pRuntime->bServerStatus.load())
		return HR_ERROR;

	if (0 != iReserved)
	{
		WEB_ERROR("ConnID=%llu,WebSocketReservedBitsInvalid=%u", (unsigned long long)dwConnID, (unsigned int)iReserved);
		return HR_ERROR;
	}

	const bool bControlFrame = (0 != (iOperationCode & 0x08));
	// wyl 2026-03-30：控制帧必须是 FIN 且长度不超过 125 字节，先在头阶段做协议校验。
	if (bControlFrame && ((0 == bFinal) || ullBodyLen > g_ulWebControlFrameMaxLen))
	{
		WEB_ERROR("ConnID=%llu,WebSocketControlFrameInvalid,Opcode=%u,Final=%d,BodyLen=%llu",
			(unsigned long long)dwConnID, (unsigned int)iOperationCode, (int)bFinal, ullBodyLen);
		return HR_ERROR;
	}

	//iOperationCode 0:连接帧；1：文本帧；2：二进制数据；8：关闭；9：ping;10:pong
	if (iOperationCode == 8) //断开连接
	{
		// 对端关闭与业务发送共用同一连接序列，Close 之后不允许再写业务帧。
		CloseWebSocketFromPeerOrdered(m_pRuntime, pSender, dwConnID);
		return HR_OK;
	}

	if (!bControlFrame && ullBodyLen > g_ulWebMessageMaxLen)
	{
		WEB_ERROR("ConnID=%llu,WebSocketBodyTooLarge=%llu", (unsigned long long)dwConnID, ullBodyLen);
		return HR_ERROR;
	}

	pthread_mutex_lock(&m_pRuntime->mutexConnection);
	bool bHasClient = (m_pRuntime->mapClient.find(dwConnID) !=
		m_pRuntime->mapClient.end());
	pthread_mutex_unlock(&m_pRuntime->mutexConnection);
	if (!bHasClient)
		return HR_ERROR;

	bool bProtocolError = false;
	pthread_mutex_lock(&m_pRuntime->mutexRequest);
	// wyl 2026-04-25：帧头阶段准备连接级缓存，后续 Body 回调按该状态累计，Complete 阶段再统一判断是否完整。
	ReqCacheData *&refReqCacheData = m_pRuntime->mapRequest[dwConnID];
	if (nullptr == refReqCacheData)
	{
		// wyl 2026-04-25：WebSocket 分片缓存对象采用 nothrow 创建；失败时终止本连接解析，不再进入半初始化状态。
		refReqCacheData = new (std::nothrow) ReqCacheData();
		if (nullptr == refReqCacheData)
		{
			pthread_mutex_unlock(&m_pRuntime->mutexRequest);
			WEB_ERROR("ConnID=%llu,WebSocketCacheAllocFail", (unsigned long long)dwConnID);
			return HR_ERROR;
		}
		refReqCacheData->ullConnID = dwConnID;
	}

	const unsigned long ulFrameLen = (unsigned long)ullBodyLen;
	// wyl 2026-03-30：每次收到新帧头都先刷新“当前帧”状态，后续 body/complete 按这份状态累计和校验。
	refReqCacheData->ulWsFrameLength = ulFrameLen;
	refReqCacheData->ulWsFramePos = 0;
	refReqCacheData->ulWsControlLength = 0;
	refReqCacheData->ulWsControlPos = 0;
	refReqCacheData->bWsFinalFrame = (bFinal != 0);
	refReqCacheData->ucWsFrameOperationCode = iOperationCode;

	if (9 == iOperationCode || 10 == iOperationCode)
	{
		// wyl 2026-03-30：ping/pong 控制帧不向上层派发业务数据，但需要保留 ping 载荷用于回 pong。
		refReqCacheData->bWsIgnoreFrame = true;
		refReqCacheData->ulWsControlLength = ulFrameLen;
	}
	else if (0 == iOperationCode)
	{
		if (!refReqCacheData->bWsMessageActive)
		{
			bProtocolError = true;
		}
		else if (ulFrameLen > 0)
		{
			refReqCacheData->bWsIgnoreFrame = false;
			if (refReqCacheData->ulLength > g_ulWebMessageMaxLen - ulFrameLen
				|| !EnsureWebCacheCapacity(refReqCacheData, refReqCacheData->ulLength + ulFrameLen))
			{
				bProtocolError = true;
			}
			else
			{
				refReqCacheData->ulLength += ulFrameLen;
			}
		}
	}
	else if (1 == iOperationCode || 2 == iOperationCode)
	{
		if (refReqCacheData->bWsMessageActive)
		{
			bProtocolError = true;
		}
		else
		{
			refReqCacheData->bWsIgnoreFrame = false;
			refReqCacheData->bWsMessageActive = true;
			refReqCacheData->ucWsOperationCode = iOperationCode;
			refReqCacheData->ulLength = 0;
			refReqCacheData->ulPos = 0;

			if (ulFrameLen > 0)
			{
				if (!EnsureWebCacheCapacity(refReqCacheData, ulFrameLen))
				{
					bProtocolError = true;
				}
				else
				{
					refReqCacheData->ulLength = ulFrameLen;
				}
			}
		}
	}
	else
	{
		bProtocolError = true;
	}

	if (bProtocolError)
	{
		ResetWebFrameState(refReqCacheData, true);
	}
	pthread_mutex_unlock(&m_pRuntime->mutexRequest);

	if (bProtocolError)
	{
		WEB_ERROR("ConnID=%llu,WebSocketFrameStateInvalid,Opcode=%u,BodyLen=%llu", (unsigned long long)dwConnID, (unsigned int)iOperationCode, ullBodyLen);
		return HR_ERROR;
	}

	return HR_OK;
}

//接收到数据事件 收到数据时触发
EnHandleResult CWebServerListerNet::OnWSMessageBody(IHttpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength)
{
	// wyl 2026-03-30：停服边界直接拒绝后续收包，避免缓存写入已经无效的运行时状态。
	if (m_pRuntime == nullptr || !m_pRuntime->bServerStatus.load())
		return HR_ERROR;

	pthread_mutex_lock(&m_pRuntime->mutexConnection);
	if (m_pRuntime->mapClient.find(dwConnID) == m_pRuntime->mapClient.end())
	{
		pthread_mutex_unlock(&m_pRuntime->mutexConnection);
		return HR_ERROR;
	}
	pthread_mutex_unlock(&m_pRuntime->mutexConnection);

	if (nullptr == pData || iLength <= 0)
		return HR_OK;

	bool bProtocolError = false;
	pthread_mutex_lock(&m_pRuntime->mutexRequest);
	std::map<CONNID, ReqCacheData*>::iterator itReqCache =
		m_pRuntime->mapRequest.find(dwConnID);
	if (itReqCache == m_pRuntime->mapRequest.end() || nullptr == itReqCache->second)
	{
		bProtocolError = true;
	}
	else
	{
		ReqCacheData *pReqCacheData = itReqCache->second;
		if (pReqCacheData->ulWsFramePos > pReqCacheData->ulWsFrameLength
			|| (unsigned long)iLength > pReqCacheData->ulWsFrameLength - pReqCacheData->ulWsFramePos)
		{
			bProtocolError = true;
		}
		else if (9 == pReqCacheData->ucWsFrameOperationCode || 10 == pReqCacheData->ucWsFrameOperationCode)
		{
			// wyl 2026-03-30：控制帧单独累计到固定小缓冲里，和业务消息缓存隔离，避免污染消息拼包状态。
			if ((unsigned long)iLength > g_ulWebControlFrameMaxLen
				|| pReqCacheData->ulWsControlPos > pReqCacheData->ulWsControlLength
				|| (unsigned long)iLength > pReqCacheData->ulWsControlLength - pReqCacheData->ulWsControlPos)
			{
				bProtocolError = true;
			}
			else if (iLength > 0)
			{
				memcpy(pReqCacheData->szWsControlBuf + pReqCacheData->ulWsControlPos, pData, iLength);
				pReqCacheData->ulWsControlPos += iLength;
			}
		}
		else if (!pReqCacheData->bWsIgnoreFrame)
		{
			// wyl 2026-03-30：数据帧只做顺序累计，不在 body 阶段提前通知上层，等 complete 统一上抛。
			if (!pReqCacheData->bWsMessageActive
				|| pReqCacheData->ulPos > pReqCacheData->ulLength
				|| (unsigned long)iLength > pReqCacheData->ulLength - pReqCacheData->ulPos)
			{
				bProtocolError = true;
			}
			else
			{
				memcpy(pReqCacheData->pBuf + pReqCacheData->ulPos, pData, iLength);
				pReqCacheData->ulPos += iLength;
			}
		}

		if (!bProtocolError)
		{
			pReqCacheData->ulWsFramePos += iLength;
		}
		else
		{
			ResetWebFrameState(pReqCacheData, true);
		}
	}
	pthread_mutex_unlock(&m_pRuntime->mutexRequest);

	if (bProtocolError)
	{
		WEB_ERROR("ConnID=%llu,WebSocketBodyStateInvalid,BodyLen=%d", (unsigned long long)dwConnID, iLength);
		return HR_ERROR;
	}

	return HR_OK;
}

EnHandleResult CWebServerListerNet::OnWSMessageComplete(IHttpServer* pSender, CONNID dwConnID)
{
	if (m_pRuntime == nullptr || !m_pRuntime->bServerStatus.load())
		return HR_ERROR;

	// wyl 2026-04-25：Complete 阶段只在整条 WebSocket 消息收齐后创建通知任务，上层不会再收到半包。
	NotifyTask *pNotifyTask = nullptr;
	bool bProtocolError = false;
	bool bAllocError = false;
	bool bNeedPong = false;
	int iPongLen = 0;
	unsigned long ulNotifyLenForLog = 0;
	char szPongBuf[125] = { 0 };

	pthread_mutex_lock(&m_pRuntime->mutexRequest);
	std::map<CONNID, ReqCacheData*>::iterator itReqCache =
		m_pRuntime->mapRequest.find(dwConnID);
	if (itReqCache != m_pRuntime->mapRequest.end() && nullptr != itReqCache->second)
	{
		ReqCacheData *pReqCacheData = itReqCache->second;
		// wyl 2026-04-25：先校验当前帧 BODY 是否收齐，未收齐说明分片状态异常，不能向上层派发。
		if (pReqCacheData->ulWsFramePos != pReqCacheData->ulWsFrameLength)
		{
			bProtocolError = true;
			ResetWebFrameState(pReqCacheData, true);
		}
		else if (9 == pReqCacheData->ucWsFrameOperationCode)
		{
			if (pReqCacheData->ulWsControlPos != pReqCacheData->ulWsControlLength)
			{
				bProtocolError = true;
				ResetWebFrameState(pReqCacheData, true);
			}
			else
			{
				// wyl 2026-03-30：ping 在帧完整结束时回 pong，并按协议原样带回 ping 的载荷。
				bNeedPong = true;
				iPongLen = (int)pReqCacheData->ulWsControlPos;
				if (iPongLen > 0)
				{
					memcpy(szPongBuf, pReqCacheData->szWsControlBuf, iPongLen);
				}
				ResetWebFrameState(pReqCacheData, false);
			}
		}
		else if (10 == pReqCacheData->ucWsFrameOperationCode)
		{
			if (pReqCacheData->ulWsControlPos != pReqCacheData->ulWsControlLength)
			{
				bProtocolError = true;
				ResetWebFrameState(pReqCacheData, true);
			}
			else
			{
				ResetWebFrameState(pReqCacheData, false);
			}
		}
		else if (pReqCacheData->bWsIgnoreFrame)
		{
			ResetWebFrameState(pReqCacheData, false);
		}
		else if (!pReqCacheData->bWsFinalFrame)
		{
			ResetWebFrameState(pReqCacheData, false);
		}
		else
		{
			// wyl 2026-03-30：只在完整 WebSocket 消息结束时向上层派发一次数据通知。
			if (1 == pReqCacheData->ucWsOperationCode || 2 == pReqCacheData->ucWsOperationCode)
			{
				pNotifyTask = new (std::nothrow) NotifyTask();
				ulNotifyLenForLog = pReqCacheData->ulPos;
				if (nullptr == pNotifyTask)
				{
					bAllocError = true;
				}
				else
				{
					pNotifyTask->enWebNotifyType = enWebData;
					pNotifyTask->ullConnID = dwConnID;
					pNotifyTask->uiLen = (unsigned int)pReqCacheData->ulPos;
					if (pReqCacheData->ulPos > 0)
					{
						if (pReqCacheData->ulCapacity > g_ulWebCacheKeepLen)
						{
							// wyl 2026-04-25：大包缓存已经是一条完整 WebSocket 消息，直接把缓冲所有权转给通知任务。
							// wyl 2026-04-25：这样避免再申请一块同等大小内存并 memcpy；转移后 ReqCacheData 不再释放这块缓冲。
							pNotifyTask->pBuf = pReqCacheData->pBuf;
							pReqCacheData->pBuf = nullptr;
							pReqCacheData->ulCapacity = 0;
						}
						else
						{
							// wyl 2026-04-25：小包继续深拷贝到通知任务，连接缓存保留下来复用，降低频繁申请释放的成本。
							pNotifyTask->pBuf = new (std::nothrow) char[pReqCacheData->ulPos];
							if (nullptr == pNotifyTask->pBuf)
							{
								bAllocError = true;
								delete pNotifyTask;
								pNotifyTask = nullptr;
							}
							else
							{
								memcpy(pNotifyTask->pBuf, pReqCacheData->pBuf, pReqCacheData->ulPos);
							}
						}
					}
				}
			}

			ResetWebFrameState(pReqCacheData, true);
		}
	}
	pthread_mutex_unlock(&m_pRuntime->mutexRequest);

	if (bProtocolError)
	{
		WEB_ERROR("ConnID=%llu,WebSocketCompleteStateInvalid", (unsigned long long)dwConnID);
		return HR_ERROR;
	}


	if (bAllocError)
	{
		WEB_ERROR("ConnID=%llu,WebSocketNotifyAllocFail,len=%lu", (unsigned long long)dwConnID, ulNotifyLenForLog);
		return HR_ERROR;
	}
	if (bNeedPong)
	{
		// wyl 2026-03-30：pong 先于业务通知发送，避免上层处理较慢时影响心跳往返时延。
		if (!SendWebSocketControlFrameOrdered(m_pRuntime, pSender, dwConnID, 10,
			iPongLen > 0 ? (const BYTE*)szPongBuf : nullptr, iPongLen, "SendPong"))
		{
			WEB_WARN("ConnID=%llu,SendPongFail,err=%d", (unsigned long long)dwConnID, SYS_GetLastError());
			return HR_ERROR;
		}
	}

	if (nullptr == pNotifyTask)
		return HR_OK;

	// wyl 2026-04-25：派发前占用单连接配额，防止上层慢消费时通知任务和内存无限堆积。
	if (!ReserveWebPendingQuota(m_pRuntime, dwConnID, pNotifyTask->uiLen))
	{
		WEB_ERROR("ConnID=%llu,PendingWebNotifyOverflow,len=%u", (unsigned long long)dwConnID, pNotifyTask->uiLen);
		delete pNotifyTask;
		return HR_ERROR;
	}

	const unsigned int uiNotifyLen = pNotifyTask->uiLen;
	if (!SubmitWebNotifyTask(m_pRuntime, pSender, dwConnID, pNotifyTask))
	{
		ReleaseWebPendingQuota(m_pRuntime, dwConnID, uiNotifyLen);
		return HR_ERROR;
	}

	return HR_OK;
}

// 客户端连接事件 监听成功时触发
EnHandleResult CWebServerListerNet::OnPrepareListen(ITcpServer*, SOCKET)
{
	// wyl 2026-03-30：启动完成前禁止继续处理监听回调，避免进入未就绪状态。
	if (m_pRuntime == nullptr || !m_pRuntime->bServerStatus.load())
		return HR_ERROR;

	return HR_OK;
}

// 客户端连接事件 接收到连接时触发
EnHandleResult CWebServerListerNet::OnAccept(ITcpServer* pSender, CONNID dwConnID, UINT_PTR)
{
	// wyl 2026-03-30：停服过程中不再接受新连接，避免连接表和任务表继续膨胀。
	if (m_pRuntime == nullptr || !m_pRuntime->bServerStatus.load())
		return HR_ERROR;

	// 客户端连接
	// 如果服务器这里做处理业务，需要为每个新接入的连接附加一个对象

	//获取监听的ip port信息
	char szAddress[100] = { 0 };
	int iAddressLen = sizeof(szAddress);
	USHORT usPort = 0;

	pSender->GetRemoteAddress(dwConnID, szAddress, iAddressLen, usPort);

	//管理连接
	pthread_mutex_lock(&m_pRuntime->mutexConnection);
	ClientData &refClientData = m_pRuntime->mapClient[dwConnID];
	refClientData.ullConnID = dwConnID;
	refClientData.unPort = usPort;
	// wyl 2026-03-30：WebSocket 连接要等 HTTP Upgrade 成功后才算业务层真正建链，这里先记为未连接。
	refClientData.bConnected = false;
	// wyl 2026-04-08：项目侧统一使用 char 地址缓存，避免字符集宏扩散到业务代码。
	SafeCopyCString(refClientData.szIp, sizeof(refClientData.szIp), szAddress);
	pthread_mutex_unlock(&m_pRuntime->mutexConnection);
	return HR_OK;
}

////////////////////////////////////////////////

// 客户端关闭事件
EnHandleResult CWebServerListerNet::OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode)
{
	WEB_INFO("ConnID=%llu,Operation=%d,ErrorCode=%d",
		(unsigned long long)dwConnID, enOperation, iErrorCode);
	if (m_pRuntime == nullptr || !m_pRuntime->bServerStatus.load())
		return HR_ERROR;
	MarkWebSocketSendConnectionClosed(m_pRuntime, dwConnID);

	bool bHasClient = false;
	bool bLocalClosing = false;
	bool bConnected = false;
	pthread_mutex_lock(&m_pRuntime->mutexConnection);
	// wyl 2026-03-30：主动断连由本端标记判断，不再依赖 SO_CLOSE 这类操作类型猜测关闭来源。
	if (m_pRuntime->setLocalClosing.find(dwConnID) !=
		m_pRuntime->setLocalClosing.end())
	{
		bLocalClosing = true;
		m_pRuntime->setLocalClosing.erase(dwConnID);
	}

	std::map<CONNID, ClientData>::iterator itClient =
		m_pRuntime->mapClient.find(dwConnID);
	if (itClient != m_pRuntime->mapClient.end())
	{
		bHasClient = true;
		bConnected = itClient->second.bConnected;
		// wyl 2026-05-19：OnClose 一进入就先撤销可发送状态，避免关闭通知排队期间业务线程继续对旧 ConnID 推送。
		itClient->second.bConnected = false;
		if (bLocalClosing || !bConnected)
		{
			m_pRuntime->mapClient.erase(itClient);
		}
	}
	pthread_mutex_unlock(&m_pRuntime->mutexConnection);

	if (bLocalClosing || !bConnected)
	{
		// wyl 2026-03-30：本端主动关闭，或尚未完成 Upgrade 就断开的连接，只做内部清理，不向上层重复发关闭通知。
		pthread_mutex_lock(&m_pRuntime->mutexRequest);
		if (m_pRuntime->mapRequest.find(dwConnID) !=
			m_pRuntime->mapRequest.end())
		{
			delete m_pRuntime->mapRequest[dwConnID];
			m_pRuntime->mapRequest.erase(dwConnID);
		}
		pthread_mutex_unlock(&m_pRuntime->mutexRequest);
		return HR_OK;
	}

	if (!bHasClient)
		return HR_ERROR;

	NotifyTask *pNotifyTask = new (std::nothrow) NotifyTask();
	if (nullptr == pNotifyTask)
	{
		WEB_ERROR("ConnID=%llu,WebCloseNotifyAllocFail", (unsigned long long)dwConnID);
		return HR_ERROR;
	}
	pNotifyTask->enWebNotifyType = enWebClose;
	pNotifyTask->ullConnID = dwConnID;
	return SubmitWebNotifyTask(m_pRuntime, (IHttpServer*)pSender,
		dwConnID, pNotifyTask) ? HR_OK : HR_ERROR;
}

// 发送数据完成事件 发送数据成功时触发
EnHandleResult CWebServerListerNet::OnSend(ITcpServer*, CONNID, const BYTE*, int)
{
	return HR_OK;
}

EnHandleResult CWebServerListerNet::OnReceive(ITcpServer*, CONNID dwConnID, int iLength)
{
	// wyl 2026-03-30：Pull 模型收包路径同样增加服务状态保护，避免停服后继续分配缓存。
	if (m_pRuntime == nullptr || !m_pRuntime->bServerStatus.load())
		return HR_ERROR;

	pthread_mutex_lock(&m_pRuntime->mutexConnection);
	if (m_pRuntime->mapClient.find(dwConnID) == m_pRuntime->mapClient.end())
	{
		pthread_mutex_unlock(&m_pRuntime->mutexConnection);
		return HR_ERROR;
	}
	pthread_mutex_unlock(&m_pRuntime->mutexConnection);

	// wyl 2026-03-30：当前 Web 服务的数据主链路走 HTTP 解析和 WebSocket 回调，这个原始 Pull 收包回调不应进入。
	// wyl 2026-04-25：如果这里被触发，通常说明底层模型或配置和当前实现预期不一致，直接拒绝比继续分配错误缓存更安全。
	WEB_WARN("ConnID=%llu,UnexpectedPullReceiveLen=%d", (unsigned long long)dwConnID, iLength);
	return HR_ERROR;
}

// 服务器占用端口事件 握手成功时触发
EnHandleResult CWebServerListerNet::OnHandShake(ITcpServer*, CONNID)
{
	return HR_OK;
}

// server shutdown event
EnHandleResult CWebServerListerNet::OnShutdown(ITcpServer*)
{
	WEB_INFO("server shutdown");
	return HR_OK;
}
