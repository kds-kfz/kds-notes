#include "publicGlobalvar.h"
#include "Log.h"

//通知任务
void ThreadNotifyTask(LPTSocketTask socketTask)
{
	if (nullptr == g_pTcpHandle)
		return;

	IHttpServer *pSender = (IHttpServer *)socketTask->sender;
	NotifyTask *pstTask = (NotifyTask *)socketTask->buf;

	ClientData stClientData;
	pthread_mutex_lock(&g_mutexConnet);
	/*
	if (g_mapClient.find(pstTask->ullConnID) == g_mapClient.end())
	{
		g_mutexConnet.unlock();
		return;
	}

	stClientData = g_mapClient[pstTask->ullConnID];
	if (enTcpClose == pstTask->enNotifyType)
	{
		g_mapClient.erase(pstTask->ullConnID);
	}
	*/
	stClientData = g_mapClient[pstTask->ullConnID];
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

	switch (pstTask->enNotifyType)
	{
	case enTcpData:
		INFO("%s ip=%s,port=%d,type=%d,len=%d,msg=%s", g_strServerName.c_str(),
			stClientData.szIp, stClientData.unPort, pstTask->enNotifyType, pstTask->uiLen, pstTask->pBuf);
		g_pTcpHandle((void*)pSender, (void*)pstTask->ullConnID, pstTask->enNotifyType, (void*)pstTask->pBuf, pstTask->uiLen,
			stClientData.szIp, stClientData.unPort, pstTask->szErrMsg);
		break;
	case enTcpClose:
		_snprintf(pstTask->szErrMsg, sizeof(pstTask->szErrMsg), "client close");
		INFO("%s ip=%s,port=%d,type=%d,len=%d,msg=%s", g_strServerName.c_str(),
			stClientData.szIp, stClientData.unPort, pstTask->enNotifyType, strlen(pstTask->szErrMsg), pstTask->szErrMsg);
		g_pTcpHandle((void*)pSender, (void*)pstTask->ullConnID, pstTask->enNotifyType, NULL, 0,
			stClientData.szIp, stClientData.unPort, pstTask->szErrMsg);
		break;
	case enTcpConnect:
		_snprintf(pstTask->szErrMsg, sizeof(pstTask->szErrMsg), "client connect");
		INFO("%s ip=%s,port=%d,type=%d,len=%d,msg=%s", g_strServerName.c_str(),
			stClientData.szIp, stClientData.unPort, pstTask->enNotifyType, strlen(pstTask->szErrMsg), pstTask->szErrMsg);
		g_pTcpHandle((void*)pSender, (void*)pstTask->ullConnID, pstTask->enNotifyType, NULL, 0,
			stClientData.szIp, stClientData.unPort, pstTask->szErrMsg);
		break;
	case enTcpError:
		INFO("%s ip=%s,port=%d,type=%d,len=%d,msg=%s", g_strServerName.c_str(),
			stClientData.szIp, stClientData.unPort, pstTask->enNotifyType, strlen(pstTask->szErrMsg), pstTask->szErrMsg);
		g_pTcpHandle((void*)pSender, (void*)pstTask->ullConnID, pstTask->enNotifyType, NULL, 0,
			stClientData.szIp, stClientData.unPort, pstTask->szErrMsg);
		break;
	default:
		break;
	}

	pthread_mutex_lock(&g_mutexTask);
	if (g_mapTask.find(pstTask->ullTaskID) != g_mapTask.end())
	{
		delete g_mapTask[pstTask->ullTaskID];
		g_mapTask.erase(pstTask->ullTaskID);
	}
	pthread_mutex_unlock(&g_mutexTask);
}

// 客户端连接事件 监听成功时触发
EnHandleResult CTcpServerListerNet::OnPrepareListen(ITcpServer* pSender, SOCKET soListen)
{
	// 准备监听
	//获取监听的ip port信息
	TCHAR lpszAddress[30] = { 0 };
	int iAddressLen = 0;
	USHORT unPort = 0;
	pSender->GetListenAddress(lpszAddress, iAddressLen, unPort);

	printf("%s\n", __FUNCTION__);
	return HR_OK;
}

// 客户端连接事件 接收到连接时触发
EnHandleResult CTcpServerListerNet::OnAccept(ITcpServer* pSender, CONNID dwConnID, UINT_PTR soClient)
{
	printf("%s\n", __FUNCTION__);
	// 客户端连接
	// 如果服务器这里做处理业务，需要为每个新接入的连接附加一个对象

	//获取监听的ip port信息
	TCHAR szAddress[100];
	int iAddressLen = sizeof(szAddress) / sizeof(TCHAR);
	USHORT usPort;

	pSender->GetRemoteAddress(dwConnID, szAddress, iAddressLen, usPort);

	//管理连接
	pthread_mutex_lock(&g_mutexConnet);
	ClientData &refClientData = g_mapClient[dwConnID];
	refClientData.ullConnID = dwConnID;
	refClientData.unPort = usPort;
	memcpy(refClientData.szIp, szAddress, 32);
	pthread_mutex_unlock(&g_mutexConnet);

	//TODO 发出连接通知
	NotifyTask *pNotifyTask = new NotifyTask();
	pNotifyTask->enNotifyType = enTcpConnect;
	pNotifyTask->ullConnID = dwConnID;
	
	pthread_mutex_lock(&g_mutexTask);
	pNotifyTask->ullTaskID = ++g_ullTaskID;
	g_mapTask[g_ullTaskID] = pNotifyTask;
	pthread_mutex_unlock(&g_mutexTask);

	LPTSocketTask task = NULL;
	task = HP_Create_SocketTaskObj((Fn_SocketTaskProc)ThreadNotifyTask, pSender, dwConnID, (const BYTE*)pNotifyTask, sizeof(NotifyTask));
	if (task == nullptr)
	{
		delete pNotifyTask;
		return HR_ERROR;
	}

	if (!g_CHPThreadPool->Submit(task, 1000 * 5))
	{
		delete pNotifyTask;
		HP_Destroy_SocketTaskObj(task);
		return HR_ERROR;
	}

	return HR_OK;
}

// 客户端关闭事件
EnHandleResult CTcpServerListerNet::OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode)
{
	printf("%s,%d,%d\n", __FUNCTION__, enOperation, iErrorCode);
	INFO("%s ConnID=%d,Operation=%d,ErrorCode=%d", g_strServerName.c_str(),
		dwConnID, enOperation, iErrorCode);
	if(SO_CLOSE == enOperation || (SO_RECEIVE == enOperation && 1223 == iErrorCode))//是服务器断开客户端
		return HR_ERROR;

	pthread_mutex_lock(&g_mutexConnet);
	if (g_mapClient.find(dwConnID) == g_mapClient.end())
	{
		pthread_mutex_unlock(&g_mutexConnet);
		return HR_ERROR;
	}
	pthread_mutex_unlock(&g_mutexConnet);

	// 客户端关闭
	//if (SO_CLOSE != enOperation)
	{
		//TODO 通知上层应用关闭连接
		NotifyTask *pNotifyTask = new NotifyTask();
		pNotifyTask->enNotifyType = enTcpClose;
		pNotifyTask->ullConnID = dwConnID;

		pthread_mutex_lock(&g_mutexTask);
		pNotifyTask->ullTaskID = ++g_ullTaskID;
		g_mapTask[g_ullTaskID] = pNotifyTask;
		pthread_mutex_unlock(&g_mutexTask);

		LPTSocketTask task = NULL;
		task = HP_Create_SocketTaskObj((Fn_SocketTaskProc)ThreadNotifyTask, pSender, dwConnID, (const BYTE*)pNotifyTask, sizeof(NotifyTask));
		if (task == nullptr)
		{
			delete pNotifyTask;
			return HR_ERROR;
		}

		if (!g_CHPThreadPool->Submit(task, 1000 * 5))
		{
			delete pNotifyTask;
			HP_Destroy_SocketTaskObj(task);
			return HR_ERROR;
		}
	}
	return HR_OK;
}

// 发送数据完成事件 发送数据成功时触发
EnHandleResult CTcpServerListerNet::OnSend(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength)
{
	// 数据发送完成

	printf("%s len=%d\n", __FUNCTION__, iLength);
	return HR_OK;
}

// 接收到数据事件 收到数据时触发
EnHandleResult CTcpServerListerNet::OnReceive(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength)
{
	printf("%s\n", __FUNCTION__);

	pthread_mutex_lock(&g_mutexConnet);
	if (g_mapClient.find(dwConnID) == g_mapClient.end())
	{
		pthread_mutex_unlock(&g_mutexConnet);
		return HR_ERROR;
	}
	pthread_mutex_unlock(&g_mutexConnet);

	// 收到数据
	pthread_mutex_lock(&g_mutexReq);
	if (g_mapQueue.find(dwConnID) == g_mapQueue.end())
	{
		ReqCacheData *pReqCacheData = new ReqCacheData();
		pReqCacheData->ulLength = iLength;
		pReqCacheData->pBuf = new char[iLength];
		g_mapQueue[dwConnID] = pReqCacheData;
	}
	else
	{
		ReqCacheData *&pReqCacheData = g_mapQueue[dwConnID];
		if (pReqCacheData->ulPos == 0)
		{
			pReqCacheData->ulLength = iLength;
			pReqCacheData->pBuf = new char[iLength];
		}
	}

	ReqCacheData *& refReqCacheData = g_mapQueue[dwConnID];

	if (refReqCacheData->ulPos < iLength)
	{
		memcpy(refReqCacheData->pBuf + refReqCacheData->ulPos, pData, iLength);
		refReqCacheData->ulPos += iLength;
	}
	if (refReqCacheData->ulPos >= refReqCacheData->ulLength)//完整包
	{
		//TODO 通知上层应用获取数据
		NotifyTask *pNotifyTask = new NotifyTask();
		pNotifyTask->enNotifyType = enTcpData;
		pNotifyTask->ullConnID = dwConnID;
		pNotifyTask->uiLen = refReqCacheData->ulLength;
		pNotifyTask->pBuf = new char[refReqCacheData->ulLength];
		memcpy(pNotifyTask->pBuf, refReqCacheData->pBuf, refReqCacheData->ulLength);

		pthread_mutex_lock(&g_mutexTask);
		pNotifyTask->ullTaskID = ++g_ullTaskID;
		g_mapTask[g_ullTaskID] = pNotifyTask;
		pthread_mutex_unlock(&g_mutexTask);

		LPTSocketTask task = NULL;
		task = HP_Create_SocketTaskObj((Fn_SocketTaskProc)ThreadNotifyTask, pSender, dwConnID, (const BYTE*)pNotifyTask, sizeof(NotifyTask));
		if (task == nullptr)
		{
			delete pNotifyTask;
			return HR_ERROR;
		}

		if (!g_CHPThreadPool->Submit(task, 1000 * 5))
		{
			delete pNotifyTask;
			HP_Destroy_SocketTaskObj(task);
			return HR_ERROR;
		}

		//通知完成释放数据
		delete []refReqCacheData->pBuf;
		refReqCacheData->pBuf = nullptr;
		refReqCacheData->ulPos = 0;
	}

	pthread_mutex_unlock(&g_mutexReq);
	
	return HR_OK;
}

EnHandleResult CTcpServerListerNet::OnReceive(ITcpServer* pSender, CONNID dwConnID, int iLength)
{
	printf("%s\n", __FUNCTION__);

	pthread_mutex_lock(&g_mutexConnet);
	if (g_mapClient.find(dwConnID) == g_mapClient.end())
	{
		pthread_mutex_unlock(&g_mutexConnet);
		return HR_ERROR;
	}
	pthread_mutex_unlock(&g_mutexConnet);

	// 收到数据

	if (iLength > pSender->GetSocketBufferSize())
	{
		NotifyTask *pNotifyTask = new NotifyTask();
		pNotifyTask->enNotifyType = enTcpError;
		pNotifyTask->ullConnID = dwConnID;
		_snprintf(pNotifyTask->szErrMsg, sizeof(pNotifyTask->szErrMsg), "recv len=%d > %llu", iLength, pSender->GetSocketBufferSize());

		pthread_mutex_lock(&g_mutexTask);
		pNotifyTask->ullTaskID = ++g_ullTaskID;
		g_mapTask[g_ullTaskID] = pNotifyTask;
		pthread_mutex_unlock(&g_mutexTask);

		LPTSocketTask task = NULL;
		task = HP_Create_SocketTaskObj((Fn_SocketTaskProc)ThreadNotifyTask, pSender, dwConnID, (const BYTE*)pNotifyTask, sizeof(NotifyTask));
		if (task == nullptr)
		{
			delete pNotifyTask;
			return HR_ERROR;
		}

		if (!g_CHPThreadPool->Submit(task, 1000 * 5))
		{
			delete pNotifyTask;
			HP_Destroy_SocketTaskObj(task);
			return HR_ERROR;
		}

		return HR_ERROR;
	}

	pthread_mutex_lock(&g_mutexReq);
	if (g_mapQueue.find(dwConnID) == g_mapQueue.end())
	{
		ReqCacheData *pReqCacheData = new ReqCacheData();
		pReqCacheData->ulLength = iLength;
		pReqCacheData->pBuf = new char[iLength];
		g_mapQueue[dwConnID] = pReqCacheData;
	}
	else
	{
		ReqCacheData *&pReqCacheData = g_mapQueue[dwConnID];
		pReqCacheData->ulLength = iLength;
		pReqCacheData->pBuf = new char[iLength];
	}
	pthread_mutex_unlock(&g_mutexReq);

	return HR_OK;
}

// 服务器占用端口事件 握手成功时触发
EnHandleResult CTcpServerListerNet::OnHandShake(ITcpServer* pSender, CONNID dwConnID)
{
	// 握手
	printf("%s\n", __FUNCTION__);
	return HR_OK;
}

//服务器关闭时触发
EnHandleResult CTcpServerListerNet::OnShutdown(ITcpServer* pSender)
{
	printf("%s\n", __FUNCTION__);
	INFO("%s 服务器关闭", g_strServerName.c_str());
	return HR_OK;
}
