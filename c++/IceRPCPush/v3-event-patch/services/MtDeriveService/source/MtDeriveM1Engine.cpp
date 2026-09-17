#include "MtDeriveM1Engine.h"

#include <algorithm>
#include <chrono>

CMtDeriveM1Engine::ST_SOURCE_STATE::ST_SOURCE_STATE()
	: ullSourceEpoch(0)
	, ullTimeAuthorityEpoch(0)
	, ullTimeGeneration(0)
	, iTimeOffsetSeconds(0)
	, llTimeValidUntilUtcMs(0)
	, bTimeReady(false)
{
}

CMtDeriveM1Engine::CMtDeriveM1Engine()
	: m_clMutex()
	, m_mapSource()
	, m_clMinuteCenter()
{
}

CMtDeriveM1Engine::~CMtDeriveM1Engine()
{
	Clear();
}

bool CMtDeriveM1Engine::Configure(
	const ST_MT_DERIVE_SERVICE_CONFIG& p_refConfig,
	std::string& p_refError)
{
	p_refError.clear();
	if (p_refConfig.stTick.szMinuteRetention == 0)
	{
		p_refError =
			"DERIVE_M1_RETENTION_INVALID: minute retention must be positive";
		return false;
	}
	if (!m_clMinuteCenter.Configure(p_refConfig, p_refError))
	{
		return false;
	}
	std::lock_guard<std::mutex> clLock(m_clMutex);
	m_mapSource.clear();
	for (const ST_MT_DERIVE_SOURCE_CONFIG& refSource :
		p_refConfig.aSource)
	{
		if (!refSource.bEnable)
		{
			continue;
		}
		const std::pair<std::uint16_t, std::int32_t>
			stKey(refSource.usVersion, refSource.iNo);
		if (!m_mapSource.insert(std::make_pair(
				stKey, ST_SOURCE_STATE())).second)
		{
			p_refError =
				"DERIVE_M1_SOURCE_DUPLICATE: Version+No already exists";
			m_mapSource.clear();
			m_clMinuteCenter.Clear();
			return false;
		}
	}
	if (m_mapSource.empty())
	{
		p_refError =
			"DERIVE_M1_SOURCE_EMPTY: no enabled source is configured";
		m_clMinuteCenter.Clear();
		return false;
	}
	return true;
}

bool CMtDeriveM1Engine::ReconcileSymbols(
	std::uint16_t p_usVersion, std::int32_t p_iNo,
	const std::vector<std::string>& p_refSymbols,
	std::string& p_refError)
{
	std::lock_guard<std::mutex> clLock(m_clMutex);
	if (m_mapSource.count(std::make_pair(p_usVersion, p_iNo)) == 0)
	{
		p_refError = "DERIVE_M1_SYMBOL_SOURCE_NOT_CONFIGURED";
		return false;
	}
	return m_clMinuteCenter.ReconcileSymbols(
		p_usVersion, p_iNo, p_refSymbols, p_refError);
}

bool CMtDeriveM1Engine::MarkSymbolActive(
	std::uint16_t p_usVersion, std::int32_t p_iNo,
	const std::string& p_refSymbol, std::string& p_refError)
{
	std::lock_guard<std::mutex> clLock(m_clMutex);
	if (m_mapSource.count(std::make_pair(p_usVersion, p_iNo)) == 0)
	{
		p_refError = "DERIVE_M1_SYMBOL_SOURCE_NOT_CONFIGURED";
		return false;
	}
	return m_clMinuteCenter.MarkSymbolActive(
		p_usVersion, p_iNo, p_refSymbol, p_refError);
}

bool CMtDeriveM1Engine::MarkSymbolDeleted(
	std::uint16_t p_usVersion, std::int32_t p_iNo,
	const std::string& p_refSymbol, std::string& p_refError)
{
	std::lock_guard<std::mutex> clLock(m_clMutex);
	if (m_mapSource.count(std::make_pair(p_usVersion, p_iNo)) == 0)
	{
		p_refError = "DERIVE_M1_SYMBOL_SOURCE_NOT_CONFIGURED";
		return false;
	}
	return m_clMinuteCenter.MarkSymbolDeleted(
		p_usVersion, p_iNo, p_refSymbol, p_refError);
}

bool CMtDeriveM1Engine::ApplyTick(
	const ST_QUOTE_BINARY_TICK& p_refTick,
	ST_DERIVE_M1_BAR& p_refChangedBar,
	std::string& p_refError)
{
	p_refChangedBar = ST_DERIVE_M1_BAR();
	if (!ValidateQuoteBinaryTick(p_refTick, p_refError))
	{
		return false;
	}
	const double dPrice = p_refTick.dLast > 0.0 ?
		p_refTick.dLast : p_refTick.dBid;
	if (dPrice <= 0.0 || p_refTick.llServerTime <= 0)
	{
		p_refError =
			"DERIVE_M1_TICK_INVALID: Tick has no positive price or market minute";
		return false;
	}

	const std::pair<std::uint16_t, std::int32_t>
		stKey(p_refTick.usPlatformVersion,
			p_refTick.iSourceNo);
	std::lock_guard<std::mutex> clLock(m_clMutex);
	std::map<std::pair<std::uint16_t, std::int32_t>,
		ST_SOURCE_STATE>::iterator itSource =
		m_mapSource.find(stKey);
	if (itSource == m_mapSource.end())
	{
		p_refError =
			"DERIVE_M1_SOURCE_NOT_CONFIGURED: Tick Version+No is disabled";
		return false;
	}
	ST_SOURCE_STATE& refSource = itSource->second;
	const std::int64_t llNowMs =
		std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
	// Quote v1 不携带时间权威；Derive 必须使用独立状态把原始服务器时间转换为 UTC。
	if (!refSource.bTimeReady || refSource.ullTimeAuthorityEpoch == 0 ||
		refSource.ullTimeGeneration == 0 ||
		refSource.llTimeValidUntilUtcMs <= llNowMs)
	{
		p_refError =
			"DERIVE_M1_TIME_NOT_READY: current 1123/1244 time state is missing or stale";
		return false;
	}
	const std::int64_t llUtcTime = p_refTick.llServerTime -
		static_cast<std::int64_t>(refSource.iTimeOffsetSeconds);
	if (llUtcTime <= 0)
	{
		p_refError =
			"DERIVE_M1_UTC_TIME_INVALID: converted UTC Tick time is not positive";
		return false;
	}
	refSource.ullSourceEpoch = p_refTick.ullSourceEpoch;
	return m_clMinuteCenter.ApplyTick(p_refTick,
		llUtcTime, refSource.ullTimeAuthorityEpoch,
		refSource.ullTimeGeneration,
		p_refChangedBar, p_refError);
}

bool CMtDeriveM1Engine::UpdateTimeState(
	const ST_MT_TIME_STATE& p_refState,
	std::string& p_refError)
{
	if (!ValidateMtTimeState(p_refState, p_refError))
	{
		return false;
	}
	std::lock_guard<std::mutex> clLock(m_clMutex);
	const auto itSource = m_mapSource.find(std::make_pair(
		p_refState.usPlatformVersion, p_refState.iSourceNo));
	if (itSource == m_mapSource.end())
	{
		p_refError = "DERIVE_M1_TIME_SOURCE_NOT_CONFIGURED: version=" +
			std::to_string(p_refState.usPlatformVersion) +
			", no=" + std::to_string(p_refState.iSourceNo);
		return false;
	}
	ST_SOURCE_STATE& refSource = itSource->second;
	if (refSource.ullTimeAuthorityEpoch ==
			p_refState.ullAuthorityEpoch &&
		p_refState.ullGeneration <
			refSource.ullTimeGeneration)
	{
		return true;
	}
	refSource.ullTimeAuthorityEpoch =
		p_refState.ullAuthorityEpoch;
	refSource.ullTimeGeneration =
		p_refState.ullGeneration;
	refSource.iTimeOffsetSeconds =
		p_refState.iCurrentOffsetSeconds;
	refSource.llTimeValidUntilUtcMs =
		p_refState.llValidUntilUtcMs;
	refSource.bTimeReady =
		p_refState.enSyncState == EN_MT_TIME_SYNC_READY;
	return true;
}

bool CMtDeriveM1Engine::Snapshot(
	const ST_DERIVE_M1_SNAPSHOT_REQUEST& p_refRequest,
	ST_DERIVE_M1_SNAPSHOT_RESPONSE& p_refResponse,
	std::string& p_refError) const
{
	p_refResponse = ST_DERIVE_M1_SNAPSHOT_RESPONSE();
	p_refError.clear();
	if ((p_refRequest.usPlatformVersion != 4 &&
		 p_refRequest.usPlatformVersion != 5) ||
		p_refRequest.iSourceNo <= 0 ||
		p_refRequest.uiMaxBars == 0 ||
		p_refRequest.uiMaxBars >
			DERIVE_BINARY_MAX_SNAPSHOT_ITEMS)
	{
		p_refError =
			"DERIVE_M1_SNAPSHOT_REQUEST_INVALID: source or maxBars is invalid";
		return false;
	}
	return m_clMinuteCenter.Snapshot(p_refRequest,
		p_refResponse, p_refError);
}

bool CMtDeriveM1Engine::Restore(
	std::uint16_t p_usVersion, std::int32_t p_iNo,
	const std::vector<ST_DERIVE_M1_BAR>& p_refBar,
	std::string& p_refError)
{
	p_refError.clear();
	const std::pair<std::uint16_t, std::int32_t>
		stKey(p_usVersion, p_iNo);
	std::lock_guard<std::mutex> clLock(m_clMutex);
	std::map<std::pair<std::uint16_t, std::int32_t>,
		ST_SOURCE_STATE>::iterator itSource =
		m_mapSource.find(stKey);
	if (itSource == m_mapSource.end())
	{
		p_refError =
			"DERIVE_M1_RESTORE_SOURCE_NOT_CONFIGURED";
		return false;
	}
	for (const ST_DERIVE_M1_BAR& refBar : p_refBar)
	{
		if (refBar.usPlatformVersion != p_usVersion ||
			refBar.iSourceNo != p_iNo ||
			refBar.strSymbol.empty() ||
			refBar.llMinute <= 0 ||
			refBar.ullLastSequence == 0)
		{
			p_refError =
				"DERIVE_M1_RESTORE_BAR_INVALID: checkpoint contains mismatched Bar";
			return false;
		}
		itSource->second.ullSourceEpoch = refBar.ullSourceEpoch;
	}
	return m_clMinuteCenter.RestoreLegacy(p_usVersion,
		p_iNo, p_refBar, p_refError);
}

bool CMtDeriveM1Engine::MergeBackfill(
	const ST_QUERY_M1_BACKFILL_RESPONSE& p_refResponse,
	std::string& p_refError)
{
	// RPC 往返期间 1123/1244 可能切换；旧代次结果禁止进入当前双槽。
	std::lock_guard<std::mutex> clLock(m_clMutex);
	const auto itSource = m_mapSource.find(std::make_pair(
		p_refResponse.usPlatformVersion, p_refResponse.iSourceNo));
	if (itSource == m_mapSource.end())
	{
		p_refError = "DERIVE_M1_BACKFILL_SOURCE_NOT_CONFIGURED";
		return false;
	}
	if (itSource->second.ullTimeAuthorityEpoch != 0 &&
		(p_refResponse.ullTimeAuthorityEpoch !=
			itSource->second.ullTimeAuthorityEpoch ||
		 p_refResponse.ullTimeGeneration !=
			itSource->second.ullTimeGeneration))
	{
		p_refError =
			"DERIVE_M1_BACKFILL_TIME_GENERATION_CHANGED: response no longer matches current 1123/1244";
		return false;
	}
	return m_clMinuteCenter.MergeBackfill(p_refResponse, p_refError);
}

bool CMtDeriveM1Engine::FlushPersistent(std::string& p_refError)
{
	return m_clMinuteCenter.FlushDirty(p_refError);
}

void CMtDeriveM1Engine::ListArchiveCandidates(
	std::vector<ST_DERIVE_MINUTE_ARCHIVE_CANDIDATE>& p_refCandidate) const
{
	m_clMinuteCenter.ListArchiveCandidates(p_refCandidate);
}

void CMtDeriveM1Engine::ListCurrentBackfillCandidates(
	std::vector<ST_DERIVE_MINUTE_BACKFILL_CANDIDATE>& p_refCandidate) const
{
	m_clMinuteCenter.ListCurrentBackfillCandidates(p_refCandidate);
}

bool CMtDeriveM1Engine::BuildArchiveBatch(
	const ST_DERIVE_MINUTE_ARCHIVE_CANDIDATE& p_refCandidate,
	std::int64_t p_llFromMinute, std::int64_t p_llToMinute,
	const std::string& p_refArchiveId,
	const std::string& p_refOwnerInstanceId,
	std::uint64_t p_ullLeaseGeneration,
	ST_DERIVE_M1_ARCHIVE_BATCH& p_refBatch,
	std::string& p_refError) const
{
	return m_clMinuteCenter.BuildArchiveBatch(p_refCandidate,
		p_llFromMinute, p_llToMinute, p_refArchiveId,
		p_refOwnerInstanceId, p_ullLeaseGeneration,
		p_refBatch, p_refError);
}

bool CMtDeriveM1Engine::MarkArchiveSpooled(
	const ST_DERIVE_MINUTE_ARCHIVE_CANDIDATE& p_refCandidate,
	std::int64_t p_llArchivedTo, std::string& p_refError)
{
	return m_clMinuteCenter.MarkArchiveSpooled(
		p_refCandidate, p_llArchivedTo, p_refError);
}

std::uint32_t CMtDeriveM1Engine::GetSymbolCount(
	std::uint16_t p_usVersion, std::int32_t p_iNo) const
{
	return m_clMinuteCenter.GetSymbolCount(p_usVersion, p_iNo);
}

void CMtDeriveM1Engine::GetSourceDiagnostics(
	std::uint16_t p_usVersion, std::int32_t p_iNo,
	ST_DERIVE_MINUTE_SOURCE_DIAGNOSTICS& p_refDiagnostics) const
{
	m_clMinuteCenter.GetSourceDiagnostics(
		p_usVersion, p_iNo, p_refDiagnostics);
}

void CMtDeriveM1Engine::Clear()
{
	std::lock_guard<std::mutex> clLock(m_clMutex);
	m_mapSource.clear();
	m_clMinuteCenter.Clear();
}
