#ifndef _WEB_SOCK_SERVER_OBJ_H_
#define _WEB_SOCK_SERVER_OBJ_H_

#include "SocketServer.h"

#include <cstdint>
#include <memory>
#include <string>

struct ST_WEB_SERVER_RUNTIME;

class CWebSockServerObj : public CSocketServer
{
public:
	// 创建带固定逻辑名称和实例编号的 WebSocket Server；运行状态由本对象独占。
	CWebSockServerObj(const std::string& p_refServiceName,
		std::uint64_t p_ullInstanceId);
	~CWebSockServerObj();
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
	virtual bool CreateHttpSock(const char*, unsigned short, unsigned int, unsigned int, unsigned int,
		HTTP_NOTIFY_PROC, unsigned int, unsigned int, char*, const char* = nullptr) {
		return false;
	}
	virtual bool CreateHttpsSock(const char*, unsigned short, unsigned int, unsigned int, unsigned int,
		HTTP_NOTIFY_PROC, unsigned int, unsigned int, char*,
		const char* = nullptr, const char* = nullptr,
		const char* = nullptr, const char* = nullptr,
		const char* = nullptr) {
		return false;
	}
	// 释放请求
	virtual bool DelHttpAsynReq(unsigned long long) { return false; }
	// 关闭服务
	virtual void StopHttpSock() {}

	/********** WEB服务模块 **********/
	virtual bool CreateWebSock(const char *p_szIp, unsigned short p_unPort, unsigned int p_uiRBufLen, unsigned int p_uiMaxConnectNum, unsigned int p_uiMaxAcceptNum,
		WEB_NOTIFY_PROC p_webHandle, unsigned int p_uiThreadNum, unsigned int p_uiQueueNum, char *p_szErr, const char *p_szLogFold = nullptr);
	virtual bool CreateWssSock(const char *p_szIp, unsigned short p_unPort, unsigned int p_uiRBufLen, unsigned int p_uiMaxConnectNum, unsigned int p_uiMaxAcceptNum,
		WEB_NOTIFY_PROC p_wssHandle, unsigned int p_uiThreadNum, unsigned int p_uiQueueNum, char *p_szErr,
		const char *p_szPemCertFile = nullptr, const char *p_szPemKeyFile = nullptr,
		const char *p_szKeyPassword = nullptr, const char *p_szCAPemCertFileOrPath = nullptr,
		const char *p_szLogFold = nullptr);
	// 关闭服务;
	virtual void StopWebSock();
	// 发送应答;
	virtual bool WebSockSend(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen);
	// 发送 UTF-8 文本帧。
	virtual bool WebSockSendText(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen);
	// 关闭客户连接;
	virtual void WebSockClose(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen);
	// 发送 UTF-8 文本错误信息后关闭连接。
	virtual void WebSockCloseText(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen);
	// 比较彼此客户端是否一致;
	virtual int WebSockCompare(void* p_refSrcClient, void* p_refObjClient);
	// 查询客户端连接是否仍可发送;
	virtual bool WebSockIsAlive(void *p_refServer, void *p_refClient);
	// 查询底层尚未发出的字节数，上层据此识别持续拥塞的慢连接。
	virtual bool WebSockGetPendingDataLength(void *p_refServer, void *p_refClient, int& p_refIPendingBytes);
	// 监听启动前保存独立 TCP listen 队列长度。
	virtual bool SetSocketListenQueue(unsigned int p_uiSocketListenQueue);

private:
	std::unique_ptr<ST_WEB_SERVER_RUNTIME> m_ptrRuntime; // 独占 WebSocket 网络、连接和线程状态。
};

#endif
