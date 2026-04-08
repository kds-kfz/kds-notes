#ifndef _WEB_SOCK_SERVER_OBJ_H_
#define _WEB_SOCK_SERVER_OBJ_H_

#include "SocketServer.h"

class CWebSockServerObj : public CSocketServer
{
public:
	CWebSockServerObj();
	~CWebSockServerObj();

public:
	/********** TCP服务模块 **********/
	virtual bool CreateTcpSock(const char*, unsigned short, unsigned int, unsigned int, unsigned int,
		TCP_NOTIFY_PROC, unsigned int, unsigned int, char*, const char* = nullptr) { return false; }
	// 关闭服务
	virtual void StopTcpSock() { return; }
	// 发送应答
	virtual void TcpSockSend(void*, void*, const char*, int) { return; }
	// 关闭客户连接
	virtual void TcpSockClose(void*, void*, const char*, int) { return; }
	// 比较彼此客户端是否一致;
	virtual int TcpSockCompare(void*, void*) { return -1; }

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
	virtual void WebSockSend(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen);
	// 关闭客户连接;
	virtual void WebSockClose(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen);

};

#endif
