#include "HttpClientListener.h"
#include "ClientInstance.h"
#include "ClientProtocol.h"
#include <cstring>

// 保存所属客户端引用，各实例的 HTTP 解析状态互不共享。
HttpClientListener::HttpClientListener(ClientInstance& p_refOwner) : m_refOwner(p_refOwner) {}

// 明文 HTTP/WebSocket 在 TCP 建连后即可发送；TLS 必须等待证书校验和握手完成。
EnHandleResult HttpClientListener::OnConnect(ITcpClient* p_pSender, CONNID p_ullConnId)
{
    if (!m_refOwner.IsSecure()) m_refOwner.Connected();
    return HR_OK;
}

// HTTPS/WSS 只有收到该事件后才允许上层发送 HTTP 请求或升级报文。
EnHandleResult HttpClientListener::OnHandShake(ITcpClient* p_pSender, CONNID p_ullConnId)
{
    if (m_refOwner.IsSecure()) m_refOwner.Connected();
    return HR_OK;
}

// 通知实例传输连接关闭。
EnHandleResult HttpClientListener::OnClose(ITcpClient* p_pSender, CONNID p_ullConnId,
    EnSocketOperation p_enOperation, int p_iError)
{ m_refOwner.Closed(p_iError); return HR_OK; }

// 向实例转发 HTTP 响应状态码和描述。
EnHttpParseResult HttpClientListener::OnStatusLine(IHttpClient* p_pSender, CONNID p_ullConnId,
    USHORT p_unStatus, LPCSTR p_szDescription)
{ m_refOwner.Status(p_unStatus, p_szDescription); return HPR_OK; }

// 向实例转发 HTTP 响应头或握手校验头。
EnHttpParseResult HttpClientListener::OnHeader(IHttpClient* p_pSender, CONNID p_ullConnId,
    LPCSTR p_szName, LPCSTR p_szValue)
{
    try { m_refOwner.Header(p_szName, p_szValue); return HPR_OK; }
    catch (...) { return HPR_ERROR; }
}

// 返回 HPR_UPGRADE 才会令 HP 切换到 WebSocket 帧回调。
EnHttpParseResult HttpClientListener::OnHeadersComplete(IHttpClient* p_pSender, CONNID p_ullConnId)
{ return m_refOwner.ShouldUpgrade() ? HPR_UPGRADE : HPR_OK; }

// 正文缓冲区仅在当前回调期间有效。
EnHttpParseResult HttpClientListener::OnBody(IHttpClient* p_pSender, CONNID p_ullConnId,
    const BYTE* p_byData, int p_iLength)
{
    return m_refOwner.HttpBody(p_byData, p_iLength) ? HPR_OK : HPR_ERROR;
}

// 通知一个 HTTP 响应已经完成。
EnHttpParseResult HttpClientListener::OnMessageComplete(IHttpClient* p_pSender, CONNID p_ullConnId)
{ m_refOwner.HttpComplete(); return HPR_OK; }

// 拒绝不匹配的 WebSocket 挑战应答。
EnHttpParseResult HttpClientListener::OnUpgrade(IHttpClient* p_pSender, CONNID p_ullConnId,
    EnHttpUpgradeType p_enUpgrade)
{ return m_refOwner.WebOpened(p_enUpgrade) ? HPR_OK : HPR_ERROR; }

// 转发解析器错误，不让用户代码异常越过底层回调边界。
EnHttpParseResult HttpClientListener::OnParseError(IHttpClient* p_pSender, CONNID p_ullConnId,
    int p_iError, LPCSTR p_szDescription)
{
    m_refOwner.HttpFailed();
    ST_SOCKET_CLIENT_EVENT event = ClientProtocol::Event(EN_SOCKET_CLIENT_HTTP_ERROR);
    event.iErrorCode = p_iError;
    event.pData = p_szDescription;
    event.iDataLength = p_szDescription ? static_cast<int>(strlen(p_szDescription)) : 0;
    m_refOwner.Notify(event);
    return HPR_OK;
}

// 单帧长度超限时仅使当前连接的解析失败。
EnHandleResult HttpClientListener::OnWSMessageHeader(IHttpClient* p_pSender,
    CONNID p_ullConnId, BOOL p_bFinal, BYTE p_byReserved, BYTE p_byOpcode,
    const BYTE p_byMask[4], ULONGLONG p_ullLength)
{
    try { return m_refOwner.FrameHeader(p_bFinal, p_byOpcode, p_ullLength) ? HR_OK : HR_ERROR; }
    catch (...) { return HR_ERROR; }
}

// 在所属实例的有界帧缓存中追加数据。
EnHandleResult HttpClientListener::OnWSMessageBody(IHttpClient* p_pSender, CONNID p_ullConnId,
    const BYTE* p_byData, int p_iLength)
{
    try { return m_refOwner.FrameBody(p_byData, p_iLength) ? HR_OK : HR_ERROR; }
    catch (...) { return HR_ERROR; }
}

// 完成后向业务层交付整帧内容。
EnHandleResult HttpClientListener::OnWSMessageComplete(IHttpClient* p_pSender,
    CONNID p_ullConnId)
{ m_refOwner.FrameComplete(); return HR_OK; }
