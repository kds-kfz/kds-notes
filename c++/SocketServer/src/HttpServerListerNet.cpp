#include "publicGlobalvar.h"
#include "publicfunc.h"
#include "ServerRuntimeContext.h"
#include "Log.h"
#include <new>
#include <windows.h>

namespace
{
	// wyl 2026-04-25：HTTP 请求保护阈值：限制头数量、头总字节和 BODY 总字节，避免异常请求或大包拖垮内存。
	const size_t HTTP_MAX_REQ_HEADER_COUNT = 128;			// wyl 2026-04-25：单个 HTTP 请求允许的最大请求头数量
	const size_t HTTP_MAX_REQ_HEADER_BYTES = 32 * 1024;	// wyl 2026-04-25：单个 HTTP 请求允许的请求头累计字节数
	const size_t HTTP_MAX_REQ_BODY_BYTES = 8 * 1024 * 1024;	// wyl 2026-04-25：单个 HTTP 请求允许的 BODY 最大字节数
	const size_t HTTP_MAX_RAW_URL_BYTES = 16 * 1024;		// 请求目标最大长度，限制异常 GET query 的内存占用
	const size_t HTTP_MAX_QUERY_PARAM_COUNT = 128;		// 单个请求允许的 query 参数数量

	const char* GetHttpQueryParseReason(EN_HTTP_QUERY_PARSE_RESULT p_enResult)
	{
		switch (p_enResult)
		{
		case HTTP_QUERY_PARSE_RAW_URL_TOO_LONG:
			return "request url too long";
		case HTTP_QUERY_PARSE_PARAM_TOO_MANY:
			return "too many query params";
		case HTTP_QUERY_PARSE_DECODED_NUL:
			return "query contains nul";
		default:
			return "request query invalid";
		}
	}

	bool HttpSocketIsConnectedNoThrow(IHttpServer* pSender, CONNID dwConnID, const char* p_szAction)
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
			HTTP_ERROR("ConnID=%llu,%sIsAliveException=0x%08X",
				(unsigned long long)dwConnID, nullptr != p_szAction ? p_szAction : "Http", dwExceptionCode);
			return false;
		}
		return bAlive ? true : false;
	}

	bool SendHttpResponseNoThrow(IHttpServer* pSender, CONNID dwConnID, HttpStatusType enStatus,
		const THeader* pHeaders, int iHeaderCount, const BYTE* pBody, int iBodyLen, const char* p_szAction)
	{
		if (nullptr == pSender)
			return false;

		BOOL bOK = FALSE;
		DWORD dwExceptionCode = 0;
		__try
		{
			bOK = pSender->SendResponse(dwConnID, (USHORT)enStatus, nullptr, pHeaders, iHeaderCount, pBody, iBodyLen);
		}
		__except (dwExceptionCode = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
		{
			// wyl 2026-05-19：HTTP 解析阶段错误回包也保护第三方库 native 异常，避免无 dump 时缺少现场。
			HTTP_ERROR("ConnID=%llu,%sException=0x%08X,Status=%d,BodyLen=%d",
				(unsigned long long)dwConnID, nullptr != p_szAction ? p_szAction : "SendResponse",
				dwExceptionCode, (int)enStatus, iBodyLen);
			return false;
		}
		return bOK ? true : false;
	}

	bool PauseHttpReceiveNoThrow(IHttpServer* pSender, CONNID dwConnID, BOOL bPause, const char* p_szAction)
	{
		if (nullptr == pSender)
			return false;

		BOOL bOK = FALSE;
		DWORD dwExceptionCode = 0;
		__try
		{
			bOK = pSender->PauseReceive(dwConnID, bPause);
		}
		__except (dwExceptionCode = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
		{
			HTTP_ERROR("ConnID=%llu,%sException=0x%08X,Pause=%d",
				(unsigned long long)dwConnID, nullptr != p_szAction ? p_szAction : "PauseReceive",
				dwExceptionCode, (int)bPause);
			return false;
		}
		return bOK ? true : false;
	}

	bool ReleaseHttpConnNoThrow(IHttpServer* pSender, CONNID dwConnID, const char* p_szAction)
	{
		if (nullptr == pSender)
			return false;

		BOOL bOK = FALSE;
		DWORD dwExceptionCode = 0;
		__try
		{
			bOK = pSender->Release(dwConnID);
		}
		__except (dwExceptionCode = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
		{
			HTTP_ERROR("ConnID=%llu,%sException=0x%08X",
				(unsigned long long)dwConnID, nullptr != p_szAction ? p_szAction : "ReleaseConn", dwExceptionCode);
			return false;
		}
		return bOK ? true : false;
	}

	bool DisconnectHttpConnNoThrow(IHttpServer* pSender, CONNID dwConnID, const char* p_szAction)
	{
		if (nullptr == pSender)
			return false;

		BOOL bOK = FALSE;
		DWORD dwExceptionCode = 0;
		__try
		{
			// false 表示优雅断开，让已经进入发送队列的 HTTP 错误响应先发给客户端。
			bOK = pSender->Disconnect(dwConnID, false);
		}
		__except (dwExceptionCode = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER)
		{
			HTTP_ERROR("ConnID=%llu,%sException=0x%08X",
				(unsigned long long)dwConnID, nullptr != p_szAction ? p_szAction : "DisconnectConn", dwExceptionCode);
			return false;
		}
		return bOK ? true : false;
	}

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

	bool IsSupportedHttpMethod(const char* p_szMethod)
	{
		static const char* s_ppMethodList[] =
		{
			"GET", "POST", "PUT", "DELETE", "PATCH", "HEAD", "OPTIONS"
		};

		if (nullptr == p_szMethod || '\0' == *p_szMethod)
			return false;

		for (size_t i = 0; i < sizeof(s_ppMethodList) / sizeof(s_ppMethodList[0]); ++i)
		{
			if (EqualsIgnoreCaseAscii(p_szMethod, s_ppMethodList[i]))
				return true;
		}

		return false;
	}

	bool FindHttpParsingReqNoLock(ST_HTTP_SERVER_RUNTIME* p_pRuntime,
		CONNID dwConnID, unsigned long long& ullReqID,
		CHttpAsynReqObj*& pReqObj)
	{
		pReqObj = nullptr;
		ullReqID = 0;
		if (p_pRuntime == nullptr)
		{
			return false;
		}

		std::map<CONNID, unsigned long long>::iterator itConnReq =
			p_pRuntime->mapParsingRequest.find(dwConnID);
		if (itConnReq == p_pRuntime->mapParsingRequest.end())
			return false;

		ullReqID = itConnReq->second;
		std::map<unsigned long long, CHttpAsynReqObj*>::iterator itReq =
			p_pRuntime->mapRequest.find(ullReqID);
		if (itReq == p_pRuntime->mapRequest.end() || nullptr == itReq->second)
			return false;

		pReqObj = itReq->second;
		return true;
	}

	void CleanupHttpParsingReq(ST_HTTP_SERVER_RUNTIME* p_pRuntime,
		CONNID dwConnID)
	{
		if (p_pRuntime == nullptr)
		{
			return;
		}
		pthread_mutex_lock(&p_pRuntime->mutexRequest);
		unsigned long long ullReqID = 0;
		CHttpAsynReqObj* pReqObj = nullptr;
		if (FindHttpParsingReqNoLock(p_pRuntime, dwConnID,
			ullReqID, pReqObj))
		{
			p_pRuntime->mapParsingRequest.erase(dwConnID);

			std::map<unsigned long long, CHttpAsynReqObj*>::iterator itReq =
				p_pRuntime->mapRequest.find(ullReqID);
			if (itReq != p_pRuntime->mapRequest.end() &&
				nullptr != itReq->second && !itReq->second->IsDispatched())
			{
				delete itReq->second;
				p_pRuntime->mapRequest.erase(itReq);
			}
		}
		pthread_mutex_unlock(&p_pRuntime->mutexRequest);
	}

	void DetachHttpActiveReq(ST_HTTP_SERVER_RUNTIME* p_pRuntime,
		CONNID dwConnID)
	{
		if (p_pRuntime == nullptr)
		{
			return;
		}
		pthread_mutex_lock(&p_pRuntime->mutexRequest);
		std::map<CONNID, unsigned long long>::iterator itActiveReq =
			p_pRuntime->mapActiveRequest.find(dwConnID);
		if (itActiveReq != p_pRuntime->mapActiveRequest.end())
		{
			const unsigned long long ullReqID = itActiveReq->second;
			p_pRuntime->mapActiveRequest.erase(itActiveReq);

			std::map<unsigned long long, CHttpAsynReqObj*>::iterator itReq =
				p_pRuntime->mapRequest.find(ullReqID);
			if (itReq != p_pRuntime->mapRequest.end() && nullptr != itReq->second)
			{
				itReq->second->DetachTransport();
			}
		}
		pthread_mutex_unlock(&p_pRuntime->mutexRequest);
	}

	bool SendSimpleHttpError(ST_HTTP_SERVER_RUNTIME* p_pRuntime,
		IHttpServer* pSender, CONNID dwConnID, HttpStatusType enStatus,
		const char* p_szMsg)
	{
		if (nullptr == pSender)
			return false;

		const char* p_szBody = (nullptr == p_szMsg) ? "" : p_szMsg;
		const THeader stHeaders[] =
		{
			{ "Content-Type", "text/plain; charset=utf-8" },
			{ "Connection", "close" }
		};

		bool bSendOk = false;
		if (p_pRuntime != nullptr && p_pRuntime->bServerStatus.load() &&
			pSender == p_pRuntime->pPackServer &&
			HttpSocketIsConnectedNoThrow(pSender, dwConnID, "RejectRequest"))
		{
			bSendOk = SendHttpResponseNoThrow(pSender, dwConnID, enStatus, stHeaders,
				sizeof(stHeaders) / sizeof(stHeaders[0]), reinterpret_cast<const BYTE*>(p_szBody), (int)strlen(p_szBody), "RejectSendResponse");
		}
		else
		{
			HTTP_WARN("ConnID=%llu,SkipRejectResponseClosed,Status=%d", (unsigned long long)dwConnID, (int)enStatus);
		}

		const bool bCloseOk = bSendOk
			? DisconnectHttpConnNoThrow(pSender, dwConnID, "RejectDisconnectConn")
			: ReleaseHttpConnNoThrow(pSender, dwConnID, "RejectReleaseConn");
		if (!bCloseOk)
		{
			HTTP_WARN("ConnID=%llu,RejectCloseFail,Sent=%d,err=%d",
				(unsigned long long)dwConnID, bSendOk ? 1 : 0, SYS_GetLastError());
		}

		return bSendOk;
	}

	EnHttpParseResult RejectHttpRequest(ST_HTTP_SERVER_RUNTIME* p_pRuntime,
		IHttpServer* pSender, CONNID dwConnID, HttpStatusType enStatus,
		const char* p_szReason)
	{
		HTTP_WARN("ConnID=%llu,RejectRequest,Status=%d,Reason=%s",
			(unsigned long long)dwConnID, (int)enStatus, nullptr == p_szReason ? "" : p_szReason);

		if (!SendSimpleHttpError(p_pRuntime, pSender, dwConnID,
			enStatus, p_szReason))
		{
			HTTP_WARN("ConnID=%llu,RejectResponseSendFail,err=%d", (unsigned long long)dwConnID, SYS_GetLastError());
		}

		CleanupHttpParsingReq(p_pRuntime, dwConnID);
		return HPR_ERROR;
	}

	void ThreadHttpRequestTask(LPTSocketTask socketTask)
	{
		if (nullptr == socketTask || nullptr == socketTask->buf)
			return;

		ST_HTTP_SERVER_RUNTIME* pRuntime =
			static_cast<ST_HTTP_SERVER_RUNTIME*>(socketTask->sender);
		CHttpAsynReqObj* pReqObj = *(CHttpAsynReqObj**)socketTask->buf;
		if (pRuntime == nullptr || pReqObj == nullptr)
			return;

		if (pRuntime->pNotifyHandler != nullptr &&
			pRuntime->bServerStatus.load())
		{
			pRuntime->pNotifyHandler(pReqObj);
		}
	}

	bool SubmitHttpRequestTask(ST_HTTP_SERVER_RUNTIME* p_pRuntime,
		CHttpAsynReqObj* pReqObj)
	{
		if (p_pRuntime == nullptr || pReqObj == nullptr ||
			p_pRuntime->pNotifyHandler == nullptr ||
			!p_pRuntime->bServerStatus.load())
			return false;

		CHttpAsynReqObj* pTaskReqObj = pReqObj;
		LPTSocketTask task = HP_Create_SocketTaskObj(
			(Fn_SocketTaskProc)ThreadHttpRequestTask, p_pRuntime,
			(CONNID)pReqObj->GetConnId(), (const BYTE*)&pTaskReqObj, sizeof(pTaskReqObj));
		if (nullptr == task)
			return false;

		if (!p_pRuntime->clThreadPool->Submit(task, 1000 * 5))
		{
			HP_Destroy_SocketTaskObj(task);
			return false;
		}

		return true;
	}
}

// 以下别名只在 Listener 成员函数内展开为当前实例字段，不对应进程级全局变量。
#define g_bHttpServerStatus (m_pRuntime->bServerStatus.load())
#define g_mutexHttpReq (m_pRuntime->mutexRequest)
#define g_mapHttpReq (m_pRuntime->mapRequest)
#define g_mapHttpConnReq (m_pRuntime->mapParsingRequest)
#define g_mapHttpConnActiveReq (m_pRuntime->mapActiveRequest)
#define g_ullHttpAsynReqID (m_pRuntime->ullAsyncRequestId)
#define g_CHttpSockServerObj (m_pRuntime->pOwner)

EnHttpParseResult CHttpServerListerNet::OnMessageBegin(IHttpServer*, CONNID dwConnID)
{
	if (!g_bHttpServerStatus)
		return HPR_ERROR;

	// wyl 2026-04-25：当前实现不支持同一连接并发处理多个 HTTP 请求；若上一个请求还没收尾，这里直接拒绝继续解析。
	pthread_mutex_lock(&g_mutexHttpReq);
	const bool bHasParsingReq = (g_mapHttpConnReq.find(dwConnID) != g_mapHttpConnReq.end());
	const bool bHasActiveReq = (g_mapHttpConnActiveReq.find(dwConnID) != g_mapHttpConnActiveReq.end());
	pthread_mutex_unlock(&g_mutexHttpReq);

	if (bHasParsingReq || bHasActiveReq)
	{
		HTTP_WARN("ConnID=%llu,PipelineOrDanglingRequestNotSupported", (unsigned long long)dwConnID);
		return HPR_ERROR;
	}

	return HPR_OK;
}

EnHttpParseResult CHttpServerListerNet::OnRequestLine(IHttpServer* pSender, CONNID dwConnID, LPCSTR lpszMethod, LPCSTR lpszUrl)
{
	if (!g_bHttpServerStatus)
		return HPR_ERROR;

	if (!IsSupportedHttpMethod(lpszMethod))
	{
		HTTP_WARN("ConnID=%llu,UnsupportedMethod=%s",
			(unsigned long long)dwConnID, nullptr == lpszMethod ? "" : lpszMethod);
		return RejectHttpRequest(m_pRuntime, pSender, dwConnID,
			NotImplemented, "method not support");
	}

	// wyl 2026-04-25：请求行阶段创建请求上下文；使用 nothrow，内存不足时直接回复 500，不让异常穿透 HP-Socket 回调。
	CHttpAsynReqObj* pReqObj = new (std::nothrow)
		CHttpAsynReqObj(m_pRuntime);
	if (nullptr == pReqObj)
	{
		HTTP_ERROR("ConnID=%llu,CreateHttpReqAllocFail", (unsigned long long)dwConnID);
		return RejectHttpRequest(m_pRuntime, pSender, dwConnID,
			InternalServerError, "request alloc fail");
	}

	// wyl 2026-04-25：这里只记录 method、url、client 地址等元信息；BODY 由后续 OnBody 分片累计，不在这里读取或打印。
	try
	{
		pReqObj->SetSender(pSender);
		pReqObj->SetConnId(dwConnID);
		pReqObj->SetMethod(lpszMethod);
		// 优先使用 HP-Socket 已拆分的 query，网络层通用解析全部参数，不绑定任何业务参数名。
		const char* p_szQuery = pSender->GetUrlField(dwConnID, HUF_QUERY);
		size_t uiObservedRawUrlBytes = 0;
		size_t uiObservedQueryParamCount = 0;
		const EN_HTTP_QUERY_PARSE_RESULT enQueryResult = pReqObj->SetRequestUrl(lpszUrl, p_szQuery,
			HTTP_MAX_RAW_URL_BYTES, HTTP_MAX_QUERY_PARAM_COUNT,
			uiObservedRawUrlBytes, uiObservedQueryParamCount);
		if (HTTP_QUERY_PARSE_OK != enQueryResult)
		{
			const char* p_szReason = GetHttpQueryParseReason(enQueryResult);
			HTTP_WARN("ConnID=%llu,RejectQuery,Type=%d,RawUrlBytes=%zu,ParamCount=%zu",
				(unsigned long long)dwConnID, (int)enQueryResult,
				uiObservedRawUrlBytes, uiObservedQueryParamCount);
			// 请求行阶段返回 HPR_ERROR 会让底层立即断开并丢弃错误响应，因此延迟到完整解析后统一回 400。
			pReqObj->SetRequestReject(BadRequest, p_szReason);
		}

		const char* p_szUrlPath = pSender->GetUrlField(dwConnID, HUF_PATH);
		pReqObj->SetUrl(nullptr != p_szUrlPath ? p_szUrlPath : lpszUrl);

		char szAddress[100] = { 0 };
		int iAddressLen = sizeof(szAddress);
		USHORT usPort = 0;
		pSender->GetRemoteAddress(dwConnID, szAddress, iAddressLen, usPort);
		char szClientIp[STR_IP_LEN] = { 0 };
		SafeCopyCString(szClientIp, sizeof(szClientIp), szAddress);
		pReqObj->SetAddress(szClientIp, usPort);
	}
	catch (...)
	{
		delete pReqObj;
		HTTP_ERROR("ConnID=%llu,InitHttpReqAllocFail", (unsigned long long)dwConnID);
		return RejectHttpRequest(m_pRuntime, pSender, dwConnID,
			InternalServerError, "request init fail");
	}

	bool bInserted = false;
	unsigned long long ullReqID = 0;
	pthread_mutex_lock(&g_mutexHttpReq);
	if (g_mapHttpConnReq.find(dwConnID) == g_mapHttpConnReq.end()
		&& g_mapHttpConnActiveReq.find(dwConnID) == g_mapHttpConnActiveReq.end())
	{
		// wyl 2026-04-25：请求对象始终先进入全局请求表，再记录“连接 -> 正在解析中的请求”，便于后续统一清理。
		ullReqID = ++g_ullHttpAsynReqID;
		pReqObj->SetConnAsyId(ullReqID);
		g_mapHttpReq[ullReqID] = pReqObj;
		g_mapHttpConnReq[dwConnID] = ullReqID;
		bInserted = true;
	}
	pthread_mutex_unlock(&g_mutexHttpReq);

	if (!bInserted)
	{
		delete pReqObj;
		HTTP_ERROR("ConnID=%llu,RequestStateConflict", (unsigned long long)dwConnID);
		return HPR_ERROR;
	}

	HTTP_INFO("ReqID=%llu,ConnID=%llu,Method=%s,Url=%s",
		ullReqID, (unsigned long long)dwConnID,
		nullptr == lpszMethod ? "" : lpszMethod,
		nullptr == pReqObj->GetUrl() ? "" : pReqObj->GetUrl());
	return HPR_OK;
}

EnHttpParseResult CHttpServerListerNet::OnHeader(IHttpServer* pSender, CONNID dwConnID, LPCSTR lpszName, LPCSTR lpszValue)
{
	if (!g_bHttpServerStatus)
		return HPR_ERROR;

	bool bFound = false;
	bool bAdded = false;
	pthread_mutex_lock(&g_mutexHttpReq);
	unsigned long long ullReqID = 0;
	CHttpAsynReqObj* pReqObj = nullptr;
	if (FindHttpParsingReqNoLock(m_pRuntime, dwConnID,
		ullReqID, pReqObj))
	{
		bFound = true;
		bAdded = pReqObj->AddRequestHead(lpszName, lpszValue, HTTP_MAX_REQ_HEADER_COUNT, HTTP_MAX_REQ_HEADER_BYTES);
	}
	pthread_mutex_unlock(&g_mutexHttpReq);

	if (!bFound)
		return HPR_ERROR;

	if (!bAdded)
	{
		return RejectHttpRequest(m_pRuntime, pSender, dwConnID,
			RequestHeaderFieldsTooLarge, "request headers too large");
	}

	return HPR_OK;
}

EnHttpParseResult CHttpServerListerNet::OnHeadersComplete(IHttpServer* pSender, CONNID dwConnID)
{
	if (!g_bHttpServerStatus)
		return HPR_ERROR;

	if (pSender->IsUpgrade(dwConnID))
	{
		HTTP_WARN("ConnID=%llu,UpgradeRequestNotSupported", (unsigned long long)dwConnID);
		return RejectHttpRequest(m_pRuntime, pSender, dwConnID,
			BadRequest, "upgrade not support");
	}

	// wyl 2026-04-25：HeadersComplete 阶段可以拿到 Content-Length：先做大包拦截，再按长度预留缓存。
	const ULONGLONG ullContentLength = pSender->GetContentLength(dwConnID);
	// wyl 2026-04-25：若 Content-Length 已经超限，就不再继续等待 BODY 分片，尽早返回 413。
	if (ullContentLength > HTTP_MAX_REQ_BODY_BYTES)
	{
		return RejectHttpRequest(m_pRuntime, pSender, dwConnID,
			PayloadTooLarge, "request body too large");
	}

	bool bFound = false;
	bool bReserved = true;
	pthread_mutex_lock(&g_mutexHttpReq);
	unsigned long long ullReqID = 0;
	CHttpAsynReqObj* pReqObj = nullptr;
	if (FindHttpParsingReqNoLock(m_pRuntime, dwConnID,
		ullReqID, pReqObj))
	{
		pReqObj->SetKeepAlive(!!pSender->IsKeepAlive(dwConnID));
		if (ullContentLength > 0)
		{
			// wyl 2026-04-25：这里只预留 BODY 容量，不代表请求已经完整；真正数据仍由 OnBody 分片追加。
			bReserved = pReqObj->ReserveContent((size_t)ullContentLength, HTTP_MAX_REQ_BODY_BYTES);
		}
		bFound = true;
	}
	pthread_mutex_unlock(&g_mutexHttpReq);

	if (!bFound)
		return HPR_ERROR;

	if (!bReserved)
	{
		return RejectHttpRequest(m_pRuntime, pSender, dwConnID,
			InternalServerError, "request body reserve fail");
	}

	return HPR_OK;
}

EnHttpParseResult CHttpServerListerNet::OnBody(IHttpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength)
{
	if (!g_bHttpServerStatus)
		return HPR_ERROR;

	if (nullptr == pData || iLength <= 0)
		return HPR_OK;

	// wyl 2026-04-25：HP-Socket 的 HTTP BODY 可能分多次触发 OnBody() 回调。
	// wyl 2026-04-25：因此这里必须按片段累计组包，并对累计总大小做上限保护。
	// wyl 2026-04-25：OnBody 只累计分片并校验总大小，不打印请求正文；完整请求在 OnMessageComplete 统一派发给上层。
	bool bFound = false;
	bool bAppended = false;
	pthread_mutex_lock(&g_mutexHttpReq);
	unsigned long long ullReqID = 0;
	CHttpAsynReqObj* pReqObj = nullptr;
	if (FindHttpParsingReqNoLock(m_pRuntime, dwConnID,
		ullReqID, pReqObj))
	{
		bFound = true;
		bAppended = pReqObj->AppendContent(reinterpret_cast<const unsigned char*>(pData), iLength, HTTP_MAX_REQ_BODY_BYTES);
	}
	pthread_mutex_unlock(&g_mutexHttpReq);

	if (!bFound)
		return HPR_ERROR;

	if (!bAppended)
	{
		return RejectHttpRequest(m_pRuntime, pSender, dwConnID,
			PayloadTooLarge, "request body too large");
	}

	return HPR_OK;
}

EnHttpParseResult CHttpServerListerNet::OnMessageComplete(IHttpServer* pSender, CONNID dwConnID)
{
	if (!g_bHttpServerStatus)
		return HPR_ERROR;

	CHttpAsynReqObj* pReqObj = nullptr;
	unsigned long long ullReqID = 0;
	pthread_mutex_lock(&g_mutexHttpReq);
	if (g_mapHttpConnActiveReq.find(dwConnID) == g_mapHttpConnActiveReq.end()
		&& FindHttpParsingReqNoLock(m_pRuntime, dwConnID,
			ullReqID, pReqObj))
	{
		// wyl 2026-04-25：到这里请求已完整解析，状态从“解析中”切到“等待上层应答中”。
		g_mapHttpConnReq.erase(dwConnID);
		g_mapHttpConnActiveReq[dwConnID] = ullReqID;
		pReqObj->SetKeepAlive(!!pSender->IsKeepAlive(dwConnID));
		pReqObj->MarkDispatched();
	}
	pthread_mutex_unlock(&g_mutexHttpReq);

	if (nullptr == pReqObj)
		return HPR_ERROR;

	if (pReqObj->HasRequestReject())
	{
		// 网络层拒绝不进入业务线程；完成解析后同步发送错误并关闭连接，保证客户端可收到 400。
		pReqObj->SetKeepAlive(false);
		pReqObj->SetResponseStatus(pReqObj->GetRequestRejectStatus());
		pReqObj->AddResponseHead("Content-Type", "text/plain; charset=utf-8");
		pReqObj->AddResponseHead("Connection", "close");
		const char* p_szReason = pReqObj->GetRequestRejectReason();
		const bool bSent = pReqObj->SendResponse(p_szReason, (int)strlen(p_szReason));
		if (nullptr != g_CHttpSockServerObj)
		{
			g_CHttpSockServerObj->DelHttpAsynReq(ullReqID);
		}
		return bSent ? HPR_OK : HPR_ERROR;
	}

	// wyl 2026-04-25：先暂停该连接继续接收，避免 keep-alive 下第二个请求先于第一个请求完成回包而产生乱序。
	if (!PauseHttpReceiveNoThrow(pSender, dwConnID, true, "PauseActiveRequest"))
	{
		pthread_mutex_lock(&g_mutexHttpReq);
		g_mapHttpConnActiveReq.erase(dwConnID);
		std::map<unsigned long long, CHttpAsynReqObj*>::iterator itReq = g_mapHttpReq.find(ullReqID);
		if (itReq != g_mapHttpReq.end())
		{
			g_mapHttpReq.erase(itReq);
		}
		pthread_mutex_unlock(&g_mutexHttpReq);

		pReqObj->DetachTransport();
		delete pReqObj;
		HTTP_ERROR("ReqID=%llu,ConnID=%llu,PauseReceiveFail,err=%d",
			ullReqID, (unsigned long long)dwConnID, SYS_GetLastError());
		return HPR_ERROR;
	}

	// wyl 2026-04-25：到这里 HTTP 头和 BODY 都已解析完成，提交给线程池后由上层异步处理并发送响应。
	if (!SubmitHttpRequestTask(m_pRuntime, pReqObj))
	{
		pthread_mutex_lock(&g_mutexHttpReq);
		std::map<unsigned long long, CHttpAsynReqObj*>::iterator itReq = g_mapHttpReq.find(ullReqID);
		if (itReq != g_mapHttpReq.end())
		{
			g_mapHttpReq.erase(itReq);
		}
		g_mapHttpConnActiveReq.erase(dwConnID);
		pthread_mutex_unlock(&g_mutexHttpReq);

		pReqObj->AbortRequest();
		delete pReqObj;
		HTTP_ERROR("ReqID=%llu,ConnID=%llu,SubmitHttpRequestTaskFail",
			ullReqID, (unsigned long long)dwConnID);
		return HPR_ERROR;
	}

	return HPR_OK;
}

EnHttpParseResult CHttpServerListerNet::OnUpgrade(IHttpServer*, CONNID dwConnID, EnHttpUpgradeType enUpgradeType)
{
	HTTP_WARN("ConnID=%llu,UnexpectedUpgradeType=%d", (unsigned long long)dwConnID, (int)enUpgradeType);
	return HPR_ERROR;
}

EnHttpParseResult CHttpServerListerNet::OnParseError(IHttpServer*, CONNID dwConnID, int iErrorCode, LPCSTR lpszErrorDesc)
{
	HTTP_ERROR("ConnID=%llu,ParseError=%d,Desc=%s",
		(unsigned long long)dwConnID, iErrorCode, nullptr == lpszErrorDesc ? "" : lpszErrorDesc);
	CleanupHttpParsingReq(m_pRuntime, dwConnID);
	return HPR_ERROR;
}

EnHandleResult CHttpServerListerNet::OnWSMessageHeader(IHttpServer*, CONNID dwConnID, BOOL, BYTE, BYTE, const BYTE[4], ULONGLONG)
{
	HTTP_WARN("ConnID=%llu,UnexpectedWSHeader", (unsigned long long)dwConnID);
	return HR_ERROR;
}

EnHandleResult CHttpServerListerNet::OnWSMessageBody(IHttpServer*, CONNID dwConnID, const BYTE*, int)
{
	HTTP_WARN("ConnID=%llu,UnexpectedWSBody", (unsigned long long)dwConnID);
	return HR_ERROR;
}

EnHandleResult CHttpServerListerNet::OnWSMessageComplete(IHttpServer*, CONNID dwConnID)
{
	HTTP_WARN("ConnID=%llu,UnexpectedWSComplete", (unsigned long long)dwConnID);
	return HR_ERROR;
}

EnHandleResult CHttpServerListerNet::OnPrepareListen(ITcpServer*, SOCKET)
{
	if (!g_bHttpServerStatus)
		return HR_ERROR;

	return HR_OK;
}

EnHandleResult CHttpServerListerNet::OnAccept(ITcpServer*, CONNID, UINT_PTR)
{
	return g_bHttpServerStatus ? HR_OK : HR_ERROR;
}

EnHandleResult CHttpServerListerNet::OnClose(ITcpServer*, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode)
{
	HTTP_INFO("ConnID=%llu,Operation=%d,ErrorCode=%d",
		(unsigned long long)dwConnID, enOperation, iErrorCode);
	CleanupHttpParsingReq(m_pRuntime, dwConnID);
	// wyl 2026-04-25：已派发给上层但尚未释放的请求对象不能再继续持有底层 sender，避免后续误回包到失效连接。
	DetachHttpActiveReq(m_pRuntime, dwConnID);
	return HR_OK;
}

EnHandleResult CHttpServerListerNet::OnSend(ITcpServer*, CONNID, const BYTE*, int)
{
	return HR_OK;
}

EnHandleResult CHttpServerListerNet::OnReceive(ITcpServer*, CONNID dwConnID, int iLength)
{
	if (!g_bHttpServerStatus)
		return HR_ERROR;

	HTTP_WARN("ConnID=%llu,UnexpectedPullReceiveLen=%d", (unsigned long long)dwConnID, iLength);
	return HR_ERROR;
}

EnHandleResult CHttpServerListerNet::OnHandShake(ITcpServer*, CONNID)
{
	return HR_OK;
}

EnHandleResult CHttpServerListerNet::OnShutdown(ITcpServer*)
{
	HTTP_INFO("server shutdown");
	return HR_OK;
}
