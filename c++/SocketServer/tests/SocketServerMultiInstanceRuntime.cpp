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
	CSocketServer* g_pTcp = nullptr;   // 测试进程持有的 TCP 实例。
	CSocketServer* g_pWeb = nullptr;   // 测试进程持有的 WebSocket 实例。

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

	// TCP 收到业务数据后原样回送，用真实收发验证 HP-Socket 6.0.9 的服务端回调与发送接口。
	void TcpNotify(void* p_pServer, void* p_pClient, TcpSockNotifyType p_enType,
		const void* p_pData, unsigned int p_uiDataLen, const char*, unsigned short, void*)
	{
		if (p_enType == enTcpData && g_pTcp != nullptr && p_pData != nullptr && p_uiDataLen > 0)
		{
			g_pTcp->TcpSockSend(p_pServer, p_pClient,
				static_cast<const char*>(p_pData), static_cast<int>(p_uiDataLen));
		}
	}

	// WebSocket 收到完整消息后返回固定文本帧，验证升级、解帧、回调和有序发送链路。
	void WebNotify(void* p_pServer, void* p_pClient, WebSockNotifyType p_enType,
		const void*, int, const char*, unsigned short, void*)
	{
		if (p_enType == enWebData && g_pWeb != nullptr)
		{
			const char szResponse[] = "W";
			g_pWeb->WebSockSendText(p_pServer, p_pClient, szResponse, 1);
		}
	}

	// 为本机回归连接设置有限超时，网络回归失败时能够给出断言而不是永久阻塞。
	void SetSocketTimeout(SOCKET p_hSocket)
	{
		const DWORD dwTimeoutMs = 5000;
		Require(setsockopt(p_hSocket, SOL_SOCKET, SO_RCVTIMEO,
			reinterpret_cast<const char*>(&dwTimeoutMs), sizeof(dwTimeoutMs)) == 0,
			"set receive timeout");
		Require(setsockopt(p_hSocket, SOL_SOCKET, SO_SNDTIMEO,
			reinterpret_cast<const char*>(&dwTimeoutMs), sizeof(dwTimeoutMs)) == 0,
			"set send timeout");
	}

	// 建立一个带超时的本机 TCP 客户端连接。
	SOCKET ConnectLocal(unsigned short p_usPort)
	{
		SOCKET hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		Require(hSocket != INVALID_SOCKET, "create client socket");
		SetSocketTimeout(hSocket);
		sockaddr_in stAddress = {};
		stAddress.sin_family = AF_INET;
		stAddress.sin_port = htons(p_usPort);
		inet_pton(AF_INET, "127.0.0.1", &stAddress.sin_addr);
		Require(connect(hSocket, reinterpret_cast<sockaddr*>(&stAddress),
			sizeof(stAddress)) == 0, "connect local service");
		return hSocket;
	}

	// 精确读取指定字节数，避免 TCP 分段造成测试误判。
	std::string ReceiveExact(SOCKET p_hSocket, size_t p_szLength)
	{
		std::string strResult;
		strResult.resize(p_szLength);
		size_t szOffset = 0;
		while (szOffset < p_szLength)
		{
			const int iRead = recv(p_hSocket, &strResult[szOffset],
				static_cast<int>(p_szLength - szOffset), 0);
			Require(iRead > 0, "receive expected bytes");
			szOffset += static_cast<size_t>(iRead);
		}
		return strResult;
	}

	// 向指定本机端口发送一次短 HTTP 请求并返回完整应答文本。
	std::string SendHttp(unsigned short p_usPort)
	{
		SOCKET hSocket = ConnectLocal(p_usPort);
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

	// 通过真实客户端连接验证原始 TCP 服务能接收并回送数据。
	void VerifyTcpEcho(unsigned short p_usPort)
	{
		SOCKET hSocket = ConnectLocal(p_usPort);
		const char szPayload[] = "tcp-609";
		Require(send(hSocket, szPayload, static_cast<int>(sizeof(szPayload) - 1), 0)
			== static_cast<int>(sizeof(szPayload) - 1), "send TCP payload");
		Require(ReceiveExact(hSocket, sizeof(szPayload) - 1) == szPayload,
			"receive TCP echo");
		closesocket(hSocket);
	}

	// 完成标准 WebSocket 握手，发送一个带掩码的文本帧，并校验服务端文本应答。
	void VerifyWebSocketEcho(unsigned short p_usPort)
	{
		SOCKET hSocket = ConnectLocal(p_usPort);
		const char szHandshake[] =
			"GET /socket-server-test HTTP/1.1\r\n"
			"Host: 127.0.0.1\r\n"
			"Upgrade: websocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
			"Sec-WebSocket-Version: 13\r\n\r\n";
		Require(send(hSocket, szHandshake, static_cast<int>(sizeof(szHandshake) - 1), 0)
			== static_cast<int>(sizeof(szHandshake) - 1), "send WebSocket handshake");

		std::string strHandshakeResponse;
		char szBuffer[512] = { 0 };
		while (strHandshakeResponse.find("\r\n\r\n") == std::string::npos)
		{
			const int iRead = recv(hSocket, szBuffer, sizeof(szBuffer), 0);
			Require(iRead > 0, "receive WebSocket handshake");
			strHandshakeResponse.append(szBuffer, iRead);
		}
		Require(strHandshakeResponse.find("101 Switching Protocols") != std::string::npos,
			"WebSocket upgrade status");
		Require(strHandshakeResponse.find("Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=")
			!= std::string::npos, "WebSocket accept key");

		const unsigned char byMask[4] = { 1, 2, 3, 4 };
		const unsigned char byPayload = static_cast<unsigned char>('x') ^ byMask[0];
		const unsigned char byFrame[7] = { 0x81, 0x81,
			byMask[0], byMask[1], byMask[2], byMask[3], byPayload };
		Require(send(hSocket, reinterpret_cast<const char*>(byFrame), sizeof(byFrame), 0)
			== sizeof(byFrame), "send masked WebSocket frame");

		const std::string strHeader = ReceiveExact(hSocket, 2);
		Require(static_cast<unsigned char>(strHeader[0]) == 0x81,
			"receive WebSocket text frame");
		Require(static_cast<unsigned char>(strHeader[1]) == 1,
			"receive WebSocket payload length");
		Require(ReceiveExact(hSocket, 1) == "W", "receive WebSocket callback response");
		closesocket(hSocket);
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

	g_pTcp = CreateTcpSockInstanceByName("SharedName");
	g_pWeb = CreateWebSockInstanceByName("SharedName");
	Require(g_pTcp != nullptr && g_pWeb != nullptr, "allow same name across protocols");
	VerifyRuntime(g_pTcp, EN_SOCKET_SERVER_TYPE_TCP, "SharedName", false, 0);
	VerifyRuntime(g_pWeb, EN_SOCKET_SERVER_TYPE_WEB, "SharedName", false, 0);

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

	Require(g_pTcp->SetSocketListenQueue(128), "configure TCP listen queue");
	Require(g_pTcp->CreateTcpSock("127.0.0.1", 39104, 64 * 1024,
		1024, 32, TcpNotify, 2, 1024, szError, nullptr), "listen TCP instance");
	VerifyRuntime(g_pTcp, EN_SOCKET_SERVER_TYPE_TCP, "SharedName", true, 39104,
		1024, 32, 128);
	VerifyTcpEcho(39104);
	g_pTcp->StopTcpSock();
	VerifyRuntime(g_pTcp, EN_SOCKET_SERVER_TYPE_TCP, "SharedName", false, 0);

	Require(g_pWeb->SetSocketListenQueue(129), "configure WebSocket listen queue");
	Require(g_pWeb->CreateWebSock("127.0.0.1", 39105, 64 * 1024,
		1024, 32, WebNotify, 2, 1024, szError, nullptr), "listen WebSocket instance");
	VerifyRuntime(g_pWeb, EN_SOCKET_SERVER_TYPE_WEB, "SharedName", true, 39105,
		1024, 32, 129);
	VerifyWebSocketEcho(39105);
	g_pWeb->StopWebSock();
	VerifyRuntime(g_pWeb, EN_SOCKET_SERVER_TYPE_WEB, "SharedName", false, 0);

	DelHttpSockInstance(g_pHttpA);
	DelHttpSockInstance(g_pHttpB);
	DelTcpSockInstance(g_pTcp);
	DelWebSockInstance(g_pWeb);
	Require(g_pHttpA == nullptr && g_pHttpB == nullptr && g_pTcp == nullptr && g_pWeb == nullptr,
		"delete named instances");
	WSACleanup();
	std::cout << "SocketServer multi-instance runtime checks passed" << std::endl;
	return 0;
}
