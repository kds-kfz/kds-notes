#ifndef H_TRADING_TERMINAL_MT_QUERY_QUOTE_CACHE
#define H_TRADING_TERMINAL_MT_QUERY_QUOTE_CACHE

#include "DeriveBinaryProtocol.h"
#include "MtQueryConfig.h"
#include "QuoteSnapshotBinaryProtocol.h"
#include "TradingTerminalProtocol.h"

#include <cstdint>
#include <deque>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

// Query 侧 M1 记录。记录只由真实 Tick 生成，无 Tick 分钟不会创建占位 K 线。
struct ST_MT_QUERY_BAR
{
	std::int64_t llDateTime;      // UTC0 周期起始秒。
	std::uint32_t uiServerDate;   // Derive 记录的 Broker 日期；旧归档输入允许为 0。
	std::uint32_t uiFlags;        // Derive M1 确认状态位，用于禁止未回填日初时伪造 OpenPrice。
	double dOpen;                 // 周期第一笔有效价格。
	double dHigh;                 // 周期最高价。
	double dLow;                  // 周期最低价。
	double dClose;                // 周期最后一笔有效价格。
	std::uint64_t ullTickVolume;  // 周期接收 Tick 数。
	std::uint64_t ullRealVolume;  // 周期 Tick 上报的扩展成交量累计值。
	std::uint64_t ullSourceEpoch; // Derive M1 来源代次，仅用于去重和缺口诊断。
	std::uint64_t ullLastSequence;// 当前记录最后一条 Quote 序号。

	ST_MT_QUERY_BAR();
};

// 单节点对外状态快照，用于 1152 精确说明冷启动和行情新鲜度。
struct ST_MT_QUERY_QUOTE_SOURCE_STATUS
{
	std::uint16_t usVersion;       // MT 平台版本。
	std::int32_t iNo;              // 节点编号。
	bool bSnapshotReady;           // 是否完成 1122 快照安装。
	bool bM1Ready;                 // 是否完成 1185 权威 M1 快照安装。
	bool bBufferOverflow;          // 本轮冷启动增量缓冲是否溢出。
	std::uint64_t ullSourceEpoch;  // 当前接受增量的 Quote 启动代次。
	std::uint64_t ullHighWatermark;// 已应用的最大 Quote 原始入口序号。
	std::int64_t llLastIngressMs;  // 最近一条已应用 Tick 的入口时间。
	bool bTimeReady;               // 当前时间权威状态是否 READY 且未过期。
	std::string strTimeZoneId;     // Windows 动态时区 ID，供历史日历边界转换和诊断。
	std::int32_t iTimeOffsetSeconds; // 当前实测服务器 UTC 偏移秒数。
	std::uint64_t ullTimeAuthorityEpoch; // 当前时间权威生命周期。
	std::uint64_t ullTimeGeneration; // 当前时间状态代次。
	std::int64_t llTimeValidUntilUtcMs; // 当前时间状态失效的 UTC 毫秒时间。
	std::size_t szSymbolCount;     // 当前最新 Tick 品种数。
	std::size_t szPendingCount;    // 尚未安装快照时的增量缓冲数。

	ST_MT_QUERY_QUOTE_SOURCE_STATUS();
};

// Query 侧行情状态中心。
// 推送线程和快照线程可并发写入，Binary 请求线程可并发读取；本类拥有 Tick、缓冲和 M1 的全部副本。
// 本类不连接 MT SDK、不读取历史文件，也不承担 PostgreSQL 客户数据职责。
class CMtQueryQuoteCache
{
public:
	CMtQueryQuoteCache();
	~CMtQueryQuoteCache();

	// 按启用节点建立冷启动状态；只能在网络启动前调用。
	bool Configure(const ST_MT_QUERY_SERVICE_CONFIG& p_refConfig,
		std::string& p_refError);
	// 接收已经严格校验的 1211 Tick；快照未安装时只进入有界缓冲。
	bool OnIncrement(const ST_QUOTE_BINARY_TICK& p_refTick,
		std::string& p_refError);
	// 原子安装单节点快照并按序应用高水位之后的缓冲 Tick；缓冲溢出时拒绝安装并要求重新拉取。
	bool InstallSnapshot(
		const ST_QUOTE_SNAPSHOT_BINARY_RESPONSE& p_refSnapshot,
		std::string& p_refError);
	// 原子安装 Derive 1185 全量 M1 快照；快照只替换指定 Version+No 的分钟线。
	bool InstallM1Snapshot(
		std::uint16_t p_usVersion, std::int32_t p_iNo,
		const ST_DERIVE_M1_SNAPSHOT_RESPONSE& p_refSnapshot,
		std::string& p_refError);
	// 应用 MtEventService 中继的单条或小批 1252；LastSequence 较旧的重复记录幂等忽略。
	bool OnM1Increment(
		const ST_DERIVE_M1_SNAPSHOT_RESPONSE& p_refUpdate,
		std::string& p_refError);
	// 应用 Quote 时间权威状态，D1/W1/MN1 等服务器日历周期只读取该动态偏移。
	bool UpdateTimeState(const ST_MT_TIME_STATE& p_refState,
		std::string& p_refError);
	// 返回指定节点最新 Tick；Symbol 过滤为空时返回全部，调用方获得独立副本。
	bool GetQuotes(std::uint16_t p_usVersion, std::int32_t p_iNo,
		const std::set<std::string>& p_refSymbols,
		std::vector<ST_QUOTE_BINARY_TICK>& p_refTick,
		ST_MT_QUERY_QUOTE_SOURCE_STATUS& p_refStatus,
		std::string& p_refError) const;
	// 返回 Derive M1 已确认的品种开盘价；未确认或来源不存在时返回 0，不修改 Quote v1 Tick。
	double GetOpenPrice(std::uint16_t p_usVersion, std::int32_t p_iNo,
		const std::string& p_refSymbol) const;
	// 从真实 M1 聚合请求周期；只返回缓存覆盖范围内的数据，不制造无 Tick K 线。
	bool GetBars(std::uint16_t p_usVersion, std::int32_t p_iNo,
		const std::string& p_refSymbol, std::int64_t p_llIntervalMinutes,
		std::int64_t p_llFrom, std::int64_t p_llTo,
		std::size_t p_szTailCount, std::vector<ST_MT_QUERY_BAR>& p_refBars,
		std::string& p_refError) const;
	// 复制全部节点状态；状态数组按 Version+No 稳定排序。
	void GetStatus(
		std::vector<ST_MT_QUERY_QUOTE_SOURCE_STATUS>& p_refStatus) const;
	// 服务停止后清空全部业务状态；必须在快照线程停止后调用。
	void Clear();

private:
	// 单节点状态由同一把总锁保护，保证快照、高水位、增量和 M1 的安装边界一致。
	struct ST_SOURCE_STATE
	{
		// 单品种最近确认交易日开盘价；交易日由动态时区 D1 边界确定，周末和休市不清空。
		struct ST_OPEN_PRICE_STATE
		{
			std::int64_t llDayBucket;   // 当前确认交易日的 UTC0 D1 起点。
			std::int64_t llFirstMinute; // 产生权威首值的最早 M1 分钟。
			double dOpenPrice;          // 该分钟的 Open；无确认值时为 0。

			ST_OPEN_PRICE_STATE();
		};

		bool bSnapshotReady;            // 是否已经安装本轮快照。
		bool bM1Ready;                  // 是否已经安装 Derive 权威 M1 快照。
		bool bBufferOverflow;           // 缓冲溢出后必须重新拉取快照。
		std::uint64_t ullSourceEpoch;   // 当前接受增量的 Quote 启动代次。
		std::uint64_t ullHighWatermark; // 当前 SourceEpoch 下已安装或应用的最大原始序号。
		std::int64_t llLastIngressMs;   // 最近 Tick 入口时间。
		int iServerTimeOffsetSeconds;   // 1244/1123 动态同步的当前服务器时间偏移。
		std::string strTimeZoneId;      // Windows 动态时区 ID，历史日期不得只使用当前偏移。
		std::uint64_t ullTimeAuthorityEpoch; // 当前偏移所属时间权威生命周期。
		std::uint64_t ullTimeGeneration;// 当前偏移代次，旧通知不得覆盖。
		std::int64_t llTimeValidUntilUtcMs; // 时间状态的 UTC 毫秒有效期。
		bool bTimeReady;                // 时间状态是否 READY 且未过期。
		std::set<std::uint64_t> setRetiredEpoch; // 已切离代次，迟到通知不得重新覆盖新代次。
		std::deque<ST_QUOTE_BINARY_TICK> clPending; // 快照前有界增量缓冲。
		std::map<std::string, ST_QUOTE_BINARY_TICK> mapLatest; // 每品种最新 Tick。
		std::map<std::string, std::map<std::int64_t, ST_MT_QUERY_BAR>>
			mapMinute; // 每品种按 UTC0 分钟保存的真实 M1。
		std::map<std::string, ST_OPEN_PRICE_STATE> mapOpenPrice; // MT4 权威 M1 恢复的最近确认交易日首值。

		ST_SOURCE_STATE();
	};

	// 在持锁状态应用一条严格递增 Tick，只维护最新行情，不再从 Best Effort 1211 生成 M1。
	void ApplyTickLocked(ST_SOURCE_STATE& p_refState,
		const ST_QUOTE_BINARY_TICK& p_refTick);
	// 在持锁状态按 SourceEpoch+Sequence 去重并应用增量；返回 false 表示迟到或重复包被幂等忽略。
	bool ApplyIncrementLocked(ST_SOURCE_STATE& p_refState,
		const ST_QUOTE_BINARY_TICK& p_refTick);
	// 在持锁状态校验并安装一条 Derive 权威 M1。
	bool ApplyM1Locked(ST_SOURCE_STATE& p_refState,
		const ST_DERIVE_M1_BAR& p_refBar,
		std::string& p_refError);
	// 在持锁状态从当前 M1 保留窗口重建全部 OpenPrice；时区未就绪时保持空状态并等待 1123/1244。
	bool RebuildOpenPriceLocked(std::uint16_t p_usVersion,
		ST_SOURCE_STATE& p_refState, std::string& p_refError);
	// 将 M1 按请求周期和节点服务器边界聚合。
	static bool ResolveBucket(std::uint16_t p_usVersion,
		const std::string& p_refTimeZoneId,
		std::int64_t p_llUtcSeconds,
		std::int64_t p_llIntervalMinutes,
		std::int64_t& p_refBucketUtcSeconds,
		std::string& p_refError);
	// 校验 V2 已发布的分钟周期集合。
	static bool IsSupportedPeriod(std::uint16_t p_usVersion,
		std::int64_t p_llIntervalMinutes);

private:
	mutable std::mutex m_clMutex; // 保护全部来源、Tick、缓冲和 M1。
	std::size_t m_szBufferCapacity; // 每节点冷启动增量上限。
	std::size_t m_szMinuteRetention;// 每品种最多保留的 M1 数量。
	std::map<std::pair<std::uint16_t, std::int32_t>, ST_SOURCE_STATE>
		m_mapSource;               // Version+No 到隔离状态。
};

#endif // H_TRADING_TERMINAL_MT_QUERY_QUOTE_CACHE
