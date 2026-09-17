#include "ClientSession.h"

#include <atomic>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace
{
// 仅接受有界十进制数，防止端口计算溢出和意外创建过多线程。
bool ParseNumber(const char* p_szText, unsigned long p_ulLimit, unsigned long& p_refValue)
{
    if (!p_szText || !*p_szText) return false;
    char* pEnd = nullptr;
    const unsigned long value = std::strtoul(p_szText, &pEnd, 10);
    if (!pEnd || *pEnd != '\0' || value == 0 || value > p_ulLimit) return false;
    p_refValue = value;
    return true;
}
}

// 同时发起多条 TCP/HTTP/WebSocket 连接，与 SocketEchoServer 三个端口配对。
int main(int p_iArgc, char** p_ppszArgv)
{
    unsigned long port = 39071, count = 2;
    const std::string host = p_iArgc > 1 ? p_ppszArgv[1] : "127.0.0.1";
    if (p_iArgc > 4 || (p_iArgc > 2 && !ParseNumber(p_ppszArgv[2], 65533, port)) ||
        (p_iArgc > 3 && !ParseNumber(p_ppszArgv[3], 256, count)))
    {
        std::cerr << "usage: demoClient [host] [base-port] [clients-per-protocol]" << std::endl;
        return 2;
    }
    if (GetSocketClientAbiVersion() != SOCKET_CLIENT_ABI_VERSION)
    {
        std::cerr << "SocketClient ABI mismatch" << std::endl;
        return 1;
    }
    std::atomic<int> failures(0);
    std::vector<std::thread> workers;
    std::mutex outputMutex;
    for (int type = EN_SOCKET_CLIENT_TCP; type <= EN_SOCKET_CLIENT_WEB; ++type)
        for (unsigned long index = 0; index < count; ++index)
            workers.emplace_back([&, type, index]() {
                const auto protocol = static_cast<EN_SOCKET_CLIENT_TYPE>(type);
                const unsigned short targetPort = static_cast<unsigned short>(port +
                    (type == EN_SOCKET_CLIENT_TCP ? 0 : type == EN_SOCKET_CLIENT_HTTP ? 1 : 2));
                std::string error;
                if (!RunClient(protocol, host, targetPort, static_cast<int>(index), error))
                {
                    ++failures;
                    std::lock_guard<std::mutex> lock(outputMutex);
                    std::cerr << error << std::endl;
                }
            });
    for (auto& worker : workers) worker.join();
    if (failures != 0) return 1;
    std::cout << "PASS: " << count << " TCP + " << count << " HTTP + "
        << count << " WebSocket clients" << std::endl;
    return 0;
}
