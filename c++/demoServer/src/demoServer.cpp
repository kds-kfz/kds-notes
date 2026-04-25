// demoServer.cpp : 定义控制台应用程序的入口点。

#include <windows.h>
#include "HttpSockMng.h"
#include "WebSockMng.h"
#include "Log.h"
#include "nsdk.h"

//服务库路径
std::string g_strDllPath = "F:\\开发资料\\MyCode\\kds-notes\\c++\\demoServer\\bin\\x64vc14";

//Mt服务日志
std::string g_strMtLogPath = "F:\\开发资料\\MyCode\\kds-notes\\c++\\demoServer\\bin\\x64vc14\\MtLog";

//Web/Http服务日志
std::string g_strWebLogPath = "F:\\开发资料\\MyCode\\kds-notes\\c++\\demoServer\\bin\\x64vc14\\WebLog";
std::string g_strHttpLogPath = "F:\\开发资料\\MyCode\\kds-notes\\c++\\demoServer\\bin\\x64vc14\\HttpLog";

//Web/Http服务网络层日志
std::string g_strWebNetLogPath = "F:\\开发资料\\MyCode\\kds-notes\\c++\\demoServer\\bin\\x64vc14\\WebNetLog";
std::string g_strHttpNetLogPath = "F:\\开发资料\\MyCode\\kds-notes\\c++\\demoServer\\bin\\x64vc14\\HttpNetLog";

int InitLog()
{
	//服务日志初始化
	int iRet = CLog::GetInstance()->InitLog(g_strMtLogPath.c_str());
	if (MA_OK == iRet)
	{
		CLog::GetInstance()->Resume();//恢复工作
		MT_INFO("启动LOG  ****************************");
		CLog::GetInstance()->SetLogLevel((char*)"info");
	}

	iRet = CHttpLog::GetInstance()->InitLog(g_strHttpLogPath.c_str());
	if (MA_OK == iRet)
	{
		CHttpLog::GetInstance()->Resume();//恢复工作
		HTTP_INFO("启动LOG  ****************************");
		CHttpLog::GetInstance()->SetLogLevel((char*)"info");
	}

	iRet = CWebLog::GetInstance()->InitLog(g_strWebLogPath.c_str());
	if (MA_OK == iRet)
	{
		CWebLog::GetInstance()->Resume();//恢复工作
		WEB_INFO("启动LOG  ****************************");
		CWebLog::GetInstance()->SetLogLevel((char*)"info");
	}

	return 0;
}

int InitAllServer()
{
	const char* szHttpIp = "0.0.0.0";
	unsigned short usHttpPort = 8897;
	unsigned short usWebPort = 8898;

	//http服务初始化
	bool bRet = false;
	

	//web服务初始化
	if (CWebSockMng::GetInstance()->InitWebServerInfo(g_strDllPath.c_str()))
	{
		bRet = CWebSockMng::GetInstance()->Start(
			szHttpIp, usWebPort, 0, 0,
			0, 0, 0, false,
			nullptr, nullptr, nullptr, nullptr, (char *)g_strWebNetLogPath.c_str());

		if (!bRet)
			return -2;
	}

	if (CHttpSockMng::GetInstance()->InitHttpServerInfo(g_strDllPath.c_str()))
	{
		bRet = CHttpSockMng::GetInstance()->Start(
			szHttpIp, usHttpPort, 0, 0,
			0, 0, 0, false,
			nullptr, nullptr, nullptr, nullptr, (char*)g_strHttpNetLogPath.c_str());

		if (!bRet)
			return -1;
	}

	return 0;
}

int main()
{
	

	//日志初始化
	//InitLog();

	//服务初始化
	//InitAllServer();

	//测试加密模块
	string strClientInfo = "10086";//userid
	string strAseClientInfo = nsdk::MakeFeatrue(strClientInfo);
	string strClientToken = "";
	//授权获取Token
	int iRet = nsdk::BuildFeatrue(strAseClientInfo, strClientInfo.length(), -1, strClientToken);
	//验证Token
	string strSrcClientToken = "";
	iRet = nsdk::AuthFeatrue(strClientInfo, strClientToken, strSrcClientToken);
	system("pause");
	return 0;
}


