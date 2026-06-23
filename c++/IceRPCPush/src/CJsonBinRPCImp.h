#pragma once

#include <Ice/Ice.h>
#include "corba/JSONBINRPCU.h"
#include "compat/IceCompat.h"
#include "compat/SimplePool.h"
#include <optional>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <mstcpip.h>
#include "DQueue.h"
#include "IceRPCPush.h"
#include "sexport.h"
#include "userdata.h"

// 全局管理指针
extern CRITICAL_SECTION		g_csHandleLock;
extern std::map<HANDLE,HANDLE>	g_mapHandle;
// 采用线程关闭ice，如果尝试N次关闭不了，强制关闭，防止卡住上层主程序;
// 正常情况下，会顺利退出，只要不要再ice的回调里面操作

// 集中推送线程使用的包节点，跨线程传递后由消费方释放。
struct ST_PACK_QUEUE 
{
	// 当前包是否使用 snappy 压缩，消费线程据此决定是否解压。
	char	bSnappy;
	// 直接提前返回，推送批量，服务器不关心返回值
	// ::JSONBINRPC::AMD_IJsonBinRPC_ProcessPackagePtr	cb;
	// 推送请求号或通知号，上层根据它区分业务类型。
	long long lReqNo;
	// p_pBuf 的有效字节数，允许二进制内容包含 0。
	long	lBufLen;
	// 跨线程复制出来的包体缓冲区，由队列消费方 delete[]。
	char *	pBuf;
	ST_PACK_QUEUE()
	{
		bSnappy = 0;
		//cb = nullptr;
		lReqNo = 0;
		pBuf = NULL;
		lBufLen = 0;
	}
};
// 64 位组合 Key，用两个 BKDR hash 降低客户端 p_strGuid 冲突概率。
union	ST_UNI_KEY
{
	unsigned long long	lKey;			// 唯一hash值（0-0的块是无效块)
	struct  
	{
		DWORD	dwLowKey;		// 低位
		DWORD	dwHighKey;	// 高位
	}stKeyPart; 
};

// 异步PUT的信息; ST_JSON_INPUT 是上层异步PUT的数据,调用时候已经把数据发送出去，可以不用长期保存
struct ST_JSON_INPUT_EX : ST_JSON_INPUT 
{
	::JSONBINRPC::AMD_IJsonBinRPC_JsonBinRPCPtr		pRpcCallback;	// RPC 异步回包对象，RPC 请求处理完成后调用。
	::JSONBINRPC::AMD_IJsonBinRPC_JsonBinPUTPtr		pPutCallback;	// PUT 异步回包对象，PUT 请求处理完成后调用。
	::JSONBINRPC::AByte								 aJsonReq;		// 深拷贝后的请求 JSON 数据，避免引用 Ice 局部缓冲。
	::JSONBINRPC::AByte								 aLParam;		// PUT 第一组二进制参数深拷贝。
	::JSONBINRPC::AByte								 aWParam;		// PUT 第二组二进制参数深拷贝。
	ST_JSON_INPUT_EX():ST_JSON_INPUT()
	{
		pRpcCallback = nullptr;
		pPutCallback = nullptr;
		lSetCode = 0;
		hSelf = NULL;
		aJsonReq.clear();
		aLParam.clear();
		aWParam.clear();
	}
	~ST_JSON_INPUT_EX()
	{
		pRpcCallback = nullptr;
		pPutCallback = nullptr;
		lSetCode = 0;
		hSelf = NULL;
		aJsonReq.clear();
		aLParam.clear();
		aWParam.clear();
	}
};
// 远程调用后的返回信息，Ex 是内部扩展后的信息。
struct ST_JSON_MULTI_RESULT_EX : ST_JSON_M_RESULT_LEVEL
{
	HANDLE										hHandle;	// 发起调用的客户端 HANDLE，用于 EndPre 和异常上报。
	::JSONBINRPC::AByte							aReqJson;	// 请求 JSON 深拷贝，保证结果对象中 stJsonReq 指针有效。
	::Ice::AsyncResultPtr						pResult;	// Ice 兼容异步结果对象，Pre/End 模式等待它完成。
	::JSONBINRPC::AByte							aLParam;	// 第一组二进制返回数据持有者。
	::JSONBINRPC::AByte							aWParam;	// 第二组二进制返回数据持有者。
	ST_JSON_MULTI_RESULT_EX() :ST_JSON_M_RESULT_LEVEL()
	{
		hHandle = NULL;
		pResult = nullptr;
		aReqJson.clear();
		aLParam.clear();
		aWParam.clear();
	}
};
// 直回调模式的内部结果节点，保存 Ice AMD 回包对象。
struct	ST_JSON_MULTI_RESULT_DIRECT_CALLBACK : ST_JSON_M_RESULT_LEVEL
{
	char											chMode;		// 请求模式，区分 RPC 和 PUT。
	long long									lSetCode;	// 业务集合或市场编码，回调给上层时保持原值。
	HANDLE										hSelf;		// 服务端保存的客户端句柄，用于回包或反向推送。
	::JSONBINRPC::AMD_IJsonBinRPC_JsonBinRPCPtr		pRpcCallback;	// RPC 直回调模式的 Ice 回包对象。
	::JSONBINRPC::AMD_IJsonBinRPC_JsonBinPUTPtr		pPutCallback;	// PUT 直回调模式的 Ice 回包对象。
	::JSONBINRPC::AByte								 aJsonReq;		// 请求 JSON 深拷贝，保证回调期间指针有效。
	::JSONBINRPC::AByte								 aLParam;		// PUT 第一组二进制参数深拷贝。
	::JSONBINRPC::AByte								 aWParam;		// PUT 第二组二进制参数深拷贝。
	ST_JSON_MULTI_RESULT_DIRECT_CALLBACK():ST_JSON_M_RESULT_LEVEL()
	{
		chMode= 0;
		pRpcCallback = nullptr;
		pPutCallback = nullptr;
		lSetCode = 0;
		hSelf = NULL;
		aJsonReq.clear();
		aLParam.clear();
		aWParam.clear();
	}
	~ST_JSON_MULTI_RESULT_DIRECT_CALLBACK()
	{
		pRpcCallback = nullptr;
		pPutCallback = nullptr;
		lSetCode = 0;
		hSelf = NULL;
		aJsonReq.clear();
		aLParam.clear();
		aWParam.clear();
	}
};

// 客户端主动连接推送通道的状态，支持 TCP/UDP/组播三种模式。
struct ST_TCP_PUSH_CONN 
{
	std::string			strIp;			// 远程推送通道 IP。
	int					iPort;			// 远程推送通道端口。

	SOCKET				hSocket;		// TCP/UDP socket 句柄。
	int					iNetType;		// 网络类型，取 SOCK_STREAM 或 SOCK_DGRAM。
	HANDLE				hHandle;		// 拉取线程句柄，由推送管理器负责关闭。
	bool				bOpenTcpOk;	// TCP 通道是否已经打开成功。
	bool				bOpenUdpOk;	// UDP 通道是否已经打开成功。
	ST_TCP_PUSH_CONN()
	{
		strIp.clear();
		iPort = 0;
		hHandle	 = NULL;
		iNetType = SOCK_STREAM;
		bOpenUdpOk=false;
		bOpenTcpOk=false;
		hSocket	 = INVALID_SOCKET;
	}
};
// UDP 推送端口和客户端映射信息，推送线程用它维护快速通道。
struct	ST_UDP_INFO
{
	long					lRef;		// 活跃引用计数，降到 0 后允许清理该 UDP 客户端。
	int						iPort;		// 本地绑定或远程发送端口。
	SOCKET					iFd;		// UDP socket 句柄。
	struct sockaddr_in		stAddr;		// 远程地址，发送 UDP 包时使用。
	//unsigned long long		dwCount;
	HANDLE					hHandle;	// 关联的 SocketServer 客户端句柄，作为服务端推送时使用。
	SOCKET					iServerFd;	// 服务端 UDP socket 句柄。
	class CPushMng	*		pParent;	// 所属推送管理器，不拥有其生命周期。
	bool					bDeleted;	// 是否已经进入删除流程，避免重复释放。
	ST_UDP_INFO()
	{
		lRef = 0;
		iPort = 0;
		iFd = INVALID_SOCKET;
		memset(&stAddr, 0, sizeof(stAddr));
		hHandle = NULL;
		iServerFd = INVALID_SOCKET;
		pParent = NULL;
		bDeleted = false;
	}
};
// 统一管理 TCP/UDP/Ice 推送队列，不区分推送来源，集中处理限流和回调。
class CPushMng
{
public:
// 获取全局推送管理器并增加引用，服务端和客户端注册都共享它。
	static	CPushMng *	CreateNewPushObj();
// 释放全局推送管理器引用，最后一个引用退出时清理线程和 socket。
	static	void		ReleaseIt();
	static	void		PushRegisterInfo(const char * p_szRegSubInfo,int p_iIsReg=1);
// 把 Ice 推送包放入集中队列，由专用线程解压并分发。
	static	void		PushPack(ST_PACK_QUEUE* p_pPack);
	static	void		ProcessPackage(long long p_lReqNo,const char * p_pBuf,long p_lBufLen);
	static	void		PushHandle(HANDLE p_hHandle,bool p_bIsDel=false);
	// 动态更新注册的回调函数
	static	void		UpdateCallBack(func_JsonICEPushClientPack p_pfnCallback,void * p_pParam,bool p_bIsDel=false);
	// 统一管理连接	SOCK_DGRAM		SOCK_STREAM
	static	void		PushConnectInfo(int p_iNetType,std::string p_strIp,std::string p_strPort);
	// 启动UDP
// 启动 UDP 点对点快速推送，适合高频行情包降低 Ice 压力。
	bool				StartP2PPush(int p_iThread,int p_iPort);
	void				UDPPush(const char * p_pBuf,long p_lBufLen);
protected:
	CPushMng();
	~CPushMng();
	static	DWORD	WINAPI	s_PushThread(void * p_pParam);
	DWORD	PushThread();
	// 客户端socket拉取线程
	static	DWORD	WINAPI	s_PeekThread(void * p_pParam);
	DWORD	PeekThread(ST_TCP_PUSH_CONN * p_pConn);
	void	SetSubReqBuf(ST_REQ_HEADER * p_pReq);
	// UDP
	HANDLE						m_hP2PThread;
	HANDLE						m_hMulThread;
	static	DWORD	WINAPI	s_P2PRecv(void * p_pParam);
	DWORD	P2PRecv(ST_UDP_INFO * p_pUdpInfo);
protected:
// 全局单例指针，历史接口按进程共享推送线程。
	static	CPushMng *  m_pThis;
// 单例引用计数，避免多个客户端注册时提前销毁推送线程。
	volatile LONG		m_lRef;
	std::map<DWORD,int>	m_mapRegSubInfo;
	//std::map<unsigned long long,std::string>			m_mapRegSubInfo;
	//std::map<unsigned long long,int>					m_aRegSubState;
	// 作为客户端，注册new的实例，发现有父类，可以把信号灯传递给父类，不用上层再次引出信号灯
// 跨线程推送包队列，生产方只入队并唤醒信号量。
	CDataQueue<ST_PACK_QUEUE>							m_clPackQueue;
	HANDLE											m_hPushPack;
	HANDLE											m_hSemPush;
protected:
	// 注册的回调函数情况
	CRITICAL_SECTION								m_csPush;
// 已注册的推送回调集合，value 是调用方透传参数。
	std::map<func_JsonICEPushClientPack,void*>		m_mapPackCallback;
	CRITICAL_SECTION								m_csHandle;
	std::map<HANDLE,HANDLE>							m_mapHandle;
	bool											m_bStop;
// 按地址缓存客户端推送连接，防止重复建连。
	std::map<std::string,ST_TCP_PUSH_CONN*>				m_mapPushConn;
	// UDP
	int												m_iUdpThread;
	std::map<int,ST_UDP_INFO*>							m_mapBindClient;
	CRITICAL_SECTION								m_csClientPushLock;
// UDP 客户端心跳表，长时间无响应的客户端不再推送。
	std::map<unsigned long long,ST_UDP_INFO*>					m_mapPushClient;			// 需要维护心跳包，长时间没反馈，不再推送
public:
	static volatile long long							m_lPushPackCrowded;
};

// U 代表 Union合并接口的升级意思
namespace JSONBINRPC
{	
	// 对handle进行改造，支持服务端从具体客户端反向RPC
// DLL 对外 HANDLE 的真实结构，统一包装服务实例或具体客户端代理。
	struct ST_JSON_BIN_HANDLE
	{
		// HANDLE 包装对象引用计数，异步推送期间通过 Ref/ReleaseIt 保护生命周期。
		long						lRef;		// 引用计数器，减到0就被删除
		// 句柄类型，区分 RPC 实例句柄和服务端保存的具体客户端句柄。
		int							iType;		// 0==CJsonBinRPCImp   1==具体客户端
		// 关联的 RPC 实例，客户端句柄也通过它访问父对象状态。
		class CJsonBinRPCImp	*	pRpc;		// 作为客户端，也有
		// 订阅功能号状态，服务端推送时按 p_lReqNo 过滤目标客户端。
		std::map<DWORD,int>			mapSubId;		// 订阅信息 1=订阅  0=不订阅
		// 主动向下游推送，管理的client信息
// Ice 3.8 代理可能为空，用 optional 表达连接/注册未完成状态。
		std::optional<JSONBINRPC::IJsonBinRPCPrx> refProxy;
		// 客户端注册标识，用于续约、注销和生成服务端保存 key。
		std::string					strGuid;
		std::string					strRet;		// 用于返回的临时变量
		int						iLastErrorCode;	// 最近一次接口错误码，供上层查询。
		std::string					strLastError;	// 最近一次接口详细英文错误描述。
		time_t						tmLive;		// 存活包
		DWORD						dwCount;
// 返回可调用的远端代理；服务端具体客户端句柄会走自身 refProxy。
		JSONBINRPC::IJsonBinRPCPrx	ClientIO();
		ST_JSON_BIN_HANDLE*				Ref();
		bool						ReleaseIt();
		ST_JSON_BIN_HANDLE();
		// BKDRHash : 固定标准hash算法
		static unsigned int GenHash(const char* p_szText, unsigned int p_uLen,unsigned int p_uSeed)
		{
			//unsigned int uSeed = 131; /* 31 131 1313 13131 131313 etc.. */
			unsigned int hash = 0;
			unsigned int i	 = 0;

			for(i = 0; i < p_uLen; p_szText++, i++)
			{
				hash = (hash * p_uSeed) + (*p_szText);
			}

			return hash;
		}

		// 统一硬编码  p_ulKey = GenKey(Code)
		static unsigned long long GenKey(const char* p_szText)
		{
			ST_UNI_KEY	uk;
			unsigned int uLen  = SafeSizeToLength<unsigned int>(strlen(p_szText));
			uk.stKeyPart.dwLowKey = GenHash(p_szText,uLen,31);
			uk.stKeyPart.dwHighKey= GenHash(p_szText,uLen,131);
			return uk.lKey;
		}
		static unsigned long long GenKey(const char* p_szText, unsigned int p_uLen)
		{
			ST_UNI_KEY	uk;
			uk.stKeyPart.dwLowKey = GenHash(p_szText,p_uLen,31);
			uk.stKeyPart.dwHighKey= GenHash(p_szText,p_uLen,131);
			return uk.lKey;
		
		}

	};

// 重连 locator 时保存新增 m_refCommunicator/m_refAdapter，避免旧连接通信中被关闭。
	struct ST_ADDED_CONN_INFO 
	{
		Ice::CommunicatorPtr												refCommunicator;	// 新建连接使用的 Communicator，老连接释放前保持存活。
		Ice::ObjectAdapterPtr											refAdapter;		// 新建连接使用的 Adapter，避免回调通道提前关闭。

		ST_ADDED_CONN_INFO()
		{
			refCommunicator = nullptr;
			refAdapter = nullptr;
		}
	};

// ICE Slice 接口的实现，负责把导出 API、Ice RPC 和推送通道串起来。
class CJsonBinRPCImp : 	public IJsonBinRPC
{
public:
	CJsonBinRPCImp(void);
	virtual	~CJsonBinRPCImp(void);

	static	void	DeleteIt(CJsonBinRPCImp * p_pThis);
	//static	DWORD	WINAPI	s_PushThread(void * p_pParam);
	//DWORD	PushThread();
public:	// 网络接口
	// 注册网络回调推送接口 Ice::Identity idguid, 直接用当前环境的
// Ice 3.5 AMD 注册入口，迁移期保留给旧代码路径参考。
	void	RegisterStockPushIO_async(const ::JSONBINRPC::AMD_IJsonBinRPC_RegisterStockPushIOPtr& p_pCallback, const ::std::string& p_strGuid, const ::Ice::Identity& p_stIdent, const ::Ice::Current& = ::Ice::Current());
	// 接口调用参数是json格式，返回时二进制，怎么解析，自行约定
	// 服务端端全部采用异步，调用end_JsonBinRPC返回数据
	void	JsonBinRPC_async(const ::JSONBINRPC::AMD_IJsonBinRPC_JsonBinRPCPtr& p_pCallback, long long p_lSynId, long long p_lFuncId,long long p_lSetCode,const ::JSONBINRPC::AByte& p_stJsonReq, const ::Ice::Current& = ::Ice::Current());
	// 远程写入的接口，写入描述信息是json，写的内容是字符串
	// 异步输入参数，所有输出调用end_xxxx部分
	void	JsonBinPUT_async(const ::JSONBINRPC::AMD_IJsonBinRPC_JsonBinPUTPtr& p_pCallback, long long p_lSynId, long long p_lFuncId,long long p_lSetCode,const ::JSONBINRPC::AByte& p_stJsonReq, 
									long long p_lParam, const ::JSONBINRPC::AByte& p_stLParam, long long p_lWParam, const ::JSONBINRPC::AByte& p_stWParam,const ::Ice::Current& = ::Ice::Current());
	// 注销
	void	UnRegisterStockPushIO_async(const ::JSONBINRPC::AMD_IJsonBinRPC_UnRegisterStockPushIOPtr& p_pCallback, const ::std::string&, const ::Ice::Current& = ::Ice::Current());
	// 推送接口
	void	ProcessPackage_async(const ::JSONBINRPC::AMD_IJsonBinRPC_ProcessPackagePtr& p_pCallback, long long p_lReqNo, const ::JSONBINRPC::AByte& p_stAByte, const ::Ice::Current& = ::Ice::Current());
	
	
	void	RegisterStockPushIO2_async(const ::JSONBINRPC::AMD_IJsonBinRPC_RegisterStockPushIO2Ptr& p_pCallback, const ::std::string& p_strGuid, const ::std::string& p_strSubInfo, const ::Ice::Identity& p_stIdent, const ::Ice::Current& p_stCurrent = ::Ice::Current());
	void	UnRegisterStockPushIO2_async(const ::JSONBINRPC::AMD_IJsonBinRPC_UnRegisterStockPushIO2Ptr& p_pCallback, const ::std::string& p_strGuid, const ::std::string& p_strSubInfo, const ::Ice::Identity& p_stIdent, const ::Ice::Current& p_stCurrent = ::Ice::Current());
	// 动态更新注册的回调函数
	//void	UpdateCallBack(func_JsonICEPushClientPack p_pfnCallback,void * p_pParam,bool p_bIsDel=false);
	// Ice 3.8 生成接口要求的异步注册入口，内部转接旧 AMD 处理流程。
	void	RegisterStockPushIOAsync(std::string p_strGuid, ::Ice::Identity p_stIdent, std::function<void(std::int64_t)> p_fnResponse, std::function<void(std::exception_ptr)> p_fnException, const Ice::Current& p_stCurrent) override;
	void	JsonBinRPCAsync(std::int64_t p_lSynId, std::int64_t p_lFuncId, std::int64_t p_lSetCode, ::JSONBINRPC::AByte p_stReqJson, std::function<void(std::int64_t, std::int64_t, const ::JSONBINRPC::AByte&, std::int64_t, const ::JSONBINRPC::AByte&, std::string_view)> p_fnResponse, std::function<void(std::exception_ptr)> p_fnException, const Ice::Current& p_stCurrent) override;
	void	JsonBinPUTAsync(std::int64_t p_lSynId, std::int64_t p_lFuncId, std::int64_t p_lSetCode, ::JSONBINRPC::AByte p_stPutJson, std::int64_t p_lParam, ::JSONBINRPC::AByte p_stLParam, std::int64_t p_lWParam, ::JSONBINRPC::AByte p_stWParam, std::function<void(std::int64_t, std::int64_t, const ::JSONBINRPC::AByte&, std::int64_t, const ::JSONBINRPC::AByte&, std::string_view)> p_fnResponse, std::function<void(std::exception_ptr)> p_fnException, const Ice::Current& p_stCurrent) override;
	void	UnRegisterStockPushIOAsync(std::string p_strGuid, std::function<void(std::int64_t)> p_fnResponse, std::function<void(std::exception_ptr)> p_fnException, const Ice::Current& p_stCurrent) override;
	void	ProcessPackageAsync(std::int64_t p_lReqNo, ::JSONBINRPC::AByte p_pBuf, std::function<void()> p_fnResponse, std::function<void(std::exception_ptr)> p_fnException, const Ice::Current& p_stCurrent) override;
	void	RegisterStockPushIO2Async(std::string p_strGuid, std::string p_strSubInfo, ::Ice::Identity p_stIdent, std::function<void(std::string_view)> p_fnResponse, std::function<void(std::exception_ptr)> p_fnException, const Ice::Current& p_stCurrent) override;
	void	UnRegisterStockPushIO2Async(std::string p_strGuid, std::string p_strSubInfo, ::Ice::Identity p_stIdent, std::function<void(std::string_view)> p_fnResponse, std::function<void(std::exception_ptr)> p_fnException, const Ice::Current& p_stCurrent) override;
void	TTLive();
public:	// 互动接口
// 服务端主动推送入口，根据订阅和网络模式选择 Ice/TCP/UDP 通道。
	long long	ProcessPackage(long long p_lReqNo,const char * p_pBuf,long p_lBufLen,bool p_bIsAsync=true);
// 以服务端模式启动 Ice m_refCommunicator 和 m_refAdapter，并初始化推送服务。
	bool	StartByServer(const char * p_szCfgFile,const char * p_szEndpointName,HANDLE & p_hSem,bool p_bSnappyCompress);
	// 轻量级
// 从配置文件启动客户端，并支持 locator/m_refProxy 属性解析。
	bool	StartByClientWithLocator(const char * p_szCfgFile,const char * p_szProxyProperty,HANDLE & p_hSem);
// 从调用方传入的 key/value 属性启动客户端，避免依赖配置文件。
	bool    StartByClientWithProperty(int p_iNum,const char * p_aPropertyKey[],const char * p_aProperty[],const char * p_szProxyProperty,HANDLE & p_hSem);
	// 返回的客户端，可以正常操作任何函数
// 注册客户端推送回调，必要时创建双向 Ice callback m_refAdapter。
	JSONBINRPC::CJsonBinRPCImp *	RegisterClient(std::string& p_strRet,const char * p_strGuid,const char *p_strSubInfo,func_JsonICEPushClientPack p_pfnCallback,int p_iIsReg=1,void * p_pParam=NULL);
// 停止 Ice、SocketServer 和后台线程，释放 DLL 内部资源。
	void	UnInit();
	void	DeleteAddedConn();
	const char * GetEndPoint();
	JSONBINRPC::IJsonBinRPCPrx	ClientIO()
	{
		return *m_refStockIo;
	}
	//void	DeleteIt();
	CDataQueue<ST_JSON_INPUT_EX> & Req()
	{
		return m_aReqMsg;
	}
	HANDLE		Sem()
	{
		return	m_semMiddle;
	}
	bool		IsClientMode()
	{
		return (m_refStockIo.has_value() || m_refCommunicatorClient );
	}
	void	SelfDelClient(unsigned long long p_ulKey);
	void	PushFinish(unsigned long long p_ulKey);
	void	RegisterCallBack(long long p_lRetVal);
	bool	AddConnectLoctor();
	bool	IsAsyncWaitCompleted() const
	{
		return m_bAsyncWaitCompleted;
	}
protected:
// 当前连接是否按 snappy 压缩推送包，由注册返回值或配置决定。
	bool											m_bSnappy;
	bool											m_bRegisterOk;
	bool											m_bNeedReconnect;
	std::string										m_strCfgFile;
	// 作为服务端记录注册的客户端proxy，用于主动推送数据给客户端
// 服务端保存已注册客户端，key 由 p_strGuid hash 生成。
	std::map<unsigned long long,ST_JSON_BIN_HANDLE*>		m_mapClients;
	CRITICAL_SECTION								m_csLock;
	// 作为服务端的接收请求队列和信号灯
	CDataQueue<ST_JSON_INPUT_EX>						m_aReqMsg;
	HANDLE											m_semMiddle;
	// 服务端连接信息
// 服务端 Ice m_refCommunicator，生命周期随服务 HANDLE。
	Ice::CommunicatorPtr							m_refCommunicator;
	Ice::ObjectAdapterPtr							m_refAdapter,m_refLastAdapter;
	Ice::Identity									m_stObjectSrvId;
	std::string										m_strEndpointSrvName;
	std::string										m_strEndpointSrv;
// Ice 3.8 locator 代理可能不存在，使用 optional 避免旧空指针语义。
	std::optional<Ice::LocatorPrx> m_refLocator;
	std::map<Ice::CommunicatorPtr,ST_ADDED_CONN_INFO>	m_mapReconLocator;	// 新的链接增加，老的链接不断开，防止原有链接通信中

	// 如果作为客户端启动的proxy情况
// 客户端 Ice m_refCommunicator，负责发起 RPC/PUT 和双向回调连接。
	Ice::CommunicatorPtr							m_refCommunicatorClient;
	Ice::ObjectAdapterPtr							m_refAdapterClient;
// 客户端远端服务代理，未连接或初始化失败时为空。
	std::optional<JSONBINRPC::IJsonBinRPCPrx> m_refStockIo;
	std::string										m_strProxyProperty,m_strClientGuid;

	CJsonBinRPCImp*									m_pStockPushIo;
	std::string										m_strError,m_strProgramName;
	// 作为客户端，注册new的实例，发现有父类，可以把信号灯传递给父类，不用上层再次引出信号灯
	CJsonBinRPCImp*									m_pParentImp;
	//CDataQueue<ST_PACK_QUEUE>							m_clPackQueue;
	//HANDLE										m_hPushPack;
	//HANDLE										m_hSemPush;
	// 消息订阅服务
	std::string										m_strPushBindPort,m_strPushBindIp,m_strPushPort,m_strPushIp,m_strPushPass;
	bool											m_bOpenTcp,m_bOpenTcpOk;
	int												m_iUdpThread;
	std::string										m_strPushUdpBindPort,m_strPushUdpBindIp,m_strPushUdpPort,m_strPushUdpIp,m_strPushUdpMulPort,m_strPushUdpMulIp;
	bool											m_bOpenUdp,m_bOpenUdpOk,m_bOpenMul,m_bOpenMulOk;
// 是否等待异步 RPC 完成，默认保留旧版阻塞行为，XML 可配置为 0 提升吞吐。
	bool											m_bAsyncWaitCompleted;
// TCP 推送服务句柄，只有开启旧 TCP 推送时有效。
	HS												m_hSocketServer;
	SOCKET											m_hMulSocket;		// 分组推送，30~50微妙
	struct sockaddr_in								m_stAddrSend;
protected:
	// 注册的回调函数情况
	//CRITICAL_SECTION								m_csPush;
	//std::map<func_JsonICEPushClientPack,void*>	m_mapPackCallback;
	//volatile long long								m_lPushPackCrowded;
	bool											m_bStop;
	CRITICAL_SECTION								m_csMem;
// 推送包临时内存池，降低高频推送时的堆分配成本。
	CSimplePool									m_clMemMng; 
public:
	func_JsonICEServerCallBsack						m_pfnServerCallback;
	void								*			m_pServerParam;
};



};

// 统一压缩辅助函数，隐藏 snappy 与 Ice AByte 字节类型差异。
int	CompressAByte(::JSONBINRPC::AByte & p_refOutByte,const char * p_pSrc,long p_lSrcLen);
// 统一解压辅助函数，返回解压后的有效字节数。
int	UnCompressAByte(::JSONBINRPC::AByte & p_refOutByte,const char * p_pSrc,long p_lSrcLen);

