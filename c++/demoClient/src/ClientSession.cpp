#include "ClientSession.h"

#include <chrono>

// 保存事件数据和状态；所有回调内容只在回调期间有效。
void OnClientEvent(CSocketClient* p_pClient, const ST_SOCKET_CLIENT_EVENT* p_pEvent,
    void* p_pContext)
{
    (void)p_pClient;
    ClientSession& state = *static_cast<ClientSession*>(p_pContext);
    std::lock_guard<std::mutex> lock(state.mutex);
    if (p_pEvent->enEvent == EN_SOCKET_CLIENT_CONNECTED) state.connected = true;
    if (p_pEvent->enEvent == EN_SOCKET_CLIENT_CLOSED ||
        p_pEvent->enEvent == EN_SOCKET_CLIENT_HTTP_ERROR) state.closed = true;
    if (p_pEvent->enEvent == EN_SOCKET_CLIENT_WEB_OPEN) state.opened = true;
    if (p_pEvent->enEvent == EN_SOCKET_CLIENT_HTTP_STATUS) state.status = p_pEvent->usStatus;
    if (p_pEvent->enEvent == EN_SOCKET_CLIENT_HTTP_COMPLETE) state.complete = true;
    if ((p_pEvent->enEvent == EN_SOCKET_CLIENT_DATA ||
        p_pEvent->enEvent == EN_SOCKET_CLIENT_HTTP_BODY ||
        p_pEvent->enEvent == EN_SOCKET_CLIENT_WEB_FRAME) &&
        p_pEvent->pData && p_pEvent->iDataLength > 0)
        state.data.append(static_cast<const char*>(p_pEvent->pData), p_pEvent->iDataLength);
    state.changed.notify_all();
}

namespace
{
// 限时等待指定事件条件，连接提前关闭也立即终止等待。
template<class Predicate>
bool Wait(ClientSession& p_refState, Predicate p_predicate)
{
    std::unique_lock<std::mutex> lock(p_refState.mutex);
    return p_refState.changed.wait_for(lock, std::chrono::seconds(5),
        [&]() { return p_predicate(p_refState) || p_refState.closed; }) &&
        p_predicate(p_refState);
}

// 按创建协议调用配对的工厂删除方法。
void DeleteClient(EN_SOCKET_CLIENT_TYPE p_enType, CSocketClient*& p_refClient)
{
    if (!p_refClient) return;
    p_refClient->Stop();
    if (p_enType == EN_SOCKET_CLIENT_TCP) DelTcpClientInstance(p_refClient);
    else if (p_enType == EN_SOCKET_CLIENT_HTTP) DelHttpClientInstance(p_refClient);
    else DelWebClientInstance(p_refClient);
}
}

// 每个工作线程只操作自己的命名实例，统一检查连接和响应内容。
bool RunClient(EN_SOCKET_CLIENT_TYPE p_enType, const std::string& p_strHost,
    unsigned short p_usPort, int p_iIndex, std::string& p_refError)
{
    const std::string name = "demo-" + std::to_string(p_enType) + "-" +
        std::to_string(p_iIndex);
    CSocketClient* client = p_enType == EN_SOCKET_CLIENT_TCP ?
        CreateTcpClientInstanceByName(name.c_str()) :
        p_enType == EN_SOCKET_CLIENT_HTTP ?
        CreateHttpClientInstanceByName(name.c_str()) :
        CreateWebClientInstanceByName(name.c_str());
    if (!client) { p_refError = name + ": create failed"; return false; }
    ClientSession state;
    bool ok = client->Connect(p_strHost.c_str(), p_usPort, OnClientEvent, &state);
    if (!ok || !Wait(state, [](const ClientSession& p_refState) { return p_refState.connected; }))
        p_refError = name + ": connect failed or timed out";
    else if (p_enType == EN_SOCKET_CLIENT_TCP)
    {
        const std::string message = "tcp-hello-" + std::to_string(p_iIndex);
        ok = client->Send(message.data(), static_cast<int>(message.size())) &&
            Wait(state, [&](const ClientSession& p_refState) { return p_refState.data == message; });
        if (!ok) p_refError = name + ": TCP echo mismatch";
    }
    else if (p_enType == EN_SOCKET_CLIENT_HTTP)
    {
        ok = client->Request("GET", "/health", nullptr, nullptr, 0, nullptr, 0) &&
            Wait(state, [](const ClientSession& p_refState) {
                return p_refState.complete && p_refState.status == 200 &&
                    p_refState.data == "http-ok";
            });
        if (!ok)
            p_refError = name + ": HTTP response mismatch";
    }
    else
    {
        const std::string host = p_strHost + ":" + std::to_string(p_usPort);
        ok = client->Upgrade("/echo", host.c_str()) &&
            Wait(state, [](const ClientSession& p_refState) { return p_refState.opened; });
        if (!ok) p_refError = name + ": WebSocket upgrade failed";
        else
        {
            const std::string message = "web-hello-" + std::to_string(p_iIndex);
            ok = client->SendFrame(1, message.data(), static_cast<int>(message.size())) &&
                Wait(state, [&](const ClientSession& p_refState) { return p_refState.data == message; });
            if (!ok) p_refError = name + ": WebSocket echo mismatch";
        }
    }
    DeleteClient(p_enType, client);
    return p_refError.empty();
}
