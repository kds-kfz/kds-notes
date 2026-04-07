#pragma once

#include "xsdk_libraryop.h"
#include "SocketServerDll.h"
#include "jztprotocol.pb.h"
#define HTTP_DLL_NAME		"SocketServerDll.dll"

#define HTTP_RECVBUF_LEN (1024 * 1024)
#define HTTP_CONNECT_NUM (300)
#define HTTP_ACCEPT_NUM (1024 * 4)
#define HTTP_THREAD_NUM (0)
#define HTTP_QUEUE_NUM (1024 * 4)

//回调
void  HttpNotifyHandle(CHttpAsynReq *p_refReq);

class CHttpSockMng
{
public:
	CHttpSockMng();
	~CHttpSockMng();

	static CHttpSockMng* GetInstance();
	void Release();

	bool Start(const char *p_szIp, unsigned short p_nPort, int p_iThreadNum = HTTP_THREAD_NUM, int p_iQueueNum = HTTP_QUEUE_NUM,
		int p_iRBufLen = HTTP_RECVBUF_LEN, int p_iMaxConnectNum = HTTP_CONNECT_NUM, int p_iMaxAcceptNum = HTTP_ACCEPT_NUM,
		bool p_bSSL = false, const char *p_szPemCertFile = nullptr, const char *p_szPemKeyFile = nullptr,
		const char *p_szKeyPassword = nullptr, const char *p_szCAPemCertFileOrPath = nullptr,
		char *p_szLogFold = nullptr);
	void Stop();
	bool HttpProcess(CHttpAsynReq *p_refReq);
	bool InitHttpServerInfo(const char* p_sHomePath);
	static int SendResponse(CHttpAsynReq *p_refReq, const char* p_szData, int p_iDataLen);
	CSocketServer* HttpHandle() { return m_pHttpServerHandle; }
	bool IsHttpUrl(const char *p_szUrl);
	Jzt::SetCodeType SetCodeTypeConvert(short p_nSetcodeType);
private:
	static CHttpSockMng *m_pThis;
	CSocketServer *m_pHttpServerHandle;
	CLibraryOp *m_pclLibraryOp;
	bool m_bStatus;					//初始化状态
	std::map<string, bool> m_mapHttpUrl;//url,是否注册
	void RegisterUrl();
};

