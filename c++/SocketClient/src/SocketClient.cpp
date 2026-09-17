#include "SocketClient.h"
#include "ClientInstance.h"
#include "ClientProtocol.h"
#include "TcpSockClientObj.h"
#include "HttpSockClientObj.h"
#include "WebSockClientObj.h"
#include <atomic>
#include <map>
#include <mutex>
#include <new>
#include <string>
#include <utility>

namespace
{
// 工厂注册表仅负责创建、诊断和删除，不参与网络收发热路径。
std::mutex registryMutex;
std::map<std::pair<int, std::string>, ClientInstance*> byName;
std::map<CSocketClient*, std::pair<int, std::string>> byPointer;
std::atomic<unsigned long long> nextId(1);

// 以协议类型与名称组成唯一键，失败不登记半初始化对象。
CSocketClient* Create(EN_SOCKET_CLIENT_TYPE p_enType, const char* p_szName)
{
    if (!ClientProtocol::ValidName(p_szName)) return nullptr;
    const std::pair<int, std::string> key(p_enType, p_szName);
    std::lock_guard<std::mutex> lock(registryMutex);
    if (byName.count(key)) return nullptr;
    ClientInstance* instance = nullptr;
    try
    {
        const unsigned long long id = nextId.fetch_add(1);
        switch (p_enType)
        {
        case EN_SOCKET_CLIENT_TCP:
            instance = new (std::nothrow) CTcpSockClientObj(p_szName, id);
            break;
        case EN_SOCKET_CLIENT_HTTP:
            instance = new (std::nothrow) CHttpSockClientObj(p_szName, id);
            break;
        case EN_SOCKET_CLIENT_WEB:
            instance = new (std::nothrow) CWebSockClientObj(p_szName, id);
            break;
        default:
            return nullptr;
        }
        if (!instance || !instance->Ready()) { delete instance; return nullptr; }
        byName.emplace(key, instance);
        byPointer.emplace(instance, key);
        return instance;
    }
    catch (...)
    {
        if (instance)
        {
            byName.erase(key);
            byPointer.erase(instance);
            delete instance;
        }
        return nullptr;
    }
}

// 先从工厂注销，再在锁外停止连接及排空本实例的底层回调。
void Destroy(EN_SOCKET_CLIENT_TYPE p_enType, CSocketClient*& p_refClient)
{
    if (!p_refClient) return;
    ClientInstance* instance;
    {
        std::lock_guard<std::mutex> lock(registryMutex);
        const auto it = byPointer.find(p_refClient);
        if (it == byPointer.end() || it->second.first != p_enType) return;
        instance = static_cast<ClientInstance*>(p_refClient);
        byName.erase(it->second);
        byPointer.erase(it);
        p_refClient = nullptr;
    }
    delete instance;
}
}

// 返回独立于服务端库的客户端 ABI 版本。
unsigned int GetSocketClientAbiVersion() { return SOCKET_CLIENT_ABI_VERSION; }

// 不解引用未登记的指针，拒绝错误大小的诊断结构体。
bool GetSocketClientRuntimeInfo(CSocketClient* p_pClient,
    ST_SOCKET_CLIENT_RUNTIME_INFO* p_pInfo)
{
    if (!p_pClient || !p_pInfo || p_pInfo->uiStructSize != sizeof(*p_pInfo)) return false;
    std::lock_guard<std::mutex> lock(registryMutex);
    if (!byPointer.count(p_pClient)) return false;
    static_cast<ClientInstance*>(p_pClient)->Snapshot(*p_pInfo);
    return true;
}

// 创建一条独立的 TCP 连接实例。
CSocketClient* CreateTcpClientInstanceByName(const char* p_szName)
{ return Create(EN_SOCKET_CLIENT_TCP, p_szName); }

// 创建一条独立的 HTTP 连接实例。
CSocketClient* CreateHttpClientInstanceByName(const char* p_szName)
{ return Create(EN_SOCKET_CLIENT_HTTP, p_szName); }

// 创建一条独立的 WebSocket 连接实例。
CSocketClient* CreateWebClientInstanceByName(const char* p_szName)
{ return Create(EN_SOCKET_CLIENT_WEB, p_szName); }

// 仅删除类型匹配的 TCP 工厂实例。
void DelTcpClientInstance(CSocketClient*& p_pClient)
{ Destroy(EN_SOCKET_CLIENT_TCP, p_pClient); }

// 仅删除类型匹配的 HTTP 工厂实例。
void DelHttpClientInstance(CSocketClient*& p_pClient)
{ Destroy(EN_SOCKET_CLIENT_HTTP, p_pClient); }

// 仅删除类型匹配的 WebSocket 工厂实例。
void DelWebClientInstance(CSocketClient*& p_pClient)
{ Destroy(EN_SOCKET_CLIENT_WEB, p_pClient); }
