#include "SocketServer.h"

#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <thread>
#include <chrono>

namespace
{
CSocketServer* g_pTcpServer = nullptr;  // TCP 回调所对应的服务实例。
CSocketServer* g_pHttpServer = nullptr; // HTTP 回调所对应的服务实例。
CSocketServer* g_pWebServer = nullptr;  // WebSocket 回调所对应的服务实例。

// 将 TCP 收到的字节原样发回当前连接，供客户端检查动态库间的数据路径。
void OnTcp(void* p_pServer, void* p_pClient, TcpSockNotifyType p_enType,
    const void* p_pData, unsigned int p_uiLength, const char* p_szIp,
    unsigned short p_usPort, void* p_pError)
{
    (void)p_szIp; (void)p_usPort; (void)p_pError;
    if (p_enType == enTcpData && p_pData != nullptr && p_uiLength != 0 &&
        p_uiLength <= static_cast<unsigned int>(std::numeric_limits<int>::max()))
        g_pTcpServer->TcpSockSend(p_pServer, p_pClient,
            static_cast<const char*>(p_pData), static_cast<int>(p_uiLength));
}

// 返回固定的 HTTP 正文，并释放服务端持有的异步请求对象。
void OnHttp(CHttpAsynReq* p_pRequest)
{
    if (p_pRequest == nullptr) return;
    const char szBody[] = "http-ok";
    p_pRequest->SetResponseStatus(OK);
    p_pRequest->SendResponse(szBody, static_cast<int>(sizeof(szBody) - 1));
    g_pHttpServer->DelHttpAsynReq(p_pRequest->GetConnAsyId());
}

// 将客户端文本消息发回同一 WebSocket 连接。
void OnWeb(void* p_pServer, void* p_pClient, WebSockNotifyType p_enType,
    const void* p_pData, int p_iLength, const char* p_szIp,
    unsigned short p_usPort, void* p_pError)
{
    (void)p_szIp; (void)p_usPort; (void)p_pError;
    if (p_enType == enWebData && p_pData != nullptr && p_iLength > 0)
        g_pWebServer->WebSockSendText(p_pServer, p_pClient,
            static_cast<const char*>(p_pData), p_iLength);
}

// 停止三个监听器并按对应协议销毁命名实例。
void StopServers()
{
    if (g_pWebServer) { g_pWebServer->StopWebSock(); DelWebSockInstance(g_pWebServer); }
    if (g_pHttpServer) { g_pHttpServer->StopHttpSock(); DelHttpSockInstance(g_pHttpServer); }
    if (g_pTcpServer) { g_pTcpServer->StopTcpSock(); DelTcpSockInstance(g_pTcpServer); }
}

// 仅接受十进制端口或运行秒数，拒绝空值及溢出。
bool ParseNumber(const char* p_szText, unsigned long p_ulLimit, unsigned long& p_refValue)
{
    if (!p_szText || !*p_szText) return false;
    char* pEnd = nullptr;
    const unsigned long value = std::strtoul(p_szText, &pEnd, 10);
    if (!pEnd || *pEnd != '\0' || value > p_ulLimit) return false;
    p_refValue = value;
    return true;
}
}

// 默认监听 127.0.0.1:39071/39072/39073；指定秒数后自动退出。
int main(int p_iArgc, char** p_ppszArgv)
{
    unsigned long port = 39071, seconds = 0;
    if (p_iArgc > 3 || (p_iArgc > 1 &&
        (!ParseNumber(p_ppszArgv[1], 65533, port) || port == 0)) ||
        (p_iArgc > 2 && !ParseNumber(p_ppszArgv[2], 3600, seconds)))
    {
        std::cerr << "usage: SocketEchoServer [base-port] [seconds]" << std::endl;
        return 2;
    }
    if (GetSocketServerAbiVersion() != SOCKET_SERVER_ABI_VERSION)
    {
        std::cerr << "SocketServer ABI mismatch" << std::endl;
        return 1;
    }
    g_pTcpServer = CreateTcpSockInstanceByName("demo-echo-tcp");
    g_pHttpServer = CreateHttpSockInstanceByName("demo-echo-http");
    g_pWebServer = CreateWebSockInstanceByName("demo-echo-web");
    char error[1024] = {};
    const bool started = g_pTcpServer && g_pHttpServer && g_pWebServer &&
        g_pTcpServer->CreateTcpSock("127.0.0.1", static_cast<unsigned short>(port),
            65536, 1024, 64, OnTcp, 2, 1024, error) &&
        g_pHttpServer->CreateHttpSock("127.0.0.1", static_cast<unsigned short>(port + 1),
            65536, 1024, 64, OnHttp, 2, 1024, error) &&
        g_pWebServer->CreateWebSock("127.0.0.1", static_cast<unsigned short>(port + 2),
            65536, 1024, 64, OnWeb, 2, 1024, error);
    if (!started)
    {
        std::cerr << "start failed: " << error << std::endl;
        StopServers();
        return 1;
    }
    std::cout << "READY tcp=" << port << " http=" << port + 1
        << " web=" << port + 2 << std::endl;
    if (seconds != 0)
        std::this_thread::sleep_for(std::chrono::seconds(seconds));
    else
    {
        std::cout << "Press Enter to stop" << std::endl;
        std::string line;
        std::getline(std::cin, line);
    }
    StopServers();
    return 0;
}
