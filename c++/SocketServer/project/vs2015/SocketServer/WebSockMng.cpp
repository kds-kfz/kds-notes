#include "StdAfx.h"
#include "WebSockProtobufMng.h"
#include "NetQueueMng.h"
#include "protobuf/ProtoPackHandle.h"

CWebSockProtobufMng* CWebSockProtobufMng::m_pThis = nullptr;
CSocketServer *CWebSockProtobufMng::m_pWebServerHandle = nullptr;

void CWebSockProtobufMng::WebSockSend(void* p_refServerHandle, void *p_refClinetHandle, const char* p_szData, int p_iDataLen)
{
	if (nullptr == m_pWebServerHandle)
		return;

	m_pWebServerHandle->WebSockSend(p_refServerHandle, p_refClinetHandle, p_szData, p_iDataLen);
}

void CWebSockProtobufMng::WebSockClose(void* p_refServerHandle, void *p_refClinetHandle, const char* p_szData, int p_iDataLen)
{
	if (nullptr == m_pWebServerHandle)
		return;

	m_pWebServerHandle->WebSockClose(p_refServerHandle, p_refClinetHandle, p_szData, p_iDataLen);
}

bool CWebSockProtobufMng::InitWebServerInfo(const char* p_sHomePath)
{
	if (nullptr == p_sHomePath || strlen(p_sHomePath) == 0)
	{
		WARN("[WEB服务] 路径是空");
		return false;
	}
	string strDllPath = p_sHomePath;
	strDllPath.append("\\");
	strDllPath.append(HTTP_DLL_NAME);

	//TODO 读取websocket服务开关

	//TODO 读取websocket服务日志路径

	//if (!m_isMonitor)
	//	return false;

	m_pclLibraryOp = new CLibraryOp;
	if (!m_pclLibraryOp)
	{
		WARN("[WEB服务] 装载三方库类失败...");
		return false;
	}

	//加载监控动态库
	const auto fileAttributes = ::GetFileAttributes(strDllPath.c_str());
	if (INVALID_FILE_ATTRIBUTES == fileAttributes || 0 != (fileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		delete m_pclLibraryOp;
		WARN("[WEB服务] 动态库文件不存在[%s]...", strDllPath.c_str());
		return false;
	}

	//加载动态库
	if (!m_pclLibraryOp->Load(strDllPath.c_str()))
	{
		delete m_pclLibraryOp;
		WARN("[WEB服务] 动态库加载失败[%s]...", strDllPath.c_str());
		return false;
	}

	// 创建函数
	pfnCreateWebSockInstance fnCreateWebSockInstance = nullptr;

	if (!m_pclLibraryOp->GetFuncAddress((void**)&fnCreateWebSockInstance, "CreateWebSockInstance") || nullptr == fnCreateWebSockInstance)
	{
		delete m_pclLibraryOp;
		WARN("[WEB服务] 获取方法[WebSockIns]失败...");
		return false;
	}

	if (nullptr == m_pWebServerHandle)
	{
		m_pWebServerHandle = fnCreateWebSockInstance();
		if (nullptr == m_pWebServerHandle)
		{
			delete m_pclLibraryOp;
			WARN("[WEB服务] 获取监控方法失败");
			return false;
		}
	}

	if (!m_pclLibraryOp->GetFuncAddress((void**)&m_fnDelWebSockInstance, "DelWebSockInstance") || nullptr == m_fnDelWebSockInstance)
	{
		delete m_pclLibraryOp;
		WARN("[WEB服务] 获取方法[DelWebSockIns]失败...");
		return false;
	}

	//TODO 初始化日志
	m_bStatus = true;

	INFO("[WEB服务] 初始化状态: %d", m_bStatus);

	return m_bStatus;
}

CWebSockProtobufMng::CWebSockProtobufMng()
{
	m_bStatus = false;
	m_fnDelWebSockInstance = nullptr;
	m_pclLibraryOp = nullptr;
}


CWebSockProtobufMng::~CWebSockProtobufMng()
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

CWebSockProtobufMng* CWebSockProtobufMng::GetInstance()
{
	if (!m_pThis)
	{
		m_pThis = new CWebSockProtobufMng;
	}
	return m_pThis;
}

void CWebSockProtobufMng::Release()
{
	if (m_pThis)
	{
		delete m_pThis;
		m_pThis = nullptr;
	}
}

bool CWebSockProtobufMng::Start(const char *p_szIp, unsigned short p_nPort, int p_iThreadNum, int p_iQueueNum, int p_iRBufLen, int p_iMaxConnectNum, int p_iMaxAcceptNum,
	bool p_bSSL, const char *p_szPemCertFile, const char *p_szPemKeyFile,
	const char *p_szKeyPassword, const char *p_szCAPemCertFileOrPath,char *p_szLogFold)
{
	if (nullptr == m_pWebServerHandle)
	{
		WARN("[WEB服务] 创建web服务失败: 服务句柄是空");
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

	INFO("[WEB服务] 服务IP: %s", p_szIp);
	INFO("[WEB服务] 服务端口: %d", p_nPort);
	INFO("[WEB服务] 线程数: %d", p_iThreadNum);
	INFO("[WEB服务] 队列数: %d", p_iQueueNum);
	INFO("[WEB服务] 缓存大小: %d", p_iRBufLen);
	INFO("[WEB服务] 最大Accept: %d", p_iMaxConnectNum);
	INFO("[WEB服务] 最大Connect: %d", p_iMaxAcceptNum);
	INFO("[WEB服务] 底层日志路径:%s", pSafeLogFold);
	INFO("[WEB服务] 是否开启wss:%d", p_bSSL);
	if (p_bSSL)
	{
		INFO("[WEB服务] 证书文件路径:%s", p_szPemCertFile);
		INFO("[WEB服务] 私钥文件路径:%s", p_szPemKeyFile);
		INFO("[WEB服务] 私钥密码:%s", p_szKeyPassword);
		INFO("[WEB服务] CA证书文件路径:%s", p_szCAPemCertFileOrPath);

		//启动服务
		if (!m_pWebServerHandle->CreateWssSock(p_szIp, p_nPort, p_iRBufLen, p_iMaxConnectNum, p_iMaxAcceptNum, WebNotifyHandle, p_iThreadNum, p_iQueueNum, szBuf, 
			p_szPemCertFile, p_szPemKeyFile, p_szKeyPassword, p_szCAPemCertFileOrPath, pLogFold))
		{
			m_bStatus = false;
			WARN("[WEB服务] 创建wss服务失败: %s", szBuf);
		}
	}
	else
	{
		//启动服务
		if (!m_pWebServerHandle->CreateWebSock(p_szIp, p_nPort, p_iRBufLen, p_iMaxConnectNum, p_iMaxAcceptNum, WebNotifyHandle, p_iThreadNum, p_iQueueNum, szBuf, pLogFold))
		{
			m_bStatus = false;
			WARN("[WEB服务] 创建web服务失败: %s", szBuf);
		}
	}
	return m_bStatus;
}

void CWebSockProtobufMng::Stop()
{
	if (nullptr == m_pWebServerHandle)
		return;

	m_pWebServerHandle->StopWebSock();
}

std::uint32_t CWebSockProtobufMng::ProcessAsynAns(NetRequsetDat * pNode, const char * pTransfer)
{
	if (nullptr == pNode->h || nullptr == pNode->user)
	{
		WARN("[WEB服务] 发送应答, 用户已不存在,功能号=%d,ip=%s,port=%d", pNode->nGNID, pNode->ip, pNode->port);
		return 0;
	}

	const char * packdata = pTransfer + sizeof(ANSHEADER);

	INFO("[WEB服务] 应答 cookie=%lu,mainid=%ld,[%p,%llu],reqid=%d,lsize=%d,len=%d",
		pNode->cookie, pNode->MainID, pNode->h, *(unsigned __int64 *)pNode->user, pNode->nGNID, pNode->lSize, pNode->len);

	CWebSockProtobufMng::GetInstance()->WebSockSend(pNode->h, pNode->user, packdata, pNode->len);

	return 0;
}



