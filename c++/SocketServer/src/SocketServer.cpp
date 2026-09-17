#include "publicGlobalvar.h"

#include <atomic>
#include <cctype>
#include <cstring>
#include <map>
#include <mutex>
#include <new>
#include <string>

namespace
{
	// 工厂登记项只参与创建、删除和诊断，不进入网络数据热路径。
	struct ST_SOCKET_SERVER_FACTORY_ENTRY
	{
		EN_SOCKET_SERVER_TYPE enType; // 实例协议类型，与登记键保持一致。
		std::string strServiceName;    // 已校验并复制的逻辑名称。
		CSocketServer* pInstance;      // 动态库拥有的服务实例。
	};

	std::mutex s_clFactoryMutex; // 串行化工厂登记表变更和运行信息查询。
	std::map<std::pair<int, std::string>, CSocketServer*> s_mapFactoryByName;
	std::map<CSocketServer*, ST_SOCKET_SERVER_FACTORY_ENTRY> s_mapFactoryByPointer;
	std::atomic<unsigned long long> s_ullNextInstanceId(1);

	// 逻辑名称只接受稳定 ASCII 标识符，避免控制字符和路径内容进入日志。
	bool IsValidServiceName(const char* p_szServiceName)
	{
		if (p_szServiceName == nullptr)
		{
			return false;
		}
		const std::size_t szLength = std::strlen(p_szServiceName);
		if (szLength == 0 || szLength >= 64U)
		{
			return false;
		}
		for (std::size_t szIndex = 0; szIndex < szLength; ++szIndex)
		{
			const unsigned char byValue = static_cast<unsigned char>(p_szServiceName[szIndex]);
			if (!std::isalnum(byValue) && byValue != '.' && byValue != '_' && byValue != '-')
			{
				return false;
			}
		}
		return true;
	}

	// 创建具体协议对象并登记唯一名称；失败时不留下半注册实例。
	CSocketServer* CreateNamedInstance(EN_SOCKET_SERVER_TYPE p_enType,
		const char* p_szServiceName)
	{
		if (!IsValidServiceName(p_szServiceName))
		{
			return nullptr;
		}
		const std::string strServiceName(p_szServiceName);
		const std::pair<int, std::string> clKey(static_cast<int>(p_enType), strServiceName);
		std::lock_guard<std::mutex> clLock(s_clFactoryMutex);
		if (s_mapFactoryByName.find(clKey) != s_mapFactoryByName.end())
		{
			return nullptr;
		}

		const unsigned long long ullInstanceId = s_ullNextInstanceId.fetch_add(1);
		CSocketServer* pInstance = nullptr;
		try
		{
			switch (p_enType)
			{
			case EN_SOCKET_SERVER_TYPE_HTTP:
				pInstance = new (std::nothrow) CHttpSockServerObj(strServiceName, ullInstanceId);
				break;
			case EN_SOCKET_SERVER_TYPE_WEB:
				pInstance = new (std::nothrow) CWebSockServerObj(strServiceName, ullInstanceId);
				break;
			case EN_SOCKET_SERVER_TYPE_TCP:
				pInstance = new (std::nothrow) CTcpSockServerObj(strServiceName, ullInstanceId);
				break;
			default:
				return nullptr;
			}
			if (pInstance == nullptr)
			{
				return nullptr;
			}
			ST_SOCKET_SERVER_FACTORY_ENTRY stEntry;
			stEntry.enType = p_enType;
			stEntry.strServiceName = strServiceName;
			stEntry.pInstance = pInstance;
			s_mapFactoryByName.insert(std::make_pair(clKey, pInstance));
			s_mapFactoryByPointer.insert(std::make_pair(pInstance, stEntry));
		}
		catch (...)
		{
			switch (p_enType)
			{
			case EN_SOCKET_SERVER_TYPE_HTTP:
				delete static_cast<CHttpSockServerObj*>(pInstance);
				break;
			case EN_SOCKET_SERVER_TYPE_WEB:
				delete static_cast<CWebSockServerObj*>(pInstance);
				break;
			case EN_SOCKET_SERVER_TYPE_TCP:
				delete static_cast<CTcpSockServerObj*>(pInstance);
				break;
			default:
				break;
			}
			return nullptr;
		}
		return pInstance;
	}

	// 只允许对应协议释放工厂登记对象，防止错误 Delete 函数删除其他类型实例。
	void DeleteNamedInstance(EN_SOCKET_SERVER_TYPE p_enType,
		CSocketServer*& p_refInstance)
	{
		if (p_refInstance == nullptr)
		{
			return;
		}
		CSocketServer* pDeleteInstance = nullptr;
		{
			std::lock_guard<std::mutex> clLock(s_clFactoryMutex);
			auto itEntry = s_mapFactoryByPointer.find(p_refInstance);
			if (itEntry == s_mapFactoryByPointer.end() || itEntry->second.enType != p_enType)
			{
				return;
			}
			const std::pair<int, std::string> clKey(
				static_cast<int>(p_enType), itEntry->second.strServiceName);
			pDeleteInstance = itEntry->second.pInstance;
			s_mapFactoryByName.erase(clKey);
			s_mapFactoryByPointer.erase(itEntry);
			p_refInstance = nullptr;
		}
		switch (p_enType)
		{
		case EN_SOCKET_SERVER_TYPE_HTTP:
			delete static_cast<CHttpSockServerObj*>(pDeleteInstance);
			break;
		case EN_SOCKET_SERVER_TYPE_WEB:
			delete static_cast<CWebSockServerObj*>(pDeleteInstance);
			break;
		case EN_SOCKET_SERVER_TYPE_TCP:
			delete static_cast<CTcpSockServerObj*>(pDeleteInstance);
			break;
		default:
			break;
		}
	}
}

// 返回当前 SocketServer 公共接口 ABI 版本，避免调用方加载不匹配的动态库。
unsigned int GetSocketServerAbiVersion()
{
	return SOCKET_SERVER_ABI_VERSION;
}

bool GetSocketServerRuntimeInfo(CSocketServer* p_pInstance,
	ST_SOCKET_SERVER_RUNTIME_INFO* p_pInfo)
{
	if (p_pInstance == nullptr || p_pInfo == nullptr ||
		p_pInfo->uiStructSize != sizeof(ST_SOCKET_SERVER_RUNTIME_INFO))
	{
		return false;
	}
	std::lock_guard<std::mutex> clLock(s_clFactoryMutex);
	auto itEntry = s_mapFactoryByPointer.find(p_pInstance);
	if (itEntry == s_mapFactoryByPointer.end())
	{
		return false;
	}
	switch (itEntry->second.enType)
	{
	case EN_SOCKET_SERVER_TYPE_HTTP:
		return static_cast<CHttpSockServerObj*>(p_pInstance)->FillRuntimeInfo(*p_pInfo);
	case EN_SOCKET_SERVER_TYPE_WEB:
		return static_cast<CWebSockServerObj*>(p_pInstance)->FillRuntimeInfo(*p_pInfo);
	case EN_SOCKET_SERVER_TYPE_TCP:
		return static_cast<CTcpSockServerObj*>(p_pInstance)->FillRuntimeInfo(*p_pInfo);
	default:
		return false;
	}
}

/********** TCP服务模块 **********/
CSocketServer* CreateTcpSockInstance()
{
	return CreateTcpSockInstanceByName("1");
}

CSocketServer* CreateTcpSockInstanceByName(const char* p_szServiceName)
{
	return CreateNamedInstance(EN_SOCKET_SERVER_TYPE_TCP, p_szServiceName);
}

void DelTcpSockInstance(CSocketServer *&pIns)
{
	DeleteNamedInstance(EN_SOCKET_SERVER_TYPE_TCP, pIns);
}

/********** HTTP服务模块 **********/
CSocketServer* CreateHttpSockInstance()
{
	return CreateHttpSockInstanceByName("1");
}

CSocketServer* CreateHttpSockInstanceByName(const char* p_szServiceName)
{
	return CreateNamedInstance(EN_SOCKET_SERVER_TYPE_HTTP, p_szServiceName);
}

void DelHttpSockInstance(CSocketServer *&pIns)
{
	DeleteNamedInstance(EN_SOCKET_SERVER_TYPE_HTTP, pIns);
}

/********** WEBSOCKET服务模块 **********/
CSocketServer* CreateWebSockInstance()
{
	return CreateWebSockInstanceByName("1");
}

CSocketServer* CreateWebSockInstanceByName(const char* p_szServiceName)
{
	return CreateNamedInstance(EN_SOCKET_SERVER_TYPE_WEB, p_szServiceName);
}

void DelWebSockInstance(CSocketServer *&pIns)
{
	DeleteNamedInstance(EN_SOCKET_SERVER_TYPE_WEB, pIns);
}


