#include "SocketServer.h"

#include <WinSock2.h>
#include <WS2tcpip.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>

namespace
{
	CSocketServer* g_pHttpA = nullptr; // 测试进程持有的第一个 HTTP 实例。
	CSocketServer* g_pHttpB = nullptr; // 测试进程持有的第二个 HTTP 实例。

	// 断言失败时立即终止测试并打印可定位的英文原因。
	void Require(bool p_bCondition, const char* p_szMessage)
	{
		if (!p_bCondition)
		{
			std::cerr << "FAILED: " << (p_szMessage != nullptr ? p_szMessage : "") << std::endl;
			std::exit(1);
		}
	}

	// 完成第一个 HTTP 实例的应答并释放其异步请求对象。
	void HttpNotifyA(CHttpAsynReq* p_pRequest)
	{
		if (p_pRequest == nullptr || g_pHttpA == nullptr)
		{
			return;
		}
		const char szResponse[] = "A";
		p_pRequest->SendResponse(szResponse, 1);
		g_pHttpA->DelHttpAsynReq(p_pRequest->GetConnAsyId());
	}

	// 完成第二个 HTTP 实例的应答并释放其异步请求对象。
	void HttpNotifyB(CHttpAsynReq* p_pRequest)
	{
		if (p_pRequest == nullptr || g_pHttpB == nullptr)
		{
			return;
		}
		const char szResponse[] = "B";
		p_pRequest->SendResponse(szResponse, 1);
		g_pHttpB->DelHttpAsynReq(p_pRequest->GetConnAsyId());
	}

	// 向指定本机端口发送一次短 HTTP 请求并返回完整应答文本。
	std::string SendHttp(unsigned short p_usPort)
	{
		SOCKET hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		Require(hSocket != INVALID_SOCKET, "create client socket");
		sockaddr_in stAddress = {};
		stAddress.sin_family = AF_INET;
		stAddress.sin_port = htons(p_usPort);
		inet_pton(AF_INET, "127.0.0.1", &stAddress.sin_addr);
		Require(connect(hSocket, reinterpret_cast<sockaddr*>(&stAddress),
			sizeof(stAddress)) == 0, "connect HTTP instance");
		const char szRequest[] = "GET /probe HTTP/1.0\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
		Require(send(hSocket, szRequest, static_cast<int>(sizeof(szRequest) - 1), 0) > 0,
			"send HTTP request");
		std::string strResponse;
		char szBuffer[1024] = { 0 };
		for (;;)
		{
			const int iRead = recv(hSocket, szBuffer, sizeof(szBuffer), 0);
			if (iRead <= 0)
			{
				break;
			}
			strResponse.append(szBuffer, iRead);
		}
		closesocket(hSocket);
		return strResponse;
	}

	// 校验运行信息中的固定身份和当前监听状态。
	void VerifyRuntime(CSocketServer* p_pServer, EN_SOCKET_SERVER_TYPE p_enType,
		const char* p_szName, bool p_bStarted, unsigned short p_usPort,
		unsigned int p_uiMaxConnections = 0,
		unsigned int p_uiAcceptSocketCount = 0,
		unsigned int p_uiSocketListenQueue = 0)
	{
		ST_SOCKET_SERVER_RUNTIME_INFO stInfo = {};
		stInfo.uiStructSize = sizeof(stInfo);
		Require(GetSocketServerRuntimeInfo(p_pServer, &stInfo), "read runtime info");
		Require(stInfo.uiAbiVersion == SOCKET_SERVER_ABI_VERSION, "runtime ABI");
		Require(stInfo.enServerType == p_enType, "runtime type");
		Require(std::strcmp(stInfo.szServiceName, p_szName) == 0, "runtime name");
		Require((stInfo.bStarted != 0) == p_bStarted, "runtime started state");
		Require(stInfo.usPort == p_usPort, "runtime port");
		Require(stInfo.ullInstanceId != 0, "runtime instance id");
		Require(stInfo.uiMaxConnectionCount == p_uiMaxConnections,
			"runtime max connection count");
		Require(stInfo.uiAcceptSocketCount == p_uiAcceptSocketCount,
			"runtime accept socket count");
		Require(stInfo.uiSocketListenQueue == p_uiSocketListenQueue,
			"runtime socket listen queue");
	}
}

// 覆盖命名唯一性、跨协议同名、双 HTTP 并存和单实例停止隔离。
int main()
{
	WSADATA stWsaData = {};
	Require(WSAStartup(MAKEWORD(2, 2), &stWsaData) == 0, "WSAStartup");
	Require(GetSocketServerAbiVersion() == 7U, "SocketServer ABI 7");

	g_pHttpA = CreateHttpSockInstance();
	Require(g_pHttpA != nullptr, "create default HTTP instance");
	Require(CreateHttpSockInstance() == nullptr, "reject duplicate default HTTP name");
	VerifyRuntime(g_pHttpA, EN_SOCKET_SERVER_TYPE_HTTP, "1", false, 0);

	g_pHttpB = CreateHttpSockInstanceByName("SecondHttp");
	Require(g_pHttpB != nullptr, "create named HTTP instance");
	Require(CreateHttpSockInstanceByName("SecondHttp") == nullptr,
		"reject duplicate named HTTP instance");
	Require(CreateHttpSockInstanceByName("bad name") == nullptr,
		"reject invalid logical name");

	CSocketServer* pTcp = CreateTcpSockInstanceByName("SharedName");
	CSocketServer* pWeb = CreateWebSockInstanceByName("SharedName");
	Require(pTcp != nullptr && pWeb != nullptr, "allow same name across protocols");
	VerifyRuntime(pTcp, EN_SOCKET_SERVER_TYPE_TCP, "SharedName", false, 0);
	VerifyRuntime(pWeb, EN_SOCKET_SERVER_TYPE_WEB, "SharedName", false, 0);

	char szError[1024] = { 0 };
	Require(!g_pHttpA->SetSocketListenQueue(0),
		"reject zero socket listen queue");
	Require(!g_pHttpA->SetSocketListenQueue(65536U),
		"reject oversized socket listen queue");
	Require(g_pHttpA->SetSocketListenQueue(257),
		"configure first socket listen queue");
	Require(g_pHttpB->SetSocketListenQueue(513),
		"configure second socket listen queue");
	Require(g_pHttpA->CreateHttpSock("127.0.0.1", 39101, 64 * 1024,
		1024, 32, HttpNotifyA, 2, 1024, szError, nullptr), "listen first HTTP");
	Require(g_pHttpB->CreateHttpSock("127.0.0.1", 39102, 64 * 1024,
		2048, 64, HttpNotifyB, 2, 1024, szError, nullptr), "listen second HTTP");
	VerifyRuntime(g_pHttpA, EN_SOCKET_SERVER_TYPE_HTTP, "1", true, 39101,
		1024, 32, 257);
	VerifyRuntime(g_pHttpB, EN_SOCKET_SERVER_TYPE_HTTP, "SecondHttp", true, 39102,
		2048, 64, 513);
	Require(!g_pHttpA->SetSocketListenQueue(1024),
		"reject socket listen queue change while started");
	Require(SendHttp(39101).find("A") != std::string::npos, "first HTTP callback");
	Require(SendHttp(39102).find("B") != std::string::npos, "second HTTP callback");

	g_pHttpA->StopHttpSock();
	VerifyRuntime(g_pHttpA, EN_SOCKET_SERVER_TYPE_HTTP, "1", false, 0);
	Require(SendHttp(39102).find("B") != std::string::npos,
		"second HTTP survives first stop");
	Require(g_pHttpA->SetSocketListenQueue(1025),
		"configure socket listen queue after stop");
	Require(g_pHttpA->CreateHttpSock("127.0.0.1", 39103, 64 * 1024,
		1024, 96, HttpNotifyA, 2, 1024, szError, nullptr),
		"restart first HTTP");
	VerifyRuntime(g_pHttpA, EN_SOCKET_SERVER_TYPE_HTTP, "1", true, 39103,
		1024, 96, 1025);
	Require(SendHttp(39103).find("A") != std::string::npos,
		"restarted first HTTP callback");
	g_pHttpA->StopHttpSock();

	DelHttpSockInstance(g_pHttpA);
	DelHttpSockInstance(g_pHttpB);
	DelTcpSockInstance(pTcp);
	DelWebSockInstance(pWeb);
	Require(g_pHttpA == nullptr && g_pHttpB == nullptr && pTcp == nullptr && pWeb == nullptr,
		"delete named instances");
	WSACleanup();
	std::cout << "SocketServer multi-instance runtime checks passed" << std::endl;
	return 0;
}
