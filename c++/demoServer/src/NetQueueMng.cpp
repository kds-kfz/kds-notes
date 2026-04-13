#include "NetQueueMng.h"
//#include "ServerProc.h"
#include "WebSockMng.h"
#include "HttpSockMng.h"
#include "CodeMsg.h"
#include "Log.h"
#include "AutoCS.h"
#include "UserMng.h"

#include "nsdk_atomic.h"

#define START_VERSION	10		//10以前的版本为旧飞越2000版本
#define REQBUFFER_LEN	1024*1024	//100*1024	// 2*1024	//20*1024
#define ANSBUFFER_LEN	1024*1024 //32*1024 //40*8196	//,在新版主站应增加回应缓冲区32K,防止超过unsigned short 65535 
#define COSENDBUF_HUGE	1024*1024*10
#define COSENDBUF_MAXHUGE	1024*1024*15
#define COSENDBUF_BIG	1024*1024*3	//1024*1024*10
#define COSENDBUF_MID	1024*1024	//1024*1024*2
#define COSENDBUF_SMALL	1024*800	// 1024*1024
#define COSENDBUF_SSMALL 1024

#define LOG_BUF_SIZE	(1024)
#define VIEW_BUF_SIZE	(512)

nsdk_atomic_t64 g_llMiddleSynId = 0;//异步ID,用来区别中间件应答的包的;
int g_uiAnsThreadNum = 10;

CNetQueueMng *CNetQueueMng::m_pThis = NULL;
nsdk_atomic_t64 CNetQueueMng::m_ullReqId = 0;

CNetQueueMng* CNetQueueMng::GetInstance()
{
	if(m_pThis == NULL)
	{
		m_pThis = new CNetQueueMng;
	}
	return m_pThis;
}

void CNetQueueMng::Release()
{
	nsdk_del(m_pThis);
}

// REQBUFFER_LEN+ANSBUFFER_LEN+sizeof(NetRequsetDat)+4*1024
CNetQueueMng::CNetQueueMng(void)//:m_MemNetRequsetDat(sizeof(NetRequsetDat)),
	//m_MemReq(REQBUFFER_LEN),m_MemSmallAns(COSENDBUF_SMALL),m_MemBigAns(COSENDBUF_BIG),m_MemMidAns(COSENDBUF_MID),m_MemView(256)
{
	m_Stop			= FALSE;
	m_clEventMiddle.Create();
	m_llUnComplete	= 0;
}


CNetQueueMng::~CNetQueueMng(void)
{
	Uninit();
}
// NetSourceType 是网络来源，TCP/HTTP/WEBSOCK，后面两种进入 ProcessIt 必须是protobuf，另外单独的HTTP是旧格式csv，不进入ProcessIt
// ReqPackType 是请求格式，二进制和protobuf两种
int CNetQueueMng::ProcessIt(int NetSourceType,const char * ip, unsigned short port, USERDATA * puserdata,const char * packdata_org,int realdatalen_org)
{
	long			len=0;
	//long			reqheadlen = sizeof(ST_REQ_HEADER);
	ST_REQ_HEADER		ReqHeader = { 0 };
	short			ReqNo=0;	// 这个当做short拷贝的，不能用long
	const char *	packdata = packdata_org;
	int				realdatalen = realdatalen_org;
	std::string		mockpack;
	//HANDLE		hreqprotobuf = NULL;
	//Jzt::QuotePackage  reqproto;
	int			MyNo = puserdata->iClientSockId;
	int			ReqPackType = TCP_PACK_TYPE;
	short		nErrCode = UNKNOW_ABNORM;
	bool		bJson = false;
	std::string strErrMsg = "";

	// 如果请求缓冲区太大，直接打回去
	if (realdatalen_org*1.2 > REQBUFFER_LEN)
	{
		nErrCode = USER_REQLEN_ABNORM;
		MT_WARN("%d:USER_REQLEN_ABNORM,ip=%s,port=%hu,reallen=%d,maxlen=%d",
			nErrCode, ip, port, realdatalen_org, REQBUFFER_LEN);
		return -1;
	}

	// 基于TCP的Protobuf和二进制tcp协议,都带有二进制包头
	if (NetSourceType == TCP_SOURCE_TYPE  ) 
	{
		
	}
	else if (NetSourceType == HTTP_SOURCE_TYPE)
	{

	}
	else if (NetSourceType == WEBSOCK_SOURCE_TYPE)
	{
		ReqPackType = PROTOBUF_PACK_TYPE;
		// 根据protobuffer，重新创建个二进制请求包头	// 解析，并且还原二进制协议头
		std::string strErrMsg = "";
		//TODO 获取包体
		mockpack.resize(realdatalen);
		memcpy(&*mockpack.begin(), packdata, realdatalen);
		if (mockpack.empty())
		{
			puserdata->enMsgType = bJson ? MSG_PROTO_JSON : MSG_PROTO_BINARY;
			_snprintf(puserdata->szError, sizeof(puserdata->szError) - 1, "WebSock Proto Parse Fail,%s", strErrMsg.c_str());
			return -1;//可能不是proto协议或是proto请求参数错误
		}
		// 根据protobuffer，重新创建个二进制请求包头
		packdata = mockpack.c_str();	// 重新指向二进制展开的包体
		realdatalen = mockpack.size();

		ReqHeader.uiCookie = 0;
		ReqHeader.uiAssisID = 0;
		ReqHeader.uiMainID = 0;
		ReqHeader.uiPacketLen = realdatalen;
		ReqHeader.Info.ucTalkCompress = PROTOBUF_PACK_TYPE;
	}
	else
	{
		nErrCode = USER_NETWORK_TYPE;
		MT_WARN("%d:USER_NETWORK_TYPE,ip=%s,port=%hu,nettype=%d",
			nErrCode, ip, port, NetSourceType);
		return -4;
	}

	NetRequsetDat *pReq = new NetRequsetDat;
	if(NULL == pReq)
	{
		nErrCode = USER_NETWORK_TYPE;
		MT_WARN("%d:USER_REQ_NEW_FAIL,ip=%s,port=%hu,nettype=%d",
			nErrCode, ip, port, NetSourceType);
		return -5;
	}

	pReq->nNetSourceType = NetSourceType;		// 网络包来源类型
	pReq->nReqPackType = ReqPackType;			// 请求内容的格式类型
	pReq->lSize = realdatalen;
	strncpy(pReq->szIp, ip, 19);
	pReq->usPort = port;
	pReq->bJson = bJson;
	if (NetSourceType == TCP_SOURCE_TYPE)
	{
		
	}
	else if (NetSourceType == HTTP_SOURCE_TYPE  || NetSourceType == WEBSOCK_SOURCE_TYPE )
	{
		pReq->bHandleOverProtobuf = false;
		pReq->lSize = mockpack.size();	// 新的长度
		// 一定要修改packlen
		ReqHeader.uiRawLen = ReqHeader.uiPacketLen = mockpack.size();
	}
	int	maxsizelen = std::max<int>(pReq->lSize, realdatalen);
	if (pReq->lSize > realdatalen)
	{
		MT_INFO("请求缓冲区变大,req=%d,lsize=%d,reallen=%d,ip=%s", ReqNo, pReq->lSize, realdatalen, ip);
	}
	pReq->ullReqId = nsdk_AtomicInc64(&m_ullReqId);
	pReq->lpReqPackData = new char[ (1+(maxsizelen /1024))*1024 ]; //[REQBUFFER_LEN];	// static_cast<char*>(m_MemReq.ordered_malloc());
	pReq->pTransfer = NULL;
	pReq->pViewBuf = NULL;
	pReq->nGNID = ReqNo;

	pReq->pServerHandle = puserdata->pServerHandle;					// 客户端的
	pReq->pClinetHandle = puserdata->pClinetHandle;				// 客户端的
	pReq->cookie = ReqHeader.uiCookie;				// 用户上层自己携带的信息
	pReq->MainID = ReqHeader.uiMainID;
	pReq->AssisID = ReqHeader.uiAssisID;
	//pReq->pHostProcess = puserdata->pHostProcess;
	pReq->lMiddle_synID = nsdk_AtomicInc64(&g_llMiddleSynId);
	// 根据实际异步触发，自行设置回调函数 TODO异步队列调用
	//nr->callbackfn	= CNetQueueMng::JsonICEAsynRoutine;	// ICE RPC专用;通用异步resume函数：CoroAsynRoutineResumCallBack
	pReq->lSize	= realdatalen;		//请求缓冲区长度
	memcpy(pReq->lpReqPackData,packdata,pReq->lSize);	//请求的缓冲区
	pReq->len = ReqHeader.uiPacketLen;		// 长度一定要复制， 具体处理会跟sizeof（对应结构) 比较长度，不合法会跳过
	pReq->ltime = time(NULL);

	// 组合预备
	int		m = 0;

	//TODO 如果是组合包,有这场景的话,在这里拆分到队列;

	nsdk_AtomicInc64(&m_llUnComplete);
	m_aRequest.AddToTail(pReq);
	m_clEventMiddle.SetEvent();

	return 0;
}

bool CNetQueueMng::Init()
{
	m_Stop = FALSE;
	m_clEventMiddle.Create();

	int iThreadNum = g_uiAnsThreadNum;
	m_vecHandleThread.resize(iThreadNum);

	for (int i = 0; i <m_vecHandleThread.size(); ++i)
	{
		//m_vecHandleThread[i].managercoro  = NULL;
		m_vecHandleThread[i].iThreadNo	= i;
		m_vecHandleThread[i].pcNetQueue = new CNetQueueThread();
		m_vecHandleThread[i].pcNetQueue->SetThis(this, i);
		m_vecHandleThread[i].pcNetQueue->CreateThread();
	}

	MT_INFO("[MT服务] 初始化完成, 线程数:%d, 开始工作", iThreadNum);

	return true;
}

void CNetQueueMng::Uninit()
{
	int		i;
	long	lPre = 0;
	m_Stop = TRUE;

	m_clEventMiddle.Close();

	for ( i=0;i<m_vecHandleThread.size();++i )
	{
		//关闭工作线程对象;
		if(NULL != m_vecHandleThread[i].pcNetQueue)
		{
			m_vecHandleThread[i].pcNetQueue->DeleteThread();
			nsdk_del(m_vecHandleThread[i].pcNetQueue);
		}

		m_vecHandleThread[i].aSynAns.ClearAll();
	}
	m_vecHandleThread.clear();
}

int CNetQueueMng::HandleAnswer(int p_iThreadNo)
{
	int		nDoTimes = 0;
	while (!m_vecHandleThread[p_iThreadNo].aSynAns.IsEmpty())
	{
		NetRequsetDat *pNode = NULL;
		m_vecHandleThread[p_iThreadNo].aSynAns.RemoveFromHead(pNode);
		if(NULL == pNode)
			continue;
		CNetQueueMng::GetInstance()->BusinessNotCoro(pNode);
		++nDoTimes;
		// 可以无限次，直到Complete结束 
		if (pNode->status == Complete)
		{
			m_vecHandleThread[p_iThreadNo].aDeleteNode.AddToTail(pNode);
		}
	}
	return nDoTimes;
}

int CNetQueueMng::DeleteCoroNode(int p_iThreadNo)
{
	int nDoTimes = 0;

	while ( !m_vecHandleThread[p_iThreadNo].aDeleteNode.IsEmpty() )
	{
		NetRequsetDat *pNode = NULL;
		m_vecHandleThread[p_iThreadNo].aDeleteNode.RemoveFromHead(pNode);
		if(NULL == pNode)
			continue;
		for (int i = 0; i < pNode->lComboReqNum;i++)
		{
			m_vecHandleThread[pNode->pstNetReqDats[i]->iThreadNo].aDeleteNode.AddToTail(pNode->pstNetReqDats[i]);
		}

		DeleteNetNode(pNode);
		++nDoTimes;
	}
	return nDoTimes;
}

void CNetQueueMng::DeleteNetNode(NetRequsetDat* p_pNode)
{
	if(NULL == p_pNode)
		return;

	if (p_pNode->pTransfer)
	{
		FreeMemPool(p_pNode->memType, p_pNode->pTransfer);
	}

	nsdk_del_arry(p_pNode->lpReqPackData);
	nsdk_del_arry(p_pNode->pViewBuf);
	nsdk_del_arry(p_pNode->pstNetReqDats);
	nsdk_del(p_pNode);

	nsdk_AtomicDec64(&m_llUnComplete);
}

//多线程异步处理;
int	CNetQueueMng::HandleThread(short p_iThreadNo)
{
	//if(g_bQuitAll || m_Stop)
	if (m_Stop)
		return 0;

	unsigned int uiTimeOut = 500;
	//旧逻辑：线程信号用于线程被唤醒，触发场景：异步请求异步返回，组合请求未完成的子请求继续被唤醒处理;
	//新逻辑：没有携程，不考虑组合请求再次触发线程回调;
	//超时往下走,最大等1毫秒;
	int iRet = m_clEventMiddle.WaitEvent(uiTimeOut);
	if(XSDK_TIMEOUT == iRet || XSDK_KO == iRet)
	{
		//return 0;
	}

	//旧逻辑: 总请求未处理数大于所有客户端请求未处理的阈值，则从旧请求中找出组合包，然后放回队列;
	//新逻辑：组合请求用于pb协议，但是目前不支持，这里的代码删除。简单处理，超过阈值时，直接丢弃最旧请求。不考虑组合;
	size_t uiRequestNum = m_aRequest.GetCount();
	if(uiRequestNum == 0)
	{
		nsdk_Sleep(10);
		return 0;
	}

	//这里加锁处理，清理期间不能再接收请求;
	//if (uiRequestNum > g_nMaxQueue)
	if (uiRequestNum > 50)
	{
		int nErrCode = FRAME_REQUEST_OEVERFLOW;
		MT_WARN("[请求管理] %d:%s, 请求总数=%zu,未返回数=%lld",
			nErrCode, CCodeMsg::GetInstance()->GetCodeMsg(nErrCode).c_str(), uiRequestNum, m_llUnComplete);

		while(!m_aRequest.IsEmpty())
		{
			NetRequsetDat *pNode = NULL;
			m_aRequest.RemoveFromHead(pNode);
			if(NULL == pNode)
				continue;
			for (int i=0;i<pNode->lComboReqNum;++i )
			{
				// 组合包，连同子包计数器减掉;
				// 并没有进入队列;
				DeleteNetNode(pNode->pstNetReqDats[i]);
			}

			ProcessAsynError(pNode);

			DeleteNetNode(pNode);
		}

		MT_WARN("[请求管理] 请求总数=%d,请求队列过大,清理完成,未完成的请求=%lld", uiRequestNum, m_llUnComplete);
	}

	//这里没有携程;不考虑携程积压情况;

	//如果队列堆积;已经清理完一部分;
	
	//每个线程进来,所有请求都会被取走,每个请求都会有处理完成状态,这时候就应答或删除;
	while(!m_aRequest.IsEmpty())
	{
		NetRequsetDat *pNode = NULL;
		m_aRequest.RemoveFromHead(pNode);
		if(NULL == pNode)
			continue;

		pNode->iThreadNo = p_iThreadNo;
		HandleAnswer(p_iThreadNo);
		DeleteCoroNode(p_iThreadNo);
		if ( m_Stop )
		{
			break;
		}

		//不创建携程直接执行业务;
		pNode->status = NoStart;
		CNetQueueMng::GetInstance()->BusinessNotCoro(pNode);

		// 直接处理完成，没有yield才会结束，中间有yield的不会立刻执行，不对多次进入aDeleteNode， 因为同一个协程锁定固定线程
		if ( pNode->status == Complete )	
		{
			m_vecHandleThread[p_iThreadNo].aDeleteNode.RemoveFromHead(pNode);
		}
	}
	HandleAnswer(p_iThreadNo);
	DeleteCoroNode(p_iThreadNo);
}

void CNetQueueMng::BusinessNotCoro(NetRequsetDat * p_stNode)
{
	p_stNode->status = Running;
	p_stNode->lbeginhandletime = time(NULL);

	DoBusiness(p_stNode);//如果转发异步应答超时直接返回错误;异步网络库直接丢弃；

	p_stNode->ltimeend = time(NULL);

	ProcessAsynAns(p_stNode);
}

//用户断开连接删除对应请求队列，防止队列堆积
unsigned long long CNetQueueMng::DelUserReq(HS p_refServerHandle, HCLIENT p_refClinetHandle, const char *p_szIp, unsigned short p_usPort)
{
	unsigned long long ulUnDoReqNum = 0;
	NetRequsetDat *pNode = NULL;

	m_aRequest.RefLock();
	deque<NetRequsetDat *> &refReq = m_aRequest.Reffer();
	for (auto it = refReq.begin(); it != refReq.end();)
	{
		pNode = *it;
		if (pNode->pServerHandle == p_refServerHandle && 
			pNode->pClinetHandle == p_refClinetHandle)
		{
			++ulUnDoReqNum;
			DeleteNetNode(pNode);
			it = refReq.erase(it);
		}
		else
		{
			++it;
		}
	}
	m_aRequest.RefUnlock();

	return ulUnDoReqNum;
}

// 处理数据可能来至不同的服务，获取的方式可能都是异步的，通过回调函数
void CNetQueueMng::DoBusiness(NetRequsetDat * r)
{
	short nErrCode = UNKNOW_ABNORM;

	BOOL	bFindClient = TRUE;
	USERDATA *puserdata = CUserMng::GetInstance()->Query(r->pServerHandle, r->pClinetHandle);
	if (!puserdata)
	{	
		// 避免断开的客户端无效请求
		bFindClient = FALSE;

		nErrCode = USER_NOT_EXIST;
		MT_WARN("[用户模块] %d:%s,ip=%s,port=%hu,reqid=%llu,creqid=%ld",
			nErrCode, CCodeMsg::GetInstance()->GetCodeMsg(nErrCode).c_str(), r->szIp, r->usPort, r->ullReqId, r->MainID);
	}
	else
	{
		// 组合请求，会导致出现负数===>> 子包不减了
		if ( !r->hasParent )
		{
			nsdk_AtomicDec64(&puserdata->llNRequest);
		}
	}
	CUserMng::GetInstance()->ReleaseIt(puserdata);
	//////////////////////////////////////////////////////////////////////////
	int req = r->nGNID;//CServerProc::GetProcessReq(r->lpReqPackData);
	// 大类型
	switch (req)
	{
		{
			//TODO 如果是组合包;拆包放入请求队列,现暂不考虑;
		}
	default:
		{
			r->pTransfer = MallocMemPool(req, r->memType, r->maxAlloclen);
			r->pViewBuf = (char*)new char[VIEW_BUF_SIZE];//m_MemView.ordered_malloc();

			memset(r->pViewBuf, 0, VIEW_BUF_SIZE);
			ST_ANS_HEADER * ans = (ST_ANS_HEADER*)r->pTransfer;
			ans->uiCookie = r->cookie;
			ans->uiAssisID = r->AssisID;
			ans->uiMainID = r->MainID;
			ans->Info.ucTalkCompress = r->nReqPackType;

			// 正常处理业务;
			r->nRet = r->pHostProcess->Process(r, r->lpReqPackData, r->pTransfer + sizeof(ST_ANS_HEADER), r->len, r->pViewBuf);
		}
		break;
	}

}

int CNetQueueMng::ProcessAsynError(NetRequsetDat *p_pNode)
{
	//CHqMonitor::GetInstance()->AddMonitorGnData(POINT_APPEND, p_pNode, 0, MONITOR_HQSERVER, __FILE__, __LINE__, 998);
	char szErrBuf[1024] = { 0};
	if (p_pNode->nReqPackType == PROTOBUF_PACK_TYPE)
	{
	}
	else
	{
		char szErrInfo[256] = { 0 };
		sprintf(szErrInfo, "gnid=%d,reqid=%llu,creqid=%ld,reqlen=%d,anslen=%d,maxlen=%d,ret=%d,areq=%llu,ip=%s,port=%hu asyn error",
			p_pNode->nGNID, p_pNode->ullReqId, p_pNode->MainID, p_pNode->lSize, p_pNode->len, p_pNode->maxAlloclen, p_pNode->nRet, m_aRequest.GetCount(), p_pNode->szIp, p_pNode->usPort);
		
		//被动引发，不是原因，队列强制清理导致
		MT_WARN("[异步应答] gnid=%d,reqid=%llu,creqid=%ld,reqlen=%d,anslen=%d,maxlen=%d,ret=%d,areq=%llu,ip=%s,port=%hu asyn error",
			p_pNode->nGNID, p_pNode->ullReqId, p_pNode->MainID, p_pNode->lSize, p_pNode->len, p_pNode->maxAlloclen, p_pNode->nRet, m_aRequest.GetCount(), p_pNode->szIp, p_pNode->usPort);

		p_pNode->nGNID = 0;	// 修改了返回的错误功能号
		p_pNode->len = strlen(szErrInfo);
		memcpy(szErrBuf + sizeof(ST_ANS_HEADER), szErrInfo, p_pNode->len);

	}

	if (p_pNode->nNetSourceType == TCP_SOURCE_TYPE)
	{
		
	}
	else if (p_pNode->nNetSourceType == HTTP_SOURCE_TYPE)
	{

	}
	else if (p_pNode->nNetSourceType == WEBSOCK_SOURCE_TYPE)
	{

	}

	return 0;
}

int CNetQueueMng::ProcessAsynAns(NetRequsetDat * pNode )
{
	int	nLeft = 0;
	short nErrCode = UNKNOW_ABNORM;
	std::string strMsg = "";
	// 组合包缓冲区延迟申请，用完及删除，减少资源消耗
	try
	{
		if (!pNode->pTransfer)
			pNode->pTransfer = MallocMemPool(pNode->nGNID, pNode->memType, pNode->maxAlloclen);
		if (!pNode->pViewBuf)
		{
			pNode->pViewBuf = (char*)new char[VIEW_BUF_SIZE];//m_MemView.ordered_malloc();
			memset(pNode->pViewBuf, 0, VIEW_BUF_SIZE);
		}

		if (pNode->len >= pNode->maxAlloclen)
		{
			FreeMemPool(pNode->memType, pNode->pTransfer);
			pNode->memType = AnsMemDyn;
			pNode->maxAlloclen = pNode->len;
			pNode->pTransfer = MallocMemPool(pNode->nGNID, pNode->memType, pNode->maxAlloclen);

			if (pNode->pTransfer == NULL)
			{
				nErrCode = USER_RESBUF_OEVERFLOW;
				MT_WARN("[MT服务] %d:%s,ip=%s,port=%hu,gnid=%d,reqid=%llu,creqid=%ld,type=%d,maxlen=%ld",
					nErrCode, CCodeMsg::GetInstance()->GetCodeMsg(nErrCode).c_str(), pNode->szIp, pNode->usPort, pNode->nGNID, pNode->ullReqId, pNode->MainID, pNode->memType, pNode->maxAlloclen);
				throw "failed to answer memory request";
			}
		}
		if (pNode->nReqPackType == PROTOBUF_PACK_TYPE || pNode->nReqPackType == 2)
		{
			// 如果协议处理出错，统一处理
			if (pNode->nRet == FRAME_DEAL || pNode->nRet == BUSINESS_DEAL || pNode->len <= 0 || pNode->len >= pNode->maxAlloclen)
			{
				
			}
			else
			{
				if (pNode->bHandleOverProtobuf == false)	// 对于差分的，case分支直接处理完成 
				{
					
				}
				else
				{
					// 表示给的数据就是已经处理好的 protobuf 格式,直接给客户端 TODO
				}
			}
		}
		else
		{
			// 如果协议处理出错，统一处理
			if (pNode->nRet == FRAME_DEAL || pNode->nRet == BUSINESS_DEAL || pNode->len <= 0 || pNode->len >= pNode->maxAlloclen)
			{
				char	szMsg[LOG_BUF_SIZE] = { 0 };
				if (pNode->nRet == FRAME_DEAL)
				{
					nErrCode = FRAME_DEAL;
					strMsg = "FRAME_DEAL";
				}
				else if (pNode->nRet == BUSINESS_DEAL)
				{
					nErrCode = BUSINESS_DEAL;
					strMsg = "BUSINESS_DEAL";
				}
#if defined( OS_IS_WINDOWS )
				_snprintf(szMsg, LOG_BUF_SIZE - 1, "[MT_WARN] %d:%s,msg=%s,gnid=%d,reqid=%llu,creqid=%ld",
					nErrCode, strMsg.c_str(), pNode->pViewBuf, pNode->nGNID, pNode->ullReqId, pNode->MainID);
#else
				snprintf(szMsg, LOG_BUF_SIZE - 1, "[MT_WARN] %d:%s,msg=%s,gnid=%d,reqid=%llu,creqid=%ld",
					nErrCode, strMsg.c_str(), pNode->pViewBuf, pNode->nGNID, pNode->ullReqId, pNode->MainID);
#endif
				MT_WARN("[MT服务] 应答异常 %d:%s,信息=%s,功能号=%d,请求id=%llu,客户请求id=%ld,返回值=%d,包长度=%ld,ip=%s,port=%hu",
					nErrCode, CCodeMsg::GetInstance()->GetCodeMsg(nErrCode).c_str(), pNode->pViewBuf, pNode->nGNID, pNode->ullReqId, pNode->MainID, pNode->nRet, pNode->len, pNode->szIp, pNode->usPort);

				pNode->nGNID = 0;	// 修改了返回的错误功能号
				pNode->len = strlen(szMsg) + 1;
				memcpy(pNode->pTransfer + sizeof(ST_ANS_HEADER), szMsg, pNode->len);

			}
		}
		//////////////////////////////////////////////////////////////////////////
		if (pNode->nNetSourceType == TCP_SOURCE_TYPE)
		{

		}
		else if (pNode->nNetSourceType == HTTP_SOURCE_TYPE)
		{
		}
		else if (pNode->nNetSourceType == WEBSOCK_SOURCE_TYPE)
		{
			CWebSockMng* wb = (CWebSockMng*)pNode->pServerHandle;
			if (!pNode->hasParent)
				wb->ProcessAsynAns(pNode, pNode->pTransfer);
		}
	}
	catch (const char *s)
	{
		MT_ERROR("[MT服务] 应答异常1[%s]", s);
	}
	catch (std::exception &e)
	{
		MT_ERROR("[MT服务] 应答异常2[%s]", e.what());
	}
	catch (...)
	{
		MT_ERROR("[MT服务] 未知的应答异常");
	}

	//非组合包到这里就已经处理结束;
	//不考虑组合包;TODO如果有这种场景;那就在创建请求时候就把子请求放到队列;
	pNode->status = Complete;	// 进入删除队列;

	return 0;
}

void CNetQueueMng::FreeMemPool(AnsMemType p_enMemType, char *p_pBuf)
{
	if(NULL == p_pBuf)
		return;

	if (p_enMemType == AnsMemHuge)
	{
		delete[] p_pBuf;
		p_pBuf = NULL;
	}
	else if (p_enMemType == AnsMemBig)
	{
		delete[] p_pBuf;
		p_pBuf = NULL;
	}
	else if (p_enMemType == AnsMemSmall)
	{
		delete[] p_pBuf;
		p_pBuf = NULL;
	}
	else
	{
		delete[] p_pBuf;
		p_pBuf = NULL;
	}
}

// 多次因为1106 代码表超过2M，导致内存越界，破坏内存池，出现MallockMemPoll异常；要准备解决内存问题，暂时改成new delete方式
char* CNetQueueMng::MallocMemPool(short p_nGNID, AnsMemType &p_refMemType,long &p_reflMaxAlloclen)
{
	char* pTransfer = NULL;

	if (p_refMemType == AnsMemDyn && p_reflMaxAlloclen >= (COSENDBUF_SMALL - sizeof(ST_ANS_HEADER)))
	{
		if (p_reflMaxAlloclen < (COSENDBUF_MID - sizeof(ST_ANS_HEADER)))
		{
			p_refMemType = AnsMemMid;
			p_reflMaxAlloclen = COSENDBUF_MID - sizeof(ST_ANS_HEADER);
			pTransfer = (char*)new char[COSENDBUF_MID];
		}
		else if (p_reflMaxAlloclen < (COSENDBUF_BIG - sizeof(ST_ANS_HEADER)))
		{
			p_refMemType = AnsMemBig;
			p_reflMaxAlloclen = COSENDBUF_BIG - sizeof(ST_ANS_HEADER);
			pTransfer = (char*)new char[COSENDBUF_BIG];
		}
		else if (p_reflMaxAlloclen < (COSENDBUF_HUGE - sizeof(ST_ANS_HEADER)))
		{
			p_refMemType = AnsMemHuge;
			p_reflMaxAlloclen = COSENDBUF_HUGE - sizeof(ST_ANS_HEADER);
			pTransfer = (char*)new char[COSENDBUF_HUGE];
		}
		else //没配匹配上内存段，按实际的大小申请,wyl 20240326
		{
			pTransfer = (char*)new char[p_reflMaxAlloclen];
		}
		if (!pTransfer)
		{
			p_refMemType = AnsMemSSmall;
			p_reflMaxAlloclen = COSENDBUF_SSMALL - sizeof(ST_ANS_HEADER);
			pTransfer = (char*)new char[COSENDBUF_SSMALL];	//以上都失败则只返回简单错误, < 1024 应答不需要压缩
			MT_WARN("[异步请求] 类型=%d,内存申请失败1", p_refMemType);
			if (!pTransfer)
			{
				MT_WARN("[异步请求] 类型=%d,内存申请失败2", p_refMemType);
			}
		}
	}

	return pTransfer;
}

