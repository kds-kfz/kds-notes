#include "TcpSockMng.h"
#include "Log.h"
#include "CodeMsg.h"
#include "publicfunc.h"

CTcpSockMng* CTcpSockMng::m_pThis = nullptr;
CSocketServer *CTcpSockMng::m_pTcpServerHandle = nullptr;

#ifdef OS_IS_WINDOWS
bool bNoneedAuth = (0 == nsdk::GetFileAttr("test_auth.dat", F_OK));
#else
bool bNoneedAuth = (0 == nsdk::GetFileAttr("./test_auth.dat", F_OK));
#endif

int ProcessIt(const char * ip, unsigned short port, USERDATA * puserdata,const char * packdata,int realdatalen)
{
	return 0;
}

/*
char *	GetPacketData(const ST_REQ_HEADER *  pi,int * datalen)
{
	if(pi->uiPacketLen<0  ) //  缓冲区不够 || pi->PacketLen>MAX_CACHE_BUFLEN-sizeof(CommPackInfo))
		return nullptr;
	/////////////////////
	char * packdata = (char*)(pi+1);
	if(pi->Info.ucCompressed==1)//zip压缩
	{
		if(pi->uiRawLen<=0  )//防恶意客户,压缩倍数不能超20倍
		{
			return nullptr;
		}
		unsigned long dlen= 1.2 * max(pi->uiRawLen,pi->uiPacketLen)+1024;
		char * tmpbuf=new char[dlen];
		int		r;
		// 消耗CPU的操作 ： memset(tmpbuf,0,dlen);
		r = uncompress(reinterpret_cast<Bytef*>(tmpbuf), &dlen, reinterpret_cast<const Bytef*>(packdata), pi->uiPacketLen);
		if(r==Z_OK)//解压成功
		{
			*datalen=dlen;
			return tmpbuf;
		}
		else
		{
			TCP_WARN("解压错误:rawlen=%d,PacketLen=%d",pi->uiRawLen,pi->uiPacketLen);
			nsdk_del_arry(tmpbuf);
		}
	}
	else if ( pi->uiPacketLen > 0 )
	{
		*datalen=pi->uiPacketLen;
		char *tmpbuf=new char[pi->uiPacketLen];
		memcpy(tmpbuf,packdata,pi->uiPacketLen);
		return tmpbuf;
	}
	*datalen = 0;
	return nullptr;
}
*/

void ProcessTcpData(void *p_refServerHandle, void *p_refClinetHandle, const void *p_szData, unsigned int p_uiDataLen, const char *p_szClientIp, unsigned short p_nClientPort)
{
	unsigned long long ulUnDoReqNum = 0;

	HS pServerHandle = (HS)p_refServerHandle;
	HCLIENT pClinetHandle = (HCLIENT)p_refClinetHandle;

	//TODO 实现获取客户端请求，并异步处理请求
}

//服务回调 注意收发内容要是utf-8编码;
void TcpNotifyHandle(void *p_refServerHandle, void *p_refClinetHandle, TcpSockNotifyType p_enType,
	const void *p_szData, unsigned int p_uiDataLen, const char *p_szClientIp, unsigned short p_nClientPort, void *p_szErrData)
{
	unsigned long long ulUnDoReqNum = 0;

	HS pServerHandle = (HS)p_refServerHandle;
	HCLIENT pClinetHandle = (HCLIENT)p_refClinetHandle;


	switch (p_enType)
	{
	case enTcpClose:
		{
			//不需要再关闭客户端，内部已关闭;
			printf("上层应用收到: 关闭通知, ip=%s,port=%d, err=%s\n", p_szClientIp, p_nClientPort, (char *)p_szErrData);

			TCP_INFO("[TCP服务] 用户断开TcpSock连接[%p,%lu],ip=%s,port=%d, 信息=%s",
				p_refServerHandle, (unsigned long)p_refClinetHandle, p_szClientIp, p_nClientPort, (char *)p_szErrData);
			//CHqMonitor::GetInstance()->AddClientData(p_szClientIp, p_nClientPort, g_UserManage.GetCount(), POINT_USER_DEL, __FILE__, __LINE__, DISCONNET_SERVER);
		}
		break;
	case enTcpConnect:
		{
			printf("上层应用收到: 连接通知, ip=%s,port=%d\n", p_szClientIp, p_nClientPort);
		}
		break;
	case enTcpError:
		{
			//连接没关闭
			printf("上层应用收到: 错误通知, ip=%s,port=%d,err=%s\n", p_szClientIp, p_nClientPort, (char *)p_szErrData);

			CTcpSockMng::GetInstance()->TcpSockClose(p_refServerHandle, p_refClinetHandle, (const char *)p_szErrData, strlen((char *)p_szErrData));
			
			TCP_WARN("[TCP服务] 错误通知 %d:%s,ip=%s,port=%hu,errdata=%s,未处理=%llu",
				USER_NET_BREAK, CCodeMsg::GetInstance()->GetCodeMsg(USER_NET_BREAK).c_str(), p_szClientIp, p_nClientPort, p_szErrData, ulUnDoReqNum);
		}
		break;
	case enTcpData:
		{
			//数据读取;
			printf("上层应用收到: 数据读取, ip=%s,port=%d,len=%d,msg=%s\n",
				p_szClientIp, p_nClientPort, p_uiDataLen, (char *)p_szData);
			ProcessTcpData(pServerHandle, pClinetHandle, p_szData, p_uiDataLen, p_szClientIp, p_nClientPort);
		}
		break;
	default:
		break;
	}
}

CTcpSockMng::CTcpSockMng()
{
	m_bStatus = false;
	m_fnDelTcpSockInstance = nullptr;
	m_pclLibraryOp = nullptr;
}

CTcpSockMng::~CTcpSockMng()
{
	if (nullptr != m_fnDelTcpSockInstance && nullptr != m_pTcpServerHandle)
	{
		m_fnDelTcpSockInstance(m_pTcpServerHandle);
		m_fnDelTcpSockInstance = nullptr;
		m_pTcpServerHandle = nullptr;
	}

	nsdk_del(m_pclLibraryOp);
}

CTcpSockMng* CTcpSockMng::GetInstance()
{
	if (!m_pThis)
	{
		m_pThis = new CTcpSockMng;
	}
	return m_pThis;
}

void CTcpSockMng::Release()
{
	nsdk_del(m_pThis);
}

bool CTcpSockMng::Start(const char *p_szIp, unsigned short p_nPort, int p_iThreadNum, int p_iQueueNum, int p_iRBufLen, int p_iMaxConnectNum, int p_iMaxAcceptNum,
	char *p_szLogFold)
{
	if (nullptr == m_pTcpServerHandle)
	{
		TCP_WARN("[TCP服务] 创建TCP服务失败: 服务句柄是空");
		return false;
	}

	p_iThreadNum = p_iThreadNum <= 0 ? TCP_THREAD_NUM : p_iThreadNum;
	p_iQueueNum = p_iQueueNum <= 0 ? TCP_QUEUE_NUM : p_iQueueNum;
	p_iRBufLen = p_iRBufLen <= 0 ? TCP_RECVBUF_LEN : p_iRBufLen;
	p_iMaxConnectNum = p_iMaxConnectNum <= 0 ? TCP_CONNECT_NUM : p_iMaxConnectNum;
	p_iMaxAcceptNum = p_iMaxAcceptNum <= 0 ? TCP_ACCEPT_NUM : p_iMaxAcceptNum;
	p_iThreadNum = p_iThreadNum <= 0 ? TCP_THREAD_NUM : p_iThreadNum;

	char szBuf[1024] = { 0 };
	char *pLogFold = (nullptr == p_szLogFold || '\0' == *p_szLogFold) ? nullptr : p_szLogFold;
	const char* pSafeLogFold = (nullptr == pLogFold) ? "" : pLogFold;

	TCP_INFO("[TCP服务] 服务IP: %s", p_szIp);
	TCP_INFO("[TCP服务] 服务端口: %d", p_nPort);
	TCP_INFO("[TCP服务] 线程数: %d", p_iThreadNum);
	TCP_INFO("[TCP服务] 队列数: %d", p_iQueueNum);
	TCP_INFO("[TCP服务] 缓存大小: %d", p_iRBufLen);
	TCP_INFO("[TCP服务] 最大Accept: %d", p_iMaxConnectNum);
	TCP_INFO("[TCP服务] 最大Connect: %d", p_iMaxAcceptNum);
	TCP_INFO("[TCP服务] 底层日志路径:%s", pSafeLogFold);

	//启动服务;
	if (!m_pTcpServerHandle->CreateTcpSock(p_szIp, p_nPort, p_iRBufLen, p_iMaxConnectNum, p_iMaxAcceptNum, TcpNotifyHandle, p_iThreadNum, p_iQueueNum, szBuf, pLogFold))
	{
		m_bStatus = false;
		TCP_WARN("[TCP服务] 创建TCP服务失败: %s", szBuf);
	}

	return m_bStatus;
}

void CTcpSockMng::Stop()
{
	if (nullptr == m_pTcpServerHandle)
		return;

	m_pTcpServerHandle->StopTcpSock();
}

int CTcpSockMng::ProcessAsynAns(NetRequsetDat *p_stNode, const char *p_szTransfer)
{
	if (nullptr == p_stNode->pServerHandle || nullptr == p_stNode->pClinetHandle)
	{
		TCP_WARN("[TCP服务] 发送应答, 用户已不存在,功能号=%d,ip=%s,port=%d",
			p_stNode->nGNID, p_stNode->szIp, p_stNode->usPort);
		return 0;
	}

	const char * packdata = p_szTransfer;

	//TCP_INFO("[TCP服务] 应答 cookie=%lu,mainid=%ld,[%p,%lu],reqid=%d,lsize=%d,len=%d",
	//	p_stNode->cookie, p_stNode->MainID, p_stNode->pServerHandle, (unsigned long)p_stNode->pClinetHandle, p_stNode->nGNID, p_stNode->lSize, p_stNode->len);

	//CHqMonitor::GetInstance()->AddMonitorGnData(POINT_APPEND, pNode, 0, MONITOR_HQSERVER, __FILE__, __LINE__, 997);

	CTcpSockMng::GetInstance()->TcpSockSend(p_stNode->pServerHandle, p_stNode->pClinetHandle, packdata, p_stNode->len);

	return 0;
}

bool CTcpSockMng::InitTcpServerInfo(const char* p_sHomePath)
{
	if (nullptr == p_sHomePath || strlen(p_sHomePath) == 0)
	{
		TCP_WARN("[TCP服务] 路径是空");
		return false;
	}

	std::string strDllPath = p_sHomePath;

#ifdef OS_IS_WINDOWS
	strDllPath.append("\\");
	strDllPath += TCP_DLL_NAME;
#else
	strDllPath.append("/");
	strDllPath += TCP_SO_NAME;
#endif

	m_pclLibraryOp = new CLibraryOp;
	if (!m_pclLibraryOp)
	{
		TCP_WARN("[TCP服务] 装载三方库类失败...");
		return false;
	}

	//加载监控动态库;
	int iRet = nsdk::GetFileAttr(strDllPath.c_str(), F_OK);
	if (0 != iRet)
	{
		nsdk_del(m_pclLibraryOp);
		TCP_WARN("[TCP服务] 动态库文件不存在[%s]...", strDllPath.c_str());
		return false;
	}

	//加载动态库;
	if (!m_pclLibraryOp->Load(strDllPath.c_str()))
	{
		nsdk_del(m_pclLibraryOp);
		TCP_WARN("[TCP服务] 动态库加载失败[%s]...", strDllPath.c_str());
		return false;
	}

	// 创建函数;
	pfnCreateTcpSockInstance fnCreateTcpSockInstance = nullptr;

	if (!m_pclLibraryOp->GetFuncAddress((void**)&fnCreateTcpSockInstance, "CreateTcpSockInstance") || nullptr == fnCreateTcpSockInstance)
	{
		nsdk_del(m_pclLibraryOp);
		TCP_WARN("[TCP服务] 获取方法[TcpSockIns]失败...");
		return false;
	}

	if (nullptr == m_pTcpServerHandle)
	{
		m_pTcpServerHandle = fnCreateTcpSockInstance();
		if (nullptr == m_pTcpServerHandle)
		{
			nsdk_del(m_pclLibraryOp);
			TCP_WARN("[TCP服务] 获取监控方法失败");
			return false;
		}
	}

	if (!m_pclLibraryOp->GetFuncAddress((void**)&m_fnDelTcpSockInstance, "DelTcpSockInstance") || nullptr == m_fnDelTcpSockInstance)
	{
		nsdk_del(m_pclLibraryOp);
		TCP_WARN("[TCP服务] 获取方法[DelTcpSockIns]失败...");
		return false;
	}

	//TODO 初始化日志;
	m_bStatus = true;

	TCP_INFO("[TCP服务] 初始化状态: %d", m_bStatus);

	return m_bStatus;
}

void CTcpSockMng::TcpSockSend(void* p_refServerHandle, void *p_refClinetHandle, const char* p_szData, int p_iDataLen)
{
	if (nullptr == m_pTcpServerHandle)
		return;

	m_pTcpServerHandle->TcpSockSend(p_refServerHandle, p_refClinetHandle, p_szData, p_iDataLen);
}

void CTcpSockMng::TcpSockClose(void* p_refServerHandle, void *p_refClinetHandle, const char* p_szData, int p_iDataLen)
{
	if (nullptr == m_pTcpServerHandle)
		return;

	m_pTcpServerHandle->TcpSockClose(p_refServerHandle, p_refClinetHandle, p_szData, p_iDataLen);
}

int CTcpSockMng::TcpSockCompare(void* p_refSrcClinetHandle, void *p_refObjClinetHandle)
{
	if (nullptr == m_pTcpServerHandle)
		return -2;

	return m_pTcpServerHandle->TcpSockCompare(p_refSrcClinetHandle, p_refObjClinetHandle);
}

