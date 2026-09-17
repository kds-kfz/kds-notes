#include "HttpAsynReqObj.h"

#include <string.h>
#include <new>
#include <windows.h>

#include "HPSocket.h"
#include "Log.h"
#include "publicfunc.h"
#include "publicGlobalvar.h"
#include "ServerRuntimeContext.h"

namespace
{
	int HexDigitValue(char p_chValue)
	{
		if (p_chValue >= '0' && p_chValue <= '9')
		{
			return p_chValue - '0';
		}
		if (p_chValue >= 'A' && p_chValue <= 'F')
		{
			return p_chValue - 'A' + 10;
		}
		if (p_chValue >= 'a' && p_chValue <= 'f')
		{
			return p_chValue - 'a' + 10;
		}
		return -1;
	}

	char ToLowerAscii(char ch)
	{
		return (ch >= 'A' && ch <= 'Z') ? (ch - 'A' + 'a') : ch;
	}

	size_t SafeStringLength(const char* p_szValue)
	{
		return (nullptr == p_szValue) ? 0 : strlen(p_szValue);
	}

	void ClearHttpActiveReq(ST_HTTP_SERVER_RUNTIME* p_pRuntime,
		CONNID dwConnID, unsigned long long ullReqID)
	{
		if (p_pRuntime == nullptr)
		{
			return;
		}
		pthread_mutex_lock(&p_pRuntime->mutexRequest);
		std::map<CONNID, unsigned long long>::iterator itActive =
			p_pRuntime->mapActiveRequest.find(dwConnID);
		if (itActive != p_pRuntime->mapActiveRequest.end() &&
			itActive->second == ullReqID)
		{
			p_pRuntime->mapActiveRequest.erase(itActive);
		}
		pthread_mutex_unlock(&p_pRuntime->mutexRequest);
	}

	bool HttpSocketIsConnectedNoThrow(IHttpServer* pSender, CONNID dwConnID, unsigned long long ullReqID)
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
			HTTP_ERROR("ReqID=%llu,ConnID=%llu,HttpIsAliveException=0x%08X",
				ullReqID, (unsigned long long)dwConnID, dwExceptionCode);
			return false;
		}
		return bAlive ? true : false;
	}

	bool IsHttpTransportSendable(ST_HTTP_SERVER_RUNTIME* p_pRuntime,
		IHttpServer* pSender, CONNID dwConnID, unsigned long long ullReqID)
	{
		// wyl 2026-05-19：异步回包前同时校验全局服务状态和 HP-Socket 连接状态，避免回包到已关闭连接。
		return p_pRuntime != nullptr && pSender != nullptr
			&& p_pRuntime->bServerStatus.load()
			&& pSender == p_pRuntime->pPackServer
			&& HttpSocketIsConnectedNoThrow(pSender, dwConnID, ullReqID);
	}

	bool SendHttpResponseNoThrow(IHttpServer* pSender, CONNID dwConnID, unsigned long long ullReqID, HttpStatusType enStatus,
		const THeader* pHeaders, int iHeaderCount, const BYTE* pBody, int iBodyLen)
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
			// wyl 2026-05-19：HTTP 回包底层异常不能让进程无日志退出，至少记录请求号、连接号和异常码。
			HTTP_ERROR("ReqID=%llu,ConnID=%llu,SendResponseException=0x%08X,Status=%d,BodyLen=%d",
				ullReqID, (unsigned long long)dwConnID, dwExceptionCode, (int)enStatus, iBodyLen);
			return false;
		}
		return bOK ? true : false;
	}

	bool PauseHttpReceiveNoThrow(IHttpServer* pSender, CONNID dwConnID, unsigned long long ullReqID, BOOL bPause)
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
			HTTP_ERROR("ReqID=%llu,ConnID=%llu,PauseReceiveException=0x%08X,Pause=%d",
				ullReqID, (unsigned long long)dwConnID, dwExceptionCode, (int)bPause);
			return false;
		}
		return bOK ? true : false;
	}

	bool ReleaseHttpConnNoThrow(IHttpServer* pSender, CONNID dwConnID, unsigned long long ullReqID, const char* p_szAction)
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
			HTTP_ERROR("ReqID=%llu,ConnID=%llu,%sException=0x%08X",
				ullReqID, (unsigned long long)dwConnID, nullptr != p_szAction ? p_szAction : "ReleaseConn", dwExceptionCode);
			return false;
		}
		return bOK ? true : false;
	}
}

CHttpAsynReqObj::CHttpAsynReqObj(ST_HTTP_SERVER_RUNTIME* p_pRuntime)
	: m_pRuntime(p_pRuntime), m_pSender(nullptr), m_dwConnID(0), m_ullReqID(0), m_unClientPort(0),
	m_enHttpStatus(OK), m_bDispatched(false), m_bKeepAlive(false), m_bResponseSent(false),
	m_bRequestRejected(false), m_enRequestRejectStatus(BadRequest),
	m_uiRequestHeadCount(0), m_uiRequestHeadBytes(0)
{
	memset(m_szClientIp, 0, sizeof(m_szClientIp));
}

CHttpAsynReqObj::~CHttpAsynReqObj()
{
}

const char* CHttpAsynReqObj::GetUrl()
{
	return m_strUrl.empty() ? nullptr : m_strUrl.c_str();
}

const char* CHttpAsynReqObj::GetRawUrl()
{
	return m_strRawUrl.empty() ? nullptr : m_strRawUrl.c_str();
}

const char* CHttpAsynReqObj::GetQueryString()
{
	return m_strQueryString.empty() ? nullptr : m_strQueryString.c_str();
}

const char* CHttpAsynReqObj::GetParam(const char* p_szName)
{
	if (nullptr == p_szName || '\0' == *p_szName)
		return nullptr;

	for (std::vector<ST_HTTP_QUERY_PARAM>::const_iterator itParam = m_vecQueryParam.begin();
		itParam != m_vecQueryParam.end(); ++itParam)
	{
		if (itParam->strName == p_szName)
		{
			return itParam->strValue.c_str();
		}
	}
	return nullptr;
}

const char* CHttpAsynReqObj::GetMethodType()
{
	return m_strMethod.empty() ? nullptr : m_strMethod.c_str();
}

const int CHttpAsynReqObj::GetContentLen()
{
	return (const int)m_strContent.size();
}

const void* CHttpAsynReqObj::GetContent()
{
	return m_strContent.empty() ? nullptr : m_strContent.c_str();
}

const char* CHttpAsynReqObj::GetHead(const char* p_szName)
{
	if (nullptr == p_szName)
		return nullptr;

	const std::string strHeaderName = NormalizeHeaderName(p_szName);
	std::map<std::string, std::string>::const_iterator itHead = m_mapRequestHead.find(strHeaderName);
	return itHead == m_mapRequestHead.end() ? nullptr : itHead->second.c_str();
}

void CHttpAsynReqObj::GetAddress(char* p_szClientIp, int p_iIpLen, unsigned short& p_nClientPort)
{
	SafeCopyCString(p_szClientIp, p_iIpLen, m_szClientIp);
	p_nClientPort = m_unClientPort;
}

unsigned long long CHttpAsynReqObj::GetConnId()
{
	return (unsigned long long)m_dwConnID;
}

unsigned long long CHttpAsynReqObj::GetConnAsyId()
{
	return m_ullReqID;
}

void CHttpAsynReqObj::SetResponseStatus(HttpStatusType p_enStatus)
{
	m_enHttpStatus = p_enStatus;
}

void CHttpAsynReqObj::AddResponseHead(const char* p_szName, const char* p_szValue)
{
	if (nullptr == p_szName || '\0' == *p_szName)
		return;

	// wyl 2026-04-25：响应头会写入 std::vector/std::string，可能触发内存分配；这里兜住异常，避免回包流程崩溃。
	try
	{
		HttpHeaderItem stHeaderItem;
		stHeaderItem.strName = p_szName;
		if (nullptr != p_szValue)
		{
			stHeaderItem.strValue = p_szValue;
		}
		m_vecResponseHead.push_back(stHeaderItem);
	}
	catch (...)
	{
		HTTP_ERROR("ReqID=%llu,ConnID=%llu,AddResponseHeadAllocFail", m_ullReqID, (unsigned long long)m_dwConnID);
	}
}

bool CHttpAsynReqObj::SendResponse(const void* p_szData, int p_iLen)
{
	IHttpServer* pSender = m_pSender;
	const CONNID dwConnID = m_dwConnID;
	const unsigned long long ullReqID = m_ullReqID;

	if (nullptr == pSender || 0 == dwConnID)
	{
		HTTP_WARN("ReqID=%llu,ConnID=%llu,SendResponseWithoutTransport",
			ullReqID, (unsigned long long)dwConnID);
		return false;
	}

	if (m_bResponseSent)
	{
		HTTP_WARN("ReqID=%llu,ConnID=%llu,DuplicateSendResponse",
			ullReqID, (unsigned long long)dwConnID);
		return false;
	}

	const int iBodyLen = p_iLen > 0 ? p_iLen : 0;

	// wyl 2026-04-25：发送前把内部响应头转换成 HP-Socket 的 THeader 数组；日志只记录响应长度，不打印响应正文。
	std::vector<THeader> vecHeaders;
	try
	{
		vecHeaders.reserve(m_vecResponseHead.size());
		for (size_t i = 0; i < m_vecResponseHead.size(); ++i)
		{
			THeader stHeader;
			stHeader.name = m_vecResponseHead[i].strName.c_str();
			stHeader.value = m_vecResponseHead[i].strValue.c_str();
			vecHeaders.push_back(stHeader);
		}
	}
	catch (...)
	{
		HTTP_ERROR("ReqID=%llu,ConnID=%llu,BuildResponseHeaderAllocFail", ullReqID, (unsigned long long)dwConnID);
		return false;
	}

	if (!IsHttpTransportSendable(m_pRuntime, pSender, dwConnID, ullReqID))
	{
		HTTP_WARN("ReqID=%llu,ConnID=%llu,SkipSendResponseClosed,Status=%d,BodyLen=%d",
			ullReqID, (unsigned long long)dwConnID, (int)m_enHttpStatus, iBodyLen);
		ClearHttpActiveReq(m_pRuntime, dwConnID, ullReqID);
		DetachTransport();
		return false;
	}

	if (!SendHttpResponseNoThrow(pSender, dwConnID, ullReqID, m_enHttpStatus,
		vecHeaders.empty() ? nullptr : &vecHeaders[0], (int)vecHeaders.size(),
		(nullptr != p_szData && iBodyLen > 0) ? reinterpret_cast<const BYTE*>(p_szData) : nullptr, iBodyLen))
	{
		HTTP_ERROR("ReqID=%llu,ConnID=%llu,SendResponseFail,err=%d",
			ullReqID, (unsigned long long)dwConnID, SYS_GetLastError());
		ClearHttpActiveReq(m_pRuntime, dwConnID, ullReqID);
		DetachTransport();
		return false;
	}

	m_bResponseSent = true;
	// wyl 2026-04-25：只有当本次响应已经被底层网络层接受后，才清理当前连接的活动请求限制。
	ClearHttpActiveReq(m_pRuntime, dwConnID, ullReqID);

	if (m_bKeepAlive)
	{
		// wyl 2026-04-25：请求派发给上层后该连接的接收已被暂停；只有响应成功进入发送队列后才恢复读取。
		if (!PauseHttpReceiveNoThrow(pSender, dwConnID, ullReqID, false))
		{
			HTTP_WARN("ReqID=%llu,ConnID=%llu,ResumeReceiveFail,err=%d",
				ullReqID, (unsigned long long)dwConnID, SYS_GetLastError());
		}
	}
	else
	{
		if (!ReleaseHttpConnNoThrow(pSender, dwConnID, ullReqID, "ReleaseConn"))
		{
			HTTP_WARN("ReqID=%llu,ConnID=%llu,ReleaseConnFail,err=%d",
				ullReqID, (unsigned long long)dwConnID, SYS_GetLastError());
		}
	}

	HTTP_INFO("ReqID=%llu,ConnID=%llu,Status=%d,BodyLen=%d,KeepAlive=%d",
		ullReqID, (unsigned long long)dwConnID, (int)m_enHttpStatus, iBodyLen, m_bKeepAlive);
	return true;
}

void CHttpAsynReqObj::SetSender(IHttpServer* p_pSender)
{
	m_pSender = p_pSender;
}

void CHttpAsynReqObj::SetConnId(CONNID p_dwConnID)
{
	m_dwConnID = p_dwConnID;
}

void CHttpAsynReqObj::SetConnAsyId(unsigned long long p_ullReqID)
{
	m_ullReqID = p_ullReqID;
}

void CHttpAsynReqObj::SetKeepAlive(bool p_bKeepAlive)
{
	m_bKeepAlive = p_bKeepAlive;
}

void CHttpAsynReqObj::SetMethod(const char* p_szMethod)
{
	m_strMethod = (nullptr == p_szMethod) ? "" : p_szMethod;
}

void CHttpAsynReqObj::SetUrl(const char* p_szUrl)
{
	m_strUrl = (nullptr == p_szUrl) ? "" : p_szUrl;
}

EN_HTTP_QUERY_PARSE_RESULT CHttpAsynReqObj::SetRequestUrl(const char* p_szRawUrl, const char* p_szQuery,
	size_t p_uiMaxRawUrlBytes, size_t p_uiMaxQueryParamCount,
	size_t& p_refUiObservedRawUrlBytes, size_t& p_refUiObservedQueryParamCount)
{
	p_refUiObservedRawUrlBytes = 0;
	p_refUiObservedQueryParamCount = 0;
	m_strRawUrl.clear();
	m_strQueryString.clear();
	m_vecQueryParam.clear();

	if (nullptr == p_szRawUrl)
	{
		return HTTP_QUERY_PARSE_OK;
	}

	// 有界计算请求目标长度，异常长 URL 不再触发无上限扫描和分配。
	p_refUiObservedRawUrlBytes = strnlen_s(p_szRawUrl, p_uiMaxRawUrlBytes + 1);
	if (p_refUiObservedRawUrlBytes > p_uiMaxRawUrlBytes)
	{
		return HTTP_QUERY_PARSE_RAW_URL_TOO_LONG;
	}
	m_strRawUrl.assign(p_szRawUrl, p_refUiObservedRawUrlBytes);

	if (nullptr != p_szQuery)
	{
		const size_t uiQueryLength = strnlen_s(p_szQuery, p_uiMaxRawUrlBytes + 1);
		if (uiQueryLength > p_uiMaxRawUrlBytes)
		{
			return HTTP_QUERY_PARSE_RAW_URL_TOO_LONG;
		}
		m_strQueryString.assign(p_szQuery, uiQueryLength);
	}
	else
	{
		// 仅在底层没有提供 HUF_QUERY 时回退到 raw URL，正常 GET 请求不走重复定位。
		const size_t uiQueryPos = m_strRawUrl.find('?');
		if (uiQueryPos != std::string::npos)
		{
			const size_t uiQueryBegin = uiQueryPos + 1;
			const size_t uiFragmentPos = m_strRawUrl.find('#', uiQueryBegin);
			const size_t uiQueryEnd = uiFragmentPos == std::string::npos ? m_strRawUrl.size() : uiFragmentPos;
			m_strQueryString.assign(m_strRawUrl.data() + uiQueryBegin, uiQueryEnd - uiQueryBegin);
		}
	}

	return ParseQueryString(p_uiMaxQueryParamCount, p_refUiObservedQueryParamCount);
}

void CHttpAsynReqObj::SetAddress(const char* p_szClientIp, unsigned short p_unClientPort)
{
	SafeCopyCString(m_szClientIp, sizeof(m_szClientIp), p_szClientIp);
	m_unClientPort = p_unClientPort;
}

bool CHttpAsynReqObj::AddRequestHead(const char* p_szName, const char* p_szValue, size_t p_uiMaxHeadCount, size_t p_uiMaxHeadBytes)
{
	if (nullptr == p_szName || '\0' == *p_szName)
		return true;

	// wyl 2026-04-25：这里按“头名 + ': ' + 头值 + CRLF”的近似格式估算单个请求头占用字节数，用于做总量限制。
	const size_t uiOneHeadBytes = SafeStringLength(p_szName) + SafeStringLength(p_szValue) + 4;
	if ((m_uiRequestHeadCount + 1) > p_uiMaxHeadCount || (m_uiRequestHeadBytes + uiOneHeadBytes) > p_uiMaxHeadBytes)
	{
		return false;
	}

	try
	{
		const std::string strHeaderName = NormalizeHeaderName(p_szName);
		const std::string strHeaderValue = (nullptr == p_szValue) ? "" : p_szValue;
		m_mapRequestHead[strHeaderName] = strHeaderValue;
	}
	catch (...)
	{
		HTTP_ERROR("ReqID=%llu,ConnID=%llu,AddRequestHeadAllocFail", m_ullReqID, (unsigned long long)m_dwConnID);
		return false;
	}

	++m_uiRequestHeadCount;
	m_uiRequestHeadBytes += uiOneHeadBytes;
	return true;
}

// wyl 2026-04-25：按 Content-Length 提前预留请求 BODY 缓冲。
// wyl 2026-04-25：作用：大包分片到达时减少 std::string 反复扩容和拷贝；失败时由 HTTP 层拒绝请求。
bool CHttpAsynReqObj::ReserveContent(size_t p_uiContentLen, size_t p_uiMaxBodyBytes)
{
	if (p_uiContentLen > p_uiMaxBodyBytes)
	{
		return false;
	}

	// wyl 2026-04-25：reserve 只改变容量，不改变已接收长度；真正内容仍由 AppendContent 逐片追加。
	try
	{
		if (p_uiContentLen > m_strContent.capacity())
		{
			m_strContent.reserve(p_uiContentLen);
		}
	}
	catch (...)
	{
		HTTP_ERROR("ReqID=%llu,ConnID=%llu,ReserveContentAllocFail,len=%zu", m_ullReqID, (unsigned long long)m_dwConnID, p_uiContentLen);
		return false;
	}
	return true;
}

// wyl 2026-04-25：追加一次 OnBody 回调带来的 BODY 分片。
// wyl 2026-04-25：作用：网络层负责把分片累计成完整 HTTP BODY，上层拿到请求时看到的是完整包。
bool CHttpAsynReqObj::AppendContent(const unsigned char* p_pData, int p_iLen, size_t p_uiMaxBodyBytes)
{
	if (nullptr == p_pData || p_iLen <= 0)
		return true;

	if ((m_strContent.size() + (size_t)p_iLen) > p_uiMaxBodyBytes)
	{
		return false;
	}

	try
	{
		// wyl 2026-04-25：只缓存正文内容，不在网络库日志中打印请求 BODY，避免大包日志和敏感信息泄露。
		m_strContent.append(reinterpret_cast<const char*>(p_pData), p_iLen);
	}
	catch (...)
	{
		HTTP_ERROR("ReqID=%llu,ConnID=%llu,AppendContentAllocFail,len=%d,total=%zu", m_ullReqID, (unsigned long long)m_dwConnID, p_iLen, m_strContent.size());
		return false;
	}
	return true;
}

void CHttpAsynReqObj::AbortRequest()
{
	ClearHttpActiveReq(m_pRuntime, m_dwConnID, m_ullReqID);

	IHttpServer* pSender = m_pSender;
	const CONNID dwConnID = m_dwConnID;
	const unsigned long long ullReqID = m_ullReqID;
	if (nullptr != pSender && 0 != dwConnID)
	{
		// wyl 2026-04-25：如果上层直接放弃请求且没有回包，最稳妥的兜底方式就是主动关闭当前连接。
		if (!ReleaseHttpConnNoThrow(pSender, dwConnID, ullReqID, "AbortReleaseConn"))
		{
			HTTP_WARN("ReqID=%llu,ConnID=%llu,AbortReleaseConnFail,err=%d",
				ullReqID, (unsigned long long)dwConnID, SYS_GetLastError());
		}
	}

	DetachTransport();
}

void CHttpAsynReqObj::DetachTransport()
{
	m_pSender = nullptr;
}

void CHttpAsynReqObj::MarkDispatched()
{
	m_bDispatched = true;
}

bool CHttpAsynReqObj::IsDispatched() const
{
	return m_bDispatched;
}

bool CHttpAsynReqObj::IsKeepAlive() const
{
	return m_bKeepAlive;
}

bool CHttpAsynReqObj::HasTransport() const
{
	return (nullptr != m_pSender && 0 != m_dwConnID);
}

bool CHttpAsynReqObj::HasSentResponse() const
{
	return m_bResponseSent;
}

std::string CHttpAsynReqObj::NormalizeHeaderName(const char* p_szName)
{
	std::string strHeaderName;
	if (nullptr == p_szName)
		return strHeaderName;

	strHeaderName.reserve(strlen(p_szName));
	while ('\0' != *p_szName)
	{
		strHeaderName.push_back(ToLowerAscii(*p_szName));
		++p_szName;
	}
	return strHeaderName;
}

void CHttpAsynReqObj::SetRequestReject(HttpStatusType p_enStatus, const char* p_szReason)
{
	m_bRequestRejected = true;
	m_enRequestRejectStatus = p_enStatus;
	m_strRequestRejectReason = nullptr == p_szReason ? "request rejected" : p_szReason;
}

bool CHttpAsynReqObj::HasRequestReject() const
{
	return m_bRequestRejected;
}

HttpStatusType CHttpAsynReqObj::GetRequestRejectStatus() const
{
	return m_enRequestRejectStatus;
}

const char* CHttpAsynReqObj::GetRequestRejectReason() const
{
	return m_strRequestRejectReason.empty() ? "request rejected" : m_strRequestRejectReason.c_str();
}

EN_HTTP_QUERY_PARSE_RESULT CHttpAsynReqObj::ParseQueryString(size_t p_uiMaxQueryParamCount,
	size_t& p_refUiObservedQueryParamCount)
{
	m_vecQueryParam.clear();
	p_refUiObservedQueryParamCount = 0;
	if (m_strQueryString.empty())
	{
		return HTTP_QUERY_PARSE_OK;
	}

	// 常用接口只有少量参数，先预留八项可避免正常请求扩容，又不放大每个请求的固定内存。
	m_vecQueryParam.reserve(8);
	ST_HTTP_QUERY_PARAM stParam;
	bool bReadingValue = false;
	bool bSegmentHasText = false;
	for (size_t uiIndex = 0; uiIndex <= m_strQueryString.size(); ++uiIndex)
	{
		const bool bAtEnd = uiIndex == m_strQueryString.size();
		const char chRaw = bAtEnd ? '&' : m_strQueryString[uiIndex];
		if (chRaw == '&')
		{
			if (bSegmentHasText)
			{
				++p_refUiObservedQueryParamCount;
				if (p_refUiObservedQueryParamCount > p_uiMaxQueryParamCount)
				{
					return HTTP_QUERY_PARSE_PARAM_TOO_MANY;
				}
				if (!stParam.strName.empty())
				{
					m_vecQueryParam.push_back(std::move(stParam));
				}
			}
			stParam = ST_HTTP_QUERY_PARAM();
			bReadingValue = false;
			bSegmentHasText = false;
			continue;
		}
		bSegmentHasText = true;
		if (chRaw == '=' && !bReadingValue)
		{
			bReadingValue = true;
			continue;
		}

		char chDecoded = chRaw == '+' ? ' ' : chRaw;
		if (chRaw == '%' && uiIndex + 2 < m_strQueryString.size())
		{
			const int iHigh = HexDigitValue(m_strQueryString[uiIndex + 1]);
			const int iLow = HexDigitValue(m_strQueryString[uiIndex + 2]);
			if (iHigh >= 0 && iLow >= 0)
			{
				chDecoded = static_cast<char>((iHigh << 4) | iLow);
				uiIndex += 2;
			}
		}
		if ('\0' == chDecoded)
		{
			return HTTP_QUERY_PARSE_DECODED_NUL;
		}
		std::string& refStrTarget = bReadingValue ? stParam.strValue : stParam.strName;
		refStrTarget.push_back(chDecoded);
	}
	return HTTP_QUERY_PARSE_OK;
}
