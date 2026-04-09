#include "WebSockMng.h"
#include <string>

#include "Log.h"

static void WebSockMngNotifyHandle(void *p_refServerHandle, void *p_refClinetHandle, WebSockNotifyType p_enType,
	const void *p_szData, int p_iDataLen, const char *p_szClientIp, unsigned short p_nClientPort, void *p_szErrData)
{
	switch (p_enType)
	{
	case enWebClose:
		WEB_INFO("[WEB服务] 连接关闭[%p,%p],ip=%s,port=%hu,err=%s",
			p_refServerHandle, p_refClinetHandle, p_szClientIp, p_nClientPort,
			(p_szErrData != nullptr) ? static_cast<const char*>(p_szErrData) : "");
		break;
	case enWebConnect:
		WEB_INFO("[WEB服务] 连接建立[%p,%p],ip=%s,port=%hu",
			p_refServerHandle, p_refClinetHandle, p_szClientIp, p_nClientPort);
		break;
	case enWebError:
		WEB_WARN("[WEB服务] 连接错误[%p,%p],ip=%s,port=%hu,err=%s",
			p_refServerHandle, p_refClinetHandle, p_szClientIp, p_nClientPort,
			(p_szErrData != nullptr) ? static_cast<const char*>(p_szErrData) : "");
		break;
	case enWebData:
		WEB_INFO("[WEB服务] 收到数据[%p,%p],ip=%s,port=%hu,len=%d",
			p_refServerHandle, p_refClinetHandle, p_szClientIp, p_nClientPort, p_iDataLen);
		break;
	default:
		WEB_WARN("[WEB服务] 未知通知类型=%d", p_enType);
		break;
	}
}

CWebSockMng* CWebSockMng::m_pThis = nullptr;
CSocketServer *CWebSockMng::m_pWebServerHandle = nullptr;

void CWebSockMng::WebSockSend(void* p_refServerHandle, void *p_refClinetHandle, const char* p_szData, int p_iDataLen)
{
	if (nullptr == m_pWebServerHandle)
		return;

	m_pWebServerHandle->WebSockSend(p_refServerHandle, p_refClinetHandle, p_szData, p_iDataLen);
}

void CWebSockMng::WebSockClose(void* p_refServerHandle, void *p_refClinetHandle, const char* p_szData, int p_iDataLen)
{
	if (nullptr == m_pWebServerHandle)
		return;

	m_pWebServerHandle->WebSockClose(p_refServerHandle, p_refClinetHandle, p_szData, p_iDataLen);
}

bool CWebSockMng::InitWebServerInfo(const char* p_sHomePath)
{
	if (nullptr == p_sHomePath || strlen(p_sHomePath) == 0)
	{
		WEB_WARN("[WEB服务] 路径是空");
		return false;
	}

	std::string strDllPath = p_sHomePath;
	strDllPath.append("\\");
	strDllPath.append(HTTP_DLL_NAME);

	m_pclLibraryOp = new CLibraryOp;
	if (!m_pclLibraryOp)
	{
		WEB_WARN("[WEB服务] 装载三方库类失败...");
		return false;
	}

	const auto fileAttributes = ::GetFileAttributes(strDllPath.c_str());
	if (INVALID_FILE_ATTRIBUTES == fileAttributes || 0 != (fileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		delete m_pclLibraryOp;
		m_pclLibraryOp = nullptr;
		WEB_WARN("[WEB服务] 动态库文件不存在[%s]...", strDllPath.c_str());
		return false;
	}

	if (!m_pclLibraryOp->Load(strDllPath.c_str()))
	{
		delete m_pclLibraryOp;
		m_pclLibraryOp = nullptr;
		WEB_WARN("[WEB服务] 动态库加载失败[%s]...", strDllPath.c_str());
		return false;
	}

	pfnCreateWebSockInstance fnCreateWebSockInstance = nullptr;
	if (!m_pclLibraryOp->GetFuncAddress((void**)&fnCreateWebSockInstance, "CreateWebSockInstance") || nullptr == fnCreateWebSockInstance)
	{
		delete m_pclLibraryOp;
		m_pclLibraryOp = nullptr;
		WEB_WARN("[WEB服务] 获取方法[WebSockIns]失败...");
		return false;
	}

	if (nullptr == m_pWebServerHandle)
	{
		m_pWebServerHandle = fnCreateWebSockInstance();
		if (nullptr == m_pWebServerHandle)
		{
			delete m_pclLibraryOp;
			m_pclLibraryOp = nullptr;
			WEB_WARN("[WEB服务] 获取监控方法失败");
			return false;
		}
	}

	if (!m_pclLibraryOp->GetFuncAddress((void**)&m_fnDelWebSockInstance, "DelWebSockInstance") || nullptr == m_fnDelWebSockInstance)
	{
		delete m_pclLibraryOp;
		m_pclLibraryOp = nullptr;
		WEB_WARN("[WEB服务] 获取方法[DelWebSockIns]失败...");
		return false;
	}

	m_bStatus = true;
	WEB_INFO("[WEB服务] 初始化状态: %d", m_bStatus);
	return m_bStatus;
}

CWebSockMng::CWebSockMng()
{
	m_bStatus = false;
	m_fnDelWebSockInstance = nullptr;
	m_pclLibraryOp = nullptr;
}

CWebSockMng::~CWebSockMng()
{
	if (nullptr != m_fnDelWebSockInstance && nullptr != m_pWebServerHandle)
	{
		m_fnDelWebSockInstance(m_pWebServerHandle);
		m_fnDelWebSockInstance = nullptr;
		m_pWebServerHandle = nullptr;
	}
	if (nullptr != m_pclLibraryOp)
	{
		delete m_pclLibraryOp;
		m_pclLibraryOp = nullptr;
	}
}

CWebSockMng* CWebSockMng::GetInstance()
{
	if (!m_pThis)
	{
		m_pThis = new CWebSockMng;
	}
	return m_pThis;
}

void CWebSockMng::Release()
{
	if (m_pThis)
	{
		delete m_pThis;
		m_pThis = nullptr;
	}
}

bool CWebSockMng::Start(const char *p_szIp, unsigned short p_nPort, int p_iThreadNum, int p_iQueueNum, int p_iRBufLen, int p_iMaxConnectNum, int p_iMaxAcceptNum,
	bool p_bSSL, const char *p_szPemCertFile, const char *p_szPemKeyFile,
	const char *p_szKeyPassword, const char *p_szCAPemCertFileOrPath, char *p_szLogFold)
{
	if (nullptr == m_pWebServerHandle)
	{
		WEB_WARN("[WEB服务] 创建web服务失败: 服务句柄是空");
		return false;
	}

	p_iThreadNum = p_iThreadNum <= 0 ? WEB_THREAD_NUM : p_iThreadNum;
	p_iQueueNum = p_iQueueNum <= 0 ? WEB_QUEUE_NUM : p_iQueueNum;
	p_iRBufLen = p_iRBufLen <= 0 ? WEB_RECVBUF_LEN : p_iRBufLen;
	p_iMaxConnectNum = p_iMaxConnectNum <= 0 ? WEB_CONNECT_NUM : p_iMaxConnectNum;
	p_iMaxAcceptNum = p_iMaxAcceptNum <= 0 ? WEB_ACCEPT_NUM : p_iMaxAcceptNum;

	char szBuf[1024] = { 0 };
	char *pLogFold = (nullptr == p_szLogFold || '\0' == *p_szLogFold) ? nullptr : p_szLogFold;
	const char* pSafeLogFold = (nullptr == pLogFold) ? "" : pLogFold;

	WEB_INFO("[WEB服务] 服务IP: %s", p_szIp);
	WEB_INFO("[WEB服务] 服务端口: %d", p_nPort);
	WEB_INFO("[WEB服务] 线程数: %d", p_iThreadNum);
	WEB_INFO("[WEB服务] 队列数: %d", p_iQueueNum);
	WEB_INFO("[WEB服务] 缓存大小: %d", p_iRBufLen);
	WEB_INFO("[WEB服务] 最大Accept: %d", p_iMaxConnectNum);
	WEB_INFO("[WEB服务] 最大Connect: %d", p_iMaxAcceptNum);
	WEB_INFO("[WEB服务] 底层日志路径:%s", pSafeLogFold);
	WEB_INFO("[WEB服务] 是否开启wss:%d", p_bSSL);
	if (p_bSSL)
	{
		WEB_INFO("[WEB服务] 证书文件路径:%s", p_szPemCertFile);
		WEB_INFO("[WEB服务] 私钥文件路径:%s", p_szPemKeyFile);
		WEB_INFO("[WEB服务] 私钥密码:%s", p_szKeyPassword);
		WEB_INFO("[WEB服务] CA证书文件路径:%s", p_szCAPemCertFileOrPath);

		if (!m_pWebServerHandle->CreateWssSock(p_szIp, p_nPort, p_iRBufLen, p_iMaxConnectNum, p_iMaxAcceptNum, WebSockMngNotifyHandle, p_iThreadNum, p_iQueueNum, szBuf,
			p_szPemCertFile, p_szPemKeyFile, p_szKeyPassword, p_szCAPemCertFileOrPath, pLogFold))
		{
			m_bStatus = false;
			WEB_WARN("[WEB服务] 创建wss服务失败: %s", szBuf);
		}
	}
	else
	{
		if (!m_pWebServerHandle->CreateWebSock(p_szIp, p_nPort, p_iRBufLen, p_iMaxConnectNum, p_iMaxAcceptNum, WebSockMngNotifyHandle, p_iThreadNum, p_iQueueNum, szBuf, pLogFold))
		{
			m_bStatus = false;
			WEB_WARN("[WEB服务] 创建web服务失败: %s", szBuf);
		}
	}
	return m_bStatus;
}

void CWebSockMng::Stop()
{
	if (nullptr == m_pWebServerHandle)
		return;

	m_pWebServerHandle->StopWebSock();
}

std::uint32_t CWebSockMng::ProcessAsynAns(NetRequsetDat * pNode, const char * pTransfer)
{
	if (nullptr == pNode || nullptr == pTransfer)
	{
		WEB_WARN("[WEB服务] 发送应答失败, 参数为空");
		return 0;
	}

	if (nullptr == pNode->pServerHandle || nullptr == pNode->pClinetHandle)
	{
		WEB_WARN("[WEB服务] 发送应答, 用户已不存在,功能号=%d,ip=%s,port=%d",
			pNode->nGNID, pNode->szIp, pNode->usPort);
		return 0;
	}

	WEB_INFO("[WEB服务] 应答 cookie=%lu,mainid=%ld,[%p,%p],reqid=%d,lsize=%ld,len=%ld",
		pNode->cookie, pNode->MainID, pNode->pServerHandle, pNode->pClinetHandle, pNode->nGNID, pNode->lSize, pNode->len);
	CWebSockMng::GetInstance()->WebSockSend(pNode->pServerHandle, pNode->pClinetHandle, pTransfer, static_cast<int>(pNode->len));
	return 0;
}