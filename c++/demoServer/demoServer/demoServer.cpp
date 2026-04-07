// demoServer.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <string>

#include <windows.h>

#include "SocketServer.h"

using namespace std;

#define HTTP_DLL_NAME		"libSocketServer.dll"

CSocketServer *g_pWebServerHandle = NULL;

//web日志
string g_strWebLogPath = "F:\\开发资料\\库平台库编译\\demoServer\\x64\\Release";
string g_strTcpLogPath = "F:\\开发资料\\库平台库编译\\demoServer\\x64\\Release";

//启动
typedef CSocketServer *(*pfnCreateWebSockInstance)();

//释放
typedef void(*pfnDelWebSockInstance)(CSocketServer *&);

pfnDelWebSockInstance g_fnDelWebSockInstance = NULL;

bool g_bStatus = false;
HMODULE g_hModule = NULL;

bool InitWebServerInfo(const char* p_sHomePath)
{
	if (NULL == p_sHomePath || strlen(p_sHomePath) == 0)
	{
		printf("[WEB服务] 路径是空");
		return false;
	}
	string strDllPath = p_sHomePath;
	strDllPath.append("\\");
	strDllPath.append(HTTP_DLL_NAME);

	//TODO 读取websocket服务开关

	//TODO 读取websocket服务日志路径
	DWORD attr = ::GetFileAttributes(strDllPath.c_str());
	if (INVALID_FILE_ATTRIBUTES == attr || 0 != (attr & FILE_ATTRIBUTE_DIRECTORY))
	{
		printf("[WEB服务] 动态库文件不存在[%s]...", strDllPath.c_str());
		return FALSE;
	}

	//加载动态库
	if (!(g_hModule = LoadLibrary(strDllPath.c_str())))
	{
		DWORD dwError = GetLastError();
		printf("[WEB服务] 动态库加载失败[%s][%d]...", strDllPath.c_str(), dwError);
		return false;
	}

	// 创建函数
	pfnCreateWebSockInstance fnCreateWebSockInstance = NULL;

	fnCreateWebSockInstance = (pfnCreateWebSockInstance)GetProcAddress(g_hModule, "CreateWebSockInstance");
	if (NULL == fnCreateWebSockInstance)
	{
		FreeLibrary(g_hModule);
		printf("[WEB服务] 获取方法[WebSockIns]失败...");
		return false;
	}

	if (NULL == g_pWebServerHandle)
	{
		g_pWebServerHandle = fnCreateWebSockInstance();
		if (NULL == g_pWebServerHandle)
		{
			FreeLibrary(g_hModule);
			printf("[WEB服务] 获取监控方法失败");
			return false;
		}
	}

	g_fnDelWebSockInstance = (pfnDelWebSockInstance)GetProcAddress(g_hModule, "DelWebSockInstance");
	if (NULL == g_fnDelWebSockInstance)
	{
		FreeLibrary(g_hModule);
		printf("[WEB服务] 获取方法[DelWebSockIns]失败...");
		return false;
	}

	//TODO 初始化日志
	g_bStatus = true;

	printf("[WEB服务] 初始化状态: %d\n", g_bStatus);

	return g_bStatus;
}

void WebNotifyHandle(void *p_refServerHandle, void *p_refClinetHandle, WebSockNotifyType p_enType,
	const void *p_szData, int p_iDataLen, const char *p_szClientIp, unsigned short p_nClientPort, void *p_szErrData)
{
	if ((enWebConnect != p_enType && enWebClose != p_enType) && (NULL == p_szData || 0 == p_iDataLen) )
	{
		printf("数据或长度为空[%d][%d]数据类型=[%d]\n", NULL == p_szData, p_iDataLen, p_enType);
		return;
	}

	unsigned long long ulUnDoReqNum = 0;
	short nErrCode = -1;

	HS h = (HS)p_refServerHandle;
	HCLIENT user = (HCLIENT)p_refClinetHandle;

	printf("上层应用收到内容:type=%d,datalen=%d,client=%llu,ip=%s,port=%d\r\n",
		p_enType, p_iDataLen, (unsigned __int64 *)p_refClinetHandle, p_szClientIp, p_nClientPort);

	switch (p_enType)
	{
	case enWebClose:
	{
		//不需要再关闭客户端，内部已关闭
		printf("上层应用收到: 断开通知，[%p,%llu],ip=%s,port=%d, 信息=%s",
			p_refServerHandle, (unsigned __int64 *)p_refClinetHandle, p_szClientIp, p_nClientPort, (char *)p_szErrData);
	}
	break;
	case enWebConnect:
	{
		printf("上层应用收到: 连接通知, ip=%s,port=%d,msg=%s\n", p_szClientIp, p_nClientPort, (char *)p_szErrData);
	}
	break;
	case enWebError:
	{
		//连接没关闭
		printf("上层应用收到: 错误通知, ip=%s,port=%d,err=%s\n", p_szClientIp, p_nClientPort, (char *)p_szErrData);

	}
	break;
	case enWebData:
	{
		printf("上层应用收到: 数据通知，数据=%s,长度=%d,ip=%s,port=%d\n",
			(char *)p_szData, p_iDataLen, p_szClientIp, p_nClientPort);
	}
	break;
	default:
		break;
	}
}

bool IniTcpServerInfo(const char* p_sHomePath)
{
	if (NULL == p_sHomePath || strlen(p_sHomePath) == 0)
	{
		printf("[TCP服务] 路径是空");
		return false;
	}
	string strDllPath = p_sHomePath;
	strDllPath.append("\\");
	strDllPath.append(HTTP_DLL_NAME);

	//TODO 读取websocket服务开关

	//TODO 读取websocket服务日志路径
	DWORD attr = ::GetFileAttributes(strDllPath.c_str());
	if (INVALID_FILE_ATTRIBUTES == attr || 0 != (attr & FILE_ATTRIBUTE_DIRECTORY))
	{
		printf("[TCP服务] 动态库文件不存在[%s]...", strDllPath.c_str());
		return FALSE;
	}

	//加载动态库
	if (!(g_hModule = LoadLibrary(strDllPath.c_str())))
	{
		DWORD dwError = GetLastError();
		printf("[TCP服务] 动态库加载失败[%s][%d]...", strDllPath.c_str(), dwError);
		return false;
	}

	// 创建函数
	pfnCreateWebSockInstance fnCreateWebSockInstance = NULL;

	fnCreateWebSockInstance = (pfnCreateWebSockInstance)GetProcAddress(g_hModule, "CreateTcpSockInstance");
	if (NULL == fnCreateWebSockInstance)
	{
		FreeLibrary(g_hModule);
		printf("[TCP服务] 获取方法[WebSockIns]失败...");
		return false;
	}

	if (NULL == g_pWebServerHandle)
	{
		g_pWebServerHandle = fnCreateWebSockInstance();
		if (NULL == g_pWebServerHandle)
		{
			FreeLibrary(g_hModule);
			printf("[TCP服务] 获取监控方法失败");
			return false;
		}
	}

	g_fnDelWebSockInstance = (pfnDelWebSockInstance)GetProcAddress(g_hModule, "DelWebSockInstance");
	if (NULL == g_fnDelWebSockInstance)
	{
		FreeLibrary(g_hModule);
		printf("[TCP服务] 获取方法[DelWebSockIns]失败...");
		return false;
	}

	//TODO 初始化日志
	g_bStatus = true;

	printf("[TCP服务] 初始化状态: %d\n", g_bStatus);

	return g_bStatus;
}

void TestWebSock()
{
	bool bFlag = InitWebServerInfo(g_strWebLogPath.c_str());

	if (bFlag)
	{
		//启动服务
		int iThreadNum = 0;
		int iQueueNum = 1024 * 4;
		int iRBufLen = 1024 * 1024;
		int iMaxConnectNum = 300;
		int iMaxAcceptNum = 1024 * 4;
		char szBuf[1024] = { 0 };
		char *pLogFold = (char *)g_strWebLogPath.data();

		/*if (g_fnDelWebSockInstance && g_pWebServerHandle)
		{
		g_fnDelWebSockInstance(g_pWebServerHandle);
		printf("[WEB服务] 停止ws服务成功\n");
		return 0;
		}*/

		if (!g_pWebServerHandle->CreateWebSock("0.0.0.0", 8898, iRBufLen, iMaxConnectNum, iMaxAcceptNum,
			WebNotifyHandle, iThreadNum, iQueueNum, szBuf, pLogFold))
		{
			printf("[WEB服务] 创建ws服务失败: %s\n", szBuf);
		}
		else
		{
			printf("[WEB服务] 创建ws服务成功---\n");
		}
	}
}

void TestTcpSock()
{
	bool bFlag = InitWebServerInfo(g_strWebLogPath.c_str());

	if (bFlag)
	{
		//启动服务
		int iThreadNum = 0;
		int iQueueNum = 1024 * 4;
		int iRBufLen = 1024 * 1024;
		int iMaxConnectNum = 300;
		int iMaxAcceptNum = 1024 * 4;
		char szBuf[1024] = { 0 };
		char *pLogFold = (char *)g_strWebLogPath.c_str();

		/*if (g_fnDelWebSockInstance && g_pWebServerHandle)
		{
		g_fnDelWebSockInstance(g_pWebServerHandle);
		printf("[WEB服务] 停止ws服务成功\n");
		return 0;
		}*/

		if (!g_pWebServerHandle->CreateWebSock("0.0.0.0", 8898, iRBufLen, iMaxConnectNum, iMaxAcceptNum,
			WebNotifyHandle, iThreadNum, iQueueNum, szBuf, pLogFold))
		{
			printf("[TCP服务] 创建ws服务失败: %s\n", szBuf);
		}
		else
		{
			printf("[TCP服务] 创建ws服务成功---\n");
		}
	}
}

int main()
{
	TestWebSock();
	system("pause");
    return 0;
}

