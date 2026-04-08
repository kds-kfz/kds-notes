#include "HttpSockServerObj.h"
#include "publicGlobalvar.h"
#include "publicfunc.h"
#include "Log.h"

namespace
{
	bool g_bHttpMutexInit = false;

	// wyl 2026-04-07：HTTP 服务当前只需要保护请求对象表和连接解析映射。
	void InitHttpMutexes()
	{
		if (g_bHttpMutexInit)
			return;

		pthread_mutex_init(&g_mutexHttpReq, nullptr);
		g_bHttpMutexInit = true;
	}

	void DestroyHttpMutexes()
	{
		if (!g_bHttpMutexInit)
			return;

		pthread_mutex_destroy(&g_mutexHttpReq);
		g_bHttpMutexInit = false;
	}

	// wyl 2026-04-07：统一清空 HTTP 异步请求对象和当前连接解析状态，避免旧状态残留到下一次启动。
	void ClearHttpRuntimeData()
	{
		if (!g_bHttpMutexInit)
			return;

		pthread_mutex_lock(&g_mutexHttpReq);
		foreach(g_mapHttpReq, it_req)
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
		g_mapHttpReq.clear();
		g_mapHttpConnReq.clear();
		g_mapHttpConnActiveReq.clear();
		pthread_mutex_unlock(&g_mutexHttpReq);
	}
}

void DeleteHttpObj(void)
{
	// wyl 2026-04-07：停服时先拉低 Http 运行状态，再停止服务和线程池，避免回调继续进入无效状态。
	g_bHttpServerStatus = false;

	if (nullptr != g_CHttpPackServer)
	{
		g_CHttpPackServer->Stop();
	}

	// wyl 2026-04-07：先等待 HTTP 业务回调自然退出，再销毁底层服务对象，避免上层回包访问悬空指针。
	g_CHttpHPThreadPool->Stop();

	// wyl 2026-04-07：线程池停稳后清理运行态，请求对象里的 sender 仍然有效，可安全断开 transport。
	ClearHttpRuntimeData();

	if (nullptr != g_CHttpPackServer)
	{
		HP_Destroy_HttpServer(g_CHttpPackServer);
		g_CHttpPackServer = NULL;
	}

	if (nullptr != g_CHttpServerListerNet)
	{
		delete g_CHttpServerListerNet;
		g_CHttpServerListerNet = nullptr;
	}
	DestroyHttpMutexes();

	g_pHttpHandle = nullptr;
	g_ullHttpAsynReqID = 0;

	CHttpLog::Release();
}

CHttpSockServerObj::CHttpSockServerObj()
{

}

CHttpSockServerObj::~CHttpSockServerObj()
{
	DeleteHttpObj();
}

bool CHttpSockServerObj::CreateHttpSock(const char* p_szIp, unsigned short p_unPort, unsigned int p_uiRBufLen, unsigned int p_uiMaxConnectNum, unsigned int p_uiMaxAcceptNum,
	HTTP_NOTIFY_PROC p_httpHandle, unsigned int p_uiThreadNum, unsigned int p_uiQueueNum, char* p_szErr, const char* p_szLogFold)
{
	if (nullptr == p_szErr)
		return false;

	if (nullptr == p_szIp || 7 > strlen(p_szIp))
	{
		_snprintf(p_szErr, 1024, "code=-1,msg=init param err");
		return false;
	}

	// wyl 2026-04-07：启动前先清理旧实例残留，避免重复启动时复用脏状态。
	DeleteHttpObj();

	int iRet = 0;

	//1.初始化日志
	if (nullptr != p_szLogFold && strlen(p_szLogFold) > 0)
	{
		// wyl 2026-04-07：Http 服务使用独立日志单例，避免与 TCP 服务共享日志路径和生命周期。
		if (MA_OK == CHttpLog::GetInstance()->InitLog(p_szLogFold))
		{
			CHttpLog::GetInstance()->Resume();//恢复工作
			HTTP_INFO("启动LOG  ****************************");
		}
		else
		{
			_snprintf(p_szErr, 1024, "code=-2,msg=log init fail");
			DeleteHttpObj();
			return false;
		}
		// 设置日志等级
		CHttpLog::GetInstance()->SetLogLevel((char *)"info");
	}

	// 设置任务回调
	g_pHttpHandle = p_httpHandle;

	// wyl 2026-04-07：先初始化锁和线程池，再启动网络监听，减少启动窗口期竞态。
	InitHttpMutexes();

	//2.设置线程池
	g_CHttpHPThreadPool->AdjustThreadCount(p_uiThreadNum);
	if (!g_CHttpHPThreadPool->Start(p_uiThreadNum, p_uiQueueNum, TRP_CALL_FAIL, 0))
	{
		_snprintf(p_szErr, 1024, "code=%d,msg=thread pool start fail", SYS_GetLastError());
		DeleteHttpObj();
		return false;
	}

	//3.创建服务监听器
	if (nullptr == g_CHttpServerListerNet)
	{
		g_CHttpServerListerNet = new CHttpServerListerNet();
	}

	if (nullptr == g_CHttpServerListerNet)
	{
		iRet = SYS_GetLastError();
		_snprintf(p_szErr, 1024, "code=%d,msg=create http server listener fail", iRet);
		DeleteHttpObj();
		return false;
	}

	//4.创建服务
	if (nullptr == g_CHttpPackServer)
	{
		g_CHttpPackServer = HP_Create_HttpServer(g_CHttpServerListerNet);
	}

	if (nullptr == g_CHttpPackServer)
	{
		_snprintf(p_szErr, 1024, "code=%d,msg=create http server fail", SYS_GetLastError());
		DeleteHttpObj();
		return false;
	}

	//5.设置超时心跳
	g_CHttpPackServer->SetKeepAliveTime(2000);
	g_CHttpPackServer->SetKeepAliveInterval(1000);

	//6.设置缓存大小
	g_CHttpPackServer->SetSocketBufferSize(p_uiRBufLen);

	//7.设置最大连接数
	g_CHttpPackServer->SetMaxConnectionCount(p_uiMaxConnectNum);

	// wyl 2026-04-07：这里设置的是底层 Accept 预分配数量，不是“同一 IP 最大连接数”限流。
	//8.设置Accept大小
	g_CHttpPackServer->SetAcceptSocketCount(p_uiMaxAcceptNum);

	// wyl 2026-04-07：资源准备完成后再标记 Http 服务可运行，供回调路径做状态保护。
	g_bHttpServerStatus = true;

	//9.启动服务
	const std::basic_string<TCHAR> strBindAddress = MakeBindAddress(p_szIp);
	if (!g_CHttpPackServer->Start(strBindAddress.c_str(), p_unPort))
	{
		char szErrDesc[256] = { 0 };
		CopyTextToAnsi(szErrDesc, sizeof(szErrDesc), g_CHttpPackServer->GetLastErrorDesc());
		_snprintf(p_szErr, 1024, "code=%d,msg=%s",
			g_CHttpPackServer->GetLastError(), szErrDesc);
		DeleteHttpObj();
		return false;
	}

	HTTP_INFO("启动完成");
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
	DeleteHttpObj();
}

bool CHttpSockServerObj::DelHttpAsynReq(unsigned long long p_lluReqId)
{
	if (!g_bHttpMutexInit)
		return false;

	CHttpAsynReqObj* pReqObj = nullptr;
	pthread_mutex_lock(&g_mutexHttpReq);
	std::map<unsigned long long, CHttpAsynReqObj*>::iterator itReq = g_mapHttpReq.find(p_lluReqId);
	if (itReq == g_mapHttpReq.end())
	{
		pthread_mutex_unlock(&g_mutexHttpReq);
		return false;
	}

	pReqObj = itReq->second;
	std::map<CONNID, unsigned long long>::iterator itConnReq = g_mapHttpConnReq.find((CONNID)pReqObj->GetConnId());
	if (itConnReq != g_mapHttpConnReq.end() && itConnReq->second == p_lluReqId)
	{
		g_mapHttpConnReq.erase(itConnReq);
	}

	std::map<CONNID, unsigned long long>::iterator itActiveReq = g_mapHttpConnActiveReq.find((CONNID)pReqObj->GetConnId());
	if (itActiveReq != g_mapHttpConnActiveReq.end() && itActiveReq->second == p_lluReqId)
	{
		g_mapHttpConnActiveReq.erase(itActiveReq);
	}

	g_mapHttpReq.erase(itReq);
	pthread_mutex_unlock(&g_mutexHttpReq);

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

