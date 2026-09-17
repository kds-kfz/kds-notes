#ifndef H_TRADING_TERMINAL_MT_DERIVE_M1_ENGINE
#define H_TRADING_TERMINAL_MT_DERIVE_M1_ENGINE

#include "DeriveBinaryProtocol.h"
#include "MtDeriveConfig.h"
#include "MtMinuteCenter.h"
#include "TradingTerminalProtocol.h"

#include <map>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

// M1 引擎只负责真实 Tick 到权威分钟线的确定性转换。
// 1182 Ice 工作线程串行调用 ApplyTick，1185 请求线程可并发读取 Snapshot；本类拥有全部 Bar 副本。
// 本类禁止连接网络、访问 MT SDK、生成无 Tick 假 K 或保存长期历史。
class CMtDeriveM1Engine
{
public:
	CMtDeriveM1Engine();
	~CMtDeriveM1Engine();

	// 按启用 Version+No 建立隔离槽；只能在服务开放 1182 前调用。
	bool Configure(const ST_MT_DERIVE_SERVICE_CONFIG& p_refConfig,
		std::string& p_refError);
	// 将 1167/1241 的权威品种集合安装到分钟中心；函数幂等且不访问网络。
	bool ReconcileSymbols(std::uint16_t p_usVersion,
		std::int32_t p_iNo, const std::vector<std::string>& p_refSymbols,
		std::string& p_refError);
	// 处理单品种创建或更新；重新启用时复用并严格校验原 V3 `.MIN`。
	bool MarkSymbolActive(std::uint16_t p_usVersion,
		std::int32_t p_iNo, const std::string& p_refSymbol,
		std::string& p_refError);
	// 处理单品种删除；关闭映射并禁止写入，但不删除磁盘文件。
	bool MarkSymbolDeleted(std::uint16_t p_usVersion,
		std::int32_t p_iNo, const std::string& p_refSymbol,
		std::string& p_refError);
	// 应用一条已经通过序号校验的 Tick，并返回更新后的 M1 独立副本。
	bool ApplyTick(const ST_QUOTE_BINARY_TICK& p_refTick,
		ST_DERIVE_M1_BAR& p_refChangedBar,
		std::string& p_refError);
	// 应用 Quote 的时间权威状态；旧代次幂等忽略，非法节点或状态返回详细英文错误。
	bool UpdateTimeState(const ST_MT_TIME_STATE& p_refState,
		std::string& p_refError);
	// 按来源、品种和分钟范围构建稳定排序快照，超过 MaxBars 时返回明确失败。
	bool Snapshot(const ST_DERIVE_M1_SNAPSHOT_REQUEST& p_refRequest,
		ST_DERIVE_M1_SNAPSHOT_RESPONSE& p_refResponse,
		std::string& p_refError) const;
	// 从检查点恢复一个来源；同一分钟只保留 LastSequence 更大的记录。
	bool Restore(std::uint16_t p_usVersion, std::int32_t p_iNo,
		const std::vector<ST_DERIVE_M1_BAR>& p_refBar,
		std::string& p_refError);
	// 合并 Query 1168 的纯 MT 回填，成功空日和完整日水位同样写入双槽元数据。
	bool MergeBackfill(const ST_QUERY_M1_BACKFILL_RESPONSE& p_refResponse,
		std::string& p_refError);
	// 刷新脏映射页；TickStore 仅在成功后推进小型元数据检查点并覆盖 WAL。
	bool FlushPersistent(std::string& p_refError);
	// 返回尚未归档的历史 Broker 日。
	void ListArchiveCandidates(
		std::vector<ST_DERIVE_MINUTE_ARCHIVE_CANDIDATE>& p_refCandidate) const;
	// 返回当前 Broker 日未校准到最新 Tick 的品种，供 1168 主动回填。
	void ListCurrentBackfillCandidates(
		std::vector<ST_DERIVE_MINUTE_BACKFILL_CANDIDATE>& p_refCandidate) const;
	// 从稳定历史槽构造确定性整日归档批次。
	bool BuildArchiveBatch(
		const ST_DERIVE_MINUTE_ARCHIVE_CANDIDATE& p_refCandidate,
		std::int64_t p_llFromMinute, std::int64_t p_llToMinute,
		const std::string& p_refArchiveId,
		const std::string& p_refOwnerInstanceId,
		std::uint64_t p_ullLeaseGeneration,
		ST_DERIVE_M1_ARCHIVE_BATCH& p_refBatch,
		std::string& p_refError) const;
	// spool 原子持久化后标记该历史槽可在下一次轮换时覆盖。
	bool MarkArchiveSpooled(
		const ST_DERIVE_MINUTE_ARCHIVE_CANDIDATE& p_refCandidate,
		std::int64_t p_llArchivedTo, std::string& p_refError);
	// 返回当前来源的品种数量，来源不存在时返回 0。
	std::uint32_t GetSymbolCount(
		std::uint16_t p_usVersion, std::int32_t p_iNo) const;
	// 汇总双槽归档和回填水位，供 1186 诊断扩展使用。
	void GetSourceDiagnostics(std::uint16_t p_usVersion,
		std::int32_t p_iNo,
		ST_DERIVE_MINUTE_SOURCE_DIAGNOSTICS& p_refDiagnostics) const;
	// 清空全部来源和分钟线；必须在 1182 请求停止后调用。
	void Clear();

private:
	// 单个来源状态由总锁保护，map 的自然顺序直接作为快照稳定顺序。
	struct ST_SOURCE_STATE
	{
		std::uint64_t ullSourceEpoch; // 最近应用的 Quote 启动代次。
		std::uint64_t ullTimeAuthorityEpoch; // 当前允许写入 M1 的时间权威生命周期。
		std::uint64_t ullTimeGeneration; // 当前允许写入 M1 的时间代次。
		std::int32_t iTimeOffsetSeconds; // Broker 服务器时间减 UTC 的当前偏移秒数。
		std::int64_t llTimeValidUntilUtcMs; // 时间状态失效的 UTC 毫秒时间。
		bool bTimeReady;              // 时间状态是否 READY，过期时仍会在 ApplyTick 再检查。

		ST_SOURCE_STATE();
	};

private:
	mutable std::mutex m_clMutex; // 保护全部来源和 M1。
	std::map<std::pair<std::uint16_t, std::int32_t>,
		ST_SOURCE_STATE> m_mapSource; // Version+No 隔离状态。
	CMtMinuteCenter m_clMinuteCenter; // 拥有双日 .MIN 文件、映射视图和归档水位。
};

#endif // H_TRADING_TERMINAL_MT_DERIVE_M1_ENGINE
