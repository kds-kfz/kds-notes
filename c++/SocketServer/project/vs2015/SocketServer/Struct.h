#ifndef _STRUCT_SERVER_H_
#define _STRUCT_SERVER_H_

// 请求缓存
struct ReqCacheData
{
	unsigned long long ullConnID;
	unsigned long ulLength;
	unsigned long ulPos;
	unsigned int uiTcpPendingTaskCount;	// wyl 2026-03-30：当前连接等待上层处理的 TCP 数据通知任务数，用于小包洪泛时做过载保护。
	unsigned long long ullTcpPendingBytes;// wyl 2026-03-30：当前连接等待上层处理的 TCP 数据通知累计字节数，用于限制排队内存占用。
	unsigned int uiWebPendingTaskCount;	// wyl 2026-03-30：当前连接等待上层处理的 Web 数据通知任务数，用于 WebSocket 慢消费场景的过载保护。
	unsigned long long ullWebPendingBytes;// wyl 2026-03-30：当前连接等待上层处理的 Web 数据通知累计字节数，用于限制排队内存占用。
	unsigned long ulCapacity;			// wyl 2026-03-30：当前缓存区已分配容量，主要用于 WebSocket 多段消息累计时按需扩容。
	unsigned long ulWsFrameLength;		// wyl 2026-03-30：当前 WebSocket 帧的消息体总长度，由 OnWSMessageHeader() 给出。
	unsigned long ulWsFramePos;			// wyl 2026-03-30：当前 WebSocket 帧已累计的字节数，用于校验 Body 是否收完整。
	unsigned long ulWsControlLength;	// wyl 2026-03-30：当前控制帧（主要是 ping/pong）消息体总长度，用于回发 pong 时原样带回载荷。
	unsigned long ulWsControlPos;		// wyl 2026-03-30：当前控制帧已累计的字节数。
	bool bWsFinalFrame;					// wyl 2026-03-30：当前 WebSocket 帧是否为一条完整消息的最后一帧。
	bool bWsIgnoreFrame;				// wyl 2026-03-30：当前 WebSocket 帧是否忽略业务通知，控制帧如 ping/pong 会置为 true。
	bool bWsMessageActive;				// wyl 2026-03-30：当前连接是否正在累计一条尚未完成的 WebSocket 业务消息。
	unsigned char ucWsFrameOperationCode;// wyl 2026-03-30：当前 WebSocket 帧操作码，区分数据帧、续帧以及 ping/pong 等控制帧。
	unsigned char ucWsOperationCode;	// wyl 2026-03-30：当前 WebSocket 消息的操作码，区分文本帧、二进制帧、续帧等类型。
	char szWsControlBuf[125];			// wyl 2026-03-30：控制帧最大载荷固定 125 字节，缓存 ping 内容以便在 complete 时回 pong。
	char *pBuf;
public:
	ReqCacheData()
		: ullConnID(0), ulLength(0), ulPos(0), uiTcpPendingTaskCount(0), ullTcpPendingBytes(0),
		uiWebPendingTaskCount(0), ullWebPendingBytes(0), ulCapacity(0),
		ulWsFrameLength(0), ulWsFramePos(0), ulWsControlLength(0), ulWsControlPos(0), bWsFinalFrame(true),
		bWsIgnoreFrame(false), bWsMessageActive(false), ucWsFrameOperationCode(0), ucWsOperationCode(0), pBuf(nullptr)
	{
		memset(szWsControlBuf, 0, sizeof(szWsControlBuf));
	}
	ReqCacheData(const ReqCacheData& p_stData)
		: ullConnID(0), ulLength(0), ulPos(0), uiTcpPendingTaskCount(0), ullTcpPendingBytes(0),
		uiWebPendingTaskCount(0), ullWebPendingBytes(0), ulCapacity(0),
		ulWsFrameLength(0), ulWsFramePos(0), ulWsControlLength(0), ulWsControlPos(0), bWsFinalFrame(true),
		bWsIgnoreFrame(false), bWsMessageActive(false), ucWsFrameOperationCode(0), ucWsOperationCode(0), pBuf(nullptr)
	{
		memset(szWsControlBuf, 0, sizeof(szWsControlBuf));
		*this = p_stData;
	}
	~ReqCacheData() {
		if (nullptr != pBuf)
		{
			delete []pBuf;
			pBuf = nullptr;
		}
	}
	ReqCacheData& operator=(const ReqCacheData &p_stData) {
		if (this != &p_stData) {
			ullConnID = p_stData.ullConnID;
			ulLength = p_stData.ulLength;
			ulPos = p_stData.ulPos;
			uiTcpPendingTaskCount = p_stData.uiTcpPendingTaskCount;
			ullTcpPendingBytes = p_stData.ullTcpPendingBytes;
			uiWebPendingTaskCount = p_stData.uiWebPendingTaskCount;
			ullWebPendingBytes = p_stData.ullWebPendingBytes;
			ulCapacity = p_stData.ulCapacity;
			ulWsFrameLength = p_stData.ulWsFrameLength;
			ulWsFramePos = p_stData.ulWsFramePos;
			ulWsControlLength = p_stData.ulWsControlLength;
			ulWsControlPos = p_stData.ulWsControlPos;
			bWsFinalFrame = p_stData.bWsFinalFrame;
			bWsIgnoreFrame = p_stData.bWsIgnoreFrame;
			bWsMessageActive = p_stData.bWsMessageActive;
			ucWsFrameOperationCode = p_stData.ucWsFrameOperationCode;
			ucWsOperationCode = p_stData.ucWsOperationCode;
			memcpy(szWsControlBuf, p_stData.szWsControlBuf, sizeof(szWsControlBuf));

			if (nullptr != pBuf)
			{
				delete[]pBuf;
				pBuf = nullptr;
			}

			if (nullptr != p_stData.pBuf && p_stData.ulCapacity > 0)
			{
				pBuf = new char[p_stData.ulCapacity];
				memcpy(pBuf, p_stData.pBuf, p_stData.ulPos);
			}
		}
		return *this;
	}
};

//用户信息
struct ClientData
{
	unsigned long long ullConnID;
	char szIp[32];
	unsigned short unPort;
	bool bConnected;					// wyl 2026-03-30：当前连接是否已经完成业务层可见的建链；Web 侧在握手成功后才置 true。
public:
	ClientData(){
		memset(this, 0, sizeof(ClientData));
	}
	ClientData& operator=(const ClientData &p_stData) {
		if (this != &p_stData) {
			ullConnID = p_stData.ullConnID;
			unPort = p_stData.unPort;
			bConnected = p_stData.bConnected;
			memcpy(szIp, p_stData.szIp, 32);
		}
		return *this;
	}
};

//通知任务
struct NotifyTask
{
	char *pBuf;
	unsigned int uiLen;
	unsigned long long ullConnID;
	char szErrMsg[1024];
	unsigned long long ullTaskID;
	TcpSockNotifyType enNotifyType;
	WebSockNotifyType enWebNotifyType;
public:
	NotifyTask(): pBuf(nullptr), uiLen(0), ullConnID(0), ullTaskID(0){
		memset(szErrMsg, 0, 1024);
	}
	~NotifyTask() {
		if (nullptr != pBuf)
		{
			// wyl 2026-03-30：任务对象负责释放自己的数据缓冲，避免通知路径持续泄漏。
			delete[]pBuf;
			pBuf = nullptr;
		}
	}
};

#endif
