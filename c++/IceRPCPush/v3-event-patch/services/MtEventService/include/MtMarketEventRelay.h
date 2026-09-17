#ifndef H_TRADING_TERMINAL_MT_MARKET_EVENT_RELAY
#define H_TRADING_TERMINAL_MT_MARKET_EVENT_RELAY

#include "EventHeartbeatBinaryProtocol.h"
#include "MtEventConfig.h"
#include "TradingTerminalProtocol.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// Event 行情中继诊断快照。调用方取得值副本后不再持有中继内部锁。
struct ST_MT_MARKET_EVENT_DIAGNOSTICS
{
	std::uint64_t ullQueueCapacity;          // 总配置容量，0 表示不主动容量淘汰。
	std::uint64_t ullQueueDepth;             // 全部分片当前待发布数量。
	std::uint64_t ullReceivedCount;          // 进入 1211 校验的累计数量。
	std::uint64_t ullValidationRejectedCount;// 协议和元数据校验拒绝数量。
	std::uint64_t ullDuplicateCount;         // 同 Epoch 重复序号数量。
	std::uint64_t ullOutOfOrderCount;        // 同 Epoch 回退序号数量。
	std::uint64_t ullEpochResetCount;        // 来源 Epoch 变化数量。
	std::uint64_t ullSequenceGapCount;       // 来源 Sequence 跳跃数量。
	std::uint64_t ullCapacityDroppedCount;   // 正容量模式淘汰最旧 Tick 数量。
	std::uint64_t ullFanoutCount;            // 成功本地 Ice 扇出数量。
	std::uint64_t ullPublishFailedCount;     // 本地 Ice 扇出失败数量。
	std::vector<ST_EVENT_HEARTBEAT_SOURCE_STATUS> aSource; // 稳定排序的逐来源水位。

	ST_MT_MARKET_EVENT_DIAGNOSTICS();
};

// 行情中继输出接口。工作线程只交付完成三层协议校验的不可变 ClientData 正文。
// 实现不得修改正文或同步执行行情业务；返回 false 时中继记录发布失败但继续处理后续 Tick。
class IMtMarketEventRelaySink
{
public:
	virtual ~IMtMarketEventRelaySink();

	// 输入为来源元数据和深拷贝正文；成功本地发布返回 true，失败返回 false 和英文原因。
	virtual bool OnMarketEventReady(
		const ST_PLUGIN_NOTIFY_META& p_refSourceMeta,
		const std::vector<unsigned char>& p_refPayload,
		std::string& p_refError) = 0;
};

struct ST_MT_MARKET_EVENT_SHARD;

// MtEventService 第一阶段单实例行情中继器。
// CloudNet 回调线程执行校验和深拷贝，固定分片线程串行发布同一 Version+No。
// 本类拥有队列、线程和连续性计数，不拥有 Sink；禁止缓存价格、进入 NATS、聚合 M1 或转换时间。
class CMtMarketEventRelay
{
public:
	CMtMarketEventRelay();
	~CMtMarketEventRelay();

	// 创建固定分片并启动线程；容量 0 为无主动淘汰，失败时不保留 Sink 或线程。
	bool Start(const ST_MT_MARKET_EVENT_CONFIG& p_refConfig,
		IMtMarketEventRelaySink* p_pSink, std::string& p_refError);
	// 关闭入队闸门、排空已受理 Tick、等待线程退出并释放队列；允许重复调用。
	void Stop();
	// 校验 Quote 来源、通知元数据、ClientData 和 Quote v1，一致后深拷贝并按来源入队。
	bool Submit(const ST_PLUGIN_NOTIFY_META& p_refMeta,
		const unsigned char* p_pPayload, std::size_t p_szPayloadLen,
		std::string& p_refError);
	// 复制当前队列、计数和逐来源水位；可与 Submit、Worker 和 Stop 并发调用。
	void GetDiagnostics(ST_MT_MARKET_EVENT_DIAGNOSTICS& p_refDiagnostics) const;

private:
	// 当前分片串行发布已受理 Tick，保持同一 Version+No 的输入顺序。
	void Worker(ST_MT_MARKET_EVENT_SHARD* p_pShard);
	// 返回 Version+No 对应的稳定分片下标；调用方已持有生命周期锁并确认数组非空。
	std::size_t SelectShard(std::uint16_t p_usVersion,
		std::int32_t p_iNo) const;
	// 容量淘汰、缺口和发布失败日志共用限频，避免异常期间按 Tick 频率写盘。
	bool ShouldLogDrop(std::int64_t p_llNowMs);

private:
	mutable std::mutex m_clLifecycleMutex; // 保护分片数组、Sink 和启停与 Submit 竞态。
	ST_MT_MARKET_EVENT_CONFIG m_stConfig; // 已校验的运行配置副本。
	IMtMarketEventRelaySink* m_pSink;     // 非拥有输出回调，全部 Worker 退出前保持有效。
	std::vector<std::unique_ptr<ST_MT_MARKET_EVENT_SHARD>> m_aShard; // 固定分片及线程所有权。
	std::atomic<bool> m_bAccepting;       // 是否允许 CloudNet 回调继续提交。
	std::atomic<std::int64_t> m_llLastDropLogMs; // 最近一次高频诊断日志时间。
	std::atomic<std::uint64_t> m_ullReceivedCount; // 进入校验的累计 Tick 数。
	std::atomic<std::uint64_t> m_ullValidationRejectedCount; // 校验拒绝累计数。
};

#endif // H_TRADING_TERMINAL_MT_MARKET_EVENT_RELAY
