#include "MtEventServiceApp.h"

#include "CodeMsg.h"
#include "DeriveBinaryProtocol.h"
#include "EventHeartbeatBinaryProtocol.h"
#include "Log.h"
#include "ReliableEventBinaryProtocol.h"

#include <chrono>
#include <limits>
#include <utility>

namespace
{
	// 读取可选 int64 字段；字段存在但类型错误时返回 false。
	bool ReadOptionalInt64(const ST_PLUGIN_BINARY_VALUE& p_refRoot,
		const char* p_szName, std::int64_t& p_refValue,
		bool& p_refFound, std::string& p_refError)
	{
		p_refFound = false;
		const ST_PLUGIN_BINARY_VALUE* pValue =
			FindPluginBinaryField(p_refRoot, p_szName);
		if (pValue == nullptr)
		{
			return true;
		}
		p_refFound = true;
		if (pValue->enType != EN_PLUGIN_BINARY_VALUE_INT64)
		{
			p_refError = std::string(
				"RELIABLE_EVENT_FENCE_FIELD_INVALID: field=") + p_szName;
			return false;
		}
		p_refValue = pValue->llIntValue;
		return true;
	}

	// 读取可选 uint64 字段；用于 LeaseGeneration，零值由业务校验拒绝。
	bool ReadOptionalUInt64(const ST_PLUGIN_BINARY_VALUE& p_refRoot,
		const char* p_szName, std::uint64_t& p_refValue,
		bool& p_refFound, std::string& p_refError)
	{
		p_refFound = false;
		const ST_PLUGIN_BINARY_VALUE* pValue =
			FindPluginBinaryField(p_refRoot, p_szName);
		if (pValue == nullptr)
		{
			return true;
		}
		p_refFound = true;
		if (pValue->enType != EN_PLUGIN_BINARY_VALUE_UINT64)
		{
			p_refError = std::string(
				"RELIABLE_EVENT_FENCE_FIELD_INVALID: field=") + p_szName;
			return false;
		}
		p_refValue = pValue->ullUIntValue;
		return true;
	}

	// 读取可选字符串字段；Owner 内容由集群协调器与 KV 当前值比较。
	bool ReadOptionalString(const ST_PLUGIN_BINARY_VALUE& p_refRoot,
		const char* p_szName, std::string& p_refValue,
		bool& p_refFound, std::string& p_refError)
	{
		p_refFound = false;
		const ST_PLUGIN_BINARY_VALUE* pValue =
			FindPluginBinaryField(p_refRoot, p_szName);
		if (pValue == nullptr)
		{
			return true;
		}
		p_refFound = true;
		if (pValue->enType != EN_PLUGIN_BINARY_VALUE_STRING)
		{
			p_refError = std::string(
				"RELIABLE_EVENT_FENCE_FIELD_INVALID: field=") + p_szName;
			return false;
		}
		p_refValue = pValue->strStringValue;
		return true;
	}
}

CMtEventServiceApp::CMtEventServiceApp()
	: CServiceApplication(EN_PLUGIN_ID_MT_EVENT_SERVICE, "MtEventService")
	, m_stEventConfig()
	, m_clBroadcastBus()
	, m_clMarketEventRelay()
	, m_clReliableEventStore()
	, m_bComponentsStarted(false)
	, m_llLastPublishErrorMs(0)
	, m_ullRelaySequence(0)
	, m_ullProcessEpoch(GenerateProcessEpoch())
{
	// 构造阶段只初始化值对象，中继线程必须在配置和 CloudNet 网络均成功后启动。
}

CMtEventServiceApp::~CMtEventServiceApp()
{
	OnStopping();
}

bool CMtEventServiceApp::OnConfigure(std::string& p_refError)
{
	if (!LoadMtEventServiceConfig(GetConfigPath(),
		m_stEventConfig, p_refError))
	{
		return false;
	}
	MT_INFO(
		"market event config loaded,workerThreads=%u,queueCapacity=%zu,dropLogIntervalMs=%u",
		m_stEventConfig.stMarketEvent.uiWorkerThreads,
		m_stEventConfig.stMarketEvent.szQueueCapacity,
		m_stEventConfig.stMarketEvent.uiDropLogIntervalMs);
	MT_INFO(
		"reliable event config loaded,enabled=%d,stream=%s,subjectPrefix=%s",
		m_stEventConfig.stReliableEvent.bEnable ? 1 : 0,
		m_stEventConfig.stReliableEvent.strStream.c_str(),
		m_stEventConfig.stReliableEvent.strSubjectPrefix.c_str());
	MT_INFO(
		"event broadcast config loaded,enabled=%d,subject=%s,dedupeCapacity=%zu",
		m_stEventConfig.stBroadcast.bEnable ? 1 : 0,
		m_stEventConfig.stBroadcast.strSubject.c_str(),
		m_stEventConfig.stBroadcast.szDedupeCapacity);
	return true;
}

bool CMtEventServiceApp::OnStarted(std::string& p_refError)
{
	m_bComponentsStarted.store(false);
	m_ullRelaySequence.store(0);
	// 第一步：可靠后端启用时必须先获得 JetStream 和 Stream，禁止先对外报告成功。
	if (!m_clReliableEventStore.Start(
			m_stEventConfig.stReliableEvent, p_refError))
	{
		return false;
	}
	// 第二步：先加入跨实例广播 Subject，再启动本地行情队列，避免 READY 后存在广播遗漏窗口。
	if (!m_clBroadcastBus.Start(
			m_stEventConfig.stBroadcast,
			m_stEventConfig.stReliableEvent.strUrl,
			GetClusterInstanceId(), this, p_refError))
	{
		m_clReliableEventStore.Stop();
		return false;
	}

	// 第三步：可靠后端和广播总线就绪后启动独立的行情中继线程。
	if (!m_clMarketEventRelay.Start(
		m_stEventConfig.stMarketEvent,
		this, p_refError))
	{
		m_clBroadcastBus.Stop();
		m_clReliableEventStore.Stop();
		return false;
	}
	m_bComponentsStarted.store(true);
	MT_INFO("market event relay started,service=%s",
		GetServiceName().c_str());
	return true;
}

void CMtEventServiceApp::OnStopping()
{
	if (!m_bComponentsStarted.exchange(false))
	{
		// 启动失败仍可能已经创建部分资源，两个 Stop 都必须可重复调用。
		m_clReliableEventStore.Stop();
		m_clMarketEventRelay.Stop();
		m_clBroadcastBus.Stop();
		return;
	}
	// 第一步：拒绝 1172-1175，阻止新的可靠事件进入在线广播。
	m_clReliableEventStore.Stop();
	// 第二步：排空已受理行情，期间广播总线仍保持可用。
	m_clMarketEventRelay.Stop();
	// 第三步：业务生产者均已停止后解除广播订阅并释放 Core NATS 资源。
	m_clBroadcastBus.Stop();
}

bool CMtEventServiceApp::HandleBusinessRequest(const ST_CLOUD_NET_BINARY_REQUEST* p_pRequest,
	ST_CLOUD_NET_BINARY_RESULT* p_pResult)
{
	if (p_pRequest == nullptr || p_pResult == nullptr ||
		p_pRequest->lFuncId <
			EN_PLUGIN_FUNC_RELIABLE_EVENT_APPEND ||
		p_pRequest->lFuncId >
			EN_PLUGIN_FUNC_EVENT_HEARTBEAT)
	{
		// 1171 已登记为 RETIRED；公共健康、版本和 Echo 仍由基类处理。
		return false;
	}
	if (p_pRequest->lFuncId == EN_PLUGIN_FUNC_EVENT_HEARTBEAT)
	{
		return HandleEventHeartbeat(p_pRequest, p_pResult);
	}
	return HandleReliableEventRequest(p_pRequest, p_pResult);
}

bool CMtEventServiceApp::HandleReliableEventRequest(
	const ST_CLOUD_NET_BINARY_REQUEST* p_pRequest,
	ST_CLOUD_NET_BINARY_RESULT* p_pResult)
{
	if (p_pRequest->stPayload.iLen <= 0 ||
		p_pRequest->stPayload.pBuffer == nullptr)
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_INVALID_REQUEST,
			"RELIABLE_EVENT_REQUEST_BUFFER_INVALID: payload is empty");
		return true;
	}

	// 第一步：严格限制 Reliable Event 领域，拒绝 Trade/Query 文档误入。
	ST_PLUGIN_BINARY_DOCUMENT stRequest;
	std::string strError;
	if (!DecodeReliableEventBinaryDocument(
			p_pRequest->stPayload.pBuffer,
			static_cast<std::size_t>(
				p_pRequest->stPayload.iLen),
			stRequest, strError) ||
		!stRequest.refRoot)
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_PROTOCOL_ERROR,
			strError.empty() ?
				"RELIABLE_EVENT_REQUEST_DECODE_FAILED: root is null" :
				strError.c_str());
		return true;
	}
	if (p_pRequest->lFuncId == EN_PLUGIN_FUNC_RELIABLE_EVENT_APPEND &&
		!ValidateReliableEventFence(*stRequest.refRoot, strError))
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_STALE_FENCING_TOKEN,
			strError.empty() ?
				"STALE_FENCING_TOKEN: reliable event owner validation failed" :
				strError.c_str());
		return true;
	}

	// 第二步：按 1172-1175 调用存储层，传输成功和业务失败分别表达。
	std::int32_t iCode = EN_TERMINAL_ERROR_OK;
	std::string strMessage = "OK";
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refData;
	ST_MT_RELIABLE_APPEND_EVENT stAppendEvent;
	bool bHandled = false;
	switch (p_pRequest->lFuncId)
	{
	case EN_PLUGIN_FUNC_RELIABLE_EVENT_APPEND:
		bHandled = m_clReliableEventStore.Append(
			*stRequest.refRoot,
			p_pRequest->stPayload.pBuffer,
			static_cast<std::size_t>(
				p_pRequest->stPayload.iLen),
			iCode, strMessage, refData,
			stAppendEvent, strError);
		break;
	case EN_PLUGIN_FUNC_RELIABLE_EVENT_FETCH:
		bHandled = m_clReliableEventStore.Fetch(
			*stRequest.refRoot, iCode, strMessage,
			refData, strError);
		break;
	case EN_PLUGIN_FUNC_RELIABLE_EVENT_ACK:
		bHandled = m_clReliableEventStore.Ack(
			*stRequest.refRoot, iCode, strMessage,
			refData, strError);
		break;
	case EN_PLUGIN_FUNC_RELIABLE_EVENT_STATUS:
		bHandled = m_clReliableEventStore.Status(
			*stRequest.refRoot, iCode, strMessage,
			refData, strError);
		break;
	default:
		break;
	}
	if (!bHandled || !refData)
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_INTERNAL_ERROR,
			strError.empty() ?
				"RELIABLE_EVENT_HANDLER_FAILED: response Data is null" :
				strError.c_str());
		return true;
	}

	// 第三步：JetStream 首次确认后在线扇出标准载荷；业务事件使用 ClientData，1244 保留可靠信封。
	if (p_pRequest->lFuncId ==
			EN_PLUGIN_FUNC_RELIABLE_EVENT_APPEND &&
		iCode == EN_TERMINAL_ERROR_OK &&
		stAppendEvent.bShouldPublish)
	{
		ST_PLUGIN_NOTIFY_META stMeta;
		stMeta.usSourcePlugin = EN_PLUGIN_ID_MT_EVENT_SERVICE;
		stMeta.usNotifyMode = EN_PLUGIN_NOTIFY_MODE_RELIABLE;
		stMeta.usNotifyAction = stAppendEvent.enAction;
		stMeta.ullSequence = stAppendEvent.ullSourceSequence;
		stMeta.llTimestampMs = stAppendEvent.llCreatedTimeMs;
		stMeta.uiPayloadLen = static_cast<std::uint32_t>(
			stAppendEvent.aOnlinePayload.size());
		std::string strPublishError;
		if (!m_clBroadcastBus.Publish(
				stAppendEvent.enNotifyId,
				stMeta,
				stAppendEvent.aOnlinePayload.data(),
				stAppendEvent.aOnlinePayload.size(),
				strPublishError))
		{
			MT_ERROR(
				"reliable event online publish failed,notifyId=%lld,sourceSequence=%llu,detail=%s",
				static_cast<long long>(
					stAppendEvent.enNotifyId),
				static_cast<unsigned long long>(
					stAppendEvent.ullSourceSequence),
				strPublishError.c_str());
		}
	}

	// 第四步：无论业务成功或失败都编码为 Reliable Event 应答，供调用方读取详细 Code/Msg/Data。
	std::vector<unsigned char> aResponse;
	if (!EncodeReliableEventBinaryResponse(
			iCode, strMessage, *refData,
			aResponse, strError) ||
		aResponse.size() >
			static_cast<std::size_t>(
				(std::numeric_limits<int>::max)()))
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_PROTOCOL_ERROR,
			strError.empty() ?
				"RELIABLE_EVENT_RESPONSE_ENCODE_FAILED: response is too large" :
				strError.c_str());
		return true;
	}
	p_pResult->lParam = iCode;
	p_pResult->wParam =
		static_cast<long long>(aResponse.size());
	SetBinaryResultPayload(p_pResult,
		aResponse.empty() ? nullptr :
			aResponse.data(), aResponse.size());
	return true;
}

bool CMtEventServiceApp::ValidateReliableEventFence(
	const ST_PLUGIN_BINARY_VALUE& p_refRequest,
	std::string& p_refError) const
{
	p_refError.clear();
	std::int64_t llEnvelopeVersion = 1;
	std::int64_t llSourcePluginId = 0;
	std::int64_t llVersion = 0;
	std::int64_t llNo = 0;
	std::int64_t llNotifyId = 0;
	std::uint64_t ullLeaseGeneration = 0;
	std::string strOwnerInstanceId;
	bool bEnvelopeVersionFound = false;
	bool bSourcePluginFound = false;
	bool bVersionFound = false;
	bool bNoFound = false;
	bool bNotifyFound = false;
	bool bGenerationFound = false;
	bool bOwnerFound = false;
	if (!ReadOptionalInt64(p_refRequest, "EnvelopeVersion",
			llEnvelopeVersion, bEnvelopeVersionFound, p_refError) ||
		!ReadOptionalInt64(p_refRequest, "SourcePluginId",
			llSourcePluginId, bSourcePluginFound, p_refError) ||
		!ReadOptionalInt64(p_refRequest, "Version",
			llVersion, bVersionFound, p_refError) ||
		!ReadOptionalInt64(p_refRequest, "No",
			llNo, bNoFound, p_refError) ||
		!ReadOptionalInt64(p_refRequest, "NotifyId",
			llNotifyId, bNotifyFound, p_refError) ||
		!ReadOptionalUInt64(p_refRequest, "LeaseGeneration",
			ullLeaseGeneration, bGenerationFound, p_refError) ||
		!ReadOptionalString(p_refRequest, "OwnerInstanceId",
			strOwnerInstanceId, bOwnerFound, p_refError))
	{
		return false;
	}
	if (!bVersionFound || !bNoFound || !bNotifyFound ||
		(llVersion != 4 && llVersion != 5) || llNo <= 0 ||
		llEnvelopeVersion < 1 || llEnvelopeVersion > 2)
	{
		p_refError =
			"RELIABLE_EVENT_FENCE_HEADER_INVALID: EnvelopeVersion, Version, No or NotifyId is invalid";
		return false;
	}
	if (!bSourcePluginFound)
	{
		// v1 没有来源字段；1244 后续由 Query 产生，其余权威业务事件来自 Trade。
		llSourcePluginId = llNotifyId == EN_PLUGIN_NOTIFY_SERVER_TIME_CHANGED ?
			EN_PLUGIN_ID_MT_QUERY_SERVICE : EN_PLUGIN_ID_MT_TRADE_SERVICE;
	}
	if ((llSourcePluginId != EN_PLUGIN_ID_MT_QUERY_SERVICE &&
		 llSourcePluginId != EN_PLUGIN_ID_MT_TRADE_SERVICE) ||
		(llEnvelopeVersion == 2 &&
		 (!bEnvelopeVersionFound || !bSourcePluginFound ||
		  !bGenerationFound || !bOwnerFound ||
		  ullLeaseGeneration == 0 || strOwnerInstanceId.empty())))
	{
		p_refError =
			"RELIABLE_EVENT_FENCE_V2_INVALID: source, OwnerInstanceId or LeaseGeneration is missing";
		return false;
	}
	ST_CLUSTER_FENCE stFence;
	stFence.strOwnerInstanceId = strOwnerInstanceId;
	stFence.ullLeaseGeneration = ullLeaseGeneration;
	return ValidateRemoteClusterFence(
		static_cast<EN_PLUGIN_ID>(llSourcePluginId),
		static_cast<std::uint16_t>(llVersion),
		static_cast<std::int32_t>(llNo), stFence, p_refError);
}

void CMtEventServiceApp::HandlePluginNotify(
	EN_PLUGIN_NOTIFY_ID p_enNotifyId,
	const ST_PLUGIN_NOTIFY_META& p_refMeta,
	const unsigned char* p_pPayload,
	std::size_t p_szPayloadLen)
{
	if (p_enNotifyId ==
		EN_PLUGIN_NOTIFY_MARKET_TICK)
	{
		// 1211 回调只负责校验和入队，网络中继由固定工作线程完成。
		std::string strError;
		if (!m_clMarketEventRelay.Submit(p_refMeta,
				p_pPayload, p_szPayloadLen, strError) &&
			strError.find("EVENT_RELAY_NOT_ACCEPTING") != 0 &&
			strError.find("EVENT_RELAY_STOPPING") != 0)
		{
			MT_WARN(
				"market event rejected,sequence=%llu,payloadLen=%zu,detail=%s",
				static_cast<unsigned long long>(
					p_refMeta.ullSequence),
				p_szPayloadLen, strError.c_str());
		}
		return;
	}
	if (p_enNotifyId ==
			EN_PLUGIN_NOTIFY_PROFIT_CHANGED ||
		p_enNotifyId ==
			EN_PLUGIN_NOTIFY_KLINE_CHANGED)
	{
		RelayDerivedEvent(p_enNotifyId,
			p_refMeta, p_pPayload,
			p_szPayloadLen);
		return;
	}
	MT_WARN(
		"event notification ignored,notifyId=%lld,sourcePlugin=%u,payloadLen=%zu,detail=handler is not implemented",
		static_cast<long long>(p_enNotifyId),
		static_cast<unsigned int>(
			p_refMeta.usSourcePlugin),
		p_szPayloadLen);
}

bool CMtEventServiceApp::OnMarketEventReady(
	const ST_PLUGIN_NOTIFY_META& p_refSourceMeta,
	const std::vector<unsigned char>& p_refPayload,
	std::string& p_refError)
{
	// 第一阶段只有一个 Event 实例，1211 不进入 NATS，正文保持字节不变并在本实例发布。
	const std::uint64_t ullRelaySequence = NextRelaySequence();
	return PublishPluginNotification(
		EN_PLUGIN_NOTIFY_MARKET_TICK,
		EN_PLUGIN_NOTIFY_MODE_BEST_EFFORT,
		EN_PLUGIN_NOTIFY_ACTION_UPDATED,
		ullRelaySequence, GetTimestampMs(),
		p_refPayload.empty() ? nullptr : p_refPayload.data(),
		p_refPayload.size(), p_refError);
}

void CMtEventServiceApp::OnBroadcastEvent(
	EN_PLUGIN_NOTIFY_ID p_enNotifyId,
	const ST_PLUGIN_NOTIFY_META& p_refMeta,
	const std::vector<unsigned char>& p_refPayload)
{
	// 广播接收端只做本实例 Ice 扇出，禁止再次写回 NATS，避免形成广播环路。
	const std::uint64_t ullRelaySequence = NextRelaySequence();
	std::string strError;
	if (!PublishPluginNotification(p_enNotifyId,
			static_cast<EN_PLUGIN_NOTIFY_MODE>(p_refMeta.usNotifyMode),
			static_cast<EN_PLUGIN_NOTIFY_ACTION>(p_refMeta.usNotifyAction),
			ullRelaySequence, GetTimestampMs(),
			p_refPayload.empty() ? nullptr : p_refPayload.data(),
			p_refPayload.size(), strError) &&
		ShouldLogPublishError(GetTimestampMs()))
	{
		MT_ERROR(
			"event local fanout failed,notifyId=%lld,relaySequence=%llu,sourceSequence=%llu,payloadLen=%zu,detail=%s",
			static_cast<long long>(p_enNotifyId),
			static_cast<unsigned long long>(ullRelaySequence),
			static_cast<unsigned long long>(p_refMeta.ullSequence),
			p_refPayload.size(), strError.c_str());
	}
}

void CMtEventServiceApp::RelayDerivedEvent(
	EN_PLUGIN_NOTIFY_ID p_enNotifyId,
	const ST_PLUGIN_NOTIFY_META& p_refMeta,
	const unsigned char* p_pPayload,
	std::size_t p_szPayloadLen)
{
	std::string strError;

	// 第一步：只接受 MtDeriveService 产生的派生通知，禁止其他插件伪造收益或权威 M1。
	if (p_refMeta.usSourcePlugin !=
		EN_PLUGIN_ID_MT_DERIVE_SERVICE)
	{
		MT_WARN(
			"derived event source rejected,notifyId=%lld,sourcePlugin=%u,detail=source must be MtDeriveService",
			static_cast<long long>(p_enNotifyId),
			static_cast<unsigned int>(
				p_refMeta.usSourcePlugin));
		return;
	}

	// 第二步：按通知类型严格解码正文；Event 不修改业务正文，校验只用于阻断畸形数据。
	if (p_enNotifyId ==
		EN_PLUGIN_NOTIFY_KLINE_CHANGED)
	{
		ST_DERIVE_M1_SNAPSHOT_RESPONSE stM1;
		if (!DecodeDeriveM1SnapshotResponse(
				p_pPayload, p_szPayloadLen,
				stM1, strError) ||
			stM1.iCode !=
				EN_TERMINAL_ERROR_OK ||
			stM1.enState !=
				EN_DERIVE_SERVICE_STATE_READY ||
			stM1.aBar.empty())
		{
			if (strError.empty())
			{
				strError =
					"EVENT_DERIVE_M1_BODY_INVALID: 1252 must contain at least one READY M1 bar";
			}
			MT_WARN(
				"derived M1 rejected,sourceSequence=%llu,payloadLen=%zu,detail=%s",
				static_cast<unsigned long long>(
					p_refMeta.ullSequence),
				p_szPayloadLen, strError.c_str());
			return;
		}
	}
	else
	{
		ST_DERIVE_PROFIT_SNAPSHOT_RESPONSE stProfit;
		if (!DecodeDeriveProfitSnapshotResponse(
				p_pPayload, p_szPayloadLen,
				stProfit, strError) ||
			stProfit.iCode !=
				EN_TERMINAL_ERROR_OK ||
			stProfit.enState !=
				EN_DERIVE_SERVICE_STATE_READY)
		{
			if (strError.empty())
			{
				strError =
					"EVENT_DERIVE_PROFIT_BODY_INVALID: 1251 must contain a READY profit snapshot";
			}
			MT_WARN(
				"derived profit rejected,sourceSequence=%llu,payloadLen=%zu,detail=%s",
				static_cast<unsigned long long>(
					p_refMeta.ullSequence),
				p_szPayloadLen, strError.c_str());
			return;
		}
	}

	// 第三步：第一阶段单 Event 直接以自身身份在线转发，不进入 Core NATS。
	if (!PublishPluginNotification(p_enNotifyId,
			EN_PLUGIN_NOTIFY_MODE_BEST_EFFORT,
			static_cast<EN_PLUGIN_NOTIFY_ACTION>(p_refMeta.usNotifyAction),
			NextRelaySequence(), GetTimestampMs(),
			p_pPayload, p_szPayloadLen, strError) &&
		ShouldLogPublishError(GetTimestampMs()))
	{
		MT_ERROR(
			"derived event broadcast failed,notifyId=%lld,sourceSequence=%llu,payloadLen=%zu,detail=%s",
			static_cast<long long>(p_enNotifyId),
			static_cast<unsigned long long>(
				p_refMeta.ullSequence),
			p_szPayloadLen, strError.c_str());
	}
}

bool CMtEventServiceApp::HandleEventHeartbeat(
	const ST_CLOUD_NET_BINARY_REQUEST* p_pRequest,
	ST_CLOUD_NET_BINARY_RESULT* p_pResult)
{
	// 第一步：严格解码调用方身份和请求序号，空正文或错误目标不得返回伪 Ready。
	ST_EVENT_HEARTBEAT_REQUEST stRequest;
	std::string strError;
	if (p_pRequest->stPayload.iLen <= 0 ||
		p_pRequest->stPayload.pBuffer == nullptr ||
		!DecodeEventHeartbeatRequest(p_pRequest->stPayload.pBuffer,
			static_cast<std::size_t>(p_pRequest->stPayload.iLen),
			stRequest, strError))
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_PROTOCOL_ERROR,
			strError.empty() ?
				"EVENT_HEARTBEAT_REQUEST_INVALID: payload is empty" :
				strError.c_str());
		return true;
	}

	// 第二步：复制 Relay 快照，构造过程不持有 NATS 或 CloudNet 内部锁。
	ST_MT_MARKET_EVENT_DIAGNOSTICS stDiagnostics;
	m_clMarketEventRelay.GetDiagnostics(stDiagnostics);
	ST_EVENT_HEARTBEAT_RESPONSE stResponse;
	stResponse.enRequesterPluginId = stRequest.enSenderPluginId;
	stResponse.enResponderPluginId = EN_PLUGIN_ID_MT_EVENT_SERVICE;
	stResponse.bJetStreamReady = m_clReliableEventStore.IsReady();
	stResponse.ullRequestSequence = stRequest.ullRequestSequence;
	stResponse.ullResponderProcessEpoch = m_ullProcessEpoch;
	stResponse.llRespondedAtMs = GetTimestampMs();
	stResponse.ullQueueCapacity = stDiagnostics.ullQueueCapacity;
	stResponse.ullQueueDepth = stDiagnostics.ullQueueDepth;
	stResponse.ullReceivedCount = stDiagnostics.ullReceivedCount;
	stResponse.ullValidationRejectedCount =
		stDiagnostics.ullValidationRejectedCount;
	stResponse.ullDuplicateCount = stDiagnostics.ullDuplicateCount;
	stResponse.ullOutOfOrderCount = stDiagnostics.ullOutOfOrderCount;
	stResponse.ullEpochResetCount = stDiagnostics.ullEpochResetCount;
	stResponse.ullSequenceGapCount = stDiagnostics.ullSequenceGapCount;
	stResponse.ullCapacityDroppedCount =
		stDiagnostics.ullCapacityDroppedCount;
	stResponse.ullFanoutCount = stDiagnostics.ullFanoutCount;
	stResponse.ullPublishFailedCount = stDiagnostics.ullPublishFailedCount;
	stResponse.aSource.swap(stDiagnostics.aSource);

	// 第三步：显式编码应答，正文只包含有界诊断字段。
	std::vector<unsigned char> aResponse;
	if (!EncodeEventHeartbeatResponse(stResponse, aResponse, strError))
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_PROTOCOL_ERROR, strError.c_str());
		return true;
	}
	p_pResult->lParam = static_cast<long long>(stResponse.aSource.size());
	SetBinaryResultPayload(p_pResult,
		aResponse.empty() ? nullptr : aResponse.data(), aResponse.size());
	return true;
}

void CMtEventServiceApp::AppendRuntimeMetrics(
	std::vector<ST_CLUSTER_RUNTIME_METRIC>& p_refMetric)
{
	ST_MT_MARKET_EVENT_DIAGNOSTICS stDiagnostics;
	m_clMarketEventRelay.GetDiagnostics(stDiagnostics);
	const auto fnAdd = [&p_refMetric](
		const char* p_szName, std::uint64_t p_ullValue)
	{
		if (p_refMetric.size() >= CLUSTER_MAX_METRIC_COUNT)
		{
			return;
		}
		ST_CLUSTER_RUNTIME_METRIC stMetric;
		stMetric.strName = p_szName;
		stMetric.ullValue = p_ullValue;
		p_refMetric.push_back(std::move(stMetric));
	};
	fnAdd("event.market.queue_capacity", stDiagnostics.ullQueueCapacity);
	fnAdd("event.market.queue_depth", stDiagnostics.ullQueueDepth);
	fnAdd("event.market.received_total", stDiagnostics.ullReceivedCount);
	fnAdd("event.market.validation_rejected_total",
		stDiagnostics.ullValidationRejectedCount);
	fnAdd("event.market.duplicate_total", stDiagnostics.ullDuplicateCount);
	fnAdd("event.market.out_of_order_total", stDiagnostics.ullOutOfOrderCount);
	fnAdd("event.market.epoch_reset_total", stDiagnostics.ullEpochResetCount);
	fnAdd("event.market.sequence_gap_total", stDiagnostics.ullSequenceGapCount);
	fnAdd("event.market.capacity_dropped_total",
		stDiagnostics.ullCapacityDroppedCount);
	fnAdd("event.market.fanout_total", stDiagnostics.ullFanoutCount);
	fnAdd("event.market.publish_failed_total",
		stDiagnostics.ullPublishFailedCount);
	fnAdd("event.reliable.jetstream_ready",
		m_clReliableEventStore.IsReady() ? 1U : 0U);
}

bool CMtEventServiceApp::ShouldLogPublishError(
	std::int64_t p_llNowMs)
{
	const std::int64_t llInterval =
		static_cast<std::int64_t>(
			m_stEventConfig.stMarketEvent.
				uiDropLogIntervalMs);
	std::int64_t llPrevious =
		m_llLastPublishErrorMs.load();
	while (llPrevious == 0 ||
		p_llNowMs - llPrevious >= llInterval)
	{
		if (m_llLastPublishErrorMs.compare_exchange_weak(
			llPrevious, p_llNowMs))
		{
			return true;
		}
	}
	return false;
}

std::uint64_t CMtEventServiceApp::NextRelaySequence()
{
	std::uint64_t ullSequence =
		m_ullRelaySequence.fetch_add(1) + 1;
	if (ullSequence == 0)
	{
		ullSequence =
			m_ullRelaySequence.fetch_add(1) + 1;
	}
	return ullSequence;
}

std::uint64_t CMtEventServiceApp::GenerateProcessEpoch()
{
	static std::atomic<std::uint64_t> s_ullCounter(
		static_cast<std::uint64_t>(
			std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count()));
	std::uint64_t ullEpoch = s_ullCounter.fetch_add(1) + 1U;
	ullEpoch ^= static_cast<std::uint64_t>(GetCurrentProcessId()) << 32U;
	return ullEpoch != 0 ? ullEpoch : 1U;
}
