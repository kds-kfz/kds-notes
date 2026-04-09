#if !defined(KDSC_TCP_SOCK_MANAGE_H)
#define KDSC_TCP_SOCK_MANAGE_H

#include "nsdk_libraryop.h"
#include "SocketServer.h"
#include "UserData.h"

#define TCP_DLL_NAME	"libSocketServer.dll"
#define TCP_SO_NAME		"libsocketserver.so"

#define TCP_RECVBUF_LEN (1024 * 1024)
#define TCP_CONNECT_NUM (300)
#define TCP_ACCEPT_NUM (1024 * 4)
#define TCP_THREAD_NUM (16)
#define TCP_QUEUE_NUM (1024 * 4)

//启动;
typedef CSocketServer *(*pfnCreateTcpSockInstance)();

//释放;
typedef void(*pfnDelTcpSockInstance)(CSocketServer *&);

class CTcpSockMng
{
public:
	CTcpSockMng();
	~CTcpSockMng();

	static CTcpSockMng* GetInstance();
	void Release();

	bool Start(const char *p_szIp, unsigned short p_nPort, int p_iThreadNum = TCP_THREAD_NUM, int p_iQueueNum = TCP_QUEUE_NUM,
		int p_iRBufLen = TCP_RECVBUF_LEN, int p_iMaxConnectNum = TCP_CONNECT_NUM, int p_iMaxAcceptNum = TCP_ACCEPT_NUM, char *p_szLogFold = nullptr);
	
	void Stop();

	int ProcessAsynAns(NetRequsetDat *p_stNode, const char *p_szTransfer);//不应答包头;

	bool InitTcpServerInfo(const char* p_sHomePath);

	void TcpSockSend(void* p_refServerHandle, void *p_refClinetHandle, const char* p_szData, int p_iDataLen);
	void TcpSockClose(void* p_refServerHandle, void *p_refClinetHandle, const char* p_szData, int p_iDataLen);
	int TcpSockCompare(void* p_refSrcClinetHandle, void *p_refObjClinetHandle);
private:
	static CTcpSockMng *m_pThis;
	CLibraryOp *m_pclLibraryOp;
	static CSocketServer *m_pTcpServerHandle;
	pfnDelTcpSockInstance m_fnDelTcpSockInstance;
	bool m_bStatus;					//初始化状态;
};

#endif