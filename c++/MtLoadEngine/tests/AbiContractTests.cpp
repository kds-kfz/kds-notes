// 最小 ABI 与生命周期回归测试，不访问真实网络。

#include <MtLoadEngine.h>

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <Windows.h>

#include <atomic>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace
{
// 测试断言统一入口；失败立即终止，避免后续检查建立在无效句柄上。
void Check(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

// 读取引擎最近错误的 UTF-8 文本，供外部计划校验失败时输出可定位原因。
std::string LastError(MTLOAD_HANDLE handle)
{
    uint32_t bytes = 0;
    MtLoad_GetLastError(handle, nullptr, &bytes);
    if (bytes == 0) return {};
    std::vector<char> buffer(bytes, '\0');
    if (MtLoad_GetLastError(handle, buffer.data(), &bytes) != MTLOAD_OK) return {};
    return std::string(buffer.data());
}

// 读取当前进程句柄数，用于验证重复并发请求结束后没有遗留线程、事件或套接字句柄。
DWORD CurrentHandleCount()
{
    DWORD count = 0;
    Check(::GetProcessHandleCount(::GetCurrentProcess(), &count) != FALSE,
        "GetProcessHandleCount");
    return count;
}

// 网络组件停止后允许系统完成异步句柄回收，但只在固定时限内等待，避免掩盖真实泄漏。
bool WaitForHandleCount(DWORD maximum, DWORD timeoutMs, DWORD& finalCount)
{
    const auto deadline = std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeoutMs);
    do
    {
        finalCount = CurrentHandleCount();
        if (finalCount <= maximum) return true;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    } while (std::chrono::steady_clock::now() < deadline);
    finalCount = CurrentHandleCount();
    return finalCount <= maximum;
}

// 本地 HTTP Mock 验证真实 SocketClient 收发、步骤屏障和 Token 提取，不访问外部环境。
class MockHttpServer
{
public:
    // 绑定回环随机端口并启动唯一服务线程；构造失败直接使测试失败。
    explicit MockHttpServer(uint32_t responseDelayMs = 0) : responseDelayMs_(responseDelayMs)
    {
        WSADATA data{};
        Check(WSAStartup(MAKEWORD(2, 2), &data) == 0, "WSAStartup");
        listener_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        Check(listener_ != INVALID_SOCKET, "socket");
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = 0;
        Check(bind(listener_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0, "bind");
        Check(listen(listener_, SOMAXCONN) == 0, "listen");
        int length = sizeof(address);
        Check(getsockname(listener_, reinterpret_cast<sockaddr*>(&address), &length) == 0, "getsockname");
        port_ = ntohs(address.sin_port);
        worker_ = std::thread([this]() { Serve(); });
    }

    // 先关闭监听套接字解除 accept，再联结线程并成对释放 Winsock。
    ~MockHttpServer()
    {
        stopping_.store(true);
        if (listener_ != INVALID_SOCKET) closesocket(listener_);
        if (worker_.joinable()) worker_.join();
        WSACleanup();
    }

    // 返回系统分配的回环端口，构造完成后保持不变。
    unsigned short Port() const { return port_; }
    // 原子读取服务端累计接受的请求数。
    uint64_t RequestCount() const { return requestCount_.load(); }
    // 在请求集合锁下统计包含指定文本的原始请求数量。
    uint64_t RequestCountContaining(const std::string& text) const
    {
        std::lock_guard<std::mutex> guard(requestsMutex_);
        return static_cast<uint64_t>(std::count_if(requests_.begin(), requests_.end(),
            [&](const std::string& request) { return request.find(text) != std::string::npos; }));
    }

private:
    // 串行接受请求、记录原文并返回确定 JSON，便于断言执行顺序和变量提取。
    void Serve()
    {
        while (!stopping_.load())
        {
            SOCKET client = accept(listener_, nullptr, nullptr);
            if (client == INVALID_SOCKET) return;
            requestCount_.fetch_add(1);
            std::string request;
            char buffer[2048];
            while (request.find("\r\n\r\n") == std::string::npos)
            {
                const int received = recv(client, buffer, sizeof(buffer), 0);
                if (received <= 0) break;
                request.append(buffer, received);
            }
            {
                std::lock_guard<std::mutex> guard(requestsMutex_);
                requests_.push_back(request);
            }
            if (responseDelayMs_ > 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(responseDelayMs_));
            std::string body = "{\"Code\":0}";
            if (request.find(" /login ") != std::string::npos)
            {
                body = "{\"Code\":0,\"Data\":{\"Token\":\"abc\"}}";
            }
            else if (request.find(" /fail ") != std::string::npos)
            {
                body = "{\"Code\":1}";
            }
            else
            {
                // /assert/{id} 把路径中的并发隔离标识回显到数组，验证数组 JSONPath 和断言变量展开。
                const std::string prefix = " /assert/";
                const size_t begin = request.find(prefix);
                if (begin != std::string::npos)
                {
                    const size_t valueBegin = begin + prefix.size();
                    const size_t valueEnd = request.find(' ', valueBegin);
                    const std::string id = request.substr(valueBegin, valueEnd - valueBegin);
                    body = "{\"Code\":0,\"Data\":{\"Items\":[{\"Id\":\"" + id + "\"}]}}";
                }
            }
            const std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: " +
                std::to_string(body.size()) + "\r\n\r\n" + body;
            send(client, response.data(), static_cast<int>(response.size()), 0);
            shutdown(client, SD_BOTH);
            closesocket(client);
        }
    }

    SOCKET listener_ = INVALID_SOCKET;              // 回环监听套接字，由析构函数关闭。
    unsigned short port_ = 0;                       // 系统分配的监听端口。
    uint32_t responseDelayMs_ = 0;                  // 每个响应前的人工延迟，单位：毫秒。
    std::atomic<bool> stopping_{ false };            // 服务线程协作停止标志。
    std::atomic<uint64_t> requestCount_{ 0 };        // 已接受连接的累计数量。
    mutable std::mutex requestsMutex_;               // 保护 requests_ 的读写。
    std::vector<std::string> requests_;              // 测试期间接收的原始 HTTP 请求。
    std::thread worker_;                             // 唯一 Mock 服务线程。
};
}

// 关键实现：无参数时运行完整 Mock；传入一个计划路径时只执行解析和语义校验，不访问目标服务。
int wmain(int argc, wchar_t* argv[])
{
    try
    {
        Check(MtLoad_GetAbiVersion() == MTLOAD_ABI_VERSION, "ABI version");
        MTLOAD_ENGINE_OPTIONS options{};
        options.structSize = sizeof(options);
        Check(MtLoad_InitOptions(&options) == MTLOAD_OK, "InitOptions");
        MTLOAD_HANDLE handle = nullptr;
        Check(MtLoad_Create(&options, &handle) == MTLOAD_OK && handle != nullptr, "Create");
        if (argc == 2)
        {
            // Windows 宽字符入口先转换为 UTF-8，避免中文目录经过当前系统代码页后损坏。
            const std::string externalPath = std::filesystem::path(argv[1]).u8string();
            if (MtLoad_LoadPlan(handle, externalPath.c_str()) != MTLOAD_OK)
                throw std::runtime_error("external plan load: " + LastError(handle));
            if (MtLoad_ValidatePlan(handle) != MTLOAD_OK)
                throw std::runtime_error("external plan validate: " + LastError(handle));
            uint32_t externalScenarioCount = 0;
            Check(MtLoad_GetScenarios(handle, nullptr, &externalScenarioCount) ==
                MTLOAD_E_BUFFER_TOO_SMALL && externalScenarioCount > 0,
                "external plan scenarios");
            Check(MtLoad_Destroy(handle) == MTLOAD_OK, "external plan destroy");
            std::cout << "MtLoadEngine plan validation: PASS, scenarios=" <<
                externalScenarioCount << '\n';
            return 0;
        }
        Check(MtLoad_Start(handle, "bad-order") == MTLOAD_E_INVALID_STATE, "start order");

        const auto root = std::filesystem::temp_directory_path() / "MtLoadEngineTests";
        std::filesystem::create_directories(root);
        const auto xml = root / "plan.xml";
        std::ofstream(xml) <<
            "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
            "<MtLoadPlan version=\"1\" name=\"contract\">"
            "<Targets><Target id=\"local\" transport=\"http\" host=\"127.0.0.1\" port=\"1\"/></Targets>"
            "<DataSets><Inline id=\"accounts\"><Row Login=\"100\" Password=\"test\"/></Inline></DataSets>"
            "<Scenarios><Scenario id=\"s\" dataSet=\"accounts\"><Load mode=\"closed\" users=\"1\" iterations=\"1\"/>"
            "<Step id=\"one\"><Request id=\"r\" label=\"r\" target=\"local\" method=\"GET\" path=\"/\" codec=\"json\">"
            "<Assertions><HttpStatus min=\"200\" max=\"299\"/></Assertions></Request></Step>"
            "</Scenario></Scenarios></MtLoadPlan>";
        Check(MtLoad_LoadPlan(handle, xml.u8string().c_str()) == MTLOAD_OK, "LoadPlan");
        Check(MtLoad_ValidatePlan(handle) == MTLOAD_OK, "ValidatePlan");
        MTLOAD_RUN_STATE state{};
        state.structSize = sizeof(state);
        state.abiVersion = MTLOAD_ABI_VERSION;
        Check(MtLoad_GetRunState(handle, &state) == MTLOAD_OK &&
            state.state == MTLOAD_STATE_VALIDATED, "validated state");
        Check(MtLoad_Destroy(handle) == MTLOAD_OK, "Destroy");

        MockHttpServer server(75);
        options.structSize = sizeof(options);
        Check(MtLoad_InitOptions(&options) == MTLOAD_OK, "runtime options");
        Check(MtLoad_Create(&options, &handle) == MTLOAD_OK, "runtime create");
        const auto runtimeXml = root / "runtime.xml";
        std::ofstream(runtimeXml) <<
            "<MtLoadPlan version=\"1\" name=\"runtime\"><Targets>"
            "<Target id=\"local\" transport=\"http\" host=\"127.0.0.1\" port=\"" << server.Port() << "\" requestTimeoutMs=\"3000\"/>"
            "</Targets><Scenarios><Scenario id=\"s\"><Load mode=\"closed\" users=\"1\" iterations=\"1\"/>"
            "<Step id=\"login\"><Request id=\"login\" label=\"login\" target=\"local\" method=\"POST\" path=\"/login\" codec=\"json\">"
            "<Body>{}</Body><Assertions><Json path=\"$.Code\" operator=\"eq\" value=\"0\"/></Assertions>"
            "<Extractors><Json path=\"$.Data.Token\" variable=\"Token\" scope=\"session\" required=\"true\"/></Extractors></Request></Step>"
            "<Step id=\"next\"><Request id=\"next\" label=\"next\" target=\"local\" method=\"POST\" path=\"/next\" codec=\"json\">"
            "<Headers><Header name=\"Token\" value=\"${Token}\"/></Headers><Body>{}</Body>"
            "<Assertions><Json path=\"$.Code\" operator=\"eq\" value=\"0\"/></Assertions></Request></Step>"
            "</Scenario></Scenarios></MtLoadPlan>";
        Check(MtLoad_LoadPlan(handle, runtimeXml.u8string().c_str()) == MTLOAD_OK, "runtime load");
        Check(MtLoad_ValidatePlan(handle) == MTLOAD_OK, "runtime validate");
        Check(MtLoad_Start(handle, "mock") == MTLOAD_OK, "runtime start");
        bool firstRunCompleted = false;
        for (int retry = 0; retry < 100; ++retry)
        {
            state = {};
            state.structSize = sizeof(state);
            state.abiVersion = MTLOAD_ABI_VERSION;
            Check(MtLoad_GetRunState(handle, &state) == MTLOAD_OK, "runtime state");
            if (state.state == MTLOAD_STATE_COMPLETED)
            {
                firstRunCompleted = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        Check(firstRunCompleted, "first runtime completion");

        // 模拟 MFC 只轮询终态、不调用 Wait：重新加载计划必须回收上一轮线程。
        Check(MtLoad_LoadPlan(handle, runtimeXml.u8string().c_str()) == MTLOAD_OK, "runtime reload");
        Check(MtLoad_ValidatePlan(handle) == MTLOAD_OK, "runtime revalidate");
        Check(MtLoad_Start(handle, "mock-repeat") == MTLOAD_OK, "runtime restart");
        Check(MtLoad_Wait(handle, 10000) == MTLOAD_OK, "runtime repeat wait");
        state = {};
        state.structSize = sizeof(state);
        state.abiVersion = MTLOAD_ABI_VERSION;
        Check(MtLoad_GetRunState(handle, &state) == MTLOAD_OK && state.runId == 2 &&
            state.totalSamples == 2, "runtime repeat state");
        uint32_t metricCount = 0;
        Check(MtLoad_GetMetricRows(handle, nullptr, &metricCount) == MTLOAD_E_BUFFER_TOO_SMALL &&
            metricCount == 2, "runtime metric count");
        std::vector<MTLOAD_METRIC_ROW> metrics(metricCount);
        Check(MtLoad_GetMetricRows(handle, metrics.data(), &metricCount) == MTLOAD_OK, "runtime metrics");
        Check(metrics[0].succeeded + metrics[1].succeeded == 2, "runtime successes");

        Check(MtLoad_ClearResults(handle) == MTLOAD_OK, "clear results");
        state = {};
        state.structSize = sizeof(state);
        state.abiVersion = MTLOAD_ABI_VERSION;
        Check(MtLoad_GetRunState(handle, &state) == MTLOAD_OK && state.totalSamples == 0,
            "cleared state");
        metricCount = 0;
        Check(MtLoad_GetMetricRows(handle, nullptr, &metricCount) == MTLOAD_OK && metricCount == 0,
            "cleared metrics");

        // 终态直接重新开始；暂停时仅让在途请求完成，继续后执行尚未开始的步骤。
        Check(MtLoad_Start(handle, "pause-resume") == MTLOAD_OK, "direct restart");
        bool requestStarted = false;
        for (int retry = 0; retry < 100; ++retry)
        {
            state = {};
            state.structSize = sizeof(state);
            state.abiVersion = MTLOAD_ABI_VERSION;
            Check(MtLoad_GetRunState(handle, &state) == MTLOAD_OK, "pause start state");
            if (state.activeRequests > 0) { requestStarted = true; break; }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        Check(requestStarted, "request started before pause");
        Check(MtLoad_Pause(handle) == MTLOAD_OK, "pause");
        bool paused = false;
        for (int retry = 0; retry < 100; ++retry)
        {
            state = {};
            state.structSize = sizeof(state);
            state.abiVersion = MTLOAD_ABI_VERSION;
            Check(MtLoad_GetRunState(handle, &state) == MTLOAD_OK, "paused state");
            if (state.state == MTLOAD_STATE_PAUSED) { paused = true; break; }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        Check(paused, "paused after inflight request");
        const uint64_t pausedSamples = state.totalSamples;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        Check(MtLoad_GetRunState(handle, &state) == MTLOAD_OK &&
            state.state == MTLOAD_STATE_PAUSED && state.totalSamples == pausedSamples,
            "paused sample stability");
        Check(MtLoad_Resume(handle) == MTLOAD_OK, "resume");
        Check(MtLoad_Wait(handle, 10000) == MTLOAD_OK, "pause run wait");
        Check(MtLoad_GetRunState(handle, &state) == MTLOAD_OK && state.runId == 3 &&
            state.totalSamples == 2, "resumed completion");

        // Stop 会打断 HTTP 等待；再次 Start 使用同一已校验计划并从第一步开始。
        Check(MtLoad_Start(handle, "stop") == MTLOAD_OK, "stop run start");
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        Check(MtLoad_Stop(handle) == MTLOAD_OK, "stop request");
        Check(MtLoad_Wait(handle, 2000) == MTLOAD_OK, "stop wait");
        Check(MtLoad_Start(handle, "restart-after-stop") == MTLOAD_OK, "restart after stop");
        Check(MtLoad_Wait(handle, 10000) == MTLOAD_OK, "restart after stop wait");
        Check(MtLoad_GetRunState(handle, &state) == MTLOAD_OK && state.runId == 5 &&
            state.totalSamples == 2, "restart from beginning");
        Check(MtLoad_Destroy(handle) == MTLOAD_OK, "runtime destroy");

        // threads 表示并发虚拟终端，允许多个线程使用同一账号；每个线程独立循环 iterations 次。
        options.structSize = sizeof(options);
        Check(MtLoad_InitOptions(&options) == MTLOAD_OK, "thread options");
        Check(MtLoad_Create(&options, &handle) == MTLOAD_OK, "thread create");
        const auto threadsXml = root / "threads.xml";
        std::ofstream(threadsXml) <<
            "<MtLoadPlan version=\"1\" name=\"threads\">"
            "<Environments default=\"dev\">"
            "<Environment id=\"dev\" name=\"dev\" scheme=\"https\" host=\"example.test\"/>"
            "<Environment id=\"test\" name=\"test\" scheme=\"http\" host=\"127.0.0.1\" port=\"" << server.Port() << "\" basePath=\"/api\"/>"
            "</Environments><Users>"
            "<User id=\"u1\" name=\"测试用户一\" version=\"5\" no=\"1\" login=\"1001\" password=\"p1\"><Param name=\"PwdType\" value=\"0\"/></User>"
            "<User id=\"u2\" name=\"测试用户二\" version=\"4\" no=\"2\" login=\"1002\" password=\"\"><Param name=\"PwdType\" value=\"1\"/></User>"
            "<User id=\"u3\" name=\"空字段用户\"/>"
            "</Users><Targets><Target id=\"api\" transport=\"http\" requestTimeoutMs=\"3000\"/></Targets>"
            "<Scenarios><Scenario id=\"threaded-a\" name=\"中文场景一\"><UserRefs><UserRef id=\"u1\"/><UserRef id=\"u2\"/></UserRefs>"
            "<Load mode=\"closed\" threads=\"3\" iterations=\"2\"/>"
            "<Step id=\"request\"><Request id=\"request\" label=\"request\" target=\"api\" method=\"POST\" path=\"/thread/${Login}\" codec=\"json\">"
            "<Body>{\"Version\":${Version},\"No\":${No},\"Login\":${Login},\"Password\":\"${Password}\",\"PwdType\":${PwdType}}</Body>"
            "<Assertions><Json path=\"$.Code\" operator=\"eq\" value=\"0\"/></Assertions>"
            "</Request></Step></Scenario>"
            "<Scenario id=\"threaded-b\" name=\"中文场景二\"><UserRefs><UserRef id=\"u2\"/></UserRefs>"
            "<Load mode=\"closed\" threads=\"2\" iterations=\"3\"/>"
            "<Step id=\"request\"><Request id=\"request\" label=\"request\" target=\"api\" method=\"POST\" path=\"/repeat/${Login}\" codec=\"json\">"
            "<Body>{\"Version\":${Version},\"No\":${No},\"Login\":${Login}}</Body>"
            "<Assertions><Json path=\"$.Code\" operator=\"eq\" value=\"0\"/></Assertions>"
            "</Request></Step></Scenario></Scenarios></MtLoadPlan>";
        Check(MtLoad_LoadPlan(handle, threadsXml.u8string().c_str()) == MTLOAD_OK,
            "thread plan load");
        Check(MtLoad_ValidatePlan(handle) == MTLOAD_OK, "thread plan validate");
        uint32_t environmentCount = 0;
        Check(MtLoad_GetEnvironments(handle, nullptr, &environmentCount) == MTLOAD_E_BUFFER_TOO_SMALL &&
            environmentCount == 2, "environment count");
        std::vector<MTLOAD_ENVIRONMENT> environments(environmentCount);
        Check(MtLoad_GetEnvironments(handle, environments.data(), &environmentCount) == MTLOAD_OK,
            "environment rows");
        Check(environments[0].selected + environments[1].selected == 1,
            "default environment");
        bool foundHttpsEnvironment = false;
        for (const auto& environment : environments)
            if (std::string(environment.id) == "dev")
                foundHttpsEnvironment = std::string(environment.scheme) == "https" && environment.port == 0;
        Check(foundHttpsEnvironment, "HTTPS environment keeps omitted port for protocol default");

        uint32_t userCount = 0;
        Check(MtLoad_GetUsers(handle, nullptr, &userCount) == MTLOAD_E_BUFFER_TOO_SMALL &&
            userCount == 3, "user count");
        std::vector<MTLOAD_USER> users(userCount);
        Check(MtLoad_GetUsers(handle, users.data(), &userCount) == MTLOAD_OK,
            "user rows");
        Check(std::string(users[0].name).find(u8"测试用户") != std::string::npos &&
            std::string(users[1].login) == "1002" && users[0].selected + users[1].selected == 0,
            "Chinese user names and safe summaries");
        Check(std::string(users[2].version).empty() && std::string(users[2].no).empty() &&
            std::string(users[2].login).empty(), "empty business fields remain selectable values");
        Check(MtLoad_SelectUser(handle, "missing") == MTLOAD_E_NOT_FOUND,
            "missing user rejected");
        Check(MtLoad_SelectUser(handle, "u2") == MTLOAD_OK, "select one user");

        uint32_t scenarioCount = 0;
        Check(MtLoad_GetScenarios(handle, nullptr, &scenarioCount) == MTLOAD_E_BUFFER_TOO_SMALL &&
            scenarioCount == 2, "scenario count");
        std::vector<MTLOAD_SCENARIO> scenarios(scenarioCount);
        Check(MtLoad_GetScenarios(handle, scenarios.data(), &scenarioCount) == MTLOAD_OK,
            "scenario rows");
        Check(std::string(scenarios[0].name).find(u8"中文场景") != std::string::npos &&
            scenarios[0].plannedRequests == 6 && scenarios[1].plannedRequests == 6,
            "Chinese scenario names and planned requests");
        Check(MtLoad_SelectEnvironment(handle, "test") == MTLOAD_OK, "select environment");
        Check(MtLoad_SelectScenario(handle, "threaded-a") == MTLOAD_OK, "select one scenario");
        state = {};
        state.structSize = sizeof(state);
        state.abiVersion = MTLOAD_ABI_VERSION;
        Check(MtLoad_GetRunState(handle, &state) == MTLOAD_OK && state.plannedRequests == 6,
            "selected scenario planned requests");
        const uint64_t requestsBefore = server.RequestCount();
        const uint64_t selectedUserRequestsBefore = server.RequestCountContaining("/api/thread/1002");
        const uint64_t emptyPasswordRequestsBefore =
            server.RequestCountContaining("\"Password\":\"\"");
        Check(MtLoad_Start(handle, "selected-scenario") == MTLOAD_OK, "selected scenario start");
        Check(MtLoad_Wait(handle, 10000) == MTLOAD_OK, "thread run wait");
        state = {};
        state.structSize = sizeof(state);
        state.abiVersion = MTLOAD_ABI_VERSION;
        Check(MtLoad_GetRunState(handle, &state) == MTLOAD_OK &&
            state.state == MTLOAD_STATE_COMPLETED && state.totalSamples == 6 &&
            state.plannedRequests == 6, "selected scenario completion");
        Check(server.RequestCount() == requestsBefore + 6,
            "selected scenario only");
        Check(server.RequestCountContaining("/api/thread/1002") == selectedUserRequestsBefore + 6,
            "selected user overrides scenario UserRefs for every virtual user");
        Check(server.RequestCountContaining("\"Password\":\"\"") ==
            emptyPasswordRequestsBefore + 6,
            "configured empty user value is sent as an empty JSON string");

        char reportPath[32768]{};
        Check(MtLoad_ExportConfiguredReport(handle, MTLOAD_REPORT_XML, reportPath,
            static_cast<uint32_t>(sizeof(reportPath))) == MTLOAD_OK,
            "configured XML export");
        const std::filesystem::path xmlReport = std::filesystem::u8path(reportPath);
        Check(std::filesystem::exists(xmlReport) && xmlReport.filename().u8string().find("LoadTest_") == 0,
            "timestamp XML report path");
        std::ifstream xmlStream(xmlReport, std::ios::binary);
        const std::string xmlText((std::istreambuf_iterator<char>(xmlStream)),
            std::istreambuf_iterator<char>());
        Check(xmlText.find(u8"指标列表") != std::string::npos &&
            xmlText.find(u8"中文场景一") != std::string::npos,
            "UTF-8 Chinese XML report");
        Check(MtLoad_ExportConfiguredReport(handle, MTLOAD_REPORT_XLSX, reportPath,
            static_cast<uint32_t>(sizeof(reportPath))) == MTLOAD_OK &&
            std::filesystem::exists(std::filesystem::u8path(reportPath)) &&
            std::filesystem::file_size(std::filesystem::u8path(reportPath)) > 0,
            "configured XLSX export");

        Check(MtLoad_SelectUser(handle, "") == MTLOAD_OK, "restore scenario UserRefs");
        Check(MtLoad_SelectScenario(handle, "") == MTLOAD_OK, "select all scenarios");
        const uint64_t allRequestsBefore = server.RequestCount();
        const uint64_t repeatedAccountBefore =
            server.RequestCountContaining("/api/repeat/1002");
        Check(MtLoad_Start(handle, "all-scenarios") == MTLOAD_OK, "all scenarios start");
        Check(MtLoad_Wait(handle, 10000) == MTLOAD_OK, "all scenarios wait");
        Check(MtLoad_GetRunState(handle, &state) == MTLOAD_OK && state.totalSamples == 12 &&
            state.plannedRequests == 12, "per-scenario threads and iterations");
        Check(server.RequestCount() == allRequestsBefore + 12,
            "sum of scenario threads multiplied by iterations");
        Check(server.RequestCountContaining("/api/repeat/1002") == repeatedAccountBefore + 6,
            "two virtual terminals may share one account and each run three iterations");
        Check(MtLoad_Destroy(handle) == MTLOAD_OK, "thread destroy");

        // 运行时变量按运行、线程和循环隔离；数组断言使用同一变量校验服务端回显值。
        options.structSize = sizeof(options);
        Check(MtLoad_InitOptions(&options) == MTLOAD_OK, "isolation options");
        Check(MtLoad_Create(&options, &handle) == MTLOAD_OK, "isolation create");
        const auto isolationXml = root / "runtime-isolation.xml";
        std::ofstream(isolationXml) <<
            "<MtLoadPlan version=\"1\" name=\"runtime-isolation\"><Targets>"
            "<Target id=\"local\" transport=\"http\" host=\"127.0.0.1\" port=\"" << server.Port() << "\" requestTimeoutMs=\"3000\"/>"
            "</Targets><Scenarios>"
            "<Scenario id=\"isolation\"><Load mode=\"closed\" threads=\"2\" iterations=\"2\"/>"
            "<Step id=\"assert\"><Request id=\"assert\" label=\"assert\" target=\"local\" method=\"POST\" "
            "path=\"/assert/loadtest-${runId}-${threadIndex}-${iterationIndex}\" codec=\"json\">"
            "<Body>{}</Body><Assertions><Json path=\"$.Code\" operator=\"eq\" value=\"0\"/>"
            "<Json path=\"$.Data.Items[0].Id\" operator=\"eq\" value=\"loadtest-${runId}-${threadIndex}-${iterationIndex}\"/>"
            "</Assertions></Request></Step></Scenario>"
            "<Scenario id=\"cleanup\"><Load mode=\"closed\" threads=\"1\" iterations=\"1\"/>"
            "<Step id=\"fail\" onError=\"stop_iteration\"><Request id=\"fail\" label=\"fail\" target=\"local\" method=\"POST\" path=\"/fail\" codec=\"json\">"
            "<Assertions><Json path=\"$.Code\" operator=\"eq\" value=\"0\"/></Assertions></Request></Step>"
            "<Step id=\"skipped\"><Request id=\"skipped\" label=\"skipped\" target=\"local\" method=\"POST\" path=\"/skipped\" codec=\"json\"/></Step>"
            "<Step id=\"cleanup\" alwaysRun=\"true\"><Request id=\"cleanup\" label=\"cleanup\" target=\"local\" method=\"POST\" path=\"/cleanup\" codec=\"json\">"
            "<Assertions><Json path=\"$.Code\" operator=\"eq\" value=\"0\"/></Assertions></Request></Step>"
            "</Scenario></Scenarios></MtLoadPlan>";
        Check(MtLoad_LoadPlan(handle, isolationXml.u8string().c_str()) == MTLOAD_OK,
            "isolation load");
        Check(MtLoad_ValidatePlan(handle) == MTLOAD_OK, "isolation validate");
        Check(MtLoad_SelectScenario(handle, "isolation") == MTLOAD_OK, "isolation select");
        const uint64_t isolationBefore = server.RequestCountContaining("/assert/loadtest-");
        Check(MtLoad_Start(handle, "runtime-isolation") == MTLOAD_OK, "isolation start");
        Check(MtLoad_Wait(handle, 10000) == MTLOAD_OK, "isolation wait");
        Check(MtLoad_GetRunState(handle, &state) == MTLOAD_OK && state.totalSamples == 4 &&
            state.plannedRequests == 4, "runtime variable request count");
        Check(server.RequestCountContaining("/assert/loadtest-") == isolationBefore + 4,
            "runtime variable unique requests");

        // stop_iteration 只停止普通业务步骤，alwaysRun 清理仍执行并纳入指标。
        Check(MtLoad_SelectScenario(handle, "cleanup") == MTLOAD_OK, "cleanup select");
        const uint64_t failBefore = server.RequestCountContaining(" /fail ");
        const uint64_t skippedBefore = server.RequestCountContaining(" /skipped ");
        const uint64_t cleanupBefore = server.RequestCountContaining(" /cleanup ");
        Check(MtLoad_Start(handle, "always-run-cleanup") == MTLOAD_OK, "cleanup start");
        Check(MtLoad_Wait(handle, 10000) == MTLOAD_OK, "cleanup wait");
        Check(MtLoad_GetRunState(handle, &state) == MTLOAD_OK && state.totalSamples == 2 &&
            state.plannedRequests == 3, "cleanup sample count");
        Check(server.RequestCountContaining(" /fail ") == failBefore + 1 &&
            server.RequestCountContaining(" /skipped ") == skippedBefore &&
            server.RequestCountContaining(" /cleanup ") == cleanupBefore + 1,
            "alwaysRun cleanup after failure");
        uint32_t cleanupTransactionCount = 0;
        Check(MtLoad_GetScenarioMetricRows(handle, nullptr, &cleanupTransactionCount) == MTLOAD_E_BUFFER_TOO_SMALL &&
            cleanupTransactionCount == 1, "cleanup transaction count");
        std::vector<MTLOAD_SCENARIO_METRIC_ROW> cleanupTransactions(cleanupTransactionCount);
        Check(MtLoad_GetScenarioMetricRows(handle, cleanupTransactions.data(), &cleanupTransactionCount) == MTLOAD_OK &&
            cleanupTransactions[0].completed == 1 && cleanupTransactions[0].succeeded == 0 &&
            cleanupTransactions[0].failed == 1, "cleanup failure belongs to complete failed transaction");
        Check(MtLoad_Destroy(handle) == MTLOAD_OK, "isolation destroy");

        // 以 16 个并发虚拟终端执行 20 轮回环请求，覆盖高频创建、关闭和句柄释放。
        MockHttpServer stressServer;
        const auto stressXml = root / "resource-stress.xml";
        std::ofstream(stressXml) <<
            "<MtLoadPlan version=\"1\" name=\"resource-stress\"><Targets>"
            "<Target id=\"local\" transport=\"http\" host=\"127.0.0.1\" port=\"" <<
            stressServer.Port() << "\" connectTimeoutMs=\"3000\" requestTimeoutMs=\"3000\"/>"
            "</Targets><Scenarios><Scenario id=\"stress\"><Load mode=\"closed\" threads=\"16\" iterations=\"20\"/>"
            "<Step id=\"request\"><Request id=\"request\" label=\"request\" target=\"local\" method=\"GET\" path=\"/stress\" codec=\"json\">"
            "<Assertions><HttpStatus min=\"200\" max=\"299\"/><Json path=\"$.Code\" operator=\"eq\" value=\"0\"/></Assertions>"
            "</Request></Step></Scenario></Scenarios></MtLoadPlan>";

        // 首轮允许网络栈建立进程级惰性资源；第二个完整周期不得继续累计句柄。
        auto runStressCycle = [&]()
        {
            options = {};
            options.structSize = sizeof(options);
            Check(MtLoad_InitOptions(&options) == MTLOAD_OK, "stress options");
            handle = nullptr;
            Check(MtLoad_Create(&options, &handle) == MTLOAD_OK, "stress create");
            Check(MtLoad_LoadPlan(handle, stressXml.u8string().c_str()) == MTLOAD_OK,
                "stress load");
            Check(MtLoad_ValidatePlan(handle) == MTLOAD_OK, "stress validate");
            Check(MtLoad_Start(handle, "resource-stress") == MTLOAD_OK, "stress start");
            Check(MtLoad_Wait(handle, 30000) == MTLOAD_OK, "stress wait");
            state = {};
            state.structSize = sizeof(state);
            state.abiVersion = MTLOAD_ABI_VERSION;
            Check(MtLoad_GetRunState(handle, &state) == MTLOAD_OK &&
                state.totalSamples == 320, "stress samples");
            metricCount = 0;
            Check(MtLoad_GetMetricRows(handle, nullptr, &metricCount) == MTLOAD_E_BUFFER_TOO_SMALL &&
                metricCount == 1, "stress metric count");
            metrics.assign(metricCount, {});
            Check(MtLoad_GetMetricRows(handle, metrics.data(), &metricCount) == MTLOAD_OK &&
                metrics[0].samples == 320 && metrics[0].succeeded == 320 && metrics[0].failed == 0,
                "stress metric successes");
            Check(MtLoad_Destroy(handle) == MTLOAD_OK, "stress destroy");
        };
        runStressCycle();
        const DWORD handlesAfterWarmup = CurrentHandleCount();
        for (int cycle = 1; cycle < 10; ++cycle) runStressCycle();
        Check(stressServer.RequestCount() == 3200, "ten stress cycles request count");
        DWORD handlesAfterStress = 0;
        if (!WaitForHandleCount(handlesAfterWarmup + 8, 3000, handlesAfterStress))
        {
            std::cerr << "stress handles: warmup=" << handlesAfterWarmup <<
                ", second=" << handlesAfterStress << '\n';
            Check(false, "stress handle release");
        }

        // V4 工作负载覆盖确定性权重、阶段到达率、最大用户丢弃和阈值终态。
        MockHttpServer arrivalFast;
        MockHttpServer arrivalSlow(80);
        const auto workloadXml = root / "workloads-v4.xml";
        std::ofstream(workloadXml) <<
            "<MtLoadPlan version=\"1\" name=\"workloads-v4\"><Targets>"
            "<Target id=\"fast\" transport=\"http\" host=\"127.0.0.1\" port=\"" << arrivalFast.Port() << "\"/>"
            "<Target id=\"slow\" transport=\"http\" host=\"127.0.0.1\" port=\"" << arrivalSlow.Port() << "\"/>"
            "</Targets><Scenarios>"
            "<Scenario id=\"a\"><Load mode=\"closed\" users=\"1\" iterations=\"1\"/><Step id=\"one\"><Request id=\"a\" label=\"a\" target=\"fast\" method=\"GET\" path=\"/a\" codec=\"json\"><Assertions><Json path=\"$.Code\" operator=\"eq\" value=\"0\"/></Assertions></Request></Step></Scenario>"
            "<Scenario id=\"b\"><Load mode=\"closed\" users=\"1\" iterations=\"1\"/><Step id=\"one\"><Request id=\"b\" label=\"b\" target=\"fast\" method=\"GET\" path=\"/b\" codec=\"json\"><Assertions><Json path=\"$.Code\" operator=\"eq\" value=\"0\"/></Assertions></Request></Step></Scenario>"
            "<Scenario id=\"slow\"><Load mode=\"closed\" users=\"1\" iterations=\"1\"/><Step id=\"one\"><Request id=\"slow\" label=\"slow\" target=\"slow\" method=\"GET\" path=\"/slow\" codec=\"json\"><Assertions><Json path=\"$.Code\" operator=\"eq\" value=\"0\"/></Assertions></Request></Step></Scenario>"
            "</Scenarios><Workloads default=\"weighted\">"
            "<Workload id=\"weighted\" name=\"weighted\" type=\"average\"><Load mode=\"closed\" users=\"1\" iterations=\"30\"/><ScenarioRef id=\"a\" weight=\"2\"/><ScenarioRef id=\"b\" weight=\"1\"/><Thresholds><Threshold id=\"errors\" scope=\"run\" metric=\"errorRate\" operator=\"eq\" value=\"0\"/></Thresholds></Workload>"
            "<Workload id=\"staged\" name=\"staged\" type=\"stress\"><Load mode=\"arrival\" ratePerSec=\"10\" preAllocatedUsers=\"2\" maxUsers=\"8\"><Stage durationMs=\"500\" targetRatePerSec=\"20\"/><Stage durationMs=\"500\" targetRatePerSec=\"20\"/></Load><ScenarioRef id=\"a\" weight=\"1\"/></Workload>"
            "<Workload id=\"drop\" name=\"drop\" type=\"spike\"><Load mode=\"arrival\" ratePerSec=\"100\" durationMs=\"400\" preAllocatedUsers=\"1\" maxUsers=\"1\"/><ScenarioRef id=\"slow\" weight=\"1\"/><Thresholds><Threshold id=\"no-drop\" scope=\"run\" metric=\"droppedIterations\" operator=\"eq\" value=\"0\"/></Thresholds></Workload>"
            "</Workloads></MtLoadPlan>";
        options = {};
        options.structSize = sizeof(options);
        Check(MtLoad_InitOptions(&options) == MTLOAD_OK, "V4 options");
        handle = nullptr;
        Check(MtLoad_Create(&options, &handle) == MTLOAD_OK, "V4 create");
        Check(MtLoad_LoadPlan(handle, workloadXml.u8string().c_str()) == MTLOAD_OK, "V4 load");
        Check(MtLoad_ValidatePlan(handle) == MTLOAD_OK, "V4 validate");
        uint32_t workloadCount = 0;
        Check(MtLoad_GetWorkloads(handle, nullptr, &workloadCount) == MTLOAD_E_BUFFER_TOO_SMALL &&
            workloadCount == 7, "explicit and compatibility workloads");
        std::vector<MTLOAD_WORKLOAD> workloads(workloadCount);
        Check(MtLoad_GetWorkloads(handle, workloads.data(), &workloadCount) == MTLOAD_OK,
            "V4 workload rows");
        Check(MtLoad_SelectWorkload(handle, "weighted") == MTLOAD_OK, "weighted select");
        Check(MtLoad_Start(handle, "weighted") == MTLOAD_OK && MtLoad_Wait(handle, 10000) == MTLOAD_OK,
            "weighted run");
        state = {};
        state.structSize = sizeof(state);
        state.abiVersion = MTLOAD_ABI_VERSION;
        Check(MtLoad_GetRunState(handle, &state) == MTLOAD_OK && state.totalSamples == 30 &&
            state.scheduledIterations == 30 && state.completedIterations == 30 &&
            state.droppedIterations == 0, "weighted run state");
        metricCount = 0;
        Check(MtLoad_GetMetricRows(handle, nullptr, &metricCount) == MTLOAD_E_BUFFER_TOO_SMALL &&
            metricCount == 2, "weighted request metric count");
        metrics.assign(metricCount, {});
        Check(MtLoad_GetMetricRows(handle, metrics.data(), &metricCount) == MTLOAD_OK,
            "weighted request metrics");
        double requestThroughputSum = 0.0;
        for (const auto& row : metrics) requestThroughputSum += row.throughputPerSec;
        Check(std::abs(requestThroughputSum - state.totalThroughputPerSec) < 0.001,
            "interface throughput sum equals total throughput");
        uint32_t transactionCount = 0;
        Check(MtLoad_GetScenarioMetricRows(handle, nullptr, &transactionCount) == MTLOAD_E_BUFFER_TOO_SMALL &&
            transactionCount == 2, "transaction count");
        std::vector<MTLOAD_SCENARIO_METRIC_ROW> transactions(transactionCount);
        Check(MtLoad_GetScenarioMetricRows(handle, transactions.data(), &transactionCount) == MTLOAD_OK,
            "transaction rows");
        uint64_t aCompleted = 0;
        uint64_t bCompleted = 0;
        for (const auto& row : transactions)
        {
            if (std::string(row.scenarioId) == "a") aCompleted = row.completed;
            if (std::string(row.scenarioId) == "b") bCompleted = row.completed;
        }
        Check(aCompleted == 20 && bCompleted == 10, "deterministic 2:1 workload weights");

        Check(MtLoad_SelectWorkload(handle, "staged") == MTLOAD_OK, "staged select");
        Check(MtLoad_Start(handle, "staged") == MTLOAD_OK, "staged start");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        Check(MtLoad_Pause(handle) == MTLOAD_OK, "staged pause");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        Check(MtLoad_Resume(handle) == MTLOAD_OK, "staged resume");
        Check(MtLoad_Wait(handle, 10000) == MTLOAD_OK, "staged wait");
        Check(MtLoad_GetRunState(handle, &state) == MTLOAD_OK &&
            state.scheduledIterations >= 15 && state.scheduledIterations <= 19 &&
            state.completedIterations == state.scheduledIterations && state.droppedIterations == 0 &&
            state.effectiveElapsedMs >= 900 && state.effectiveElapsedMs <= 1300,
            "staged arrival precision and active duration");

        // arrival 在运行中收到 Stop 后必须立即停止调度、排空取消记账并退出工作池；
        // 随后的工作负载还能复用同一引擎实例，防止 STOPPING 死锁或残留线程。
        Check(MtLoad_Start(handle, "staged-stop") == MTLOAD_OK, "arrival stop start");
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        Check(MtLoad_Stop(handle) == MTLOAD_OK, "arrival stop request");
        Check(MtLoad_Wait(handle, 3000) == MTLOAD_OK, "arrival stop converges");
        Check(MtLoad_GetRunState(handle, &state) == MTLOAD_OK &&
            state.state == MTLOAD_STATE_COMPLETED && state.activeUsers == 0 &&
            state.activeRequests == 0 && state.completedIterations + state.droppedIterations ==
            state.scheduledIterations, "arrival stop accounting and terminal state");

        Check(MtLoad_SelectWorkload(handle, "drop") == MTLOAD_OK, "drop select");
        Check(MtLoad_Start(handle, "drop") == MTLOAD_OK && MtLoad_Wait(handle, 10000) == MTLOAD_OK,
            "drop run");
        Check(MtLoad_GetRunState(handle, &state) == MTLOAD_OK && state.state == MTLOAD_STATE_COMPLETED &&
            state.lastResult == MTLOAD_E_THRESHOLD_FAILED && state.droppedIterations > 0 &&
            state.peakActiveUsers == 1 && state.completedIterations + state.droppedIterations ==
            state.scheduledIterations, "drop accounting and threshold result");
        uint32_t thresholdCount = 0;
        Check(MtLoad_GetThresholdResults(handle, nullptr, &thresholdCount) == MTLOAD_E_BUFFER_TOO_SMALL &&
            thresholdCount == 1, "threshold count");
        std::vector<MTLOAD_THRESHOLD_RESULT> thresholdRows(thresholdCount);
        Check(MtLoad_GetThresholdResults(handle, thresholdRows.data(), &thresholdCount) == MTLOAD_OK &&
            thresholdRows[0].passed == 0, "threshold failure detail");

        const auto v4ReportDirectory = root / "workloads-v4-reports";
        const auto v4Json = v4ReportDirectory / "MtLoadResult.json";
        const auto v4Xml = root / "workloads-v4-report.xml";
        const auto v4Xlsx = root / "workloads-v4.xlsx";
        Check(MtLoad_ExportReport(handle, v4ReportDirectory.u8string().c_str(), MTLOAD_REPORT_JSON) == MTLOAD_OK,
            "V4 JSON report export");
        Check(MtLoad_ExportReportFile(handle, v4Xml.u8string().c_str(), MTLOAD_REPORT_XML) == MTLOAD_OK,
            "V4 XML report export");
        Check(MtLoad_ExportReportFile(handle, v4Xlsx.u8string().c_str(), MTLOAD_REPORT_XLSX) == MTLOAD_OK &&
            std::filesystem::file_size(v4Xlsx) > 0, "V4 XLSX report export");
        std::ifstream v4JsonStream(v4Json, std::ios::binary);
        const std::string v4JsonText((std::istreambuf_iterator<char>(v4JsonStream)),
            std::istreambuf_iterator<char>());
        std::ifstream v4XmlStream(v4Xml, std::ios::binary);
        const std::string v4XmlText((std::istreambuf_iterator<char>(v4XmlStream)),
            std::istreambuf_iterator<char>());
        Check(v4JsonText.find("\"Transactions\"") != std::string::npos &&
            v4JsonText.find("\"Thresholds\"") != std::string::npos &&
            v4JsonText.find("no-drop") != std::string::npos,
            "V4 JSON transaction and threshold content");
        Check(v4XmlText.find(u8"业务事务列表") != std::string::npos &&
            v4XmlText.find(u8"阈值列表") != std::string::npos &&
            v4XmlText.find(u8"结果码=\"1302\"") != std::string::npos,
            "V4 XML transaction and threshold content");

        // 到达率允许降到 0，但任何负初始值或负阶段目标都必须在校验期拒绝。
        const auto invalidRateXml = root / "invalid-arrival-rate.xml";
        std::ofstream(invalidRateXml) <<
            "<MtLoadPlan version=\"1\" name=\"invalid-rate\"><Targets>"
            "<Target id=\"fast\" transport=\"http\" host=\"127.0.0.1\" port=\"" << arrivalFast.Port() << "\"/>"
            "</Targets><Scenarios><Scenario id=\"a\"><Load mode=\"closed\" users=\"1\" iterations=\"1\"/>"
            "<Step id=\"one\"><Request id=\"a\" label=\"a\" target=\"fast\" method=\"GET\" path=\"/a\" codec=\"json\"/>"
            "</Step></Scenario></Scenarios><Workloads default=\"invalid\">"
            "<Workload id=\"invalid\"><Load mode=\"arrival\" ratePerSec=\"10\" preAllocatedUsers=\"1\" maxUsers=\"1\">"
            "<Stage durationMs=\"100\" targetRatePerSec=\"-1\"/></Load><ScenarioRef id=\"a\" weight=\"1\"/>"
            "</Workload></Workloads></MtLoadPlan>";
        Check(MtLoad_LoadPlan(handle, invalidRateXml.u8string().c_str()) == MTLOAD_OK,
            "invalid arrival rate parse");
        Check(MtLoad_ValidatePlan(handle) == MTLOAD_E_CONFIG_SEMANTIC,
            "negative arrival rate rejected");
        Check(MtLoad_Destroy(handle) == MTLOAD_OK, "V4 destroy");
        std::cout << "MtLoadEngine ABI/lifecycle: PASS\n";
        return 0;
    }
    catch (const std::exception& exception)
    {
        std::cerr << "FAIL: " << exception.what() << '\n';
        return 1;
    }
}
