#ifndef _SOCKET_SERVER_H_
#define _SOCKET_SERVER_H_

// Base define.
#if defined(WIN32) || defined(WIN64) || defined(_WIN32) || defined(_WIN64) || defined(_INC_WINDOWS)
#if !defined(OS_IS_WINDOWS)
#undef OS_IS_LINUX
#define OS_IS_WINDOWS
#endif
#else
#undef OS_IS_WINDOWS
#if !defined(OS_IS_LINUX)
#define OS_IS_LINUX
#endif
#endif

#if defined(OS_IS_WINDOWS)
#define SOCKETSERVER_API       __declspec(dllexport)
#else
#define SOCKETSERVER_API
#endif  // defined(OS_IS_WINDOWS)

#define STR_IP_LEN 32

/********** TCP 消息类型 **********/

/********** HTTP 状态类型 **********/
enum HttpStatusType
{
	OK = 200,
	Created = 201,
	Accepted = 202,
	NoContent = 204,
	MultipleChoices = 300,
	MovedPermanently = 301,
	MovedTemporarily = 302,
	NotModified = 304,
	BadRequest = 400,
	Unauthorized = 401,
	Forbidden = 403,
	NotFound = 404,
	InternalServerError = 500,
	NotImplemented = 501,
	BadGateway = 502,
	ServiceUnavailable = 503
};

/********** WEBSOCKET 消息类型 **********/
enum WebSockNotifyType
{
	enWebClose = 0,	// 连接断开, 由内部组件断开连接。异常断开时,携带错误信息;
	enWebConnect,	// 连接成功;
	enWebError,		// 网络出错, 内部错误，不断开连接,由上层应用断开;
	enWebData,		// 读取数据;
};

/********** TCP 消息类型 **********/
enum TcpSockNotifyType
{
	enTcpClose = 0,	// 连接断开, 由内部组件断开连接。异常断开时,携带错误信息;
	enTcpConnect,	// 连接成功;
	enTcpError,		// 网络出错, 内部错误，不断开连接,由上层应用断开;
	enTcpData,		// 读取数据;
};

/**********
* 名称：TCP 回调函数;
* 描述：回调函数需应用层实现;
* 参数：;
* 返回值：;
* 无;
**********/

class CHttpAsynReq
{
public:
	// 请求;
	virtual const char* GetUrl() = 0;
	virtual const char* GetMethodType() = 0;
	virtual const int GetContentLen() = 0;
	virtual const void* GetContent() = 0;
	virtual const char* GetHead(const char* p_szName) = 0;
	virtual void GetAddress(char *p_szClientIp, int p_iIpLen, unsigned short &p_nClientPort) = 0;
	virtual unsigned long long GetConnId() = 0;
	virtual unsigned long long GetConnAsyId() = 0;

	// 应答包;
	virtual void SetResponseStatus(HttpStatusType p_enStatus) = 0;
	// 添加head;
	virtual void AddResponseHead(const char* p_szName, const char* p_szValue) = 0;
	// 发送;
	virtual void SendResponse(const void* p_szData, int p_iLen) = 0;
};

/**********
* 名称：HTTP 请求发送句柄;
* 描述：业务处理完成后，请求包由上层应用释放;
* 参数：CHttpAsynReq;
* 返回值：;
* 无;
**********/
typedef void(*HTTP_NOTIFY_PROC)(CHttpAsynReq *p_refReq);

/**********
* 名称：WEBSOCKET 回调函数;
* 描述：回调函数需应用层实现;
* 参数：;
* p_refServerHandle -- 服务器句柄;
* p_refClinetHandle -- 客户端句柄;
* p_enType -- 通知类型;
* p_szData -- 包体内容;
* p_iDataLen -- 包体长度;
* p_szClientIp -- 客户端ip;
* p_nClientPort -- 客户端port;
* p_szErrData -- 错误信息,1024字节buf;
* 返回值：;
* 无;
**********/
typedef void(*WEB_NOTIFY_PROC)(void *p_refServerHandle, void *p_refClinetHandle, WebSockNotifyType p_enType,
	const void *p_szData, int p_iDataLen, const char *p_szClientIp, unsigned short p_nClientPort, void *p_szErrData);

/**********
* 名称：TCP 回调函数;
* 描述：回调函数需应用层实现;
* 参数：;
* p_refServerHandle -- 服务器句柄;
* p_refClinetHandle -- 客户端句柄;
* p_enType -- 通知类型;
* p_szData -- 包体内容;
* p_uiDataLen -- 包体长度;
* p_szClientIp -- 客户端ip;
* p_nClientPort -- 客户端port;
* p_szErrData -- 错误信息,1024字节buf;
* 返回值：;
* 无;
**********/
typedef void(*TCP_NOTIFY_PROC)(void *p_refServerHandle, void *p_refClinetHandle, TcpSockNotifyType p_enType,
	const void *p_szData, unsigned int p_uiDataLen, const char *p_szClientIp, unsigned short p_unClientPort, void *p_szErrData);

//服务句柄定义;
typedef struct TAG_HS* HS;			//服务端;
typedef struct TAG_HCLIENT *HCLIENT;//客户端;

#ifdef __cplusplus
extern "C"
{
#endif

	class SOCKETSERVER_API CSocketServer
	{
	public:
		CSocketServer() {}
		~CSocketServer() {}

		/********** TCP服务模块 **********/
		/********** 创建服务 **********
		p_szIp：绑定的IP
		p_nPort：绑定的端口
		p_uiRBufLen：接收缓冲区长度
		p_uiMaxConnectNum：最大连接数
		p_uiMaxAcceptNum：Accept 预分配数量（底层监听 accept 队列大小，不是同一IP最大连接数）
		p_httpHandle：通知回调
		p_iThreadNum：处理线程数,为0表示cpu数乘以2
		p_iQueueNum：处理队列数
		p_szErr：错误信息，如果CreateHttpSock失败，则包含了出错信息，内存由外部分配，不小1024字节
		p_szLogFold：日志路径。nullptr：默认不建立日志；default：工作路径下/Day_Logs；其他：自定义日志全路径
		******************************/
		virtual bool CreateTcpSock(const char *p_szIp, unsigned short p_unPort, unsigned int p_uiRBufLen, unsigned int p_uiMaxConnectNum, unsigned int p_uiMaxAcceptNum,
			TCP_NOTIFY_PROC p_tcpHandle, unsigned int p_uiThreadNum, unsigned int p_uiQueueNum, char *p_szErr, const char *p_szLogFold = nullptr) = 0;
		// 关闭服务;
		virtual void StopTcpSock() = 0;
		// 发送应答;
		virtual void TcpSockSend(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen) = 0;
		// 关闭客户连接;
		virtual void TcpSockClose(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen) = 0;
		// 比较彼此客户端是否一致;
		virtual int TcpSockCompare(void *p_refSrcClient, void *p_refObjClient) = 0;

		/********** HTTP服务模块 **********/
		/********** 创建服务 **********
		p_szIp：绑定的IP
		p_nPort：绑定的端口
		p_uiRBufLen：接收缓冲区长度
		p_uiMaxConnectNum：最大连接数
		p_uiMaxAcceptNum：Accept 预分配数量（底层监听 accept 队列大小，不是同一IP最大连接数）
		p_httpHandle：通知回调
		p_uiThreadNum：处理线程数,为0表示cpu数乘以2
		p_uiQueueNum：处理队列数
		p_szErr：错误信息，如果CreateHttpSock失败，则包含了出错信息，内存由外部分配，不小1024字节
		p_szLogFold：日志路径。nullptr：默认不建立日志；default：工作路径下/Day_Logs；其他：自定义日志全路径
		CreateHttpsSock：创建https服务
		p_szPemCertFile：证书文件
		p_szPemKeyFile：私钥文件
		p_szKeyPassword：私钥密码（没有密码则为空）
		p_szCAPemCertFileOrPath：CA 证书文件或目录（单向验证或客户端可选）;
		******************************/
		virtual bool CreateHttpSock(const char *p_szIp, unsigned short p_unPort, unsigned int p_uiRBufLen, unsigned int p_uiMaxConnectNum, unsigned int p_uiMaxAcceptNum,
			HTTP_NOTIFY_PROC p_httpHandle, unsigned int p_uiThreadNum, unsigned int p_uiQueueNum, char *p_szErr, const char *p_szLogFold = nullptr) = 0;
		virtual bool CreateHttpsSock(const char *p_szIp, unsigned short p_unPort, unsigned int p_uiRBufLen, unsigned int p_uiMaxConnectNum, unsigned int p_uiMaxAcceptNum,
			HTTP_NOTIFY_PROC p_httpsHandle, unsigned int p_uiThreadNum, unsigned int p_uiQueueNum, char *p_szErr,
			const char *p_szPemCertFile = nullptr, const char *p_szPemKeyFile = nullptr,
			const char *p_szKeyPassword = nullptr, const char *p_szCAPemCertFileOrPath = nullptr,
			const char *p_szLogFold = nullptr) = 0;
		// 释放请求;
		virtual bool DelHttpAsynReq(unsigned long long p_lluReqId) = 0;
		// 关闭服务;
		virtual void StopHttpSock() = 0;

		/********** WEBSOCKET服务模块 **********/
		/********** 创建服务 **********
		p_szIp：绑定的IP
		p_nPort：绑定的端口
		p_uiRBufLen：接收缓冲区长度
		p_uiMaxConnectNum：最大连接数
		p_uiMaxAcceptNum：Accept 预分配数量（底层监听 accept 队列大小，不是同一IP最大连接数）
		p_handle：通知回调
		p_uiThreadNum：处理线程数,为0表示cpu数乘以2
		p_uiQueueNum：处理队列数
		p_szErr：错误信息，如果CreateWebSock失败，则包含了出错信息，内存由外部分配，不小1024字节
		p_szLogFold：日志路径。nullptr：默认不建立日志；default：工作路径下/Day_Logs；其他：自定义日志全路径
		CreateWebsSock：创建wss服务
		p_szPemCertFile：证书文件
		p_szPemKeyFile：私钥文件
		p_szKeyPassword：私钥密码（没有密码则为空）
		p_szCAPemCertFileOrPath：CA 证书文件或目录（单向验证或客户端可选）;
		******************************/
		virtual bool CreateWebSock(const char *p_szIp, unsigned short p_unPort, unsigned int p_uiRBufLen, unsigned int p_uiMaxConnectNum, unsigned int p_uiMaxAcceptNum,
			WEB_NOTIFY_PROC p_webHandle, unsigned int p_uiThreadNum, unsigned int p_uiQueueNum, char *p_szErr, const char *p_szLogFold = nullptr) = 0;
		virtual bool CreateWssSock(const char *p_szIp, unsigned short p_unPort, unsigned int p_uiRBufLen, unsigned int p_uiMaxConnectNum, unsigned int p_uiMaxAcceptNum,
			WEB_NOTIFY_PROC p_wssHandle, unsigned int p_uiThreadNum, unsigned int p_uiQueueNum, char *p_szErr,
			const char *p_szPemCertFile = nullptr, const char *p_szPemKeyFile = nullptr,
			const char *p_szKeyPassword = nullptr, const char *p_szCAPemCertFileOrPath = nullptr,
			const char *p_szLogFold = nullptr) = 0;
		// 关闭服务;
		virtual void StopWebSock() = 0;
		// 发送应答;
		virtual void WebSockSend(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen) = 0;
		// 关闭客户连接;
		virtual void WebSockClose(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen) = 0;
	};
	/***************************************************************************
	接口说明: 用于获取实例, 内部单例;
	参数说明: NA;
	返回值:  CSocketServer *对象指针;
	***************************************************************************/
	SOCKETSERVER_API CSocketServer* CreateHttpSockInstance();

	/***************************************************************************
	接口说明: 用于释放实例;
	参数说明: CSocketServer *&对象指针;
	返回值:  void;
	***************************************************************************/
	SOCKETSERVER_API void DelHttpSockInstance(CSocketServer *&pIns);

	/***************************************************************************
	接口说明: 用于获取实例, 内部单例;
	参数说明: NA;
	返回值:  CSocketServer *对象指针;
	***************************************************************************/
	SOCKETSERVER_API CSocketServer* CreateWebSockInstance();

	/***************************************************************************
	接口说明: 用于释放实例;
	参数说明: CSocketServer *&对象指针;
	返回值:  void;
	***************************************************************************/
	SOCKETSERVER_API void DelWebSockInstance(CSocketServer *&pIns);

	/***************************************************************************
	接口说明: 用于获取实例, 内部单例;
	参数说明: NA;
	返回值:  CSocketServer *对象指针;
	***************************************************************************/
	SOCKETSERVER_API CSocketServer* CreateTcpSockInstance();

	/***************************************************************************
	接口说明: 用于释放实例;
	参数说明: CSocketServer *&对象指针;
	返回值:  void;
	***************************************************************************/
	SOCKETSERVER_API void DelTcpSockInstance(CSocketServer *&pIns);




#ifdef __cplusplus
}
#endif

#endif //


