#ifndef _chan_userdata_h_
#define _chan_userdata_h_

 

#include "sexport.h"


#define HANDLE_SYN		(-2015)
#define HANDLE_OVER		2018




#pragma  pack(push,1)

//DZH_FDID_SYS_IPADDR		1204	//	IP地址

//const int MAX_CACHE_BUFLEN=10*1024;//1024*1024;//用户最大缓冲数据长度,超过这个长度则断开用户

//const int MAX_MIDDLE_CACHE=8*1024*1024;	// 连接数目不能太多,中间件不能连接太长

const int MAX_CACHE_BUFLEN= 1024*1024*32;//16;//32;	// 1024*1024*64;
const int MAX_REQ_BUFLEN  = 1024*32;	//1024*1024;
const short PACKET_PUSH_HQ_SUB	= 134;
#define SEPARATOR_NUM	7654321

const	unsigned char	FIRST_SUB	= 1;	// 首次订阅
const	unsigned char	SUB_SUCCESS	= 2;	// 订阅成功，并且首次推送过
const	unsigned char	UN_SUB		= 3;	// 取消订阅

enum	EN_COMPRESS_TALK	// 压缩协商	unsigned char
{
	EN_COMPRESS_DEFAULT = 0,	// 缺省模式： 服务器缺省会使用zlib压缩
	EN_COMPRESS_ZLIB	 = 1,	
	EN_COMPRESS_SNAPPY	 = 2,
};


// 每个股票的订阅信息
// 单个品种的订阅计数，保留旧行情协议的二进制布局。
struct	ST_SUB_INFO_BY_STOCK
{
	unsigned	short		unType;		// 订阅类型，具体值由行情协议定义。
	unsigned	short		unSubCount;		// 当前品种的订阅次数，用于重复订阅计数。
	ST_SUB_INFO_BY_STOCK()
	{
		unType = 0;
		unSubCount = 0;
	}
};

// 推送包在队列中的轻量描述，缓冲区由生产方/消费方按约定释放。
struct ST_PUSH_DATA_INFO 
{
	long long		lReqNo;		// 推送请求号或功能号，接收端按它分发业务。
	char		pBuf[0];	// 变长推送数据起始地址，实际空间跟随结构体分配。
};

// 不允许保存对象，map，vector等，因为有 memcpy ，memset等基础操作
// TCP 推送连接的运行态数据，位于 CUserManage 的预分配数组中。
struct ST_USER_DATA
{
// SocketServer 服务句柄，用于回调中定位监听实例。
	HS		hServer;		// 记录了提供端口服务的总句柄
// 具体客户端连接句柄，由 SocketServer 兼容层分配。
    HCLIENT hHandle;		// 记录了这个用户的链接句柄
	// 改成动态分配
	// char*	lpbuf;
	// 不要用缓冲区，临时变量的缓冲区被WSABUF指向，消失后，导致缓冲区飞掉，因为指向的是老的
	// 性能提高，跟大面积缓冲区直接对拷有关，负担很重
// 接收缓存来自统一内存池，避免每次网络回调频繁分配。
	char*	pBuf;	//[MAX_CACHE_BUFLEN];
// 当前缓存中尚未解析的字节数，处理半包/粘包时持续累积。
	int		iRecvLen;	// 总是记录现在缓冲区的数据内容的长度
// 最近活动时间，用于未来清理空闲连接。
	time_t	tmLastActive;	// 最后活动时间
	char	strIp[256];	// 客户端 IP 文本，固定缓冲区并以 0 结尾。
	int		iPort;		// 客户端端口号。
	////////////////////
// 内部引用计数，为 0 表示槽位空闲，Query 后必须 ReleaseIt。
	int		lRef;//内部引用计数,为0表示未使用
	int		iHandleRef;		// 处理引用计数,防止在处理的过程中,同一个连接,通过另外一个线程再次来到数据,破坏缓冲区的处理情况,通常在完成端口的处理接收的时候是等上层处理完才处理下一个
	DWORD	dwReqGnid;		// 当前处理的功能ID,方便查错的
	DWORD	dwAnsGnid;		// 当前应答功能 ID，便于日志排查。
	char	szError[128];	// 单连接错误信息缓存，固定长度并以 0 结尾。
	// 支持TCP订阅 : const short COMBHQ2_10MMP_NREQ 			= 1728; // 请求指定若干品种行情（支持10档买卖盘）
	// std::map<unsigned long long,ST_SUB_INFO_BY_STOCK>	*	pMapKey;
	std::map<DWORD,int>	*		pMapKey;		// 订阅信息 1=订阅  0=不订阅
	//////////////////////////////////////////////////////////////////////////
/*
当用户校验返回时//字典功能入口11101
此时,把里面的营业部编码(1005),客户号(1016),保存到每个用户相应的变量
当用户股东查询返回时//字典功能入口11155
此时,把里面股东代码(1019)保存到每个用户相应的变量

  当用户使用12010(取系统支持登陆方式)和11100(客户校验)功能时,不作任保校验
  其它所有功能做如下校验
*/	
	ST_USER_DATA()
	{
		iHandleRef	= 0;
		dwReqGnid	= 0;
		dwAnsGnid	= 0;
		hHandle=(HCLIENT)-1;
		hServer=0;
		iRecvLen=0;
		tmLastActive=0;
		lRef=0;
		pBuf = NULL;
		iPort = 0;

		pMapKey = NULL;
		memset(szError,0,sizeof(szError));
		memset(strIp,0,sizeof(strIp));
	}
	// 必要的时候初始化，不要放到构造中，因为模板库会导致无谓的调用构造 : 例如resize操作
// 初始化连接槽位的动态资源，必须在加入 CUserManage 前调用。
	void Init();
	// 务必最后释放的时候release，不要依靠析构，因为中间临时变量太多，容易出问题；
	// 其他时候，临时变量对拷就不用重新申请空间;
// 释放接收缓存和订阅映射，并把槽位恢复为空闲状态。
	void ResetIt();
	
};

// 旧 TCP 推送请求协议头，使用 pack(1) 保持网络二进制兼容。
typedef struct ST_REQ_HEADER
{
	unsigned long dwCrc;
	struct
	{
		unsigned char chVersion;//:4;
		unsigned char chCompressed;//:1;		// 1 : zlib ; 2 : snappy
		unsigned char chEncrypted;//:1;
		unsigned char chTalkCompress;//:2;		EN_COMPRESS_TALK 指定压缩算法
	}     stInfo;
	DWORD	dwCookie;				// 用户上层自己携带的信息
	long	lMainId;
	long	lAssistId;
	unsigned long  dwPacketLen;
	unsigned long  dwRawLen;
	char  chPriority;
} ST_REQ_HEADER;

// 旧 TCP 应答协议头，字段顺序不能调整，否则客户端解析会错位。
typedef struct ST_ANS_HEADER
{
	unsigned long  dwCrc;
	long		   lSeparator;
	unsigned long  dwPacketLen;
	unsigned long  dwRawLen;
	struct
	{
		unsigned char chVersion;//:4;
		unsigned char chCompressed;//:1;		// 1 : zlib ; 2 : snappy
		unsigned char chEncrypted;//:1;
		unsigned char chTalkCompress;//:2;		EN_COMPRESS_TALK 指定压缩算法
	}     stInfo;
	DWORD	dwCookie;				// 用户上层自己携带的信息
	long	lMainId;
	long	lAssistId;
	char	chPriority;
	long	req;
} ST_ANS_HEADER;
// UDP 不能发大包
// UDP 快速推送头，只保留压缩方式和请求号以减小包体开销。
typedef struct ST_UDP_HEADER
{
//	unsigned short	dwPacketLen;
//	unsigned short	dwRawLen;
	unsigned char	chCompressed;//:1;		// 压缩标记，1 表示 zlib，2 表示 snappy。
	unsigned short	req;				// 推送请求号，UDP 头中使用 16 位保存。
	ST_UDP_HEADER()
	{
		memset(this, 0, sizeof(ST_UDP_HEADER));
	}
} ST_UDP_HEADER;

// 固定数组形式的行情订阅请求，旧协议仍可能发送。
struct ST_SUB_UNSUB_HQ_REQ	// 订阅行情请求
{
	short				req;
	short				iNum;		// 订阅几个品种; -1 表示取消全部订阅
	unsigned long long	aKey[0];	// 唯一key
};
// 字符串形式订阅请求，多个功能号用分隔符传输。
struct ST_SUB_UNSUB_STRING 
{
	short				req;		// 订阅或取消订阅请求号。
	long				lLen;		// pBuf 的有效字节数，单位为字节。
	char				pBuf[0];	// 变长订阅字符串起始地址，实际空间跟随结构体分配。
};
//pop时，无需带参数
#pragma  pack(pop)

#endif

