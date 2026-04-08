#include "publicGlobalvar.h"
#include "Log.h"

namespace
{
	// wyl 2026-03-30：单连接允许排队的 TCP 数据通知任务上限，防止小包洪泛把线程池任务队列顶满。
	const unsigned int g_uiTcpMaxPendingTaskPerConn = 1024;
	// wyl 2026-03-30：单连接允许排队的 TCP 数据通知累计字节上限，限制慢消费时的内存占用。
	const unsigned long long g_ullTcpMaxPendingBytesPerConn = 4ULL * 1024 * 1024;

	// wyl 2026-03-30：统一处理 TCHAR 到 char 的地址复制，兼容 Unicode 配置。
	void CopyClientIp(char* pDst, size_t dwDstLen, const TCHAR* pSrc)
	{
		if (nullptr == pDst || 0 == dwDstLen)
			return;

		pDst[0] = '\0';
		if (nullptr == pSrc)
			return;

#if defined(UNICODE) || defined(_UNICODE)
		WideCharToMultiByte(CP_ACP, 0, pSrc, -1, pDst, (int)dwDstLen, nullptr, nullptr);
#else
		_snprintf(pDst, dwDstLen, "%s", pSrc);
#endif
		pDst[dwDstLen - 1] = '\0';
	}

	// wyl 2026-03-30：为单连接的 TCP 原始数据通知做轻量配额控制，避免小包洪泛把任务队列和内存持续顶满。
	bool ReserveTcpPendingQuota(CONNID dwConnID, unsigned int uiDataLen)
	{
		if (0 == uiDataLen)
			return true;

		bool bReserved = false;
		pthread_mutex_lock(&g_mutexReq);
		ReqCacheData *&refReqCacheData = g_mapQueue[dwConnID];
		if (nullptr == refReqCacheData)
		{
			refReqCacheData = new ReqCacheData();
			refReqCacheData->ullConnID = dwConnID;
		}

		if (uiDataLen <= g_ullTcpMaxPendingBytesPerConn
			&& refReqCacheData->uiTcpPendingTaskCount < g_uiTcpMaxPendingTaskPerConn
			&& refReqCacheData->ullTcpPendingBytes <= g_ullTcpMaxPendingBytesPerConn - uiDataLen)
		{
			++refReqCacheData->uiTcpPendingTaskCount;
			refReqCacheData->ullTcpPendingBytes += uiDataLen;
			bReserved = true;
		}
		pthread_mutex_unlock(&g_mutexReq);
		return bReserved;
	}

	// wyl 2026-03-30：数据通知完成后归还配额，空闲时顺手移除统计对象，避免无效状态长期残留。
	void ReleaseTcpPendingQuota(CONNID dwConnID, unsigned int uiDataLen)
	{
		pthread_mutex_lock(&g_mutexReq);
		std::map<CONNID, ReqCacheData*>::iterator itReq = g_mapQueue.find(dwConnID);
		if (itReq != g_mapQueue.end() && nullptr != itReq->second)
		{
			ReqCacheData *pReqCacheData = itReq->second;
			if (pReqCacheData->uiTcpPendingTaskCount > 0)
			{
				--pReqCacheData->uiTcpPendingTaskCount;
			}
			if (pReqCacheData->ullTcpPendingBytes >= uiDataLen)
			{
				pReqCacheData->ullTcpPendingBytes -= uiDataLen;
			}
			else
			{
				pReqCacheData->ullTcpPendingBytes = 0;
			}

			if (0 == pReqCacheData->uiTcpPendingTaskCount
				&& 0 == pReqCacheData->ullTcpPendingBytes
				&& 0 == pReqCacheData->ulLength
				&& 0 == pReqCacheData->ulPos
				&& 0 == pReqCacheData->ulCapacity)
			{
				delete pReqCacheData;
				g_mapQueue.erase(itReq);
			}
		}
		pthread_mutex_unlock(&g_mutexReq);
	}
}

//通知任务
void ThreadNotifyTask(LPTSocketTask socketTask)
{
	// wyl 2026-03-30：回调入口先做空指针保护，避免停服边界下访问失效任务对象。
	if (nullptr == socketTask || nullptr == socketTask->buf)
		return;

	ITcpServer *pSender = (ITcpServer *)socketTask->sender;
	NotifyTask *pstTask = (NotifyTask *)socketTask->buf;

	ClientData stClientData;
	pthread_mutex_lock(&g_mutexConnet);
	std::map<CONNID, ClientData>::iterator itClient = g_mapClient.find(pstTask->ullConnID);
	if (itClient != g_mapClient.end())
	{
		stClientData = itClient->second;
	}
	if (enTcpClose == pstTask->enNotifyType)
	{
		g_mapClient.erase(pstTask->ullConnID);
	}
	pthread_mutex_unlock(&g_mutexConnet);

	if (enTcpClose == pstTask->enNotifyType)
	{
		pthread_mutex_lock(&g_mutexReq);
		if (g_mapQueue.find(pstTask->ullConnID) != g_mapQueue.end())
		{
			delete g_mapQueue[pstTask->ullConnID];
			g_mapQueue.erase(pstTask->ullConnID);
		}
		pthread_mutex_unlock(&g_mutexReq);
	}

	// wyl 2026-03-30：只有服务仍处于运行状态时，才继续向上层派发通知。
	if (nullptr != g_pTcpHandle && g_bServerStatus)
	{
		switch (pstTask->enNotifyType)
		{
		case enTcpData:
			// wyl 2026-03-30：不再按字符串打印原始 TCP 数据，避免二进制数据越界读取。
			TCP_INFO("ip=%s,port=%d,type=%d,len=%u",
				stClientData.szIp, stClientData.unPort, pstTask->enNotifyType, pstTask->uiLen);
			g_pTcpHandle((void*)pSender, (void*)pstTask->ullConnID, pstTask->enNotifyType, (void*)pstTask->pBuf, pstTask->uiLen,
				stClientData.szIp, stClientData.unPort, pstTask->szErrMsg);
			break;
		case enTcpClose:
			_snprintf(pstTask->szErrMsg, sizeof(pstTask->szErrMsg), "client close");
			TCP_INFO("ip=%s,port=%d,type=%d,len=%d,msg=%s",
				stClientData.szIp, stClientData.unPort, pstTask->enNotifyType, (int)strlen(pstTask->szErrMsg), pstTask->szErrMsg);
			g_pTcpHandle((void*)pSender, (void*)pstTask->ullConnID, pstTask->enNotifyType, NULL, 0,
				stClientData.szIp, stClientData.unPort, pstTask->szErrMsg);
			break;
		case enTcpConnect:
			_snprintf(pstTask->szErrMsg, sizeof(pstTask->szErrMsg), "client connect");
			TCP_INFO("ip=%s,port=%d,type=%d,len=%d,msg=%s",
				stClientData.szIp, stClientData.unPort, pstTask->enNotifyType, (int)strlen(pstTask->szErrMsg), pstTask->szErrMsg);
			g_pTcpHandle((void*)pSender, (void*)pstTask->ullConnID, pstTask->enNotifyType, NULL, 0,
				stClientData.szIp, stClientData.unPort, pstTask->szErrMsg);
			break;
		case enTcpError:
			TCP_INFO("ip=%s,port=%d,type=%d,len=%d,msg=%s",
				stClientData.szIp, stClientData.unPort, pstTask->enNotifyType, (int)strlen(pstTask->szErrMsg), pstTask->szErrMsg);
			g_pTcpHandle((void*)pSender, (void*)pstTask->ullConnID, pstTask->enNotifyType, NULL, 0,
				stClientData.szIp, stClientData.unPort, pstTask->szErrMsg);
			break;
		default:
			break;
		}
	}

	if (enTcpData == pstTask->enNotifyType)
	{
		ReleaseTcpPendingQuota(pstTask->ullConnID, pstTask->uiLen);
	}

	pthread_mutex_lock(&g_mutexTask);
	if (g_mapTask.find(pstTask->ullTaskID) != g_mapTask.end())
	{
		delete g_mapTask[pstTask->ullTaskID];
		g_mapTask.erase(pstTask->ullTaskID);
	}
	pthread_mutex_unlock(&g_mutexTask);
}

namespace
{
	// wyl 2026-03-30：统一封装 TCP 通知任务提交流程，避免重复代码和失败路径遗漏清理。
	bool SubmitTcpNotifyTask(ITcpServer* pSender, CONNID dwConnID, NotifyTask* pNotifyTask)
	{
		if (nullptr == pSender || nullptr == pNotifyTask)
			return false;

		unsigned long long ullTaskID = 0;
		pthread_mutex_lock(&g_mutexTask);
		pNotifyTask->ullTaskID = ++g_ullTaskID;
		ullTaskID = pNotifyTask->ullTaskID;
		g_mapTask[ullTaskID] = pNotifyTask;
		pthread_mutex_unlock(&g_mutexTask);

		LPTSocketTask task = HP_Create_SocketTaskObj((Fn_SocketTaskProc)ThreadNotifyTask, pSender, dwConnID, (const BYTE*)pNotifyTask, sizeof(NotifyTask));
		if (task == nullptr)
		{
			pthread_mutex_lock(&g_mutexTask);
			g_mapTask.erase(ullTaskID);
			pthread_mutex_unlock(&g_mutexTask);
			delete pNotifyTask;
			return false;
		}

		if (!g_CTcpHPThreadPool->Submit(task, 1000 * 5))
		{
			pthread_mutex_lock(&g_mutexTask);
			g_mapTask.erase(ullTaskID);
			pthread_mutex_unlock(&g_mutexTask);
			delete pNotifyTask;
			HP_Destroy_SocketTaskObj(task);
			return false;
		}

		return true;
	}
}

// 客户端连接事件 监听成功时触发
EnHandleResult CTcpServerListerNet::OnPrepareListen(ITcpServer* pSender, SOCKET soListen)
{
	// wyl 2026-03-30：启动完成前禁止继续处理监听回调，避免进入未就绪状态。
	if (!g_bServerStatus)
		return HR_ERROR;

	// 准备监听
	//获取监听的ip port信息
	TCHAR lpszAddress[30] = { 0 };
	int iAddressLen = sizeof(lpszAddress) / sizeof(TCHAR);
	USHORT unPort = 0;
	pSender->GetListenAddress(lpszAddress, iAddressLen, unPort);
	return HR_OK;
}

// 客户端连接事件 接收到连接时触发
EnHandleResult CTcpServerListerNet::OnAccept(ITcpServer* pSender, CONNID dwConnID, UINT_PTR soClient)
{
	// wyl 2026-03-30：停服过程中不再接受新连接，避免连接表和任务表继续膨胀。
	if (!g_bServerStatus)
		return HR_ERROR;

	// 客户端连接
	// 如果服务器这里做处理业务，需要为每个新接入的连接附加一个对象

	//获取监听的ip port信息
	TCHAR szAddress[100] = { 0 };
	int iAddressLen = sizeof(szAddress) / sizeof(TCHAR);
	USHORT usPort = 0;

	pSender->GetRemoteAddress(dwConnID, szAddress, iAddressLen, usPort);

	//管理连接
	pthread_mutex_lock(&g_mutexConnet);
	ClientData &refClientData = g_mapClient[dwConnID];
	refClientData.ullConnID = dwConnID;
	refClientData.unPort = usPort;
	// wyl 2026-03-30：TCP 连接在 accept 成功后即可视为业务层可用，直接标记为已连接。
	refClientData.bConnected = true;
	// wyl 2026-03-30：客户端地址统一按字符集安全复制，避免 Unicode 配置下地址乱码。
	CopyClientIp(refClientData.szIp, sizeof(refClientData.szIp), szAddress);
	pthread_mutex_unlock(&g_mutexConnet);

	//TODO 发出连接通知
	NotifyTask *pNotifyTask = new NotifyTask();
	pNotifyTask->enNotifyType = enTcpConnect;
	pNotifyTask->ullConnID = dwConnID;

	return SubmitTcpNotifyTask(pSender, dwConnID, pNotifyTask) ? HR_OK : HR_ERROR;
}

// 客户端关闭事件
EnHandleResult CTcpServerListerNet::OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode)
{
	TCP_INFO("ConnID=%llu,Operation=%d,ErrorCode=%d",
		(unsigned long long)dwConnID, enOperation, iErrorCode);
	if (!g_bServerStatus)
		return HR_ERROR;

	bool bHasClient = false;
	bool bLocalClosing = false;
	pthread_mutex_lock(&g_mutexConnet);
	// wyl 2026-03-30：主动断连由本端标记判断，不再依赖 SO_RECEIVE + 1223 这类平台相关错误码猜测。
	if (g_setTcpLocalClosing.find(dwConnID) != g_setTcpLocalClosing.end())
	{
		bLocalClosing = true;
		g_setTcpLocalClosing.erase(dwConnID);
	}

	std::map<CONNID, ClientData>::iterator itClient = g_mapClient.find(dwConnID);
	if (itClient != g_mapClient.end())
	{
		bHasClient = true;
		if (bLocalClosing)
		{
			g_mapClient.erase(itClient);
		}
	}
	pthread_mutex_unlock(&g_mutexConnet);

	if (bLocalClosing)
	{
		pthread_mutex_lock(&g_mutexReq);
		if (g_mapQueue.find(dwConnID) != g_mapQueue.end())
		{
			delete g_mapQueue[dwConnID];
			g_mapQueue.erase(dwConnID);
		}
		pthread_mutex_unlock(&g_mutexReq);
		return HR_OK;
	}

	if (!bHasClient)
		return HR_ERROR;

	// 客户端关闭
	//if (SO_CLOSE != enOperation)
	{
		//TODO 通知上层应用关闭连接
		NotifyTask *pNotifyTask = new NotifyTask();
		pNotifyTask->enNotifyType = enTcpClose;
		pNotifyTask->ullConnID = dwConnID;
		return SubmitTcpNotifyTask(pSender, dwConnID, pNotifyTask) ? HR_OK : HR_ERROR;
	}
	return HR_OK;
}

// 发送数据完成事件 发送数据成功时触发
EnHandleResult CTcpServerListerNet::OnSend(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength)
{
	// 数据发送完成
	return HR_OK;
}

// 接收到数据事件 收到数据时触发
EnHandleResult CTcpServerListerNet::OnReceive(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength)
{
	// wyl 2026-03-30：停服边界直接拒绝后续收包，避免缓存写入已经无效的运行时状态。
	if (!g_bServerStatus)
		return HR_ERROR;

	// wyl 2026-03-30：当前 TCP 服务不在库内做拆包组包，收到多少字节就原样抛给上层缓存处理。
	if (nullptr == pData || iLength <= 0)
		return HR_OK;

	pthread_mutex_lock(&g_mutexConnet);
	if (g_mapClient.find(dwConnID) == g_mapClient.end())
	{
		pthread_mutex_unlock(&g_mutexConnet);
		return HR_ERROR;
	}
	pthread_mutex_unlock(&g_mutexConnet);

	if (!ReserveTcpPendingQuota(dwConnID, (unsigned int)iLength))
	{
		TCP_ERROR("ConnID=%llu,PendingTcpNotifyOverflow,len=%d", (unsigned long long)dwConnID, iLength);
		return HR_ERROR;
	}

	NotifyTask *pNotifyTask = new NotifyTask();
	pNotifyTask->enNotifyType = enTcpData;
	pNotifyTask->ullConnID = dwConnID;
	pNotifyTask->uiLen = (unsigned int)iLength;
	pNotifyTask->pBuf = new char[iLength];
	memcpy(pNotifyTask->pBuf, pData, iLength);

	if (!SubmitTcpNotifyTask(pSender, dwConnID, pNotifyTask))
	{
		ReleaseTcpPendingQuota(dwConnID, (unsigned int)iLength);
		return HR_ERROR;
	}

	return HR_OK;
}

EnHandleResult CTcpServerListerNet::OnReceive(ITcpServer* pSender, CONNID dwConnID, int iLength)
{
	// wyl 2026-03-30：当前创建的是 Push 模型 TCP 服务，这个 Pull 回调理论上不应进入。
	if (!g_bServerStatus)
		return HR_ERROR;

	TCP_WARN("ConnID=%llu,UnexpectedPullReceiveLen=%d", (unsigned long long)dwConnID, iLength);
	return HR_ERROR;
}

// 服务器占用端口事件 握手成功时触发
EnHandleResult CTcpServerListerNet::OnHandShake(ITcpServer* pSender, CONNID dwConnID)
{
	// 握手
	return HR_OK;
}

//服务器关闭时触发
EnHandleResult CTcpServerListerNet::OnShutdown(ITcpServer* pSender)
{
	TCP_INFO("服务器关闭");
	return HR_OK;
}
