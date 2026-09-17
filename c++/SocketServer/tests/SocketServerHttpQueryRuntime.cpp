#include "SocketServer.h"

#include <WinSock2.h>
#include <WS2tcpip.h>

#include <chrono>
#include <condition_variable>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	struct ST_HTTP_OBSERVATION
	{
		std::string strUrl;
		std::string strRawUrl;
		std::string strQuery;
		std::string strMethod;
		std::string strBody;
		std::map<std::string, std::string> mapParams;
	};

	CSocketServer* g_pHttpServer = nullptr;
	std::mutex g_mtxObservation;
	std::condition_variable g_cvObservation;
	std::vector<ST_HTTP_OBSERVATION> g_vecObservations;
	const char* g_aQueryNames[] =
	{
		"key", "empty", "dup", "encoded", "plus", "bad", "utf", "p0", "p127"
	};

	void Require(bool p_bCondition, const char* p_szMessage)
	{
		if (!p_bCondition)
		{
			std::cerr << "FAILED: " << (nullptr == p_szMessage ? "" : p_szMessage) << std::endl;
			std::exit(1);
		}
	}

	std::string SafeText(const char* p_szText)
	{
		return nullptr == p_szText ? std::string() : std::string(p_szText);
	}

	void HttpNotify(CHttpAsynReq* p_pReq)
	{
		if (nullptr == p_pReq || nullptr == g_pHttpServer)
		{
			return;
		}

		ST_HTTP_OBSERVATION stObservation;
		stObservation.strUrl = SafeText(p_pReq->GetUrl());
		stObservation.strRawUrl = SafeText(p_pReq->GetRawUrl());
		stObservation.strQuery = SafeText(p_pReq->GetQueryString());
		stObservation.strMethod = SafeText(p_pReq->GetMethodType());
		const char* pBody = static_cast<const char*>(p_pReq->GetContent());
		if (nullptr != pBody && p_pReq->GetContentLen() > 0)
		{
			stObservation.strBody.assign(pBody, pBody + p_pReq->GetContentLen());
		}
		for (size_t uiIndex = 0; uiIndex < sizeof(g_aQueryNames) / sizeof(g_aQueryNames[0]); ++uiIndex)
		{
			const char* pValue = p_pReq->GetParam(g_aQueryNames[uiIndex]);
			if (nullptr != pValue)
			{
				stObservation.mapParams[g_aQueryNames[uiIndex]] = pValue;
			}
		}

		{
			std::lock_guard<std::mutex> lock(g_mtxObservation);
			g_vecObservations.push_back(stObservation);
		}
		g_cvObservation.notify_all();

		const char szResponse[] = "ok";
		p_pReq->AddResponseHead("Content-Type", "text/plain");
		p_pReq->SendResponse(szResponse, static_cast<int>(sizeof(szResponse) - 1));
		g_pHttpServer->DelHttpAsynReq(p_pReq->GetConnAsyId());
	}

	std::string SendRawHttp(unsigned short p_usPort, const std::string& p_refStrRequest)
	{
		SOCKET hSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		Require(INVALID_SOCKET != hSocket, "create client socket");

		sockaddr_in stAddress = {};
		stAddress.sin_family = AF_INET;
		stAddress.sin_port = htons(p_usPort);
		inet_pton(AF_INET, "127.0.0.1", &stAddress.sin_addr);
		Require(0 == connect(hSocket, reinterpret_cast<sockaddr*>(&stAddress), sizeof(stAddress)), "connect HTTP server");

		size_t uiSent = 0;
		while (uiSent < p_refStrRequest.size())
		{
			const int iSent = send(hSocket, p_refStrRequest.data() + uiSent,
				static_cast<int>(p_refStrRequest.size() - uiSent), 0);
			Require(iSent > 0, "send HTTP request");
			uiSent += static_cast<size_t>(iSent);
		}

		std::string strResponse;
		char szBuffer[4096] = { 0 };
		for (;;)
		{
			fd_set stReadSet;
			FD_ZERO(&stReadSet);
			FD_SET(hSocket, &stReadSet);
			timeval stTimeout = { 5, 0 };
			if (select(0, &stReadSet, nullptr, nullptr, &stTimeout) <= 0)
			{
				break;
			}
			const int iRead = recv(hSocket, szBuffer, sizeof(szBuffer), 0);
			if (iRead <= 0)
			{
				break;
			}
			strResponse.append(szBuffer, iRead);
			const size_t uiHeaderEnd = strResponse.find("\r\n\r\n");
			if (uiHeaderEnd != std::string::npos && std::string::npos != strResponse.find(" 400 "))
			{
				break;
			}
			const size_t uiContentLength = strResponse.find("Content-Length:");
			if (uiHeaderEnd != std::string::npos && uiContentLength != std::string::npos)
			{
				const size_t uiValueBegin = uiContentLength + strlen("Content-Length:");
				const size_t uiValueEnd = strResponse.find("\r\n", uiValueBegin);
				const size_t uiBodyLength = static_cast<size_t>(std::strtoull(
					strResponse.substr(uiValueBegin, uiValueEnd - uiValueBegin).c_str(), nullptr, 10));
				if (strResponse.size() >= uiHeaderEnd + 4 + uiBodyLength)
				{
					break;
				}
			}
		}
		closesocket(hSocket);
		return strResponse;
	}

	std::string BuildGetRequest(const std::string& p_refStrTarget)
	{
		return "GET " + p_refStrTarget + " HTTP/1.0\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
	}

	ST_HTTP_OBSERVATION WaitObservation(size_t p_uiExpectedCount)
	{
		std::unique_lock<std::mutex> lock(g_mtxObservation);
		const bool bReady = g_cvObservation.wait_for(lock, std::chrono::seconds(5), [p_uiExpectedCount]() {
			return g_vecObservations.size() >= p_uiExpectedCount;
		});
		Require(bReady, "wait HTTP callback");
		return g_vecObservations[p_uiExpectedCount - 1];
	}

	unsigned short StartHttpServer()
	{
		char szError[1024] = { 0 };
		for (unsigned short usPort = 39081; usPort < 39100; ++usPort)
		{
			if (g_pHttpServer->CreateHttpSock("127.0.0.1", usPort, 64 * 1024, 1024, 32,
				HttpNotify, 4, 1024, szError, nullptr))
			{
				return usPort;
			}
		}
		std::cerr << "FAILED: start HTTP server, error=" << szError << std::endl;
		std::exit(1);
	}
}

int main()
{
	WSADATA stWsaData = {};
	Require(0 == WSAStartup(MAKEWORD(2, 2), &stWsaData), "WSAStartup");
	Require(7U == GetSocketServerAbiVersion(), "SocketServer ABI 7");
	g_pHttpServer = CreateHttpSockInstance();
	Require(nullptr != g_pHttpServer, "create HTTP instance");
	const unsigned short usPort = StartHttpServer();

	const std::string strQuery = "key&empty=&dup=first&dup=second&encoded=a%26b%3Dc&plus=a+b&bad=%ZZ&utf=%E4%B8%AD";
	const std::string strBody = "{\"side\":\"buy\"}";
	std::cout << "query test: POST compatibility" << std::endl;
	std::ostringstream ossPost;
	ossPost << "POST /probe?" << strQuery << " HTTP/1.0\r\nHost: 127.0.0.1\r\nConnection: close\r\n"
		<< "Content-Type: application/json\r\nContent-Length: " << strBody.size() << "\r\n\r\n" << strBody;
	const std::string strPostResponse = SendRawHttp(usPort, ossPost.str());
	Require(std::string::npos != strPostResponse.find("200"), "POST query response status");
	const ST_HTTP_OBSERVATION stPost = WaitObservation(1);
	Require("/probe" == stPost.strUrl, "path excludes query");
	Require("/probe?" + strQuery == stPost.strRawUrl, "raw URL preserved");
	Require(strQuery == stPost.strQuery, "raw query preserved");
	Require("POST" == stPost.strMethod && strBody == stPost.strBody, "POST body and query coexist");
	Require("" == stPost.mapParams.at("key") && "" == stPost.mapParams.at("empty"), "empty query values");
	Require("first" == stPost.mapParams.at("dup"), "duplicate query keeps first value");
	Require("a&b=c" == stPost.mapParams.at("encoded"), "encoded delimiters");
	Require("a b" == stPost.mapParams.at("plus"), "plus converts to space");
	Require("%ZZ" == stPost.mapParams.at("bad"), "invalid percent remains literal");
	Require(std::string("\xE4\xB8\xAD") == stPost.mapParams.at("utf"), "UTF-8 percent decode");

	std::ostringstream oss128;
	std::cout << "query test: 128 parameter boundary" << std::endl;
	oss128 << "/probe?";
	for (int i = 0; i < 128; ++i)
	{
		if (i > 0)
		{
			oss128 << '&';
		}
		oss128 << 'p' << i << '=' << i;
	}
	const std::string str128Response = SendRawHttp(usPort, BuildGetRequest(oss128.str()));
	Require(std::string::npos != str128Response.find("200"), "128 query params accepted");
	const ST_HTTP_OBSERVATION st128 = WaitObservation(2);
	Require("0" == st128.mapParams.at("p0") && "127" == st128.mapParams.at("p127"), "128 parameter boundary");

	const size_t uiCallbackCountBeforeReject = g_vecObservations.size();
	std::cout << "query test: reject 129 parameters" << std::endl;
	const std::string str129Response = SendRawHttp(usPort, BuildGetRequest(oss128.str() + "&overflow=1"));
	std::cerr << "query test: 129 response bytes=" << str129Response.size() << ",head="
		<< str129Response.substr(0, 64) << std::endl;
	Require(std::string::npos != str129Response.find("400"), "129 query params rejected");
	Require(uiCallbackCountBeforeReject == g_vecObservations.size(), "rejected query bypasses business callback");

	std::cout << "query test: reject decoded NUL and oversized URL" << std::endl;
	const std::string strNulResponse = SendRawHttp(usPort, BuildGetRequest("/probe?value=%00"));
	Require(std::string::npos != strNulResponse.find("400"), "decoded NUL rejected");
	const std::string strLongResponse = SendRawHttp(usPort, BuildGetRequest("/probe?value=" + std::string(17 * 1024, 'a')));
	Require(std::string::npos != strLongResponse.find("400"), "oversized raw URL rejected");

	g_pHttpServer->StopHttpSock();
	DelHttpSockInstance(g_pHttpServer);
	WSACleanup();
	std::cout << "SocketServer HTTP query runtime checks passed" << std::endl;
	return 0;
}
