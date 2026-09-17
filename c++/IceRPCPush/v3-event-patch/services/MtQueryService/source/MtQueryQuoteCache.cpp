#include "MtQueryQuoteCache.h"

#include "MtQueryTimeZone.h"

#include <algorithm>
#include <chrono>
#include <ctime>

ST_MT_QUERY_BAR::ST_MT_QUERY_BAR()
	: llDateTime(0)
	, uiServerDate(0)
	, uiFlags(0)
	, dOpen(0.0)
	, dHigh(0.0)
	, dLow(0.0)
	, dClose(0.0)
	, ullTickVolume(0)
	, ullRealVolume(0)
	, ullSourceEpoch(0)
	, ullLastSequence(0)
{
}

ST_MT_QUERY_QUOTE_SOURCE_STATUS::ST_MT_QUERY_QUOTE_SOURCE_STATUS()
	: usVersion(0)
	, iNo(0)
	, bSnapshotReady(false)
	, bM1Ready(false)
	, bBufferOverflow(false)
	, ullSourceEpoch(0)
	, ullHighWatermark(0)
	, llLastIngressMs(0)
	, bTimeReady(false)
	, strTimeZoneId()
	, iTimeOffsetSeconds(0)
	, ullTimeAuthorityEpoch(0)
	, ullTimeGeneration(0)
	, llTimeValidUntilUtcMs(0)
	, szSymbolCount(0)
	, szPendingCount(0)
{
}

CMtQueryQuoteCache::ST_SOURCE_STATE::ST_OPEN_PRICE_STATE::
	ST_OPEN_PRICE_STATE()
	: llDayBucket(0)
	, llFirstMinute(0)
	, dOpenPrice(0.0)
{
}

CMtQueryQuoteCache::ST_SOURCE_STATE::ST_SOURCE_STATE()
	: bSnapshotReady(false)
	, bM1Ready(false)
	, bBufferOverflow(false)
	, ullSourceEpoch(0)
	, ullHighWatermark(0)
	, llLastIngressMs(0)
	, iServerTimeOffsetSeconds(0)
	, strTimeZoneId()
	, ullTimeAuthorityEpoch(0)
	, ullTimeGeneration(0)
	, llTimeValidUntilUtcMs(0)
	, bTimeReady(false)
	, setRetiredEpoch()
	, clPending()
	, mapLatest()
	, mapMinute()
	, mapOpenPrice()
{
}

CMtQueryQuoteCache::CMtQueryQuoteCache()
	: m_clMutex()
	, m_szBufferCapacity(0)
	, m_szMinuteRetention(0)
	, m_mapSource()
{
}

CMtQueryQuoteCache::~CMtQueryQuoteCache()
{
	Clear();
}

bool CMtQueryQuoteCache::Configure(
	const ST_MT_QUERY_SERVICE_CONFIG& p_refConfig,
	std::string& p_refError)
{
	p_refError.clear();
	if (p_refConfig.szBootstrapBufferCapacity == 0 ||
		p_refConfig.szMinuteRetention == 0)
	{
		p_refError =
			"QUERY_QUOTE_CACHE_CONFIG_INVALID: buffer and minute retention must be positive";
		return false;
	}
	std::lock_guard<std::mutex> clGuard(m_clMutex);
	m_mapSource.clear();
	m_szBufferCapacity =
		p_refConfig.szBootstrapBufferCapacity;
	m_szMinuteRetention = p_refConfig.szMinuteRetention;
	for (const ST_MT_QUERY_SOURCE_CONFIG& refSource :
		p_refConfig.aSource)
	{
		if (!refSource.bEnable)
		{
			continue;
		}
		const std::pair<std::uint16_t, std::int32_t> stKey(
			refSource.usVersion, refSource.iNo);
		ST_SOURCE_STATE stState;
		if (!m_mapSource.insert(std::make_pair(stKey,
			stState)).second)
		{
			p_refError =
				"QUERY_QUOTE_CACHE_SOURCE_DUPLICATE: enabled Version+No is duplicated";
			m_mapSource.clear();
			return false;
		}
	}
	if (m_mapSource.empty())
	{
		p_refError =
			"QUERY_QUOTE_CACHE_SOURCE_EMPTY: at least one enabled source is required";
		return false;
	}
	return true;
}

bool CMtQueryQuoteCache::OnIncrement(
	const ST_QUOTE_BINARY_TICK& p_refTick,
	std::string& p_refError)
{
	p_refError.clear();
	if (!ValidateQuoteBinaryTick(p_refTick, p_refError))
	{
		p_refError =
			"QUERY_QUOTE_INCREMENT_INVALID: detail=" +
			p_refError;
		return false;
	}
	const std::pair<std::uint16_t, std::int32_t> stKey(
		p_refTick.usPlatformVersion, p_refTick.iSourceNo);
	std::lock_guard<std::mutex> clGuard(m_clMutex);
	std::map<std::pair<std::uint16_t, std::int32_t>,
		ST_SOURCE_STATE>::iterator it = m_mapSource.find(stKey);
	if (it == m_mapSource.end())
	{
		p_refError =
			"QUERY_QUOTE_SOURCE_NOT_CONFIGURED: event Version+No is not enabled";
		return false;
	}
	ST_SOURCE_STATE& refState = it->second;
	if (refState.bSnapshotReady)
	{
		ApplyIncrementLocked(refState, p_refTick);
		return true;
	}

	// 冷启动期间只缓存原始增量；溢出后不覆盖旧数据，否则无法证明快照后的序号连续性。
	if (refState.clPending.size() >= m_szBufferCapacity)
	{
		refState.bBufferOverflow = true;
		p_refError =
			"QUERY_QUOTE_BOOTSTRAP_BUFFER_FULL: snapshot must be retried for this Version+No";
		return false;
	}
	refState.clPending.push_back(p_refTick);
	return true;
}

bool CMtQueryQuoteCache::InstallSnapshot(
	const ST_QUOTE_SNAPSHOT_BINARY_RESPONSE& p_refSnapshot,
	std::string& p_refError)
{
	p_refError.clear();
	const std::pair<std::uint16_t, std::int32_t> stKey(
		p_refSnapshot.usPlatformVersion, p_refSnapshot.iSourceNo);
	std::lock_guard<std::mutex> clGuard(m_clMutex);
	std::map<std::pair<std::uint16_t, std::int32_t>,
		ST_SOURCE_STATE>::iterator it = m_mapSource.find(stKey);
	if (it == m_mapSource.end())
	{
		p_refError =
			"QUERY_QUOTE_SNAPSHOT_SOURCE_NOT_CONFIGURED: response Version+No is not enabled";
		return false;
	}
	ST_SOURCE_STATE& refState = it->second;
	if (refState.bBufferOverflow)
	{
		// 清空不完整缓冲并保持 bootstrap 状态，调用线程下一轮会重新获取更新后的 Quote 快照。
		refState.clPending.clear();
		refState.bBufferOverflow = false;
		p_refError =
			"QUERY_QUOTE_SNAPSHOT_RETRY_REQUIRED: increment buffer overflowed during snapshot request";
		return false;
	}

	std::map<std::string, ST_QUOTE_BINARY_TICK> mapSnapshot;
	for (const ST_QUOTE_BINARY_TICK& refTick :
		p_refSnapshot.aTick)
	{
		mapSnapshot[refTick.strSymbol] = refTick;
	}
	const std::vector<ST_QUOTE_BINARY_TICK> aPending(
		refState.clPending.begin(), refState.clPending.end());

	// Quote 快照只替换最新行情；权威 M1 由独立 1185/1252 链路维护，不能在这里清空。
	refState.mapLatest.swap(mapSnapshot);
	refState.ullSourceEpoch =
		p_refSnapshot.ullSourceEpoch;
	refState.ullHighWatermark =
		p_refSnapshot.ullHighWatermark;
	refState.setRetiredEpoch.clear();
	refState.llLastIngressMs = 0;
	for (const std::pair<const std::string,
		ST_QUOTE_BINARY_TICK>& refItem : refState.mapLatest)
	{
		refState.llLastIngressMs = (std::max)(
			refState.llLastIngressMs,
			refItem.second.llIngressTimeMs);
	}
	for (const ST_QUOTE_BINARY_TICK& refTick : aPending)
	{
		// 缓冲保持实际到达顺序；跨 Epoch 时单独比较各自 Sequence，不能按 Sequence 全局排序。
		ApplyIncrementLocked(refState, refTick);
	}
	refState.clPending.clear();
	refState.bSnapshotReady = true;
	return true;
}

bool CMtQueryQuoteCache::InstallM1Snapshot(
	std::uint16_t p_usVersion, std::int32_t p_iNo,
	const ST_DERIVE_M1_SNAPSHOT_RESPONSE& p_refSnapshot,
	std::string& p_refError)
{
	p_refError.clear();
	if (p_refSnapshot.enState !=
			EN_DERIVE_SERVICE_STATE_READY ||
		p_refSnapshot.iCode != 0)
	{
		p_refError =
			"QUERY_M1_SNAPSHOT_NOT_READY: state=" +
			std::to_string(
				static_cast<unsigned int>(
					p_refSnapshot.enState)) +
			", code=" +
			std::to_string(p_refSnapshot.iCode) +
			", detail=" + p_refSnapshot.strMessage;
		return false;
	}
	const std::pair<std::uint16_t, std::int32_t> stKey(
		p_usVersion, p_iNo);
	std::lock_guard<std::mutex> clGuard(m_clMutex);
	std::map<std::pair<std::uint16_t, std::int32_t>,
		ST_SOURCE_STATE>::iterator it =
		m_mapSource.find(stKey);
	if (it == m_mapSource.end())
	{
		p_refError =
			"QUERY_M1_SNAPSHOT_SOURCE_NOT_CONFIGURED: requested Version+No is not enabled";
		return false;
	}

	// 在临时状态完成全部校验后一次替换，畸形记录不会破坏旧快照。
	ST_SOURCE_STATE stCandidate = it->second;
	stCandidate.mapMinute.clear();
	for (const ST_DERIVE_M1_BAR& refBar :
		p_refSnapshot.aBar)
	{
		if (refBar.usPlatformVersion != p_usVersion ||
			refBar.iSourceNo != p_iNo ||
			!ApplyM1Locked(stCandidate, refBar,
				p_refError))
		{
			if (p_refError.empty())
			{
				p_refError =
					"QUERY_M1_SNAPSHOT_SOURCE_MISMATCH: response contains another Version+No";
			}
			return false;
		}
	}
	it->second.mapMinute.swap(
		stCandidate.mapMinute);
	if (!RebuildOpenPriceLocked(p_usVersion, it->second,
		p_refError))
	{
		return false;
	}
	it->second.bM1Ready = true;
	return true;
}

bool CMtQueryQuoteCache::OnM1Increment(
	const ST_DERIVE_M1_SNAPSHOT_RESPONSE& p_refUpdate,
	std::string& p_refError)
{
	p_refError.clear();
	if (p_refUpdate.enState !=
			EN_DERIVE_SERVICE_STATE_READY ||
		p_refUpdate.iCode != 0 ||
		p_refUpdate.aBar.empty())
	{
		p_refError =
			"QUERY_M1_INCREMENT_INVALID: 1252 must contain at least one READY bar";
		return false;
	}
	std::lock_guard<std::mutex> clGuard(m_clMutex);
	for (const ST_DERIVE_M1_BAR& refBar :
		p_refUpdate.aBar)
	{
		const std::pair<std::uint16_t,
			std::int32_t> stKey(
				refBar.usPlatformVersion,
				refBar.iSourceNo);
		std::map<std::pair<std::uint16_t,
			std::int32_t>, ST_SOURCE_STATE>::
			iterator it = m_mapSource.find(stKey);
		if (it == m_mapSource.end() ||
			!ApplyM1Locked(it->second, refBar,
				p_refError))
		{
			if (p_refError.empty())
			{
				p_refError =
					"QUERY_M1_INCREMENT_SOURCE_NOT_CONFIGURED: event Version+No is not enabled";
			}
			return false;
		}
	}
	// 一批 1252 全部通过后统一重建，避免同批多根 M1 反复扫描保留窗口。
	std::set<std::pair<std::uint16_t, std::int32_t>> setChangedSource;
	for (const ST_DERIVE_M1_BAR& refBar : p_refUpdate.aBar)
	{
		setChangedSource.insert(std::make_pair(
			refBar.usPlatformVersion, refBar.iSourceNo));
	}
	for (const auto& refKey : setChangedSource)
	{
		ST_SOURCE_STATE& refState = m_mapSource[refKey];
		if (!RebuildOpenPriceLocked(refKey.first, refState,
			p_refError))
		{
			return false;
		}
	}
	return true;
}

bool CMtQueryQuoteCache::UpdateTimeState(
	const ST_MT_TIME_STATE& p_refState, std::string& p_refError)
{
	if (!ValidateMtTimeState(p_refState, p_refError))
	{
		return false;
	}
	std::lock_guard<std::mutex> clGuard(m_clMutex);
	const auto iterSource = m_mapSource.find(std::make_pair(
		p_refState.usPlatformVersion, p_refState.iSourceNo));
	if (iterSource == m_mapSource.end())
	{
		p_refError = "QUERY_TIME_SOURCE_NOT_FOUND: version=" +
			std::to_string(p_refState.usPlatformVersion) +
			", no=" + std::to_string(p_refState.iSourceNo);
		return false;
	}
	ST_SOURCE_STATE& refSource = iterSource->second;
	if (refSource.ullTimeAuthorityEpoch == p_refState.ullAuthorityEpoch &&
		p_refState.ullGeneration < refSource.ullTimeGeneration)
	{
		return true;
	}
	refSource.iServerTimeOffsetSeconds =
		p_refState.iCurrentOffsetSeconds;
	refSource.strTimeZoneId = p_refState.strTimeZoneId;
	refSource.ullTimeAuthorityEpoch =
		p_refState.ullAuthorityEpoch;
	refSource.ullTimeGeneration = p_refState.ullGeneration;
	refSource.llTimeValidUntilUtcMs =
		p_refState.llValidUntilUtcMs;
	refSource.bTimeReady =
		p_refState.enSyncState == EN_MT_TIME_SYNC_READY &&
		p_refState.llValidUntilUtcMs >
			std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
	// 时区或 DST 代次变化会改变 Broker D1 边界，必须从权威 M1 全量重建首值。
	if (!RebuildOpenPriceLocked(p_refState.usPlatformVersion,
		refSource, p_refError))
	{
		return false;
	}
	return true;
}

bool CMtQueryQuoteCache::GetQuotes(std::uint16_t p_usVersion,
	std::int32_t p_iNo, const std::set<std::string>& p_refSymbols,
	std::vector<ST_QUOTE_BINARY_TICK>& p_refTick,
	ST_MT_QUERY_QUOTE_SOURCE_STATUS& p_refStatus,
	std::string& p_refError) const
{
	p_refTick.clear();
	p_refStatus = ST_MT_QUERY_QUOTE_SOURCE_STATUS();
	p_refError.clear();
	const std::pair<std::uint16_t, std::int32_t> stKey(
		p_usVersion, p_iNo);
	std::lock_guard<std::mutex> clGuard(m_clMutex);
	const std::map<std::pair<std::uint16_t, std::int32_t>,
		ST_SOURCE_STATE>::const_iterator it =
		m_mapSource.find(stKey);
	if (it == m_mapSource.end())
	{
		p_refError =
			"QUERY_QUOTE_SOURCE_NOT_CONFIGURED: requested Version+No is not enabled";
		return false;
	}
	const ST_SOURCE_STATE& refState = it->second;
	p_refStatus.usVersion = p_usVersion;
	p_refStatus.iNo = p_iNo;
	p_refStatus.bSnapshotReady = refState.bSnapshotReady;
	p_refStatus.bM1Ready = refState.bM1Ready;
	p_refStatus.bBufferOverflow = refState.bBufferOverflow;
	p_refStatus.ullSourceEpoch =
		refState.ullSourceEpoch;
	p_refStatus.ullHighWatermark =
		refState.ullHighWatermark;
	p_refStatus.llLastIngressMs =
		refState.llLastIngressMs;
	p_refStatus.szSymbolCount = refState.mapLatest.size();
	p_refStatus.szPendingCount = refState.clPending.size();
	if (!refState.bSnapshotReady)
	{
		p_refError =
			"QUERY_QUOTE_SNAPSHOT_NOT_READY: requested Version+No is still bootstrapping";
		return false;
	}
	for (const std::pair<const std::string,
		ST_QUOTE_BINARY_TICK>& refItem : refState.mapLatest)
	{
		if (p_refSymbols.empty() ||
			p_refSymbols.find(refItem.first) !=
				p_refSymbols.end())
		{
			p_refTick.push_back(refItem.second);
		}
	}
	return true;
}

double CMtQueryQuoteCache::GetOpenPrice(std::uint16_t p_usVersion,
	std::int32_t p_iNo, const std::string& p_refSymbol) const
{
	std::lock_guard<std::mutex> clGuard(m_clMutex);
	const auto itSource = m_mapSource.find(
		std::make_pair(p_usVersion, p_iNo));
	if (itSource == m_mapSource.end())
	{
		return 0.0;
	}
	const auto itOpen = itSource->second.mapOpenPrice.find(p_refSymbol);
	return itOpen != itSource->second.mapOpenPrice.end() ?
		itOpen->second.dOpenPrice : 0.0;
}

bool CMtQueryQuoteCache::GetBars(std::uint16_t p_usVersion,
	std::int32_t p_iNo, const std::string& p_refSymbol,
	std::int64_t p_llIntervalMinutes, std::int64_t p_llFrom,
	std::int64_t p_llTo, std::size_t p_szTailCount,
	std::vector<ST_MT_QUERY_BAR>& p_refBars,
	std::string& p_refError) const
{
	p_refBars.clear();
	p_refError.clear();
	if (p_refSymbol.empty() ||
		!IsSupportedPeriod(p_usVersion, p_llIntervalMinutes) ||
		((p_llFrom > 0 || p_llTo > 0) &&
			(p_llFrom <= 0 || p_llTo < p_llFrom)) ||
		(p_llFrom == 0 && p_llTo == 0 && p_szTailCount == 0))
	{
		p_refError =
			"QUERY_BARS_PARAMETER_INVALID: symbol, interval or range/tail parameters are invalid";
		return false;
	}
	const std::pair<std::uint16_t, std::int32_t> stKey(
		p_usVersion, p_iNo);
	std::lock_guard<std::mutex> clGuard(m_clMutex);
	const std::map<std::pair<std::uint16_t, std::int32_t>,
		ST_SOURCE_STATE>::const_iterator itSource =
		m_mapSource.find(stKey);
	if (itSource == m_mapSource.end() ||
		!itSource->second.bM1Ready)
	{
		p_refError =
			"QUERY_BARS_SOURCE_NOT_READY: requested Version+No has no installed Derive M1 snapshot";
		return false;
	}
	const std::int64_t llNowMs =
		std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
	if (p_llIntervalMinutes >= 1440 &&
		(!itSource->second.bTimeReady ||
			itSource->second.llTimeValidUntilUtcMs <= llNowMs))
	{
		p_refError =
			"QUERY_BARS_TIME_NOT_READY: server calendar aggregation requires a current 1123/1244 time state";
		return false;
	}
	const std::map<std::string,
		std::map<std::int64_t, ST_MT_QUERY_BAR>>::const_iterator
		itSymbol = itSource->second.mapMinute.find(p_refSymbol);
	if (itSymbol == itSource->second.mapMinute.end())
	{
		return true;
	}

	std::map<std::int64_t, ST_MT_QUERY_BAR> mapAggregate;
	for (const std::pair<const std::int64_t,
		ST_MT_QUERY_BAR>& refItem : itSymbol->second)
	{
		if (p_llFrom > 0 &&
			(refItem.first < p_llFrom ||
				refItem.first > p_llTo))
		{
			continue;
		}
		std::int64_t llBucket = 0;
		if (!ResolveBucket(p_usVersion,
			itSource->second.strTimeZoneId, refItem.first,
			p_llIntervalMinutes, llBucket, p_refError))
		{
			return false;
		}
		std::map<std::int64_t,
			ST_MT_QUERY_BAR>::iterator itBar =
			mapAggregate.find(llBucket);
		if (itBar == mapAggregate.end())
		{
			ST_MT_QUERY_BAR stBar = refItem.second;
			stBar.llDateTime = llBucket;
			mapAggregate[llBucket] = stBar;
		}
		else
		{
			ST_MT_QUERY_BAR& refBar = itBar->second;
			refBar.dHigh = (std::max)(refBar.dHigh,
				refItem.second.dHigh);
			refBar.dLow = (std::min)(refBar.dLow,
				refItem.second.dLow);
			refBar.dClose = refItem.second.dClose;
			refBar.ullTickVolume +=
				refItem.second.ullTickVolume;
			refBar.ullRealVolume +=
				refItem.second.ullRealVolume;
			refBar.ullSourceEpoch =
				refItem.second.ullSourceEpoch;
			refBar.ullLastSequence =
				refItem.second.ullLastSequence;
		}
	}
	for (const std::pair<const std::int64_t,
		ST_MT_QUERY_BAR>& refItem : mapAggregate)
	{
		p_refBars.push_back(refItem.second);
	}
	if (p_llFrom == 0 && p_szTailCount > 0 &&
		p_refBars.size() > p_szTailCount)
	{
		p_refBars.erase(p_refBars.begin(),
			p_refBars.end() -
			static_cast<std::ptrdiff_t>(p_szTailCount));
	}
	return true;
}

void CMtQueryQuoteCache::GetStatus(
	std::vector<ST_MT_QUERY_QUOTE_SOURCE_STATUS>& p_refStatus) const
{
	p_refStatus.clear();
	std::lock_guard<std::mutex> clGuard(m_clMutex);
	for (const std::pair<const std::pair<std::uint16_t,
		std::int32_t>, ST_SOURCE_STATE>& refItem : m_mapSource)
	{
		ST_MT_QUERY_QUOTE_SOURCE_STATUS stStatus;
		stStatus.usVersion = refItem.first.first;
		stStatus.iNo = refItem.first.second;
		stStatus.bSnapshotReady =
			refItem.second.bSnapshotReady;
		stStatus.bM1Ready =
			refItem.second.bM1Ready;
		stStatus.bBufferOverflow =
			refItem.second.bBufferOverflow;
		stStatus.ullSourceEpoch =
			refItem.second.ullSourceEpoch;
		stStatus.ullHighWatermark =
			refItem.second.ullHighWatermark;
		stStatus.llLastIngressMs =
			refItem.second.llLastIngressMs;
		const std::int64_t llNowMs =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count();
		stStatus.bTimeReady =
			refItem.second.bTimeReady &&
			refItem.second.llTimeValidUntilUtcMs > llNowMs;
		stStatus.strTimeZoneId =
			refItem.second.strTimeZoneId;
		stStatus.iTimeOffsetSeconds =
			refItem.second.iServerTimeOffsetSeconds;
		stStatus.ullTimeAuthorityEpoch =
			refItem.second.ullTimeAuthorityEpoch;
		stStatus.ullTimeGeneration =
			refItem.second.ullTimeGeneration;
		stStatus.llTimeValidUntilUtcMs =
			refItem.second.llTimeValidUntilUtcMs;
		stStatus.szSymbolCount =
			refItem.second.mapLatest.size();
		stStatus.szPendingCount =
			refItem.second.clPending.size();
		p_refStatus.push_back(stStatus);
	}
}

void CMtQueryQuoteCache::Clear()
{
	std::lock_guard<std::mutex> clGuard(m_clMutex);
	m_mapSource.clear();
	m_szBufferCapacity = 0;
	m_szMinuteRetention = 0;
}

void CMtQueryQuoteCache::ApplyTickLocked(
	ST_SOURCE_STATE& p_refState,
	const ST_QUOTE_BINARY_TICK& p_refTick)
{
	p_refState.ullSourceEpoch =
		p_refTick.ullSourceEpoch;
	p_refState.ullHighWatermark =
		p_refTick.ullIngressSequence;
	p_refState.llLastIngressMs = (std::max)(
		p_refState.llLastIngressMs,
		p_refTick.llIngressTimeMs);
	p_refState.mapLatest[p_refTick.strSymbol] = p_refTick;
}

bool CMtQueryQuoteCache::ApplyIncrementLocked(
	ST_SOURCE_STATE& p_refState,
	const ST_QUOTE_BINARY_TICK& p_refTick)
{
	if (p_refState.ullSourceEpoch ==
		p_refTick.ullSourceEpoch)
	{
		if (p_refTick.ullIngressSequence <=
			p_refState.ullHighWatermark)
		{
			return false;
		}
		ApplyTickLocked(p_refState, p_refTick);
		return true;
	}
	if (p_refState.setRetiredEpoch.find(
			p_refTick.ullSourceEpoch) !=
		p_refState.setRetiredEpoch.end())
	{
		// 新代次已开始后，旧进程在网络中的迟到包只能幂等丢弃，不能把缓存切回旧代次。
		return false;
	}
	if (p_refState.ullSourceEpoch != 0)
	{
		p_refState.setRetiredEpoch.insert(
			p_refState.ullSourceEpoch);
	}
	p_refState.ullSourceEpoch =
		p_refTick.ullSourceEpoch;
	p_refState.ullHighWatermark = 0;
	ApplyTickLocked(p_refState, p_refTick);
	return true;
}

bool CMtQueryQuoteCache::ApplyM1Locked(
	ST_SOURCE_STATE& p_refState,
	const ST_DERIVE_M1_BAR& p_refBar,
	std::string& p_refError)
{
	if (p_refBar.strSymbol.empty() ||
		p_refBar.llMinute <= 0 ||
		p_refBar.ullSourceEpoch == 0 ||
		p_refBar.ullFirstSequence == 0 ||
		p_refBar.ullLastSequence <
			p_refBar.ullFirstSequence ||
		p_refBar.ullTickVolume == 0 ||
		p_refBar.dOpen <= 0.0 ||
		p_refBar.dHigh <
			(std::max)(p_refBar.dOpen,
				p_refBar.dClose) ||
		p_refBar.dLow >
			(std::min)(p_refBar.dOpen,
				p_refBar.dClose))
	{
		p_refError =
			"QUERY_M1_BAR_INVALID: symbol, minute, epoch, sequence, volume or OHLC is invalid";
		return false;
	}
	std::map<std::int64_t, ST_MT_QUERY_BAR>& refMinute =
		p_refState.mapMinute[p_refBar.strSymbol];
	std::map<std::int64_t, ST_MT_QUERY_BAR>::iterator it =
		refMinute.find(p_refBar.llMinute);
	if (it != refMinute.end())
	{
		// 同一来源代次只接受最终序号更新的状态；旧代次通知不能覆盖新代次。
		if (it->second.ullSourceEpoch >
				p_refBar.ullSourceEpoch ||
			(it->second.ullSourceEpoch ==
				p_refBar.ullSourceEpoch &&
			 it->second.ullLastSequence >=
				p_refBar.ullLastSequence))
		{
			return true;
		}
	}
	ST_MT_QUERY_BAR stBar;
	stBar.llDateTime = p_refBar.llMinute;
	stBar.uiServerDate = p_refBar.uiServerDate;
	stBar.uiFlags = p_refBar.uiFlags;
	stBar.dOpen = p_refBar.dOpen;
	stBar.dHigh = p_refBar.dHigh;
	stBar.dLow = p_refBar.dLow;
	stBar.dClose = p_refBar.dClose;
	stBar.ullTickVolume =
		p_refBar.ullTickVolume;
	stBar.ullRealVolume =
		p_refBar.ullRealVolume;
	stBar.ullSourceEpoch =
		p_refBar.ullSourceEpoch;
	stBar.ullLastSequence =
		p_refBar.ullLastSequence;
	refMinute[p_refBar.llMinute] = stBar;
	while (refMinute.size() > m_szMinuteRetention)
	{
		refMinute.erase(refMinute.begin());
	}
	return true;
}

bool CMtQueryQuoteCache::RebuildOpenPriceLocked(
	std::uint16_t p_usVersion, ST_SOURCE_STATE& p_refState,
	std::string& p_refError)
{
	p_refState.mapOpenPrice.clear();
	if (p_refState.strTimeZoneId.empty())
	{
		return true;
	}
	for (const auto& refSymbol : p_refState.mapMinute)
	{
		ST_SOURCE_STATE::ST_OPEN_PRICE_STATE stOpen;
		for (const auto& refMinute : refSymbol.second)
		{
			if ((refMinute.second.uiFlags &
				DERIVE_M1_FLAG_OPEN_PRICE_CONFIRMED) == 0)
			{
				continue;
			}
			std::int64_t llDayBucket =
				static_cast<std::int64_t>(refMinute.second.uiServerDate);
			if (llDayBucket == 0 &&
				!ResolveBucket(p_usVersion, p_refState.strTimeZoneId,
					refMinute.first, 1440, llDayBucket, p_refError))
			{
				p_refState.mapOpenPrice.clear();
				return false;
			}
			if (llDayBucket > stOpen.llDayBucket ||
				(llDayBucket == stOpen.llDayBucket &&
				 (stOpen.llFirstMinute == 0 ||
				  refMinute.first < stOpen.llFirstMinute)))
			{
				stOpen.llDayBucket = llDayBucket;
				stOpen.llFirstMinute = refMinute.first;
				stOpen.dOpenPrice = refMinute.second.dOpen;
			}
		}
		if (stOpen.dOpenPrice > 0.0)
		{
			p_refState.mapOpenPrice[refSymbol.first] = stOpen;
		}
	}
	return true;
}

bool CMtQueryQuoteCache::ResolveBucket(
	std::uint16_t p_usVersion,
	const std::string& p_refTimeZoneId,
	std::int64_t p_llUtcSeconds,
	std::int64_t p_llIntervalMinutes,
	std::int64_t& p_refBucketUtcSeconds,
	std::string& p_refError)
{
	// Derive 内存 M1 与 SDK 历史使用同一转换器，避免重叠区因 DST 边界不同落入两个周期。
	return CMtQueryTimeZone::ResolveBarBucket(p_usVersion,
		p_refTimeZoneId, p_llUtcSeconds, p_llIntervalMinutes,
		p_refBucketUtcSeconds, p_refError);
}

bool CMtQueryQuoteCache::IsSupportedPeriod(
	std::uint16_t p_usVersion, std::int64_t p_llIntervalMinutes)
{
	static const std::int64_t aMt4Period[] =
		{1, 5, 15, 30, 60, 240, 1440, 10080, 43200};
	static const std::int64_t aMt5Period[] =
		{1, 2, 3, 4, 5, 6, 10, 12, 15, 20, 30, 60,
			120, 180, 240, 360, 480, 720, 1440, 10080, 43200};
	const std::int64_t* pBegin =
		p_usVersion == 4 ? aMt4Period : aMt5Period;
	const std::size_t szCount = p_usVersion == 4 ?
		sizeof(aMt4Period) / sizeof(aMt4Period[0]) :
		sizeof(aMt5Period) / sizeof(aMt5Period[0]);
	return (p_usVersion == 4 || p_usVersion == 5) &&
		std::find(pBegin, pBegin + szCount,
			p_llIntervalMinutes) != pBegin + szCount;
}
