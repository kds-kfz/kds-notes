#ifndef _WEB_SERVER_LISTERNET_H_
#define _WEB_SERVER_LISTERNET_H_

#include <stdio.h>
#include <sys/stat.h>  
#include <iostream>
#include <algorithm>

#include "SocketInterface.h"
#include "HPSocket.h"

using namespace std;

/************************************************************************
名称：IComplexHttp 组件监听器基接口
描述：定义 IComplexHttp 组件监听器的所有事件
************************************************************************/
class  CWebServerListerNet : public IHttpServerListener
{
	//开始解析通知
	EnHttpParseResult OnMessageBegin(IHttpServer* pSender, CONNID dwConnID){ return HPR_OK; }
	// wyl 2026-03-30：以下回调签名中的 LPCSTR / BOOL 来自当前 HP-Socket 监听器接口定义，不能直接改成 const char* / bool，否则会破坏 override；本次新增逻辑内部尽量统一使用标准 C++ 类型。
	//请求行解析完成通知（仅用于 HTTP 服务端）
	EnHttpParseResult OnRequestLine(IHttpServer* pSender, CONNID dwConnID, LPCSTR lpszMethod, LPCSTR lpszUrl) override;
	//BODY 报文通知
	EnHttpParseResult OnBody(IHttpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength)
	{
		// wyl 2026-03-30：Web 模块当前不在 OnBody() 中按分片自动回 HTTP 响应，避免 chunked 或多次 body 回调时出现重复响应、半截响应。
		//pSender->SendLocalFile(dwConnID, "f:\\Desktop\\桌面文件夹整理\\青岛站点4.png", HSC_OK, nullptr, nullptr, 0);
		return HPR_OK;
	}
	//状态行解析完成通知（仅用于 HTTP 客户端）
	EnHttpParseResult OnStatusLine(IHttpServer* pSender, CONNID dwConnID, USHORT usStatusCode, LPCSTR lpszDesc){ return HPR_OK; }
	//请求头通知
	EnHttpParseResult OnHeader(IHttpServer* pSender, CONNID dwConnID, LPCSTR lpszName, LPCSTR lpszValue) override;
	//请求头完成通知
	EnHttpParseResult OnHeadersComplete(IHttpServer* pSender, CONNID dwConnID) override;
	//Chunked 报文头通知
	EnHttpParseResult OnChunkHeader(IHttpServer* pSender, CONNID dwConnID, int iLength){ return HPR_OK; }
	//Chunked 报文结束通知
	EnHttpParseResult OnChunkComplete(IHttpServer* pSender, CONNID dwConnID){ return HPR_OK; }
	//完成解析通知
	EnHttpParseResult OnMessageComplete(IHttpServer* pSender, CONNID dwConnID)
	{
		//pSender->Send(dwConnID, (BYTE *)"123456\r\n", 8, 0);
		//pSender->SendSmallFile(dwConnID, L"f:\\Desktop\\桌面文件夹整理\\新建文本文档 (3).html", nullptr, nullptr);
		return HPR_OK;
	}
	//升级协议通知
	EnHttpParseResult OnUpgrade(IHttpServer* pSender, CONNID dwConnID, EnHttpUpgradeType enUpgradeType) override;
	//解析错误通知
	EnHttpParseResult OnParseError(IHttpServer* pSender, CONNID dwConnID, int iErrorCode, LPCSTR lpszErrorDesc){ return HPR_OK; }
	//WebSocket 数据包头通知
	EnHandleResult OnWSMessageHeader(IHttpServer* pSender, CONNID dwConnID, BOOL bFinal, BYTE iReserved, BYTE iOperationCode, const BYTE lpszMask[4], ULONGLONG ullBodyLen) override;
	//WebSocket 数据包体通知
	EnHandleResult OnWSMessageBody(IHttpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override;
	//WebSocket 数据包完成通知
	EnHandleResult OnWSMessageComplete(IHttpServer* pSender, CONNID dwConnID) override;
	//准备监听通知
	EnHandleResult OnPrepareListen(ITcpServer* pSender, SOCKET soListen) override;
	//客户端连接事件 接收到连接时触发
	EnHandleResult OnAccept(ITcpServer* pSender, CONNID dwConnID, UINT_PTR soClient) override;
	//服务器占用端口事件 握手成功时触发
	EnHandleResult OnHandShake(ITcpServer* pSender, CONNID dwConnID) override;
	//发送数据完成事件 发送数据成功时触发
	EnHandleResult OnSend(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override;
	//数据到达通知（PUSH 模型）
	EnHandleResult OnReceive(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) { return HR_OK; }
	//数据到达通知（PULL 模型）
	EnHandleResult OnReceive(ITcpServer* pSender, CONNID dwConnID, int iLength) override;
	// 客户端关闭事件 关闭某个连接时触发
	EnHandleResult OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) override;
	//服务器关闭时触发
	EnHandleResult OnShutdown(ITcpServer* pSender) override;

public:
	CWebServerListerNet() {}
	~CWebServerListerNet() {}
};

extern CHPThreadPoolPtr g_thread_pool;
#endif
