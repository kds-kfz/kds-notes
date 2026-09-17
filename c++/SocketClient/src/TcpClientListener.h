#ifndef SOCKET_TCP_CLIENT_LISTENER_H
#define SOCKET_TCP_CLIENT_LISTENER_H

#include "ClientHPSocket.h"

class ClientInstance;

// 每个 TCP 客户端实例独占一个监听器，回调不查询全局工厂。
class TcpClientListener final : public CTcpClientListener
{
public:
    // 将监听器绑定到唯一所属的客户端实例。
    explicit TcpClientListener(ClientInstance& p_refOwner);

    // 建立连接后更新实例状态。
    EnHandleResult OnConnect(ITcpClient* p_pSender, CONNID p_ullConnId) override;

    // 连接关闭后通知业务错误码。
    EnHandleResult OnClose(ITcpClient* p_pSender, CONNID p_ullConnId,
        EnSocketOperation p_enOperation, int p_iError) override;

    // 原样转发 TCP 字节片段，不假定业务报文边界。
    EnHandleResult OnReceive(ITcpClient* p_pSender, CONNID p_ullConnId,
        const BYTE* p_byData, int p_iLength) override;

private:
    ClientInstance& owner_; // 所属实例负责保证监听器生命周期。
};

#endif
