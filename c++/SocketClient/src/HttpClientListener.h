#ifndef SOCKET_HTTP_CLIENT_LISTENER_H
#define SOCKET_HTTP_CLIENT_LISTENER_H

#include "ClientHPSocket.h"

class ClientInstance;

// 每个 HTTP 或 WebSocket 实例独占一个 HP 协议监听器。
class HttpClientListener final : public CHttpClientListener
{
public:
    // 将监听器绑定到唯一所属的客户端实例。
    explicit HttpClientListener(ClientInstance& p_refOwner);

    // 通知传输连接已经建立。
    EnHandleResult OnConnect(ITcpClient* p_pSender, CONNID p_ullConnId) override;
    // 通知传输连接关闭并清理协议状态。
    EnHandleResult OnClose(ITcpClient* p_pSender, CONNID p_ullConnId,
        EnSocketOperation p_enOperation, int p_iError) override;
    // 读取 HTTP 响应的状态行。
    EnHttpParseResult OnStatusLine(IHttpClient* p_pSender, CONNID p_ullConnId,
        USHORT p_unStatus, LPCSTR p_szDescription) override;
    // 读取 HTTP 响应中的单条头字段。
    EnHttpParseResult OnHeader(IHttpClient* p_pSender, CONNID p_ullConnId,
        LPCSTR p_szName, LPCSTR p_szValue) override;
    // 对有效的 101 响应通知 HP 解析器升级协议。
    EnHttpParseResult OnHeadersComplete(IHttpClient* p_pSender, CONNID p_ullConnId) override;
    // 转发 HTTP 正文片段。
    EnHttpParseResult OnBody(IHttpClient* p_pSender, CONNID p_ullConnId,
        const BYTE* p_byData, int p_iLength) override;
    // 通知一个 HTTP 响应已经解析完毕。
    EnHttpParseResult OnMessageComplete(IHttpClient* p_pSender, CONNID p_ullConnId) override;
    // 校验 WebSocket 升级时的挑战应答。
    EnHttpParseResult OnUpgrade(IHttpClient* p_pSender, CONNID p_ullConnId,
        EnHttpUpgradeType p_enUpgrade) override;
    // 向业务层报告协议解析错误。
    EnHttpParseResult OnParseError(IHttpClient* p_pSender, CONNID p_ullConnId,
        int p_iError, LPCSTR p_szDescription) override;
    // 校验 WebSocket 帧长度并准备当前接收缓存。
    EnHandleResult OnWSMessageHeader(IHttpClient* p_pSender, CONNID p_ullConnId,
        BOOL p_bFinal, BYTE p_byReserved, BYTE p_byOpcode,
        const BYTE p_byMask[4], ULONGLONG p_ullLength) override;
    // 追加已解析的 WebSocket 帧片段。
    EnHandleResult OnWSMessageBody(IHttpClient* p_pSender, CONNID p_ullConnId,
        const BYTE* p_byData, int p_iLength) override;
    // 将完整 WebSocket 帧交付业务层。
    EnHandleResult OnWSMessageComplete(IHttpClient* p_pSender, CONNID p_ullConnId) override;

private:
    ClientInstance& owner_; // 所属实例负责保证监听器生命周期。
};

#endif
