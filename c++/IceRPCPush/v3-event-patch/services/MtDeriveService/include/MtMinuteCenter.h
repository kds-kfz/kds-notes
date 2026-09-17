#ifndef H_TRADING_TERMINAL_MT_MINUTE_CENTER
#define H_TRADING_TERMINAL_MT_MINUTE_CENTER

#include "DeriveBinaryProtocol.h"
#include "MtDeriveConfig.h"

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

// 单个 V3 分钟映射文件。实现隐藏在源文件中，禁止业务层直接持有原生句柄。
class CMinuteAsFile;
// 单个 Version+No+Symbol 分钟运行态，拥有文件、生命周期、回填和归档状态。
class CMtGoods;
// 单个 Version+No 品种容器，负责权威品种集合的幂等核对。
class CMtPlatform;
// 单次回填完成守卫，保证异常和提前返回时释放本操作占用。
class CMtMinuteBackfillFinishGuard;

// 单个待归档 Broker 日。范围由 M1Engine 使用动态时区计算，本结构只保存映射文件的稳定身份。
struct ST_DERIVE_MINUTE_ARCHIVE_CANDIDATE
{
	std::uint16_t usPlatformVersion; // MT 平台版本。
	std::int32_t iSourceNo;          // 同平台节点编号。
	std::string strSymbol;           // 原始 UTF-8 品种代码。
	std::uint32_t uiServerDate;      // 待归档 Broker 日期 YYYYMMDD。
	std::uint64_t ullTimeAuthorityEpoch; // 该槽最近写入的时间权威生命周期。
	std::uint64_t ullTimeGeneration; // 该槽最近写入的时间代次。
	std::uint64_t ullFileGeneration; // 构建候选时的文件代次，归档标记必须执行 CAS。

	ST_DERIVE_MINUTE_ARCHIVE_CANDIDATE();
};

// 当前 Broker 日的主动回填任务。From=0 表示 Query 必须按动态时区解析该日 UTC 起点。
struct ST_DERIVE_MINUTE_BACKFILL_CANDIDATE
{
	std::uint16_t usPlatformVersion; // MT 平台版本。
	std::int32_t iSourceNo;          // 同平台节点编号。
	std::string strSymbol;           // 原始 UTF-8 品种代码。
	std::uint32_t uiServerDate;      // 当前 Broker 日期 YYYYMMDD。
	std::int64_t llCheckedTo;        // 已由 1168 确认的 UTC 分钟，0 表示尚未回填日初。
	std::int64_t llLatestTickMinute; // 当前双槽看到的最新 UTC Tick 分钟。
	std::uint64_t ullTimeAuthorityEpoch; // 当前槽使用的时间权威生命周期。
	std::uint64_t ullTimeGeneration; // 当前槽使用的时间代次。

	ST_DERIVE_MINUTE_BACKFILL_CANDIDATE();
};

// 单来源双槽和归档诊断；只汇总计数及水位，不暴露品种、账号或连接参数。
struct ST_DERIVE_MINUTE_SOURCE_DIAGNOSTICS
{
	std::int64_t llArchiveWatermark; // 全部映射文件共同完成的最小归档 UTC 水位。
	std::uint32_t uiArchivePendingCount; // 尚未写入 spool 的历史槽数量。
	std::uint32_t uiBackfillPendingCount; // 历史日未完整校准或当前日落后最新 Tick 的槽数量。

	ST_DERIVE_MINUTE_SOURCE_DIAGNOSTICS();
};

// Derive 私有双日 M1 映射存储。
// TickStore 的来源锁保证同一 Version+No 写入串行，本类仍使用内部锁保护 1185、归档线程和停机并发。
// 本类拥有 Windows 文件、映射和视图句柄；禁止连接 Query/NATS、计算收益或写长期历史文件。
class CMtMinuteCenter
{
public:
	CMtMinuteCenter();
	~CMtMinuteCenter();

	// 创建 minute_work 目录并扫描已有 .MIN；损坏文件会阻止来源进入 READY，禁止静默重建。
	bool Configure(const ST_MT_DERIVE_SERVICE_CONFIG& p_refConfig,
		std::string& p_refError);
	// 使用 1167/1241 的权威品种集合幂等核对来源运行态；磁盘失败时不提交 READY。
	bool ReconcileSymbols(std::uint16_t p_usVersion,
		std::int32_t p_iNo, const std::vector<std::string>& p_refSymbols,
		std::string& p_refError);
	// 将单品种恢复为 active；仅允许已配置来源调用，失败时保留原状态。
	bool MarkSymbolActive(std::uint16_t p_usVersion,
		std::int32_t p_iNo, const std::string& p_refSymbol,
		std::string& p_refError);
	// 将单品种标记为 deleted 并关闭映射；保留磁盘文件供重新启用。
	bool MarkSymbolDeleted(std::uint16_t p_usVersion,
		std::int32_t p_iNo, const std::string& p_refSymbol,
		std::string& p_refError);
	// 按调用方提供的 UTC 秒和 Tick 原始 Broker 时间更新固定槽；时间代次来自 Derive 独立权威状态。
	bool ApplyTick(const ST_QUOTE_BINARY_TICK& p_refTick,
		std::int64_t p_llUtcTime,
		std::uint64_t p_ullTimeAuthorityEpoch,
		std::uint64_t p_ullTimeGeneration,
		ST_DERIVE_M1_BAR& p_refChangedBar,
		std::string& p_refError);
	// 合并 1168 的纯 MT M1；先保留重叠区在线 Sequence，再补齐空槽并提交完整检查水位。
	bool MergeBackfill(
		const ST_QUERY_M1_BACKFILL_RESPONSE& p_refResponse,
		std::string& p_refError);
	// 首次升级只读迁移旧 .chk；后续 1168 会按 Broker 日历修复 UTC 日近似槽。
	bool RestoreLegacy(std::uint16_t p_usVersion,
		std::int32_t p_iNo,
		const std::vector<ST_DERIVE_M1_BAR>& p_refBars,
		std::string& p_refError);
	// 复制指定来源、品种和 UTC 范围内的全部已提交记录，结果稳定排序。
	bool Snapshot(const ST_DERIVE_M1_SNAPSHOT_REQUEST& p_refRequest,
		ST_DERIVE_M1_SNAPSHOT_RESPONSE& p_refResponse,
		std::string& p_refError) const;
	// 刷新全部脏映射页；成功后调用方才允许覆盖对应 WAL 段。
	bool FlushDirty(std::string& p_refError);
	// 返回尚未归档的历史槽，归档线程再通过动态时区计算完整日 UTC 边界。
	void ListArchiveCandidates(
		std::vector<ST_DERIVE_MINUTE_ARCHIVE_CANDIDATE>& p_refCandidate) const;
	// 返回当前日尚未校准到最新 Tick 的品种；归档管理器按 tail delay 计算稳定终点。
	void ListCurrentBackfillCandidates(
		std::vector<ST_DERIVE_MINUTE_BACKFILL_CANDIDATE>& p_refCandidate) const;
	// 构造确定性归档批次；调用方提供已校准的完整日 UTC 边界和当前 fencing。
	bool BuildArchiveBatch(
		const ST_DERIVE_MINUTE_ARCHIVE_CANDIDATE& p_refCandidate,
		std::int64_t p_llFromMinute, std::int64_t p_llToMinute,
		const std::string& p_refArchiveId,
		const std::string& p_refOwnerInstanceId,
		std::uint64_t p_ullLeaseGeneration,
		ST_DERIVE_M1_ARCHIVE_BATCH& p_refBatch,
		std::string& p_refError) const;
	// spool 原子落盘后标记历史槽已归档；失败时保持未归档并阻止下一次覆盖。
	bool MarkArchiveSpooled(
		const ST_DERIVE_MINUTE_ARCHIVE_CANDIDATE& p_refCandidate,
		std::int64_t p_llArchivedTo,
		std::string& p_refError);
	// 返回来源当前映射文件数量。
	std::uint32_t GetSymbolCount(std::uint16_t p_usVersion,
		std::int32_t p_iNo) const;
	// 汇总来源归档水位和待处理数量，供 1186 扩展诊断使用。
	void GetSourceDiagnostics(std::uint16_t p_usVersion,
		std::int32_t p_iNo,
		ST_DERIVE_MINUTE_SOURCE_DIAGNOSTICS& p_refDiagnostics) const;
	// 关闭全部映射并清空来源；不删除任何恢复文件。
	void Clear();

private:
	// 校验来源并延迟创建品种映射；返回指针只在调用方持有 m_clMutex 时有效。
	CMtGoods* GetOrCreateLocked(std::uint16_t p_usVersion,
		std::int32_t p_iNo, const std::string& p_refSymbol,
		bool p_bCreate, std::string& p_refError);
	// 扫描一个来源的既有 .MIN 文件并恢复原始 Symbol。
	bool LoadSourceFilesLocked(std::uint16_t p_usVersion,
		std::int32_t p_iNo, std::string& p_refError);

private:
	mutable std::mutex m_clMutex; // 保护映射表、映射视图内容和 dirty 状态。
	std::string m_strRootPath;    // <cache>\minute_work 绝对路径。
	std::set<std::pair<std::uint16_t, std::int32_t>> m_setSource; // 启用来源。
	std::set<std::pair<std::uint16_t, std::int32_t>> m_setReconciledSource; // 已安装权威品种集合的来源。
	std::map<std::pair<std::uint16_t, std::int32_t>,
		std::map<std::string, std::unique_ptr<CMtGoods>>>
		m_mapRuntime; // 来源和原始品种到映射所有权。
};

#endif // H_TRADING_TERMINAL_MT_MINUTE_CENTER
