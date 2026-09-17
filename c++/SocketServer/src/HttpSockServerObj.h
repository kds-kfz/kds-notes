#ifndef _HTTP_SOCK_SERVER_OBJ_H_
#define _HTTP_SOCK_SERVER_OBJ_H_

#include "SocketServer.h"

#include <cstdint>
#include <memory>
#include <string>

struct ST_HTTP_SERVER_RUNTIME;

class CHttpSockServerObj : public CSocketServer
{
public:
	// 创建带固定逻辑名称和实例编号的 HTTP Server；运行状态由本对象独占。
	CHttpSockServerObj(const std::string& p_refServiceName,
		std::uint64_t p_ullInstanceId);
	~CHttpSockServerObj();
	// 复制当前实例运行信息；线程安全且不返回内部字符串指针。
	bool FillRuntimeInfo(ST_SOCKET_SERVER_RUNTIME_INFO& p_refInfo) const;

public:
	/********** TCP服务模块 **********/
	virtual bool CreateTcpSock(const char*, unsigned short, unsigned int, unsigned int, unsigned int,
		TCP_NOTIFY_PROC, unsigned int, unsigned int, char*, const char* = nullptr) { return false; }
	// 关闭服务
	virtual void StopTcpSock() { return; }
	// 发送应答
	virtual bool TcpSockSend(void*, void*, const char*, int) { return false; }
	// 关闭客户连接
	virtual void TcpSockClose(void*, void*, const char*, int) { return; }
	// 比较彼此客户端是否一致;
	virtual int TcpSockCompare(void*, void*) { return -1; }
	// 查询客户端连接是否仍可发送;
	virtual bool TcpSockIsAlive(void*, void*) { return false; }

	/********** HTTP服务模块 **********/
	virtual bool CreateHttpSock(const char* p_szIp, unsigned short p_unPort, unsigned int p_uiRBufLen, unsigned int p_uiMaxConnectNum, unsigned int p_uiMaxAcceptNum,
		HTTP_NOTIFY_PROC p_httpHandle, unsigned int p_uiThreadNum, unsigned int p_uiQueueNum, char* p_szErr, const char* p_szLogFold = nullptr);
	virtual bool CreateHttpsSock(const char* p_szIp, unsigned short p_unPort, unsigned int p_uiRBufLen, unsigned int p_uiMaxConnectNum, unsigned int p_uiMaxAcceptNum,
		HTTP_NOTIFY_PROC p_httpsHandle, unsigned int p_uiThreadNum, unsigned int p_uiQueueNum, char* p_szErr,
		const char* p_szPemCertFile = nullptr, const char* p_szPemKeyFile = nullptr,
		const char* p_szKeyPassword = nullptr, const char* p_szCAPemCertFileOrPath = nullptr,
		const char* p_szLogFold = nullptr);
	// 释放请求
	virtual bool DelHttpAsynReq(unsigned long long p_lluReqId);
	// 关闭服务
	virtual void StopHttpSock();

	/********** WEB服务模块 **********/
	virtual bool CreateWebSock(const char*, unsigned short, unsigned int, unsigned int, unsigned int,
		WEB_NOTIFY_PROC, unsigned int, unsigned int, char*, const char* = nullptr) {
		return false;
	}
	virtual bool CreateWssSock(const char*, unsigned short, unsigned int, unsigned int, unsigned int,
		WEB_NOTIFY_PROC, unsigned int, unsigned int, char*,
		const char* = nullptr, const char* = nullptr,
		const char* = nullptr, const char* = nullptr,
		const char* = nullptr) {
		return false;
	}
	// 关闭服务;
	virtual void StopWebSock() {}
	// 发送应答;
	virtual bool WebSockSend(void*, void*, const char*, int) { return false; }
	virtual bool WebSockSendText(void*, void*, const char*, int) { return false; }
	// 关闭客户连接;
	virtual void WebSockClose(void*, void*, const char*, int) {}
	virtual void WebSockCloseText(void*, void*, const char*, int) {}
	// 比较彼此客户端是否一致;
	virtual int WebSockCompare(void* p_refSrcClient, void* p_refObjClient) { return -1; }
	// 查询客户端连接是否仍可发送;
	virtual bool WebSockIsAlive(void*, void*) { return false; }
	// HTTP 实例不承载 WebSocket 连接。
	virtual bool WebSockGetPendingDataLength(void*, void*, int&) { return false; }
	// 监听启动前保存独立 TCP listen 队列长度。
	virtual bool SetSocketListenQueue(unsigned int p_uiSocketListenQueue);

private:
	std::unique_ptr<ST_HTTP_SERVER_RUNTIME> m_ptrRuntime; // 独占 HTTP 网络、请求和线程状态。
};

#endif
