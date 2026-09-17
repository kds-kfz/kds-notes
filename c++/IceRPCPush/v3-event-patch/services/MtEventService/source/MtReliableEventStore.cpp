#include "MtReliableEventStore.h"

#include "ClientDataBinaryProtocol.h"
#include "CodeMsg.h"

#include "nats/nats.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <new>
#include <sstream>

namespace
{
	// 创建 Binary 值节点；所有内存失败都返回可定位的英文错误。
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE> MakeValue(
		EN_PLUGIN_BINARY_VALUE_TYPE p_enType,
		std::string& p_refError)
	{
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refValue =
			CreatePluginBinaryValue(p_enType);
		if (!refValue)
		{
			p_refError =
				"RELIABLE_EVENT_MEMORY_ERROR: failed to allocate Binary value";
		}
		return refValue;
	}

	// 向对象追加字段；字段和值均需有效。
	bool AddField(ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName,
		const std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& p_refValue,
		std::string& p_refError)
	{
		if (p_refObject.enType !=
				EN_PLUGIN_BINARY_VALUE_OBJECT ||
			p_szName == nullptr || p_szName[0] == '\0' ||
			!p_refValue)
		{
			p_refError =
				"RELIABLE_EVENT_RESPONSE_FIELD_INVALID: object, field name or value is invalid";
			return false;
		}
		ST_PLUGIN_BINARY_FIELD stField;
		stField.strName = p_szName;
		stField.refValue = p_refValue;
		p_refObject.aObjectField.push_back(stField);
		return true;
	}

	bool AddInt64(ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName, std::int64_t p_llValue,
		std::string& p_refError)
	{
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refValue =
			MakeValue(EN_PLUGIN_BINARY_VALUE_INT64,
				p_refError);
		if (!refValue)
		{
			return false;
		}
		refValue->llIntValue = p_llValue;
		return AddField(p_refObject, p_szName,
			refValue, p_refError);
	}

	bool AddUInt64(ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName, std::uint64_t p_ullValue,
		std::string& p_refError)
	{
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refValue =
			MakeValue(EN_PLUGIN_BINARY_VALUE_UINT64,
				p_refError);
		if (!refValue)
		{
			return false;
		}
		refValue->ullUIntValue = p_ullValue;
		return AddField(p_refObject, p_szName,
			refValue, p_refError);
	}

	bool AddBool(ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName, bool p_bValue,
		std::string& p_refError)
	{
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refValue =
			MakeValue(EN_PLUGIN_BINARY_VALUE_BOOL,
				p_refError);
		if (!refValue)
		{
			return false;
		}
		refValue->ucBoolValue =
			p_bValue ? static_cast<std::uint8_t>(1) :
				static_cast<std::uint8_t>(0);
		return AddField(p_refObject, p_szName,
			refValue, p_refError);
	}

	bool AddString(ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName,
		const std::string& p_refText,
		std::string& p_refError)
	{
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refValue =
			MakeValue(EN_PLUGIN_BINARY_VALUE_STRING,
				p_refError);
		if (!refValue)
		{
			return false;
		}
		refValue->strStringValue = p_refText;
		return AddField(p_refObject, p_szName,
			refValue, p_refError);
	}

	bool AddBytes(ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName,
		const unsigned char* p_pData,
		std::size_t p_szDataLen,
		std::string& p_refError)
	{
		if (p_szDataLen > 0 && p_pData == nullptr)
		{
			p_refError =
				"RELIABLE_EVENT_RESPONSE_BYTES_INVALID: data pointer is null";
			return false;
		}
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refValue =
			MakeValue(EN_PLUGIN_BINARY_VALUE_BYTES,
				p_refError);
		if (!refValue)
		{
			return false;
		}
		if (p_szDataLen > 0)
		{
			refValue->aByteValue.assign(p_pData,
				p_pData + p_szDataLen);
		}
		return AddField(p_refObject, p_szName,
			refValue, p_refError);
	}

	bool ReadString(const ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName, std::string& p_refValue,
		std::string& p_refError, bool p_bRequired = true)
	{
		const ST_PLUGIN_BINARY_VALUE* pValue =
			FindPluginBinaryField(p_refObject, p_szName);
		if (pValue == nullptr)
		{
			if (!p_bRequired)
			{
				p_refValue.clear();
				return true;
			}
			p_refError = std::string(
				"RELIABLE_EVENT_FIELD_MISSING: field=") +
				p_szName;
			return false;
		}
		if (pValue->enType !=
			EN_PLUGIN_BINARY_VALUE_STRING)
		{
			p_refError = std::string(
				"RELIABLE_EVENT_FIELD_TYPE_INVALID: field=") +
				p_szName + ", expected=string";
			return false;
		}
		p_refValue = pValue->strStringValue;
		return true;
	}

	bool ReadInt64(const ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName, std::int64_t& p_refValue,
		std::string& p_refError)
	{
		const ST_PLUGIN_BINARY_VALUE* pValue =
			FindPluginBinaryField(p_refObject, p_szName);
		if (pValue == nullptr)
		{
			p_refError = std::string(
				"RELIABLE_EVENT_FIELD_MISSING: field=") +
				p_szName;
			return false;
		}
		if (pValue->enType ==
			EN_PLUGIN_BINARY_VALUE_INT64)
		{
			p_refValue = pValue->llIntValue;
			return true;
		}
		if (pValue->enType ==
				EN_PLUGIN_BINARY_VALUE_UINT64 &&
			pValue->ullUIntValue <=
				static_cast<std::uint64_t>(
					(std::numeric_limits<
						std::int64_t>::max)()))
		{
			p_refValue = static_cast<std::int64_t>(
				pValue->ullUIntValue);
			return true;
		}
		p_refError = std::string(
			"RELIABLE_EVENT_FIELD_TYPE_INVALID: field=") +
			p_szName + ", expected=int64";
		return false;
	}

	bool ReadUInt64(const ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName, std::uint64_t& p_refValue,
		std::string& p_refError)
	{
		const ST_PLUGIN_BINARY_VALUE* pValue =
			FindPluginBinaryField(p_refObject, p_szName);
		if (pValue == nullptr)
		{
			p_refError = std::string(
				"RELIABLE_EVENT_FIELD_MISSING: field=") +
				p_szName;
			return false;
		}
		if (pValue->enType ==
			EN_PLUGIN_BINARY_VALUE_UINT64)
		{
			p_refValue = pValue->ullUIntValue;
			return true;
		}
		if (pValue->enType ==
				EN_PLUGIN_BINARY_VALUE_INT64 &&
			pValue->llIntValue >= 0)
		{
			p_refValue = static_cast<std::uint64_t>(
				pValue->llIntValue);
			return true;
		}
		p_refError = std::string(
			"RELIABLE_EVENT_FIELD_TYPE_INVALID: field=") +
			p_szName + ", expected=uint64";
		return false;
	}

	bool ReadBoundedUInt(const ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName, unsigned int p_uiDefault,
		unsigned int p_uiMaximum,
		unsigned int& p_refValue,
		std::string& p_refError)
	{
		p_refValue = p_uiDefault;
		const ST_PLUGIN_BINARY_VALUE* pValue =
			FindPluginBinaryField(p_refObject, p_szName);
		if (pValue == nullptr)
		{
			return true;
		}
		std::uint64_t ullValue = 0;
		if (pValue->enType ==
			EN_PLUGIN_BINARY_VALUE_UINT64)
		{
			ullValue = pValue->ullUIntValue;
		}
		else if (pValue->enType ==
				EN_PLUGIN_BINARY_VALUE_INT64 &&
			pValue->llIntValue >= 0)
		{
			ullValue = static_cast<std::uint64_t>(
				pValue->llIntValue);
		}
		else
		{
			p_refError = std::string(
				"RELIABLE_EVENT_FIELD_TYPE_INVALID: field=") +
				p_szName + ", expected=unsigned integer";
			return false;
		}
		if (ullValue == 0 || ullValue > p_uiMaximum)
		{
			p_refError = std::string(
				"RELIABLE_EVENT_FIELD_RANGE_INVALID: field=") +
				p_szName + ", value=" +
				std::to_string(ullValue) +
				", maximum=" +
				std::to_string(p_uiMaximum);
			return false;
		}
		p_refValue = static_cast<unsigned int>(ullValue);
		return true;
	}

	std::shared_ptr<ST_PLUGIN_BINARY_VALUE> MakeErrorData(
		std::string& p_refError)
	{
		return MakeValue(EN_PLUGIN_BINARY_VALUE_OBJECT,
			p_refError);
	}
}

ST_MT_RELIABLE_APPEND_EVENT::
	ST_MT_RELIABLE_APPEND_EVENT()
	: bShouldPublish(false)
	, enNotifyId(EN_PLUGIN_NOTIFY_ORDER_CHANGED)
	, enAction(EN_PLUGIN_NOTIFY_ACTION_UPDATED)
	, ullSourceSequence(0)
	, llCreatedTimeMs(0)
	, aEnvelope()
	, aOnlinePayload()
{
}

CMtReliableEventStore::CMtReliableEventStore()
	: m_stConfig()
	, m_bAccepting(false)
	, m_bReady(false)
	, m_clApiMutex()
	, m_clConsumerMutex()
	, m_pConnection(nullptr)
	, m_pJetStream(nullptr)
	, m_mapSubscription()
	, m_mapPendingAck()
	, m_mapPendingConsumer()
{
}

CMtReliableEventStore::~CMtReliableEventStore()
{
	Stop();
}

bool CMtReliableEventStore::Start(
	const ST_MT_RELIABLE_EVENT_CONFIG& p_refConfig,
	std::string& p_refError)
{
	Stop();
	p_refError.clear();
	m_stConfig = p_refConfig;
	if (!m_stConfig.bEnable)
	{
		m_bAccepting.store(true);
		return true;
	}

	std::unique_lock<std::shared_mutex> clApiLock(
		m_clApiMutex);
	natsStatus enStatus = natsConnection_ConnectTo(
		&m_pConnection, m_stConfig.strUrl.c_str());
	if (enStatus != NATS_OK)
	{
		p_refError = BuildNatsError(
			"natsConnection_ConnectTo",
			static_cast<int>(enStatus), 0);
		clApiLock.unlock();
		Stop();
		return false;
	}
	enStatus = natsConnection_JetStream(
		&m_pJetStream, m_pConnection, nullptr);
	if (enStatus != NATS_OK || m_pJetStream == nullptr)
	{
		p_refError = BuildNatsError(
			"natsConnection_JetStream",
			static_cast<int>(enStatus), 0);
		clApiLock.unlock();
		Stop();
		return false;
	}
	if (!EnsureStream(p_refError))
	{
		clApiLock.unlock();
		Stop();
		return false;
	}
	m_bReady.store(true);
	m_bAccepting.store(true);
	return true;
}

void CMtReliableEventStore::Stop()
{
	m_bAccepting.store(false);
	std::unique_lock<std::shared_mutex> clApiLock(
		m_clApiMutex);
	std::lock_guard<std::mutex> clConsumerLock(
		m_clConsumerMutex);

	// 第一步：未 ACK 消息只释放本地句柄，不向服务端确认，使 durable consumer 后续重投。
	for (std::map<std::string, natsMsg*>::iterator
		it = m_mapPendingAck.begin();
		it != m_mapPendingAck.end(); ++it)
	{
		if (it->second != nullptr)
		{
			natsMsg_Destroy(it->second);
		}
	}
	m_mapPendingAck.clear();
	m_mapPendingConsumer.clear();

	// 第二步：释放 pull subscription 后再销毁 JetStream 和底层连接。
	for (std::map<std::string,
		natsSubscription*>::iterator
		it = m_mapSubscription.begin();
		it != m_mapSubscription.end(); ++it)
	{
		if (it->second != nullptr)
		{
			natsSubscription_Destroy(it->second);
		}
	}
	m_mapSubscription.clear();
	if (m_pJetStream != nullptr)
	{
		jsCtx_Destroy(m_pJetStream);
		m_pJetStream = nullptr;
	}
	if (m_pConnection != nullptr)
	{
		natsConnection_Destroy(m_pConnection);
		m_pConnection = nullptr;
	}
	m_bReady.store(false);
}

bool CMtReliableEventStore::Append(
	const ST_PLUGIN_BINARY_VALUE& p_refRequest,
	const unsigned char* p_pEnvelope,
	std::size_t p_szEnvelopeLen,
	std::int32_t& p_refCode,
	std::string& p_refMessage,
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& p_refData,
	ST_MT_RELIABLE_APPEND_EVENT& p_refEvent,
	std::string& p_refError)
{
	p_refCode = EN_TERMINAL_ERROR_OK;
	p_refMessage = "OK";
	p_refData.reset();
	p_refEvent = ST_MT_RELIABLE_APPEND_EVENT();
	p_refError.clear();
	if (p_refRequest.enType !=
			EN_PLUGIN_BINARY_VALUE_OBJECT ||
		p_pEnvelope == nullptr || p_szEnvelopeLen == 0)
	{
		p_refError =
			"RELIABLE_EVENT_APPEND_REQUEST_INVALID: root or envelope is invalid";
		return false;
	}
	p_refData = MakeErrorData(p_refError);
	if (!p_refData)
	{
		return false;
	}
	if (!m_stConfig.bEnable)
	{
		p_refCode =
			EN_TERMINAL_ERROR_RELIABLE_EVENT_DISABLED;
		p_refMessage =
			"RELIABLE_EVENT_DISABLED: enable=0 in MtEventService.xml";
		return true;
	}
	if (!m_bAccepting.load() || !m_bReady.load())
	{
		p_refCode =
			EN_TERMINAL_ERROR_RELIABLE_EVENT_BACKEND;
		p_refMessage =
			"RELIABLE_EVENT_BACKEND_NOT_READY: JetStream connection is unavailable";
		return true;
	}

	std::string strEventId;
	std::string strSubject;
	EN_PLUGIN_NOTIFY_ID enNotifyId =
		EN_PLUGIN_NOTIFY_ORDER_CHANGED;
	EN_PLUGIN_NOTIFY_ACTION enAction =
		EN_PLUGIN_NOTIFY_ACTION_UPDATED;
	std::uint64_t ullSourceSequence = 0;
	std::int64_t llCreatedTimeMs = 0;
	if (!ValidateAppendRequest(p_refRequest,
			strEventId, strSubject, enNotifyId,
			enAction, ullSourceSequence,
			llCreatedTimeMs, p_refError))
	{
		p_refCode = EN_TERMINAL_ERROR_INVALID_REQUEST;
		p_refMessage = p_refError;
		p_refError.clear();
		return true;
	}

	// 可靠时间通知由公共层从完整信封解包；其余权威事件必须先转换成网关可消费的 ClientData。
	std::vector<unsigned char> aOnlinePayload;
	if (enNotifyId == EN_PLUGIN_NOTIFY_SERVER_TIME_CHANGED)
	{
		aOnlinePayload.assign(p_pEnvelope,
			p_pEnvelope + p_szEnvelopeLen);
	}
	else if (!EncodeReliableTradeEventAsClientData(
			p_refRequest, aOnlinePayload, p_refError))
	{
		p_refCode = EN_TERMINAL_ERROR_PROTOCOL_ERROR;
		p_refMessage =
			"RELIABLE_EVENT_ONLINE_PAYLOAD_INVALID: " +
			p_refError;
		p_refError.clear();
		return true;
	}

	std::shared_lock<std::shared_mutex> clApiLock(
		m_clApiMutex);
	natsMsg* pMessage = nullptr;
	natsStatus enStatus = natsMsg_Create(
		&pMessage, strSubject.c_str(), nullptr,
		reinterpret_cast<const char*>(p_pEnvelope),
		static_cast<int>(p_szEnvelopeLen));
	if (enStatus == NATS_OK)
	{
		enStatus = natsMsgHeader_Set(pMessage,
			"Nats-Msg-Id", strEventId.c_str());
	}
	jsPubAck* pAck = nullptr;
	jsErrCode enJetStreamError =
		static_cast<jsErrCode>(0);
	if (enStatus == NATS_OK)
	{
		enStatus = js_PublishMsg(&pAck,
			m_pJetStream, pMessage, nullptr,
			&enJetStreamError);
	}
	natsMsg_Destroy(pMessage);
	if (enStatus != NATS_OK || pAck == nullptr)
	{
		p_refCode =
			EN_TERMINAL_ERROR_RELIABLE_EVENT_APPEND;
		p_refMessage = BuildNatsError(
			"js_PublishMsg",
			static_cast<int>(enStatus),
			static_cast<int>(enJetStreamError));
		if (pAck != nullptr)
		{
			jsPubAck_Destroy(pAck);
		}
		return true;
	}

	const std::uint64_t ullStreamSequence =
		pAck->Sequence;
	const bool bDuplicate = pAck->Duplicate;
	jsPubAck_Destroy(pAck);
	if (!AddString(*p_refData, "EventId",
			strEventId, p_refError) ||
		!AddUInt64(*p_refData, "StreamSequence",
			ullStreamSequence, p_refError) ||
		!AddBool(*p_refData, "Duplicate",
			bDuplicate, p_refError) ||
		!AddString(*p_refData, "Subject",
			strSubject, p_refError))
	{
		return false;
	}

	p_refEvent.bShouldPublish = !bDuplicate;
	p_refEvent.enNotifyId = enNotifyId;
	p_refEvent.enAction = enAction;
	p_refEvent.ullSourceSequence =
		ullSourceSequence;
	p_refEvent.llCreatedTimeMs = llCreatedTimeMs;
	p_refEvent.aEnvelope.assign(p_pEnvelope,
		p_pEnvelope + p_szEnvelopeLen);
	p_refEvent.aOnlinePayload.swap(aOnlinePayload);
	return true;
}

bool CMtReliableEventStore::Fetch(
	const ST_PLUGIN_BINARY_VALUE& p_refRequest,
	std::int32_t& p_refCode,
	std::string& p_refMessage,
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& p_refData,
	std::string& p_refError)
{
	p_refCode = EN_TERMINAL_ERROR_OK;
	p_refMessage = "OK";
	p_refError.clear();
	p_refData = MakeErrorData(p_refError);
	if (!p_refData)
	{
		return false;
	}
	if (!m_stConfig.bEnable)
	{
		p_refCode =
			EN_TERMINAL_ERROR_RELIABLE_EVENT_DISABLED;
		p_refMessage =
			"RELIABLE_EVENT_DISABLED: enable=0 in MtEventService.xml";
		return true;
	}
	std::string strConsumer;
	unsigned int uiBatch =
		(std::min)(m_stConfig.uiFetchBatchMax, 100U);
	unsigned int uiWaitMs =
		m_stConfig.uiFetchWaitMs;
	if (!ReadString(p_refRequest, "Consumer",
			strConsumer, p_refError) ||
		!ReadBoundedUInt(p_refRequest, "Batch",
			uiBatch, m_stConfig.uiFetchBatchMax,
			uiBatch, p_refError) ||
		!ReadBoundedUInt(p_refRequest, "WaitMs",
			uiWaitMs, m_stConfig.uiFetchWaitMs,
			uiWaitMs, p_refError) ||
		!IsValidConsumerName(strConsumer))
	{
		p_refCode = EN_TERMINAL_ERROR_INVALID_REQUEST;
		p_refMessage = p_refError.empty() ?
			"RELIABLE_EVENT_CONSUMER_INVALID: use 1-128 letters, digits, underscore or hyphen" :
			p_refError;
		p_refError.clear();
		return true;
	}
	if (!m_bAccepting.load() || !m_bReady.load())
	{
		p_refCode =
			EN_TERMINAL_ERROR_RELIABLE_EVENT_BACKEND;
		p_refMessage =
			"RELIABLE_EVENT_BACKEND_NOT_READY: JetStream connection is unavailable";
		return true;
	}

	std::shared_lock<std::shared_mutex> clApiLock(
		m_clApiMutex);
	natsSubscription* pSubscription = nullptr;
	{
		std::lock_guard<std::mutex> clConsumerLock(
			m_clConsumerMutex);
		pSubscription = GetOrCreateSubscription(
			strConsumer, p_refError);
	}
	if (pSubscription == nullptr)
	{
		p_refCode =
			EN_TERMINAL_ERROR_RELIABLE_EVENT_FETCH;
		p_refMessage = p_refError;
		p_refError.clear();
		return true;
	}

	natsMsgList stList;
	stList.Msgs = nullptr;
	stList.Count = 0;
	jsErrCode enJetStreamError =
		static_cast<jsErrCode>(0);
	const natsStatus enStatus =
		natsSubscription_Fetch(&stList,
			pSubscription, static_cast<int>(uiBatch),
			static_cast<std::int64_t>(uiWaitMs),
			&enJetStreamError);
	if (enStatus != NATS_OK &&
		enStatus != NATS_TIMEOUT)
	{
		natsMsgList_Destroy(&stList);
		p_refCode =
			EN_TERMINAL_ERROR_RELIABLE_EVENT_FETCH;
		p_refMessage = BuildNatsError(
			"natsSubscription_Fetch",
			static_cast<int>(enStatus),
			static_cast<int>(enJetStreamError));
		return true;
	}

	std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refEvents =
		MakeValue(EN_PLUGIN_BINARY_VALUE_ARRAY,
			p_refError);
	if (!refEvents)
	{
		natsMsgList_Destroy(&stList);
		return false;
	}
	std::vector<std::string> aInsertedToken;
	for (int iIndex = 0;
		iIndex < stList.Count; ++iIndex)
	{
		natsMsg* pMessage = stList.Msgs[iIndex];
		if (pMessage == nullptr)
		{
			continue;
		}
		const std::uint64_t ullSequence =
			natsMsg_GetSequence(pMessage);
		const std::string strAckToken =
			BuildAckToken(strConsumer, ullSequence);
		const char* pData = natsMsg_GetData(pMessage);
		const int iDataLen =
			natsMsg_GetDataLength(pMessage);
		const char* pSubject =
			natsMsg_GetSubject(pMessage);
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE>
			refEvent = MakeValue(
				EN_PLUGIN_BINARY_VALUE_OBJECT,
				p_refError);
		if (!refEvent ||
			!AddString(*refEvent, "AckToken",
				strAckToken, p_refError) ||
			!AddUInt64(*refEvent, "StreamSequence",
				ullSequence, p_refError) ||
			!AddString(*refEvent, "Subject",
				pSubject != nullptr ? pSubject :
					std::string(), p_refError) ||
			!AddBytes(*refEvent, "Envelope",
				reinterpret_cast<
					const unsigned char*>(pData),
				iDataLen > 0 ?
					static_cast<std::size_t>(iDataLen) :
					0, p_refError))
		{
			break;
		}

		bool bAccepted = false;
		{
			std::lock_guard<std::mutex>
				clConsumerLock(m_clConsumerMutex);
			if (m_mapPendingAck.find(strAckToken) ==
				m_mapPendingAck.end())
			{
				m_mapPendingAck[strAckToken] =
					pMessage;
				m_mapPendingConsumer[strAckToken] =
					strConsumer;
				bAccepted = true;
			}
		}
		if (!bAccepted)
		{
			// 同一消息已在本进程等待 ACK，本次重复投递不再返回第二个句柄。
			continue;
		}
		stList.Msgs[iIndex] = nullptr;
		aInsertedToken.push_back(strAckToken);
		refEvents->aArrayValue.push_back(refEvent);
	}
	natsMsgList_Destroy(&stList);
	const auto fnReleaseInserted =
		[this, &aInsertedToken]()
	{
		std::lock_guard<std::mutex> clConsumerLock(
			m_clConsumerMutex);
		for (std::size_t szIndex = 0;
			szIndex < aInsertedToken.size(); ++szIndex)
		{
			const std::string& strToken =
				aInsertedToken[szIndex];
			std::map<std::string, natsMsg*>::iterator
				it = m_mapPendingAck.find(strToken);
			if (it != m_mapPendingAck.end())
			{
				natsMsg_Destroy(it->second);
				m_mapPendingAck.erase(it);
			}
			m_mapPendingConsumer.erase(strToken);
		}
	};
	if (!p_refError.empty())
	{
		fnReleaseInserted();
		return false;
	}
	if (!AddString(*p_refData, "Consumer",
			strConsumer, p_refError) ||
		!AddField(*p_refData, "Events",
			refEvents, p_refError) ||
		!AddUInt64(*p_refData, "Count",
			static_cast<std::uint64_t>(
				refEvents->aArrayValue.size()),
			p_refError))
	{
		// 响应未能交给调用方时不能保留不可见 AckToken；释放本地句柄后由 JetStream 重投。
		fnReleaseInserted();
		return false;
	}
	return true;
}

bool CMtReliableEventStore::Ack(
	const ST_PLUGIN_BINARY_VALUE& p_refRequest,
	std::int32_t& p_refCode,
	std::string& p_refMessage,
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& p_refData,
	std::string& p_refError)
{
	p_refCode = EN_TERMINAL_ERROR_OK;
	p_refMessage = "OK";
	p_refError.clear();
	p_refData = MakeErrorData(p_refError);
	if (!p_refData)
	{
		return false;
	}
	if (!m_stConfig.bEnable)
	{
		p_refCode =
			EN_TERMINAL_ERROR_RELIABLE_EVENT_DISABLED;
		p_refMessage =
			"RELIABLE_EVENT_DISABLED: enable=0 in MtEventService.xml";
		return true;
	}
	std::string strConsumer;
	const ST_PLUGIN_BINARY_VALUE* pTokens =
		FindPluginBinaryField(p_refRequest,
			"AckTokens");
	if (!ReadString(p_refRequest, "Consumer",
			strConsumer, p_refError) ||
		!IsValidConsumerName(strConsumer) ||
		pTokens == nullptr ||
		pTokens->enType !=
			EN_PLUGIN_BINARY_VALUE_ARRAY ||
		pTokens->aArrayValue.empty() ||
		pTokens->aArrayValue.size() >
			m_stConfig.uiFetchBatchMax)
	{
		p_refCode = EN_TERMINAL_ERROR_INVALID_REQUEST;
		p_refMessage = p_refError.empty() ?
			"RELIABLE_EVENT_ACK_REQUEST_INVALID: Consumer and non-empty AckTokens are required" :
			p_refError;
		p_refError.clear();
		return true;
	}

	std::shared_lock<std::shared_mutex> clApiLock(
		m_clApiMutex);
	std::uint64_t ullAcked = 0;
	const auto fnBuildAckProgress =
		[&p_refData, &strConsumer, &ullAcked,
		 &p_refError](const std::string& p_refFailedToken)
	{
		if (!AddString(*p_refData, "Consumer",
				strConsumer, p_refError) ||
			!AddUInt64(*p_refData, "Acked",
				ullAcked, p_refError))
		{
			return false;
		}
		if (!p_refFailedToken.empty() &&
			!AddString(*p_refData, "FailedToken",
				p_refFailedToken, p_refError))
		{
			return false;
		}
		return true;
	};
	for (std::size_t szIndex = 0;
		szIndex < pTokens->aArrayValue.size(); ++szIndex)
	{
		const std::shared_ptr<ST_PLUGIN_BINARY_VALUE>&
			refToken = pTokens->aArrayValue[szIndex];
		if (!refToken ||
			refToken->enType !=
				EN_PLUGIN_BINARY_VALUE_STRING ||
			refToken->strStringValue.empty())
		{
			p_refCode =
				EN_TERMINAL_ERROR_INVALID_REQUEST;
			p_refMessage =
				"RELIABLE_EVENT_ACK_TOKEN_INVALID: every AckTokens item must be a non-empty string";
			return fnBuildAckProgress(
				std::string());
		}
		natsMsg* pMessage = nullptr;
		{
			std::lock_guard<std::mutex>
				clConsumerLock(m_clConsumerMutex);
			std::map<std::string, std::string>::
				const_iterator itConsumer =
				m_mapPendingConsumer.find(
					refToken->strStringValue);
			std::map<std::string, natsMsg*>::
				iterator itMessage =
				m_mapPendingAck.find(
					refToken->strStringValue);
			if (itConsumer == m_mapPendingConsumer.end() ||
				itMessage == m_mapPendingAck.end() ||
				itConsumer->second != strConsumer)
			{
				p_refCode =
					EN_TERMINAL_ERROR_RELIABLE_EVENT_ACK;
				p_refMessage =
					"RELIABLE_EVENT_ACK_TOKEN_NOT_FOUND: token=" +
					refToken->strStringValue +
					", consumer=" + strConsumer;
				return fnBuildAckProgress(
					refToken->strStringValue);
			}
			pMessage = itMessage->second;
			// 先从 pending 表中摘除所有权，防止两个并发 ACK 重复释放同一消息。
			m_mapPendingAck.erase(itMessage);
			m_mapPendingConsumer.erase(
				refToken->strStringValue);
		}

		jsErrCode enJetStreamError =
			static_cast<jsErrCode>(0);
		const natsStatus enStatus =
			natsMsg_AckSync(pMessage, nullptr,
				&enJetStreamError);
		if (enStatus != NATS_OK)
		{
			// 服务端未确认时恢复 pending 所有权，调用方可使用同一 token 重试。
			std::lock_guard<std::mutex>
				clConsumerLock(m_clConsumerMutex);
			m_mapPendingAck[
				refToken->strStringValue] = pMessage;
			m_mapPendingConsumer[
				refToken->strStringValue] =
				strConsumer;
			p_refCode =
				EN_TERMINAL_ERROR_RELIABLE_EVENT_ACK;
			p_refMessage = BuildNatsError(
				"natsMsg_AckSync",
				static_cast<int>(enStatus),
				static_cast<int>(
					enJetStreamError));
			return fnBuildAckProgress(
				refToken->strStringValue);
		}
		natsMsg_Destroy(pMessage);
		++ullAcked;
	}
	if (!fnBuildAckProgress(std::string()))
	{
		return false;
	}
	return true;
}

bool CMtReliableEventStore::Status(
	const ST_PLUGIN_BINARY_VALUE& p_refRequest,
	std::int32_t& p_refCode,
	std::string& p_refMessage,
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& p_refData,
	std::string& p_refError)
{
	p_refCode = EN_TERMINAL_ERROR_OK;
	p_refMessage = "OK";
	p_refError.clear();
	p_refData = MakeErrorData(p_refError);
	if (!p_refData)
	{
		return false;
	}
	std::string strConsumer;
	if (!ReadString(p_refRequest, "Consumer",
			strConsumer, p_refError, false) ||
		(!strConsumer.empty() &&
			!IsValidConsumerName(strConsumer)))
	{
		p_refCode = EN_TERMINAL_ERROR_INVALID_REQUEST;
		p_refMessage = p_refError.empty() ?
			"RELIABLE_EVENT_CONSUMER_INVALID" :
			p_refError;
		p_refError.clear();
		return true;
	}
	if (!AddBool(*p_refData, "Enabled",
			m_stConfig.bEnable, p_refError) ||
		!AddBool(*p_refData, "Ready",
			m_bReady.load(), p_refError) ||
		!AddString(*p_refData, "Stream",
			m_stConfig.strStream, p_refError))
	{
		return false;
	}
	if (!m_stConfig.bEnable)
	{
		return true;
	}
	if (!m_bReady.load())
	{
		p_refCode =
			EN_TERMINAL_ERROR_RELIABLE_EVENT_BACKEND;
		p_refMessage =
			"RELIABLE_EVENT_BACKEND_NOT_READY: JetStream connection is unavailable";
		return true;
	}

	std::shared_lock<std::shared_mutex> clApiLock(
		m_clApiMutex);
	jsStreamInfo* pStreamInfo = nullptr;
	jsErrCode enJetStreamError =
		static_cast<jsErrCode>(0);
	natsStatus enStatus = js_GetStreamInfo(
		&pStreamInfo, m_pJetStream,
		m_stConfig.strStream.c_str(), nullptr,
		&enJetStreamError);
	if (enStatus != NATS_OK ||
		pStreamInfo == nullptr)
	{
		p_refCode =
			EN_TERMINAL_ERROR_RELIABLE_EVENT_BACKEND;
		p_refMessage = BuildNatsError(
			"js_GetStreamInfo",
			static_cast<int>(enStatus),
			static_cast<int>(enJetStreamError));
		return true;
	}
	const bool bStreamAdded =
		AddUInt64(*p_refData, "Messages",
			pStreamInfo->State.Msgs, p_refError) &&
		AddUInt64(*p_refData, "Bytes",
			pStreamInfo->State.Bytes, p_refError) &&
		AddUInt64(*p_refData, "FirstSequence",
			pStreamInfo->State.FirstSeq, p_refError) &&
		AddUInt64(*p_refData, "LastSequence",
			pStreamInfo->State.LastSeq, p_refError) &&
		AddInt64(*p_refData, "Consumers",
			pStreamInfo->State.Consumers, p_refError);
	jsStreamInfo_Destroy(pStreamInfo);
	if (!bStreamAdded)
	{
		return false;
	}
	if (strConsumer.empty())
	{
		return true;
	}

	jsConsumerInfo* pConsumerInfo = nullptr;
	enJetStreamError =
		static_cast<jsErrCode>(0);
	enStatus = js_GetConsumerInfo(
		&pConsumerInfo, m_pJetStream,
		m_stConfig.strStream.c_str(),
		strConsumer.c_str(), nullptr,
		&enJetStreamError);
	if (enStatus == NATS_NOT_FOUND)
	{
		if (!AddString(*p_refData, "Consumer",
				strConsumer, p_refError) ||
			!AddBool(*p_refData, "ConsumerExists",
				false, p_refError))
		{
			return false;
		}
		return true;
	}
	if (enStatus != NATS_OK ||
		pConsumerInfo == nullptr)
	{
		p_refCode =
			EN_TERMINAL_ERROR_RELIABLE_EVENT_BACKEND;
		p_refMessage = BuildNatsError(
			"js_GetConsumerInfo",
			static_cast<int>(enStatus),
			static_cast<int>(enJetStreamError));
		return true;
	}
	const bool bConsumerAdded =
		AddString(*p_refData, "Consumer",
			strConsumer, p_refError) &&
		AddBool(*p_refData, "ConsumerExists",
			true, p_refError) &&
		AddUInt64(*p_refData, "Pending",
			pConsumerInfo->NumPending, p_refError) &&
		AddInt64(*p_refData, "AckPending",
			pConsumerInfo->NumAckPending,
			p_refError) &&
		AddInt64(*p_refData, "Redelivered",
			pConsumerInfo->NumRedelivered,
			p_refError);
	jsConsumerInfo_Destroy(pConsumerInfo);
	return bConsumerAdded;
}

bool CMtReliableEventStore::IsReady() const
{
	return m_bReady.load();
}

bool CMtReliableEventStore::ValidateAppendRequest(
	const ST_PLUGIN_BINARY_VALUE& p_refRequest,
	std::string& p_refEventId,
	std::string& p_refSubject,
	EN_PLUGIN_NOTIFY_ID& p_refNotifyId,
	EN_PLUGIN_NOTIFY_ACTION& p_refAction,
	std::uint64_t& p_refSourceSequence,
	std::int64_t& p_refCreatedTimeMs,
	std::string& p_refError) const
{
	std::int64_t llVersion = 0;
	std::int64_t llNo = 0;
	std::int64_t llNotifyId = 0;
	std::int64_t llAction = 0;
	std::uint64_t ullSourceEpoch = 0;
	const ST_PLUGIN_BINARY_VALUE* pPayload =
		FindPluginBinaryField(p_refRequest, "Payload");
	if (!ReadString(p_refRequest, "EventId",
			p_refEventId, p_refError) ||
		!ReadInt64(p_refRequest, "Version",
			llVersion, p_refError) ||
		!ReadInt64(p_refRequest, "No",
			llNo, p_refError) ||
		!ReadInt64(p_refRequest, "NotifyId",
			llNotifyId, p_refError) ||
		!ReadInt64(p_refRequest, "Action",
			llAction, p_refError) ||
		!ReadUInt64(p_refRequest, "SourceEpoch",
			ullSourceEpoch, p_refError) ||
		!ReadUInt64(p_refRequest, "SourceSequence",
			p_refSourceSequence, p_refError) ||
		!ReadInt64(p_refRequest, "CreatedTimeMs",
			p_refCreatedTimeMs, p_refError) ||
		pPayload == nullptr ||
		pPayload->enType !=
			EN_PLUGIN_BINARY_VALUE_BYTES)
	{
		if (p_refError.empty())
		{
			p_refError =
				"RELIABLE_EVENT_PAYLOAD_INVALID: Payload must be bytes";
		}
		return false;
	}
	if (p_refEventId.empty() ||
		p_refEventId.size() > 160 ||
		(llVersion != 4 && llVersion != 5) ||
		llNo <= 0 ||
		!IsPluginNotifyId(llNotifyId) ||
		(llNotifyId < EN_PLUGIN_NOTIFY_ORDER_CHANGED ||
		 llNotifyId >
			EN_PLUGIN_NOTIFY_SERVER_TIME_CHANGED) ||
		llAction < EN_PLUGIN_NOTIFY_ACTION_CREATED ||
		llAction > EN_PLUGIN_NOTIFY_ACTION_RESET ||
		ullSourceEpoch == 0 ||
		p_refSourceSequence == 0 ||
		pPayload->aByteValue.empty())
	{
		p_refError =
			"RELIABLE_EVENT_APPEND_FIELD_INVALID: EventId, Version, No, NotifyId, Action, SourceEpoch, SourceSequence or Payload is out of range";
		return false;
	}
	p_refNotifyId =
		static_cast<EN_PLUGIN_NOTIFY_ID>(llNotifyId);
	p_refAction =
		static_cast<EN_PLUGIN_NOTIFY_ACTION>(llAction);
	std::ostringstream clSubject;
	clSubject << m_stConfig.strSubjectPrefix <<
		".v" << llVersion << ".n" << llNo <<
		"." << llNotifyId;
	p_refSubject = clSubject.str();
	return true;
}

natsSubscription*
CMtReliableEventStore::GetOrCreateSubscription(
	const std::string& p_refConsumer,
	std::string& p_refError)
{
	std::map<std::string, natsSubscription*>::
		const_iterator it =
		m_mapSubscription.find(p_refConsumer);
	if (it != m_mapSubscription.end())
	{
		return it->second;
	}
	jsSubOptions stOptions;
	jsSubOptions_Init(&stOptions);
	stOptions.Stream = m_stConfig.strStream.c_str();
	stOptions.ManualAck = true;
	stOptions.Config.Durable =
		p_refConsumer.c_str();
	stOptions.Config.DeliverPolicy = js_DeliverAll;
	stOptions.Config.AckPolicy = js_AckExplicit;
	stOptions.Config.MaxDeliver =
		static_cast<std::int64_t>(
			m_stConfig.uiMaxDeliver);
	stOptions.Config.AckWait =
		static_cast<std::int64_t>(
			m_stConfig.uiFetchWaitMs) *
			1000000LL * 30LL;
	const std::string strSubject =
		m_stConfig.strSubjectPrefix + ".>";
	natsSubscription* pSubscription = nullptr;
	jsErrCode enJetStreamError =
		static_cast<jsErrCode>(0);
	const natsStatus enStatus =
		js_PullSubscribe(&pSubscription,
			m_pJetStream, strSubject.c_str(),
			p_refConsumer.c_str(), nullptr,
			&stOptions, &enJetStreamError);
	if (enStatus != NATS_OK ||
		pSubscription == nullptr)
	{
		p_refError = BuildNatsError(
			"js_PullSubscribe",
			static_cast<int>(enStatus),
			static_cast<int>(enJetStreamError));
		return nullptr;
	}
	m_mapSubscription[p_refConsumer] =
		pSubscription;
	return pSubscription;
}

bool CMtReliableEventStore::IsValidConsumerName(
	const std::string& p_refConsumer)
{
	if (p_refConsumer.empty() ||
		p_refConsumer.size() > 128)
	{
		return false;
	}
	for (std::size_t szIndex = 0;
		szIndex < p_refConsumer.size(); ++szIndex)
	{
		const unsigned char ucValue =
			static_cast<unsigned char>(
				p_refConsumer[szIndex]);
		if (!std::isalnum(ucValue) &&
			ucValue != '_' && ucValue != '-')
		{
			return false;
		}
	}
	return true;
}

std::string CMtReliableEventStore::BuildAckToken(
	const std::string& p_refConsumer,
	std::uint64_t p_ullStreamSequence)
{
	std::ostringstream clToken;
	clToken << p_refConsumer << "-" <<
		p_ullStreamSequence;
	return clToken.str();
}

std::string CMtReliableEventStore::BuildNatsError(
	const char* p_szOperation, int p_iStatus,
	int p_iJetStreamError)
{
	const char* pStatusText =
		natsStatus_GetText(
			static_cast<natsStatus>(p_iStatus));
	return std::string(
		"RELIABLE_EVENT_NATS_ERROR: operation=") +
		(p_szOperation != nullptr ? p_szOperation :
			"unknown") + ", status=" +
		std::to_string(p_iStatus) + ", statusText=" +
		(pStatusText != nullptr ? pStatusText :
			"UNKNOWN") + ", jetStreamError=" +
		std::to_string(p_iJetStreamError);
}

bool CMtReliableEventStore::EnsureStream(
	std::string& p_refError)
{
	const std::string strSubject =
		m_stConfig.strSubjectPrefix + ".>";
	const char* aSubject[1] =
	{
		strSubject.c_str()
	};
	jsStreamConfig stConfig;
	jsStreamConfig_Init(&stConfig);
	stConfig.Name = m_stConfig.strStream.c_str();
	stConfig.Subjects = aSubject;
	stConfig.SubjectsLen = 1;
	stConfig.Retention = js_LimitsPolicy;
	stConfig.Storage = js_FileStorage;
	stConfig.Replicas = static_cast<int>(m_stConfig.uiReplicas);
	stConfig.Discard = js_DiscardOld;
	stConfig.MaxBytes = m_stConfig.llMaxBytes;
	stConfig.MaxAge =
		m_stConfig.llMaxAgeSeconds *
		1000000000LL;
	stConfig.Duplicates =
		m_stConfig.llDuplicateWindowSeconds *
		1000000000LL;

	jsStreamInfo* pStreamInfo = nullptr;
	jsErrCode enJetStreamError =
		static_cast<jsErrCode>(0);
	natsStatus enStatus = js_GetStreamInfo(
		&pStreamInfo, m_pJetStream,
		m_stConfig.strStream.c_str(), nullptr,
		&enJetStreamError);
	if (enStatus == NATS_OK && pStreamInfo != nullptr)
	{
		jsStreamInfo_Destroy(pStreamInfo);
		pStreamInfo = nullptr;
		enJetStreamError =
			static_cast<jsErrCode>(0);
		enStatus = js_UpdateStream(&pStreamInfo,
			m_pJetStream, &stConfig, nullptr,
			&enJetStreamError);
	}
	else if (enStatus == NATS_NOT_FOUND)
	{
		enJetStreamError =
			static_cast<jsErrCode>(0);
		enStatus = js_AddStream(&pStreamInfo,
			m_pJetStream, &stConfig, nullptr,
			&enJetStreamError);
	}
	if (enStatus != NATS_OK ||
		pStreamInfo == nullptr)
	{
		if (pStreamInfo != nullptr)
		{
			jsStreamInfo_Destroy(pStreamInfo);
		}
		p_refError = BuildNatsError(
			"EnsureStream",
			static_cast<int>(enStatus),
			static_cast<int>(enJetStreamError));
		return false;
	}
	jsStreamInfo_Destroy(pStreamInfo);
	return true;
}
