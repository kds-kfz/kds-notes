#include "TcpClientListener.h"
#include "ClientInstance.h"

// 保存所属实例引用，避免在接收热路径查找工厂。
TcpClientListener::TcpClientListener(ClientInstance& p_refOwner) : owner_(p_refOwner) {}

// 通知业务层传输连接已经建立。
EnHandleResult TcpClientListener::OnConnect(ITcpClient* p_pSender, CONNID p_ullConnId)
{ owner_.Connected(); return HR_OK; }

// 清理连接状态并转发底层关闭错误码。
EnHandleResult TcpClientListener::OnClose(ITcpClient* p_pSender, CONNID p_ullConnId,
    EnSocketOperation p_enOperation, int p_iError)
{ owner_.Closed(p_iError); return HR_OK; }

// 不缓存借用的底层缓冲区；由业务回调决定是否复制。
EnHandleResult TcpClientListener::OnReceive(ITcpClient* p_pSender, CONNID p_ullConnId,
    const BYTE* p_byData, int p_iLength)
{ owner_.Data(p_byData, p_iLength); return HR_OK; }
