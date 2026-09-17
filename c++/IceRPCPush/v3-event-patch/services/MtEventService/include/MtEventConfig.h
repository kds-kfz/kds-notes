#ifndef H_TRADING_TERMINAL_MT_EVENT_CONFIG
#define H_TRADING_TERMINAL_MT_EVENT_CONFIG

#include <cstddef>
#include <cstdint>
#include <string>

// MtEventService 行情中继配置。配置只控制传输治理，不保存行情业务规则。
struct ST_MT_MARKET_EVENT_CONFIG
{
	unsigned int uiWorkerThreads;      // 行情中继分片工作线程数，同一 Version+No 固定进入同一分片。
	std::size_t szQueueCapacity;       // 0 不主动容量淘汰；正数 128-10000000 时满载淘汰最旧 Tick。
	unsigned int uiDropLogIntervalMs;  // 队列淘汰和发布失败日志的最小间隔毫秒数。

	ST_MT_MARKET_EVENT_CONFIG();
};

// Event 实例间广播配置。广播只复制已经完成业务协议校验的通知，不提供持久化语义。
struct ST_MT_EVENT_BROADCAST_CONFIG
{
	bool bEnable;                       // 是否通过 Core NATS 把通知广播给全部 Event 实例。
	std::string strSubject;             // 所有 Event 实例共同订阅且不使用 queue group 的 Subject。
	std::size_t szMaxPayloadBytes;      // 单条业务正文上限，防止异常消息耗尽进程内存。
	std::size_t szDedupeCapacity;       // 进程内确定性事件 ID 去重窗口容量。
	unsigned int uiPendingMessages;     // 当前订阅允许积压的最大消息数。
	unsigned int uiPendingBytes;        // 当前订阅允许积压的最大字节数。

	ST_MT_EVENT_BROADCAST_CONFIG();
};

// NATS JetStream 可靠事件配置。URL 中不得携带明文生产凭据，优先使用环境变量。
struct ST_MT_RELIABLE_EVENT_CONFIG
{
	bool bEnable;                       // 是否启用 1172-1175 可靠事件中心。
	std::string strUrl;                 // NATS 服务地址，例如 nats://127.0.0.1:4222。
	std::string strStream;              // JetStream Stream 名称。
	std::string strSubjectPrefix;       // 交易事件 Subject 前缀。
	std::int64_t llMaxAgeSeconds;       // Stream 最大保留秒数。
	std::int64_t llMaxBytes;            // Stream 磁盘容量上限。
	std::int64_t llDuplicateWindowSeconds; // EventId 服务端去重窗口秒数。
	unsigned int uiReplicas;           // JetStream Stream 副本数；第一阶段单节点固定为 1。
	unsigned int uiFetchBatchMax;       // 单次 1173 最大拉取数量。
	unsigned int uiFetchWaitMs;         // 单次 JetStream Pull 等待毫秒数。
	std::uint32_t uiMaxDeliver;         // 消费者最大重投次数。

	ST_MT_RELIABLE_EVENT_CONFIG();
};

// MtEventService 完整启动期配置，加载后由 App 和两个隔离引擎只读。
struct ST_MT_EVENT_SERVICE_CONFIG
{
	ST_MT_MARKET_EVENT_CONFIG stMarketEvent;   // Best Effort 行情中继配置。
	ST_MT_EVENT_BROADCAST_CONFIG stBroadcast;  // Event 实例间 Core NATS 广播配置。
	ST_MT_RELIABLE_EVENT_CONFIG stReliableEvent; // Reliable JetStream 配置。

	ST_MT_EVENT_SERVICE_CONFIG();
};

// 从 MtEventService.xml 读取 MarketEvent 和 ReliableEvent；非法值返回详细英文错误。
bool LoadMtEventServiceConfig(const std::string& p_refConfigPath,
	ST_MT_EVENT_SERVICE_CONFIG& p_refConfig,
	std::string& p_refError);

#endif // H_TRADING_TERMINAL_MT_EVENT_CONFIG
