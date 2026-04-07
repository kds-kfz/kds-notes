#ifndef _HTTP_SERVER_LISTERNET_H_
#define _HTTP_SERVER_LISTERNET_H_

#include "SocketInterface.h"
#include "HPSocket.h"

class CHttpServerListerNet : public IHttpServerListener
{
	// 开始解析 HTTP 报文时，向监听器发送该通知
	EnHttpParseResult OnMessageBegin(IHttpServer* pSender, CONNID dwConnID) override;
	// 请求行解析完成通知（仅用于 HTTP 服务端）
	EnHttpParseResult OnRequestLine(IHttpServer* pSender, CONNID dwConnID, LPCSTR lpszMethod, LPCSTR lpszUrl) override;
	// 每当接收到 HTTP BODY 报文，向监听器发送该通知
	EnHttpParseResult OnBody(IHttpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override;
	// 状态行解析完成通知（仅用于 HTTP 客户端）
	EnHttpParseResult OnStatusLine(IHttpServer*, CONNID, USHORT, LPCSTR) override { return HPR_OK; }
	// 每当解析完成一个请求头后，向监听器发送该通知
	EnHttpParseResult OnHeader(IHttpServer* pSender, CONNID dwConnID, LPCSTR lpszName, LPCSTR lpszValue) override;
	// 解析完成所有请求头后，向监听器发送该通知
	EnHttpParseResult OnHeadersComplete(IHttpServer* pSender, CONNID dwConnID) override;
	// 每当解析出一个 Chunked 报文头，向监听器发送该通知
	EnHttpParseResult OnChunkHeader(IHttpServer*, CONNID, int) override { return HPR_OK; }
	// 每当解析完一个 Chunked 报文，向监听器发送该通知
	EnHttpParseResult OnChunkComplete(IHttpServer*, CONNID) override { return HPR_OK; }
	// 每当解析完成一个完整 HTTP 报文，向监听器发送该通知
	EnHttpParseResult OnMessageComplete(IHttpServer* pSender, CONNID dwConnID) override;
	// 当需要升级协议时，向监听器发送该通知
	EnHttpParseResult OnUpgrade(IHttpServer* pSender, CONNID dwConnID, EnHttpUpgradeType enUpgradeType) override;
	// 当解析 HTTP 报文错误时，向监听器发送该通知
	EnHttpParseResult OnParseError(IHttpServer* pSender, CONNID dwConnID, int iErrorCode, LPCSTR lpszErrorDesc) override;
	// 当解析 WebSocket 数据包头时，向监听器发送该通知
	EnHandleResult OnWSMessageHeader(IHttpServer* pSender, CONNID dwConnID, BOOL bFinal, BYTE iReserved, BYTE iOperationCode, const BYTE lpszMask[4], ULONGLONG ullBodyLen) override;
	// 当接收到 WebSocket 数据包体时，向监听器发送该通知
	EnHandleResult OnWSMessageBody(IHttpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override;
	// 完整接收一个 WebSocket 数据包时，向监听器发送该通知
	EnHandleResult OnWSMessageComplete(IHttpServer* pSender, CONNID dwConnID) override;
	// 准备监听通知
	EnHandleResult OnPrepareListen(ITcpServer* pSender, SOCKET soListen) override;
	// 接收连接通知
	EnHandleResult OnAccept(ITcpServer* pSender, CONNID dwConnID, UINT_PTR soClient) override;
	// 握手完成通知
	EnHandleResult OnHandShake(ITcpServer* pSender, CONNID dwConnID) override;
	// 已发送数据通知
	EnHandleResult OnSend(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override;
	// 数据到达通知（PUSH 模型）
	EnHandleResult OnReceive(ITcpServer*, CONNID, const BYTE*, int) override { return HR_OK; }
	// 数据到达通知（PULL 模型）
	EnHandleResult OnReceive(ITcpServer* pSender, CONNID dwConnID, int iLength) override;
	// 通信错误通知
	EnHandleResult OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) override;
	// 关闭通信组件通知
	EnHandleResult OnShutdown(ITcpServer* pSender) override;

public:
	CHttpServerListerNet() {}
	~CHttpServerListerNet() {}
};

#endif

