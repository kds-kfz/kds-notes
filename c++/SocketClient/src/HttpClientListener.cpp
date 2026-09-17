#include "HttpClientListener.h"
#include "ClientInstance.h"
#include "ClientProtocol.h"
#include <cstring>

// 保存所属客户端引用，各实例的 HTTP 解析状态互不共享。
HttpClientListener::HttpClientListener(ClientInstance& p_refOwner) : owner_(p_refOwner) {}

// 通知实例传输连接已经建立。
EnHandleResult HttpClientListener::OnConnect(ITcpClient* p_pSender, CONNID p_ullConnId)
{ owner_.Connected(); return HR_OK; }

// 通知实例传输连接关闭。
EnHandleResult HttpClientListener::OnClose(ITcpClient* p_pSender, CONNID p_ullConnId,
    EnSocketOperation p_enOperation, int p_iError)
{ owner_.Closed(p_iError); return HR_OK; }

// 向实例转发 HTTP 响应状态码和描述。
EnHttpParseResult HttpClientListener::OnStatusLine(IHttpClient* p_pSender, CONNID p_ullConnId,
    USHORT p_unStatus, LPCSTR p_szDescription)
{ owner_.Status(p_unStatus, p_szDescription); return HPR_OK; }

// 向实例转发 HTTP 响应头或握手校验头。
EnHttpParseResult HttpClientListener::OnHeader(IHttpClient* p_pSender, CONNID p_ullConnId,
    LPCSTR p_szName, LPCSTR p_szValue)
{
    try { owner_.Header(p_szName, p_szValue); return HPR_OK; }
    catch (...) { return HPR_ERROR; }
}

// 返回 HPR_UPGRADE 才会令 HP 切换到 WebSocket 帧回调。
EnHttpParseResult HttpClientListener::OnHeadersComplete(IHttpClient* p_pSender, CONNID p_ullConnId)
{ return owner_.ShouldUpgrade() ? HPR_UPGRADE : HPR_OK; }

// 正文缓冲区仅在当前回调期间有效。
EnHttpParseResult HttpClientListener::OnBody(IHttpClient* p_pSender, CONNID p_ullConnId,
    const BYTE* p_byData, int p_iLength)
{
    ST_SOCKET_CLIENT_EVENT event = ClientProtocol::Event(EN_SOCKET_CLIENT_HTTP_BODY);
    event.pData = p_byData;
    event.iDataLength = p_iLength;
    owner_.Notify(event);
    return HPR_OK;
}

// 通知一个 HTTP 响应已经完成。
EnHttpParseResult HttpClientListener::OnMessageComplete(IHttpClient* p_pSender, CONNID p_ullConnId)
{ owner_.Notify(ClientProtocol::Event(EN_SOCKET_CLIENT_HTTP_COMPLETE)); return HPR_OK; }

// 拒绝不匹配的 WebSocket 挑战应答。
EnHttpParseResult HttpClientListener::OnUpgrade(IHttpClient* p_pSender, CONNID p_ullConnId,
    EnHttpUpgradeType p_enUpgrade)
{ return owner_.WebOpened(p_enUpgrade) ? HPR_OK : HPR_ERROR; }

// 转发解析器错误，不让用户代码异常越过底层回调边界。
EnHttpParseResult HttpClientListener::OnParseError(IHttpClient* p_pSender, CONNID p_ullConnId,
    int p_iError, LPCSTR p_szDescription)
{
    ST_SOCKET_CLIENT_EVENT event = ClientProtocol::Event(EN_SOCKET_CLIENT_HTTP_ERROR);
    event.iErrorCode = p_iError;
    event.pData = p_szDescription;
    event.iDataLength = p_szDescription ? static_cast<int>(strlen(p_szDescription)) : 0;
    owner_.Notify(event);
    return HPR_OK;
}

// 单帧长度超限时仅使当前连接的解析失败。
EnHandleResult HttpClientListener::OnWSMessageHeader(IHttpClient* p_pSender,
    CONNID p_ullConnId, BOOL p_bFinal, BYTE p_byReserved, BYTE p_byOpcode,
    const BYTE p_byMask[4], ULONGLONG p_ullLength)
{
    try { return owner_.FrameHeader(p_bFinal, p_byOpcode, p_ullLength) ? HR_OK : HR_ERROR; }
    catch (...) { return HR_ERROR; }
}

// 在所属实例的有界帧缓存中追加数据。
EnHandleResult HttpClientListener::OnWSMessageBody(IHttpClient* p_pSender, CONNID p_ullConnId,
    const BYTE* p_byData, int p_iLength)
{
    try { return owner_.FrameBody(p_byData, p_iLength) ? HR_OK : HR_ERROR; }
    catch (...) { return HR_ERROR; }
}

// 完成后向业务层交付整帧内容。
EnHandleResult HttpClientListener::OnWSMessageComplete(IHttpClient* p_pSender,
    CONNID p_ullConnId)
{ owner_.FrameComplete(); return HR_OK; }
