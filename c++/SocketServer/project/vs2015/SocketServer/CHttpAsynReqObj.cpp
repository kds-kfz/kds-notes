#include "CHttpAsynReqObj.h"

#include <string.h>

#include "HPSocket.h"
#include "Log.h"
#include "publicGlobalvar.h"

namespace
{
	void CopyIpValue(char* p_szDst, int p_iDstLen, const char* p_szSrc)
	{
		if (nullptr == p_szDst || p_iDstLen <= 0)
			return;

		p_szDst[0] = '\0';
		if (nullptr == p_szSrc)
			return;

		_snprintf(p_szDst, p_iDstLen, "%s", p_szSrc);
		p_szDst[p_iDstLen - 1] = '\0';
	}

	char ToLowerAscii(char ch)
	{
		return (ch >= 'A' && ch <= 'Z') ? (ch - 'A' + 'a') : ch;
	}

	size_t SafeStringLength(const char* p_szValue)
	{
		return (nullptr == p_szValue) ? 0 : strlen(p_szValue);
	}

	void ClearHttpActiveReq(CONNID dwConnID, unsigned long long ullReqID)
	{
		pthread_mutex_lock(&g_mutexHttpReq);
		std::map<CONNID, unsigned long long>::iterator itActive = g_mapHttpConnActiveReq.find(dwConnID);
		if (itActive != g_mapHttpConnActiveReq.end() && itActive->second == ullReqID)
		{
			g_mapHttpConnActiveReq.erase(itActive);
		}
		pthread_mutex_unlock(&g_mutexHttpReq);
	}
}

CHttpAsynReqObj::CHttpAsynReqObj()
	: m_pSender(nullptr), m_dwConnID(0), m_ullReqID(0), m_unClientPort(0),
	m_enHttpStatus(OK), m_bDispatched(false), m_bKeepAlive(false), m_bResponseSent(false),
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
	CopyIpValue(p_szClientIp, p_iIpLen, m_szClientIp);
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

	HttpHeaderItem stHeaderItem;
	stHeaderItem.strName = p_szName;
	if (nullptr != p_szValue)
	{
		stHeaderItem.strValue = p_szValue;
	}
	m_vecResponseHead.push_back(stHeaderItem);
}

bool CHttpAsynReqObj::SendResponse(const void* p_szData, int p_iLen)
{
	if (nullptr == m_pSender || 0 == m_dwConnID)
	{
		HTTP_WARN("ReqID=%llu,ConnID=%llu,SendResponseWithoutTransport",
			m_ullReqID, (unsigned long long)m_dwConnID);
		return false;
	}

	if (m_bResponseSent)
	{
		HTTP_WARN("ReqID=%llu,ConnID=%llu,DuplicateSendResponse",
			m_ullReqID, (unsigned long long)m_dwConnID);
		return false;
	}

	const BYTE* pBody = (nullptr != p_szData && p_iLen > 0) ? (const BYTE*)p_szData : nullptr;
	const int iBodyLen = p_iLen > 0 ? p_iLen : 0;

	std::vector<THeader> vecHeaders;
	vecHeaders.reserve(m_vecResponseHead.size());
	for (size_t i = 0; i < m_vecResponseHead.size(); ++i)
	{
		THeader stHeader;
		stHeader.name = m_vecResponseHead[i].strName.c_str();
		stHeader.value = m_vecResponseHead[i].strValue.c_str();
		vecHeaders.push_back(stHeader);
	}

	if (!m_pSender->SendResponse(m_dwConnID, (USHORT)m_enHttpStatus, nullptr,
		vecHeaders.empty() ? nullptr : &vecHeaders[0], (int)vecHeaders.size(), pBody, iBodyLen))
	{
		HTTP_ERROR("ReqID=%llu,ConnID=%llu,SendResponseFail,err=%d",
			m_ullReqID, (unsigned long long)m_dwConnID, SYS_GetLastError());
		return false;
	}

	m_bResponseSent = true;
	// 只有当本次响应已经被底层网络层接受后，才清理当前连接的活动请求限制。
	ClearHttpActiveReq(m_dwConnID, m_ullReqID);

	if (m_bKeepAlive)
	{
		// 请求派发给上层后该连接的接收已被暂停；只有响应成功进入发送队列后才恢复读取。
		if (!m_pSender->PauseReceive(m_dwConnID, FALSE))
		{
			HTTP_WARN("ReqID=%llu,ConnID=%llu,ResumeReceiveFail,err=%d",
				m_ullReqID, (unsigned long long)m_dwConnID, SYS_GetLastError());
		}
	}
	else
	{
		if (!m_pSender->Release(m_dwConnID))
		{
			HTTP_WARN("ReqID=%llu,ConnID=%llu,ReleaseConnFail,err=%d",
				m_ullReqID, (unsigned long long)m_dwConnID, SYS_GetLastError());
		}
	}

	HTTP_INFO("ReqID=%llu,ConnID=%llu,Status=%d,BodyLen=%d,KeepAlive=%d",
		m_ullReqID, (unsigned long long)m_dwConnID, (int)m_enHttpStatus, iBodyLen, m_bKeepAlive);
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

void CHttpAsynReqObj::SetAddress(const char* p_szClientIp, unsigned short p_unClientPort)
{
	CopyIpValue(m_szClientIp, sizeof(m_szClientIp), p_szClientIp);
	m_unClientPort = p_unClientPort;
}

bool CHttpAsynReqObj::AddRequestHead(const char* p_szName, const char* p_szValue, size_t p_uiMaxHeadCount, size_t p_uiMaxHeadBytes)
{
	if (nullptr == p_szName || '\0' == *p_szName)
		return true;

	// 这里按“头名 + ': ' + 头值 + CRLF”的近似格式估算单个请求头占用字节数，用于做总量限制。
	const size_t uiOneHeadBytes = SafeStringLength(p_szName) + SafeStringLength(p_szValue) + 4;
	if ((m_uiRequestHeadCount + 1) > p_uiMaxHeadCount || (m_uiRequestHeadBytes + uiOneHeadBytes) > p_uiMaxHeadBytes)
	{
		return false;
	}

	++m_uiRequestHeadCount;
	m_uiRequestHeadBytes += uiOneHeadBytes;
	m_mapRequestHead[NormalizeHeaderName(p_szName)] = (nullptr == p_szValue) ? "" : p_szValue;
	return true;
}

bool CHttpAsynReqObj::AppendContent(const BYTE* p_pData, int p_iLen, size_t p_uiMaxBodyBytes)
{
	if (nullptr == p_pData || p_iLen <= 0)
		return true;

	if ((m_strContent.size() + (size_t)p_iLen) > p_uiMaxBodyBytes)
	{
		return false;
	}

	m_strContent.append((const char*)p_pData, p_iLen);
	return true;
}

void CHttpAsynReqObj::AbortRequest()
{
	ClearHttpActiveReq(m_dwConnID, m_ullReqID);

	if (nullptr != m_pSender && 0 != m_dwConnID)
	{
		// 如果上层直接放弃请求且没有回包，最稳妥的兜底方式就是主动关闭当前连接。
		if (!m_pSender->Release(m_dwConnID))
		{
			HTTP_WARN("ReqID=%llu,ConnID=%llu,AbortReleaseConnFail,err=%d",
				m_ullReqID, (unsigned long long)m_dwConnID, SYS_GetLastError());
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
