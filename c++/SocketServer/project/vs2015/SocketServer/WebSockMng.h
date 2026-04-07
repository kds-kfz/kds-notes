#pragma once

#include "xsdk_libraryop.h"
#include "SocketServerDll.h"

#define HTTP_DLL_NAME		"SocketServerDll.dll"

//int p_iRBufLen, int p_iMaxConnectNum, int p_iMaxAcceptNum, int p_iThreadNum, int p_iQueueNum
#define WEB_RECVBUF_LEN (1024 * 1024)
#define WEB_CONNECT_NUM (300)
#define WEB_ACCEPT_NUM (1024 * 4)
#define WEB_THREAD_NUM (0)
#define WEB_QUEUE_NUM (1024 * 4)

//启动
typedef CSocketServer *(*pfnCreateWebSockInstance)();

//释放
typedef void(*pfnDelWebSockInstance)(CSocketServer *&);

class CWebSockProtobufMng 
{
public:
	CWebSockProtobufMng();
	~CWebSockProtobufMng();

	static CWebSockProtobufMng* GetInstance();
	void Release();

	bool Start(const char *p_szIp, unsigned short p_nPort, int p_iThreadNum = WEB_THREAD_NUM, int p_iQueueNum = WEB_QUEUE_NUM,
		int p_iRBufLen = WEB_RECVBUF_LEN, int p_iMaxConnectNum = WEB_CONNECT_NUM, int p_iMaxAcceptNum = WEB_ACCEPT_NUM,
		bool p_bSSL = false, const char *p_szPemCertFile = nullptr, const char *p_szPemKeyFile = nullptr,
		const char *p_szKeyPassword = nullptr, const char *p_szCAPemCertFileOrPath = nullptr,
		char *p_szLogFold = nullptr);
	void Stop();

	DWORD ProcessAsynAns(NetRequsetDat * pNode, const char * pTransfer);

	bool InitWebServerInfo(const char* p_sHomePath);
	
	void WebSockSend(void* p_refServerHandle, void *p_refClinetHandle, const char* p_szData, int p_iDataLen);
	void WebSockClose(void* p_refServerHandle, void *p_refClinetHandle, const char* p_szData, int p_iDataLen);

private:
	static CWebSockProtobufMng *m_pThis;
	CLibraryOp *m_pclLibraryOp;
	static CSocketServer *m_pWebServerHandle;
	pfnDelWebSockInstance m_fnDelWebSockInstance;
	bool m_bStatus;					//初始化状态
};

