#include "TcpSockMng.h"
#include "HQLog.h"
#include "CodeMsg.h"
#include "UserMng.h"
#include "Globalvar.h"
#include "ServerProc.h"
#include "NetQueueMng.h"
#include "Func.h"
#include "Protocol.h"
#include "zlib.h"
#include "nsdk.h"
#include "HQServiceData.h"

CTcpSockMng* CTcpSockMng::m_pThis = nullptr;
CSocketServer *CTcpSockMng::m_pTcpServerHandle = nullptr;

#ifdef OS_IS_WINDOWS
bool bNoneedAuth = nullptr == fopen("test_auth.dat", "r+") ? false : true;
#else
bool bNoneedAuth = nullptr == fopen("./test_auth.dat", "r+") ? false : true;
#endif

void ReleasePackData(char *packdata)
{
	TDELARRY(packdata);
}

int ReleasePackDataFunc(void *lpParameter)
{
	if ( lpParameter )
		ReleasePackData((char*)lpParameter);
	return 0;
}

int ProcessIt(const char * ip, unsigned short port, USERDATA * puserdata,const char * packdata,int realdatalen)
{
	return CNetQueueMng::GetInstance()->ProcessIt(TCP_SOURCE_TYPE,ip, port, puserdata,packdata,realdatalen);
}

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
			WARN("解压错误:rawlen=%d,PacketLen=%d",pi->uiRawLen,pi->uiPacketLen);
			TDELARRY(tmpbuf);
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

void ProcessTcpData(void *p_refServerHandle, void *p_refClinetHandle, const void *p_szData, unsigned int p_uiDataLen, const char *p_szClientIp, unsigned short p_nClientPort)
{
	unsigned long long ulUnDoReqNum = 0;

	HS pServerHandle = (HS)p_refServerHandle;
	HCLIENT pClinetHandle = (HCLIENT)p_refClinetHandle;

	USERDATA * puserdata = CUserMng::GetInstance()->Query(pServerHandle, pClinetHandle);
	AutoReleaseFunc	autoFunc(puserdata, UserManageFunc);
	if (!puserdata)
	{
		const char *p_szErrData = "USER_NOT_EXIST";
		CTcpSockMng::GetInstance()->TcpSockClose(p_refServerHandle, p_refClinetHandle, (const char *)p_szErrData, strlen((char *)p_szErrData));

		WARN("[用户模块] %d:%s,ip=%s,port=%hu", USER_NOT_EXIST, 
			CCodeMsg::GetInstance()->GetCodeMsg(USER_NOT_EXIST).c_str(), p_szClientIp, p_nClientPort);
		return;
	}

	puserdata->pHostProcess = CServerProc::GetInstance();

	if (p_uiDataLen < TCP_RECVBUF_LEN)
	{
		//缓存包体 二次管理包体完整性
		memcpy(puserdata->pBuf + puserdata->iRecvLen, p_szData, p_uiDataLen);
		puserdata->iRecvLen += p_uiDataLen;

		//管理收到的内容
		AutoReleaseFunc	packFunc((void *)p_szData, ReleasePackDataFunc);

		//判断是否需要处理了,一个数据或许包含多个请求,此处循环处理
		while(true && !g_stHQServiceParam.bQuitAll )
		{
			if(puserdata->iRecvLen >= sizeof(ST_REQ_HEADER))
			{ 
				ST_REQ_HEADER *pi=nullptr;
				pi = (ST_REQ_HEADER*)puserdata->pBuf;
				if(pi->uiPacketLen <0 || pi->Info.ucCompressed > 2 || 
					pi->Info.ucVersion != NOW_VERSION ||
					pi->uiPacketLen >= MAX_CACHE_BUFLEN - sizeof(ST_REQ_HEADER) )
				{

					CUserMng::GetInstance()->Del(pServerHandle, pClinetHandle);

					WARN("[用户模块] %d:%s,ip=%s,port=%hu,包大小=%d,压缩=%d,版本=%d,最大缓存=%ld",
						USER_MULTIPLE_ABNORM, CCodeMsg::GetInstance()->GetCodeMsg(USER_MULTIPLE_ABNORM).c_str(), 
						p_szClientIp, p_nClientPort, pi->uiPacketLen, pi->Info.ucCompressed, pi->Info.ucVersion, MAX_CACHE_BUFLEN - sizeof(ST_REQ_HEADER));
					return;
				}

				//判断内容是否已经收完,没有则继续收
				if(puserdata->iRecvLen  <  pi->uiPacketLen+sizeof(ST_REQ_HEADER))
				{
					break;//不够一个包,退出循环继续收
				}

				xsdk_AtomicInc64(&puserdata->llNRequest);

				// 如果单个人队列超过了上线，关闭连接
				// 组合请求，会导致出现负数===>> 子包不减了
				if (g_stHQServiceParam.ullMaxInQueue < puserdata->llNRequest)
					g_stHQServiceParam.ullMaxInQueue = puserdata->llNRequest;
				if (puserdata->llNRequest > g_stHQServiceParam.nOneMaxQueue)
				{
					ulUnDoReqNum = CNetQueueMng::GetInstance()->DelUserReq(pServerHandle, pClinetHandle, p_szClientIp, p_nClientPort);
					CUserMng::GetInstance()->Del(pServerHandle, pClinetHandle);

					WARN("[用户模块] %d:%s,ip=%s,port=%hu,请求=%lld,未处理=%llu,最大=%d",
						USER_REQUEST_OEVERFLOW, CCodeMsg::GetInstance()->GetCodeMsg(USER_REQUEST_OEVERFLOW).c_str(),
						p_szClientIp, p_nClientPort, puserdata->llNRequest, ulUnDoReqNum, g_stHQServiceParam.nOneMaxQueue);
					return;
				}

				int		realdatalen = 0,iRecode = 0;
				char* packdata = GetPacketData(pi,&realdatalen);
				char* savebuf=puserdata->pBuf;
				AutoReleaseFunc	packFunc(packdata,ReleasePackDataFunc);

				if(packdata && realdatalen > 0 )
				{

					short ReqNo;
					memcpy(&ReqNo,packdata,sizeof(short));
					puserdata->llLastAcitve=time(0);

					bool bDealAuth = false;

					if (bNoneedAuth)
					{
						// 测试模式
						iRecode = ProcessIt(p_szClientIp, p_nClientPort, puserdata, packdata, realdatalen);
					}
					else
					{
						if (!puserdata->bAuth)
						{
							// 是不是认证包
							if (ReqNo == AUTH_NREQ)
							{
								bDealAuth = true;
								INFO("[认证模块] 完成认证, TCP用户[%p,%d] ip:%s post:%d  ", pServerHandle, (int)pClinetHandle, p_szClientIp, p_nClientPort);
							}
							else
							{
								CUserMng::GetInstance()->Del(pServerHandle, pClinetHandle);
								WARN("[认证模块] 未认证, 断开TCP用户[%p,%d] ip:%s post:%d", pServerHandle, (int)pClinetHandle, p_szClientIp, p_nClientPort);
								return;
							}
						}
						else
						{
							if (ReqNo == AUTH_NREQ)
							{
								bDealAuth = true;
								INFO("[认证模块] 再次认证, TCP用户[%p,%d] ip:%s post:%d  ", pServerHandle, (int)pClinetHandle, p_szClientIp, p_nClientPort);
							}
							else
							{
								iRecode = ProcessIt(p_szClientIp, p_nClientPort, puserdata, packdata, realdatalen);
							}
						}
					}

					if (0 > iRecode)//解析失败回包
					{
						// 回包
						ST_ANS_HEADER	AnsHeader = { 0 };
						// 创建应答包头
						AnsHeader.uiSeparator = SEPARATOR_NUM;
						AnsHeader.Info.ucVersion = NOW_VERSION;
						AnsHeader.Info.ucCompressed = COMP_DEFAULT;
						AnsHeader.Info.ucTalkCompress = 0;
						AnsHeader.uiRawLen = strlen(puserdata->szError);	// pHandleBuffer 要发送的数据长度
						AnsHeader.uiPacketLen = AnsHeader.uiRawLen;	// pHandleBuffer 要发送的数据长度
						AnsHeader.uiMainID = pi->uiMainID;
						AnsHeader.uiAssisID = pi->uiAssisID;
						AnsHeader.uiCookie = pi->uiCookie;
						AnsHeader.uiFunid = 0;

						ST_REQ_HEADER ReqHeader;
						memcpy(&ReqHeader, puserdata->pBuf, sizeof(ST_REQ_HEADER));

						int iLenSend = 0;
						char* pRsp = nullptr;

						iLenSend = sizeof(AnsHeader) + AnsHeader.uiPacketLen;
						pRsp = new char[iLenSend];
						memcpy(pRsp + sizeof(AnsHeader), puserdata->szError, AnsHeader.uiPacketLen);

						memcpy(pRsp, &AnsHeader, sizeof(AnsHeader));

						CTcpSockMng::GetInstance()->TcpSockSend(puserdata->pServerHandle, puserdata->pClinetHandle, pRsp, iLenSend);	// 直接发送应答数据
						TDELARRY(pRsp);
					}

					if (bDealAuth)
					{
						// 认证成功, 更新状态								
						CUserMng::GetInstance()->SetAuth(puserdata, true);

						// 回包
						ST_ANS_HEADER	AnsHeader = { 0 };
						// 创建应答包头
						AnsHeader.uiSeparator = SEPARATOR_NUM;
						AnsHeader.Info.ucVersion = NOW_VERSION;
						AnsHeader.Info.ucCompressed = COMP_DEFAULT;
						AnsHeader.Info.ucTalkCompress = 0;
						AnsHeader.uiRawLen = sizeof(ST_CLIENT_AUTH_ANS);	// pHandleBuffer 要发送的数据长度
						AnsHeader.uiPacketLen = AnsHeader.uiRawLen;	// pHandleBuffer 要发送的数据长度
						AnsHeader.uiMainID = pi->uiMainID;
						AnsHeader.uiAssisID = pi->uiAssisID;
						AnsHeader.uiCookie = pi->uiCookie;
						AnsHeader.uiFunid = ReqNo;

						// 包体
						ST_CLIENT_AUTH_ANS stAns = { 0 };
						stAns.iCode = 1;
						strcpy(stAns.szMsg, "OK");

						int iLenSend = sizeof(AnsHeader) + sizeof(stAns);
						char* pRsp = new char[iLenSend];
						memcpy(pRsp, &AnsHeader, sizeof(AnsHeader));
						memcpy(pRsp + sizeof(AnsHeader), &stAns, sizeof(stAns));

						CTcpSockMng::GetInstance()->TcpSockSend(puserdata->pServerHandle, puserdata->pClinetHandle, pRsp, iLenSend);	// 直接发送应答数据
						TDELARRY(pRsp);
					}

					if(true)
					{
						int left = puserdata->iRecvLen - (pi->uiPacketLen+sizeof(ST_REQ_HEADER));
						if ( left < 0 )
						{
							ulUnDoReqNum = CNetQueueMng::GetInstance()->DelUserReq(pServerHandle, pClinetHandle, p_szClientIp, p_nClientPort);

							CUserMng::GetInstance()->Del(pServerHandle, pClinetHandle);

							WARN("[用户模块] %d:%s,ip=%s,port=%hu,未处理=%llu",
								USER_REQREMAIN_FAIL, CCodeMsg::GetInstance()->GetCodeMsg(USER_REQREMAIN_FAIL).c_str(),
								p_szClientIp, p_nClientPort, ulUnDoReqNum);

							return;
						}

						if(left==0)
						{
							puserdata->iRecvLen=0;
						}
						else
						{
							if ( savebuf != puserdata->pBuf || puserdata->pBuf == nullptr  )
							{
								ulUnDoReqNum = CNetQueueMng::GetInstance()->DelUserReq(pServerHandle, pClinetHandle, p_szClientIp, p_nClientPort);

								CUserMng::GetInstance()->Del(pServerHandle, pClinetHandle);

								WARN("[用户模块] %d:%s,ip=%s,port=%hu,未处理=%llu,savebuf=%d,buf=%d",
									USER_REQBUF_OEVERFLOW, CCodeMsg::GetInstance()->GetCodeMsg(USER_REQBUF_OEVERFLOW).c_str(),
									p_szClientIp, p_nClientPort, ulUnDoReqNum, savebuf != puserdata->pBuf, puserdata->pBuf == nullptr);

								return;
							}
							memmove(puserdata->pBuf,puserdata->pBuf+(pi->uiPacketLen+sizeof(ST_REQ_HEADER)),left);
							puserdata->iRecvLen=left;
						}
					}
					else
					{

						CUserMng::GetInstance()->Del(pServerHandle, pClinetHandle); //请求失败，关闭客户端
						return;
					}



					if ( g_stHQServiceParam.bQuitAll )
					{
						ulUnDoReqNum = CNetQueueMng::GetInstance()->DelUserReq(pServerHandle, pClinetHandle, p_szClientIp, p_nClientPort);
						CUserMng::GetInstance()->Del(pServerHandle, pClinetHandle);

						WARN("[框架模块] %d:%s,ip=%s,port=%hu,未处理=%llu",
							FRAME_SERVER_STOP, CCodeMsg::GetInstance()->GetCodeMsg(FRAME_SERVER_STOP).c_str(),
							p_szClientIp, p_nClientPort, ulUnDoReqNum);

						return;//break;
					}

				}
				else
				{
					ulUnDoReqNum = CNetQueueMng::GetInstance()->DelUserReq(pServerHandle, pClinetHandle, p_szClientIp, p_nClientPort);

					CUserMng::GetInstance()->Del(pServerHandle, pClinetHandle);

					WARN("[用户模块] %d:%s,ip=%s,port=%hu,未处理=%llu,data=%d,len=%d",
						USER_GETREQUEST_ABNORM, CCodeMsg::GetInstance()->GetCodeMsg(USER_GETREQUEST_ABNORM).c_str(),
						p_szClientIp, p_nClientPort, ulUnDoReqNum, packdata == nullptr, realdatalen);

					return;
				}
			}
			else//包头未收完,继续收
			{
				break;
			}
		}
	}
	else//缓冲区太满,直接删
	{
		ulUnDoReqNum = CNetQueueMng::GetInstance()->DelUserReq(pServerHandle, pClinetHandle, p_szClientIp, p_nClientPort);
		CUserMng::GetInstance()->Del(pServerHandle, pClinetHandle);

		WARN("[用户模块] %d:%s,ip=%s,port=%hu,未处理=%llu,包大小=%d,缓存预设值=%d,最大值=%d",
			USER_RECV_OEVERFLOW, CCodeMsg::GetInstance()->GetCodeMsg(USER_RECV_OEVERFLOW).c_str(),
			p_szClientIp, p_nClientPort, ulUnDoReqNum, p_uiDataLen, puserdata->iRecvLen, MAX_CACHE_BUFLEN);
		return;
	}
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

			CUserMng::GetInstance()->Del(pServerHandle, pClinetHandle);
			g_stHQServiceParam.nClientNum--;

			INFO("[TCP服务] 用户断开TcpSock连接[%p,%lu],ip=%s,port=%d, 信息=%s",
				p_refServerHandle, (unsigned long)p_refClinetHandle, p_szClientIp, p_nClientPort, (char *)p_szErrData);
			//CHqMonitor::GetInstance()->AddClientData(p_szClientIp, p_nClientPort, g_UserManage.GetCount(), POINT_USER_DEL, __FILE__, __LINE__, DISCONNET_SERVER);
		}
		break;
	case enTcpConnect:
		{
			printf("上层应用收到: 连接通知, ip=%s,port=%d\n", p_szClientIp, p_nClientPort);

			USERDATA ud;
			ud.pServerHandle = pServerHandle;
			ud.pClinetHandle = pClinetHandle;
			ud.nNetSourceType = TCP_SOURCE_TYPE;	// 一定要标注数据来源，方便应答处理
			ud.llLastAcitve = time(0);
			ud.pHostProcess = CServerProc::GetInstance();	// 处理分支
			// 把申请缓冲区，放到Manager内部 
			ud.Init();		// 申请缓冲区
			if (CUserMng::GetInstance()->Add(&ud))	// me
			{
				USERDATA *puserdata = CUserMng::GetInstance()->Query(pServerHandle, pClinetHandle);
				AutoReleaseFunc	autoFunc(puserdata, UserManageFunc);
				if (puserdata)
				{
					g_stHQServiceParam.nClientNum++;
					g_stHQServiceParam.nUsedClientNum++;
					puserdata->iClientSockId = (int)ud.pClinetHandle;		// 记录下信息
					INFO("[TCP服务] 用户TcpSock连接成功[%p,%lu,%p]", pServerHandle, (unsigned long)pClinetHandle, pClinetHandle);
				}

				//CHqMonitor::GetInstance()->AddClientData(p_szClientIp, p_nClientPort, g_UserManage.GetCount(), POINT_USER_ADD, __FILE__, __LINE__);
			}
			else
			{
				ud.ResetIt();

				WARN("[TCP服务] %d:%s,ip=%s,port=%hu,最大用户数=%d,已有用户数=%d",
					USER_COUNT_TOPLIMIT, CCodeMsg::GetInstance()->GetCodeMsg(USER_COUNT_TOPLIMIT).c_str(),
					p_szClientIp, p_nClientPort, 
					CUserMng::GetInstance()->m_iMaxUserCount, 
					CUserMng::GetInstance()->m_iUserCount);
				//CHqMonitor::GetInstance()->AddClientData(p_szClientIp, p_nClientPort, g_UserManage.GetCount(), UNKNOW_POINT, __FILE__, __LINE__);
			}
		}
		break;
	case enTcpError:
		{
			//连接没关闭
			printf("上层应用收到: 错误通知, ip=%s,port=%d,err=%s\n", p_szClientIp, p_nClientPort, (char *)p_szErrData);

			CTcpSockMng::GetInstance()->TcpSockClose(p_refServerHandle, p_refClinetHandle, (const char *)p_szErrData, strlen((char *)p_szErrData));
			ulUnDoReqNum = CNetQueueMng::GetInstance()->DelUserReq((HS)p_refServerHandle, (HCLIENT)p_refClinetHandle, p_szClientIp, p_nClientPort);

			CUserMng::GetInstance()->Del(pServerHandle, pClinetHandle);
			g_stHQServiceParam.nClientNum--;

			WARN("[TCP服务] 错误通知 %d:%s,ip=%s,port=%hu,errdata=%s,未处理=%llu",
				USER_NET_BREAK, CCodeMsg::GetInstance()->GetCodeMsg(USER_NET_BREAK).c_str(), p_szClientIp, p_nClientPort, p_szErrData, ulUnDoReqNum);
			//CHqMonitor::GetInstance()->AddClientData(p_szClientIp, p_nClientPort, g_UserManage.GetCount(), POINT_USER_DEL, __FILE__, __LINE__, DISCONNET_SERVER);
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

	TDEL(m_pclLibraryOp);
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
	TDEL(m_pThis);
}

bool CTcpSockMng::Start(const char *p_szIp, unsigned short p_nPort, int p_iThreadNum, int p_iQueueNum, int p_iRBufLen, int p_iMaxConnectNum, int p_iMaxAcceptNum,
	char *p_szLogFold)
{
	if (nullptr == m_pTcpServerHandle)
	{
		WARN("[TCP服务] 创建TCP服务失败: 服务句柄是空");
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

	INFO("[TCP服务] 服务IP: %s", p_szIp);
	INFO("[TCP服务] 服务端口: %d", p_nPort);
	INFO("[TCP服务] 线程数: %d", p_iThreadNum);
	INFO("[TCP服务] 队列数: %d", p_iQueueNum);
	INFO("[TCP服务] 缓存大小: %d", p_iRBufLen);
	INFO("[TCP服务] 最大Accept: %d", p_iMaxConnectNum);
	INFO("[TCP服务] 最大Connect: %d", p_iMaxAcceptNum);
	INFO("[TCP服务] 底层日志路径:%s", pSafeLogFold);

	//启动服务;
	if (!m_pTcpServerHandle->CreateTcpSock(p_szIp, p_nPort, p_iRBufLen, p_iMaxConnectNum, p_iMaxAcceptNum, TcpNotifyHandle, p_iThreadNum, p_iQueueNum, szBuf, pLogFold))
	{
		m_bStatus = false;
		WARN("[TCP服务] 创建TCP服务失败: %s", szBuf);
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
		WARN("[TCP服务] 发送应答, 用户已不存在,功能号=%d,ip=%s,port=%d",
			p_stNode->nGNID, p_stNode->szIp, p_stNode->usPort);
		return 0;
	}

	const char * packdata = p_szTransfer + sizeof(ST_ANS_HEADER);

	INFO("[TCP服务] 应答 cookie=%lu,mainid=%ld,[%p,%lu],reqid=%d,lsize=%d,len=%d",
		p_stNode->cookie, p_stNode->MainID, p_stNode->pServerHandle, (unsigned long)p_stNode->pClinetHandle, p_stNode->nGNID, p_stNode->lSize, p_stNode->len);

	//CHqMonitor::GetInstance()->AddMonitorGnData(POINT_APPEND, pNode, 0, MONITOR_HQSERVER, __FILE__, __LINE__, 997);

	CTcpSockMng::GetInstance()->TcpSockSend(p_stNode->pServerHandle, p_stNode->pClinetHandle, packdata, p_stNode->len);

	return 0;
}

bool CTcpSockMng::InitTcpServerInfo(const char* p_sHomePath)
{
	if (nullptr == p_sHomePath || strlen(p_sHomePath) == 0)
	{
		WARN("[TCP服务] 路径是空");
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
		WARN("[TCP服务] 装载三方库类失败...");
		return false;
	}

	//加载监控动态库;
	int iRet = nsdk::GetFileAttr(strDllPath.c_str(), F_OK);
	if (0 != iRet)
	{
		TDEL(m_pclLibraryOp);
		WARN("[TCP服务] 动态库文件不存在[%s]...", strDllPath.c_str());
		return false;
	}

	//加载动态库;
	if (!m_pclLibraryOp->Load(strDllPath.c_str()))
	{
		TDEL(m_pclLibraryOp);
		WARN("[TCP服务] 动态库加载失败[%s]...", strDllPath.c_str());
		return false;
	}

	// 创建函数;
	pfnCreateTcpSockInstance fnCreateTcpSockInstance = nullptr;

	if (!m_pclLibraryOp->GetFuncAddress((void**)&fnCreateTcpSockInstance, "CreateTcpSockInstance") || nullptr == fnCreateTcpSockInstance)
	{
		TDEL(m_pclLibraryOp);
		WARN("[TCP服务] 获取方法[TcpSockIns]失败...");
		return false;
	}

	if (nullptr == m_pTcpServerHandle)
	{
		m_pTcpServerHandle = fnCreateTcpSockInstance();
		if (nullptr == m_pTcpServerHandle)
		{
			TDEL(m_pclLibraryOp);
			WARN("[TCP服务] 获取监控方法失败");
			return false;
		}
	}

	if (!m_pclLibraryOp->GetFuncAddress((void**)&m_fnDelTcpSockInstance, "DelTcpSockInstance") || nullptr == m_fnDelTcpSockInstance)
	{
		TDEL(m_pclLibraryOp);
		WARN("[TCP服务] 获取方法[DelTcpSockIns]失败...");
		return false;
	}

	//TODO 初始化日志;
	m_bStatus = true;

	INFO("[TCP服务] 初始化状态: %d", m_bStatus);

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

