#include "ServerRuntimeContext.h"

#include "publicfunc.h"

#include <cstring>

namespace
{
	// 固定长度字段统一安全复制，运行信息不返回库内字符串指针。
	void CopyRuntimeText(char* p_szTarget, std::size_t p_szCapacity,
		const std::string& p_refValue)
	{
		if (p_szTarget == nullptr || p_szCapacity == 0)
		{
			return;
		}
		nsdk::SafeCopyCString(p_szTarget, p_szCapacity, p_refValue.c_str());
	}
}

CSocketServerRuntimeIdentity::CSocketServerRuntimeIdentity(
	EN_SOCKET_SERVER_TYPE p_enType, const std::string& p_refServiceName,
	std::uint64_t p_ullInstanceId)
	: m_clMutex()
	, m_enType(p_enType)
	, m_strServiceName(p_refServiceName)
	, m_ullInstanceId(p_ullInstanceId)
	, m_bStarted(false)
	, m_strBindIp()
	, m_usPort(0)
{
}

void CSocketServerRuntimeIdentity::MarkStarted(const char* p_szBindIp,
	unsigned short p_usPort)
{
	std::lock_guard<std::mutex> clLock(m_clMutex);
	m_strBindIp = p_szBindIp != nullptr ? p_szBindIp : "";
	m_usPort = p_usPort;
	m_bStarted = true;
}

void CSocketServerRuntimeIdentity::MarkStopped()
{
	std::lock_guard<std::mutex> clLock(m_clMutex);
	m_bStarted = false;
	m_strBindIp.clear();
	m_usPort = 0;
}

bool CSocketServerRuntimeIdentity::Snapshot(
	ST_SOCKET_SERVER_RUNTIME_INFO& p_refInfo) const
{
	std::lock_guard<std::mutex> clLock(m_clMutex);
	std::memset(&p_refInfo, 0, sizeof(p_refInfo));
	p_refInfo.uiStructSize = sizeof(p_refInfo);
	p_refInfo.uiAbiVersion = SOCKET_SERVER_ABI_VERSION;
	p_refInfo.enServerType = m_enType;
	p_refInfo.ullInstanceId = m_ullInstanceId;
	p_refInfo.bStarted = m_bStarted ? 1 : 0;
	CopyRuntimeText(p_refInfo.szServiceName,
		sizeof(p_refInfo.szServiceName), m_strServiceName);
	CopyRuntimeText(p_refInfo.szBindIp, sizeof(p_refInfo.szBindIp),
		m_strBindIp);
	p_refInfo.usPort = m_usPort;
	return true;
}

ST_TCP_SERVER_RUNTIME::ST_TCP_SERVER_RUNTIME(
	const std::string& p_refServiceName, std::uint64_t p_ullInstanceId)
	: clIdentity(EN_SOCKET_SERVER_TYPE_TCP, p_refServiceName, p_ullInstanceId)
	, clLifecycleMutex()
	, clThreadPool()
	, bServerStatus(false)
	, pListener(nullptr)
	, pPackServer(nullptr)
	, pNotifyHandler(nullptr)
	, ullTaskId(0)
	, mutexConnection()
	, mapClient()
	, setLocalClosing()
	, mutexRequest()
	, mapRequest()
	, mutexTask()
	, mapTask()
	, uiSocketListenQueue(0)
{
	pthread_mutex_init(&mutexConnection, nullptr);
	pthread_mutex_init(&mutexRequest, nullptr);
	pthread_mutex_init(&mutexTask, nullptr);
}

ST_TCP_SERVER_RUNTIME::~ST_TCP_SERVER_RUNTIME()
{
	pthread_mutex_destroy(&mutexTask);
	pthread_mutex_destroy(&mutexRequest);
	pthread_mutex_destroy(&mutexConnection);
}

ST_WEB_NOTIFY_QUEUE_V2::ST_WEB_NOTIFY_QUEUE_V2()
	: deqTasks()
	, bDraining(false)
	, bClosed(false)
{
}

ST_WEB_SEND_ORDER::ST_WEB_SEND_ORDER()
	: clMutex()
	, clCondition()
	, ullNextTicket(0)
	, ullServingTicket(0)
	, bClosing(false)
	, bConnectionClosed(false)
{
}

ST_WEB_SERVER_RUNTIME::ST_WEB_SERVER_RUNTIME(
	const std::string& p_refServiceName, std::uint64_t p_ullInstanceId)
	: clIdentity(EN_SOCKET_SERVER_TYPE_WEB, p_refServiceName, p_ullInstanceId)
	, clLifecycleMutex()
	, clThreadPool()
	, bServerStatus(false)
	, pListener(nullptr)
	, pPackServer(nullptr)
	, pNotifyHandler(nullptr)
	, ullTaskId(0)
	, mutexConnection()
	, mapClient()
	, setLocalClosing()
	, mutexRequest()
	, mapRequest()
	, mutexTask()
	, mapNotifyQueue()
	, clSendOrdersMutex()
	, mapSendOrders()
	, clSendLifecycleMutex()
	, clSendLifecycleCondition()
	, bSendAccepting(false)
	, szActiveSendCount(0)
	, uiSocketListenQueue(0)
{
	pthread_mutex_init(&mutexConnection, nullptr);
	pthread_mutex_init(&mutexRequest, nullptr);
	pthread_mutex_init(&mutexTask, nullptr);
}

ST_WEB_SERVER_RUNTIME::~ST_WEB_SERVER_RUNTIME()
{
	pthread_mutex_destroy(&mutexTask);
	pthread_mutex_destroy(&mutexRequest);
	pthread_mutex_destroy(&mutexConnection);
}

ST_HTTP_SERVER_RUNTIME::ST_HTTP_SERVER_RUNTIME(
	const std::string& p_refServiceName, std::uint64_t p_ullInstanceId)
	: clIdentity(EN_SOCKET_SERVER_TYPE_HTTP, p_refServiceName, p_ullInstanceId)
	, clLifecycleMutex()
	, clThreadPool()
	, bServerStatus(false)
	, pListener(nullptr)
	, pPackServer(nullptr)
	, pOwner(nullptr)
	, pNotifyHandler(nullptr)
	, ullAsyncRequestId(0)
	, mutexRequest()
	, mapRequest()
	, mapParsingRequest()
	, mapActiveRequest()
	, uiSocketListenQueue(0)
{
	pthread_mutex_init(&mutexRequest, nullptr);
}

ST_HTTP_SERVER_RUNTIME::~ST_HTTP_SERVER_RUNTIME()
{
	pthread_mutex_destroy(&mutexRequest);
}
