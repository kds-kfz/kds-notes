#include "WebSockMng.h"
#include <string>

#include "Log.h"
#include "UserMng.h"
#include "CodeMsg.h"
#include "NetQueueMng.h"
#include "publicfunc.h"

bool g_bQuitAll = false;
int g_nClientNum = 0;
long g_lMaxInQueue = 0;
long g_lMaxOutQueue = 0;
int g_nOneMaxQueue = 50;
bool g_bStressTest = false;

void ProcessWebData(void* p_refServerHandle, void* p_refClinetHandle, const void* p_szData, int p_iDataLen, const char* p_szClientIp, unsigned short p_nClientPort)
{
	short nErrCode = UNKNOW_ABNORM;
	unsigned long long ulUnDoReqNum = 0;

	HS pServerHandle = (HS)p_refServerHandle;
	HCLIENT pClinetHandle = (HCLIENT)p_refClinetHandle;

	USERDATA* puserdata = CUserMng::GetInstance()->Query(pServerHandle, pClinetHandle);
	AutoReleaseFunc	autoFunc(puserdata, UserManageFunc);
	if (!puserdata)
	{
		const char* p_szErrData = CCodeMsg::GetInstance()->GetCodeMsg(USER_NOT_EXIST).c_str();
		CWebSockMng::GetInstance()->WebSockClose(p_refServerHandle, p_refClinetHandle, p_szErrData, strlen(p_szErrData));

		MT_WARN("[用户模块] %d:%s,ip=%s,port=%hu", USER_NOT_EXIST,
			CCodeMsg::GetInstance()->GetCodeMsg(USER_NOT_EXIST).c_str(), p_szClientIp, p_nClientPort);
		return;
	}

	//TODO 回调函数
	puserdata->pHostProcess = CServerProc::GetInstance();

	if (p_iDataLen < WEB_RECVBUF_LEN)
	{
		//缓存包体
		//memcpy(puserdata->buf, p_szData, p_iDataLen);
		//puserdata->recvlen = p_iDataLen;

		AutoReleaseFunc	packFunc((void*)p_szData, ReleasePackDataFunc);

		nsdk_AtomicInc64(&puserdata->llNRequest);

		// 如果单个人队列超过了上线，关闭连接
		if (g_lMaxInQueue < puserdata->llNRequest)
			g_lMaxInQueue = puserdata->llNRequest;
		if (puserdata->llNRequest > g_nOneMaxQueue && !g_bStressTest)
		{
			const char* p_szErrData = CCodeMsg::GetInstance()->GetCodeMsg(USER_REQUEST_OEVERFLOW).c_str();
			CWebSockMng::GetInstance()->WebSockClose(p_refServerHandle, p_refClinetHandle, (const char*)p_szErrData, strlen((char*)p_szErrData));

			ulUnDoReqNum = CNetQueueMng::GetInstance()->DelUserReq(pServerHandle, pClinetHandle, p_szClientIp, p_nClientPort);
			CUserMng::GetInstance()->Del(pServerHandle, pClinetHandle);

			MT_WARN("[用户模块] %d:%s,ip=%s,port=%hu,请求=%lld,未处理=%llu,最大=%d",
				USER_REQUEST_OEVERFLOW, CCodeMsg::GetInstance()->GetCodeMsg(USER_REQUEST_OEVERFLOW).c_str(),
				p_szClientIp, p_nClientPort, puserdata->llNRequest, ulUnDoReqNum, g_nOneMaxQueue);
			return;
		}

		//printf("上层应用收到: 读数据通知, ip=%s,port=%d,data=%s,datalen=%d\n", p_szClientIp, p_nClientPort, p_szData, p_iDataLen);
		MT_INFO("[WEB服务] 读数据通知 [%p,%llu,%p],ip=%s,port=%d,datalen=%d",
			p_refServerHandle, *(unsigned __int64*)p_refClinetHandle, p_refClinetHandle,
			p_szClientIp, p_nClientPort, p_iDataLen);

		//处理请求
		int iRet = CNetQueueMng::GetInstance()->ProcessIt(WEBSOCK_SOURCE_TYPE, p_szClientIp, p_nClientPort, puserdata, (char*)p_szData, p_iDataLen);
		if (0 > iRet)//-1 协议解析错误 
		{
			//TODO 错误包返回
			string strMsg = "protocol parsing error";
			CWebSockMng::GetInstance()->WebSockSend(p_refServerHandle, p_refClinetHandle, strMsg.c_str(), strMsg.size());
			return;
		}

		if (g_bQuitAll)//退出了
		{
			const char* p_szErrData = CCodeMsg::GetInstance()->GetCodeMsg(FRAME_SERVER_STOP).c_str();
			CWebSockMng::GetInstance()->WebSockClose(p_refServerHandle, p_refClinetHandle, p_szErrData, strlen(p_szErrData));

			ulUnDoReqNum = CNetQueueMng::GetInstance()->DelUserReq(pServerHandle, pClinetHandle, p_szClientIp, p_nClientPort);
			CUserMng::GetInstance()->Del(pServerHandle, pClinetHandle);
			MT_WARN("[框架模块] %d:%s,ip=%s,port=%hu,未处理=%llu",
				FRAME_SERVER_STOP, CCodeMsg::GetInstance()->GetCodeMsg(FRAME_SERVER_STOP).c_str(),
				p_szClientIp, p_nClientPort, ulUnDoReqNum);
			return;
		}
	}
	else//缓冲区太满,直接删
	{
		const char* p_szErrData = CCodeMsg::GetInstance()->GetCodeMsg(USER_RECV_OEVERFLOW).c_str();
		CWebSockMng::GetInstance()->WebSockClose(p_refServerHandle, p_refClinetHandle, p_szErrData, strlen(p_szErrData));

		ulUnDoReqNum = CNetQueueMng::GetInstance()->DelUserReq(pServerHandle, pClinetHandle, p_szClientIp, p_nClientPort);
		CUserMng::GetInstance()->Del(pServerHandle, pClinetHandle);

		MT_WARN("[用户模块] %d:%s,ip=%s,port=%hu,未处理=%llu,包大小=%d,缓存预设值=%d,最大值=%d",
			USER_RECV_OEVERFLOW, CCodeMsg::GetInstance()->GetCodeMsg(USER_RECV_OEVERFLOW).c_str(),
			p_szClientIp, p_nClientPort, ulUnDoReqNum, p_iDataLen, puserdata->iRecvLen, MAX_CACHE_BUFLEN);

		return;
	}
}

static void WebSockMngNotifyHandle(void *p_refServerHandle, void *p_refClinetHandle, WebSockNotifyType p_enType,
	const void *p_szData, int p_iDataLen, const char *p_szClientIp, unsigned short p_nClientPort, void *p_szErrData)
{
	short nErrCode = UNKNOW_ABNORM;
	unsigned long long ulUnDoReqNum = 0;

	HS pServerHandle = (HS)p_refServerHandle;
	HCLIENT pClinetHandle = (HCLIENT)p_refClinetHandle;

	switch (p_enType)
	{
	case enWebClose:
	{
		//不需要再关闭客户端，内部已关闭
		printf("上层应用收到: 关闭通知, ip=%s,port=%d, err=%s\n", p_szClientIp, p_nClientPort, (char*)p_szErrData);

		CUserMng::GetInstance()->Del(pServerHandle, pClinetHandle);
		g_nClientNum--;

		MT_INFO("[WEB服务] 用户断开WebSock连接[%p,%lu],ip=%s,port=%d, 信息=%s",
			p_refServerHandle, (unsigned long)p_refClinetHandle, p_szClientIp, p_nClientPort, (char*)p_szErrData);
	}
	break;
	case enWebConnect:
	{
		printf("上层应用收到: 连接通知, ip=%s,port=%d\n", p_szClientIp, p_nClientPort);

		USERDATA ud;
		ud.pServerHandle = pServerHandle;
		ud.pClinetHandle = pClinetHandle;
		ud.nNetSourceType = WEBSOCK_SOURCE_TYPE;	// 一定要标注数据来源，方便应答处理
		ud.llLastAcitve = time(0);
		//ud.pHostProcess = CServerProc::GetInstance();	// 处理分支
		// 把申请缓冲区，放到Manager内部 
		ud.Init();		// 申请缓冲区
		if (CUserMng::GetInstance()->Add(&ud))	// me
		{
			USERDATA* puserdata = CUserMng::GetInstance()->Query(pServerHandle, pClinetHandle);
			AutoReleaseFunc	autoFunc(puserdata, UserManageFunc);
			if (puserdata)
			{
				g_nClientNum++;
				puserdata->iClientSockId = (int)ud.pClinetHandle;		// 记录下信息
				MT_INFO("[WEB服务] 用户WebSock连接成功[%p,%lu,%p]", pServerHandle, (unsigned long)pClinetHandle, pClinetHandle);
			}
		}
		else
		{
			ud.ResetIt();
			nErrCode = USER_COUNT_TOPLIMIT;

			MT_WARN("[WEB服务] %d:%s,ip=%s,port=%hu,最大用户数=%d,已有用户数=%d",
				USER_COUNT_TOPLIMIT, CCodeMsg::GetInstance()->GetCodeMsg(USER_COUNT_TOPLIMIT).c_str(),
				p_szClientIp, p_nClientPort,
				CUserMng::GetInstance()->m_iMaxUserCount,
				CUserMng::GetInstance()->m_iUserCount);
		}
	}
	break;
	case enWebError:
	{
		printf("上层应用收到: 错误通知, ip=%s,port=%d,err=%s\n", p_szClientIp, p_nClientPort, (char*)p_szErrData);

		CWebSockMng::GetInstance()->WebSockClose(p_refServerHandle, p_refClinetHandle, (const char*)p_szErrData, strlen((char*)p_szErrData));
		//删除请求
		//ulUnDoReqNum = CNetQueueMng::GetInstance()->DelUserReq((HS)p_refServerHandle, (HCLIENT)p_refClinetHandle, p_szClientIp, p_nClientPort);

		CUserMng::GetInstance()->Del(pServerHandle, pClinetHandle);
		g_nClientNum--;

		MT_WARN("[WEB服务] 错误通知 %d:%s,ip=%s,port=%hu,errdata=%s,未处理=%llu",
			USER_NET_BREAK, CCodeMsg::GetInstance()->GetCodeMsg(USER_NET_BREAK).c_str(), p_szClientIp, p_nClientPort, p_szErrData, ulUnDoReqNum);

	}
	break;
	case enWebData:
	{
		ProcessWebData(pServerHandle, pClinetHandle, p_szData, p_iDataLen, p_szClientIp, p_nClientPort);
	}
	break;
	default:
		MT_WARN("[WEB服务] 未知通知类型=%d", p_enType);
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
		MT_WARN("[WEB服务] 路径是空");
		return false;
	}

	std::string strDllPath = p_sHomePath;
	strDllPath.append("\\");
	strDllPath.append(HTTP_DLL_NAME);

	m_pclLibraryOp = new CLibraryOp;
	if (!m_pclLibraryOp)
	{
		MT_WARN("[WEB服务] 装载三方库类失败...");
		return false;
	}

	const auto fileAttributes = ::GetFileAttributes(strDllPath.c_str());
	if (INVALID_FILE_ATTRIBUTES == fileAttributes || 0 != (fileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		delete m_pclLibraryOp;
		m_pclLibraryOp = nullptr;
		MT_WARN("[WEB服务] 动态库文件不存在[%s]...", strDllPath.c_str());
		return false;
	}

	if (!m_pclLibraryOp->Load(strDllPath.c_str()))
	{
		delete m_pclLibraryOp;
		m_pclLibraryOp = nullptr;
		MT_WARN("[WEB服务] 动态库加载失败[%s]...", strDllPath.c_str());
		return false;
	}

	pfnCreateWebSockInstance fnCreateWebSockInstance = nullptr;
	if (!m_pclLibraryOp->GetFuncAddress((void**)&fnCreateWebSockInstance, "CreateWebSockInstance") || nullptr == fnCreateWebSockInstance)
	{
		delete m_pclLibraryOp;
		m_pclLibraryOp = nullptr;
		MT_WARN("[WEB服务] 获取方法[WebSockIns]失败...");
		return false;
	}

	if (nullptr == m_pWebServerHandle)
	{
		m_pWebServerHandle = fnCreateWebSockInstance();
		if (nullptr == m_pWebServerHandle)
		{
			delete m_pclLibraryOp;
			m_pclLibraryOp = nullptr;
			MT_WARN("[WEB服务] 获取监控方法失败");
			return false;
		}
	}

	if (!m_pclLibraryOp->GetFuncAddress((void**)&m_fnDelWebSockInstance, "DelWebSockInstance") || nullptr == m_fnDelWebSockInstance)
	{
		delete m_pclLibraryOp;
		m_pclLibraryOp = nullptr;
		MT_WARN("[WEB服务] 获取方法[DelWebSockIns]失败...");
		return false;
	}

	m_bStatus = true;
	MT_INFO("[WEB服务] 初始化状态: %d", m_bStatus);
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
		MT_WARN("[WEB服务] 创建web服务失败: 服务句柄是空");
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

	MT_INFO("[WEB服务] 服务IP: %s", p_szIp);
	MT_INFO("[WEB服务] 服务端口: %d", p_nPort);
	MT_INFO("[WEB服务] 线程数: %d", p_iThreadNum);
	MT_INFO("[WEB服务] 队列数: %d", p_iQueueNum);
	MT_INFO("[WEB服务] 缓存大小: %d", p_iRBufLen);
	MT_INFO("[WEB服务] 最大Accept: %d", p_iMaxConnectNum);
	MT_INFO("[WEB服务] 最大Connect: %d", p_iMaxAcceptNum);
	MT_INFO("[WEB服务] 底层日志路径:%s", pSafeLogFold);
	MT_INFO("[WEB服务] 是否开启wss:%d", p_bSSL);
	if (p_bSSL)
	{
		MT_INFO("[WEB服务] 证书文件路径:%s", p_szPemCertFile);
		MT_INFO("[WEB服务] 私钥文件路径:%s", p_szPemKeyFile);
		MT_INFO("[WEB服务] 私钥密码:%s", p_szKeyPassword);
		MT_INFO("[WEB服务] CA证书文件路径:%s", p_szCAPemCertFileOrPath);

		if (!m_pWebServerHandle->CreateWssSock(p_szIp, p_nPort, p_iRBufLen, p_iMaxConnectNum, p_iMaxAcceptNum, WebSockMngNotifyHandle, p_iThreadNum, p_iQueueNum, szBuf,
			p_szPemCertFile, p_szPemKeyFile, p_szKeyPassword, p_szCAPemCertFileOrPath, pLogFold))
		{
			m_bStatus = false;
			MT_WARN("[WEB服务] 创建wss服务失败: %s", szBuf);
		}
	}
	else
	{
		if (!m_pWebServerHandle->CreateWebSock(p_szIp, p_nPort, p_iRBufLen, p_iMaxConnectNum, p_iMaxAcceptNum, WebSockMngNotifyHandle, p_iThreadNum, p_iQueueNum, szBuf, pLogFold))
		{
			m_bStatus = false;
			MT_WARN("[WEB服务] 创建web服务失败: %s", szBuf);
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
		MT_WARN("[WEB服务] 发送应答失败, 参数为空");
		return 0;
	}

	if (nullptr == pNode->pServerHandle || nullptr == pNode->pClinetHandle)
	{
		MT_WARN("[WEB服务] 发送应答, 用户已不存在,功能号=%d,ip=%s,port=%d",
			pNode->nGNID, pNode->szIp, pNode->usPort);
		return 0;
	}

	MT_INFO("[WEB服务] 应答 cookie=%lu,mainid=%ld,[%p,%p],reqid=%d,lsize=%ld,len=%ld",
		pNode->cookie, pNode->MainID, pNode->pServerHandle, pNode->pClinetHandle, pNode->nGNID, pNode->lSize, pNode->len);

	CWebSockMng::GetInstance()->WebSockSend(pNode->pServerHandle, pNode->pClinetHandle, pTransfer, static_cast<int>(pNode->len));
	return 0;
}