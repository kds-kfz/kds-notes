/*
d:\ZeroC\Ice-3.5.1\bin\slice2cpp.exe -Id:\ZeroC\Ice-3.5.1\slice\ JSONBINRPCU.ICE


2015
Asynchronous Method Dispatch (AMD) 。
Using AMD, a server can receive a request but then suspend its processing in order to release the dispatch thread as soon as possible. 
When processing resumes and the results are available, the server sends a p_fnResponse explicitly using a callback object provided by the Ice run time.
AMD对客户端来说是透明的，客户端无需区分.


2007
配置必须的步骤：
1. Create an object m_refAdapter to receive callback requests. This m_refAdapter does not
require endpoints if its only purpose is to receive callbacks over bi-directional
connections.
建立一个对象适配器，用来接收callback请求。这个适配器不需要endpoint，如果他仅仅是用来接收callbacks的。
2. Register the callback object with the object m_refAdapter.
用这个对象适配器注册这个callback对象
3. Activate the object m_refAdapter. 激活
4. Obtain the Ice::Connection object (see Section 33.5.1) by calling
ice_connection on the refProxy.
5. Invoke setAdapter on the connection, passing the callback object m_refAdapter.
This associates an object m_refAdapter with the connection and enables callback
requests to be dispatched.
在这个连接上调用setAdapter，传递callback对象给这个适配器。这样就关联一个对象适配器和这个连接，同时允许请求被分派。
6. Pass the identity of the callback object to the server.
把callback对象的identity传递给服务器。

m_ident.name = IceUtil::generateUUID();

//#千万不能随便打开，callback会按照这个时间超时断开
//#Ice.Override.Timeout=15000


*/
#include "publicfunc.h"
#include "BinaryPayloadCodec.h"
#include "CJsonBinRPCImp.h"
#include "CSubscriptionFilter.h"
#include "vld.h"
#include "UserManage.h"
#include "Log.h"
#include "XmlConfig.h"
#include <algorithm>

extern	CUserManage	g_UserManage;
void WINAPI ServerCallBack(HS p_hHandle,HCLIENT p_hClient,EN_S_NOTIFY_TYPE p_enType,const void *p_pData,int p_iDataLen,const char *p_szIp,unsigned short p_uPort,void * p_pParam);

// 正数参数覆盖客户端 Communicator 的线程池；0 保留 XML 属性或 Ice 默认值。
static void ApplyClientThreadPool(const Ice::PropertiesPtr& p_refProperties,int p_iThreadPool)
{
	if (p_refProperties == nullptr || p_iThreadPool <= 0)
	{
		return;
	}

	const std::string strThreadPool = std::to_string(p_iThreadPool);
	p_refProperties->setProperty("Ice.ThreadPool.Client.Size", strThreadPool);
	p_refProperties->setProperty("Ice.ThreadPool.Client.SizeMax", strThreadPool);
}

// 将旧代码的 char 缓冲区压缩成 Ice AByte，集中处理 Ice 3.8 字节类型差异。
int	CompressAByte(::JSONBINRPC::AByte & p_refOutByte,const char * p_pSrc,long p_lSrcLen)
{
	if (p_pSrc == NULL || p_lSrcLen <= 0)
	{
		p_refOutByte.clear();
		return 0;
	}

	// snappy::Compress 要求输入缓冲不能和输出对象别名，先复制原始输入再改写输出。
	std::string strSrc(p_pSrc, static_cast<size_t>(p_lSrcLen));
	p_refOutByte.clear();
	std::string strDest;
	snappy::Compress(strSrc.data(), strSrc.size(), &strDest);
	if (strDest.empty())
	{
		return 0;
	}

	p_refOutByte.resize(strDest.size());
	memcpy(p_refOutByte.data(), strDest.data(), strDest.size());
	return SafeSizeToLength<int>(strDest.size());
}
// 将 Ice AByte 中的 snappy 数据解压回原始字节，返回解压长度。
int	UnCompressAByte(::JSONBINRPC::AByte & p_refOutByte,const char * p_pSrc,long p_lSrcLen)
{
	if (p_pSrc == NULL || p_lSrcLen <= 0)
	{
		p_refOutByte.clear();
		return 0;
	}

	// 支持输入来自 p_refOutByte.data() 的历史写法，避免 clear 后输入指针失效。
	std::string strSrc(p_pSrc, static_cast<size_t>(p_lSrcLen));
	p_refOutByte.clear();
	std::string strDest;
	if (!snappy::Uncompress(strSrc.data(), strSrc.size(), &strDest) || strDest.empty())
	{
		return 0;
	}

	p_refOutByte.resize(strDest.size());
	memcpy(p_refOutByte.data(), strDest.data(), strDest.size());
	return SafeSizeToLength<int>(strDest.size());
}

// 后台销毁 Ice m_refCommunicator，避免在回调线程中同步 destroy 卡住上层。
DWORD	WINAPI	s_DetroyIceCommunicator(void * p_pParam)
{
	Ice::CommunicatorPtr * ic = (Ice::CommunicatorPtr *)p_pParam;
	try
	{
		if ( *ic )
		{
			(*ic)->shutdown();
			(*ic)->destroy();
		}
	}
	catch(const IceUtil::Exception& ex)
	{
		char	str[1024]={0};
		UTF82ASC(ex.what(),str,1023);
		CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "JSONBINRPCU %m_hSocket,%m_hSocket\r\n",ex.ice_id(),str);//ex.what());
	}
	return 0;
}
CPushMng * CPushMng::m_pThis = NULL;
volatile long long CPushMng::m_lPushPackCrowded=0;
// 全局管理指针
std::mutex			g_clHandleMutex;
std::map<HANDLE,HANDLE>	g_mapHandle;


// 创建或复用全局推送管理器，并增加引用计数。
CPushMng * CPushMng::CreateNewPushObj()
{
	if ( m_pThis==NULL)
	{
		m_pThis = new CPushMng;

		char szBuf[50];
		_snprintf(szBuf, 50, "JSONRPC%u.txt", GetCurDate());
		CIceRPCPushLog::Instance().Open(szBuf);

	}
	::InterlockedIncrement(&m_pThis->m_lRef);
	return m_pThis;
}
// 释放推送管理器引用，最后一个引用负责关闭线程和 socket。
void CPushMng::ReleaseIt()
{
	if ( m_pThis )
	{
		LONG lret = InterlockedDecrement(&m_pThis->m_lRef);
		if ( lret == 0 )
		{
			CPushMng * p = m_pThis;
			m_pThis = NULL;

			delete p;
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "删除推送管理类\r\n");
		}
	}
}



// 初始化推送队列、信号量和后台线程，构造后即可接收推送包。
CPushMng::CPushMng()
{
	m_lRef			= 0;
	m_hSemPush		= NULL;
	m_hPushPack		= NULL;
	m_lPushPackCrowded = 0;
	m_bStop = false;
	m_hP2PThread	= NULL;	// 点到点极速推送，基本4~8微秒
	m_hMulThread	= NULL;	// 分组极速推送，基本10~15微秒
	InitializeCriticalSection(&m_csPush);
	InitializeCriticalSection(&m_csHandle);
	InitializeCriticalSection(&m_csClientPushLock);
	
	m_hPushPack	= NULL;
	m_iUdpThread = 4;
	m_hSemPush	= CreateSemaphore( NULL,0,0x7fffffff,NULL );
	m_hPushPack	= CreateThread(NULL,0,s_PushThread,this,0,NULL);
}

// 停止所有推送线程并清理积压包，退出时优先避免句柄泄漏。
CPushMng::~CPushMng()
{
	m_bStop = true;
	// 集中管理的连接端，客户端，接收推送的线程，每个sock一个线程管理
	std::map<std::string,ST_TCP_PUSH_CONN*>::iterator	t = m_mapPushConn.begin();
	while ( t != m_mapPushConn.end() )
	{
		closesocket(t->second->hSocket);
		if ( t->second->hHandle )
		{
			if ( WaitForSingleObject(t->second->hHandle,3000)!=WAIT_OBJECT_0)
				TerminateThread(t->second->hHandle,1);
			CloseHandle(t->second->hHandle);
			t->second->hHandle = NULL;
		}
		delete t->second;
		t->second = NULL;
		t++;
	}
	m_mapPushConn.clear();
	/////////////////////////
	if ( m_hSemPush )
	{
		ReleaseSemaphore(m_hSemPush,0x7fffffff,NULL);
	}
	if ( m_hPushPack )
	{
		if ( WaitForSingleObject(m_hPushPack,3000)!=WAIT_OBJECT_0 )
			::TerminateThread(m_hPushPack,1111);
		CloseHandle(m_hPushPack);
		m_hPushPack = NULL;
	}
	if ( m_hSemPush )
	{
		CloseHandle( m_hSemPush );
		m_hSemPush = NULL;

	}
	ST_PACK_QUEUE * pack = m_clPackQueue.PopFront();
	while ( pack )
	{
		delete	pack;
		pack = m_clPackQueue.PopFront();
	}
	if ( m_hP2PThread )
	{
		if ( WaitForSingleObject(m_hP2PThread,3000)!=WAIT_OBJECT_0 )
			::TerminateThread(m_hP2PThread,1111);
		CloseHandle(m_hP2PThread);
		m_hP2PThread = NULL;
	}
	if ( m_hMulThread )
	{
		if ( WaitForSingleObject(m_hMulThread,3000)!=WAIT_OBJECT_0 )
			::TerminateThread(m_hMulThread,1111);
		CloseHandle(m_hMulThread);
		m_hMulThread = NULL;
	}

	std::map<int,ST_UDP_INFO*>::iterator	itbind = m_mapBindClient.begin();
	while ( itbind != m_mapBindClient.end() )
	{
		delete itbind->second;
		++itbind;
	}
	m_mapBindClient.clear();
	std::map<unsigned long long,ST_UDP_INFO*>::iterator	itclient = m_mapPushClient.begin();
	while ( itclient != m_mapPushClient.end() )
	{
		delete itclient->second;
		++itclient;
	}
	m_mapPushClient.clear();

	DeleteCriticalSection(&m_csPush);
	DeleteCriticalSection(&m_csHandle);
	DeleteCriticalSection(&m_csClientPushLock);
}
// 把最终推送数据分发给所有已注册回调，先复制回调表再锁外调用，避免回调里注册/注销造成死锁。
void CPushMng::ProcessPackage(long long p_lReqNo,const char * p_pBuf,long p_lBufLen)
{
	if ( m_pThis )
	{
		std::map<func_JsonICEPushClientPack,void*> mapCallback;
		EnterCriticalSection(&m_pThis->m_csPush);
		mapCallback = m_pThis->m_mapPackCallback;
		LeaveCriticalSection(&m_pThis->m_csPush);

		std::map<func_JsonICEPushClientPack,void*>::iterator it = mapCallback.begin();
		while ( it != mapCallback.end() )
		{
			if ( it->first != NULL )
			{
				it->first(p_lReqNo,p_pBuf,p_lBufLen,it->second);
			}
			++it;
		}
	}
}
// 区分推送的通道类型
// 记录并启动客户端快速推送连接，同一地址只创建一个拉取线程。
void CPushMng::PushConnectInfo(int p_iNetType,std::string p_strIp,std::string p_strPort)
{
	if ( m_pThis )
	{
		std::string	szkey, sztype;
		std::vector<std::string>	tip,tport;
		if ( p_iNetType == SOCK_STREAM )
			sztype = "T";
		else if ( p_iNetType == SOCK_RAW )
			sztype = "R";
		else
			sztype = "U";
		TokenizeOR(tip,p_strIp.c_str(),"|");
		TokenizeOR(tport,p_strPort.c_str(),"|");
		int	n = SafeSizeToLength<int>(std::min(tip.size(),tport.size()));
		for ( int i=0;i<n;++i )
		{
			szkey	= sztype + tip[i] + ":" + tport[i];
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "Start Client ICE Push %m_hSocket\r\n", szkey.c_str());
			if (m_pThis->m_mapPushConn.find(szkey.c_str()) == m_pThis->m_mapPushConn.end() )
			{
				m_pThis->m_mapPushConn[szkey] = new ST_TCP_PUSH_CONN;
				m_pThis->m_mapPushConn[szkey]->iNetType = p_iNetType;
				m_pThis->m_mapPushConn[szkey]->strIp = tip[i];
				m_pThis->m_mapPushConn[szkey]->iPort = atol(tport[i].c_str());
				m_pThis->m_mapPushConn[szkey]->hHandle = CreateThread(NULL,2*MAX_CACHE_BUFLEN+MAX_REQ_BUFLEN,s_PeekThread,m_pThis->m_mapPushConn[szkey],0,NULL);
			
			}
		}
	}
}
// 启动点到点推送线程
// 启动 UDP 点对点推送服务端，供行情类数据绕过 Ice 快速下发。
bool CPushMng::StartP2PPush(int p_iThread,int p_iPort)
{
	m_iUdpThread = p_iThread;
	EnterCriticalSection(&m_csClientPushLock);
	if ( m_mapBindClient.find(p_iPort)!=m_mapBindClient.end() )
	{
		LeaveCriticalSection(&m_csClientPushLock);	
		return true;
	}
	LeaveCriticalSection(&m_csClientPushLock);	

	ST_UDP_INFO* u = new ST_UDP_INFO;
	u->lRef	= 1;
	u->iPort	= p_iPort;
	u->iFd	= socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	int		yes = 1 , ret = -1;;
	if (setsockopt(u->iFd,SOL_SOCKET,SO_REUSEADDR,(char *)&yes,sizeof(yes)) < 0)     
	{    
		delete	u;
		return false;    
	}   
	//将socket与制定端口和0.0.0.0绑定
	u->stAddr.sin_family=AF_INET;
	u->stAddr.sin_port=htons(p_iPort);
	u->stAddr.sin_addr.s_addr=htonl(INADDR_ANY);
	ret = ::bind(u->iFd,(SOCKADDR *)&u->stAddr,sizeof(u->stAddr));
	if ( ret != 0 )
	{
		closesocket(u->iFd);
		delete	u;
		CIceRPCPushLog::Instance().WriteLog("RPCX","Start","RPCX Server  bind failed");    
		return false;
	}
	u->pParent	= this;
	u->hHandle		= CreateThread(NULL,0,s_P2PRecv,u,0,NULL);
	//////////////////////////////////////////////////////////////////////////
	EnterCriticalSection(&m_csClientPushLock);
	m_mapBindClient[p_iPort] = u;
	LeaveCriticalSection(&m_csClientPushLock);	
	return true;
}
// 遍历节点，并且发送数据
// 通过已维护的 UDP 客户端表广播数据，失败客户端后续靠心跳清理。
void CPushMng::UDPPush(const char * p_pBuf,long p_lBufLen)
{
	std::map<unsigned long long,ST_UDP_INFO*>	aclient;
	// 改成指针
	EnterCriticalSection(&m_csClientPushLock);
	aclient = m_mapPushClient;
	std::map<unsigned long long,ST_UDP_INFO*>::iterator	it = aclient.begin();
	while ( it != aclient.end() )
	{
		InterlockedIncrement(&it->second->lRef);
		++it;
	}
	LeaveCriticalSection(&m_csClientPushLock);

	if (aclient.size() <= 0)
	{
		//Sleep(1);		// 如果没有客户端，停留下，主要给压力测试，可以删除sleep
		return;
	}

	it = aclient.begin();
	while ( it != aclient.end() )
	{
		if ( !it->second->bDeleted && sendto(it->second->iServerFd,p_pBuf, p_lBufLen, 0, (struct sockaddr *) &it->second->stAddr, sizeof(struct sockaddr_in)) < 0)     
		{    
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "sendto fail:%u ,%I64u\n",GetLastError(),it->first);
			InterlockedDecrement(&it->second->lRef);
			it->second->bDeleted = true;
		}
		++it;
	}
	//////////////////////////////////////////////////////////////////////////
	it = aclient.begin();
	while ( it != aclient.end() )
	{
		InterlockedDecrement(&it->second->lRef);	
		// 删除丢给其他线程
// 		if ( it->second->lRef == 0 )
// 		{
// 			EnterCriticalSection(&m_csClientPushLock);
// 			m_mapPushClient.erase(it->first);
// 			LeaveCriticalSection(&m_csClientPushLock);
// 
// 			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "recv fail , delete it  %I64u \n",it->first);
// 
// 			delete it->second;
// 			it->second = NULL;
// 		}
		++it;
	}
}
// 作为接收推送数据端，启动线程接收
DWORD	WINAPI	CPushMng::s_P2PRecv(void * p_pParam)
{
	ST_UDP_INFO * u = (ST_UDP_INFO*)p_pParam;
	return u->pParent->P2PRecv(u);
}

// UDP 接收线程，维护客户端地址和心跳，支持点对点推送。
DWORD	CPushMng::P2PRecv(ST_UDP_INFO * p_pUdpInfo)
{
	int		nbytes=0;
	char	RecvBuf[1024];	//发送数据的缓冲区
	int		BufLen=1024;	//缓冲区大小
	sockaddr_in SenderAddr;
	int		SenderAddrSize;
	char	szkey[256]={0};
	while ( !m_bStop )
	{
		SenderAddrSize=sizeof(SenderAddr);
		memset(&SenderAddr,0,sizeof(SenderAddr));
		if ((nbytes= recvfrom(p_pUdpInfo->iFd,RecvBuf,BufLen,0,(SOCKADDR *)&SenderAddr,&SenderAddrSize) ) < 0)     
		{
			sprintf(szkey,"%u_%d",SenderAddr.sin_addr.S_un.S_addr,SenderAddr.sin_port);
			unsigned long long p_ulKey = SenderAddr.sin_addr.S_un.S_addr;
			p_ulKey = p_ulKey<< 32;
			p_ulKey +=  SenderAddr.sin_port;
			ST_UDP_INFO * p  = NULL;
			EnterCriticalSection(&m_csClientPushLock);
			if ( m_mapPushClient.find(p_ulKey) != m_mapPushClient.end() )
			{
				ST_UDP_INFO * p = m_mapPushClient[p_ulKey];
				p->bDeleted = true;
				InterlockedDecrement(&m_mapPushClient[p_ulKey]->lRef);
				if (m_mapPushClient[p_ulKey]->lRef <= 0 )
				{
					m_mapPushClient.erase(p_ulKey);
					CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "recv fail , delete it  %m_hSocket \n",szkey);
					// delete p;
				}
			}
			LeaveCriticalSection(&m_csClientPushLock);
			if ( p && p->lRef<=0 )
			{
				delete p;
			}

			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "recv error : %u,%m_hSocket \r\n",GetLastError(),szkey);
			Sleep(1);
			continue;
		} 
		//else
		//	printf("recv \n");
		sprintf(szkey, "%u_%d", SenderAddr.sin_addr.S_un.S_addr, SenderAddr.sin_port);
		int		nflag = 0;
		unsigned long long p_ulKey = SenderAddr.sin_addr.S_un.S_addr;
		p_ulKey = p_ulKey<< 32;
		p_ulKey +=  SenderAddr.sin_port;
		EnterCriticalSection(&m_csClientPushLock);
		if ( m_mapPushClient.find(p_ulKey) != m_mapPushClient.end() ) 
		{
			nflag = 1;
		}
		LeaveCriticalSection(&m_csClientPushLock);
		if ( !nflag )
		{
			ST_UDP_INFO	* client = new ST_UDP_INFO;
			client->lRef		= 1;
			client->stAddr	= SenderAddr;
			client->iServerFd	= p_pUdpInfo->iFd;

			EnterCriticalSection(&m_csClientPushLock);
			m_mapPushClient[p_ulKey] = client;
			LeaveCriticalSection(&m_csClientPushLock);
		}
	}
	return 0;
}
// 订阅的类型信息
// 通过 TCP 快速通道发送订阅/退订请求，保持服务端订阅状态同步。
void CPushMng::PushRegisterInfo(const char * p_szRegSubInfo,int p_iIsReg)
{
	if ( m_pThis )
	{
		int		i;
		DWORD	dwV=0;
		const	char * pstr = NULL;
		std::vector<std::string>	tosecs;
		TokenizeOR(tosecs,p_szRegSubInfo,"|");
		// 增加订阅的类型
		EnterCriticalSection(&m_pThis->m_csHandle);
		// 因为很多是guid，从第二个开始 |257 for ( i=0;i<tosecs.size();++i )
		for ( i=1;i<tosecs.size();++i )
		{
			if ( tosecs[i].size()>0 && isdigit(tosecs[i][0]) )	// 必须数字开头
			{
				dwV = atol(tosecs[i].c_str());
				m_pThis->m_mapRegSubInfo[ dwV ]	= p_iIsReg;		// 订阅
			}
		}
		LeaveCriticalSection(&m_pThis->m_csHandle);	
		CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "PushReg=%m_hSocket,%d\r\n",p_szRegSubInfo,p_iIsReg);
	}
}

DWORD	WINAPI CPushMng::s_PeekThread(void * p_pParam)
{
	return m_pThis->PeekThread((ST_TCP_PUSH_CONN*)p_pParam);
}
// 订阅缓冲区不能超过1M
// 填充旧 TCP 订阅包头，字段顺序必须与 userdata.hHandle 中协议头一致。
void  CPushMng::SetSubReqBuf(ST_REQ_HEADER * p_pReq)
{
	ST_SUB_UNSUB_STRING* req2 = (ST_SUB_UNSUB_STRING*)(p_pReq + 1);
	char * p_pBuf = req2->pBuf;
	int	 n = 0;
	EnterCriticalSection(&m_csHandle);
	std::map<DWORD,int>::const_iterator  it = m_mapRegSubInfo.begin();
	while ( it != m_mapRegSubInfo.end() )
	{
		if ( it->second	)
			n += sprintf(p_pBuf+n,"|%d",it->first);
		else
			n += sprintf(p_pBuf+n,"|~%d",it->first);
		++it;
	}
	req2->lLen = SafeSizeToLength<long>(strlen(p_pBuf) + 1);
	LeaveCriticalSection(&m_csHandle);	
	p_pReq->stInfo.chVersion = 2;
	p_pReq->dwPacketLen = sizeof(ST_SUB_UNSUB_STRING)+req2->lLen;
	p_pReq->dwRawLen = sizeof(ST_SUB_UNSUB_STRING)+req2->lLen;
	req2->req = PACKET_PUSH_HQ_SUB;
	CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "发起订阅:%m_hSocket,%d\r\n",req2->pBuf,req2->lLen);
}
// 从new改成栈，大内存，速度会提升100倍
// 客户端 TCP/UDP/组播拉取线程，解析包头后交给统一推送分发。
DWORD CPushMng::PeekThread(ST_TCP_PUSH_CONN * p_pConn)
{
	char	  p_pBuf[MAX_CACHE_BUFLEN];
	//char	* subbuf = new char[MAX_REQ_BUFLEN+1024];
	char	  subbuf[MAX_REQ_BUFLEN+1024];
	const int per_rcvlen = 1024*1024*4;
	int		  rcvoffset=0;
	char	  rcvbuf[per_rcvlen];
	//std::string		srcvbuf;
	//srcvbuf.resize(per_rcvlen);
	time_t	lastsend = time(NULL);	// 服务端发现客户端没请求，虽然推送，但是还是会导致超时断开
	int		lasterror = 0;
	memset(subbuf,0,MAX_REQ_BUFLEN);
	ST_REQ_HEADER *	req = (ST_REQ_HEADER*)subbuf;
	ST_SUB_UNSUB_STRING* req2 = (ST_SUB_UNSUB_STRING*)(req + 1);
	SetSubReqBuf(req);
	int	nSendLen = sizeof(ST_REQ_HEADER)+sizeof(ST_SUB_UNSUB_STRING)+req2->lLen;
	if ( p_pConn->iNetType == SOCK_DGRAM )
	{
		char	*udpbuf = rcvbuf;//[1024*64];
		//std::string	tmpbuf;
		//tmpbuf.resize(64*1024);
		//char * udpbuf = rcvbuf;//srcvbuf._Myptr();
		int		udplen = 1024*64;
		int		SenderAddrSize,nbytes,m_dwPacketLen;
		if ( p_pConn->hSocket != INVALID_SOCKET )
		{
			closesocket(p_pConn->hSocket);
			p_pConn->hSocket = INVALID_SOCKET;
		}
		//创建Socket对象
		p_pConn->hSocket=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
		int timeout = 15 * 1000;	// 转码机30秒发送一次时间状态信息
		setsockopt(p_pConn->hSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
		setsockopt(p_pConn->hSocket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

		sockaddr_in si;
		memset(&si,0,sizeof(si));
		si.sin_addr.S_un.S_addr=inet_addr(p_pConn->strIp.c_str());
		si.sin_family=AF_INET;
		si.sin_port=htons(p_pConn->iPort);

		memset(subbuf,0,MAX_REQ_BUFLEN);
		SetSubReqBuf(req);
		int	nSendLen = sizeof(ST_REQ_HEADER)+sizeof(ST_SUB_UNSUB_STRING)+req2->lLen;
		if (sendto(p_pConn->hSocket,subbuf, nSendLen, 0, (struct sockaddr *) &si, sizeof(si)) < 0)     
		{    
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "sendto error: %u , %m_hSocket \n",GetLastError(),p_pConn->strIp.c_str());    
			//closesocket(p_pConn->hSocket);
			//p_pConn->hSocket = INVALID_SOCKET;  
			Sleep(1);
			//continue;
		}    
		while (!m_bStop)    
		{    
			SenderAddrSize = sizeof(si);
			if ((nbytes= recvfrom(p_pConn->hSocket,udpbuf,udplen,0,(SOCKADDR *)&si,&SenderAddrSize) ) < 0)
			{    
				DWORD	dwErr = GetLastError();
				if (WSAEWOULDBLOCK == dwErr || WSAEINPROGRESS == dwErr ||
					EINPROGRESS == dwErr ||
					WSAETIMEDOUT == dwErr ||
					// ERROR_SUCCESS == dwErr||		// 这个情况？？？？？
					EAGAIN == errno || EINTR == errno || EWOULDBLOCK == errno || ENOENT == errno)
					// if ( WSAETIMEDOUT == dwErr || WSAEWOULDBLOCK == dwErr )
				{
					if (m_bStop)	break;
					memset(subbuf, 0, MAX_REQ_BUFLEN);
					SetSubReqBuf(req);
					int	nSendLen = sizeof(ST_REQ_HEADER) + sizeof(ST_SUB_UNSUB_STRING) + req2->lLen;
					sendto(p_pConn->hSocket, subbuf, nSendLen, 0, (struct sockaddr *) &si, sizeof(si));
					lastsend = time(NULL);
					CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "等待超时:%m_hSocket,%d,%u  [%u] \r\n", p_pConn->strIp.c_str(), p_pConn->iPort, dwErr, GetCurrentThreadId());
					Sleep(1);   
					continue;
				}
				CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "recvfrom %u \r\n",GetLastError());
				//closesocket(p_pConn->hSocket);
				//p_pConn->hSocket = INVALID_SOCKET;  
				Sleep(1);    
				//break;
				continue;
			} 
			ST_UDP_HEADER * pi = (ST_UDP_HEADER*)udpbuf;
			m_dwPacketLen = nbytes - sizeof(ST_UDP_HEADER);
			//解压缩
			if (!pi->chCompressed)
			{
				//CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "Push %d,%d :%m_hSocket,%d [%u] \r\n",pi->req,pi->dwPacketLen,p_pConn->strIp.c_str(),p_pConn->iPort,GetCurrentThreadId());

				CPushMng::ProcessPackage(pi->req,(char*)(pi + 1),m_dwPacketLen);
			}
			else
			{
				// 推送只能用snappy
				//std::string dest;
				//size_t size = snappy::Uncompress((const char*)(pi + 1),m_dwPacketLen,&dest);

				//CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "Push %d,%d :%m_hSocket,%d [%u] \r\n",pi->req,dest.size(),p_pConn->strIp.c_str(),p_pConn->iPort,GetCurrentThreadId());
				//CPushMng::ProcessPackage(pi->req,dest.data(),SafeSizeToLength<long>(dest.size()));
				size_t ulength;
				if (!snappy::GetUncompressedLength((const char*)(pi+1), m_dwPacketLen, &ulength)) 
				{
					continue;
				}
				bool bok = snappy::RawUncompress((const char*)(pi+1),m_dwPacketLen,p_pBuf);
				CPushMng::ProcessPackage(pi->req,p_pBuf,SafeSizeToLength<long>(ulength));

// 				uLongf nUnCompressLen = sizeof(p_pBuf);
// 				int nRet = uncompress((BYTE*)p_pBuf,&nUnCompressLen,(unsigned char*)(pi+1),m_dwPacketLen);
// 				if (nRet == Z_OK)
// 				{
// 					CPushMng::ProcessPackage(pi->req,p_pBuf,nUnCompressLen);
// 				}
			}
			if ( time(NULL) - lastsend > 30 )
			{
				lastsend = time(NULL);
				memset(subbuf,0,MAX_REQ_BUFLEN);
				SetSubReqBuf(req);
				int	nSendLen = sizeof(ST_REQ_HEADER)+sizeof(ST_SUB_UNSUB_STRING)+req2->lLen;
				if ( !Sendn(p_pConn->hSocket,subbuf,nSendLen,&lasterror) )
				{
					CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "Sendn Error %u \r\n",GetLastError());
					//break;
					//Sleep(1);   
					continue;
				}
			}
		}    
	}
	else if ( SOCK_RAW == p_pConn->iNetType  )
	{
		int addrlen,nbytes,m_dwPacketLen;
		struct ip_mreq mreq;  
		if ( p_pConn->hSocket != INVALID_SOCKET )
		{
			closesocket(p_pConn->hSocket);
			p_pConn->hSocket = INVALID_SOCKET;
		}
		//创建Socket对象
		p_pConn->hSocket=socket(AF_INET,SOCK_DGRAM,0);
		/**** MODIFICATION TO ORIGINAL */    
		/* allow multiple sockets to use the same PORT number */   
		int	yes = 1;
		if (setsockopt(p_pConn->hSocket,SOL_SOCKET,SO_REUSEADDR,(char *)&yes,sizeof(yes)) < 0)     
		{    
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "Reusing ADDR failed");    
		}   
		/*** END OF MODIFICATION TO ORIGINAL */    
		/* set up destination address */    
		struct sockaddr_in m_stAddr;
		memset(&m_stAddr,0,sizeof(m_stAddr));    
		m_stAddr.sin_family=AF_INET;    
		m_stAddr.sin_addr.s_addr=htonl(INADDR_ANY); /* N.B.: differs from sender */    
		m_stAddr.sin_port=htons(p_pConn->iPort);    
		/* bind to receive address */    
		if (::bind(p_pConn->hSocket,(struct sockaddr *) &m_stAddr,sizeof(m_stAddr)) < 0)    
		{    
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "bind Error");    
		}    

		/* use setsockopt() to request that the kernel join a multicast group */    
		mreq.imr_multiaddr.s_addr=inet_addr(p_pConn->strIp.c_str());    
		mreq.imr_interface.s_addr=htonl(INADDR_ANY);    
		if (setsockopt(p_pConn->hSocket,IPPROTO_IP,IP_ADD_MEMBERSHIP,(char *)&mreq,sizeof(mreq)) < 0)     
		{    
			int err=GetLastError();
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "setsockopt:%d",err);    
		}    

		while (!m_bStop)    
		{    
			addrlen=sizeof(m_stAddr);  
			//printf("Receiving..."); 
			//if((nbytes=recvfrom(m_iFd,msgbuf,MSGBUFSIZE,0,NULL,NULL))<0)
			if ((nbytes=recvfrom(p_pConn->hSocket, rcvbuf,per_rcvlen, 0, (struct sockaddr *) &m_stAddr, (int *)&addrlen)) < 0)     
			{    
				CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "recvfrom");    
			} 
			ST_UDP_HEADER * pi = (ST_UDP_HEADER*)rcvbuf;
			m_dwPacketLen = nbytes - sizeof(ST_UDP_HEADER);
			//解压缩
			if (!pi->chCompressed)
			{
				//CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "Push %d,%d :%m_hSocket,%d [%u] \r\n",pi->req,pi->dwPacketLen,p_pConn->strIp.c_str(),p_pConn->iPort,GetCurrentThreadId());

				CPushMng::ProcessPackage(pi->req,(char*)(pi + 1),m_dwPacketLen);
			}
			else
			{
				// 推送只能用snappy
				std::string dest;
				size_t size = snappy::Uncompress((const char*)(pi + 1),m_dwPacketLen,&dest);

				//CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "Push %d,%d :%m_hSocket,%d [%u] \r\n",pi->req,dest.size(),p_pConn->strIp.c_str(),p_pConn->iPort,GetCurrentThreadId());
				CPushMng::ProcessPackage(pi->req,dest.data(),SafeSizeToLength<long>(dest.size()));
			}
		}    
	}
	else   // TCP 推送接收
	{
		do
		{
			rcvoffset=0;
			if ( p_pConn->hSocket != INVALID_SOCKET )
			{
				closesocket(p_pConn->hSocket);
				p_pConn->hSocket = INVALID_SOCKET;
			}
			//创建连接
			p_pConn->hSocket=socket(AF_INET,SOCK_STREAM,0);
			if(p_pConn->hSocket==SOCKET_ERROR)
			{
				CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "socket创建失败 %m_hSocket:%d\r\n",p_pConn->strIp.c_str(),p_pConn->iPort);
				CIceRPCPushLog::Instance().WriteLog("JSONRPC","PEEKTHREAD","socket创建失败 %m_hSocket:%d\r\n",p_pConn->strIp.c_str(),p_pConn->iPort);
				if ( m_bStop )	break;
				Sleep(3000);
				continue;
			}
			sockaddr_in si;
			memset(&si,0,sizeof(si));
			si.sin_addr.S_un.S_addr=inet_addr(p_pConn->strIp.c_str());
			si.sin_family=AF_INET;
			si.sin_port=htons(p_pConn->iPort);

			// int ret = Connect(p_pConn->hSocket,(sockaddr*)&si,sizeof(si),6);
			int ret = connect(p_pConn->hSocket,(sockaddr*)&si,sizeof(si));
			if( ret == SOCKET_ERROR )
			{
				closesocket(p_pConn->hSocket);		// 忘记关闭socket，导致服务器socket全部失效
				p_pConn->hSocket = INVALID_SOCKET;
				if ( m_bStop )	break;
				DWORD	dwErr = GetLastError();
				CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "连接服务器超时:%m_hSocket,%d,%u [%u] \r\n",p_pConn->strIp.c_str(),p_pConn->iPort,dwErr,GetCurrentThreadId());
				CIceRPCPushLog::Instance().WriteLog("JSONRPC","PEEKTHREAD","连接服务器超时:%m_hSocket,%d,%u [%u] \r\n",p_pConn->strIp.c_str(),p_pConn->iPort,dwErr,GetCurrentThreadId());
				Sleep(3000);
				continue;
			}
			BOOL bNoDelay = TRUE;
			setsockopt(p_pConn->hSocket,IPPROTO_TCP,TCP_NODELAY,(const char *)&bNoDelay,sizeof(BOOL));

			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "连接服务器成功:%m_hSocket,%d [%u] \r\n",p_pConn->strIp.c_str(),p_pConn->iPort,GetCurrentThreadId());
			CIceRPCPushLog::Instance().WriteLog("JSONRPC","PEEKTHREAD","连接服务器成功:%m_hSocket,%d [%u] \r\n",p_pConn->strIp.c_str(),p_pConn->iPort,GetCurrentThreadId());
			p_pConn->bOpenTcpOk = true;
			//激活tcp协议保活包
			DWORD dwRet = 0;
			lastsend = time(NULL);	// 服务端发现客户端没请求，虽然推送，但是还是会导致超时断开
			lasterror = 0;
			tcp_keepalive keepalive = {1ul,10 * 1000ul,1 * 1000ul};
			WSAIoctl(p_pConn->hSocket,SIO_KEEPALIVE_VALS,&keepalive,sizeof(keepalive),NULL,0,&dwRet,NULL,NULL);
			int timeout = 15 * 1000;	// 转码机30秒发送一次时间状态信息
			setsockopt(p_pConn->hSocket,SOL_SOCKET,SO_RCVTIMEO,(char*)&timeout,sizeof(timeout));
			setsockopt(p_pConn->hSocket,SOL_SOCKET,SO_SNDTIMEO,(char*)&timeout,sizeof(timeout));
			//////////////////////////////////////////////////////////////////////////
			memset(subbuf,0,MAX_REQ_BUFLEN);
			SetSubReqBuf(req);
			int	nSendLen = sizeof(ST_REQ_HEADER)+sizeof(ST_SUB_UNSUB_STRING)+req2->lLen;
			Sendn(p_pConn->hSocket,subbuf,nSendLen,&lasterror);
			while (!m_bStop )
			{
				//CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "准备recv:%m_hSocket,%d [%u] \r\n",p_pConn->strIp.c_str(),p_pConn->iPort,GetCurrentThreadId());
				// ST_ANS_HEADER head = {0};
				int nread = recv(p_pConn->hSocket,rcvbuf,per_rcvlen,0);
				//CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "准备recv=%d :%m_hSocket,%d [%u] \r\n",nread,p_pConn->strIp.c_str(),p_pConn->iPort,GetCurrentThreadId());
				if ( nread <= 0 )
				{
					DWORD	dwErr = GetLastError();
					if (  WSAEWOULDBLOCK == dwErr || WSAEINPROGRESS == dwErr || 
						EINPROGRESS == dwErr || 
						WSAETIMEDOUT == dwErr || 
						// ERROR_SUCCESS == dwErr||		// 这个情况？？？？？
						EAGAIN==errno || EINTR==errno || EWOULDBLOCK==errno || ENOENT==errno )
					// if ( WSAETIMEDOUT == dwErr || WSAEWOULDBLOCK == dwErr )
					{
					if ( m_bStop )
					{
						closesocket(p_pConn->hSocket);	// 忘记关闭socket，导致服务器socket全部失效
						p_pConn->hSocket = INVALID_SOCKET;
						break;
					}
						memset(subbuf,0,MAX_REQ_BUFLEN);
						SetSubReqBuf(req);
						int	nSendLen = sizeof(ST_REQ_HEADER)+sizeof(ST_SUB_UNSUB_STRING)+req2->lLen;
						if( !Sendn(p_pConn->hSocket,subbuf,nSendLen,&lasterror) )
						{
							closesocket(p_pConn->hSocket);
							p_pConn->hSocket = INVALID_SOCKET;
							break;
						}
						lastsend = time(NULL);
						CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "等待超时:%m_hSocket,%d,%u  [%u] \r\n",p_pConn->strIp.c_str(),p_pConn->iPort,dwErr,GetCurrentThreadId());
						continue;
					}
					CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "读取数据失败=%d:%m_hSocket,%d,%u  [%u] \r\n",nread,p_pConn->strIp.c_str(),p_pConn->iPort,dwErr,GetCurrentThreadId());
					CIceRPCPushLog::Instance().WriteLog("JSONRPC","PEEKTHREAD","读取数据失败=%d:%m_hSocket,%d,%u  [%u] \r\n",nread,p_pConn->strIp.c_str(),p_pConn->iPort,dwErr,GetCurrentThreadId());
					closesocket(p_pConn->hSocket);
					p_pConn->hSocket = INVALID_SOCKET;
					break;
				}
				//防止打穿缓冲区
				if ( rcvoffset+nread >= MAX_CACHE_BUFLEN )
				{
					CIceRPCPushLog::Instance().WriteLog("JSONRPC","PEEKTHREAD","防止打穿缓冲区=%d > MAXCAHE=%d [%u]\r\n",rcvoffset+nread,MAX_CACHE_BUFLEN,GetCurrentThreadId());
					closesocket(p_pConn->hSocket);
					p_pConn->hSocket = INVALID_SOCKET;
					break;
				}
				//保存最新数据
				memcpy(p_pBuf+rcvoffset,rcvbuf,nread);
				rcvoffset += nread;
				while (	!m_bStop)
				{
					if ( rcvoffset < sizeof(ST_ANS_HEADER) )
					{
						break;
					}
					ST_ANS_HEADER * pi = (ST_ANS_HEADER*)p_pBuf;
					if( pi->lSeparator != SEPARATOR_NUM )	// 包头不对
					{
						CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "包头解析错误，主动断开连接，重新连接\r\n");
						CIceRPCPushLog::Instance().WriteLog("JSONRPC","PEEKTHREAD","包头解析错误，主动断开连接，重新连接\r\n");
						closesocket(p_pConn->hSocket);
						p_pConn->hSocket = INVALID_SOCKET;
						break;
					}
					//判断内容是否已经收完,没有则继续收
					if( rcvoffset  <  pi->dwPacketLen+sizeof(ST_ANS_HEADER))
					{
						break;//不够一个包,退出循环继续收
					}
					// 处理协议,跳过空包
					if ( pi->dwPacketLen > 0 )
					{
						//CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "%d,%d,zip=%d:%m_hSocket,%d [%u] \r\n",pi->req,pi->dwPacketLen,pi->stInfo.chCompressed,p_pConn->strIp.c_str(),p_pConn->iPort,GetCurrentThreadId());
						//解压缩
						if (!pi->stInfo.chCompressed)
						{
							//CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "Push %d,%d :%m_hSocket,%d [%u] \r\n",pi->req,pi->dwPacketLen,p_pConn->strIp.c_str(),p_pConn->iPort,GetCurrentThreadId());

							CPushMng::ProcessPackage(pi->req,(char*)(pi + 1),pi->dwPacketLen);
						}
						else
						{
							// 推送只能用snappy
							std::string dest;
							size_t size = snappy::Uncompress((const char*)(pi + 1),pi->dwPacketLen,&dest);

							//CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "Push %d,%d :%m_hSocket,%d [%u] \r\n",pi->req,dest.size(),p_pConn->strIp.c_str(),p_pConn->iPort,GetCurrentThreadId());
							CPushMng::ProcessPackage(pi->req,dest.data(),SafeSizeToLength<long>(dest.size()));
						}
					}

					int left = rcvoffset - (pi->dwPacketLen+sizeof(ST_ANS_HEADER));
					if ( left > 0 )
					{
						memmove(p_pBuf,p_pBuf+(pi->dwPacketLen+sizeof(ST_ANS_HEADER)),left);
					}
					rcvoffset=left;

					if ( time(NULL) - lastsend > 10 )
					{
						memset(subbuf,0,MAX_REQ_BUFLEN);
						SetSubReqBuf(req);
						int	nSendLen = sizeof(ST_REQ_HEADER)+sizeof(ST_SUB_UNSUB_STRING)+req2->lLen;
						if ( !Sendn(p_pConn->hSocket,subbuf,nSendLen,&lasterror) )
						{
							break;
						}
						lastsend = time(NULL);
					}

				}
			}
		}while(!m_bStop);
	}
	//delete [] p_pBuf;
	//delete [] rcvbuf;
	//delete [] subbuf;
	CIceRPCPushLog::Instance().WriteLog("JSONRPC","PEEKTHREAD","Peek接收推送线程退出\r\n");
	return 0;
}
// 维护需要 Ice 主动推送的 HANDLE 集合，删除时避免后台线程继续访问。
void CPushMng::PushHandle(HANDLE p_hHandle,bool p_bIsDel)
{
	if ( m_pThis )
	{
		EnterCriticalSection(&m_pThis->m_csHandle);
		if ( p_bIsDel )
		{
			m_pThis->m_mapHandle.erase(p_hHandle);
		}
		else
		{
			m_pThis->m_mapHandle[p_hHandle] = p_hHandle;
		}
		LeaveCriticalSection(&m_pThis->m_csHandle);	
	}
}

// Ice 回调收到的推送包入队，队列长度作为积压限流指标。
void CPushMng::PushPack(ST_PACK_QUEUE* p_pPack)
{
	if ( m_pThis )
	{
		m_pThis->m_clPackQueue.PushBack(p_pPack);
		ReleaseSemaphore(m_pThis->m_hSemPush,1,NULL);
	}	
	else
	{
		delete	p_pPack;
		CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "推送空接收..........................\r\n");
	}
}

DWORD	WINAPI CPushMng::s_PushThread(void * p_pParam)
{
	return ((CPushMng*)p_pParam)->PushThread();
}

// 推送队列消费线程，负责校验和解压逐包载荷并广播原始二进制给本地回调。
DWORD CPushMng::PushThread()
{
	time_t	lastT = time(NULL);
	while(!m_bStop)
	{
		if ( time(NULL) - lastT > 10 )
		{
			EnterCriticalSection(&m_csHandle);
			std::map<HANDLE,HANDLE>::iterator it = m_mapHandle.begin();
			while ( it!=m_mapHandle.end())
			{
				((JSONBINRPC::CJsonBinRPCImp*)it->second)->TTLive();
				++it;
			}
			LeaveCriticalSection(&m_csHandle);	
			lastT = time(NULL);
		}
		WaitForSingleObject(m_hSemPush,6000);
		ST_PACK_QUEUE * pack = m_clPackQueue.PopFront();
		while ( pack )
		{
			std::vector<unsigned char> vecRaw;
			std::string strError;
			if (binarypayload::Decode(pack->stPayload,
				pack->iProtocolVersion, pack->iMaxPayloadBytes,
				vecRaw, strError))
			{
				std::map<func_JsonICEPushClientPack,void*> mapCallback;
				EnterCriticalSection(&m_csPush);
				mapCallback = m_mapPackCallback;
				LeaveCriticalSection(&m_csPush);
				const char* pRaw = vecRaw.empty() ? NULL :
					reinterpret_cast<const char*>(vecRaw.data());
				std::map<func_JsonICEPushClientPack,void*>::iterator it = mapCallback.begin();
				while ( it != mapCallback.end() )
				{
					if ( it->first != NULL )
					{
						it->first(pack->lReqNo, pRaw,
							SafeSizeToLength<long>(vecRaw.size()), it->second);
					}
					++it;
				}
			}
			else
			{
				CIceRPCPushLog::Instance().WriteLog("JSONRPC", "PUSHPACK",
					"BINARY_PUSH_DECODE_FAILED: req=%I64d, detail=%s\r\n",
					pack->lReqNo, strError.c_str());
			}
			delete	pack;
			if ( m_bStop )
				break;
			pack = m_clPackQueue.PopFront();
		}
	}
	return 0;
}

// 注册或移除本地推送回调，p_pParam 与函数指针成对保存。
void CPushMng::UpdateCallBack(func_JsonICEPushClientPack p_pfnCallback,void * p_pParam,bool p_bIsDel)
{
	if ( m_pThis )
	{
		EnterCriticalSection(&m_pThis->m_csPush);
		if ( p_bIsDel )
		{
			m_pThis->m_mapPackCallback.erase(p_pfnCallback);
		}
		else
		{
			m_pThis->m_mapPackCallback[p_pfnCallback] = p_pParam;
		}
		LeaveCriticalSection(&m_pThis->m_csPush);
	}
}



namespace JSONBINRPC
{
	namespace
	{
		const int s_iBinaryProtocolVersion = 1;
		const int s_iBinaryProtocolError = -20008;
		const int s_iBinaryPayloadTooLarge = -20009;
		const int s_iBinaryCompressionError = -20010;
		const int s_iBinaryQueueFull = -20011;
		const int s_iBinaryCallTimeout = -20012;
		const int s_iBinaryPendingFull = -20013;
		const int s_iBinaryCallbackError = -20014;
		const int s_iBinaryStateError = -20003;

		// 编码结果直接拥有 Ice 生成的两个字节容器，公共结构只暴露只读视图。
		struct ST_BINARY_ENCODED_RESULT_OWNER
		{
			::JSONBINRPC::AByte aPayload; // 主 BinaryPayload 编码正文。
			::JSONBINRPC::AByte aExtra;   // 扩展 BinaryPayload 编码正文。
		};

		void FillBinaryEncodedBuffer(
			const ::JSONBINRPC::BinaryPayload& p_refPayload,
			const ::JSONBINRPC::AByte& p_refData, int p_iMaxPayloadBytes,
			ST_BINARY_ENCODED_BUFFER& p_refBuffer)
		{
			p_refBuffer.iVersion = p_refPayload.version;
			p_refBuffer.iCompression =
				static_cast<int>(p_refPayload.compression);
			p_refBuffer.iRawSize = p_refPayload.rawSize;
			p_refBuffer.iWireSize = SafeSizeToLength<int>(p_refData.size());
			p_refBuffer.iMaxPayloadBytes = p_iMaxPayloadBytes;
			p_refBuffer.pBuffer = p_refData.empty() ? nullptr :
				reinterpret_cast<const unsigned char*>(p_refData.data());
		}

		// 把本层协议校验详情转换为稳定错误码，供同步和异步提交路径共用。
		int InferBinaryProtocolErrorCode(const std::string& p_strError)
		{
			if (p_strError.rfind("BINARY_PAYLOAD_TOO_LARGE", 0) == 0)
			{
				return s_iBinaryPayloadTooLarge;
			}
			if (p_strError.rfind("BINARY_SNAPPY_", 0) == 0 || p_strError.rfind("BINARY_COMPRESSION_", 0) == 0)
			{
				return s_iBinaryCompressionError;
			}
			return s_iBinaryProtocolError;
		}

		// 为异步异常构造稳定的英文描述，避免把异常对象生命周期传出回调。
		std::string MakeBinaryExceptionDetail(const char* p_szFunction, std::exception_ptr p_refException)
		{
			std::string strDetail = "BINARY_REMOTE_EXCEPTION: func=";
			strDetail += p_szFunction != NULL ? p_szFunction : "unknown";
			try
			{
				if (p_refException)
				{
					std::rethrow_exception(p_refException);
				}
			}
			catch (const IceUtil::Exception& ex)
			{
				strDetail += ", ice=";
				strDetail += ex.ice_id();
				strDetail += ", what=";
				strDetail += ex.what();
			}
			catch (const std::exception& ex)
			{
				strDetail += ", what=";
				strDetail += ex.what();
			}
			catch (...)
			{
				strDetail += ", what=unknown exception";
			}
			return strDetail;
		}
	}

	// 单个服务端 Binary AMD 请求；工作线程拥有对象并在发送应答后释放。
	struct CJsonBinRPCImp::ST_BINARY_SERVER_TASK
	{
		CJsonBinRPCImp* pOwner;                                           // 处理该任务的服务实例，不拥有其生命周期。
		ST_BINARY_RESPONSE_CONTEXT* pResponseContext;                      // 直接绑定的 DLL 内部引用计数应答上下文。
		HANDLE hResponse;                                                  // 仅向 C ABI 暴露的不透明上下文指针。
		bool bPut;                                                        // true 表示 PUT，false 表示 RPC。
		bool bCompleted;                                                  // 业务已经提交应答内容。
		bool bCallbackReturned;                                           // 服务回调已经返回 disposition。
		bool bDeferred;                                                   // 回调选择延迟应答。
		bool bResponseQueued;                                             // 完整结果已经领取应答队列所有权。
		bool bOwnedResult;                                                // true 表示结果仍是未压缩的自有缓冲。
		int iProtocolVersion;                                             // 创建任务时冻结的协议版本。
		int iCompressionThresholdBytes;                                   // 创建任务时冻结的压缩阈值。
		int iMaxPayloadBytes;                                             // 创建任务时冻结的载荷上限。
		std::mutex clMutex;                                               // 保护完成、延迟和发送状态。
		::JSONBINRPC::BinaryRequest stRequest;                            // Ice 解码后的传输请求。
		::JSONBINRPC::BinaryResponse stResponse;                          // 业务完成后待发送的传输应答。
		std::vector<unsigned char> vecPayload;                            // 解压后的主请求载荷。
		std::vector<unsigned char> vecExtra;                              // 解压后的扩展请求载荷。
		std::vector<unsigned char> vecResultPayload;                      // 业务返回的未压缩主载荷自有副本。
		std::vector<unsigned char> vecResultExtra;                        // 业务返回的未压缩扩展载荷自有副本。
		long long lResultRetVal;                                          // 业务主返回值。
		int iResultErrorCode;                                             // 业务或网络层错误码。
		long long lResultParam;                                           // 第一业务返回参数。
		long long lResultWParam;                                          // 第二业务返回参数。
		std::string strResultError;                                       // 不包含大正文的英文错误描述。
		std::function<void(const ::JSONBINRPC::BinaryResponse&)> fnResponse; // Ice AMD 成功回调。
		std::function<void(std::exception_ptr)> fnException;              // Ice AMD 异常回调。

		ST_BINARY_SERVER_TASK()
		{
			pOwner = NULL;
			pResponseContext = NULL;
			hResponse = NULL;
			bPut = false;
			bCompleted = false;
			bCallbackReturned = false;
			bDeferred = false;
			bResponseQueued = false;
			bOwnedResult = false;
			iProtocolVersion = s_iBinaryProtocolVersion;
			iCompressionThresholdBytes = 1024;
			iMaxPayloadBytes = 50 * 1024 * 1024;
			lResultRetVal = 0;
			iResultErrorCode = 0;
			lResultParam = 0;
			lResultWParam = 0;
		}
	};

	// 不透明上下文只在 DLL 内保存 shared_ptr；C ABI 和 CloudNet 仅透传裸指针。
	struct CJsonBinRPCImp::ST_BINARY_RESPONSE_CONTEXT
	{
		std::atomic<long> lReferences;                                    // 原始句柄、应答队列或回收队列持有的引用数。
		CJsonBinRPCImp* pOwner;                                           // 创建上下文的服务实例，停机前必须完成收敛。
		std::shared_ptr<ST_BINARY_SERVER_TASK> refTask;                   // 保证异步应答和回收期间任务仍然有效。

		explicit ST_BINARY_RESPONSE_CONTEXT(
			const std::shared_ptr<ST_BINARY_SERVER_TASK>& p_refTask)
			: lReferences(1)
			, pOwner(p_refTask ? p_refTask->pOwner : NULL)
			, refTask(p_refTask)
		{
		}
	};
// 注册推送的 Ice 异步回调，服务端返回压缩模式后更新父对象状态。
	class CRegisterIOCallBack : public IceUtil::Shared
	{
	public:
		void ice_response(::Ice::Long p_lRetVal)
		{
			std::lock_guard<std::mutex> clLock(g_clHandleMutex);
			if ( g_mapHandle.find(m_lpParent) != g_mapHandle.end() )
			{
				m_lpParent->RegisterCallBack(p_lRetVal);
			}
		}
		void exception(const Ice::Exception& p_refException)
		{

		}
		void sent(bool p_bSent)
		{
		}
		CJsonBinRPCImp	*	m_lpParent;	
	};
	typedef IceUtil::Handle<CRegisterIOCallBack> CRegisterIOCallBackPtr;
	// 调用推送，异步回调的类(结构)，注意控制内存大小
// 单个客户端 Ice 推送的异步回调，用于失败后清理客户端句柄。
	class CPushCallback : public IceUtil::Shared
	{
	public:
		CPushCallback(unsigned long long p_ulKey,CJsonBinRPCImp * p_pParent)
		{
			m_nKey = p_ulKey;
			m_lpParent = p_pParent;
		}
		void response()
		{
			InterlockedDecrement64(&CPushMng::m_lPushPackCrowded);
			// 底层不再统计具体每个链接包的个数，只统计总数 m_lpParent->PushFinish(m_nKey);
		}
		void sent(bool p_bSent)
		{
		}
		void exception(const Ice::Exception& p_refException)
		{
			InterlockedDecrement64(&CPushMng::m_lPushPackCrowded);
			//cerr << "sayHello AMI call failed:\n" << ex << endl;
			//CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "%m_hSocket: ERROR=%u\r\n",ex.ice_id(),GetLastError());
			std::lock_guard<std::mutex> clLock(g_clHandleMutex);
			if ( g_mapHandle.find(m_lpParent) != g_mapHandle.end() )
			{
				m_lpParent->SelfDelClient(m_nKey);
			}
		}
		 void finished(const Ice::AsyncResultPtr& p_refResult) 
		 {

		 }
		 unsigned long long	m_nKey;
		//std::string		m_guid;
		CJsonBinRPCImp	*	m_lpParent;	
	};
	typedef IceUtil::Handle<CPushCallback> CPushCallbackPtr;


	ST_JSON_BIN_HANDLE::ST_JSON_BIN_HANDLE()
	{
		lRef = 1;
		iType = 0;
		dwCount= 0;
		pRpc = NULL;
		// 0x100 以上的值需要单独订阅
		// m_mapSubId[0x02] = 1;	// 缺省订阅基础行情
		tmLive		= time(NULL);
		refProxy.reset();
	}
// 根据句柄类型返回可调用代理，具体客户端句柄优先使用自身 refProxy。
	JSONBINRPC::IJsonBinRPCPrx ST_JSON_BIN_HANDLE::ClientIO()
	{
		if ( iType ==EN_JSON_HANDLE_RPC )
			return pRpc->ClientIO();
		else
			return refProxy.value();
	}
// 增加 HANDLE 引用计数，防止异步推送期间对象被提前释放。
	ST_JSON_BIN_HANDLE * ST_JSON_BIN_HANDLE::Ref()
	{
		InterlockedIncrement(&lRef);
		return this;
	}
// 释放 HANDLE 引用，计数归零时删除包装对象。
	bool ST_JSON_BIN_HANDLE::ReleaseIt()
	{	
		if ( 0==InterlockedDecrement(&lRef) )
		{
			delete this;
			return true;
		}
		return false;
	}


// 初始化 RPC 实例的服务端、客户端和推送状态，保证后续失败路径可安全清理。
CJsonBinRPCImp::CJsonBinRPCImp(void):m_clMemMng(MAX_CACHE_BUFLEN+1024*1024)
{
	m_iUdpThread	= 4;
	m_bSnappy		= false;
	m_bRegisterOk	= false;
	m_bNeedReconnect = false;
	m_semMiddle		= NULL;
	m_pfnServerCallback		= NULL;
	m_pfnBinaryServerCallback = NULL;
	m_pfnBinaryServerCallbackEx = NULL;
	m_pfnBinaryPriorityClassifier = NULL;
	m_pBinaryServerParam = NULL;
	//m_hSemPush		= NULL;
	//m_hPushPack		= NULL;
	m_refCommunicator	= nullptr;
	m_refAdapter		= nullptr;
	m_refLastAdapter   = nullptr;
	m_refCommunicatorClient=nullptr;
	m_refAdapterClient		=nullptr;
	m_pStockPushIo = nullptr;
	m_pParentImp	= nullptr;
	m_refStockIo.reset();
	m_refLocator.reset();
	//m_lPushPackCrowded = 0;
	m_bStop = false;
	m_bOpenTcp=false;
	m_bOpenTcpOk=false;
	m_bOpenUdp = false;
	m_bOpenUdpOk=false;
	m_bOpenMul = false;
	m_bOpenMulOk = false;
	m_bAsyncWaitCompleted = true;
	m_iBinaryProtocolVersion = s_iBinaryProtocolVersion;
	m_iBinaryCompressionThresholdBytes = 1024;
	m_iBinaryMaxPayloadBytes = 50 * 1024 * 1024;
	m_iBinaryCallTimeoutMs = 15000;
	m_iBinaryServerWorkerThreads = 4;
	m_iBinaryServerQueueCapacity = 4096;
	m_iBinaryResponseWorkerThreads = 8;
	m_iBinaryResponseQueueCapacity = 4096;
	m_iBinaryReclaimWorkerThreads = 2;
	m_iBinaryMaxPendingAsync = 10000;
	m_lBinaryPendingAsync.store(0);
	m_lBinaryPendingResponses.store(0);
	m_bBinaryWorkerStop.store(true);
	m_bBinaryResponseWorkerStop.store(true);
	m_bBinaryReclaimWorkerStop.store(true);
	m_hSocketServer	= nullptr;
	InitializeCriticalSection(&m_csLock);
	//InitializeCriticalSection(&m_csPush);
	//m_hPushPack	= NULL;
	{
		std::lock_guard<std::mutex> clLock(g_clHandleMutex);
		g_mapHandle[this] = this;
	}
	InitializeCriticalSection(&m_csMem);
}


// 析构只做兜底清理，正常释放应先走 DeleteIt/UnInit 完成线程退出。
CJsonBinRPCImp::~CJsonBinRPCImp(void)
{
	UnInit();
	DeleteCriticalSection(&m_csLock);
	DeleteCriticalSection(&m_csMem);
	//DeleteCriticalSection(&m_csPush);
	CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "正常退出~CJsonBinRPCImp\r\n");
}
// 静态函数
// 按句柄生命周期销毁 RPC 实例，集中处理全局句柄表和 Ice 资源。
void CJsonBinRPCImp::DeleteIt(CJsonBinRPCImp * p_pThis)
{
	HANDLE p_hHandle = NULL;
	bool	p_bIsDel = false;

	p_pThis->UnInit();
	int		i=0;
	if ( p_pThis->m_refCommunicatorClient )	// 必须前面处理，因为对于stockio；后面的对于this，导致删除了，不存在了
	{	// 如果是客户端存在回调
		p_hHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)s_DetroyIceCommunicator,&p_pThis->m_refCommunicatorClient,0,0);
		// 跳过线程处理，有的机器上，不能完全 destroy ，会卡住
		for ( i=0;i<8;++i )
		{
			if ( WaitForSingleObject(p_hHandle,1000)==WAIT_OBJECT_0 )
			{
				CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "[ %d 次/8] 成功关闭CJsonBinRPCImp  communicatorClient\r\n",i+1);
				break;
			}
			else
			{
				if ( i==7 )
				{
					::TerminateThread(p_hHandle,100);
					CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "关闭CJsonBinRPCImp  communicatorClient失败，强行关闭\r\n");
				}
				else
				{
					CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "[ %d 次/8] 尝试等待关闭CJsonBinRPCImp  communicatorClient\r\n",i+1);
				}
			}
		}

// 		if ( WaitForSingleObject(p_hHandle,8000)!=WAIT_OBJECT_0 )
// 		{
// 			::TerminateThread(p_hHandle,100);
// 			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "关闭CJsonBinRPCImp  communicatorClient失败，强行关闭\r\n");
// 		}
		CloseHandle(p_hHandle);
		p_pThis->m_refCommunicatorClient = nullptr;
	}
	if ( p_pThis->m_refCommunicator )	
	{
		p_bIsDel = true;
		p_hHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)s_DetroyIceCommunicator,&p_pThis->m_refCommunicator,0,0);
		for ( i=0;i<8;++i )
		{
			if ( WaitForSingleObject(p_hHandle,1000)==WAIT_OBJECT_0 )
			{
				CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "[ %d 次/8] 成功关闭CJsonBinRPCImp  communicatorServer\r\n",i+1);
				break;
			}
			else
			{
				if ( i==7 )
				{
					::TerminateThread(p_hHandle,100);
					CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "关闭CJsonBinRPCImp  communicatorServer失败，强行关闭\r\n");
				}
				else
				{
					CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "[ %d 次/8] 尝试等待关闭CJsonBinRPCImp  communicatorServer\r\n",i+1);
				}
			}
		}
// 		if ( WaitForSingleObject(p_hHandle,8000)!=WAIT_OBJECT_0 )
// 		{
// 			::TerminateThread(p_hHandle,100);
// 			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "关闭CJsonBinRPCImp  communicator失败，强行关闭\r\n");
// 			// 补充删除，减少内存损失
// 			//it->second.pushadapter->deactivate();
// 			// 主动删除，导致delelte  it->second.pushadapter->remove(it->second.pushident);
// 			//it->second.pushadapter->destroy();
// 		}
		CloseHandle(p_hHandle);
		// 导致删除了，不存在了 p_pThis->m_refCommunicator	= nullptr;
	}
	if ( !p_bIsDel )
	{
		delete	p_pThis;	// 客户端不会自动删除自己，adapter原因
	}
	CPushMng::ReleaseIt();
}

// 清理重连 locator 创建的附加 m_refCommunicator，避免旧连接残留。
void CJsonBinRPCImp::DeleteAddedConn()
{
	HANDLE p_hHandle = NULL;
	std::map<Ice::CommunicatorPtr,ST_ADDED_CONN_INFO>::iterator it = m_mapReconLocator.begin();
	while ( it != m_mapReconLocator.end() )
	{
		p_hHandle = CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)s_DetroyIceCommunicator,&it->second.refCommunicator,0,0);
		for (int i=0;i<8;++i )
		{
			if ( WaitForSingleObject(p_hHandle,1000)==WAIT_OBJECT_0 )
			{
				CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "[ %d 次/8] 成功关闭CJsonBinRPCImp  AddedConn\r\n",i+1);
				break;
			}
			else
			{
				if ( i==7 )
				{
					::TerminateThread(p_hHandle,100);
					CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "关闭CJsonBinRPCImp  AddedConn失败，强行关闭\r\n");
				}
				else
				{
					CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "[ %d 次/8] 尝试等待关闭CJsonBinRPCImp  AddedConn\r\n",i+1);
				}
			}
		}
		CloseHandle(p_hHandle);
		++it;
	}
	m_mapReconLocator.clear();
}
// void CJsonBinRPCImp::DeleteIt()
// {
// 
// 	if ( m_refCommunicator  )
// 	{
// 		try
// 		{	// 测试 ： m_refCommunicator->createObjectAdapter("WithCapitalio.Client");
// 			// WithCapitalio.Client.Endpoints=default -p_hHandle *  正常
// 			// WithCapitalio.Client.Endpoints=default -p_hHandle 127.0.0.1 导致退出失败，但是有时候 * 服务器支持不正常
// 			// 测试2：
// 			// WithCapitalio.Client.Endpoints=tcp -z -p_hHandle * -p 10009:udp -z -p_hHandle * -p 10009 固定端口，多次连接，导致服务端退出卡死
// 			// 测试3：
// 			// 在配置中加入  -t 5000 ，也导致destroy卡死
// 			m_refCommunicator->destroy();	// 删除自己
// 			// 已经删除了自己，因为adapter已经和CJsonBinRPCImp绑定,destroy访问成员变量就崩溃，非法指针  m_refCommunicator = nullptr;
// 		}
// 		catch(const IceUtil::Exception&)
// 		{
// 
// 		}
// 	}
// }
/*
#指定获取数据支持的市场;
#指定对应的ICE服务;如果是ZMQ，就自动注册;ICE可以考虑注册到Zookeeper上
Markets=SZ,SH
MarketsNetID=JsonBinRPCSZSH


#服务名字是自行配置的，不同的市场可以分拆，自行配置
JsonBinRPCSZSH.Endpoints=tcp -z -p_hHandle * -p 30001:udp -z -p_hHandle * -p 30001
JsonBinRPCSZSH.Proxy=Corbastockio:tcp -p_hHandle 192.168.0.156 -p 30001:udp -p_hHandle 192.168.0.156 -p 30001
#带增值服务推送接口
JsonBinRPCSZSH.Client.Endpoints=default -p_hHandle localhost
*/
// 服务端启动流程：读取配置、创建 m_refAdapter、注册 servant 并打开推送通道。
bool CJsonBinRPCImp::StartByServer(const char * p_szCfgFile,const char * p_szEndpointName,HANDLE & p_hSem,bool p_bSnappyCompress)
{
	//char	p_szEndpointName[1024]={0};
	//GetPrivateProfileString("COMM","MarketsConID","JsonBinRPC",p_szEndpointName,sizeof(p_szEndpointName),p_szCfgFile);
	char	tmpval[256]={0};
	m_strEndpointSrvName = p_szEndpointName;
	m_bSnappy = p_bSnappyCompress;
	m_strCfgFile = p_szCfgFile;
	if ( !m_refCommunicator  )
	{
		try
		{
			int argc = 0;
			Ice::InitializationData initData;
			initData.properties = Ice::createProperties();
			ST_XML_CONFIG_DATA clConfig;
			LoadIcePropertiesFromConfig(p_szCfgFile, initData.properties, &clConfig);
			m_bAsyncWaitCompleted = GetConfigInt(&clConfig, p_szCfgFile, "ICEPUSH", "AsyncWaitCompleted", 1) != 0;
			LoadBinaryConfig(&clConfig, p_szCfgFile);
			CIceRPCPushLog::Instance().ApplyConfig(p_szCfgFile, &clConfig);
			// Ice 3.8 initialize 会移走 InitializationData，后续读取属性必须保留独立 shared_ptr。
			Ice::PropertiesPtr refProperties = initData.properties;
			//log = new LogI;
			//initData.logger = log;
			m_refCommunicator = Ice::initialize(std::move(initData));
			m_refAdapter = m_refCommunicator->createObjectAdapter(m_strEndpointSrvName.c_str());	// 对应adapter 的name
			// Identity 要单独指定
			std::string idstr = refProperties->getProperty("Identity");
			// Push.iPort 要单独指定	: "0" tcp ； “1” udp
			// Push.strIp
// 			m_strPushPort = initData.properties->getPropertyWithDefault("RPCPushport","0");
// 			m_strPushPort = initData.properties->getProperty("RPCPushport");	// 指定推送模式
// 			m_strPushIp = initData.properties->getProperty("Push.strIp");	// 指定推送模式
			// 统一读取 XML/旧 INI 推送配置，非法数值回退到历史默认值。
			m_strPushBindPort = std::to_string(GetConfigInt(&clConfig, p_szCfgFile, "ICEPUSH", "RPCPushBindport", 0));
			m_strPushBindIp = GetConfigString(&clConfig, p_szCfgFile, "ICEPUSH", "RPCPushBindip", "127.0.0.1");
			m_strPushUdpBindPort = std::to_string(GetConfigInt(&clConfig, p_szCfgFile, "ICEPUSH", "RPCXPushBindport", 0));
			m_strPushUdpBindIp = GetConfigString(&clConfig, p_szCfgFile, "ICEPUSH", "RPCXPushBindip", "127.0.0.1");
			m_strPushUdpMulPort = std::to_string(GetConfigInt(&clConfig, p_szCfgFile, "ICEPUSH", "RPCMPushport", 0));
			m_strPushUdpMulIp = GetConfigString(&clConfig, p_szCfgFile, "ICEPUSH", "RPCMPuship", "127.0.0.1");
			int maxconnectnum = GetConfigInt(&clConfig, p_szCfgFile, "ICEPUSH", "RPCPushmax", 1000);
			if (maxconnectnum <= 0)
			{
				maxconnectnum = 1000;
			}
			m_iUdpThread = GetConfigInt(&clConfig, p_szCfgFile, "ICEPUSH", "RPCXThreadNum", 3);
			if (m_iUdpThread <= 0)
			{
				m_iUdpThread = 3;
			}
			if ( atol(m_strPushBindPort.c_str()) > 0 )
			{
				m_bOpenTcp = true;
			}
			if ( atol(m_strPushUdpBindPort.c_str()) > 0 )
			{
				m_bOpenUdp = true;
			}
			if ( atol(m_strPushUdpMulPort.c_str()) > 0 )
			{
				m_bOpenMul = true;
			}
			//////////////////////////////////////////////////////////////////////////
			m_stObjectSrvId  = Ice::stringToIdentity(idstr.c_str());
			m_strProgramName = refProperties->getProperty("Ice.ProgramName");	// 本次程序名字

			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "Adapter:%m_hSocket,ID:%m_hSocket\r\n",m_strEndpointSrvName.c_str(),idstr.c_str());
			CIceRPCPushLog::Instance().WriteLog("JSONPC","CJsonBinRPCImp::StartByServer",
						"Adapter:%m_hSocket,ID:%m_hSocket\r\n",m_strEndpointSrvName.c_str(),idstr.c_str());

			
			Ice::ObjectPtr stockio = icecompat::MakeServantPtr(this);
			m_refAdapter->add(stockio, m_stObjectSrvId);
			m_refAdapter->activate();
			::Ice::EndpointSeq vecEnd = m_refAdapter->getEndpoints();
			for ( int i=0;i<vecEnd.size();++i )
			{
				m_strEndpointSrv = vecEnd[i]->toString();
				CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "服务节点在: %m_hSocket\r\n",vecEnd[i]->toString().c_str());
				
				CIceRPCPushLog::Instance().WriteLog("JSONPC","CJsonBinRPCImp::StartByServer",
						"服务节点在: %m_hSocket\r\n",vecEnd[i]->toString().c_str());


				::Ice::EndpointInfoPtr p = vecEnd[i]->getInfo();
			}
			m_refLocator = m_refCommunicator->getDefaultLocator();
			m_refLastAdapter = m_refAdapter;

			if ( m_bOpenTcp )
			{
				char	err[1024];
				SYSTEM_INFO si;
				GetSystemInfo (&si);
				// 接收缓冲区太大，每次new浪费时间 pc->or.pBuf[0].pBuf=new char[pdata->rbuflen]; ，可以多次接收
				m_hSocketServer = s_create(SafeLongLongToLength<unsigned short>(atol(m_strPushBindPort.c_str())),1024*1024,maxconnectnum,maxconnectnum,30*60,ServerCallBack,this,si.dwNumberOfProcessors*2,err);
				if ( m_hSocketServer == NULL )
				{
					m_bOpenTcpOk = false;	// 开启失败
					CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "启动推送服务失败:%m_hSocket \r\n",m_strPushBindPort.c_str());
				}
				else
				{
					m_bOpenTcpOk = true;
					g_UserManage.Init(maxconnectnum);
				}
			}
			if ( m_bOpenMul )	// 组播
			{
				if ((m_hMulSocket=socket(AF_INET,SOCK_DGRAM,0)) < 0)     
				{    
					m_bOpenMulOk = false;
					CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "启动推送服务Multi失败:%m_hSocket \r\n",m_strPushUdpMulPort.c_str());
				}
				else
					m_bOpenMulOk = true;
				// set up destination address    
				memset(&m_stAddrSend,0,sizeof(m_stAddrSend));    
				m_stAddrSend.sin_family=AF_INET;    
				m_stAddrSend.sin_addr.s_addr=inet_addr(m_strPushUdpMulIp.c_str());    
				m_stAddrSend.sin_port=htons(SafeLongLongToLength<unsigned short>(atol(m_strPushUdpMulPort.c_str())));    
			}
		}
		catch(const IceUtil::Exception& ex)
		{
			m_strError = "ICE_SERVER_START_EXCEPTION: adapter=" + m_strEndpointSrvName + ", ice=" + ex.ice_id() + ", what=" + ex.what();
			char	str[1024]={0};
			UTF82ASC(ex.what(),str,1023);

			std::ostringstream ostr;
			ostr << ex << str;//ex.what();
			std::string m_hSocket = ostr.str();
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", m_hSocket.c_str());
			
			CIceRPCPushLog::Instance().WriteLog("JSONPC","CJsonBinRPCImp::AddConnectLoctor",
						"异常:%m_hSocket\r\n",m_hSocket.c_str());

			// 初始化中途失败时必须销毁已创建的 communicator，避免 DLL 卸载阶段报告资源泄漏。
			try
			{
				if (m_refCommunicator)
				{
					m_refCommunicator->destroy();
				}
			}
			catch (...)
			{
			}
			m_refAdapter = nullptr;
			m_refCommunicator = nullptr;
			return false;
		}
	}

	m_semMiddle		= CreateSemaphore( NULL,0,0x7fffffff,NULL );
	p_hSem = m_semMiddle;
	CPushMng::CreateNewPushObj();
	CPushMng::PushHandle(this);
	if ( m_bOpenUdp )	// 点到点推送
	{
		if ( CPushMng::CreateNewPushObj()->StartP2PPush(m_iUdpThread,atol(m_strPushUdpBindPort.c_str())) )
		{
			m_bOpenUdpOk = true;
		}
		else
		{
			m_bOpenUdpOk = false;
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "启动推送服务X失败:%m_hSocket \r\n",m_strPushUdpBindPort.c_str());
		}
		CPushMng::ReleaseIt();
	}
// 	m_hSemPush		= CreateSemaphore( NULL,0,0x7fffffff,NULL );
// 	if ( m_hPushPack == NULL )
// 	{
// 		m_hPushPack	= CreateThread(NULL,0,s_PushThread,this,0,NULL);
// 	}
	return true;
}
// 返回服务端配置中的 endpoint 名称，供导出 API 转交上层。
const char * CJsonBinRPCImp::GetEndPoint()
{
	return	m_strEndpointSrv.c_str();
}

// 停止 RPC 实例，依次释放 Ice、SocketServer、队列和客户端注册信息。
void CJsonBinRPCImp::UnInit()
{
	StopBinaryWorkers();
	{
		std::lock_guard<std::mutex> clLock(g_clHandleMutex);
		g_mapHandle.erase(this);
	}

	// 注销交给上层主动调用，因为上层可能因为对方链接断开，主动注销，这样导致卡在这里
// 	if ( m_refStockIo && !m_strClientGuid.empty() )
// 	{
// 		try
// 		{
// 			m_refStockIo->UnRegisterStockPushIO(m_strClientGuid.c_str());
// 		}
// 		catch(const IceUtil::Exception& ex)
// 		{
// 			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "%m_hSocket\r\n",ex.ice_id());
// 		}
// 
// 		m_strClientGuid = "";
// 
// 		//Ice::ConnectionPtr pconnect =  m_refStockIo->ice_getConnection();
// 
// 		//pconnect->close(true);
// 
// 	}
	m_bStop = true;
	
	if ( m_hSocketServer )
	{
		s_destroy(m_hSocketServer);
		m_hSocketServer = NULL;
	}

	CPushMng::PushHandle(this,true);	// 删除指针

	EnterCriticalSection(&m_csLock);
	std::map<unsigned long long,ST_JSON_BIN_HANDLE*>::iterator itclient = m_mapClients.begin();
	while ( itclient != m_mapClients.end() )
	{
		delete itclient->second;
		++itclient;
	}
	m_mapClients.clear();
	LeaveCriticalSection(&m_csLock);

	if ( m_semMiddle )
	{
		ReleaseSemaphore(m_semMiddle,0x7fffffff,NULL);
	}
	
// 	if ( m_hSemPush )
// 	{
// 		ReleaseSemaphore(m_hSemPush,0x7fffffff,NULL);
// 	}
// 	if ( m_hPushPack )
// 	{
// 		if ( WaitForSingleObject(m_hPushPack,3000)!=WAIT_OBJECT_0 )
// 			::TerminateThread(m_hPushPack,1111);
// 		CloseHandle(m_hPushPack);
// 		m_hPushPack = NULL;
// 	}

	std::map<Ice::CommunicatorPtr,ST_ADDED_CONN_INFO>::iterator it = m_mapReconLocator.begin();
	while ( it != m_mapReconLocator.end() )
	{
		try
		{
			//it->second.refAdapter->deactivate();
			it->second.refAdapter->remove(m_stObjectSrvId);		// 防止this多次被自动删除
			it->second.refCommunicator->shutdown();
		}
		catch(const IceUtil::Exception& ex)
		{
			char	str[1024]={0};
			UTF82ASC(ex.what(),str,1023);
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "%m_hSocket,%m_hSocket\r\n",ex.ice_id(),str);//ex.what());
		}
		++it;
	}
	DeleteAddedConn();

	ST_JSON_INPUT_EX * pNode = m_aReqMsg.PopFront();
	while ( pNode )
	{	// 发送请求

		if ( pNode->pRpcCallback || pNode->pPutCallback )
		{

			if ( pNode->chMode==EN_JSON_INPUT_RPC )
			{
				//delete	((::JSONBINRPC::AMD_IJsonBinRPC_JsonBinRPCPtr*)pNode->cbptr);	// 类型不对，会导致内存泄漏
				//::JSONBINRPC::AMD_IJsonBinRPC_JsonBinRPCPtr		cb((::JSONBINRPC::AMD_IJsonBinRPC_JsonBinRPC*)pNode->cbptr);// = *((::JSONBINRPC::AMD_IJsonBinRPC_JsonBinRPCPtr*)pNode->cbptr);

				pNode->pRpcCallback->ice_exception();

			}
			else if ( pNode->chMode ==EN_JSON_INPUT_PUT )
			{
				//delete ((::JSONBINRPC::AMD_IJsonBinRPC_JsonBinPUTPtr*)pNode->cbptr);	// 类型不对，会导致内存泄漏
				//::JSONBINRPC::AMD_IJsonBinRPC_JsonBinPUTPtr		cb((::JSONBINRPC::AMD_IJsonBinRPC_JsonBinPUT*)pNode->cbptr);// = *((::JSONBINRPC::AMD_IJsonBinRPC_JsonBinRPCPtr*)pNode->cbptr);

				pNode->pPutCallback->ice_exception();

			}
		}
		if ( pNode->stLParam.pBuffer )
		{
			delete [] pNode->stLParam.pBuffer;
			pNode->stLParam.pBuffer = NULL;
		}
		if ( pNode->stWParam.pBuffer )
		{
			delete [] pNode->stWParam.pBuffer;
			pNode->stWParam.pBuffer = NULL;
		}
		if ( pNode->stJsonReq.pBuffer )
		{
			delete [] pNode->stJsonReq.pBuffer;
			pNode->stJsonReq.pBuffer = NULL;
		}
		// 当做数组操作会出现崩溃现象 0xffffffff错误 delete [] pNode;
		delete pNode;
		pNode = m_aReqMsg.PopFront();
	}



	if ( m_semMiddle )
	{
		CloseHandle( m_semMiddle );
		m_semMiddle = NULL;
	}
// 	if ( m_hSemPush )
// 	{
// 		CloseHandle( m_hSemPush );
// 		m_hSemPush = NULL;
// 
// 	}
}

// ICE Slice 接口的实现
// ::JSONBINRPCU::AMD_IJsonBinRPC_JsonBinRPCPtr cb = ::IceUtil::Handle< ::JSONBINRPCU::AMD_IJsonBinRPC_JsonBinRPC>((::JSONBINRPCU::AMD_IJsonBinRPC_JsonBinRPC*)ireq->cb);
// 服务端 AMD RPC 入口，把请求复制到队列或直回调，尽快释放 Ice 派发线程。
void CJsonBinRPCImp::JsonBinRPC_async(const ::JSONBINRPC::AMD_IJsonBinRPC_JsonBinRPCPtr& p_pCallback, long long p_lSynId, long long p_lFuncId,long long p_lSetCode,const ::JSONBINRPC::AByte& p_stJsonReq, const ::Ice::Current& /*= ::Ice::Current()*/)
{
	::JSONBINRPC::AByte myjsonReq;
	UnCompressAByte(myjsonReq, p_stJsonReq.empty() ? NULL : (const char*)p_stJsonReq.data(), SafeSizeToLength<long>(p_stJsonReq.size()));

	if ( m_pfnServerCallback )
	{
		ST_JSON_MULTI_RESULT_DIRECT_CALLBACK * result = new ST_JSON_MULTI_RESULT_DIRECT_CALLBACK;
		result->lFuncId = p_lFuncId;
		result->lSynId = p_lSynId;
		result->aJsonReq = myjsonReq;
		result->stJsonReq.lLen = SafeSizeToLength<int>(result->aJsonReq.size());
		result->stJsonReq.pBuffer = result->aJsonReq.empty() ? NULL : (unsigned char*)result->aJsonReq.data();
		result->pRpcCallback = p_pCallback;
		result->chMode = EN_JSON_INPUT_RPC;

		m_pfnServerCallback(m_pServerParam, EN_JSON_INPUT_RPC, p_lSetCode, result);
		return;
	}

	ST_JSON_INPUT_EX * req = new ST_JSON_INPUT_EX;
	req->chMode = EN_JSON_INPUT_RPC;
	req->pRpcCallback = p_pCallback;
	req->lSetCode = p_lSetCode;
	req->hSelf = this;
	req->lSynId = p_lSynId;
	req->lFuncId = p_lFuncId;
	req->stJsonReq.lLen = SafeSizeToLength<int>(myjsonReq.size());
	if ( myjsonReq.size() > 0 )
	{
		req->stJsonReq.pBuffer = new unsigned char[myjsonReq.size() + 1];
		memcpy(req->stJsonReq.pBuffer, myjsonReq.data(), myjsonReq.size());
	}
#ifdef _DEBUG
	CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "进服务队列: %d %m_hSocket \r\n",req->lFuncId,req->stJsonReq);
#endif

	if ( m_pParentImp )
	{
		m_pParentImp->m_aReqMsg.PushBack(req);
		ReleaseSemaphore(m_pParentImp->m_semMiddle, 1, NULL);
	}
	else
	{
		m_aReqMsg.PushBack(req);
		ReleaseSemaphore(m_semMiddle, 1, NULL);
	}
}
// ICE Slice 接口的实现
// 服务端 AMD PUT 入口，除请求 JSON 外还复制上传参数供业务线程处理。
void CJsonBinRPCImp::JsonBinPUT_async(const ::JSONBINRPC::AMD_IJsonBinRPC_JsonBinPUTPtr& p_pCallback, long long p_lSynId, long long p_lFuncId,long long p_lSetCode,const ::JSONBINRPC::AByte& p_stJsonReq, long long p_lParam, const ::JSONBINRPC::AByte& p_stLParam, long long p_lWParam, const ::JSONBINRPC::AByte& p_stWParam, const ::Ice::Current& /* = ::Ice::Current() */)
{
	::JSONBINRPC::AByte myjsonReq,tmpbyte;
	UnCompressAByte(myjsonReq, p_stJsonReq.empty() ? NULL : (const char*)p_stJsonReq.data(), SafeSizeToLength<long>(p_stJsonReq.size()));

	if ( m_pfnServerCallback )
	{
		ST_JSON_MULTI_RESULT_DIRECT_CALLBACK * result = new ST_JSON_MULTI_RESULT_DIRECT_CALLBACK;
		result->lSetCode = p_lSetCode;
		result->chMode = EN_JSON_INPUT_PUT;
		result->pPutCallback = p_pCallback;
		result->hSelf = this;
		result->lSynId = p_lSynId;
		result->lFuncId = p_lFuncId;
		result->lParam = p_lParam;
		result->wParam = p_lWParam;
		result->aJsonReq = myjsonReq;
		result->stJsonReq.lLen = SafeSizeToLength<int>(result->aJsonReq.size());
		result->stJsonReq.pBuffer = result->aJsonReq.empty() ? NULL : (unsigned char*)result->aJsonReq.data();
		if ( p_stLParam.size() > 0 )
		{
			UnCompressAByte(result->aLParam, (const char*)p_stLParam.data(), SafeSizeToLength<long>(p_stLParam.size()));
			result->stLParam.lLen = SafeSizeToLength<int>(result->aLParam.size());
			result->stLParam.pBuffer = result->aLParam.empty() ? NULL : (unsigned char*)result->aLParam.data();
		}
		if ( p_stWParam.size() > 0 )
		{
			UnCompressAByte(result->aWParam, (const char*)p_stWParam.data(), SafeSizeToLength<long>(p_stWParam.size()));
			result->stWParam.lLen = SafeSizeToLength<int>(result->aWParam.size());
			result->stWParam.pBuffer = result->aWParam.empty() ? NULL : (unsigned char*)result->aWParam.data();
		}

		m_pfnServerCallback(m_pServerParam, EN_JSON_INPUT_PUT, p_lSetCode, result);
		return;
	}

	ST_JSON_INPUT_EX * req = new ST_JSON_INPUT_EX;
	req->lSetCode = p_lSetCode;
	req->chMode = EN_JSON_INPUT_PUT;
	req->pPutCallback = p_pCallback;
	req->hSelf = this;
	req->lSynId = p_lSynId;
	req->lFuncId = p_lFuncId;
	req->stJsonReq.lLen = SafeSizeToLength<int>(myjsonReq.size());
	if ( myjsonReq.size() > 0 )
	{
		req->stJsonReq.pBuffer = new unsigned char[myjsonReq.size() + 1];
		memcpy(req->stJsonReq.pBuffer, myjsonReq.data(), myjsonReq.size());
	}
	req->lParam = p_lParam;
	req->wParam = p_lWParam;
	if ( p_stLParam.size() > 0 )
	{
		UnCompressAByte(tmpbyte, (const char*)p_stLParam.data(), SafeSizeToLength<long>(p_stLParam.size()));
		req->stLParam.lLen = SafeSizeToLength<int>(tmpbyte.size());
		if (req->stLParam.lLen > 0)
		{
			req->stLParam.pBuffer = new unsigned char[req->stLParam.lLen];
			memcpy(req->stLParam.pBuffer, tmpbyte.data(), req->stLParam.lLen);
		}
	}
	if ( p_stWParam.size() > 0 )
	{
		UnCompressAByte(tmpbyte, (const char*)p_stWParam.data(), SafeSizeToLength<long>(p_stWParam.size()));
		req->stWParam.lLen = SafeSizeToLength<int>(tmpbyte.size());
		if (req->stWParam.lLen > 0)
		{
			req->stWParam.pBuffer = new unsigned char[req->stWParam.lLen];
			memcpy(req->stWParam.pBuffer, tmpbyte.data(), req->stWParam.lLen);
		}
	}

	if ( m_pParentImp )
	{
		m_pParentImp->m_aReqMsg.PushBack(req);
		ReleaseSemaphore(m_pParentImp->m_semMiddle, 1, NULL);
	}
	else
	{
		m_aReqMsg.PushBack(req);
		ReleaseSemaphore(m_semMiddle, 1, NULL);
	}
}
// 旧版推送注册入口，保存客户端代理并通知上层新增客户端。
void CJsonBinRPCImp::RegisterStockPushIO_async(const ::JSONBINRPC::AMD_IJsonBinRPC_RegisterStockPushIOPtr& p_pCallback, const ::std::string& p_strGuid, const ::Ice::Identity& p_stIdent, const ::Ice::Current& p_stCurrent)
{	
	::Ice::ConnectionPtr con	= p_stCurrent.con;
	std::optional<::JSONBINRPC::IJsonBinRPCPrx> sameproxy;
	try
	{
		sameproxy = con->createProxy<::JSONBINRPC::IJsonBinRPCPrx>(p_stIdent);// 这是原有链接的，不是新的链接的 p_stCurrent.id));
		//sameproxy = JSONBINRPCU::IStockPushIOPrx::uncheckedCast(sameproxy->ice_twoway());
		sameproxy = sameproxy->ice_twoway();
		//sameproxy = sameproxy->ice_timeout(10000);
	}
	catch(const IceUtil::Exception& ex)
	{
		CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "%m_hSocket\r\n",ex.ice_id());
		p_pCallback->ice_response(m_bSnappy);	// 告知客户端服务端用了压缩没有
		return;
	}
	//////////////////////////////////////////////////////////////////////////
	bool	newflag = false;
	ST_JSON_BIN_HANDLE * p = NULL;
	char	tmpbuf[512];
	snprintf(tmpbuf, sizeof(tmpbuf), "%s%p", p_strGuid.c_str(), con.get());	// 防止终端给同样guid或者空
	unsigned long long	p_ulKey = ST_JSON_BIN_HANDLE::GenKey(tmpbuf);//p_strGuid.c_str());
	EnterCriticalSection(&m_csLock);
	if ( m_mapClients.find(p_ulKey) == m_mapClients.end() )
	{
		m_mapClients[p_ulKey] = new ST_JSON_BIN_HANDLE;
		m_mapClients[p_ulKey]->iType = EN_JSON_HANDLE_CLIENT;
		m_mapClients[p_ulKey]->pRpc = this;
		m_mapClients[p_ulKey]->refProxy = sameproxy;	// 调用过程，修改指针，有踏空风险。调用函数不在临界区内
		m_mapClients[p_ulKey]->strGuid= tmpbuf;//p_strGuid;
		newflag = true;
	}
	p = m_mapClients[p_ulKey];
	int		i;
	DWORD	dwV=0;
	const	char * pstr = NULL;
	std::vector<std::string>	tosecs;
	TokenizeOR(tosecs,p_strGuid.c_str(),"|");
	// Keep the legacy GUID subscription convention for RegisterStockPushIO.
	for ( i=0;i<tosecs.size();++i )
	{
		pstr = SundayQuickSearch(tosecs[i].c_str(),"~");
		if ( pstr )
		{
			dwV = atol(pstr+1);
			p->mapSubId[ dwV ]	= 0;		// 取消订阅
		}
		else
		{
			dwV = atol(tosecs[i].c_str());
			p->mapSubId[ dwV ]	= 1;		// 订阅
		}
	}
	m_mapClients[p_ulKey]->tmLive = time(NULL);
	// m_mapClients[p_ulKey]->refProxy = sameproxy;
	//m_mapClients[p_ulKey]->strGuid= tmpbuf;//p_strGuid;
	p->Ref();		// 多一次计数器，防止被删除
	LeaveCriticalSection(&m_csLock);

	p_pCallback->ice_response(m_bSnappy);	// 告知客户端服务端用了压缩没有

	CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "注册[%d] ：%m_hSocket %m_hSocket \r\n",m_mapClients.size(),p_strGuid.c_str(),p_stIdent.name.c_str());
	if ( newflag )
	{
		CPushMng::ProcessPackage(NOTIFY_ADD_CLIENT,(const char*)p,sizeof(ST_JSON_BIN_HANDLE*));
	}
	CPushMng::PushHandle(this);
	p->ReleaseIt();	// 减少计数器
}
// 旧版推送注销入口，移除客户端代理并通知上层断开。
void CJsonBinRPCImp::UnRegisterStockPushIO_async(const ::JSONBINRPC::AMD_IJsonBinRPC_UnRegisterStockPushIOPtr& p_pCallback, const ::std::string& p_strGuid, const ::Ice::Current& p_stCurrent)
{
	::Ice::ConnectionPtr con	= p_stCurrent.con;
	ST_JSON_BIN_HANDLE * p = NULL;
	char	tmpbuf[512];
	snprintf(tmpbuf, sizeof(tmpbuf), "%s%p", p_strGuid.c_str(), con.get());	// 防止终端给同样guid或者空
	unsigned long long	p_ulKey = ST_JSON_BIN_HANDLE::GenKey(tmpbuf);//p_strGuid.c_str());
	EnterCriticalSection(&m_csLock);
	if ( m_mapClients.find(p_ulKey)!=m_mapClients.end() )
	{
		p =  m_mapClients[p_ulKey];
		m_mapClients.erase(p_ulKey);
	}
	LeaveCriticalSection(&m_csLock);
	
	p_pCallback->ice_response(m_bSnappy);
	if ( p )
	{
		CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "主动反注册[%d] ：%m_hSocket \r\n",m_mapClients.size(),p_strGuid.c_str());
		CPushMng::ProcessPackage(NOTIFY_DEL_CLIENT,(const char*)p,sizeof(ST_JSON_BIN_HANDLE*));
		p->ReleaseIt();
	}
}

// 带订阅信息的推送注册入口，上层可根据 p_strSubInfo 返回快速通道配置。
void CJsonBinRPCImp::RegisterStockPushIO2_async(const ::JSONBINRPC::AMD_IJsonBinRPC_RegisterStockPushIO2Ptr& p_pCallback, const ::std::string& p_strGuid, const ::std::string& p_strSubInfo, const ::Ice::Identity& p_stIdent, const ::Ice::Current& p_stCurrent/*= ::Ice::Current()*/)
{
	std::string	p_strRet;
	char		retbuf[256] = {0};
	// 底层内部固定的几个配置
	snprintf(retbuf, sizeof(retbuf), "snappy=%d;", m_bSnappy);
	p_strRet += retbuf;

	snprintf(retbuf, sizeof(retbuf), "rpcpushbindport=%s;", m_strPushBindPort.c_str());
	p_strRet += retbuf;

	snprintf(retbuf, sizeof(retbuf), "rpcpushbindip=%s;", m_strPushBindIp.c_str());		// 127.0.0.1|192.168.0.15
	p_strRet += retbuf;

	//客户端请求的不要重复写了 p_strRet += p_strSubInfo.c_str();		// 上层可以自己设置的订阅参数信息

	::Ice::ConnectionPtr con	= p_stCurrent.con;
	std::optional<::JSONBINRPC::IJsonBinRPCPrx> sameproxy;
	try
	{
		sameproxy = con->createProxy<::JSONBINRPC::IJsonBinRPCPrx>(p_stIdent);// 这是原有链接的，不是新的链接的 p_stCurrent.id));
		//sameproxy = JSONBINRPCU::IStockPushIOPrx::uncheckedCast(sameproxy->ice_twoway());
		sameproxy = sameproxy->ice_twoway();
		//sameproxy = sameproxy->ice_timeout(10000);
	}
	catch(const IceUtil::Exception& ex)
	{
		CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "%m_hSocket\r\n",ex.ice_id());
		p_pCallback->ice_response(p_strRet);	// 告知客户端服务端用了压缩没有
		return;
	}
	//////////////////////////////////////////////////////////////////////////
	bool	newflag = false;
	ST_JSON_BIN_HANDLE * p = NULL;
	char	tmpbuf[512];
	snprintf(tmpbuf, sizeof(tmpbuf), "%s%p", p_strGuid.c_str(), con.get());	// 防止终端给同样guid或者空
	unsigned long long	p_ulKey = ST_JSON_BIN_HANDLE::GenKey(tmpbuf);//p_strGuid.c_str());
	EnterCriticalSection(&m_csLock);
	if ( m_mapClients.find(p_ulKey) == m_mapClients.end() )
	{
		m_mapClients[p_ulKey] = new ST_JSON_BIN_HANDLE;
		m_mapClients[p_ulKey]->iType = EN_JSON_HANDLE_CLIENT;
		m_mapClients[p_ulKey]->pRpc = this;
		m_mapClients[p_ulKey]->refProxy = sameproxy;	// 调用过程，修改指针，有踏空风险。调用函数不在临界区内
		m_mapClients[p_ulKey]->strGuid= tmpbuf;//p_strGuid;
		newflag = true;
	}
	p = m_mapClients[p_ulKey];
	// RegisterStockPushIO2 uses subInfo and atomically replaces the old set.
	CSubscriptionFilter::Replace(p_strSubInfo, p->mapSubId);
	m_mapClients[p_ulKey]->tmLive = time(NULL);
	// m_mapClients[p_ulKey]->refProxy = sameproxy;
	//m_mapClients[p_ulKey]->strGuid= tmpbuf;//p_strGuid;
	p->Ref();		// 多一次计数器，防止被删除
	LeaveCriticalSection(&m_csLock);

	p_pCallback->ice_response(p_strRet);	// 告知客户端服务端用了压缩没有

	CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "注册[%d] ：%m_hSocket %m_hSocket \r\n",m_mapClients.size(),p_strGuid.c_str(),p_stIdent.name.c_str());
	std::string	ncistr(sizeof(ST_NOTIFY_CLIENT_INFO)+p_strSubInfo.size(),0);
	ST_NOTIFY_CLIENT_INFO * nci = (ST_NOTIFY_CLIENT_INFO*)ncistr.data();
	nci->hClient = p;
	nci->lLen = SafeSizeToLength<long>(p_strSubInfo.size());
	if (nci->lLen > 0)
	{
		memcpy(nci->szData, p_strSubInfo.data(), static_cast<size_t>(nci->lLen));
	}
	if ( newflag )
	{
		CPushMng::ProcessPackage(NOTIFY_ADD_CLIENT,(const char*)nci,sizeof(ST_NOTIFY_CLIENT_INFO)+nci->lLen);
	}
	CPushMng::ProcessPackage(NOTIFY_SUB_CLIENT,(const char*)nci,sizeof(ST_NOTIFY_CLIENT_INFO)+nci->lLen);		// 通知上层订阅的信息
	CPushMng::PushHandle(this);
	p->ReleaseIt();	// 减少计数器
}

// 带订阅信息的注销入口，区分退订和彻底断开两种通知。
void CJsonBinRPCImp::UnRegisterStockPushIO2_async(const ::JSONBINRPC::AMD_IJsonBinRPC_UnRegisterStockPushIO2Ptr& p_pCallback, const ::std::string& p_strGuid, const ::std::string& p_strSubInfo, const ::Ice::Identity& p_stIdent, const ::Ice::Current& p_stCurrent/*= ::Ice::Current()*/)
{
	std::string	p_strRet;
	char		retbuf[256] = {0};
	// 底层内部固定的几个配置
	snprintf(retbuf, sizeof(retbuf), "snappy=%d;", m_bSnappy);
	p_strRet += retbuf;

	snprintf(retbuf, sizeof(retbuf), "rpcpushbindport=%s;", m_strPushBindPort.c_str());
	p_strRet += retbuf;

	snprintf(retbuf, sizeof(retbuf), "rpcpushbindip=%s;", m_strPushBindIp.c_str());		// 127.0.0.1|192.168.0.15
	p_strRet += retbuf;

	// p_strRet += p_strSubInfo.c_str();		// 上层可以自己设置的订阅参数信息

	::Ice::ConnectionPtr con	= p_stCurrent.con;
	ST_JSON_BIN_HANDLE * p = NULL;
	char	tmpbuf[512];
	snprintf(tmpbuf, sizeof(tmpbuf), "%s%p", p_strGuid.c_str(), con.get());	// 防止终端给同样guid或者空
	unsigned long long	p_ulKey = ST_JSON_BIN_HANDLE::GenKey(tmpbuf);//p_strGuid.c_str());
	EnterCriticalSection(&m_csLock);
	if ( m_mapClients.find(p_ulKey)!=m_mapClients.end() )
	{
		p =  m_mapClients[p_ulKey];
		m_mapClients.erase(p_ulKey);
	}
	LeaveCriticalSection(&m_csLock);

	p_pCallback->ice_response(p_strRet);
	if ( p )
	{
		std::string	ncistr(sizeof(ST_NOTIFY_CLIENT_INFO)+p_strSubInfo.size(),0);
		ST_NOTIFY_CLIENT_INFO * nci = (ST_NOTIFY_CLIENT_INFO*)ncistr.data();
		nci->hClient = p;
		nci->lLen	 = SafeSizeToLength<long>(p_strSubInfo.size());
		memcpy(nci->szData,p_strSubInfo.c_str(),p_strSubInfo.size());

		CPushMng::ProcessPackage(NOTIFY_UNSUB_CLIENT,(const char*)nci,sizeof(ST_NOTIFY_CLIENT_INFO)+nci->lLen);

		CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "主动反注册[%d] ：%m_hSocket \r\n",m_mapClients.size(),p_strGuid.c_str());
		CPushMng::ProcessPackage(NOTIFY_DEL_CLIENT,(const char*)nci,sizeof(ST_NOTIFY_CLIENT_INFO));
		p->ReleaseIt();
	}
}
// 客户端保活和重连检查，注册超时或连接拒绝时触发重建。
void CJsonBinRPCImp::TTLive()
{
	if ( m_bStop )
		return;
	std::vector<unsigned long long>	aDelKey;
	EnterCriticalSection(&m_csLock);
	std::map<unsigned long long,ST_JSON_BIN_HANDLE*>::iterator it = m_mapClients.begin();
	while ( it != m_mapClients.end() )
	{
		if ( time(NULL) - it->second->tmLive > 30*60 )	
		{
			aDelKey.push_back(it->first);
		}
		++it;
	}
	LeaveCriticalSection(&m_csLock);
	// 超时，并且发送积压严重，没有任何返回的链接删除
	for ( int i=0;i<aDelKey.size();++i )
	{
		SelfDelClient(aDelKey[i]);
	}
	if ( m_bNeedReconnect )
	{
		AddConnectLoctor();
	}
	else if ( m_refLocator.has_value() )
	{	// 加入一个和定位服务器之间的存活包
		//m_refLocator->begin_findObjectById(m_stObjectSrvId);
		try
		{
			//::Ice::ObjectPrx  psrv = m_refLocator->findObjectById(m_stObjectSrvId);	// 增加跟定位服务器之间的存活包,但是没有解决和node之间的存活包
			m_refLastAdapter->setPublishedEndpoints(m_refLastAdapter->getPublishedEndpoints());
// #ifdef _DEBUG
// 			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "定位服务查找 : %p ,%m_hSocket\r\n",psrv,m_stObjectSrvId.name.c_str());
// #endif
		}
		catch(const IceUtil::Exception& ex)
		{
			if ( SundayQuickSearch(ex.what(),"NotRegisteredException")  )
			{
				// 可以自动通过 refreshPublishedEndpoints 恢复，这个是icenode节点消失导致
			}
			else if ( SundayQuickSearch(ex.what(),"ConnectionRefusedException")  )
			{
				// locator消失导致
				m_bNeedReconnect = true;
			}

			char	str[1024]={0};
			UTF82ASC(ex.what(),str,1023);

			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "locator 调用失败%m_hSocket:%m_hSocket\r\n",ex.ice_id(),str);//ex.what());
			
			CIceRPCPushLog::Instance().WriteLog("JSONPC","CJsonBinRPCImp::TTLive",
						"refreshPublishedEndpoints失败%m_hSocket:%m_hSocket，m_bNeedReconnect=%d\r\n",ex.ice_id(),str,m_bNeedReconnect);
			
		}
	}
}
// 定期刷新定位器
// locator 重连辅助流程，创建新 m_refAdapter 后再替换，减少通信中的断点。
bool CJsonBinRPCImp::AddConnectLoctor()
{
	HANDLE p_hHandle = NULL;
	m_bNeedReconnect = false;
	int		i=0;
	Ice::CommunicatorPtr conn;
	Ice::ObjectAdapterPtr m_refAdapter;
	try
	{
		int argc = 0;
		Ice::InitializationData initData;
		initData.properties = Ice::createProperties();
		ST_XML_CONFIG_DATA clConfig;
		LoadIcePropertiesFromConfig(m_strCfgFile.c_str(), initData.properties, &clConfig);
		m_bAsyncWaitCompleted = GetConfigInt(&clConfig, m_strCfgFile.c_str(), "ICEPUSH", "AsyncWaitCompleted", 1) != 0;
		LoadBinaryConfig(&clConfig, m_strCfgFile.c_str());
		CIceRPCPushLog::Instance().ApplyConfig(m_strCfgFile.c_str(), &clConfig);
		// 保留属性对象，避免 Ice 3.8 initialize 移动 initData 后继续访问空指针。
		Ice::PropertiesPtr refProperties = initData.properties;
		//log = new LogI;
		//initData.logger = log;
		conn = Ice::initialize(std::move(initData));
		m_refAdapter = conn->createObjectAdapter(m_strEndpointSrvName.c_str());	// 对应adapter 的name
		// Identity 要单独指定
		std::string idstr = refProperties->getProperty("Identity");
		m_stObjectSrvId  = Ice::stringToIdentity(idstr.c_str());
		m_strProgramName = refProperties->getProperty("Ice.ProgramName");	// 本次程序名字

		CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "Adapter:%m_hSocket,ID:%m_hSocket\r\n",m_strEndpointSrvName.c_str(),idstr.c_str());
		CIceRPCPushLog::Instance().WriteLog("JSONPC","CJsonBinRPCImp::AddConnectLoctor",
						"Adapter:%m_hSocket,ID:%m_hSocket\r\n",m_strEndpointSrvName.c_str(),idstr.c_str());
		
		Ice::ObjectPtr stockio = icecompat::MakeServantPtr(this);
		m_refAdapter->add(stockio, m_stObjectSrvId);
		m_refAdapter->activate();
		::Ice::EndpointSeq vecEnd = m_refAdapter->getEndpoints();
		for ( int i=0;i<vecEnd.size();++i )
		{
			m_strEndpointSrv = vecEnd[i]->toString();
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "服务节点在: %m_hSocket\r\n",vecEnd[i]->toString().c_str());
			
			CIceRPCPushLog::Instance().WriteLog("JSONPC","CJsonBinRPCImp::AddConnectLoctor",
						"服务节点在: %m_hSocket\r\n",vecEnd[i]->toString().c_str());


			::Ice::EndpointInfoPtr p = vecEnd[i]->getInfo();
		}
		m_refLocator = conn->getDefaultLocator();
		m_refAdapter->setPublishedEndpoints(m_refAdapter->getPublishedEndpoints());
		m_refLastAdapter = m_refAdapter;
	}
	catch(const IceUtil::Exception& ex)
	{
		char	str[1024]={0};
		UTF82ASC(ex.what(),str,1023);
		std::ostringstream ostr;
		ostr << ex << str;//ex.what();
		std::string m_hSocket = ostr.str();
		CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", m_hSocket.c_str());
		
		CIceRPCPushLog::Instance().WriteLog("JSONPC","CJsonBinRPCImp::AddConnectLoctor",
						"异常:%m_hSocket\r\n",m_hSocket.c_str());

		return false;
	}
	m_mapReconLocator[conn].refCommunicator = conn;
	m_mapReconLocator[conn].refAdapter	= m_refAdapter;
	return true;
}

// 异步推送失败后的客户端自删除流程，同时向上层发送断开通知。
void CJsonBinRPCImp::SelfDelClient(unsigned long long	p_ulKey)
{
	ST_JSON_BIN_HANDLE * p=NULL;
	std::string	thclient;
	EnterCriticalSection(&m_csLock);
	if ( m_mapClients.find(p_ulKey)!=m_mapClients.end() )
	{
		p =  m_mapClients[p_ulKey];
		m_mapClients.erase(p_ulKey);
		thclient = p->strGuid;
	}
	else
	{
		CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "~");
	}
	LeaveCriticalSection(&m_csLock);
	
	if ( p )
	{
		CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "异常反注册[%d] ：GUID=%m_hSocket,Key=%I64u,Left=%I64d \r\n",m_mapClients.size(),thclient.c_str(),p_ulKey,CPushMng::m_lPushPackCrowded);
		CPushMng::ProcessPackage(NOTIFY_DEL_CLIENT,(const char*)p,sizeof(ST_JSON_BIN_HANDLE*));
		p->ReleaseIt();
	}
}

// 推送完成后释放客户端句柄引用，配合 CPushCallback 防止悬空访问。
void CJsonBinRPCImp::PushFinish(unsigned long long p_ulKey)
{
	EnterCriticalSection(&m_csLock);
	if ( m_mapClients.find(p_ulKey)!=m_mapClients.end() )
	{
		m_mapClients[p_ulKey]->tmLive = time(NULL);
		m_mapClients[p_ulKey]->dwCount--;
	}
	LeaveCriticalSection(&m_csLock);
}


// 互动接口,不是Slice实现,用于批量推送到连接上来的客户端
// 要想去掉堆的影响，把底层改成队列，上层就不需要队列了
// 服务端主动推送核心流程，按订阅状态和通道类型复制/压缩/发送数据。
long long CJsonBinRPCImp::ProcessPackage( long long p_lReqNo,const char * p_pBuf,long p_lBufLen,bool p_bIsAsync )
{
	if ( p_lBufLen <=0 )
	{
		CIceRPCPushLog::Instance().WriteLog("JSONPC","Pushdata","推送失败:数据0 %d / %d ",p_lBufLen,MAX_CACHE_BUFLEN);
		return 0;
	}
	// 每个推送包独立决定是否压缩，所有通道共享同一版本、阈值和长度校验规则。
	ST_BINARY_VIEW stView;
	stView.lLen = p_lBufLen;
	stView.pBuffer = reinterpret_cast<const unsigned char*>(p_pBuf);
	::JSONBINRPC::BinaryPayload stPayload;
	std::string strPayloadError;
	if (!EncodeBinaryPayload(stView, stPayload, strPayloadError))
	{
		CIceRPCPushLog::Instance().WriteLog("JSONPC", "Pushdata",
			"BINARY_PUSH_ENCODE_FAILED: req=%I64d, detail=%s\r\n",
			p_lReqNo, strPayloadError.c_str());
		return InferBinaryProtocolErrorCode(strPayloadError);
	}
	const long lWireLen = SafeSizeToLength<long>(stPayload.data.size());
	const char* pWireData = stPayload.data.empty() ? NULL :
		reinterpret_cast<const char*>(stPayload.data.data());
	const bool bPacketSnappy = stPayload.compression ==
		::JSONBINRPC::BinaryCompression::BinaryCompressionSnappy;
	if ( m_bOpenTcpOk )
	{
		// InterlockedIncrement64(&CPushMng::m_lPushPackCrowded);
		std::map<ST_HDATA_HCLIENT,int>	auser;
		g_UserManage.GetAllSubUser(auser);
		std::map<ST_HDATA_HCLIENT,int>::iterator it = auser.begin();
		while ( it != auser.end() )
		{
			long	m_lLen = lWireLen;
			std::map<DWORD,int>		subid;
			ST_USER_DATA * puserdata=g_UserManage.Query(it->first.hServer,it->first.hHandle);
			CAutoReleaseFunc	autoFunc(puserdata,UserManageFunc);
			if ( puserdata )
			{
				g_UserManage.GetSubKey(puserdata,subid);
				
				if ( p_lReqNo > 0x100 )
				{
					DWORD dwReqNo = SafeLongLongToLength<DWORD>(p_lReqNo);
					if ( subid.find(dwReqNo) == subid.end() )	// 没有订阅此功能，忽略
					{
						//CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "%d:未订阅%m_hSocket,%d\r\n",p_lReqNo,puserdata->strIp,puserdata->iPort);
						++it;
						continue;
					}
					else  if ( subid[dwReqNo] <= 0 )			// 订阅状态不是 1 ，忽略
					{
						//CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "%d:取消订阅%m_hSocket,%d\r\n",p_lReqNo,puserdata->strIp,puserdata->iPort);
						++it;
						continue;
					}
				}

				EnterCriticalSection(&m_csMem);
				char * pTransfer = (char*)m_clMemMng.Malloc();
				if ( pTransfer == NULL )
				{
					CIceRPCPushLog::Instance().WriteLog("JSONPC","Pushdata","申请内存失败:%p, memsize=%d\r\n",pTransfer,m_clMemMng.GetMaxSize());
					LeaveCriticalSection(&m_csMem);
					++it;
					continue;
				}
				LeaveCriticalSection(&m_csMem);

				char * transfer_buf		= pTransfer +sizeof(ST_ANS_HEADER);
				ST_ANS_HEADER * pAnsHead = (ST_ANS_HEADER*)pTransfer;
				ST_ANS_HEADER &	AnsHeader= *pAnsHead;
				AnsHeader.dwRawLen		= stPayload.rawSize;	// pHandleBuffer 要发送的数据长度
				if ( m_lLen < MAX_CACHE_BUFLEN )
				{
					if (m_lLen > 0)
					{
						memcpy(transfer_buf, pWireData, m_lLen);
					}
				}
				else
				{
					m_lLen = 0;
					CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "数据包太大，丢弃\r\n");
				}

				// 创建应答包头
				AnsHeader.dwCrc				= 0;
				AnsHeader.lSeparator			= SEPARATOR_NUM;
				AnsHeader.stInfo.chVersion		= 2;
				AnsHeader.stInfo.chCompressed	= bPacketSnappy ? EN_COMPRESS_SNAPPY : EN_COMPRESS_DEFAULT;
				AnsHeader.stInfo.chEncrypted	= 0;
				AnsHeader.stInfo.chTalkCompress	= 0;
				
				AnsHeader.dwPacketLen			= m_lLen;	// pHandleBuffer 要发送的数据长度
				AnsHeader.dwCookie			= 0;
				AnsHeader.lMainId			= 0;
				AnsHeader.lAssistId 			= 0;
				AnsHeader.chPriority 			= 0;
				AnsHeader.req				= SafeLongLongToLength<long>(p_lReqNo);// 这个值不能太大  PACKET_PUSH_HQ_SUB;//pNode->nGNID;

				// m_lLen = AnsHeader.dwPacketLen;	// 压缩后的长度
				// memcpy(pTransfer,&AnsHeader,sizeof(ST_ANS_HEADER));		// 拷贝应答包头

				long nLeft = sizeof(ST_ANS_HEADER)+m_lLen;
				s_send(it->first.hServer,it->first.hHandle, pTransfer,nLeft);	// 直接发送应答数据

				EnterCriticalSection(&m_csMem);
				m_clMemMng.Free(pTransfer);
				LeaveCriticalSection(&m_csMem);
			}
			++it;
		}
		return CPushMng::m_lPushPackCrowded;
	}
	// UDP 数据大于5K，要分页发送，这个逻辑要实现....................
	if ( m_bOpenMulOk )
	{
		long	m_lLen = lWireLen;

		EnterCriticalSection(&m_csMem);
		char * pTransfer = (char*)m_clMemMng.Malloc();
		if ( pTransfer == NULL )
		{
			CIceRPCPushLog::Instance().WriteLog("JSONPC","Pushdata","申请内存失败:%p, memsize=%d\r\n",pTransfer,m_clMemMng.GetMaxSize());
			LeaveCriticalSection(&m_csMem);
			return CPushMng::m_lPushPackCrowded;
		}
		LeaveCriticalSection(&m_csMem);


		char * transfer_buf		= pTransfer +sizeof(ST_UDP_HEADER);
		ST_UDP_HEADER * pUDPHead = (ST_UDP_HEADER*)pTransfer;
		ST_UDP_HEADER &	UDPHeader= *pUDPHead;
		if ( m_lLen < MAX_CACHE_BUFLEN )
		{
			if (m_lLen > 0)
			{
				memcpy(transfer_buf, pWireData, m_lLen);
			}
		}
		else
		{
			m_lLen = 0;
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "数据包太大，丢弃\r\n");
		}

		// 创建应答包头
		UDPHeader.chCompressed	= bPacketSnappy ? EN_COMPRESS_SNAPPY : EN_COMPRESS_DEFAULT;

		UDPHeader.req				= SafeLongLongToLength<unsigned short>(p_lReqNo);// 这个值不能太大  PACKET_PUSH_HQ_SUB;//pNode->nGNID;

		long nLeft = sizeof(ST_UDP_HEADER)+m_lLen;
		if ( sendto(m_hMulSocket,pTransfer,nLeft, 0, (struct sockaddr *) &m_stAddrSend, sizeof(m_stAddrSend))< 0 )
		{
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "send fail : %u \n",GetLastError());
		}

		EnterCriticalSection(&m_csMem);
		m_clMemMng.Free(pTransfer);
		LeaveCriticalSection(&m_csMem);


		return CPushMng::m_lPushPackCrowded;
	}
	if ( m_bOpenUdpOk )
	{
		long	m_lLen = lWireLen;
		EnterCriticalSection(&m_csMem);
		char * pTransfer = (char*)m_clMemMng.Malloc();
		if ( pTransfer == NULL )
		{
			CIceRPCPushLog::Instance().WriteLog("JSONPC","Pushdata","申请内存失败:%p, memsize=%d\r\n",pTransfer,m_clMemMng.GetMaxSize());
			LeaveCriticalSection(&m_csMem);
			return CPushMng::m_lPushPackCrowded;
		}
 		LeaveCriticalSection(&m_csMem);


		char * transfer_buf		= pTransfer +sizeof(ST_UDP_HEADER);
		ST_UDP_HEADER * pUDPHead = (ST_UDP_HEADER*)pTransfer;
		ST_UDP_HEADER &	UDPHeader= *pUDPHead;
		if ( m_lLen < MAX_CACHE_BUFLEN )
		{
			if (m_lLen > 0)
			{
				memcpy(transfer_buf, pWireData, m_lLen);
			}
		}
		else
		{
			m_lLen = 0;
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "数据包太大，丢弃\r\n");
		}

		// 创建应答包头
		UDPHeader.chCompressed	= bPacketSnappy ? EN_COMPRESS_SNAPPY : EN_COMPRESS_DEFAULT;

		UDPHeader.req				= SafeLongLongToLength<unsigned short>(p_lReqNo);// 这个值不能太大  PACKET_PUSH_HQ_SUB;//pNode->nGNID;

		long nLeft = sizeof(ST_UDP_HEADER)+m_lLen;
		CPushMng::CreateNewPushObj()->UDPPush(pTransfer,nLeft);
		CPushMng::ReleaseIt();

		EnterCriticalSection(&m_csMem);
		m_clMemMng.Free(pTransfer);
 		LeaveCriticalSection(&m_csMem);


		return CPushMng::m_lPushPackCrowded;
	}
	std::map<unsigned long long,ST_JSON_BIN_HANDLE*>	copyclients;
	const bool bFilterBySubscription = p_lReqNo > 0x100;
	const DWORD dwReqNo = bFilterBySubscription ?
		SafeLongLongToLength<DWORD>(p_lReqNo) : 0;
	EnterCriticalSection(&m_csLock);
	std::map<unsigned long long,ST_JSON_BIN_HANDLE*>::iterator it = m_mapClients.begin();
	while ( it != m_mapClients.end() )
	{
		// Read and replace mapSubId under the same lock. Only selected handles
		// receive a reference, so filtered clients cannot leak one per push.
		if (bFilterBySubscription &&
			!CSubscriptionFilter::IsSubscribed(
				it->second->mapSubId, dwReqNo))
		{
			++it;
			continue;
		}
		it->second->Ref();	// 先增加计数器
		copyclients[it->first] = it->second;
		++it;
	}
	LeaveCriticalSection(&m_csLock);
//#if _DEBUG
//	CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "ICE客户端数目:%d\r\n",m_mapClients.size());
//#endif
	std::vector<unsigned long long>	aDelKey;
	it = copyclients.begin();
	while ( it != copyclients.end() )
	{
		//////////////////////////////////////////////////////////////////////////
		try
		{
			if ( p_bIsAsync )
			{
				InterlockedIncrement64(&CPushMng::m_lPushPackCrowded);
				// 同时在接收端改成进入队列，不立刻处理，避免上层处理风险； 发送端也控制风险，如果网络不好，发布过去，多的直接丢弃
// #ifdef _DEBUG
// 				if ( it->second->dwCount < 1024*10 )	// 如果太多包积压发不出去，暂时不要跟这个链接发送数据了
// #else
// 				if ( it->second->dwCount < 1024*20 )	// 如果太多包积压发不出去，暂时不要跟这个链接发送数据了
// #endif
				{
					CPushCallbackPtr cb = std::make_shared<CPushCallback>(it->first,this);
					Ice::AsyncResultPtr r= icecompat::begin_ProcessPackage(it->second->refProxy.value(),p_lReqNo,stPayload,
						newCallback_IJsonBinRPC_ProcessPackage(cb,&CPushCallback::response, &CPushCallback::exception, &CPushCallback::sent));
					//it->second->dwCount++;
				}
// 				else
// 				{
// 					CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "\r\n%m_hSocket 积压=%d,系统积压=%I64d主动丢包\r\n",it->second->strGuid.c_str(),it->second->dwCount,CPushMng::m_lPushPackCrowded);
// 					if ( time(NULL) - it->second->tmLive > 30 )
// 					{
// 						aDelKey.push_back(it->first);
// 					}
// 				}
			}
			else
				it->second->refProxy->ProcessPackage(p_lReqNo,stPayload);
		}
		catch(const IceUtil::Exception& ex)
		{
			char	str[1024]={0};
			UTF82ASC(ex.what(),str,1023);
			SelfDelClient(it->first);
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "%m_hSocket , %m_hSocket\r\n",ex.ice_id(),str);//ex.what());
		}
		++it;
	}
	//////////////////////////////////////////////////////////////////////////
	it = copyclients.begin();
	while ( it != copyclients.end() )
	{
		it->second->ReleaseIt();	// 释放计数器
		++it;
	}
	// 超时，并且发送积压严重，没有任何返回的链接删除
	for ( int i=0;i<aDelKey.size();++i )
	{
		SelfDelClient(aDelKey[i]);
	}
	return CPushMng::m_lPushPackCrowded;
}


//////////////////////////////////////////////////////////////////////////
// 客户端模式
// IceGridAdmin 的xml不支持.Proxy  传入参数要一步到位
//////////////////////////////////////////////////////////////////////////
// 客户端从配置文件启动，并根据配置打开 TCP/UDP/组播快速推送通道。
bool CJsonBinRPCImp::StartByClientWithLocator(const char * p_szCfgFile,const char * p_szProxyProperty,HANDLE & p_hSem,int p_iThreadPool)
{
	char	tmpval[256]={0};
	m_strProxyProperty = p_szProxyProperty;
	// char	szproxy[256];
	std::optional<Ice::ObjectPrx> prx;
	// snprintf(szproxy, sizeof(szproxy), "%s.Proxy", p_szProxyProperty);
	if ( !m_refCommunicatorClient )
	{
		std::string refProxy = p_szProxyProperty;
		try
		{
			int argc = 0;
			Ice::InitializationData initData;
			initData.properties = Ice::createProperties();
			ST_XML_CONFIG_DATA clConfig;
			LoadIcePropertiesFromConfig(p_szCfgFile, initData.properties, &clConfig);
			ApplyClientThreadPool(initData.properties, p_iThreadPool);
			m_bAsyncWaitCompleted = GetConfigInt(&clConfig, p_szCfgFile, "ICEPUSH", "AsyncWaitCompleted", 1) != 0;
			LoadBinaryConfig(&clConfig, p_szCfgFile);
			CIceRPCPushLog::Instance().ApplyConfig(p_szCfgFile, &clConfig);
			// Ice 3.8 initialize 会移动 initData，定位器日志仍需要原属性对象。
			Ice::PropertiesPtr refProperties = initData.properties;
			m_refCommunicatorClient = Ice::initialize(std::move(initData));
			// Push.iPort 要单独指定	: "0" tcp ； “1” udp
			// Push.strIp
// 			m_strPushPort = initData.properties->getProperty("Push.iPort");	// 指定推送模式
// 			m_strPushIp = initData.properties->getProperty("Push.strIp");	// 指定推送模式
			// 客户端推送地址支持 XML 和旧 INI 两种配置来源。
			m_strPushPort = std::to_string(GetConfigInt(&clConfig, p_szCfgFile, "ICEPUSH", "RPCPushport", 0));
			m_strPushIp = GetConfigString(&clConfig, p_szCfgFile, "ICEPUSH", "RPCPuship", "");
			if ( atol(m_strPushPort.c_str()) > 0 && !m_strPushIp.empty() )
			{
				m_bOpenTcp = true;
			}
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "Client ICE Push:open=%d,%m_hSocket\r\n",m_bOpenTcp, p_szCfgFile);
			//////////////////////////////////////////////////////////////////////////
			m_strPushUdpPort = std::to_string(GetConfigInt(&clConfig, p_szCfgFile, "ICEPUSH", "RPCXPushport", 0));
			m_strPushUdpIp = GetConfigString(&clConfig, p_szCfgFile, "ICEPUSH", "RPCXPuship", "");
			if ( atol(m_strPushUdpPort.c_str()) > 0 && !m_strPushUdpIp.empty() )
			{
				m_bOpenUdp = true;
			}
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "Client X Push:open=%d,%m_hSocket\r\n",m_bOpenUdp, p_szCfgFile);
			//////////////////////////////////////////////////////////////////////////
			m_strPushUdpMulPort = std::to_string(GetConfigInt(&clConfig, p_szCfgFile, "ICEPUSH", "RPCMPushport", 0));
			m_strPushUdpMulIp = GetConfigString(&clConfig, p_szCfgFile, "ICEPUSH", "RPCMPuship", "");
			if ( atol(m_strPushUdpMulPort.c_str()) > 0 && !m_strPushUdpMulIp.empty() )
			{
				m_bOpenMul = true;
			}
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "Client M Push:open=%d,%m_hSocket\r\n",m_bOpenMul, p_szCfgFile);

			std::string locator = "Ice.Default.Locator";
			std::string locatorValue = refProperties->getProperty(locator);
			std::string configuredProxy = refProperties->getProperty(p_szProxyProperty);
			if (!configuredProxy.empty())
			{
				refProxy = configuredProxy;
			}
			CIceRPCPushLog::Instance().WriteLog("配置", "StartByClientWithLocator",
				"proxy=%m_hSocket,locator=%m_hSocket", refProxy.c_str(), locatorValue.c_str());

			//////////////////////////////////////////////////////////////			
			// XML 中存在同名 Property 时使用属性值，缺失时兼容调用方直接传入完整代理字符串。
			prx = m_refCommunicatorClient->stringToProxy(refProxy);
			if (prx) { prx = prx->ice_twoway(); }
			// Returns a new refProxy with the given timeout value in milliseconds. A value of  disable -1 m_hSocket timeouts.
			//prx = prx->ice_timeout(15000);	// 数据大了，时间不够用
			m_refStockIo = Ice::uncheckedCast<JSONBINRPC::IJsonBinRPCPrx>(prx);
		}
		catch(const IceUtil::Exception& ex)
		{
			char	str[1024]={0};
			UTF82ASC(ex.what(),str,1023);
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "%m_hSocket  %m_hSocket:%m_hSocket\r\n",refProxy.c_str(),ex.ice_id(),str);//ex.what());
			m_strError = "ICE_CLIENT_START_EXCEPTION: proxy=" + refProxy + ", ice=" + ex.ice_id() + ", what=" + ex.what();
			return false;
		}
	}
	if ( m_semMiddle==NULL )
	{
		m_semMiddle		= CreateSemaphore( NULL,0,0x7fffffff,NULL );
	}
	p_hSem = m_semMiddle;
	CPushMng::CreateNewPushObj();
	CPushMng::PushHandle(this);
	if (m_bOpenTcp)
	{
		CPushMng::PushConnectInfo(SOCK_STREAM,m_strPushIp, m_strPushPort);
	}
	if (m_bOpenUdp)
	{
		CPushMng::PushConnectInfo(SOCK_DGRAM,m_strPushUdpIp, m_strPushUdpPort);
	}
	if ( m_bOpenMul )
	{
		CPushMng::PushConnectInfo(SOCK_RAW,m_strPushUdpMulIp,m_strPushUdpMulPort);
	}
// 	m_hSemPush		= CreateSemaphore( NULL,0,0x7fffffff,NULL );
// 	if ( m_hPushPack == NULL )
// 	{
// 		m_hPushPack	= CreateThread(NULL,0,s_PushThread,this,0,NULL);
// 	}
	return true;
}
// 客户端从属性数组启动，适合调用方动态生成 Ice 配置。
bool CJsonBinRPCImp::StartByClientWithProperty(int p_iNum,const char * p_aPropertyKey[],const char * p_aProperty[],const char * p_szProxyProperty,HANDLE & p_hSem,int p_iThreadPool)
{
	char	tmpval[256]={0};
	m_strProxyProperty = p_szProxyProperty;
	// char	szproxy[256];
	std::optional<Ice::ObjectPrx> prx;
	// snprintf(szproxy, sizeof(szproxy), "%s.Proxy", p_szProxyProperty);
	if ( !m_refCommunicatorClient )
	{
		std::string refProxy = p_szProxyProperty;
		try
		{
			int argc = 0;
			Ice::InitializationData initData;
			initData.properties = Ice::createProperties();
			for ( int i=0;i<p_iNum;++i )
			{
				initData.properties->setProperty(p_aPropertyKey[i],p_aProperty[i]);
				if ( stricmp(p_aPropertyKey[i],"RPCPushport")==0 )
				{
					m_strPushPort		= p_aProperty[i];
				}
				else if ( stricmp(p_aPropertyKey[i],"RPCPuship")==0 )
				{
					m_strPushIp		= p_aProperty[i];
				}
				else if ( stricmp(p_aPropertyKey[i],"AsyncWaitCompleted")==0 )
				{
					m_bAsyncWaitCompleted = atoi(p_aProperty[i]) != 0;
				}
				//////////////////////////////////////////////////////////////////////////
				else if ( stricmp(p_aPropertyKey[i],"RPCXPuship")==0 )
				{
					m_strPushUdpIp		= p_aProperty[i];
				}
				else if ( stricmp(p_aPropertyKey[i],"RPCXPushport")==0 )
				{
					m_strPushUdpPort	= p_aProperty[i];
				}
				//////////////////////////////////////////////////////////////////////////
				else if ( stricmp(p_aPropertyKey[i],"RPCMPuship")==0 )
				{
					m_strPushUdpMulIp		= p_aProperty[i];
				}
				else if ( stricmp(p_aPropertyKey[i],"RPCMPushport")==0 )
				{
					m_strPushUdpMulPort	= p_aProperty[i];
				}
				//////////////////////////////////////////////////////////////////////////

			}
			ApplyClientThreadPool(initData.properties, p_iThreadPool);
			m_refCommunicatorClient = Ice::initialize(std::move(initData));
			// Push.iPort 要单独指定	: "0" tcp ； “1” udp
			// Push.strIp
			Ice::PropertiesPtr prop = Ice::createProperties();

// 			m_strPushPort = initData.properties->getProperty("Push.iPort");	// 指定推送模式
// 			m_strPushIp = initData.properties->getProperty("Push.strIp");	// 指定推送模式
			if ( atol(m_strPushPort.c_str()) > 0 && !m_strPushIp.empty() )
			{
				m_bOpenTcp = true;
				CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "Open Client ICE Push:%m_hSocket,%m_hSocket\r\n", m_strPushIp.c_str(), m_strPushPort.c_str());
			}
			if ( atol(m_strPushUdpPort.c_str()) > 0 && !m_strPushUdpIp.empty() )
			{
				m_bOpenUdp = true;
				CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "Open Client ICE X Push:%m_hSocket,%m_hSocket\r\n", m_strPushUdpIp.c_str(), m_strPushUdpPort.c_str());
			}
			if ( atol(m_strPushUdpMulPort.c_str()) > 0 && !m_strPushUdpMulIp.empty() )
			{
				m_bOpenMul = true;
				CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "Open Client ICE M Push:%m_hSocket,%m_hSocket\r\n", m_strPushUdpMulIp.c_str(), m_strPushUdpMulPort.c_str());
			}
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "Client ICE Push:open=%d,%d\r\n",m_bOpenTcp,m_bOpenUdp);
			prx = m_refCommunicatorClient->propertyToProxy(p_szProxyProperty);
			if (prx) { prx = prx->ice_twoway(); }		// 一步到位 stringToProxy (propertyToProxy )
			m_refStockIo = Ice::uncheckedCast<JSONBINRPC::IJsonBinRPCPrx>(prx);
		}
		catch(const IceUtil::Exception& ex)
		{
			char	str[1024]={0};
			UTF82ASC(ex.what(),str,1023);
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "%m_hSocket  %m_hSocket,%m_hSocket\r\n",refProxy.c_str(),ex.ice_id(),str);//ex.what());
			m_strError = "ICE_CLIENT_START_EXCEPTION: proxy=" + refProxy + ", ice=" + ex.ice_id() + ", what=" + ex.what();
			return false;
		}
	}
	if ( m_semMiddle==NULL )
	{
		m_semMiddle		= CreateSemaphore( NULL,0,0x7fffffff,NULL );
	}
	p_hSem = m_semMiddle;
	CPushMng::CreateNewPushObj();
	// 此时还没有建立连接
	if (m_bOpenTcp)
	{
		CPushMng::PushConnectInfo(SOCK_STREAM,m_strPushIp, m_strPushPort);
	}
	if (m_bOpenUdp)
	{
		CPushMng::PushConnectInfo(SOCK_DGRAM,m_strPushUdpIp, m_strPushUdpPort);
	}
	if (m_bOpenMul )
	{
		CPushMng::PushConnectInfo(SOCK_RAW,m_strPushUdpMulIp,m_strPushUdpMulPort);
	}
// 	m_hSemPush		= CreateSemaphore( NULL,0,0x7fffffff,NULL );
// 	if ( m_hPushPack == NULL )
// 	{
// 		m_hPushPack	= CreateThread(NULL,0,s_PushThread,this,0,NULL);
// 	}
	return true;
}
// 注册回调函数；同时可以注册或者注销
// 客户端注册推送回调，必要时创建双向 Ice callback servant 并解析服务端返回配置。
CJsonBinRPCImp * CJsonBinRPCImp::RegisterClient(std::string& p_strRet,const char * p_strGuid,const char *p_strSubInfo,func_JsonICEPushClientPack p_pfnCallback,int p_iIsReg/*=1*/,void * p_pParam)
{
	m_strClientGuid = p_strGuid;
	// 如果自己是服务端,提供注册函数指针即可
	// 推送是单独new的实例，收到的pack、包，统一放到父类m_pParentImp,所以注册函数，this也要注册好，m_pStockPushIo其实只是收
	if ( p_iIsReg)
		CPushMng::UpdateCallBack(p_pfnCallback,p_pParam,false);
	else
		CPushMng::UpdateCallBack(p_pfnCallback,p_pParam,true);
	if ( !IsClientMode() )	// 如果是服务端模式（客户端，服务端只能一个模式）
	{
		m_bRegisterOk = true;
		return this;
	}
	//////////////////////////////////////////////////////////////////////////
	try
	{
		char	idinfo[256],clientendpoint[256];
		snprintf(idinfo, sizeof(idinfo), "%s.Client.%s", m_strProxyProperty.c_str(), p_strGuid);
		// Ice 3.8 会把 Service.Client.Endpoints 误判为服务端 Adapter 的未知子属性，改用独立 ServiceClient 前缀。
		snprintf(clientendpoint, sizeof(clientendpoint), "%sClient", m_strProxyProperty.c_str());
		Ice::Identity	iceID = Ice::stringToIdentity(idinfo);	// 仅仅标识符
		Ice::ConnectionPtr pconnect =  m_refStockIo->ice_getConnection();	// 如果连接不上，尽量提前触发异常，防止多次new不必要实例
		if ( p_iIsReg)
		{
			if ( !m_refAdapterClient )
			{
				// 回调功能接口读取 ServiceClient.Endpoints，避免和服务端 Service.Endpoints 属性前缀冲突。
				m_refAdapterClient = m_refCommunicatorClient->createObjectAdapter(clientendpoint);
				// 回调指针保存好
				CJsonBinRPCImp * pStockPush = new CJsonBinRPCImp();//(f,fbyhead,p_pParam);	// 智能指针
				//pStockPush->UpdateCallBack(f,p_pParam,false);
				m_refAdapterClient->add(icecompat::MakeServantPtr(pStockPush), iceID);//m_refCommunicatorClient->stringToIdentity("WithCapitalio.Pushio.Client.PushioReceiver"));
				m_refAdapterClient->activate();

				m_pStockPushIo = pStockPush;
				m_pStockPushIo->m_pParentImp = this;	// 父类
				m_pStockPushIo->m_refStockIo = m_refStockIo;
				// 用原来链接:可能第一次连不上
				//Ice::ConnectionPtr pconnect =  m_refStockIo->ice_getConnection();
				pconnect->setAdapter(m_refAdapterClient);
			}
			else
			{
				//m_pStockPushIo->UpdateCallBack(f,p_pParam);
				//m_refAdapterClient->remove(iceID);
				//m_refAdapterClient->add(m_pStockPushIo, iceID);//m_refCommunicatorClient->stringToIdentity("WithCapitalio.Pushio.Client.PushioReceiver"));
				m_refAdapterClient->activate();
			}
			// 用原来链接:可能第一次连不上
			pconnect->setAdapter(m_refAdapterClient);
			// 注册回调接口,返回告知是否采用了snappy压缩
// 			m_bSnappy = m_refStockIo->RegisterStockPushIO(p_strGuid,iceID);
// 			if( m_pStockPushIo )
// 				m_pStockPushIo->m_bSnappy = m_bSnappy;
			if ( p_strSubInfo )	// 如果有订阅的详细内容，阻塞获得服务端的配置信息，例如推送的端口信息
			{
				p_strRet = m_refStockIo->RegisterStockPushIO2(p_strGuid,p_strSubInfo,iceID);
			}
			else
			{
				// 异步注册
				// Ice::AsyncResultPtr m_pResult= m_refStockIo->begin_RegisterStockPushIO(p_strGuid,iceID);
				CRegisterIOCallBackPtr	cb = std::make_shared<CRegisterIOCallBack>();
				cb->m_lpParent = this;
				JSONBINRPC::Callback_IJsonBinRPC_RegisterStockPushIOPtr<CRegisterIOCallBack>	ProcessCB = 
					JSONBINRPC::newCallback_IJsonBinRPC_RegisterStockPushIO(cb,
					&CRegisterIOCallBack::ice_response, 
					&CRegisterIOCallBack::exception, 
					&CRegisterIOCallBack::sent);
				icecompat::begin_RegisterStockPushIO(m_refStockIo.value(),p_strGuid,iceID,ProcessCB);

			}
		}
		else
		{
			if ( p_strSubInfo )	// 如果有订阅的详细内容，阻塞获得服务端的配置信息，例如推送的端口信息
			{
				p_strRet = m_refStockIo->UnRegisterStockPushIO2(p_strGuid,p_strSubInfo,iceID);
			}
			else
			{
				//m_pStockPushIo->UpdateCallBack(f,p_pParam,true);
				// m_bsanppy = m_refStockIo->UnRegisterStockPushIO(p_strGuid);
				icecompat::begin_UnRegisterStockPushIO(m_refStockIo.value(),p_strGuid);	// 注销的时候，异步操作，能完成即完成，不能则快速返回，方便上层退出
				if( m_pStockPushIo )
					m_pStockPushIo->m_bSnappy = m_bSnappy;
			}
		}
	}
	catch(const IceUtil::Exception& ex)
	{
		char	str[1024]={0};
		UTF82ASC(ex.what(),str,1023);
		CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "%m_hSocket,%m_hSocket\r\n",ex.ice_id(),str);//ex.what());
		CIceRPCPushLog::Instance().WriteLog("Error","RegisterClient","%s_%m_hSocket",ex.ice_id(),str);
	}
	if ( p_strRet.length() > 0 )
	{	// RegisterJsonICEClient2(m_ice, "testice", "subxx=1;p_strIp=127.0.0.1|192.168.0.12;p_strPort=999", JsonICEPushClientPack, 1);
		std::vector<std::string>	toids;
		transform(p_strRet.begin(), p_strRet.end(), p_strRet.begin(), ::tolower);  
		TokenizeOR(toids,p_strRet.c_str(),";");
		for ( int i=0;i<toids.size();++i )
		{
			if ( SundayQuickSearch(toids[i].c_str(),"snappy=") )
			{
				m_bSnappy = atoi(toids[i].c_str() + strlen("snappy=")) != 0;
			}
			else if ( SundayQuickSearch(toids[i].c_str(),"rpcpushbindport=") )
			{
				m_strPushPort = toids[i].c_str() + strlen("rpcpushbindport=");
			}
			else if ( SundayQuickSearch(toids[i].c_str(),"rpcpushbindip=") )
			{
				m_strPushIp = toids[i].c_str() + strlen("rpcpushbindip=");
			}
			else if ( SundayQuickSearch(toids[i].c_str(),"rpcxpushbindport=") )
			{
				m_strPushUdpPort = toids[i].c_str() + strlen("rpcxpushbindport=");
			}
			else if ( SundayQuickSearch(toids[i].c_str(),"rpcxpushbindip=") )
			{
				m_strPushUdpIp = toids[i].c_str() + strlen("rpcxpushbindip=");
			}
		}
		if ( atol(m_strPushPort.c_str()) > 0 && !m_strPushIp.empty() )
		{
			m_bOpenTcp = true;
			CPushMng::PushConnectInfo(SOCK_STREAM,m_strPushIp, m_strPushPort);
		}
		if (atol(m_strPushUdpPort.c_str()) > 0 )
		{
			CPushMng::PushConnectInfo(SOCK_DGRAM,m_strPushUdpIp, m_strPushUdpPort);
		}
		if ( atol(m_strPushUdpMulPort.c_str()) > 0 )
		{
			CPushMng::PushConnectInfo(SOCK_RAW,m_strPushUdpMulIp,m_strPushUdpMulPort);
		}
		CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "Client ICE Push:open=%d,%m_hSocket:%m_hSocket\r\n",m_bOpenTcp,m_strPushIp.c_str(),m_strPushPort.c_str());
		if (p_iIsReg && m_pStockPushIo != NULL)
		{
			m_bRegisterOk = true;
			m_pStockPushIo->m_bRegisterOk = true;
			m_pStockPushIo->m_bSnappy = m_bSnappy;
		}
	}
	
	if (m_bOpenTcp)
	{
		CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "订阅:%m_hSocket\r\n",p_strGuid);
		CPushMng::PushRegisterInfo(p_strGuid,p_iIsReg);
	}

	return m_pStockPushIo;
}
// 注册异步返回后同步本对象和推送子对象的压缩/注册状态。
void CJsonBinRPCImp::RegisterCallBack(long long p_lRetVal)
{
	m_bRegisterOk = true;
	m_bSnappy = (bool)p_lRetVal;
	if( m_pStockPushIo )
	{
		m_pStockPushIo->m_bRegisterOk = m_bRegisterOk;
		m_pStockPushIo->m_bSnappy = m_bSnappy;
	}
}

// 客户端收到 Ice 推送后的入口，只深拷贝自描述载荷并入队，解压和业务回调由推送线程完成。
void CJsonBinRPCImp::ProcessPackage_async(const ::JSONBINRPC::AMD_IJsonBinRPC_ProcessPackagePtr& p_pCallback,long long p_lReqNo, const ::JSONBINRPC::BinaryPayload& p_stPayload, const ::Ice::Current& /* = ::Ice::Current() */)
{
	if ( !m_bRegisterOk )
	{
		CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "没有注册成功，拒绝接收推送\r\n");
		p_pCallback->ice_response();
		return;
	}

	ST_PACK_QUEUE * p = new (std::nothrow) ST_PACK_QUEUE;
	if (p == NULL)
	{
		CIceRPCPushLog::Instance().WriteLog("JSONRPC", "PUSHPACK",
			"BINARY_PUSH_MEMORY_FAILED: req=%I64d\r\n", p_lReqNo);
		p_pCallback->ice_response();
		return;
	}
	p->lReqNo = p_lReqNo;
	p->stPayload = p_stPayload;
	p->iProtocolVersion = m_iBinaryProtocolVersion;
	p->iMaxPayloadBytes = m_iBinaryMaxPayloadBytes;
	CPushMng::PushPack(p);
	
	// 提前返回
	p_pCallback->ice_response();
}





// 读取 Binary 协议配置；所有上限都在此处收敛，避免非法配置放大资源占用。
void CJsonBinRPCImp::LoadBinaryConfig(const ST_XML_CONFIG_DATA* p_pConfig, const char* p_szCfgFile)
{
	m_iBinaryProtocolVersion = GetConfigInt(p_pConfig, p_szCfgFile, "ICEPUSH", "BinaryProtocolVersion", 1);
	if (m_iBinaryProtocolVersion != s_iBinaryProtocolVersion)
	{
		m_iBinaryProtocolVersion = s_iBinaryProtocolVersion;
	}
	m_iBinaryCompressionThresholdBytes = GetConfigInt(p_pConfig, p_szCfgFile, "ICEPUSH", "BinaryCompressionThresholdBytes", 1024);
	if (m_iBinaryCompressionThresholdBytes < 0 || m_iBinaryCompressionThresholdBytes > 1024 * 1024)
	{
		m_iBinaryCompressionThresholdBytes = 1024;
	}
	m_iBinaryMaxPayloadBytes = GetConfigInt(p_pConfig, p_szCfgFile, "ICEPUSH", "BinaryMaxPayloadBytes", 50 * 1024 * 1024);
	if (m_iBinaryMaxPayloadBytes <= 0 || m_iBinaryMaxPayloadBytes > 512 * 1024 * 1024)
	{
		m_iBinaryMaxPayloadBytes = 50 * 1024 * 1024;
	}
	m_iBinaryCallTimeoutMs = GetConfigInt(p_pConfig, p_szCfgFile, "ICEPUSH", "BinaryCallTimeoutMs", 15000);
	if (m_iBinaryCallTimeoutMs <= 0 || m_iBinaryCallTimeoutMs > 10 * 60 * 1000)
	{
		m_iBinaryCallTimeoutMs = 15000;
	}
	m_iBinaryServerWorkerThreads = GetConfigInt(p_pConfig, p_szCfgFile, "ICEPUSH", "BinaryServerWorkerThreads", 4);
	if (m_iBinaryServerWorkerThreads <= 0 || m_iBinaryServerWorkerThreads > 64)
	{
		m_iBinaryServerWorkerThreads = 4;
	}
	m_iBinaryServerQueueCapacity = GetConfigInt(p_pConfig, p_szCfgFile, "ICEPUSH", "BinaryServerQueueCapacity", 4096);
	if (m_iBinaryServerQueueCapacity <= 0 || m_iBinaryServerQueueCapacity > 1000000)
	{
		m_iBinaryServerQueueCapacity = 4096;
	}
	m_iBinaryResponseWorkerThreads = GetConfigInt(p_pConfig, p_szCfgFile,
		"ICEPUSH", "BinaryResponseWorkerThreads", 8);
	if (m_iBinaryResponseWorkerThreads <= 0 ||
		m_iBinaryResponseWorkerThreads > 64)
	{
		m_iBinaryResponseWorkerThreads = 8;
	}
	m_iBinaryResponseQueueCapacity = GetConfigInt(p_pConfig, p_szCfgFile,
		"ICEPUSH", "BinaryResponseQueueCapacity", 4096);
	if (m_iBinaryResponseQueueCapacity < m_iBinaryServerQueueCapacity ||
		m_iBinaryResponseQueueCapacity > 1000000)
	{
		m_iBinaryResponseQueueCapacity = m_iBinaryServerQueueCapacity;
	}
	m_iBinaryReclaimWorkerThreads = GetConfigInt(p_pConfig, p_szCfgFile,
		"ICEPUSH", "BinaryReclaimWorkerThreads", 2);
	if (m_iBinaryReclaimWorkerThreads <= 0 ||
		m_iBinaryReclaimWorkerThreads > 16)
	{
		m_iBinaryReclaimWorkerThreads = 2;
	}
	m_iBinaryMaxPendingAsync = GetConfigInt(p_pConfig, p_szCfgFile, "ICEPUSH", "BinaryMaxPendingAsync", 10000);
	if (m_iBinaryMaxPendingAsync <= 0 || m_iBinaryMaxPendingAsync > 1000000)
	{
		m_iBinaryMaxPendingAsync = 10000;
	}
}

// 把业务原始字节编码成自描述 BinaryPayload，小包不压缩，大包仅在压缩有效时使用 Snappy。
bool CJsonBinRPCImp::EncodeBinaryPayload(const ST_BINARY_VIEW& p_refView, ::JSONBINRPC::BinaryPayload& p_refPayload, std::string& p_refError) const
{
	return binarypayload::Encode(p_refView.pBuffer, p_refView.lLen,
		m_iBinaryProtocolVersion, m_iBinaryCompressionThresholdBytes,
		m_iBinaryMaxPayloadBytes, p_refPayload, p_refError);
}

// 校验版本、压缩类型和原始长度后解码，拒绝超限载荷和不完整 Snappy 数据。
bool CJsonBinRPCImp::DecodeBinaryPayload(const ::JSONBINRPC::BinaryPayload& p_refPayload, std::vector<unsigned char>& p_refBuffer, std::string& p_refError) const
{
	return binarypayload::Decode(p_refPayload,
		m_iBinaryProtocolVersion, m_iBinaryMaxPayloadBytes,
		p_refBuffer, p_refError);
}

// 校验客户端公共结构并构造 Ice 请求，主载荷和扩展载荷合计不得超过配置上限。
bool CJsonBinRPCImp::BuildBinaryRequest(const ST_BINARY_CALL* p_pCall, ::JSONBINRPC::BinaryRequest& p_refRequest, std::string& p_refError) const
{
	if (p_pCall == NULL)
	{
		p_refError = "INVALID_PARAM: binary call is null";
		return false;
	}
	if (p_pCall->iVersion != m_iBinaryProtocolVersion)
	{
		p_refError = "BINARY_PROTOCOL_VERSION_UNSUPPORTED: call version does not match local version";
		return false;
	}
	const long long lTotalBytes = static_cast<long long>(p_pCall->stPayload.lLen) + static_cast<long long>(p_pCall->stExtra.lLen);
	if (p_pCall->stPayload.lLen < 0 || p_pCall->stExtra.lLen < 0 || lTotalBytes > m_iBinaryMaxPayloadBytes)
	{
		p_refError = "BINARY_PAYLOAD_TOO_LARGE: request payload total exceeds BinaryMaxPayloadBytes";
		return false;
	}
	p_refRequest.version = m_iBinaryProtocolVersion;
	p_refRequest.syn = p_pCall->lSynId;
	p_refRequest.nFuncID = p_pCall->lFuncId;
	p_refRequest.routeCode = p_pCall->lRouteCode;
	p_refRequest.param = p_pCall->lParam;
	p_refRequest.wparam = p_pCall->wParam;
	return EncodeBinaryPayload(p_pCall->stPayload, p_refRequest.payload, p_refError) &&
		EncodeBinaryPayload(p_pCall->stExtra, p_refRequest.extra, p_refError);
}

// 构造统一的传输错误应答，错误文本只使用详细英文描述。
::JSONBINRPC::BinaryResponse CJsonBinRPCImp::MakeBinaryErrorResponse(int p_iErrorCode, const std::string& p_strError) const
{
	::JSONBINRPC::BinaryResponse stResponse;
	stResponse.version = m_iBinaryProtocolVersion;
	stResponse.retVal = p_iErrorCode;
	stResponse.errorCode = p_iErrorCode;
	stResponse.param = 0;
	stResponse.wparam = 0;
	stResponse.errInfo = p_strError;
	stResponse.payload.version = m_iBinaryProtocolVersion;
	stResponse.payload.compression = ::JSONBINRPC::BinaryCompression::BinaryCompressionNone;
	stResponse.payload.rawSize = 0;
	stResponse.payload.data.clear();
	stResponse.extra = stResponse.payload;
	return stResponse;
}

// 深拷贝并解码 Ice 应答，调用方后续统一通过 BinaryResultFree 释放。
ST_BINARY_RESULT* CJsonBinRPCImp::BuildBinaryResult(const ::JSONBINRPC::BinaryResponse& p_refResponse, void* p_pParam, std::string& p_refError) const
{
	ST_BINARY_RESULT* pResult = new ST_BINARY_RESULT;
	pResult->iVersion = p_refResponse.version;
	pResult->lRetVal = p_refResponse.retVal;
	pResult->iErrorCode = p_refResponse.errorCode;
	pResult->lParam = p_refResponse.param;
	pResult->wParam = p_refResponse.wparam;
	pResult->pParam = p_pParam;
	strncpy(pResult->szErrInfo, p_refResponse.errInfo.c_str(), sizeof(pResult->szErrInfo) - 1);

	std::vector<unsigned char> vecPayload;
	std::vector<unsigned char> vecExtra;
	if (!DecodeBinaryPayload(p_refResponse.payload, vecPayload, p_refError) ||
		!DecodeBinaryPayload(p_refResponse.extra, vecExtra, p_refError))
	{
		pResult->lRetVal = s_iBinaryCompressionError;
		pResult->iErrorCode = s_iBinaryCompressionError;
		strncpy(pResult->szErrInfo, p_refError.c_str(), sizeof(pResult->szErrInfo) - 1);
		return pResult;
	}
	if (!vecPayload.empty())
	{
		pResult->stPayload.lLen = SafeSizeToLength<int>(vecPayload.size());
		pResult->stPayload.pBuffer = new unsigned char[vecPayload.size()];
		memcpy(pResult->stPayload.pBuffer, vecPayload.data(), vecPayload.size());
	}
	if (!vecExtra.empty())
	{
		pResult->stExtra.lLen = SafeSizeToLength<int>(vecExtra.size());
		pResult->stExtra.pBuffer = new unsigned char[vecExtra.size()];
		memcpy(pResult->stExtra.pBuffer, vecExtra.data(), vecExtra.size());
	}
	return pResult;
}

// 移动 Ice BinaryPayload 字节容器，CloudNet/Gateway 可把解压推迟到最终消费线程。
ST_BINARY_ENCODED_RESULT* CJsonBinRPCImp::BuildBinaryEncodedResult(
	::JSONBINRPC::BinaryResponse&& p_refResponse,
	ST_BINARY_ENCODED_RESULT* p_pPreparedResult, void* p_pParam,
	std::string& p_refError) const
{
	p_refError.clear();
	ST_BINARY_ENCODED_RESULT_OWNER* pOwner =
		p_pPreparedResult != nullptr ?
		static_cast<ST_BINARY_ENCODED_RESULT_OWNER*>(
			p_pPreparedResult->pInternalOwner) : nullptr;
	if (p_pPreparedResult == nullptr || pOwner == nullptr)
	{
		p_refError =
			"BINARY_RESPONSE_OWNER_INVALID: prepared result has no owner";
		return nullptr;
	}
	try
	{
		pOwner->aPayload = std::move(p_refResponse.payload.data);
		pOwner->aExtra = std::move(p_refResponse.extra.data);
		p_pPreparedResult->iVersion = p_refResponse.version;
		p_pPreparedResult->lRetVal = p_refResponse.retVal;
		p_pPreparedResult->iErrorCode = p_refResponse.errorCode;
		p_pPreparedResult->lParam = p_refResponse.param;
		p_pPreparedResult->wParam = p_refResponse.wparam;
		p_pPreparedResult->pParam = p_pParam;
		strncpy(p_pPreparedResult->szErrInfo, p_refResponse.errInfo.c_str(),
			sizeof(p_pPreparedResult->szErrInfo) - 1);
		FillBinaryEncodedBuffer(p_refResponse.payload, pOwner->aPayload,
			m_iBinaryMaxPayloadBytes, p_pPreparedResult->stPayload);
		FillBinaryEncodedBuffer(p_refResponse.extra, pOwner->aExtra,
			m_iBinaryMaxPayloadBytes, p_pPreparedResult->stExtra);
		return p_pPreparedResult;
	}
	catch (...)
	{
		p_refError =
			"BINARY_RESPONSE_MEMORY_FAILED: unable to move encoded result";
		pOwner->aPayload.clear();
		pOwner->aExtra.clear();
		p_pPreparedResult->iVersion = m_iBinaryProtocolVersion;
		p_pPreparedResult->lRetVal = s_iBinaryCallbackError;
		p_pPreparedResult->iErrorCode = s_iBinaryCallbackError;
		p_pPreparedResult->lParam = 0;
		p_pPreparedResult->wParam = 0;
		p_pPreparedResult->pParam = p_pParam;
		strncpy(p_pPreparedResult->szErrInfo, p_refError.c_str(),
			sizeof(p_pPreparedResult->szErrInfo) - 1);
		p_pPreparedResult->stPayload = ST_BINARY_ENCODED_BUFFER();
		p_pPreparedResult->stExtra = ST_BINARY_ENCODED_BUFFER();
		return p_pPreparedResult;
	}
}

void CJsonBinRPCImp::FreeBinaryEncodedResult(
	ST_BINARY_ENCODED_RESULT* p_pResult)
{
	if (p_pResult == nullptr)
	{
		return;
	}
	ST_BINARY_ENCODED_RESULT_OWNER* pOwner =
		static_cast<ST_BINARY_ENCODED_RESULT_OWNER*>(
			p_pResult->pInternalOwner);
	p_pResult->pInternalOwner = nullptr;
	delete pOwner;
	delete p_pResult;
}

// 注册 Binary 回调时才启动工作线程，未使用 Binary 的旧服务不会增加线程开销。
void CJsonBinRPCImp::RegisterBinaryServerCallback(func_IceBinaryServerCallback p_pfnCallback, void* p_pParam)
{
	m_pfnBinaryServerCallback = p_pfnCallback;
	m_pfnBinaryServerCallbackEx = NULL;
	m_pfnBinaryPriorityClassifier = NULL;
	m_pBinaryServerParam = p_pParam;
	if (p_pfnCallback != NULL)
	{
		StartBinaryWorkers();
	}
	else
	{
		StopBinaryWorkers();
	}
}

void CJsonBinRPCImp::RegisterBinaryServerCallbackEx(
	func_IceBinaryServerCallbackEx p_pfnCallback,
	func_IceBinaryPriorityClassifier p_pfnPriorityClassifier,
	void* p_pParam)
{
	m_pfnBinaryServerCallback = NULL;
	m_pfnBinaryServerCallbackEx = p_pfnCallback;
	m_pfnBinaryPriorityClassifier = p_pfnPriorityClassifier;
	m_pBinaryServerParam = p_pParam;
	if (p_pfnCallback != NULL)
	{
		StartBinaryWorkers();
	}
	else
	{
		StopBinaryWorkers();
	}
}

HANDLE CJsonBinRPCImp::RegisterBinaryResponseTask(
	const std::shared_ptr<ST_BINARY_SERVER_TASK>& p_refTask)
{
	if (!p_refTask || p_refTask->pOwner == NULL)
	{
		return NULL;
	}
	CJsonBinRPCImp* pOwner = p_refTask->pOwner;
	long long lPending = pOwner->m_lBinaryPendingResponses.load();
	for (;;)
	{
		if (lPending >= pOwner->m_iBinaryServerQueueCapacity)
		{
			return NULL;
		}
		if (pOwner->m_lBinaryPendingResponses.compare_exchange_weak(
				lPending, lPending + 1))
		{
			break;
		}
	}
	ST_BINARY_RESPONSE_CONTEXT* pContext = NULL;
	try
	{
		pContext = new ST_BINARY_RESPONSE_CONTEXT(p_refTask);
		p_refTask->pResponseContext = pContext;
		p_refTask->hResponse = reinterpret_cast<HANDLE>(pContext);
		std::lock_guard<std::mutex> clLock(
			pOwner->m_clBinaryPendingContextMutex);
		pOwner->m_setBinaryPendingContexts.insert(pContext);
	}
	catch (...)
	{
		delete pContext;
		p_refTask->pResponseContext = NULL;
		p_refTask->hResponse = NULL;
		pOwner->m_lBinaryPendingResponses.fetch_sub(1);
		return NULL;
	}
	return reinterpret_cast<HANDLE>(pContext);
}

void CJsonBinRPCImp::RetainBinaryResponseContext(
	ST_BINARY_RESPONSE_CONTEXT* p_pContext)
{
	if (p_pContext != NULL)
	{
		p_pContext->lReferences.fetch_add(1);
	}
}

void CJsonBinRPCImp::ReleaseBinaryResponseContext(
	ST_BINARY_RESPONSE_CONTEXT* p_pContext)

{
	if (p_pContext == NULL || p_pContext->lReferences.fetch_sub(1) != 1)
	{
		return;
	}
	CJsonBinRPCImp* pOwner = p_pContext->pOwner;
	if (pOwner != NULL)
	{
		pOwner->m_lBinaryPendingResponses.fetch_sub(1);
	}
	delete p_pContext;
}

bool CJsonBinRPCImp::QueueBinaryResponseTask(
	ST_BINARY_RESPONSE_CONTEXT* p_pContext)
{
	if (p_pContext == NULL || p_pContext->pOwner != this ||
		!p_pContext->refTask)
	{
		return false;
	}
	{
		std::lock_guard<std::mutex> clTask(p_pContext->refTask->clMutex);
		if (p_pContext->refTask->bResponseQueued)
		{
			return false;
		}
		p_pContext->refTask->bResponseQueued = true;
	}
	{
		std::lock_guard<std::mutex> clPending(m_clBinaryPendingContextMutex);
		m_setBinaryPendingContexts.erase(p_pContext);
	}
	try
	{
		std::lock_guard<std::mutex> clQueue(m_clBinaryResponseQueueMutex);
		if (m_bBinaryResponseWorkerStop.load() ||
			m_dequeBinaryResponses.size() >=
				static_cast<std::size_t>(m_iBinaryResponseQueueCapacity))
		{
			return false;
		}
		m_dequeBinaryResponses.push_back(p_pContext);
	}
	catch (...)
	{
		return false;
	}
	m_clBinaryResponseQueueCondition.notify_one();
	return true;
}

void CJsonBinRPCImp::StartBinaryWorkers()
{
	if (!m_vecBinaryWorkers.empty())
	{
		return;
	}
	m_bBinaryReclaimWorkerStop.store(false);
	for (int iIndex = 0; iIndex < m_iBinaryReclaimWorkerThreads; ++iIndex)
	{
		m_vecBinaryReclaimWorkers.push_back(std::thread(
			&CJsonBinRPCImp::BinaryReclaimWorker, this));
	}
	m_bBinaryResponseWorkerStop.store(false);
	for (int iIndex = 0; iIndex < m_iBinaryResponseWorkerThreads; ++iIndex)
	{
		m_vecBinaryResponseWorkers.push_back(std::thread(
			&CJsonBinRPCImp::BinaryResponseWorker, this));
	}
	m_bBinaryWorkerStop.store(false);
	for (int iIndex = 0; iIndex < m_iBinaryServerWorkerThreads; ++iIndex)
	{
		m_vecBinaryWorkers.push_back(std::thread(&CJsonBinRPCImp::BinaryWorker, this));
	}
}

void CJsonBinRPCImp::StopBinaryWorkers()
{
	std::deque<std::shared_ptr<ST_BINARY_SERVER_TASK>> dequePending;
	std::deque<std::shared_ptr<ST_BINARY_SERVER_TASK>> dequeControlPending;
	{
		std::lock_guard<std::mutex> clLock(m_clBinaryQueueMutex);
		m_bBinaryWorkerStop.store(true);
		dequePending.swap(m_dequeBinaryTasks);
		dequeControlPending.swap(m_dequeBinaryControlTasks);
	}
	m_clBinaryQueueCondition.notify_all();
	for (size_t uIndex = 0; uIndex < m_vecBinaryWorkers.size(); ++uIndex)
	{
		if (m_vecBinaryWorkers[uIndex].joinable())
		{
			m_vecBinaryWorkers[uIndex].join();
		}
	}
	m_vecBinaryWorkers.clear();
	dequePending.insert(dequePending.end(), dequeControlPending.begin(),
		dequeControlPending.end());
	while (!dequePending.empty())
	{
		std::shared_ptr<ST_BINARY_SERVER_TASK> pTask = dequePending.front();
		dequePending.pop_front();
		{
			std::lock_guard<std::mutex> clTask(pTask->clMutex);
			pTask->stResponse = MakeBinaryErrorResponse(s_iBinaryStateError,
				"BINARY_SERVICE_STOPPED: server is stopping");
			pTask->bCompleted = true;
			pTask->bCallbackReturned = true;
			pTask->bDeferred = false;
			pTask->bOwnedResult = false;
		}
		if (!QueueBinaryResponseTask(pTask->pResponseContext))
		{
			QueueBinaryReclaimTask(pTask->pResponseContext);
		}
	}
	CompleteStoppedBinaryTasks(this);
	{
		std::lock_guard<std::mutex> clLock(m_clBinaryResponseQueueMutex);
		m_bBinaryResponseWorkerStop.store(true);
	}
	m_clBinaryResponseQueueCondition.notify_all();
	for (std::thread& refThread : m_vecBinaryResponseWorkers)
	{
		if (refThread.joinable())
		{
			refThread.join();
		}
	}
	m_vecBinaryResponseWorkers.clear();
	{
		std::lock_guard<std::mutex> clLock(m_clBinaryReclaimQueueMutex);
		m_bBinaryReclaimWorkerStop.store(true);
	}
	m_clBinaryReclaimQueueCondition.notify_all();
	for (std::thread& refThread : m_vecBinaryReclaimWorkers)
	{
		if (refThread.joinable())
		{
			refThread.join();
		}
	}
	m_vecBinaryReclaimWorkers.clear();
}

void CJsonBinRPCImp::CompleteStoppedBinaryTasks(CJsonBinRPCImp* p_pOwner)
{
	std::vector<ST_BINARY_RESPONSE_CONTEXT*> vecStopped;
	{
		std::lock_guard<std::mutex> clPending(
			p_pOwner->m_clBinaryPendingContextMutex);
		for (ST_BINARY_RESPONSE_CONTEXT* pContext :
			p_pOwner->m_setBinaryPendingContexts)
		{
			vecStopped.push_back(pContext);
		}
	}
	for (ST_BINARY_RESPONSE_CONTEXT* pContext : vecStopped)
	{
		if (pContext == NULL || !pContext->refTask)
		{
			continue;
		}
		std::shared_ptr<ST_BINARY_SERVER_TASK> pTask = pContext->refTask;
		{
			std::lock_guard<std::mutex> clTask(pTask->clMutex);
			if (!pTask->bResponseQueued)
			{
				pTask->stResponse = p_pOwner->MakeBinaryErrorResponse(
					s_iBinaryStateError,
					"BINARY_SERVICE_STOPPED: deferred request cancelled");
				pTask->bCompleted = true;
				pTask->bCallbackReturned = true;
				pTask->bDeferred = true;
				pTask->bOwnedResult = false;
			}
		}
		if (!pTask->bResponseQueued &&
			!p_pOwner->QueueBinaryResponseTask(pContext))
		{
			p_pOwner->QueueBinaryReclaimTask(pContext);
		}
	}
}

// 请求入队只执行常量级操作，保证 Ice 派发线程不会被业务处理阻塞。
void CJsonBinRPCImp::QueueBinaryTask(bool p_bPut, ::JSONBINRPC::BinaryRequest p_stRequest, std::function<void(const ::JSONBINRPC::BinaryResponse&)> p_fnResponse, std::function<void(std::exception_ptr)> p_fnException)
{
	if ((m_pfnBinaryServerCallback == NULL &&
		m_pfnBinaryServerCallbackEx == NULL) || m_bBinaryWorkerStop.load())
	{
		p_fnResponse(MakeBinaryErrorResponse(s_iBinaryStateError, "BINARY_HANDLER_NOT_REGISTERED: binary server callback is not registered"));
		return;
	}
	std::shared_ptr<ST_BINARY_SERVER_TASK> pTask = std::make_shared<ST_BINARY_SERVER_TASK>();
	pTask->pOwner = this;
	pTask->bPut = p_bPut;
	pTask->stRequest = std::move(p_stRequest);
	pTask->fnResponse = std::move(p_fnResponse);
	pTask->fnException = std::move(p_fnException);
	pTask->iProtocolVersion = m_iBinaryProtocolVersion;
	pTask->iCompressionThresholdBytes =
		m_iBinaryCompressionThresholdBytes;
	pTask->iMaxPayloadBytes = m_iBinaryMaxPayloadBytes;
	bool bControl = false;
	if (m_pfnBinaryPriorityClassifier != NULL)
	{
		try
		{
			bControl = m_pfnBinaryPriorityClassifier(m_pBinaryServerParam,
				pTask->stRequest.nFuncID) != 0;
		}
		catch (...)
		{
			bControl = false;
		}
	}
	bool bQueueFull = false;
	bool bHandleFailed = false;
	{
		std::lock_guard<std::mutex> clLock(m_clBinaryQueueMutex);
		const std::size_t szCapacity = static_cast<std::size_t>(
			m_iBinaryServerQueueCapacity);
		const std::size_t szControlReserve = szCapacity > 1U ?
			(std::max)(static_cast<std::size_t>(1U), szCapacity / 16U) : 0U;
		const std::size_t szTotal = m_dequeBinaryTasks.size() +
			m_dequeBinaryControlTasks.size();
		const bool bFull = szTotal >= szCapacity ||
			(!bControl && m_dequeBinaryTasks.size() >=
				szCapacity - szControlReserve);
		if (bFull)
		{
			bQueueFull = true;
		}
		else if (RegisterBinaryResponseTask(pTask) == NULL)
		{
			bHandleFailed = true;
		}
		else if (bControl)
		{
			m_dequeBinaryControlTasks.push_back(pTask);
		}
		else
		{
			m_dequeBinaryTasks.push_back(pTask);
		}
	}
	if (bQueueFull || bHandleFailed)
	{
		pTask->fnResponse(MakeBinaryErrorResponse(
			bQueueFull ? s_iBinaryQueueFull : s_iBinaryStateError,
			bQueueFull ?
			"BINARY_QUEUE_FULL: server binary request queue reached capacity" :
			"BINARY_RESPONSE_HANDLE_FAILED: unable to allocate response handle"));
		return;
	}
	m_clBinaryQueueCondition.notify_one();
}

// BinaryResponseData 只复制结果，真正的 Ice response 在业务回调返回后由工作线程发送。
bool CJsonBinRPCImp::CompleteBinaryResponse(HANDLE p_hResponse, const ST_BINARY_RESULT* p_pResult)
{
	return CompleteBinaryResponseTask(p_hResponse, p_pResult);
}

bool CJsonBinRPCImp::CompleteBinaryResponseTask(HANDLE p_hResponse,
	const ST_BINARY_RESULT* p_pResult)
{
	if (p_hResponse == NULL || p_pResult == NULL)
	{
		return false;
	}
	ST_BINARY_RESPONSE_CONTEXT* pContext =
		reinterpret_cast<ST_BINARY_RESPONSE_CONTEXT*>(p_hResponse);
	if (pContext->lReferences.load() <= 0 || pContext->pOwner == NULL ||
		!pContext->refTask)
	{
		return false;
	}
	std::shared_ptr<ST_BINARY_SERVER_TASK> pTask = pContext->refTask;
	bool bQueueResponse = false;
	{
		std::lock_guard<std::mutex> clTask(pTask->clMutex);
		if (pTask->bCompleted || pTask->bResponseQueued ||
			pTask->pOwner == NULL)
		{
			return false;
		}
		const long long lTotalBytes = static_cast<long long>(
			p_pResult->stPayload.lLen) + static_cast<long long>(
				p_pResult->stExtra.lLen);
		if (p_pResult->stPayload.lLen < 0 ||
			p_pResult->stExtra.lLen < 0 || lTotalBytes < 0 ||
			lTotalBytes > pTask->iMaxPayloadBytes ||
			(p_pResult->stPayload.lLen > 0 &&
				p_pResult->stPayload.pBuffer == NULL) ||
			(p_pResult->stExtra.lLen > 0 &&
				p_pResult->stExtra.pBuffer == NULL))
		{
			pTask->stResponse = pTask->pOwner->MakeBinaryErrorResponse(
				s_iBinaryPayloadTooLarge,
				"BINARY_RESPONSE_INVALID: response payload is invalid");
			pTask->bOwnedResult = false;
		}
		else
		{
			try
			{
				pTask->lResultRetVal = p_pResult->lRetVal;
				pTask->iResultErrorCode = p_pResult->iErrorCode;
				pTask->lResultParam = p_pResult->lParam;
				pTask->lResultWParam = p_pResult->wParam;
				pTask->strResultError = p_pResult->szErrInfo;
				if (p_pResult->stPayload.lLen > 0)
				{
					pTask->vecResultPayload.assign(
						p_pResult->stPayload.pBuffer,
						p_pResult->stPayload.pBuffer +
							p_pResult->stPayload.lLen);
				}
				if (p_pResult->stExtra.lLen > 0)
				{
					pTask->vecResultExtra.assign(
						p_pResult->stExtra.pBuffer,
						p_pResult->stExtra.pBuffer +
							p_pResult->stExtra.lLen);
				}
				pTask->bOwnedResult = true;
			}
			catch (...)
			{
				pTask->vecResultPayload.clear();
				pTask->vecResultExtra.clear();
				pTask->stResponse = pTask->pOwner->MakeBinaryErrorResponse(
					s_iBinaryStateError,
					"BINARY_RESPONSE_MEMORY_FAILED: unable to own result buffers");
				pTask->bOwnedResult = false;
			}
		}
		pTask->bCompleted = true;
		if (pTask->bCallbackReturned && pTask->bDeferred)
		{
			bQueueResponse = true;
		}
	}
	if (bQueueResponse && !pTask->pOwner->QueueBinaryResponseTask(pContext))
	{
		pTask->pOwner->QueueBinaryReclaimTask(pContext);
		return false;
	}
	return true;
}

// 应答线程从直绑定上下文取得完整结果，在业务 Worker 之外执行压缩和 Ice 回调。
void CJsonBinRPCImp::BinaryResponseWorker()
{
	for (;;)
	{
		ST_BINARY_RESPONSE_CONTEXT* pContext = NULL;
		{
			std::unique_lock<std::mutex> clLock(m_clBinaryResponseQueueMutex);
			m_clBinaryResponseQueueCondition.wait(clLock, [this]() {
				return m_bBinaryResponseWorkerStop.load() ||
					!m_dequeBinaryResponses.empty();
			});
			if (m_bBinaryResponseWorkerStop.load() &&
				m_dequeBinaryResponses.empty())
			{
				return;
			}
			pContext = m_dequeBinaryResponses.front();
			m_dequeBinaryResponses.pop_front();
		}
		if (pContext == NULL || !pContext->refTask)
		{
			QueueBinaryReclaimTask(pContext);
			continue;
		}
		std::shared_ptr<ST_BINARY_SERVER_TASK> pTask = pContext->refTask;
		::JSONBINRPC::BinaryResponse stSendResponse;
		if (pTask->bOwnedResult)
		{
			stSendResponse.version = pTask->iProtocolVersion;
			stSendResponse.retVal = pTask->lResultRetVal;
			stSendResponse.errorCode = pTask->iResultErrorCode;
			stSendResponse.param = pTask->lResultParam;
			stSendResponse.wparam = pTask->lResultWParam;
			stSendResponse.errInfo = pTask->strResultError;
			ST_BINARY_VIEW stPayload;
			stPayload.lLen = SafeSizeToLength<int>(
				pTask->vecResultPayload.size());
			stPayload.pBuffer = pTask->vecResultPayload.empty() ? NULL :
				pTask->vecResultPayload.data();
			ST_BINARY_VIEW stExtra;
			stExtra.lLen = SafeSizeToLength<int>(pTask->vecResultExtra.size());
			stExtra.pBuffer = pTask->vecResultExtra.empty() ? NULL :
				pTask->vecResultExtra.data();
			std::string strError;
			if (!EncodeBinaryPayload(stPayload, stSendResponse.payload,
					strError) || !EncodeBinaryPayload(stExtra,
					stSendResponse.extra, strError))
			{
				stSendResponse = MakeBinaryErrorResponse(
					s_iBinaryCompressionError, strError.empty() ?
					"BINARY_RESPONSE_ENCODE_FAILED: unable to encode result" :
					strError);
			}
		}
		else
		{
			stSendResponse = pTask->stResponse;
		}
		try
		{
			if (pTask->fnResponse)
			{
				pTask->fnResponse(stSendResponse);
			}
		}
		catch (...)
		{
			if (pTask->fnException)
			{
				pTask->fnException(std::current_exception());
			}
		}
		QueueBinaryReclaimTask(pContext);
	}
}

void CJsonBinRPCImp::QueueBinaryReclaimTask(
	ST_BINARY_RESPONSE_CONTEXT* p_pContext)
{
	if (p_pContext == NULL)
	{
		return;
	}
	try
	{
		std::lock_guard<std::mutex> clLock(m_clBinaryReclaimQueueMutex);
		if (!m_bBinaryReclaimWorkerStop.load() &&
			m_dequeBinaryReclaims.size() <
				static_cast<std::size_t>(m_iBinaryResponseQueueCapacity))
		{
			m_dequeBinaryReclaims.push_back(p_pContext);
			m_clBinaryReclaimQueueCondition.notify_one();
			return;
		}
	}
	catch (...)
	{
	}
	// 内存不足或停机末尾必须安全兜底，不能为了异步形式泄漏资源。
	ReleaseBinaryResponseContext(p_pContext);
}

void CJsonBinRPCImp::BinaryReclaimWorker()
{
	for (;;)
	{
		ST_BINARY_RESPONSE_CONTEXT* pContext = NULL;
		{
			std::unique_lock<std::mutex> clLock(m_clBinaryReclaimQueueMutex);
			m_clBinaryReclaimQueueCondition.wait(clLock, [this]() {
				return m_bBinaryReclaimWorkerStop.load() ||
					!m_dequeBinaryReclaims.empty();
			});
			if (m_bBinaryReclaimWorkerStop.load() &&
				m_dequeBinaryReclaims.empty())
			{
				return;
			}
			pContext = m_dequeBinaryReclaims.front();
			m_dequeBinaryReclaims.pop_front();
		}
		ReleaseBinaryResponseContext(pContext);
	}
}

// 工作线程完成解压、业务回调和应答，慢业务不会占用 Ice 派发线程。
void CJsonBinRPCImp::BinaryWorker()
{
	for (;;)
	{
		std::shared_ptr<ST_BINARY_SERVER_TASK> pTask;
		{
			std::unique_lock<std::mutex> clLock(m_clBinaryQueueMutex);
			m_clBinaryQueueCondition.wait(clLock, [this]() {
				return m_bBinaryWorkerStop.load() ||
					!m_dequeBinaryControlTasks.empty() ||
					!m_dequeBinaryTasks.empty();
			});
			if (m_bBinaryWorkerStop.load() && m_dequeBinaryTasks.empty() &&
				m_dequeBinaryControlTasks.empty())
			{
				return;
			}
			if (!m_dequeBinaryControlTasks.empty())
			{
				pTask = m_dequeBinaryControlTasks.front();
				m_dequeBinaryControlTasks.pop_front();
			}
			else
			{
				pTask = m_dequeBinaryTasks.front();
				m_dequeBinaryTasks.pop_front();
			}
		}

		std::string strError;
		bool bCallbackFailed = false;
		::JSONBINRPC::BinaryResponse stCallbackError;
		int iDisposition = EN_BINARY_SERVER_CALLBACK_COMPLETED;
		const long long lTotalBytes = static_cast<long long>(pTask->stRequest.payload.rawSize) + static_cast<long long>(pTask->stRequest.extra.rawSize);
		if (pTask->stRequest.version != m_iBinaryProtocolVersion || lTotalBytes < 0 || lTotalBytes > m_iBinaryMaxPayloadBytes ||
			!DecodeBinaryPayload(pTask->stRequest.payload, pTask->vecPayload, strError) ||
			!DecodeBinaryPayload(pTask->stRequest.extra, pTask->vecExtra, strError))
		{
			pTask->stResponse = MakeBinaryErrorResponse(s_iBinaryProtocolError, strError.empty() ? "BINARY_REQUEST_INVALID: request validation failed" : strError);
			pTask->bCompleted = true;
		}
		else
		{
			ST_BINARY_REQUEST stRequest;
			stRequest.iVersion = pTask->stRequest.version;
			stRequest.lSynId = pTask->stRequest.syn;
			stRequest.lFuncId = pTask->stRequest.nFuncID;
			stRequest.lRouteCode = pTask->stRequest.routeCode;
			stRequest.lParam = pTask->stRequest.param;
			stRequest.wParam = pTask->stRequest.wparam;
			stRequest.chMode = pTask->bPut ? EN_BINARY_INPUT_PUT : EN_BINARY_INPUT_RPC;
			stRequest.hResponse = pTask->hResponse;
			stRequest.stPayload.lLen = SafeSizeToLength<int>(pTask->vecPayload.size());
			stRequest.stPayload.pBuffer = pTask->vecPayload.empty() ? NULL : pTask->vecPayload.data();
			stRequest.stExtra.lLen = SafeSizeToLength<int>(pTask->vecExtra.size());
			stRequest.stExtra.pBuffer = pTask->vecExtra.empty() ? NULL : pTask->vecExtra.data();
			try
			{
				if (m_pfnBinaryServerCallbackEx != NULL)
				{
					iDisposition = m_pfnBinaryServerCallbackEx(
						m_pBinaryServerParam, &stRequest);
				}
				else
				{
					m_pfnBinaryServerCallback(m_pBinaryServerParam, &stRequest);
				}
			}
			catch (const std::exception& ex)
			{
				stCallbackError = MakeBinaryErrorResponse(s_iBinaryCallbackError,
					std::string("BINARY_CALLBACK_EXCEPTION: what=") + ex.what());
				bCallbackFailed = true;
			}
			catch (...)
			{
				stCallbackError = MakeBinaryErrorResponse(s_iBinaryCallbackError,
					"BINARY_CALLBACK_EXCEPTION: unknown exception");
				bCallbackFailed = true;
			}
		}
		bool bQueueResponse = false;
		{
			std::lock_guard<std::mutex> clTask(pTask->clMutex);
			pTask->bCallbackReturned = true;
			if (bCallbackFailed && !pTask->bCompleted)
			{
				pTask->stResponse = std::move(stCallbackError);
				pTask->bCompleted = true;
			}
			pTask->bDeferred = iDisposition ==
				EN_BINARY_SERVER_CALLBACK_DEFERRED;
			if (iDisposition != EN_BINARY_SERVER_CALLBACK_COMPLETED &&
				iDisposition != EN_BINARY_SERVER_CALLBACK_DEFERRED)
			{
				pTask->bDeferred = false;
				pTask->stResponse = MakeBinaryErrorResponse(
					s_iBinaryCallbackError,
					"BINARY_CALLBACK_RESULT_INVALID: callback returned an unsupported disposition");
				pTask->bCompleted = true;
			}
			if (!pTask->bDeferred && !pTask->bCompleted)
			{
				pTask->stResponse = MakeBinaryErrorResponse(
					s_iBinaryCallbackError,
					"BINARY_RESPONSE_MISSING: callback returned without BinaryResponseData");
				pTask->bCompleted = true;
			}
			if (pTask->bCompleted && !pTask->bResponseQueued)
			{
				bQueueResponse = true;
			}
		}
		if (!bQueueResponse)
		{
			continue;
		}
		if (!QueueBinaryResponseTask(pTask->pResponseContext))
		{
			QueueBinaryReclaimTask(pTask->pResponseContext);
		}
	}
}

// 同步 Binary 调用直接使用 Ice 3.8 proxy，并应用独立调用超时。
ST_BINARY_RESULT* CJsonBinRPCImp::BinaryCall(const ST_BINARY_CALL* p_pCall, bool p_bPut)
{
	::JSONBINRPC::BinaryRequest stRequest;
	std::string strError;
	if (!BuildBinaryRequest(p_pCall, stRequest, strError))
	{
		m_strError = strError;
		return NULL;
	}
	::JSONBINRPC::IJsonBinRPCPrx clProxy = ClientIO().ice_invocationTimeout(m_iBinaryCallTimeoutMs);
	::JSONBINRPC::BinaryResponse stResponse = p_bPut ? clProxy.BinaryPUT(stRequest) : clProxy.BinaryRPC(stRequest);
	return BuildBinaryResult(stResponse, NULL, strError);
}

// 异步 Binary 调用真正非阻塞，达到积压上限时在发起 Ice 调用前拒绝。
long long CJsonBinRPCImp::BinaryCallAsync(const ST_BINARY_CALL* p_pCall,
	bool p_bPut, LPTHREAD_START_ROUTINE p_pfnCallback, void* p_pParam,
	int p_iTimeoutMs, bool p_bExplicitTimeout)
{
	::JSONBINRPC::BinaryRequest stRequest;
	std::string strError;
	if (p_pfnCallback == NULL || !BuildBinaryRequest(p_pCall, stRequest, strError))
	{
		m_strError = p_pfnCallback == NULL ? "INVALID_PARAM: binary async callback is null" : strError;
		return p_pfnCallback == NULL ? s_iBinaryProtocolError : InferBinaryProtocolErrorCode(strError);
	}
	const long long lPending = m_lBinaryPendingAsync.fetch_add(1) + 1;
	if (lPending > m_iBinaryMaxPendingAsync)
	{
		m_lBinaryPendingAsync.fetch_sub(1);
		m_strError = "BINARY_PENDING_FULL: client async pending count reached limit";
		return s_iBinaryPendingFull;
	}
	try
	{
		// Ex 接口的 0 明确表示无限等待；普通接口仍使用连接默认超时。
		const int iTimeoutMs = p_bExplicitTimeout ?
			(p_iTimeoutMs == 0 ? -1 : p_iTimeoutMs) :
			m_iBinaryCallTimeoutMs;
		::JSONBINRPC::IJsonBinRPCPrx clProxy =
			ClientIO().ice_invocationTimeout(iTimeoutMs);
		auto fnResponse = [this, p_pfnCallback, p_pParam](::JSONBINRPC::BinaryResponse p_stResponse)
		{
			std::string strBuildError;
			ST_BINARY_RESULT* pResult = BuildBinaryResult(p_stResponse, p_pParam, strBuildError);
			try
			{
				p_pfnCallback(pResult);
			}
			catch (...)
			{
				if (pResult != NULL)
				{
					delete[] pResult->stPayload.pBuffer;
					delete[] pResult->stExtra.pBuffer;
					delete pResult;
				}
			}
			m_lBinaryPendingAsync.fetch_sub(1);
		};
		auto fnException = [this, p_pfnCallback, p_pParam](std::exception_ptr p_refException)
		{
			const std::string strDetail = MakeBinaryExceptionDetail("BinaryCallAsync", p_refException);
			std::string strBuildError;
			ST_BINARY_RESULT* pResult = BuildBinaryResult(MakeBinaryErrorResponse(s_iBinaryCallTimeout, strDetail), p_pParam, strBuildError);
			try
			{
				p_pfnCallback(pResult);
			}
			catch (...)
			{
				if (pResult != NULL)
				{
					delete[] pResult->stPayload.pBuffer;
					delete[] pResult->stExtra.pBuffer;
					delete pResult;
				}
			}
			m_lBinaryPendingAsync.fetch_sub(1);
		};
		if (p_bPut)
		{
			clProxy.BinaryPUTAsync(stRequest, fnResponse, fnException);
		}
		else
		{
			clProxy.BinaryRPCAsync(stRequest, fnResponse, fnException);
		}
	}
	catch (...)
	{
		m_lBinaryPendingAsync.fetch_sub(1);
		throw;
	}
	return lPending;
}

// 编码异步接口只移动 BinaryPayload 所有权，不在 Ice 回调线程解压或复制正文。
long long CJsonBinRPCImp::BinaryCallAsyncEncoded(
	const ST_BINARY_CALL* p_pCall, bool p_bPut,
	LPTHREAD_START_ROUTINE p_pfnCallback, void* p_pParam,
	int p_iTimeoutMs, bool p_bExplicitTimeout)
{
	::JSONBINRPC::BinaryRequest stRequest;
	std::string strError;
	if (p_pfnCallback == nullptr ||
		!BuildBinaryRequest(p_pCall, stRequest, strError))
	{
		m_strError = p_pfnCallback == nullptr ?
			"INVALID_PARAM: encoded binary async callback is null" : strError;
		return p_pfnCallback == nullptr ? s_iBinaryProtocolError :
			InferBinaryProtocolErrorCode(strError);
	}
	const long long lPending = m_lBinaryPendingAsync.fetch_add(1) + 1;
	if (lPending > m_iBinaryMaxPendingAsync)
	{
		m_lBinaryPendingAsync.fetch_sub(1);
		m_strError =
			"BINARY_PENDING_FULL: client async pending count reached limit";
		return s_iBinaryPendingFull;
	}
	ST_BINARY_ENCODED_RESULT* pPreparedResult =
		new (std::nothrow) ST_BINARY_ENCODED_RESULT;
	ST_BINARY_ENCODED_RESULT_OWNER* pPreparedOwner =
		new (std::nothrow) ST_BINARY_ENCODED_RESULT_OWNER;
	if (pPreparedResult == nullptr || pPreparedOwner == nullptr)
	{
		delete pPreparedOwner;
		delete pPreparedResult;
		m_lBinaryPendingAsync.fetch_sub(1);
		m_strError =
			"BINARY_RESPONSE_MEMORY_FAILED: unable to prepare encoded result";
		return s_iBinaryCallbackError;
	}
	pPreparedResult->pParam = p_pParam;
	pPreparedResult->pInternalOwner = pPreparedOwner;

	try
	{
		const int iTimeoutMs = p_bExplicitTimeout ?
			(p_iTimeoutMs == 0 ? -1 : p_iTimeoutMs) :
			m_iBinaryCallTimeoutMs;
		::JSONBINRPC::IJsonBinRPCPrx clProxy =
			ClientIO().ice_invocationTimeout(iTimeoutMs);
		auto fnResponse = [this, p_pfnCallback, p_pParam, pPreparedResult](
			::JSONBINRPC::BinaryResponse p_stResponse)
		{
			std::string strBuildError;
			ST_BINARY_ENCODED_RESULT* pResult = BuildBinaryEncodedResult(
				std::move(p_stResponse), pPreparedResult, p_pParam,
				strBuildError);
			try
			{
				p_pfnCallback(pResult);
			}
			catch (...)
			{
				FreeBinaryEncodedResult(pResult);
			}
			m_lBinaryPendingAsync.fetch_sub(1);
		};
		auto fnException = [this, p_pfnCallback, p_pParam, pPreparedResult](
			std::exception_ptr p_refException)
		{
			const std::string strDetail = MakeBinaryExceptionDetail(
				"BinaryCallAsyncEncoded", p_refException);
			std::string strBuildError;
			ST_BINARY_ENCODED_RESULT* pResult = BuildBinaryEncodedResult(
				MakeBinaryErrorResponse(s_iBinaryCallTimeout, strDetail),
				pPreparedResult, p_pParam, strBuildError);
			try
			{
				p_pfnCallback(pResult);
			}
			catch (...)
			{
				FreeBinaryEncodedResult(pResult);
			}
			m_lBinaryPendingAsync.fetch_sub(1);
		};
		if (p_bPut)
		{
			clProxy.BinaryPUTAsync(stRequest, fnResponse, fnException);
		}
		else
		{
			clProxy.BinaryRPCAsync(stRequest, fnResponse, fnException);
		}
	}
	catch (...)
	{
		FreeBinaryEncodedResult(pPreparedResult);
		m_lBinaryPendingAsync.fetch_sub(1);
		throw;
	}
	return lPending;
}

// Ice 3.8 二进制 RPC 入口，只负责把请求转入服务端工作队列。
void CJsonBinRPCImp::BinaryRPCAsync(::JSONBINRPC::BinaryRequest p_stRequest, std::function<void(const ::JSONBINRPC::BinaryResponse&)> p_fnResponse, std::function<void(std::exception_ptr)> p_fnException, const Ice::Current& p_stCurrent)
{
	QueueBinaryTask(false, std::move(p_stRequest), std::move(p_fnResponse), std::move(p_fnException));
}

// Ice 3.8 二进制 PUT 入口，和 RPC 保持一致的限流、解压和错误处理。
void CJsonBinRPCImp::BinaryPUTAsync(::JSONBINRPC::BinaryRequest p_stRequest, std::function<void(const ::JSONBINRPC::BinaryResponse&)> p_fnResponse, std::function<void(std::exception_ptr)> p_fnException, const Ice::Current& p_stCurrent)
{
	QueueBinaryTask(true, std::move(p_stRequest), std::move(p_fnResponse), std::move(p_fnException));
}

// Ice 3.8 生成代码要求的注册入口，内部继续复用旧 AMD 处理流程。
void CJsonBinRPCImp::RegisterStockPushIOAsync(std::string p_strGuid, ::Ice::Identity p_stIdent, std::function<void(std::int64_t)> p_fnResponse, std::function<void(std::exception_ptr)> p_fnException, const Ice::Current& p_stCurrent)
{
	::JSONBINRPC::AMD_IJsonBinRPC_RegisterStockPushIOPtr cb = std::make_shared<::JSONBINRPC::AMD_IJsonBinRPC_RegisterStockPushIO>(p_fnResponse, p_fnException);
	RegisterStockPushIO_async(cb, p_strGuid, p_stIdent, p_stCurrent);
}

// Ice 3.8 RPC 入口，转接到旧 AMD 队列逻辑以保持服务端处理语义一致。
void CJsonBinRPCImp::JsonBinRPCAsync(std::int64_t p_lSynId, std::int64_t p_lFuncId, std::int64_t p_lSetCode, ::JSONBINRPC::AByte p_stReqJson, std::function<void(std::int64_t, std::int64_t, const ::JSONBINRPC::AByte&, std::int64_t, const ::JSONBINRPC::AByte&, std::string_view)> p_fnResponse, std::function<void(std::exception_ptr)> p_fnException, const Ice::Current& p_stCurrent)
{
	::JSONBINRPC::AMD_IJsonBinRPC_JsonBinRPCPtr cb = std::make_shared<::JSONBINRPC::AMD_IJsonBinRPC_JsonBinRPC>(p_fnResponse, p_fnException);
	JsonBinRPC_async(cb, p_lSynId, p_lFuncId, p_lSetCode, p_stReqJson, p_stCurrent);
}

// Ice 3.8 PUT 入口，转接到旧 AMD 队列逻辑并保留二进制参数压缩约定。
void CJsonBinRPCImp::JsonBinPUTAsync(std::int64_t p_lSynId, std::int64_t p_lFuncId, std::int64_t p_lSetCode, ::JSONBINRPC::AByte p_stPutJson, std::int64_t p_lParam, ::JSONBINRPC::AByte p_stLParam, std::int64_t p_lWParam, ::JSONBINRPC::AByte p_stWParam, std::function<void(std::int64_t, std::int64_t, const ::JSONBINRPC::AByte&, std::int64_t, const ::JSONBINRPC::AByte&, std::string_view)> p_fnResponse, std::function<void(std::exception_ptr)> p_fnException, const Ice::Current& p_stCurrent)
{
	::JSONBINRPC::AMD_IJsonBinRPC_JsonBinPUTPtr cb = std::make_shared<::JSONBINRPC::AMD_IJsonBinRPC_JsonBinPUT>(p_fnResponse, p_fnException);
	JsonBinPUT_async(cb, p_lSynId, p_lFuncId, p_lSetCode, p_stPutJson, p_lParam, p_stLParam, p_lWParam, p_stWParam, p_stCurrent);
}

// Ice 3.8 注销入口，内部继续使用旧注销逻辑清理客户端映射。
void CJsonBinRPCImp::UnRegisterStockPushIOAsync(std::string p_strGuid, std::function<void(std::int64_t)> p_fnResponse, std::function<void(std::exception_ptr)> p_fnException, const Ice::Current& p_stCurrent)
{
	::JSONBINRPC::AMD_IJsonBinRPC_UnRegisterStockPushIOPtr cb = std::make_shared<::JSONBINRPC::AMD_IJsonBinRPC_UnRegisterStockPushIO>(p_fnResponse, p_fnException);
	UnRegisterStockPushIO_async(cb, p_strGuid, p_stCurrent);
}

// Ice 3.8 推送包入口，转接到旧异步推送队列。
void CJsonBinRPCImp::ProcessPackageAsync(std::int64_t p_lReqNo, ::JSONBINRPC::BinaryPayload p_stPayload, std::function<void()> p_fnResponse, std::function<void(std::exception_ptr)> p_fnException, const Ice::Current& p_stCurrent)
{
	::JSONBINRPC::AMD_IJsonBinRPC_ProcessPackagePtr cb = std::make_shared<::JSONBINRPC::AMD_IJsonBinRPC_ProcessPackage>(p_fnResponse, p_fnException);
	ProcessPackage_async(cb, p_lReqNo, p_stPayload, p_stCurrent);
}

// Ice 3.8 带订阅信息注册入口，保持旧版返回快速通道配置的行为。
void CJsonBinRPCImp::RegisterStockPushIO2Async(std::string p_strGuid, std::string p_strSubInfo, ::Ice::Identity p_stIdent, std::function<void(std::string_view)> p_fnResponse, std::function<void(std::exception_ptr)> p_fnException, const Ice::Current& p_stCurrent)
{
	::JSONBINRPC::AMD_IJsonBinRPC_RegisterStockPushIO2Ptr cb = std::make_shared<::JSONBINRPC::AMD_IJsonBinRPC_RegisterStockPushIO2>(p_fnResponse, p_fnException);
	RegisterStockPushIO2_async(cb, p_strGuid, p_strSubInfo, p_stIdent, p_stCurrent);
}

// Ice 3.8 带订阅信息注销入口，复用旧版退订和断开通知逻辑。
void CJsonBinRPCImp::UnRegisterStockPushIO2Async(std::string p_strGuid, std::string p_strSubInfo, ::Ice::Identity p_stIdent, std::function<void(std::string_view)> p_fnResponse, std::function<void(std::exception_ptr)> p_fnException, const Ice::Current& p_stCurrent)
{
	::JSONBINRPC::AMD_IJsonBinRPC_UnRegisterStockPushIO2Ptr cb = std::make_shared<::JSONBINRPC::AMD_IJsonBinRPC_UnRegisterStockPushIO2>(p_fnResponse, p_fnException);
	UnRegisterStockPushIO2_async(cb, p_strGuid, p_strSubInfo, p_stIdent, p_stCurrent);
}
}

