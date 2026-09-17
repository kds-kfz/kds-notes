#ifndef H_TRADING_TERMINAL_MT_RELIABLE_EVENT_STORE
#define H_TRADING_TERMINAL_MT_RELIABLE_EVENT_STORE

#include "MtEventConfig.h"
#include "PluginBinaryProtocol.h"
#include "TradingTerminalProtocol.h"

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <vector>

typedef struct __natsConnection natsConnection;
typedef struct __natsSubscription natsSubscription;
typedef struct __natsMsg natsMsg;
typedef struct __jsCtx jsCtx;

// 1172 追加成功后需要在线扇出的事件信息。
// JetStream 去重命中时 bShouldPublish 为 false，避免重复在线通知。
struct ST_MT_RELIABLE_APPEND_EVENT
{
	bool bShouldPublish;                     // 本次是否首次持久化并需要在线发布。
	EN_PLUGIN_NOTIFY_ID enNotifyId;          // 122x/124x 通知号。
	EN_PLUGIN_NOTIFY_ACTION enAction;        // 事件新增、修改或删除动作。
	std::uint64_t ullSourceSequence;         // 交易源单调序号。
	std::int64_t llCreatedTimeMs;            // 交易服务生成事件的 Unix 毫秒时间。
	std::vector<unsigned char> aEnvelope;    // 完整 Reliable Event Binary 信封。
	std::vector<unsigned char> aOnlinePayload; // 首次在线扇出的 ClientData 正文；1244 保留完整可靠信封。

	ST_MT_RELIABLE_APPEND_EVENT();
};

// JetStream 可靠事件封装，只允许 MtEventService 持有 NATS 连接和消息句柄。
// 请求线程可并发发布和拉取；停机使用独占生命周期锁等待全部 NATS 调用返回。
class CMtReliableEventStore
{
public:
	CMtReliableEventStore();
	~CMtReliableEventStore();

	// 连接 NATS、创建或更新 Stream；enable=0 时以禁用状态正常启动。
	bool Start(const ST_MT_RELIABLE_EVENT_CONFIG& p_refConfig,
		std::string& p_refError);
	// 拒绝新调用，释放 pending message、pull subscription、JetStream 和连接。
	void Stop();

	// 幂等追加完整可靠事件；返回值只表示是否成功生成业务应答。
	bool Append(const ST_PLUGIN_BINARY_VALUE& p_refRequest,
		const unsigned char* p_pEnvelope,
		std::size_t p_szEnvelopeLen,
		std::int32_t& p_refCode,
		std::string& p_refMessage,
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& p_refData,
		ST_MT_RELIABLE_APPEND_EVENT& p_refEvent,
		std::string& p_refError);
	// 从调用方专属 durable consumer 拉取消息；返回 AckToken 供 1174 使用。
	bool Fetch(const ST_PLUGIN_BINARY_VALUE& p_refRequest,
		std::int32_t& p_refCode,
		std::string& p_refMessage,
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& p_refData,
		std::string& p_refError);
	// 显式确认 AckToken；未确认消息在进程停止或 AckWait 到期后由 JetStream 重投。
	bool Ack(const ST_PLUGIN_BINARY_VALUE& p_refRequest,
		std::int32_t& p_refCode,
		std::string& p_refMessage,
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& p_refData,
		std::string& p_refError);
	// 查询 Stream 和可选 consumer 的服务端状态，不泄漏 NATS 句柄。
	bool Status(const ST_PLUGIN_BINARY_VALUE& p_refRequest,
		std::int32_t& p_refCode,
		std::string& p_refMessage,
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& p_refData,
		std::string& p_refError);
	// 返回 NATS 和 Stream 是否已就绪；仅读取原子状态，可由 1103/1176 与 Stop 并发调用。
	bool IsReady() const;

private:
	// 校验 EventId、节点、通知号、动作、序号和正文，并生成 NATS Subject。
	bool ValidateAppendRequest(
		const ST_PLUGIN_BINARY_VALUE& p_refRequest,
		std::string& p_refEventId,
		std::string& p_refSubject,
		EN_PLUGIN_NOTIFY_ID& p_refNotifyId,
		EN_PLUGIN_NOTIFY_ACTION& p_refAction,
		std::uint64_t& p_refSourceSequence,
		std::int64_t& p_refCreatedTimeMs,
		std::string& p_refError) const;
	// 创建或取得 consumer 专属 pull subscription；调用方持有 m_clConsumerMutex。
	natsSubscription* GetOrCreateSubscription(
		const std::string& p_refConsumer,
		std::string& p_refError);
	// 检查 consumer 名称只含安全字符，防止创建非法 durable 名称。
	static bool IsValidConsumerName(
		const std::string& p_refConsumer);
	// 为 pending message 生成进程内确认令牌；令牌不暴露 NATS reply subject。
	static std::string BuildAckToken(
		const std::string& p_refConsumer,
		std::uint64_t p_ullStreamSequence);
	// 把 NATS 状态转换为带操作和 JetStream 错误码的详细英文文本。
	static std::string BuildNatsError(
		const char* p_szOperation, int p_iStatus,
		int p_iJetStreamError);
	// Stream 不存在时创建，存在时按当前容量和保留配置更新。
	bool EnsureStream(std::string& p_refError);

private:
	ST_MT_RELIABLE_EVENT_CONFIG m_stConfig; // 启动后只读的 JetStream 配置副本。
	std::atomic<bool> m_bAccepting;         // 是否允许新的 1172-1175 调用。
	std::atomic<bool> m_bReady;             // NATS 和 Stream 是否已经可用。
	mutable std::shared_mutex m_clApiMutex; // NATS 调用共享锁与 Stop 独占锁。
	std::mutex m_clConsumerMutex;           // 保护 subscription 和 pending message 所有权。
	natsConnection* m_pConnection;          // nats.c 连接，由本类独占释放。
	jsCtx* m_pJetStream;                    // JetStream 上下文，由本类独占释放。
	std::map<std::string, natsSubscription*> m_mapSubscription; // durable consumer 到 pull subscription。
	std::map<std::string, natsMsg*> m_mapPendingAck; // AckToken 到尚未确认的消息。
	std::map<std::string, std::string> m_mapPendingConsumer; // AckToken 所属 consumer。
};

#endif // H_TRADING_TERMINAL_MT_RELIABLE_EVENT_STORE
