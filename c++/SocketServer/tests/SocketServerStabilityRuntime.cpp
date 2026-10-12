#include "SocketServer.h"

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <Psapi.h>
#include <TlHelp32.h>

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace
{
	const unsigned short kHttpPort = 39201;
	const unsigned short kTcpPort = 39202;
	const unsigned short kWebPort = 39203;
	const DWORD kSocketTimeoutMs = 5000;

	CSocketServer* g_pHttp = nullptr;
	CSocketServer* g_pTcp = nullptr;
	CSocketServer* g_pWeb = nullptr;
	std::atomic<unsigned long long> g_ullHttpCallbacks(0);
	std::atomic<unsigned long long> g_ullTcpCallbacks(0);
	std::atomic<unsigned long long> g_ullWebCallbacks(0);

	struct ProcessSnapshot
	{
		DWORD dwHandles;
		DWORD dwThreads;
		SIZE_T szPrivateBytes;
	};

	void Require(bool p_bCondition, const char* p_szMessage)
	{
		if (!p_bCondition)
		{
			std::cerr << "FAILED: " << (p_szMessage != nullptr ? p_szMessage : "") << std::endl;
			std::exit(1);
		}
	}

	void HttpNotify(CHttpAsynReq* p_pRequest)
	{
		if (p_pRequest == nullptr || g_pHttp == nullptr)
		{
			return;
		}
		const char szResponse[] = "OK";
		p_pRequest->SendResponse(szResponse, 2);
		g_pHttp->DelHttpAsynReq(p_pRequest->GetConnAsyId());
		g_ullHttpCallbacks.fetch_add(1, std::memory_order_relaxed);
	}

	void TcpNotify(void* p_pServer, void* p_pClient, TcpSockNotifyType p_enType,
		const void* p_pData, unsigned int p_uiDataLen, const char*, unsigned short, void*)
	{
		if (p_enType != enTcpData || g_pTcp == nullptr || p_pData == nullptr || p_uiDataLen == 0)
		{
			return;
		}
		if (g_pTcp->TcpSockSend(p_pServer, p_pClient,
			static_cast<const char*>(p_pData), static_cast<int>(p_uiDataLen)))
		{
			g_ullTcpCallbacks.fetch_add(1, std::memory_order_relaxed);
		}
	}

	void WebNotify(void* p_pServer, void* p_pClient, WebSockNotifyType p_enType,
		const void*, int, const char*, unsigned short, void*)
	{
		if (p_enType != enWebData || g_pWeb == nullptr)
		{
			return;
		}
		const char szResponse[] = "W";
		if (g_pWeb->WebSockSendText(p_pServer, p_pClient, szResponse, 1))
		{
			g_ullWebCallbacks.fetch_add(1, std::memory_order_relaxed);
		}
	}

	bool SetSocketTimeout(SOCKET p_hSocket)
	{
		return setsockopt(p_hSocket, SOL_SOCKET, SO_RCVTIMEO,
			reinterpret_cast<const char*>(&kSocketTimeoutMs), sizeof(kSocketTimeoutMs)) == 0 &&
			setsockopt(p_hSocket, SOL_SOCKET, SO_SNDTIMEO,
			reinterpret_cast<const char*>(&kSocketTimeoutMs), sizeof(kSocketTimeoutMs)) == 0;
	}

	SOCKET ConnectLocal(unsigned short p_usPort)
	{
		SOCKET hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (hSocket == INVALID_SOCKET || !SetSocketTimeout(hSocket))
		{
			if (hSocket != INVALID_SOCKET)
			{
				closesocket(hSocket);
			}
			return INVALID_SOCKET;
		}
		sockaddr_in stAddress = {};
		stAddress.sin_family = AF_INET;
		stAddress.sin_port = htons(p_usPort);
		inet_pton(AF_INET, "127.0.0.1", &stAddress.sin_addr);
		if (connect(hSocket, reinterpret_cast<sockaddr*>(&stAddress), sizeof(stAddress)) != 0)
		{
			closesocket(hSocket);
			return INVALID_SOCKET;
		}
		return hSocket;
	}

	bool SendAll(SOCKET p_hSocket, const char* p_pData, size_t p_szLength)
	{
		size_t szOffset = 0;
		while (szOffset < p_szLength)
		{
			const int iSent = send(p_hSocket, p_pData + szOffset,
				static_cast<int>(p_szLength - szOffset), 0);
			if (iSent <= 0)
			{
				return false;
			}
			szOffset += static_cast<size_t>(iSent);
		}
		return true;
	}

	bool ReceiveExact(SOCKET p_hSocket, char* p_pData, size_t p_szLength)
	{
		size_t szOffset = 0;
		while (szOffset < p_szLength)
		{
			const int iRead = recv(p_hSocket, p_pData + szOffset,
				static_cast<int>(p_szLength - szOffset), 0);
			if (iRead <= 0)
			{
				return false;
			}
			szOffset += static_cast<size_t>(iRead);
		}
		return true;
	}

	bool RunHttpRequest()
	{
		SOCKET hSocket = ConnectLocal(kHttpPort);
		if (hSocket == INVALID_SOCKET)
		{
			return false;
		}
		const char szRequest[] =
			"GET /stability HTTP/1.0\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
		bool bOK = SendAll(hSocket, szRequest, sizeof(szRequest) - 1);
		std::string strResponse;
		char szBuffer[1024] = { 0 };
		while (bOK)
		{
			const int iRead = recv(hSocket, szBuffer, sizeof(szBuffer), 0);
			if (iRead == 0)
			{
				break;
			}
			if (iRead < 0)
			{
				bOK = false;
				break;
			}
			strResponse.append(szBuffer, iRead);
			const size_t szHeaderEnd = strResponse.find("\r\n\r\n");
			if (szHeaderEnd != std::string::npos &&
				strResponse.size() >= szHeaderEnd + 4 + 2)
			{
				break;
			}
		}
		closesocket(hSocket);
		// HP-Socket 在 HTTP/1.0 响应完整写出后释放连接时，Windows 客户端可能以
		// WSAECONNRESET 而不是 recv=0 结束读取；只要完整状态行和正文已收到即为成功。
		const bool bValid = strResponse.find("200") != std::string::npos &&
			strResponse.find("OK") != std::string::npos;
		return bValid;
	}

	bool OpenWebSocket(SOCKET& p_refSocket)
	{
		p_refSocket = ConnectLocal(kWebPort);
		if (p_refSocket == INVALID_SOCKET)
		{
			return false;
		}
		const char szHandshake[] =
			"GET /stability HTTP/1.1\r\n"
			"Host: 127.0.0.1\r\n"
			"Upgrade: websocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
			"Sec-WebSocket-Version: 13\r\n\r\n";
		if (!SendAll(p_refSocket, szHandshake, sizeof(szHandshake) - 1))
		{
			closesocket(p_refSocket);
			p_refSocket = INVALID_SOCKET;
			return false;
		}
		std::string strResponse;
		char szBuffer[512] = { 0 };
		while (strResponse.find("\r\n\r\n") == std::string::npos)
		{
			const int iRead = recv(p_refSocket, szBuffer, sizeof(szBuffer), 0);
			if (iRead <= 0)
			{
				closesocket(p_refSocket);
				p_refSocket = INVALID_SOCKET;
				return false;
			}
			strResponse.append(szBuffer, iRead);
		}
		return strResponse.find("101 Switching Protocols") != std::string::npos;
	}

	bool RunWebMessage(SOCKET p_hSocket, unsigned char p_byValue)
	{
		const unsigned char byMask[4] = { 0x31, 0x72, 0x19, 0xA4 };
		const unsigned char byFrame[7] = { 0x81, 0x81,
			byMask[0], byMask[1], byMask[2], byMask[3],
			static_cast<unsigned char>(p_byValue ^ byMask[0]) };
		if (!SendAll(p_hSocket, reinterpret_cast<const char*>(byFrame), sizeof(byFrame)))
		{
			return false;
		}
		unsigned char byHeader[2] = { 0 };
		char szPayload = 0;
		return ReceiveExact(p_hSocket, reinterpret_cast<char*>(byHeader), sizeof(byHeader)) &&
			byHeader[0] == 0x81 && byHeader[1] == 1 &&
			ReceiveExact(p_hSocket, &szPayload, 1) && szPayload == 'W';
	}

	void WaitForStart(const std::atomic<bool>& p_refStart)
	{
		while (!p_refStart.load(std::memory_order_acquire))
		{
			std::this_thread::yield();
		}
	}

	bool StartServers()
	{
		g_pHttp = CreateHttpSockInstanceByName("StabilityHttp");
		g_pTcp = CreateTcpSockInstanceByName("StabilityTcp");
		g_pWeb = CreateWebSockInstanceByName("StabilityWeb");
		if (g_pHttp == nullptr || g_pTcp == nullptr || g_pWeb == nullptr)
		{
			return false;
		}
		char szError[1024] = { 0 };
		if (!g_pHttp->SetSocketListenQueue(1024) ||
			!g_pHttp->CreateHttpSock("127.0.0.1", kHttpPort, 64 * 1024,
				8192, 128, HttpNotify, 8, 16384, szError, nullptr))
		{
			std::cerr << "HTTP start failed: " << szError << std::endl;
			return false;
		}
		std::memset(szError, 0, sizeof(szError));
		if (!g_pTcp->SetSocketListenQueue(1024) ||
			!g_pTcp->CreateTcpSock("127.0.0.1", kTcpPort, 64 * 1024,
				8192, 128, TcpNotify, 8, 16384, szError, nullptr))
		{
			std::cerr << "TCP start failed: " << szError << std::endl;
			return false;
		}
		std::memset(szError, 0, sizeof(szError));
		if (!g_pWeb->SetSocketListenQueue(1024) ||
			!g_pWeb->CreateWebSock("127.0.0.1", kWebPort, 64 * 1024,
				8192, 128, WebNotify, 8, 16384, szError, nullptr))
		{
			std::cerr << "WebSocket start failed: " << szError << std::endl;
			return false;
		}
		return true;
	}

	void StopAndDeleteServers()
	{
		if (g_pHttp != nullptr)
		{
			g_pHttp->StopHttpSock();
		}
		if (g_pTcp != nullptr)
		{
			g_pTcp->StopTcpSock();
		}
		if (g_pWeb != nullptr)
		{
			g_pWeb->StopWebSock();
		}
		DelHttpSockInstance(g_pHttp);
		DelTcpSockInstance(g_pTcp);
		DelWebSockInstance(g_pWeb);
	}

	void RunAbruptDisconnects(int p_iCount)
	{
		const char szPartialHttp[] = "POST /partial HTTP/1.1\r\nContent-Length: 4096\r\n\r\nshort";
		const char szPartialWeb[] = "GET /partial HTTP/1.1\r\nUpgrade: websocket\r\n";
		for (int i = 0; i < p_iCount; ++i)
		{
			SOCKET hHttp = ConnectLocal(kHttpPort);
			if (hHttp != INVALID_SOCKET)
			{
				SendAll(hHttp, szPartialHttp, sizeof(szPartialHttp) - 1);
				closesocket(hHttp);
			}
			SOCKET hTcp = ConnectLocal(kTcpPort);
			if (hTcp != INVALID_SOCKET)
			{
				const char szPartialTcp[] = "partial";
				SendAll(hTcp, szPartialTcp, sizeof(szPartialTcp) - 1);
				closesocket(hTcp);
			}
			SOCKET hWeb = ConnectLocal(kWebPort);
			if (hWeb != INVALID_SOCKET)
			{
				SendAll(hWeb, szPartialWeb, sizeof(szPartialWeb) - 1);
				closesocket(hWeb);
			}
		}
	}

	void RunTraffic(int p_iHttpWorkers, int p_iHttpRequests,
		int p_iTcpWorkers, int p_iTcpMessages,
		int p_iWebWorkers, int p_iWebMessages)
	{
		std::atomic<bool> bStart(false);
		std::atomic<unsigned long long> ullHttpFailures(0);
		std::atomic<unsigned long long> ullTcpFailures(0);
		std::atomic<unsigned long long> ullWebFailures(0);
		std::vector<std::thread> vecWorkers;
		for (int i = 0; i < p_iHttpWorkers; ++i)
		{
			vecWorkers.emplace_back([&]() {
				WaitForStart(bStart);
				for (int n = 0; n < p_iHttpRequests; ++n)
				{
					if (!RunHttpRequest())
					{
						ullHttpFailures.fetch_add(1, std::memory_order_relaxed);
					}
				}
			});
		}
		for (int i = 0; i < p_iTcpWorkers; ++i)
		{
			vecWorkers.emplace_back([&, i]() {
				WaitForStart(bStart);
				SOCKET hSocket = ConnectLocal(kTcpPort);
				if (hSocket == INVALID_SOCKET)
				{
					ullTcpFailures.fetch_add(1, std::memory_order_relaxed);
					return;
				}
				for (int n = 0; n < p_iTcpMessages; ++n)
				{
					char szPayload[64] = { 0 };
					snprintf(szPayload, sizeof(szPayload), "tcp-worker=%d,message=%d", i, n);
					const size_t szLength = std::strlen(szPayload);
					std::vector<char> vecEcho(szLength);
					if (!SendAll(hSocket, szPayload, szLength) ||
						!ReceiveExact(hSocket, vecEcho.data(), vecEcho.size()) ||
						std::memcmp(szPayload, vecEcho.data(), szLength) != 0)
					{
						ullTcpFailures.fetch_add(1, std::memory_order_relaxed);
						break;
					}
				}
				closesocket(hSocket);
			});
		}
		for (int i = 0; i < p_iWebWorkers; ++i)
		{
			vecWorkers.emplace_back([&, i]() {
				WaitForStart(bStart);
				SOCKET hSocket = INVALID_SOCKET;
				if (!OpenWebSocket(hSocket))
				{
					ullWebFailures.fetch_add(1, std::memory_order_relaxed);
					return;
				}
				for (int n = 0; n < p_iWebMessages; ++n)
				{
					if (!RunWebMessage(hSocket, static_cast<unsigned char>('a' + ((i + n) % 26))))
					{
						ullWebFailures.fetch_add(1, std::memory_order_relaxed);
						break;
					}
				}
				closesocket(hSocket);
			});
		}
		bStart.store(true, std::memory_order_release);
		for (std::thread& clWorker : vecWorkers)
		{
			clWorker.join();
		}
		const unsigned long long ullHttpFailureCount = ullHttpFailures.load();
		const unsigned long long ullTcpFailureCount = ullTcpFailures.load();
		const unsigned long long ullWebFailureCount = ullWebFailures.load();
		if (ullHttpFailureCount != 0 || ullTcpFailureCount != 0 || ullWebFailureCount != 0)
		{
			std::cerr << "traffic_failures: http=" << ullHttpFailureCount
				<< ",tcp=" << ullTcpFailureCount
				<< ",web=" << ullWebFailureCount << std::endl;
		}
		Require(ullHttpFailureCount == 0 && ullTcpFailureCount == 0 && ullWebFailureCount == 0,
			"concurrent traffic completed without client failures");
	}

	void RunShutdownRace()
	{
		Require(StartServers(), "start shutdown-race services");
		std::atomic<bool> bStop(false);
		std::atomic<unsigned long long> ullHttpCompleted(0);
		std::atomic<unsigned long long> ullTcpCompleted(0);
		std::atomic<unsigned long long> ullWebCompleted(0);
		std::vector<std::thread> vecWorkers;
		for (int i = 0; i < 8; ++i)
		{
			vecWorkers.emplace_back([&]() {
				while (!bStop.load(std::memory_order_acquire))
				{
					if (RunHttpRequest())
					{
						ullHttpCompleted.fetch_add(1, std::memory_order_relaxed);
					}
				}
			});
		}
		for (int i = 0; i < 4; ++i)
		{
			vecWorkers.emplace_back([&]() {
				SOCKET hSocket = ConnectLocal(kTcpPort);
				if (hSocket == INVALID_SOCKET)
				{
					return;
				}
				const char szPayload[] = "shutdown-tcp";
				char szEcho[sizeof(szPayload) - 1] = { 0 };
				while (!bStop.load(std::memory_order_acquire) &&
					SendAll(hSocket, szPayload, sizeof(szPayload) - 1) &&
					ReceiveExact(hSocket, szEcho, sizeof(szEcho)))
				{
					ullTcpCompleted.fetch_add(1, std::memory_order_relaxed);
				}
				closesocket(hSocket);
			});
		}
		for (int i = 0; i < 4; ++i)
		{
			vecWorkers.emplace_back([&]() {
				SOCKET hSocket = INVALID_SOCKET;
				if (!OpenWebSocket(hSocket))
				{
					return;
				}
				while (!bStop.load(std::memory_order_acquire) &&
					RunWebMessage(hSocket, 's'))
				{
					ullWebCompleted.fetch_add(1, std::memory_order_relaxed);
				}
				closesocket(hSocket);
			});
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		const auto clStart = std::chrono::steady_clock::now();
		g_pHttp->StopHttpSock();
		g_pTcp->StopTcpSock();
		g_pWeb->StopWebSock();
		const auto clElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - clStart).count();
		bStop.store(true, std::memory_order_release);
		for (std::thread& clWorker : vecWorkers)
		{
			clWorker.join();
		}
		Require(clElapsed < 15000, "shutdown with in-flight requests completed within 15 seconds");
		Require(ullHttpCompleted.load(std::memory_order_relaxed) > 0 &&
			ullTcpCompleted.load(std::memory_order_relaxed) > 0 &&
			ullWebCompleted.load(std::memory_order_relaxed) > 0,
			"shutdown race exercised all three active protocols");
		DelHttpSockInstance(g_pHttp);
		DelTcpSockInstance(g_pTcp);
		DelWebSockInstance(g_pWeb);
		std::cout << "shutdown_ms=" << clElapsed
			<< ",http_before_stop=" << ullHttpCompleted.load()
			<< ",tcp_before_stop=" << ullTcpCompleted.load()
			<< ",web_before_stop=" << ullWebCompleted.load() << std::endl;
	}

	DWORD CountProcessThreads()
	{
		DWORD dwCount = 0;
		const DWORD dwProcessId = GetCurrentProcessId();
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
		Require(hSnapshot != INVALID_HANDLE_VALUE, "create thread snapshot");
		THREADENTRY32 stEntry = {};
		stEntry.dwSize = sizeof(stEntry);
		if (Thread32First(hSnapshot, &stEntry))
		{
			do
			{
				if (stEntry.th32OwnerProcessID == dwProcessId)
				{
					++dwCount;
				}
			} while (Thread32Next(hSnapshot, &stEntry));
		}
		CloseHandle(hSnapshot);
		return dwCount;
	}

	ProcessSnapshot CaptureProcessSnapshot()
	{
		ProcessSnapshot stSnapshot = {};
		Require(GetProcessHandleCount(GetCurrentProcess(), &stSnapshot.dwHandles) != FALSE,
			"read process handle count");
		stSnapshot.dwThreads = CountProcessThreads();
		PROCESS_MEMORY_COUNTERS_EX stMemory = {};
		stMemory.cb = sizeof(stMemory);
		Require(GetProcessMemoryInfo(GetCurrentProcess(),
			reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&stMemory), sizeof(stMemory)) != FALSE,
			"read process memory counters");
		stSnapshot.szPrivateBytes = stMemory.PrivateUsage;
		return stSnapshot;
	}

	void PrintSnapshot(const char* p_szName, const ProcessSnapshot& p_refSnapshot)
	{
		std::cout << p_szName << ": handles=" << p_refSnapshot.dwHandles
			<< ",threads=" << p_refSnapshot.dwThreads
			<< ",private_bytes=" << static_cast<unsigned long long>(p_refSnapshot.szPrivateBytes)
			<< std::endl;
	}

	void RunOneCycle(int p_iCycle, int p_iHttpWorkers, int p_iHttpRequests,
		int p_iTcpWorkers, int p_iTcpMessages,
		int p_iWebWorkers, int p_iWebMessages)
	{
		Require(StartServers(), "start stability services");
		const unsigned long long ullHttpBefore = g_ullHttpCallbacks.load();
		const unsigned long long ullWebBefore = g_ullWebCallbacks.load();
		RunTraffic(p_iHttpWorkers, p_iHttpRequests,
			p_iTcpWorkers, p_iTcpMessages, p_iWebWorkers, p_iWebMessages);
		if (p_iHttpWorkers + p_iTcpWorkers + p_iWebWorkers > 0)
		{
			RunAbruptDisconnects(24);
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(150));
		Require(g_ullHttpCallbacks.load() - ullHttpBefore ==
			static_cast<unsigned long long>(p_iHttpWorkers) * p_iHttpRequests,
			"all HTTP callbacks completed exactly once");
		Require(g_ullWebCallbacks.load() - ullWebBefore ==
			static_cast<unsigned long long>(p_iWebWorkers) * p_iWebMessages,
			"all WebSocket callbacks completed exactly once");
		StopAndDeleteServers();
		std::this_thread::sleep_for(std::chrono::milliseconds(250));
		const ProcessSnapshot stCycle = CaptureProcessSnapshot();
		std::cout << "cycle=" << p_iCycle << ",http="
			<< static_cast<unsigned long long>(p_iHttpWorkers) * p_iHttpRequests
			<< ",tcp_messages="
			<< static_cast<unsigned long long>(p_iTcpWorkers) * p_iTcpMessages
			<< ",web_messages="
			<< static_cast<unsigned long long>(p_iWebWorkers) * p_iWebMessages
			<< ",handles=" << stCycle.dwHandles
			<< ",threads=" << stCycle.dwThreads
			<< ",private_bytes=" << static_cast<unsigned long long>(stCycle.szPrivateBytes)
			<< std::endl;
	}
}

int main(int argc, char* argv[])
{
	const int iCycles = argc > 1 ? std::atoi(argv[1]) : 8;
	const int iHttpWorkers = argc > 2 ? std::atoi(argv[2]) : 12;
	const int iHttpRequests = argc > 3 ? std::atoi(argv[3]) : 100;
	const int iTcpWorkers = argc > 4 ? std::atoi(argv[4]) : 8;
	const int iTcpMessages = argc > 5 ? std::atoi(argv[5]) : 200;
	const int iWebWorkers = argc > 6 ? std::atoi(argv[6]) : 8;
	const int iWebMessages = argc > 7 ? std::atoi(argv[7]) : 100;
	Require(iCycles > 0 && iHttpWorkers >= 0 && iHttpRequests >= 0 &&
		iTcpWorkers >= 0 && iTcpMessages >= 0 && iWebWorkers >= 0 && iWebMessages >= 0,
		"cycle count must be positive and workload parameters cannot be negative");

	WSADATA stWsa = {};
	Require(WSAStartup(MAKEWORD(2, 2), &stWsa) == 0, "WSAStartup");
	Require(GetSocketServerAbiVersion() == SOCKET_SERVER_ABI_VERSION,
		"SocketServer ABI matches test header");
	const auto clTestStart = std::chrono::steady_clock::now();

	// 小流量预热让 DLL 延迟加载、线程池和进程堆完成一次完整生命周期，
	// 后续资源差值才反映重复启停是否泄漏，而不是首次加载开销。
	RunOneCycle(0, 2, 10, 2, 20, 2, 10);
	std::this_thread::sleep_for(std::chrono::milliseconds(500));
	const ProcessSnapshot stBaseline = CaptureProcessSnapshot();
	PrintSnapshot("baseline", stBaseline);

	for (int i = 1; i <= iCycles; ++i)
	{
		RunOneCycle(i, iHttpWorkers, iHttpRequests,
			iTcpWorkers, iTcpMessages, iWebWorkers, iWebMessages);
	}
	RunShutdownRace();
	std::this_thread::sleep_for(std::chrono::seconds(1));
	const ProcessSnapshot stFinal = CaptureProcessSnapshot();
	PrintSnapshot("final", stFinal);

	const SIZE_T szMemoryAllowance = 32ULL * 1024ULL * 1024ULL;
	Require(stFinal.dwHandles <= stBaseline.dwHandles + 12,
		"handle count returned to the post-warmup range");
	Require(stFinal.dwThreads <= stBaseline.dwThreads + 2,
		"thread count returned to the post-warmup range");
	Require(stFinal.szPrivateBytes <= stBaseline.szPrivateBytes + szMemoryAllowance,
		"private bytes returned to the post-warmup range");

	WSACleanup();
	const auto clTestElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - clTestStart).count();
	std::cout << "SocketServer stability checks passed: cycles=" << iCycles
		<< ",http_callbacks=" << g_ullHttpCallbacks.load()
		<< ",tcp_callbacks=" << g_ullTcpCallbacks.load()
		<< ",web_callbacks=" << g_ullWebCallbacks.load()
		<< ",elapsed_ms=" << clTestElapsed << std::endl;
	return 0;
}
