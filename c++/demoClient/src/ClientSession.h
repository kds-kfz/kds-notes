#ifndef DEMO_CLIENT_SESSION_H
#define DEMO_CLIENT_SESSION_H

#include "SocketClient.h"
#include <condition_variable>
#include <mutex>
#include <string>

// 单条异步连接的事件快照；回调写入，发起请求的线程等待结果。
struct ClientSession
{
    std::mutex mutex;                 // 保护本结构中的可变状态。
    std::condition_variable changed; // 通知请求线程状态已经变化。
    bool connected = false;          // 传输连接是否已建立。
    bool closed = false;             // 传输连接是否已关闭。
    bool opened = false;             // WebSocket 升级是否成功。
    bool complete = false;           // HTTP 应答是否已经接收完毕。
    unsigned short status = 0;       // HTTP 响应状态码。
    std::string data;                // 拷贝后的正文或回显数据。
};

// 在 SocketClient 回调线程中复制事件，避免保存失效的借用指针。
void OnClientEvent(CSocketClient* p_pClient, const ST_SOCKET_CLIENT_EVENT* p_pEvent,
    void* p_pContext);

// 使用一个独立命名实例完成一次 TCP、HTTP 或 WebSocket 回环校验。
bool RunClient(EN_SOCKET_CLIENT_TYPE p_enType, const std::string& p_strHost,
    unsigned short p_usPort, int p_iIndex, std::string& p_refError);

#endif
