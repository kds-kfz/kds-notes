#ifndef _TCP_SERVER_LISTERNET_H_
#define _TCP_SERVER_LISTERNET_H_

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
class  CTcpServerListerNet : public ITcpServerListener
{
	// 客户端连接事件 监听成功时触发
	EnHandleResult OnPrepareListen(ITcpServer* pSender, SOCKET soListen) override;

	// 客户端连接事件 接收到连接时触发
	EnHandleResult OnAccept(ITcpServer* pSender, CONNID dwConnID, UINT_PTR soClient) override;

	// 客户端关闭事件 关闭某个连接时触发
	EnHandleResult OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode) override;

	// 发送数据完成事件 发送数据成功时触发
	EnHandleResult OnSend(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override;

	// 接收到数据事件 收到数据时触发
	EnHandleResult OnReceive(ITcpServer* pSender, CONNID dwConnID, const BYTE* pData, int iLength) override;

	EnHandleResult OnReceive(ITcpServer* pSender, CONNID dwConnID, int iLength) override;

	// 服务器占用端口事件 握手成功时触发
	EnHandleResult OnHandShake(ITcpServer* pSender, CONNID dwConnID) override;

	//服务器关闭时触发
	EnHandleResult OnShutdown(ITcpServer* pSender) override;

public:
	CTcpServerListerNet() {}
	~CTcpServerListerNet() {}
};

#endif
