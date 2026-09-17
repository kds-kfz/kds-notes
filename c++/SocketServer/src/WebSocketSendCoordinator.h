#ifndef _WEB_SOCKET_SEND_COORDINATOR_H_
#define _WEB_SOCKET_SEND_COORDINATOR_H_

#include "SocketInterface.h"

struct ST_WEB_SERVER_RUNTIME;

// 启动 WebSocket 发送协调器；服务开始接收业务发送前调用。
void StartWebSocketSendCoordinator(ST_WEB_SERVER_RUNTIME* p_pRuntime);
// 停止接收新发送并等待活动调用退出；底层 server 和连接锁必须在其返回后释放。
void StopWebSocketSendCoordinator(ST_WEB_SERVER_RUNTIME* p_pRuntime);
// 连接关闭时标记对应发送序列终止，阻止旧 ConnID 再创建并行发送序列。
void MarkWebSocketSendConnectionClosed(ST_WEB_SERVER_RUNTIME* p_pRuntime,
	CONNID p_ullConnID);
// 按单连接调用顺序发送控制帧，供 pong 等网络层帧与业务帧串行写入。
bool SendWebSocketControlFrameOrdered(ST_WEB_SERVER_RUNTIME* p_pRuntime,
	IHttpServer* p_pSender, CONNID p_ullConnID,
	BYTE p_byOperationCode, const BYTE* p_pData, int p_iDataLen, const char* p_szAction);
// 收到对端 Close 后按发送顺序回 Close 帧并断开，期间拒绝新增业务帧。
bool CloseWebSocketFromPeerOrdered(ST_WEB_SERVER_RUNTIME* p_pRuntime,
	IHttpServer* p_pSender, CONNID p_ullConnID);

#endif
