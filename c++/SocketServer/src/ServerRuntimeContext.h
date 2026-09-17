#ifndef H_SOCKET_SERVER_RUNTIME_CONTEXT
#define H_SOCKET_SERVER_RUNTIME_CONTEXT

#include "SocketServer.h"
#include "SocketInterface.h"
#include "HPSocket.h"
#include "Struct.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>

#include <pthread.h>

class CHttpAsynReqObj;
class CHttpServerListerNet;
class CHttpSockServerObj;
class CTcpServerListerNet;
class CWebServerListerNet;

// SocketServer 单实例的公共身份和监听状态；数据热路径只保存对象地址，不按名称查询。
class CSocketServerRuntimeIdentity
{
public:
	// 创建不可变协议类型、逻辑名称和进程内实例编号。
	CSocketServerRuntimeIdentity(EN_SOCKET_SERVER_TYPE p_enType,
		const std::string& p_refServiceName, std::uint64_t p_ullInstanceId);
	// 记录监听成功后的端点；调用方必须已完成底层 Start。
	void MarkStarted(const char* p_szBindIp, unsigned short p_usPort);
	// 清除监听状态；逻辑名称和实例编号保持不变。
	void MarkStopped();
	// 复制固定长度运行信息，不向调用方暴露内部字符串所有权。
	bool Snapshot(ST_SOCKET_SERVER_RUNTIME_INFO& p_refInfo) const;

private:
	mutable std::mutex m_clMutex;        // 保护启动状态和监听端点快照。
	EN_SOCKET_SERVER_TYPE m_enType;      // 工厂创建时绑定的协议类型。
	std::string m_strServiceName;        // 工厂创建时复制的逻辑服务名称。
	std::uint64_t m_ullInstanceId;       // 进程内单调生成且不复用的诊断编号。
	bool m_bStarted;                     // 底层监听已成功启动时为 true。
	std::string m_strBindIp;             // 最近一次成功监听的绑定 IP。
	unsigned short m_usPort;             // 最近一次成功监听的端口。
};

// 单个 TCP 实例的全部可变运行状态。
struct ST_TCP_SERVER_RUNTIME
{
	CSocketServerRuntimeIdentity clIdentity;       // 逻辑身份和只读监听状态。
	mutable std::mutex clLifecycleMutex;           // 串行化本实例启动、停止和运行参数快照。
	CHPThreadPoolPtr clThreadPool;                 // 本实例独占的回调线程池。
	std::atomic<bool> bServerStatus;               // 是否允许接收、发送和派发回调。
	CTcpServerListerNet* pListener;                // 本实例拥有的 Listener。
	ITcpServer* pPackServer;                       // 本实例拥有的 HP-Socket Server。
	TCP_NOTIFY_PROC pNotifyHandler;                // 调用方注册的 TCP 回调。
	CONNID ullTaskId;                              // 本实例任务序号。
	pthread_mutex_t mutexConnection;               // 保护连接表和本端关闭集合。
	std::map<CONNID, ClientData> mapClient;         // 本实例连接状态。
	std::set<CONNID> setLocalClosing;              // 本实例主动关闭中的连接。
	pthread_mutex_t mutexRequest;                  // 保护分包请求缓存。
	std::map<CONNID, ReqCacheData*> mapRequest;     // 本实例 TCP 分包缓存。
	pthread_mutex_t mutexTask;                     // 保护回调任务缓存。
	std::map<CONNID, NotifyTask*> mapTask;          // 本实例待派发任务。
	unsigned int uiSocketListenQueue;              // 启动前设置的 TCP listen 队列；0 使用 HPSocket 默认值。

	ST_TCP_SERVER_RUNTIME(const std::string& p_refServiceName,
		std::uint64_t p_ullInstanceId);
	~ST_TCP_SERVER_RUNTIME();
};

// 单个 WebSocket 连接的通知队列，同一连接只允许一个线程顺序派发回调。
struct ST_WEB_NOTIFY_QUEUE_V2
{
	std::deque<NotifyTask*> deqTasks; // 按接收顺序保存尚未派发的通知。
	bool bDraining;                   // 当前是否已有线程负责排空该连接队列。
	bool bClosed;                     // 连接关闭后拒绝继续追加普通通知。

	ST_WEB_NOTIFY_QUEUE_V2();
};

// 单连接发送票据，网络控制帧与业务帧共用同一发送顺序。
struct ST_WEB_SEND_ORDER
{
	std::mutex clMutex;                  // 保护票据和关闭状态。
	std::condition_variable clCondition; // 前序发送完成后唤醒下一票据。
	unsigned long long ullNextTicket;    // 下一次分配的发送票据。
	unsigned long long ullServingTicket; // 当前允许执行的发送票据。
	bool bClosing;                       // 已提交关闭发送，不再接纳普通发送。
	bool bConnectionClosed;              // HP-Socket 已报告连接关闭。

	ST_WEB_SEND_ORDER();
};

// 单个 WebSocket 实例的全部可变运行状态。
struct ST_WEB_SERVER_RUNTIME
{
	CSocketServerRuntimeIdentity clIdentity;       // 逻辑身份和只读监听状态。
	mutable std::mutex clLifecycleMutex;           // 串行化本实例启动、停止和运行参数快照。
	CHPThreadPoolPtr clThreadPool;                 // 本实例独占的通知线程池。
	std::atomic<bool> bServerStatus;               // 是否允许连接和通知处理。
	CWebServerListerNet* pListener;                // 本实例拥有的 Web Listener。
	IHttpServer* pPackServer;                      // 本实例拥有的 HTTP/WebSocket Server。
	WEB_NOTIFY_PROC pNotifyHandler;                // 调用方注册的 WebSocket 回调。
	CONNID ullTaskId;                              // 本实例通知任务序号。
	pthread_mutex_t mutexConnection;               // 保护连接表和关闭集合。
	std::map<CONNID, ClientData> mapClient;         // 本实例连接状态。
	std::set<CONNID> setLocalClosing;              // 本实例主动关闭中的连接。
	pthread_mutex_t mutexRequest;                  // 保护 WebSocket 分帧缓存。
	std::map<CONNID, ReqCacheData*> mapRequest;     // 本实例消息分帧缓存。
	pthread_mutex_t mutexTask;                     // 保护按连接通知队列。
	std::map<CONNID, ST_WEB_NOTIFY_QUEUE_V2> mapNotifyQueue; // 本实例顺序通知队列。
	std::mutex clSendOrdersMutex;                  // 保护单连接发送票据表。
	std::map<CONNID, std::shared_ptr<ST_WEB_SEND_ORDER>> mapSendOrders; // 发送票据表。
	std::mutex clSendLifecycleMutex;               // 保护活动发送数和接纳闸门。
	std::condition_variable clSendLifecycleCondition; // 停服时等待活动发送退出。
	bool bSendAccepting;                           // 是否接纳新的发送调用。
	std::size_t szActiveSendCount;                 // 当前正在执行或等待票据的发送数。
	unsigned int uiSocketListenQueue;              // 启动前设置的 TCP listen 队列；0 使用 HPSocket 默认值。

	ST_WEB_SERVER_RUNTIME(const std::string& p_refServiceName,
		std::uint64_t p_ullInstanceId);
	~ST_WEB_SERVER_RUNTIME();
};

// 单个 HTTP 实例的全部可变运行状态。
struct ST_HTTP_SERVER_RUNTIME
{
	CSocketServerRuntimeIdentity clIdentity;       // 逻辑身份和只读监听状态。
	mutable std::mutex clLifecycleMutex;           // 串行化本实例启动、停止和运行参数快照。
	CHPThreadPoolPtr clThreadPool;                 // 本实例独占的请求回调线程池。
	std::atomic<bool> bServerStatus;               // 是否允许解析、派发和回包。
	CHttpServerListerNet* pListener;               // 本实例拥有的 HTTP Listener。
	IHttpServer* pPackServer;                      // 本实例拥有的 HTTP Server。
	CHttpSockServerObj* pOwner;                    // 请求完成时调用的所属 Server。
	HTTP_NOTIFY_PROC pNotifyHandler;               // 调用方注册的 HTTP 回调。
	unsigned long long ullAsyncRequestId;          // 本实例异步请求序号。
	pthread_mutex_t mutexRequest;                  // 保护全部 HTTP 请求状态表。
	std::map<unsigned long long, CHttpAsynReqObj*> mapRequest; // 异步请求号到请求对象。
	std::map<CONNID, unsigned long long> mapParsingRequest; // 连接当前解析中的请求。
	std::map<CONNID, unsigned long long> mapActiveRequest; // 已派发并等待响应的请求。
	unsigned int uiSocketListenQueue;              // 启动前设置的 TCP listen 队列；0 使用 HPSocket 默认值。

	ST_HTTP_SERVER_RUNTIME(const std::string& p_refServiceName,
		std::uint64_t p_ullInstanceId);
	~ST_HTTP_SERVER_RUNTIME();
};

#endif
