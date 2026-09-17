#include "SocketClient.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <bcrypt.h>
#include <wincrypt.h>
#include <chrono>
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "crypt32.lib")

// 即使 Release 定义了 NDEBUG，也能让校验失败终止测试。
void Check(bool p_bOk, const char* p_szMessage)
{ if (!p_bOk) throw std::runtime_error(p_szMessage); }

// 跨越任意 TCP 分段，准确读取指定字节数。
bool ReadBytes(SOCKET p_hSocket, char* p_szData, int p_iLength)
{
    for (int offset = 0; offset < p_iLength;)
    {
        int count = recv(p_hSocket, p_szData + offset, p_iLength - offset, 0);
        if (count <= 0) return false;
        offset += count;
    }
    return true;
}

// 测试端用系统加密 API 独立计算 Base64，避免再次实现编码算法。
std::string Base64(const unsigned char* p_byBytes, size_t p_uiSize)
{
    DWORD length = 0;
    const DWORD flags = CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF;
    Check(CryptBinaryToStringA(p_byBytes, static_cast<DWORD>(p_uiSize), flags,
        nullptr, &length) != FALSE, "Base64 length");
    std::string result(length, '\0');
    Check(CryptBinaryToStringA(p_byBytes, static_cast<DWORD>(p_uiSize), flags,
        &result[0], &length) != FALSE, "Base64 encode");
    result.resize(std::strlen(result.c_str()));
    return result;
}

// 独立计算测试服务端的 WebSocket 握手校验值。
std::string Accept(const std::string& p_strKey)
{
    std::string source = p_strKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    BCRYPT_ALG_HANDLE alg = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    DWORD size = 0, actual = 0;
    Check(BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA1_ALGORITHM, nullptr, 0)), "hash provider");
    Check(BCRYPT_SUCCESS(BCryptGetProperty(alg, BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&size), sizeof(size), &actual, 0)), "hash length");
    std::string buffer(size, '\0');
    Check(BCRYPT_SUCCESS(BCryptCreateHash(alg, &hash,
        reinterpret_cast<PUCHAR>(&buffer[0]), size, nullptr, 0, 0)), "create hash");
    Check(BCRYPT_SUCCESS(BCryptHashData(hash, reinterpret_cast<PUCHAR>(&source[0]),
        static_cast<ULONG>(source.size()), 0)), "hash data");
    unsigned char digest[20];
    Check(BCRYPT_SUCCESS(BCryptFinishHash(hash, digest, sizeof(digest), 0)), "finish hash");
    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(alg, 0);
    return Base64(digest, sizeof(digest));
}

// 为指定协议测试提供独立的本地回环服务。
class LoopbackServer
{
public:
    enum Kind { Tcp, Http, Web, WebBad, Stress }; // 本实例模拟的服务端协议类型。

    // 建立本地监听端口并启动独立的服务线程。
    explicit LoopbackServer(Kind p_enKind, int p_iConnections = 1)
        : kind_(p_enKind), count_(p_iConnections), socket_(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP))
    {
        Check(socket_ != INVALID_SOCKET, "server socket");
        sockaddr_in addr = {};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;
        Check(bind(socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0, "bind");
        Check(listen(socket_, SOMAXCONN) == 0, "listen");
        int len = sizeof(addr);
        Check(getsockname(socket_, reinterpret_cast<sockaddr*>(&addr), &len) == 0, "port");
        port_ = ntohs(addr.sin_port);
        worker_ = std::thread([this]() { Serve(); });
    }

    // 关闭监听套接字并等待服务线程结束。
    ~LoopbackServer()
    {
        closesocket(socket_);
        if (worker_.joinable()) worker_.join();
    }

    // 返回操作系统分配的回环监听端口。
    unsigned short Port() const { return port_; }

private:
    // 处理指定数量的独立回环客户端连接。
    void Serve()
    {
        for (int i = 0; i < count_; ++i)
        {
            SOCKET client = accept(socket_, nullptr, nullptr);
            if (client == INVALID_SOCKET) return;
            if (kind_ == Tcp)
            {
                char data[4];
                if (ReadBytes(client, data, 4)) send(client, data, 4, 0);
            }
            else if (kind_ == Stress)
            {
                char data[80];
                if (ReadBytes(client, data, 80))
                {
                    for (int sent = 0; sent < 80;)
                    {
                        const int n = send(client, data + sent, 80 - sent, 0);
                        if (n <= 0) break;
                        sent += n;
                    }
                }
            }
            else
            {
                std::string request;
                char byte;
                while (request.find("\r\n\r\n") == std::string::npos && request.size() < 8192)
                {
                    if (recv(client, &byte, 1, 0) != 1) break;
                    request.push_back(byte);
                }
                if (kind_ == Http)
                {
                    const char response[] = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n"
                        "Connection: close\r\n\r\nok";
                    send(client, response, sizeof(response) - 1, 0);
                }
                else
                {
                    const std::string marker = "Sec-WebSocket-Key: ";
                    const size_t first = request.find(marker);
                    if (first != std::string::npos)
                    {
                        const size_t start = first + marker.size();
                        const std::string accept = kind_ == WebBad ? "invalid" :
                            Accept(request.substr(start, request.find("\r\n", start) - start));
                        const std::string response = "HTTP/1.1 101 Switching Protocols\r\n"
                            "Upgrade: websocket\r\nConnection: Upgrade\r\n"
                            "Sec-WebSocket-Accept: " + accept + "\r\n\r\n";
                        send(client, response.data(), static_cast<int>(response.size()), 0);
                        if (kind_ == Web)
                        {
                            const unsigned char frame[] = {0x81, 0x02, 'o', 'k'};
                            send(client, reinterpret_cast<const char*>(frame), sizeof(frame), 0);
                            unsigned char header[2], masked[6];
                            if (ReadBytes(client, reinterpret_cast<char*>(header), 2) &&
                                header[0] == 0x81 && header[1] == 0x82 &&
                                ReadBytes(client, reinterpret_cast<char*>(masked), 6) &&
                                (masked[4] ^ masked[0]) == 'h' &&
                                (masked[5] ^ masked[1]) == 'i')
                            {
                                const unsigned char ack[] = {0x81, 0x01, 'y'};
                                send(client, reinterpret_cast<const char*>(ack), sizeof(ack), 0);
                            }
                        }
                    }
                }
            }
            shutdown(client, SD_BOTH);
            closesocket(client);
        }
    }

    Kind kind_; // 服务端协议类型。
    int count_; // 预期连接数量。
    SOCKET socket_; // 自有监听套接字。
    unsigned short port_; // 操作系统分配的回环端口。
    std::thread worker_; // 接受连接并回复数据的线程。
};

// 线程安全地累计 HP-Socket 工作线程投递的测试事件。
struct EventState
{
    std::mutex mutex; // 保护下列所有字段。
    std::condition_variable changed; // 通知等待线程收到新事件。
    int connects = 0; // 传输连接建立次数。
    int closes = 0; // 传输连接关闭次数。
    int status = 0; // 最近一次 HTTP 响应状态码。
    int opens = 0; // WebSocket 握手成功次数。
    std::string data; // 已复制的 TCP、HTTP 或 WebSocket 数据。
};

// 从 HP-Socket 回调返回之前复制所借用的事件数据。
void OnEvent(CSocketClient* p_pClient, const ST_SOCKET_CLIENT_EVENT* p_pEvent,
    void* p_pContext)
{
    (void)p_pClient;
    EventState& state = *static_cast<EventState*>(p_pContext);
    std::lock_guard<std::mutex> lock(state.mutex);
    if (p_pEvent->enEvent == EN_SOCKET_CLIENT_CONNECTED) ++state.connects;
    if (p_pEvent->enEvent == EN_SOCKET_CLIENT_CLOSED) ++state.closes;
    if (p_pEvent->enEvent == EN_SOCKET_CLIENT_HTTP_STATUS) state.status = p_pEvent->usStatus;
    if (p_pEvent->enEvent == EN_SOCKET_CLIENT_WEB_OPEN) ++state.opens;
    if (p_pEvent->enEvent == EN_SOCKET_CLIENT_DATA ||
        p_pEvent->enEvent == EN_SOCKET_CLIENT_HTTP_BODY ||
        p_pEvent->enEvent == EN_SOCKET_CLIENT_WEB_FRAME)
        state.data.append(static_cast<const char*>(p_pEvent->pData), p_pEvent->iDataLength);
    state.changed.notify_all();
}

// 在限定时间内等待事件条件，避免无条件休眠。
template<class Predicate> void Until(EventState& p_refState, Predicate p_predicate,
    const char* p_szFailure)
{
    std::unique_lock<std::mutex> lock(p_refState.mutex);
    Check(p_refState.changed.wait_for(lock, std::chrono::seconds(5), p_predicate), p_szFailure);
}

// 验证工厂唯一性、重连、TCP 实例隔离以及 HTTP/Web 解析。
int main()
{
    try
    {
        WSADATA wsa;
        Check(WSAStartup(MAKEWORD(2, 2), &wsa) == 0, "WSAStartup");
        Check(GetSocketClientAbiVersion() == SOCKET_CLIENT_ABI_VERSION, "ABI version");
        LoopbackServer tcpServer(LoopbackServer::Tcp, 3);
        LoopbackServer httpServer(LoopbackServer::Http);
        LoopbackServer webServer(LoopbackServer::Web);
        LoopbackServer badServer(LoopbackServer::WebBad);
        LoopbackServer stressServer(LoopbackServer::Stress, 4);
        LoopbackServer httpStressServer(LoopbackServer::Http, 4);
        CSocketClient* tcp1 = CreateTcpClientInstanceByName("quote-1");
        CSocketClient* tcp2 = CreateTcpClientInstanceByName("quote-2");
        CSocketClient* http = CreateHttpClientInstanceByName("quote-1");
        CSocketClient* web = CreateWebClientInstanceByName("quote-1");
        CSocketClient* badWeb = CreateWebClientInstanceByName("bad-web");
        Check(tcp1 && tcp2 && http && web && badWeb, "multi-instance factories");
        Check(!CreateTcpClientInstanceByName("quote-1"), "same-type duplicate");
        CSocketClient* wrong = tcp1;
        DelHttpClientInstance(wrong);
        Check(wrong == tcp1, "wrong-type deletion");
        ST_SOCKET_CLIENT_RUNTIME_INFO info = {};
        info.uiStructSize = sizeof(info);
        Check(GetSocketClientRuntimeInfo(web, &info) &&
            info.enType == EN_SOCKET_CLIENT_WEB, "runtime info");
        EventState a, b, h, w, bad;
        Check(tcp1->Connect("127.0.0.1", tcpServer.Port(), OnEvent, &a), "TCP 1 connect");
        Check(tcp2->Connect("127.0.0.1", tcpServer.Port(), OnEvent, &b), "TCP 2 connect");
        Until(a, [&]() { return a.connects > 0; }, "TCP 1 connected");
        Until(b, [&]() { return b.connects > 0; }, "TCP 2 connected");
        Check(tcp1->Send("one!", 4) && tcp2->Send("two!", 4), "TCP send");
        Until(a, [&]() { return a.data == "one!"; }, "TCP 1 echo");
        Until(b, [&]() { return b.data == "two!"; }, "TCP 2 echo");
        tcp2->Stop();
        {
            std::lock_guard<std::mutex> lock(b.mutex);
            b.data.clear();
        }
        Check(tcp2->Connect("127.0.0.1", tcpServer.Port(), OnEvent, &b), "TCP reconnect");
        Until(b, [&]() { return b.connects == 2; }, "TCP reconnected");
        Check(tcp2->Send("next", 4), "TCP reconnect send");
        Until(b, [&]() { return b.data == "next"; }, "TCP reconnect echo");
        Check(http->Connect("127.0.0.1", httpServer.Port(), OnEvent, &h), "HTTP connect");
        Until(h, [&]() { return h.connects > 0; }, "HTTP connected");
        Check(http->Request("GET", "/", nullptr, nullptr, 0, nullptr, 0), "HTTP request");
        Until(h, [&]() { return h.data == "ok"; }, "HTTP response");
        Check(h.status == 200, "HTTP status");
        Check(web->Connect("127.0.0.1", webServer.Port(), OnEvent, &w), "Web connect");
        Until(w, [&]() { return w.connects > 0; }, "Web connected");
        Check(web->Upgrade("/feed", "127.0.0.1"), "Web Upgrade request");
        Until(w, [&]() { return w.opens > 0 && w.data == "ok"; }, "Web frame");
        Check(web->SendFrame(1, "hi", 2), "Web masked send");
        Until(w, [&]() { return w.data == "oky"; }, "Web server verified mask");
        Check(badWeb->Connect("127.0.0.1", badServer.Port(), OnEvent, &bad), "bad Web connect");
        Until(bad, [&]() { return bad.connects > 0; }, "bad Web connected");
        Check(badWeb->Upgrade("/feed", "127.0.0.1"), "bad Web request");
        Until(bad, [&]() { return bad.closes > 0; }, "bad Web rejected");
        Check(bad.opens == 0, "bad Web cannot upgrade");

        // 多客户端同时连接，同一个实例也允许多个业务线程并发提交异步发送。
        std::array<CSocketClient*, 4> stressClients = {};
        std::array<EventState, 4> stressStates;
        for (int i = 0; i < 4; ++i)
        {
            const std::string name = "stress-tcp-" + std::to_string(i);
            stressClients[i] = CreateTcpClientInstanceByName(name.c_str());
            Check(stressClients[i] && stressClients[i]->Connect("127.0.0.1",
                stressServer.Port(), OnEvent, &stressStates[i]), "stress connect");
        }
        for (int i = 0; i < 4; ++i)
            Until(stressStates[i], [&]() { return stressStates[i].connects > 0; }, "stress connected");
        std::atomic<bool> sendsOk(true);
        std::vector<std::thread> senders;
        for (int i = 0; i < 8; ++i)
            senders.emplace_back([&, i]() {
                for (int j = 0; j < 10; ++j)
                    if (!stressClients[i / 2]->Send("load", 4)) sendsOk = false;
            });
        for (auto& sender : senders) sender.join();
        Check(sendsOk, "concurrent TCP send");
        for (int i = 0; i < 4; ++i)
        {
            Until(stressStates[i], [&]() { return stressStates[i].data.size() == 80; },
                "stress echo");
            DelTcpClientInstance(stressClients[i]);
        }

        // HTTP 请求从不同线程投递到多个实例，响应数据与连接互不混淆。
        std::array<CSocketClient*, 4> httpClients = {};
        std::array<EventState, 4> httpStates;
        for (int i = 0; i < 4; ++i)
        {
            const std::string name = "stress-http-" + std::to_string(i);
            httpClients[i] = CreateHttpClientInstanceByName(name.c_str());
            Check(httpClients[i] && httpClients[i]->Connect("127.0.0.1",
                httpStressServer.Port(), OnEvent, &httpStates[i]), "stress HTTP connect");
        }
        for (int i = 0; i < 4; ++i)
            Until(httpStates[i], [&]() { return httpStates[i].connects > 0; },
                "stress HTTP connected");
        senders.clear();
        for (int i = 0; i < 4; ++i)
            senders.emplace_back([&, i]() {
                if (!httpClients[i]->Request("GET", "/", nullptr, nullptr, 0, nullptr, 0))
                    sendsOk = false;
            });
        for (auto& sender : senders) sender.join();
        Check(sendsOk, "concurrent HTTP requests");
        for (int i = 0; i < 4; ++i)
        {
            Until(httpStates[i], [&]() { return httpStates[i].data == "ok"; },
                "stress HTTP response");
            DelHttpClientInstance(httpClients[i]);
        }
        DelTcpClientInstance(tcp1);
        Check(!tcp1 && tcp2, "isolated TCP deletion");
        DelTcpClientInstance(tcp2);
        DelHttpClientInstance(http);
        DelWebClientInstance(web);
        DelWebClientInstance(badWeb);
        WSACleanup();
        std::cout << "SocketClient multi-instance TCP/HTTP/Web: PASS\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "SocketClient test failed: " << error.what() << '\n';
        return 1;
    }
}
