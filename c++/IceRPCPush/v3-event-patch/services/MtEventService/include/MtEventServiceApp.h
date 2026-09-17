#ifndef H_TRADING_TERMINAL_ICE_EVENT_SERVICE_APP
#define H_TRADING_TERMINAL_ICE_EVENT_SERVICE_APP

#include "MtEventConfig.h"
#include "MtEventBroadcastBus.h"
#include "MtMarketEventRelay.h"
#include "MtReliableEventStore.h"
#include "ServiceApplication.h"
#include "EventHeartbeatBinaryProtocol.h"

#include <atomic>
#include <cstdint>

// MtEventService 隔离两类引擎：1211 行情使用 Best Effort 分片队列，
// 122x/124x 使用 JetStream 持久化、Replay 和显式 ACK。
// 本类拥有两个引擎和配置副本，禁止接入 MT SDK、业务数据库或直接向 Web 发送 JSON。
class CMtEventServiceApp : public CServiceApplication,
	public IMtMarketEventRelaySink,
	public IMtEventBroadcastSink
{
public:
	CMtEventServiceApp();
	virtual ~CMtEventServiceApp();

protected:
	// 读取行情中继和 JetStream 配置；失败时返回详细英文错误。
	virtual bool OnConfigure(std::string& p_refError) override;
	// CloudNet 网络启动后先连接可靠后端，再创建行情中继分片。
	virtual bool OnStarted(std::string& p_refError) override;
	// 拒绝可靠事件请求并释放 NATS，再排空已受理行情。
	virtual void OnStopping() override;
	// 处理 1172-1176；1171 已废弃，其他功能号返回 false 交由基类生成明确错误。
	virtual bool HandleBusinessRequest(const ST_CLOUD_NET_BINARY_REQUEST* p_pRequest,
		ST_CLOUD_NET_BINARY_RESULT* p_pResult) override;
	// 接收 MtQuoteService 1211 推送，正文仅在回调期间有效，中继器负责完成深拷贝。
	virtual void HandlePluginNotify(EN_PLUGIN_NOTIFY_ID p_enNotifyId,
		const ST_PLUGIN_NOTIFY_META& p_refMeta,
		const unsigned char* p_pPayload,
		std::size_t p_szPayloadLen) override;
	// 中继工作线程以 Event 身份本地发布不可变正文；返回值供 Relay 统计成功和失败。
	virtual bool OnMarketEventReady(
		const ST_PLUGIN_NOTIFY_META& p_refSourceMeta,
		const std::vector<unsigned char>& p_refPayload,
		std::string& p_refError) override;
	// 广播总线完成跨实例去重后，只向连接到当前 Event 实例的 Ice 订阅者发布一次。
	virtual void OnBroadcastEvent(EN_PLUGIN_NOTIFY_ID p_enNotifyId,
		const ST_PLUGIN_NOTIFY_META& p_refMeta,
		const std::vector<unsigned char>& p_refPayload) override;
	// 向 1103 追加 Event 队列、连续性、扇出和 JetStream Ready 指标。
	virtual void AppendRuntimeMetrics(
		std::vector<ST_CLUSTER_RUNTIME_METRIC>& p_refMetric) override;

private:
	// 从 1172 v1/v2 字段中解析来源和 token，并在集群启用时实时校验远端租约。
	bool ValidateReliableEventFence(
		const ST_PLUGIN_BINARY_VALUE& p_refRequest,
		std::string& p_refError) const;
	// 调用可靠存储并编码统一 Reliable Event Binary 应答。
	bool HandleReliableEventRequest(
		const ST_CLOUD_NET_BINARY_REQUEST* p_pRequest,
		ST_CLOUD_NET_BINARY_RESULT* p_pResult);
	// 解码 1176 请求并返回 Event 进程、队列、逐来源水位和 JetStream Ready。
	bool HandleEventHeartbeat(
		const ST_CLOUD_NET_BINARY_REQUEST* p_pRequest,
		ST_CLOUD_NET_BINARY_RESULT* p_pResult);
	// 发布失败日志限频，避免 Event 到 Gateway 断线期间按 Tick 频率写盘。
	bool ShouldLogPublishError(std::int64_t p_llNowMs);
	// 校验 Derive 产生的 1251/1252 正文并以 Event 身份重新发布；函数可由推送回调线程并发调用。
	void RelayDerivedEvent(EN_PLUGIN_NOTIFY_ID p_enNotifyId,
		const ST_PLUGIN_NOTIFY_META& p_refMeta,
		const unsigned char* p_pPayload,
		std::size_t p_szPayloadLen);
	// 生成 MtEventService 自身的中继序号；原始 Quote 序号继续保留在 ClientData 和 Quote 正文中。
	std::uint64_t NextRelaySequence();
	// 生成当前 Event 进程代次；仅在构造时调用一次，禁止因连接重连而变化。
	static std::uint64_t GenerateProcessEpoch();

private:
	ST_MT_EVENT_SERVICE_CONFIG m_stEventConfig;      // 已校验的双引擎运行配置。
	CMtEventBroadcastBus m_clBroadcastBus;           // 拥有跨实例 Core NATS 连接和有界去重窗口。
	CMtMarketEventRelay m_clMarketEventRelay;        // 拥有分片队列和发布线程。
	CMtReliableEventStore m_clReliableEventStore;    // 独占 NATS 和 JetStream 资源。
	std::atomic<bool> m_bComponentsStarted;          // 防止失败和析构路径重复停止。
	std::atomic<std::int64_t> m_llLastPublishErrorMs; // 上次中继发布失败日志时间。
	std::atomic<std::uint64_t> m_ullRelaySequence;    // 当前 Event 实例的中继关联序号。
	std::uint64_t m_ullProcessEpoch;                  // 1176 返回的当前进程代次。
};

#endif // H_TRADING_TERMINAL_ICE_EVENT_SERVICE_APP
