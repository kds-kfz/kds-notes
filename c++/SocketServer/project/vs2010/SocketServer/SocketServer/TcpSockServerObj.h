#ifndef _TCP_SOCK_SERVER_OBJ_H_
#define _TCP_SOCK_SERVER_OBJ_H_

#include "SocketServer.h"

class CTcpSockServerObj : public CSocketServer
{
public:
	CTcpSockServerObj();
	virtual ~CTcpSockServerObj();

public:
	/********** TCP服务模块 **********/
	virtual bool CreateTcpSock(const char *p_szIp, unsigned short p_unPort, unsigned int p_uiRBufLen, unsigned int p_uiMaxConnectNum, unsigned int p_uiMaxAcceptNum,
		TCP_NOTIFY_PROC p_tcpHandle, unsigned int p_uiThreadNum, unsigned int p_uiQueueNum, char *p_szErr, const char *p_szLogFold = nullptr);
	// 关闭服务
	virtual void StopTcpSock();
	// 发送应答
	virtual void TcpSockSend(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen);
	// 关闭客户连接
	virtual void TcpSockClose(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen);
	// 比较彼此客户端是否一致;
	virtual int TcpSockCompare(void *p_refSrcClient, void *p_refObjClient);

	/********** HTTP服务模块 **********/
	virtual bool CreateHttpSock(const char *p_szIp, unsigned short p_unPort, unsigned int p_uiRBufLen, unsigned int p_uiMaxConnectNum, unsigned int p_uiMaxAcceptNum,
		HTTP_NOTIFY_PROC p_httpHandle, unsigned int p_uiThreadNum, unsigned int p_uiQueueNum, char *p_szErr, const char *p_szLogFold = nullptr) {
		return true;
	}
	virtual bool CreateHttpsSock(const char *p_szIp, unsigned short p_unPort, unsigned int p_uiRBufLen, unsigned int p_uiMaxConnectNum, unsigned int p_uiMaxAcceptNum,
		HTTP_NOTIFY_PROC p_httpsHandle, unsigned int p_uiThreadNum, unsigned int p_uiQueueNum, char *p_szErr,
		const char *p_szPemCertFile = nullptr, const char *p_szPemKeyFile = nullptr,
		const char *p_szKeyPassword = nullptr, const char *p_szCAPemCertFileOrPath = nullptr,
		const char *p_szLogFold = nullptr) {
		return true;
	}
	// 释放请求
	virtual bool DelHttpAsynReq(unsigned long long p_lluReqId) { return true; }
	// 关闭服务
	virtual void StopHttpSock() {}

	/********** WEBSOCKET服务模块 **********/
	virtual bool CreateWebSock(const char *p_szIp, unsigned short p_unPort, unsigned int p_uiRBufLen, unsigned int p_uiMaxConnectNum, unsigned int p_uiMaxAcceptNum,
		WEB_NOTIFY_PROC p_webHandle, unsigned int p_uiThreadNum, unsigned int p_uiQueueNum, char *p_szErr, const char *p_szLogFold = nullptr) {
		return true;
	}
	virtual bool CreateWssSock(const char *p_szIp, unsigned short p_unPort, unsigned int p_uiRBufLen, unsigned int p_uiMaxConnectNum, unsigned int p_uiMaxAcceptNum,
		WEB_NOTIFY_PROC p_wssHandle, unsigned int p_uiThreadNum, unsigned int p_uiQueueNum, char *p_szErr,
		const char *p_szPemCertFile = nullptr, const char *p_szPemKeyFile = nullptr,
		const char *p_szKeyPassword = nullptr, const char *p_szCAPemCertFileOrPath = nullptr,
		const char *p_szLogFold = nullptr) {
		return true;
	}
	// 关闭服务
	virtual void StopWebSock() {}
	// 发送应答
	virtual void WebSockSend(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen) {}
	// 关闭客户连接
	virtual void WebSockClose(void *p_refServer, void *p_refClient, const char *p_szData, int p_iDataLen) {}

};

#endif