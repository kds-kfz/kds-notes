#include "publicGlobalvar.h"
#include "publicfunc.h"
#include "Log.h"

namespace
{
	const size_t HTTP_MAX_REQ_HEADER_COUNT = 128;
	const size_t HTTP_MAX_REQ_HEADER_BYTES = 32 * 1024;
	const size_t HTTP_MAX_REQ_BODY_BYTES = 8 * 1024 * 1024;

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

	bool FindHttpParsingReqNoLock(CONNID dwConnID, unsigned long long& ullReqID, CHttpAsynReqObj*& pReqObj)
	{
		pReqObj = nullptr;
		ullReqID = 0;

		std::map<CONNID, unsigned long long>::iterator itConnReq = g_mapHttpConnReq.find(dwConnID);
		if (itConnReq == g_mapHttpConnReq.end())
			return false;

		ullReqID = itConnReq->second;
		std::map<unsigned long long, CHttpAsynReqObj*>::iterator itReq = g_mapHttpReq.find(ullReqID);
		if (itReq == g_mapHttpReq.end() || nullptr == itReq->second)
			return false;

		pReqObj = itReq->second;
		return true;
	}

	void CleanupHttpParsingReq(CONNID dwConnID)
	{
		pthread_mutex_lock(&g_mutexHttpReq);
		unsigned long long ullReqID = 0;
		CHttpAsynReqObj* pReqObj = nullptr;
		if (FindHttpParsingReqNoLock(dwConnID, ullReqID, pReqObj))
		{
			g_mapHttpConnReq.erase(dwConnID);

			std::map<unsigned long long, CHttpAsynReqObj*>::iterator itReq = g_mapHttpReq.find(ullReqID);
			if (itReq != g_mapHttpReq.end() && nullptr != itReq->second && !itReq->second->IsDispatched())
			{
				delete itReq->second;
				g_mapHttpReq.erase(itReq);
			}
		}
		pthread_mutex_unlock(&g_mutexHttpReq);
	}

	void DetachHttpActiveReq(CONNID dwConnID)
	{
		pthread_mutex_lock(&g_mutexHttpReq);
		std::map<CONNID, unsigned long long>::iterator itActiveReq = g_mapHttpConnActiveReq.find(dwConnID);
		if (itActiveReq != g_mapHttpConnActiveReq.end())
		{
			const unsigned long long ullReqID = itActiveReq->second;
			g_mapHttpConnActiveReq.erase(itActiveReq);

			std::map<unsigned long long, CHttpAsynReqObj*>::iterator itReq = g_mapHttpReq.find(ullReqID);
			if (itReq != g_mapHttpReq.end() && nullptr != itReq->second)
			{
				itReq->second->DetachTransport();
			}
		}
		pthread_mutex_unlock(&g_mutexHttpReq);
	}

	bool SendSimpleHttpError(IHttpServer* pSender, CONNID dwConnID, HttpStatusType enStatus, const char* p_szMsg)
	{
		if (nullptr == pSender)
			return false;

		const char* p_szBody = (nullptr == p_szMsg) ? "" : p_szMsg;
		const THeader stHeaders[] =
		{
			{ "Content-Type", "text/plain; charset=utf-8" },
			{ "Connection", "close" }
		};

		const bool bSendOk = !!pSender->SendResponse(dwConnID, (USHORT)enStatus, nullptr, stHeaders,
			sizeof(stHeaders) / sizeof(stHeaders[0]), reinterpret_cast<const BYTE*>(p_szBody), (int)strlen(p_szBody));
		if (!pSender->Release(dwConnID))
		{
			HTTP_WARN("ConnID=%llu,RejectReleaseFail,err=%d", (unsigned long long)dwConnID, SYS_GetLastError());
		}

		return bSendOk;
	}

	EnHttpParseResult RejectHttpRequest(IHttpServer* pSender, CONNID dwConnID, HttpStatusType enStatus, const char* p_szReason)
	{
		HTTP_WARN("ConnID=%llu,RejectRequest,Status=%d,Reason=%s",
			(unsigned long long)dwConnID, (int)enStatus, nullptr == p_szReason ? "" : p_szReason);

		if (!SendSimpleHttpError(pSender, dwConnID, enStatus, p_szReason))
		{
			HTTP_WARN("ConnID=%llu,RejectResponseSendFail,err=%d", (unsigned long long)dwConnID, SYS_GetLastError());
		}

		CleanupHttpParsingReq(dwConnID);
		return HPR_ERROR;
	}

	void ThreadHttpRequestTask(LPTSocketTask socketTask)
	{
		if (nullptr == socketTask || nullptr == socketTask->buf)
			return;

		CHttpAsynReqObj* pReqObj = *(CHttpAsynReqObj**)socketTask->buf;
		if (nullptr == pReqObj)
			return;

		if (nullptr != g_pHttpHandle && g_bHttpServerStatus)
		{
			g_pHttpHandle(pReqObj);
		}
	}

	bool SubmitHttpRequestTask(CHttpAsynReqObj* pReqObj)
	{
		if (nullptr == pReqObj || nullptr == g_pHttpHandle || !g_bHttpServerStatus)
			return false;

		CHttpAsynReqObj* pTaskReqObj = pReqObj;
		LPTSocketTask task = HP_Create_SocketTaskObj((Fn_SocketTaskProc)ThreadHttpRequestTask, pReqObj,
			(CONNID)pReqObj->GetConnId(), (const BYTE*)&pTaskReqObj, sizeof(pTaskReqObj));
		if (nullptr == task)
			return false;

		if (!g_CHttpHPThreadPool->Submit(task, 1000 * 5))
		{
			HP_Destroy_SocketTaskObj(task);
			return false;
		}

		return true;
	}
}

EnHttpParseResult CHttpServerListerNet::OnMessageBegin(IHttpServer*, CONNID dwConnID)
{
	if (!g_bHttpServerStatus)
		return HPR_ERROR;

	// 当前实现不支持同一连接并发处理多个 HTTP 请求；若上一个请求还没收尾，这里直接拒绝继续解析。
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
		return RejectHttpRequest(pSender, dwConnID, NotImplemented, "method not support");
	}

	CHttpAsynReqObj* pReqObj = new CHttpAsynReqObj();
	pReqObj->SetSender(pSender);
	pReqObj->SetConnId(dwConnID);
	pReqObj->SetMethod(lpszMethod);

	const char* p_szUrlPath = pSender->GetUrlField(dwConnID, HUF_PATH);
	pReqObj->SetUrl(nullptr != p_szUrlPath ? p_szUrlPath : lpszUrl);

	char szAddress[100] = { 0 };
	int iAddressLen = sizeof(szAddress);
	USHORT usPort = 0;
	pSender->GetRemoteAddress(dwConnID, szAddress, iAddressLen, usPort);
	char szClientIp[STR_IP_LEN] = { 0 };
	SafeCopyCString(szClientIp, sizeof(szClientIp), szAddress);
	pReqObj->SetAddress(szClientIp, usPort);

	bool bInserted = false;
	unsigned long long ullReqID = 0;
	pthread_mutex_lock(&g_mutexHttpReq);
	if (g_mapHttpConnReq.find(dwConnID) == g_mapHttpConnReq.end()
		&& g_mapHttpConnActiveReq.find(dwConnID) == g_mapHttpConnActiveReq.end())
	{
		// 请求对象始终先进入全局请求表，再记录“连接 -> 正在解析中的请求”，便于后续统一清理。
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
	if (FindHttpParsingReqNoLock(dwConnID, ullReqID, pReqObj))
	{
		bFound = true;
		bAdded = pReqObj->AddRequestHead(lpszName, lpszValue, HTTP_MAX_REQ_HEADER_COUNT, HTTP_MAX_REQ_HEADER_BYTES);
	}
	pthread_mutex_unlock(&g_mutexHttpReq);

	if (!bFound)
		return HPR_ERROR;

	if (!bAdded)
	{
		return RejectHttpRequest(pSender, dwConnID, RequestHeaderFieldsTooLarge, "request headers too large");
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
		return RejectHttpRequest(pSender, dwConnID, BadRequest, "upgrade not support");
	}

	bool bFound = false;
	pthread_mutex_lock(&g_mutexHttpReq);
	unsigned long long ullReqID = 0;
	CHttpAsynReqObj* pReqObj = nullptr;
	if (FindHttpParsingReqNoLock(dwConnID, ullReqID, pReqObj))
	{
		pReqObj->SetKeepAlive(!!pSender->IsKeepAlive(dwConnID));
		bFound = true;
	}
	pthread_mutex_unlock(&g_mutexHttpReq);

	if (!bFound)
		return HPR_ERROR;

	// 若 Content-Length 已经超限，就不再继续等待 BODY 分片，尽早返回 413。
	if (pSender->GetContentLength(dwConnID) > HTTP_MAX_REQ_BODY_BYTES)
	{
		return RejectHttpRequest(pSender, dwConnID, PayloadTooLarge, "request body too large");
	}

	return HPR_OK;
}

EnHttpParseResult CHttpServerListerNet::OnBody(IHttpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength)
{
	if (!g_bHttpServerStatus)
		return HPR_ERROR;

	if (nullptr == pData || iLength <= 0)
		return HPR_OK;

	// HP-Socket 的 HTTP BODY 可能分多次触发 OnBody() 回调。
	// 因此这里必须按片段累计组包，并对累计总大小做上限保护。
	bool bFound = false;
	bool bAppended = false;
	pthread_mutex_lock(&g_mutexHttpReq);
	unsigned long long ullReqID = 0;
	CHttpAsynReqObj* pReqObj = nullptr;
	if (FindHttpParsingReqNoLock(dwConnID, ullReqID, pReqObj))
	{
		bFound = true;
		bAppended = pReqObj->AppendContent(reinterpret_cast<const unsigned char*>(pData), iLength, HTTP_MAX_REQ_BODY_BYTES);
	}
	pthread_mutex_unlock(&g_mutexHttpReq);

	if (!bFound)
		return HPR_ERROR;

	if (!bAppended)
	{
		return RejectHttpRequest(pSender, dwConnID, PayloadTooLarge, "request body too large");
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
		&& FindHttpParsingReqNoLock(dwConnID, ullReqID, pReqObj))
	{
		// 到这里请求已完整解析，状态从“解析中”切到“等待上层应答中”。
		g_mapHttpConnReq.erase(dwConnID);
		g_mapHttpConnActiveReq[dwConnID] = ullReqID;
		pReqObj->SetKeepAlive(!!pSender->IsKeepAlive(dwConnID));
		pReqObj->MarkDispatched();
	}
	pthread_mutex_unlock(&g_mutexHttpReq);

	if (nullptr == pReqObj)
		return HPR_ERROR;

	// 先暂停该连接继续接收，避免 keep-alive 下第二个请求先于第一个请求完成回包而产生乱序。
	if (!pSender->PauseReceive(dwConnID, true))
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

	if (!SubmitHttpRequestTask(pReqObj))
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
	CleanupHttpParsingReq(dwConnID);
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
	CleanupHttpParsingReq(dwConnID);
	// 已派发给上层但尚未释放的请求对象不能再继续持有底层 sender，避免后续误回包到失效连接。
	DetachHttpActiveReq(dwConnID);
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
