#if !defined(KDSC_USER_DATA_H)
#define KDSC_USER_DATA_H

#include <time.h>
#include <map>
#include "Struct.h"
#include "SocketServer.h"
#include "nsdk_atomic.h"

//协议类型;
const char	TCP_PACK_TYPE = 0;
const char	PROTOBUF_PACK_TYPE = 1;

//连接类型;
const char	TCP_SOURCE_TYPE		= 0;
const char  HTTP_SOURCE_TYPE	= 1;
const char	WEBSOCK_SOURCE_TYPE = 2;

//用户数据;
const unsigned char	FIRST_SUB	= 1;	// 首次订阅;
const unsigned char	SUB_SUCCESS	= 2;	// 订阅成功，并且首次推送过;
const unsigned char	UN_SUB		= 3;	// 取消订阅;

// SUB_INFO_BY_STOCK 旧版已弃用, 使用138新订阅协议;
// 每个股票的订阅信息。20230313  适用于每个协议的订阅信息, 用于该界面动态行情，经过插件计算完成推送;
struct SUB_INFO_BY_STOCK
{
	unsigned short cType;		// 订阅的类型;
	unsigned short nSub;		// 订阅次数;
};

//报文类型;
enum MsgType
{
	MSG_DEFAULT = 0,	//0:默认TCP协议,字节流;
	MSG_PROTO_JSON,		//proto json报文;
	MSG_PROTO_BINARY	//proto 二进制报文;
};

//内存类型;
typedef enum  
{
	AnsMemSmall = 0,
	AnsMemMid,
	AnsMemBig,
	AnsMemHuge,
	AnsMemMaxHuge,
	AnsMemSSmall,
	AnsMemDyn			//这个是按实际情况申请内存
}AnsMemType;

enum coroutine_status
{
	NoStart,		//协程尚未运行
	Running,		//协议正在处理
	Yielding,		//协程挂起
	Complete,		//协议处理完毕
	CompleteWaitCombo,	//完成，并且等待组合结束 
	Timeout,		//协议处理超时
	AbortCoro,		//放弃了协程
};

//TODO 携程处理后续优化
struct	NetRequsetDat
{
	coroutine_status		status;				//协程状态
	int						nNetSourceType;	// 数据网络来源，tcp或者http，websock
	unsigned char			nReqPackType;//  0=tcp二进制  1=protobuff包格式
	// 组合包;
	char					hasParent;//20241113 组合包的请求全部删除,未发现使用 wyl;
	long					lParentMapMemIncrement;	// 分组的，组合包的父类
	long volatile			lComboReqNum;	// 组合有几个请求，减到0可以发送回客户端了
	// 处理好，回去的地方
	HS						pServerHandle;					// 客户端的
	HCLIENT					pClinetHandle;				// 客户端的
	unsigned long			cookie;				// 用户上层自己携带的信息
	long					MainID;				// 客户端请求编号
	long					AssisID;
	//short					iThreadNo;			// 第几个线程中处理
	__int64					lMiddle_synID;		// 异步ID,用来区别中间件应答的包的,网络异步请求返回的异步ID合一，采用全局ID
	class   CServerProc    *pHostProcess;	// 具体的处理函数+排序等
	AnsMemType				memType;
	char	*				pTransfer;		// 发送缓冲区
	long					len;
	long					maxAlloclen;	// 最大缓冲区大小
	char	*				pViewBuf;		// 反过来，给列表显示的信息
	int						nRet;
	// 同样优先级，剩余时间越少，紧迫性越大
	time_t					ltime;		//发起时间
	time_t					lbeginhandletime;	// 开始处理业务时间
	time_t					ltimeend;	//超时的终止时间，到一定的时间全部删除
	short					nGNID;		//功能号
	long					lSize;		//请求缓冲区长度
	int						iThreadNo;
	char *					lpReqPackData;
	bool					bHandleOverProtobuf;	// 有些需要差分编码的，在case内部处理完毕
	char					szIp[20];	// 记录来源的ip地址
	unsigned short			usPort;
	unsigned short			usTimeoutTimes;	// 超时次数
	NetRequsetDat**			pstNetReqDats;
	unsigned long long		ullReqId;//请求编号
	bool					bJson;//pb协议json包
	NetRequsetDat()
	{
		memset(this,0,sizeof(NetRequsetDat));
		status = NoStart;
		bHandleOverProtobuf = false;
		bJson = false;
	}
};

// 不允许保存对象，map，vector等，因为有 memcpy ，memset等基础操作;
struct USERDATA
{
	// 句柄信息;
	HS		pServerHandle;				// 记录了提供端口服务的总句柄;
	HCLIENT	pClinetHandle;				// 记录了这个用户的链接句柄;
	int		iClientSockId;	// 客户端连接sock编号;
	// 缓存信息;
	char	*pBuf;			// 改成动态分配;
	// 不要用缓冲区，临时变量的缓冲区被WSABUF指向，消失后，导致缓冲区飞掉，因为指向的是老的;
	// 性能提高，跟大面积缓冲区直接对拷有关，负担很重;
	int		iRecvLen;		// 总是记录现在缓冲区的数据内容的长度;
	// 连接信息;
	bool	bAuth;			// 是否通过了认证;
	time_t	llLastAcitve;	// 最后活动时间;
	int		iRef;			// 内部引用计数,为0表示未使用;
	short	nUserType;		// 0:未知用户， 1:监控用户 , 2:TCP普通用户 , 3:HTTP普通用户 4:web用户;
	short	nNetSourceType;	// 数据网络来源，tcp或者http，websock;
	bool	bAskDel;		// 是否请求发送过删除;确保只能del一次有效;
	MsgType	enMsgType;		// 消息类型;
	// 请求信息;
	unsigned long	ulThreadId;	// 线程号;
	unsigned long	ulReqGNID;	// 当前处理的功能ID,方便查错的;
	nsdk_atomic_t64	llNRequest;	// 客户端可以累积在服务器堆积的请求数目,MAX_REQUEST;
	// 业务处理类;
	//class CServerProc *pHostProcess;	// 具体的处理函数+排序等;
	// 订阅信息;
	std::map<unsigned long long, StockDataSubType> *lpKeyMap;	// 品种数据;
	std::map<unsigned long long, TagCode> *lpTagCodeMap;		// 市场+代码映射;

	char	szError[128];	// 错误信息;

	USERDATA()
	{
		// 句柄信息;
		pServerHandle = 0;
		pClinetHandle = 0;
		iClientSockId = 0;
		// 缓存信息;
		pBuf = nullptr;
		iRecvLen = 0;
		// 连接信息;
		bAuth = false;
		llLastAcitve = 0;
		iRef = 0;
		nUserType = 0;
		nNetSourceType = 0;
		bAskDel = false;
		enMsgType = MSG_DEFAULT;
		// 请求信息;
		ulThreadId = 0;
		ulReqGNID = 0;
		llNRequest = 0;
		lpKeyMap = nullptr;
		lpTagCodeMap = nullptr;

		memset(szError,0,sizeof(szError));
	}
public:
	// 必要的时候初始化，不要放到构造中，因为模板库会导致无谓的调用构造 : 例如resize操作;
	void Init();
	// 务必最后释放的时候release，不要依靠析构，因为中间临时变量太多，容易出问题;
	// 其他时候，临时变量对拷就不用重新申请空间;
	void ResetIt();
	
};

#endif