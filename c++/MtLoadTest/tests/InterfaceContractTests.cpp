#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Windows.h>

#include "interface_test/InterfaceTestModel.h"
#include "interface_test/InterfaceTransport.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
// 测试断言统一入口；失败立即终止，避免后续检查建立在无效配置或句柄上。
void Check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

// 读取当前进程句柄数，用于验证反复请求后线程、事件和套接字均已释放。
DWORD CurrentHandleCount()
{
    DWORD count = 0;
    Check(::GetProcessHandleCount(::GetCurrentProcess(), &count) != FALSE,
        "GetProcessHandleCount");
    return count;
}

// 接口传输回环服务按路径返回正常、超限或延迟响应，不访问任何外部环境。
class MockHttpServer
{
public:
    MockHttpServer()
    {
        WSADATA data{};
        Check(::WSAStartup(MAKEWORD(2, 2), &data) == 0, "WSAStartup");
        listener_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        Check(listener_ != INVALID_SOCKET, "socket");
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;
        Check(::bind(listener_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0,
            "bind");
        Check(::listen(listener_, SOMAXCONN) == 0, "listen");
        int length = sizeof(address);
        Check(::getsockname(listener_, reinterpret_cast<sockaddr*>(&address), &length) == 0,
            "getsockname");
        port_ = ntohs(address.sin_port);
        worker_ = std::thread(&MockHttpServer::Serve, this);
    }

    ~MockHttpServer()
    {
        stopping_.store(true);
        if (listener_ != INVALID_SOCKET) ::closesocket(listener_);
        if (worker_.joinable()) worker_.join();
        ::WSACleanup();
    }

    unsigned short Port() const { return port_; }

private:
    // 每条连接只处理一个请求，符合接口测试当前“单次执行、确定性关闭”的语义。
    void Serve()
    {
        while (!stopping_.load())
        {
            SOCKET client = ::accept(listener_, nullptr, nullptr);
            if (client == INVALID_SOCKET) return;
            std::string request;
            char buffer[2048]{};
            while (request.find("\r\n\r\n") == std::string::npos)
            {
                const int received = ::recv(client, buffer, sizeof(buffer), 0);
                if (received <= 0) break;
                request.append(buffer, static_cast<size_t>(received));
            }
            if (request.find(" /slow ") != std::string::npos)
                std::this_thread::sleep_for(std::chrono::seconds(2));
            const std::string body = request.find(" /large ") != std::string::npos ?
                std::string(4096, 'x') : "{\"Code\":0}";
            const std::string response =
                "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: " +
                std::to_string(body.size()) + "\r\n\r\n" + body;
            ::send(client, response.data(), static_cast<int>(response.size()), 0);
            ::shutdown(client, SD_BOTH);
            ::closesocket(client);
        }
    }

    SOCKET listener_ = INVALID_SOCKET;       // 析构关闭以解除 accept。
    unsigned short port_ = 0;                // 系统分配的回环端口。
    std::atomic<bool> stopping_{false};       // 服务线程协作停止标记。
    std::thread worker_;                      // 唯一服务线程。
};

// 构造一条使用回环服务的 HTTP 请求；调用方只覆盖路径和响应上限。
interface_test::HttpRequest MakeRequest(unsigned short port, const char* path,
    unsigned int maxResponseBytes = 8U * 1024U * 1024U)
{
    interface_test::HttpRequest request;
    request.server.id = "mock";
    request.server.name = "mock";
    request.server.transport = interface_test::TransportKind::Http;
    request.server.scheme = "http";
    request.server.host = "127.0.0.1";
    request.server.port = port;
    request.server.hasExplicitPort = true;
    request.server.connectTimeoutMs = 2000;
    request.server.requestTimeoutMs = 5000;
    request.server.maxResponseBytes = maxResponseBytes;
    request.method = "GET";
    request.path = path;
    return request;
}
}

// 参数一必须是仓库内 interface-test.xml；测试不修改原文件，只验证临时副本原子保存。
int wmain(int argc, wchar_t* argv[])
{
    try
    {
        Check(argc == 2, "usage: MtLoadTestTests <interface-test.xml>");
        const std::filesystem::path configPath(argv[1]);
        interface_test::InterfaceTestConfig config;
        std::string error;
        Check(config.Load(configPath.u8string(), error), error.c_str());
        Check(config.environments.size() == 2, "environment count");
        Check(config.servers.size() == 7, "server count");
        Check(config.cases.size() >= 36, "published interface case coverage");
        for (const auto& server : config.servers)
            Check(server.maxResponseBytes == 8U * 1024U * 1024U,
                "default maxResponseBytes");

        const std::filesystem::path saved = std::filesystem::temp_directory_path() /
            "MtLoadTest-interface-roundtrip.xml";
        Check(config.SaveAtomic(saved.u8string(), error), error.c_str());
        interface_test::InterfaceTestConfig roundTrip;
        Check(roundTrip.Load(saved.u8string(), error), error.c_str());
        Check(roundTrip.cases.size() == config.cases.size(), "config round trip");
        std::error_code removeError;
        std::filesystem::remove(saved, removeError);

        MockHttpServer server;
        interface_test::SocketHttpTransport transport;
        auto response = transport.Execute(MakeRequest(server.Port(), "/ok"));
        Check(response.transportSucceeded && response.statusCode == 200 &&
            response.responseBody == "{\"Code\":0}", "HTTP response");

        const DWORD handlesBefore = CurrentHandleCount();
        for (int index = 0; index < 32; ++index)
        {
            response = transport.Execute(MakeRequest(server.Port(), "/ok"));
            Check(response.transportSucceeded, "repeated HTTP response");
        }
        Check(CurrentHandleCount() <= handlesBefore + 4, "HTTP handle release");

        response = transport.Execute(MakeRequest(server.Port(), "/large", 64));
        Check(!response.transportSucceeded &&
            response.error.find("maxResponseBytes") != std::string::npos,
            "response limit");

        auto pending = std::async(std::launch::async, [&transport, &server]() {
            return transport.Execute(MakeRequest(server.Port(), "/slow"));
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        transport.Cancel();
        Check(pending.wait_for(std::chrono::seconds(1)) == std::future_status::ready,
            "cancel completion");
        Check(!pending.get().transportSucceeded, "cancel result");

        std::cout << "MtLoadTest interface config/transport: PASS\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "FAIL: " << exception.what() << '\n';
        return 1;
    }
}
