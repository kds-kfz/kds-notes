#ifndef _HTTP_SOCK_MNG_H_
#define _HTTP_SOCK_MNG_H_

#include <map>
#include <string>

#include "nsdk_libraryop.h"
#include "SocketServer.h"

#define HTTP_DLL_NAME		"libSocketServer.dll"

#define HTTP_RECVBUF_LEN (1024 * 64)//一般情况：32k/64k
#define HTTP_CONNECT_NUM (300)//一般情况：200/300/500
#define HTTP_ACCEPT_NUM (128)//一般情况：64/128/256
#define HTTP_THREAD_NUM (10)
#define HTTP_QUEUE_NUM (512)//一般情况：256/512/1024

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
private:
	static CHttpSockMng *m_pThis;
	CSocketServer *m_pHttpServerHandle;
	CLibraryOp *m_pclLibraryOp;
	bool m_bStatus;					//初始化状态
	std::map<std::string, bool> m_mapHttpUrl;//url,是否注册
	void RegisterUrl();
};

#endif

