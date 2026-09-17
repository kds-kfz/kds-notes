#include "MtQuoteServiceApp.h"

#include "ClientDataBinaryProtocol.h"
#include "CodeMsg.h"
#include "Log.h"
#include "PluginBinaryProtocol.h"
#include "QuoteBinaryProtocol.h"
#include "QuoteHeartbeatBinaryProtocol.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <algorithm>
#include <chrono>
#include <limits>
#include <memory>
#include <vector>

CMtQuoteServiceApp::CMtQuoteServiceApp()
	: CServiceApplication(EN_PLUGIN_ID_MT_QUOTE_SERVICE, "MtQuoteService")
	, m_stQuoteConfig()
	, m_clQuoteManager()
	, m_ullProcessEpoch(0)
	, m_bQuoteComponentsStarted(false)
{
}

CMtQuoteServiceApp::~CMtQuoteServiceApp()
{
	OnStopping();
}

bool CMtQuoteServiceApp::OnConfigure(std::string& p_refError)
{
	if (!LoadMtQuoteServiceConfig(GetConfigPath(), GetExeDirectory(),
		m_stQuoteConfig, p_refError))
	{
		return false;
	}
	m_bQuoteComponentsStarted.store(false);
	m_ullProcessEpoch = GenerateProcessEpoch();
	std::size_t szEnabled = 0;
	for (const ST_MT_QUOTE_SOURCE_CONFIG& refSource :
		m_stQuoteConfig.aSource)
	{
		if (!refSource.bEnable)
		{
			continue;
		}
		++szEnabled;
		if (!RegisterClusterShard(refSource.usVersion, refSource.iNo,
			p_refError))
		{
			return false;
		}
	}
	if (szEnabled == 0 && !m_stQuoteConfig.bAllowDegradedStart)
	{
		p_refError =
			"QUOTE_CONFIG_NO_ENABLED_SOURCE: at least one Source must have enable=1";
		return false;
	}
	if (szEnabled == 0)
	{
		MT_WARN(
			"quote service configured degraded,reason=QUOTE_SOURCE_DISABLED,service=%s",
			GetServiceName().c_str());
		return true;
	}
	MT_INFO(
		"quote config loaded,sourceCount=%zu,enabledCount=%zu,processEpoch=%llu",
		m_stQuoteConfig.aSource.size(), szEnabled,
		static_cast<unsigned long long>(m_ullProcessEpoch));
	return true;
}

bool CMtQuoteServiceApp::OnStarted(std::string& p_refError)
{
	std::size_t szEnabled = 0;
	for (const ST_MT_QUOTE_SOURCE_CONFIG& refSource :
		m_stQuoteConfig.aSource)
	{
		if (refSource.bEnable)
		{
			++szEnabled;
		}
	}
	if (szEnabled == 0 && m_stQuoteConfig.bAllowDegradedStart)
	{
		p_refError.clear();
		MT_WARN(
			"quote service started degraded,state=DEGRADED,reason=QUOTE_SOURCE_DISABLED");
		return true;
	}
	if (!m_clQuoteManager.Start(m_stQuoteConfig, this, p_refError))
	{
		return false;
	}
	m_bQuoteComponentsStarted.store(true);
	for (const ST_MT_QUOTE_SOURCE_CONFIG& refSource :
		m_stQuoteConfig.aSource)
	{
		if (!refSource.bEnable ||
			!HasClusterLease(refSource.usVersion, refSource.iNo))
		{
			continue;
		}
		ST_CLUSTER_FENCE stFence;
		if (!GetClusterFence(refSource.usVersion, refSource.iNo,
				stFence, p_refError) ||
			!m_clQuoteManager.ActivateShard(refSource.usVersion,
				refSource.iNo, stFence.ullLeaseGeneration, p_refError) ||
			!SetClusterShardReady(refSource.usVersion, refSource.iNo,
				true, p_refError))
		{
			m_bQuoteComponentsStarted.store(false);
			m_clQuoteManager.Stop();
			return false;
		}
	}
	MT_INFO("quote manager started,service=%s",
		GetServiceName().c_str());
	return true;
}

void CMtQuoteServiceApp::OnStopping()
{
	if (!m_bQuoteComponentsStarted.exchange(false))
	{
		return;
	}
	m_clQuoteManager.Stop();
}

bool CMtQuoteServiceApp::HandleBusinessRequest(
	const ST_CLOUD_NET_BINARY_REQUEST* p_pRequest,
	ST_CLOUD_NET_BINARY_RESULT* p_pResult)
{
	if (p_pRequest == nullptr || p_pResult == nullptr ||
		p_pRequest->lFuncId != EN_PLUGIN_FUNC_QUOTE_HEARTBEAT)
	{
		return false;
	}
	ST_QUOTE_HEARTBEAT_REQUEST stRequest;
	std::string strError;
	if (p_pRequest->stPayload.pBuffer == nullptr ||
		p_pRequest->stPayload.iLen <= 0 ||
		!DecodeQuoteHeartbeatRequest(p_pRequest->stPayload.pBuffer,
			static_cast<std::size_t>(p_pRequest->stPayload.iLen),
			stRequest, strError))
	{
		SetBinaryResultError(p_pResult, EN_TERMINAL_ERROR_PROTOCOL_ERROR,
			strError.empty() ?
				"QUOTE_HEARTBEAT_REQUEST_INVALID: Binary payload is required" :
				strError.c_str());
		return true;
	}

	ST_QUOTE_HEARTBEAT_RESPONSE stResponse;
	stResponse.enSenderPluginNo = EN_SERVICE_HEARTBEAT_PLUGIN_QUOTE;
	stResponse.enTargetPluginNo = EN_SERVICE_HEARTBEAT_PLUGIN_DERIVE;
	stResponse.ullRequestSequence = stRequest.ullRequestSequence;
	stResponse.ullSenderProcessEpoch = m_ullProcessEpoch;
	stResponse.llRespondedAtMs = GetTimestampMs();
	std::vector<ST_MT_QUOTE_SOURCE_STATUS> aStatus;
	m_clQuoteManager.GetStatus(aStatus);
	for (const ST_MT_QUOTE_SOURCE_STATUS& refStatus : aStatus)
	{
		ST_QUOTE_HEARTBEAT_SOURCE_STATE stSource;
		stSource.usPlatformVersion = refStatus.usVersion;
		stSource.iSourceNo = refStatus.iNo;
		stSource.uiActivePriority = refStatus.uiActivePriority;
		stSource.uiQueueDepth = static_cast<std::uint32_t>(
			(std::min)(refStatus.szQueueDepth,
				static_cast<std::size_t>(
					(std::numeric_limits<std::uint32_t>::max)())));
		stSource.ullSourceEpoch = refStatus.ullSourceEpoch;
		stSource.ullLastSequence = refStatus.ullLastSequence;
		stSource.llLastTickTimeMs = refStatus.llLastTickTimeMs;
		stSource.ullSwitchCount = refStatus.ullSwitchCount;
		stSource.ullDroppedCount = refStatus.ullCapacityDropped +
			refStatus.ullResetDiscarded;
		stSource.ullPublishFailedCount = refStatus.ullPublishFailed;
		for (const ST_MT_QUOTE_CONNECTION_STATUS& refConnection :
			refStatus.aConnection)
		{
			stSource.ullReconnectCount += refConnection.ullReconnectCount;
			if (refConnection.bActive)
			{
				stSource.usConnectionState = static_cast<std::uint16_t>(
					refConnection.enState);
			}
		}
		stResponse.aSource.push_back(stSource);
	}
	std::vector<unsigned char> aResponse;
	if (!EncodeQuoteHeartbeatResponse(stResponse, aResponse, strError))
	{
		SetBinaryResultError(p_pResult, EN_TERMINAL_ERROR_PROTOCOL_ERROR,
			strError.c_str());
		return true;
	}
	p_pResult->lParam = static_cast<long long>(stResponse.aSource.size());
	SetBinaryResultPayload(p_pResult,
		aResponse.empty() ? nullptr : aResponse.data(), aResponse.size());
	return true;
}

bool CMtQuoteServiceApp::OnQuoteReady(
	const ST_QUOTE_BINARY_TICK& p_refTick, std::string& p_refError)
{
	p_refError.clear();
	if (!IsClusterOwner(p_refTick.usPlatformVersion, p_refTick.iSourceNo))
	{
		p_refError =
			"SERVICE_NOT_OWNER: MtQuoteService does not own the requested Version+No";
		return false;
	}
	ST_QUOTE_BINARY_TICK stTick = p_refTick;
	stTick.usVersion = QUOTE_BINARY_PROTOCOL_VERSION;

	// Quote v1 只编码原始 Tick；时间权威、业务价格和集群 fencing 不进入行情正文。
	std::vector<unsigned char> aQuotePayload;
	if (!EncodeQuoteBinaryTick(stTick, aQuotePayload, p_refError))
	{
		return false;
	}
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refQuote =
		CreatePluginBinaryValue(EN_PLUGIN_BINARY_VALUE_BYTES);
	if (!refQuote)
	{
		p_refError =
			"QUOTE_EVENT_ALLOCATION_FAILED: unable to allocate Binary value";
		return false;
	}
	refQuote->aByteValue.swap(aQuotePayload);
	ST_CLIENT_DATA_BINARY_EVENT stEvent;
	stEvent.ullTopic = EN_CLIENT_DATA_BINARY_TOPIC_QUOTE;
	stEvent.iSourceVersion = stTick.usPlatformVersion;
	stEvent.iSourceNo = stTick.iSourceNo;
	stEvent.strSymbol = stTick.strSymbol;
	stEvent.ullSequence = stTick.ullIngressSequence;
	stEvent.llTimestampMs = stTick.llIngressTimeMs;
	stEvent.refData = refQuote;
	std::vector<unsigned char> aEventPayload;
	if (!EncodeClientDataBinaryEvent(stEvent, aEventPayload, p_refError))
	{
		return false;
	}
	return PublishPluginNotification(EN_PLUGIN_NOTIFY_MARKET_TICK,
		EN_PLUGIN_NOTIFY_MODE_BEST_EFFORT,
		EN_PLUGIN_NOTIFY_ACTION_UPDATED,
		stTick.ullIngressSequence, stTick.llIngressTimeMs,
		aEventPayload.data(), aEventPayload.size(), p_refError);
}

void CMtQuoteServiceApp::OnClusterRoleChanged(
	const ST_CLUSTER_LEASE& p_refLease)
{
	CServiceApplication::OnClusterRoleChanged(p_refLease);
	if (!m_bQuoteComponentsStarted.load())
	{
		return;
	}
	if (p_refLease.enRole != EN_CLUSTER_ROLE_RECOVERING)
	{
		m_clQuoteManager.DeactivateShard(p_refLease.usPlatformVersion,
			p_refLease.iSourceNo);
		return;
	}
	std::string strError;
	if (!m_clQuoteManager.ActivateShard(p_refLease.usPlatformVersion,
			p_refLease.iSourceNo, p_refLease.ullLeaseGeneration, strError) ||
		!SetClusterShardReady(p_refLease.usPlatformVersion,
			p_refLease.iSourceNo, true, strError))
	{
		MT_ERROR(
			"quote shard activation failed,version=%u,no=%d,generation=%llu,detail=%s",
			static_cast<unsigned int>(p_refLease.usPlatformVersion),
			p_refLease.iSourceNo,
			static_cast<unsigned long long>(p_refLease.ullLeaseGeneration),
			strError.c_str());
	}
}

void CMtQuoteServiceApp::AppendRuntimeMetrics(
	std::vector<ST_CLUSTER_RUNTIME_METRIC>& p_refMetric)
{
	std::vector<ST_MT_QUOTE_SOURCE_STATUS> aStatus;
	m_clQuoteManager.GetStatus(aStatus);
	for (const ST_MT_QUOTE_SOURCE_STATUS& refStatus : aStatus)
	{
		const std::string strPrefix = "quote.source.mt" +
			std::to_string(refStatus.usVersion) + "." +
			std::to_string(refStatus.iNo) + ".";
		const auto fnAdd = [&p_refMetric, &strPrefix](
			const char* p_szSuffix, std::uint64_t p_ullValue)
		{
			if (p_refMetric.size() >= CLUSTER_MAX_METRIC_COUNT)
			{
				return;
			}
			ST_CLUSTER_RUNTIME_METRIC stMetric;
			stMetric.strName = strPrefix + p_szSuffix;
			stMetric.ullValue = p_ullValue;
			p_refMetric.push_back(std::move(stMetric));
		};
		fnAdd("active", refStatus.bActive ? 1U : 0U);
		fnAdd("queue_capacity", refStatus.szQueueCapacity);
		fnAdd("queue_depth", refStatus.szQueueDepth);
		fnAdd("source_epoch", refStatus.ullSourceEpoch);
		fnAdd("last_sequence", refStatus.ullLastSequence);
		fnAdd("received_total", refStatus.ullReceived);
		fnAdd("published_total", refStatus.ullPublished);
		fnAdd("publish_failed_total", refStatus.ullPublishFailed);
		fnAdd("capacity_dropped_total", refStatus.ullCapacityDropped);
		fnAdd("reset_discarded_total", refStatus.ullResetDiscarded);
		fnAdd("switch_total", refStatus.ullSwitchCount);
		fnAdd("active_priority", refStatus.uiActivePriority);
	}
}

std::uint64_t CMtQuoteServiceApp::GenerateProcessEpoch()
{
	static std::atomic<std::uint64_t> s_ullCounter(
		static_cast<std::uint64_t>(
			std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count()));
	std::uint64_t ullEpoch = s_ullCounter.fetch_add(1) + 1U;
	ullEpoch ^= static_cast<std::uint64_t>(GetCurrentProcessId()) << 32U;
	return ullEpoch != 0 ? ullEpoch : 1U;
}
