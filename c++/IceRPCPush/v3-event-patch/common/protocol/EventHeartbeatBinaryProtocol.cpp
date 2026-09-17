#include "EventHeartbeatBinaryProtocol.h"

#include <limits>

namespace
{
	static const std::uint32_t EVENT_HEARTBEAT_MAGIC = 0x48455645U;
	static const std::uint16_t EVENT_HEARTBEAT_VERSION = 1;
	static const std::uint16_t EVENT_HEARTBEAT_REQUEST_SIZE = 40;
	static const std::uint16_t EVENT_HEARTBEAT_RESPONSE_SIZE = 136;
	static const std::size_t EVENT_HEARTBEAT_SOURCE_SIZE = 32;
	static const std::size_t EVENT_HEARTBEAT_MAX_SOURCES = 4096;
	static const std::uint32_t EVENT_HEARTBEAT_FLAG_JETSTREAM_READY = 0x1U;

	// 写入固定宽度小端整数，协议不依赖平台结构体布局。
	template <typename TValue>
	void AppendUnsigned(std::vector<unsigned char>& p_refBuffer,
		TValue p_uValue)
	{
		for (std::size_t szIndex = 0; szIndex < sizeof(TValue); ++szIndex)
		{
			p_refBuffer.push_back(static_cast<unsigned char>(
				(p_uValue >> (szIndex * 8U)) & 0xFFU));
		}
	}

	// 读取固定宽度小端整数；失败时不访问输入边界之外的内存。
	template <typename TValue>
	bool ReadUnsigned(const unsigned char* p_pBuffer,
		std::size_t p_szBufferLen, std::size_t& p_refOffset,
		TValue& p_refValue)
	{
		if (p_pBuffer == nullptr || p_refOffset > p_szBufferLen ||
			p_szBufferLen - p_refOffset < sizeof(TValue))
		{
			return false;
		}
		TValue uValue = 0;
		for (std::size_t szIndex = 0; szIndex < sizeof(TValue); ++szIndex)
		{
			uValue |= static_cast<TValue>(
				static_cast<TValue>(p_pBuffer[p_refOffset + szIndex]) <<
				(szIndex * 8U));
		}
		p_refOffset += sizeof(TValue);
		p_refValue = uValue;
		return true;
	}

	// 写入有符号整数的补码位模式。
	void AppendInt64(std::vector<unsigned char>& p_refBuffer,
		std::int64_t p_llValue)
	{
		AppendUnsigned<std::uint64_t>(p_refBuffer,
			static_cast<std::uint64_t>(p_llValue));
	}

	// 六服务身份必须处于公开枚举范围内。
	bool IsValidPluginId(EN_PLUGIN_ID p_enPluginId)
	{
		return p_enPluginId >= EN_PLUGIN_ID_MT_GATEWAY_SERVICE &&
			p_enPluginId <= EN_PLUGIN_ID_MT_DERIVE_SERVICE;
	}

	// 校验请求身份和时间；调用线程不访问全局状态。
	bool ValidateRequest(const ST_EVENT_HEARTBEAT_REQUEST& p_refRequest,
		std::string& p_refError)
	{
		if (p_refRequest.usVersion != EVENT_HEARTBEAT_VERSION ||
			!IsValidPluginId(p_refRequest.enSenderPluginId) ||
			p_refRequest.enTargetPluginId != EN_PLUGIN_ID_MT_EVENT_SERVICE ||
			p_refRequest.enSenderPluginId == EN_PLUGIN_ID_MT_EVENT_SERVICE ||
			p_refRequest.ullRequestSequence == 0 ||
			p_refRequest.ullSenderProcessEpoch == 0 ||
			p_refRequest.llSentAtMs <= 0)
		{
			p_refError =
				"EVENT_HEARTBEAT_REQUEST_INVALID: version, plugin identity, sequence, process epoch or time is invalid";
			return false;
		}
		return true;
	}

	// 校验应答公共字段和逐来源水位，不要求计数之间存在固定大小关系。
	bool ValidateResponse(const ST_EVENT_HEARTBEAT_RESPONSE& p_refResponse,
		std::string& p_refError)
	{
		if (p_refResponse.usVersion != EVENT_HEARTBEAT_VERSION ||
			!IsValidPluginId(p_refResponse.enRequesterPluginId) ||
			p_refResponse.enRequesterPluginId == EN_PLUGIN_ID_MT_EVENT_SERVICE ||
			p_refResponse.enResponderPluginId != EN_PLUGIN_ID_MT_EVENT_SERVICE ||
			p_refResponse.ullRequestSequence == 0 ||
			p_refResponse.ullResponderProcessEpoch == 0 ||
			p_refResponse.llRespondedAtMs <= 0 ||
			p_refResponse.aSource.size() > EVENT_HEARTBEAT_MAX_SOURCES)
		{
			p_refError =
				"EVENT_HEARTBEAT_RESPONSE_INVALID: identity, sequence, process epoch, time or source count is invalid";
			return false;
		}
		for (const ST_EVENT_HEARTBEAT_SOURCE_STATUS& refSource :
			p_refResponse.aSource)
		{
			if ((refSource.usPlatformVersion != 4 &&
				 refSource.usPlatformVersion != 5) ||
				refSource.iSourceNo <= 0 || refSource.ullSourceEpoch == 0 ||
				refSource.ullLastSequence == 0)
			{
				p_refError =
					"EVENT_HEARTBEAT_SOURCE_INVALID: source Version, No, Epoch or Sequence is invalid";
				return false;
			}
		}
		return true;
	}
}

ST_EVENT_HEARTBEAT_SOURCE_STATUS::ST_EVENT_HEARTBEAT_SOURCE_STATUS()
	: usPlatformVersion(0)
	, iSourceNo(0)
	, ullSourceEpoch(0)
	, ullLastSequence(0)
	, ullGapCount(0)
{
}

ST_EVENT_HEARTBEAT_REQUEST::ST_EVENT_HEARTBEAT_REQUEST()
	: usVersion(EVENT_HEARTBEAT_VERSION)
	, enSenderPluginId(EN_PLUGIN_ID_INVALID)
	, enTargetPluginId(EN_PLUGIN_ID_MT_EVENT_SERVICE)
	, ullRequestSequence(0)
	, ullSenderProcessEpoch(0)
	, llSentAtMs(0)
{
}

ST_EVENT_HEARTBEAT_RESPONSE::ST_EVENT_HEARTBEAT_RESPONSE()
	: usVersion(EVENT_HEARTBEAT_VERSION)
	, enRequesterPluginId(EN_PLUGIN_ID_INVALID)
	, enResponderPluginId(EN_PLUGIN_ID_MT_EVENT_SERVICE)
	, bJetStreamReady(false)
	, ullRequestSequence(0)
	, ullResponderProcessEpoch(0)
	, llRespondedAtMs(0)
	, ullQueueCapacity(0)
	, ullQueueDepth(0)
	, ullReceivedCount(0)
	, ullValidationRejectedCount(0)
	, ullDuplicateCount(0)
	, ullOutOfOrderCount(0)
	, ullEpochResetCount(0)
	, ullSequenceGapCount(0)
	, ullCapacityDroppedCount(0)
	, ullFanoutCount(0)
	, ullPublishFailedCount(0)
	, aSource()
{
}

bool EncodeEventHeartbeatRequest(const ST_EVENT_HEARTBEAT_REQUEST& p_refRequest,
	std::vector<unsigned char>& p_refBuffer, std::string& p_refError)
{
	p_refBuffer.clear();
	p_refError.clear();
	if (!ValidateRequest(p_refRequest, p_refError))
	{
		return false;
	}
	p_refBuffer.reserve(EVENT_HEARTBEAT_REQUEST_SIZE);
	AppendUnsigned<std::uint32_t>(p_refBuffer, EVENT_HEARTBEAT_MAGIC);
	AppendUnsigned<std::uint16_t>(p_refBuffer, p_refRequest.usVersion);
	AppendUnsigned<std::uint16_t>(p_refBuffer, EVENT_HEARTBEAT_REQUEST_SIZE);
	AppendUnsigned<std::uint16_t>(p_refBuffer, p_refRequest.enSenderPluginId);
	AppendUnsigned<std::uint16_t>(p_refBuffer, p_refRequest.enTargetPluginId);
	AppendUnsigned<std::uint32_t>(p_refBuffer, 0);
	AppendUnsigned<std::uint64_t>(p_refBuffer, p_refRequest.ullRequestSequence);
	AppendUnsigned<std::uint64_t>(p_refBuffer, p_refRequest.ullSenderProcessEpoch);
	AppendInt64(p_refBuffer, p_refRequest.llSentAtMs);
	return true;
}

bool DecodeEventHeartbeatRequest(const unsigned char* p_pBuffer,
	std::size_t p_szBufferLen, ST_EVENT_HEARTBEAT_REQUEST& p_refRequest,
	std::string& p_refError)
{
	p_refRequest = ST_EVENT_HEARTBEAT_REQUEST();
	p_refError.clear();
	std::size_t szOffset = 0;
	std::uint32_t uiMagic = 0;
	std::uint16_t usHeaderSize = 0;
	std::uint16_t usSender = 0;
	std::uint16_t usTarget = 0;
	std::uint32_t uiReserved = 0;
	std::uint64_t ullSentAtMs = 0;
	if (p_pBuffer == nullptr || p_szBufferLen != EVENT_HEARTBEAT_REQUEST_SIZE ||
		!ReadUnsigned<std::uint32_t>(p_pBuffer, p_szBufferLen, szOffset, uiMagic) ||
		!ReadUnsigned<std::uint16_t>(p_pBuffer, p_szBufferLen, szOffset,
			p_refRequest.usVersion) ||
		!ReadUnsigned<std::uint16_t>(p_pBuffer, p_szBufferLen, szOffset,
			usHeaderSize) ||
		!ReadUnsigned<std::uint16_t>(p_pBuffer, p_szBufferLen, szOffset, usSender) ||
		!ReadUnsigned<std::uint16_t>(p_pBuffer, p_szBufferLen, szOffset, usTarget) ||
		!ReadUnsigned<std::uint32_t>(p_pBuffer, p_szBufferLen, szOffset,
			uiReserved) ||
		!ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen, szOffset,
			p_refRequest.ullRequestSequence) ||
		!ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen, szOffset,
			p_refRequest.ullSenderProcessEpoch) ||
		!ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen, szOffset,
			ullSentAtMs) || szOffset != p_szBufferLen ||
		uiMagic != EVENT_HEARTBEAT_MAGIC ||
		usHeaderSize != EVENT_HEARTBEAT_REQUEST_SIZE || uiReserved != 0)
	{
		p_refError =
			"EVENT_HEARTBEAT_REQUEST_DECODE_FAILED: magic, header, reserved field or length is invalid";
		return false;
	}
	p_refRequest.enSenderPluginId = static_cast<EN_PLUGIN_ID>(usSender);
	p_refRequest.enTargetPluginId = static_cast<EN_PLUGIN_ID>(usTarget);
	p_refRequest.llSentAtMs = static_cast<std::int64_t>(ullSentAtMs);
	return ValidateRequest(p_refRequest, p_refError);
}

bool EncodeEventHeartbeatResponse(const ST_EVENT_HEARTBEAT_RESPONSE& p_refResponse,
	std::vector<unsigned char>& p_refBuffer, std::string& p_refError)
{
	p_refBuffer.clear();
	p_refError.clear();
	if (!ValidateResponse(p_refResponse, p_refError))
	{
		return false;
	}
	const std::size_t szPayloadSize = EVENT_HEARTBEAT_RESPONSE_SIZE +
		p_refResponse.aSource.size() * EVENT_HEARTBEAT_SOURCE_SIZE;
	if (szPayloadSize > (std::numeric_limits<std::uint32_t>::max)())
	{
		p_refError =
			"EVENT_HEARTBEAT_RESPONSE_TOO_LARGE: encoded response exceeds uint32 range";
		return false;
	}
	p_refBuffer.reserve(szPayloadSize);
	AppendUnsigned<std::uint32_t>(p_refBuffer, EVENT_HEARTBEAT_MAGIC);
	AppendUnsigned<std::uint16_t>(p_refBuffer, p_refResponse.usVersion);
	AppendUnsigned<std::uint16_t>(p_refBuffer, EVENT_HEARTBEAT_RESPONSE_SIZE);
	AppendUnsigned<std::uint16_t>(p_refBuffer, p_refResponse.enRequesterPluginId);
	AppendUnsigned<std::uint16_t>(p_refBuffer, p_refResponse.enResponderPluginId);
	AppendUnsigned<std::uint32_t>(p_refBuffer,
		p_refResponse.bJetStreamReady ? EVENT_HEARTBEAT_FLAG_JETSTREAM_READY : 0U);
	AppendUnsigned<std::uint64_t>(p_refBuffer, p_refResponse.ullRequestSequence);
	AppendUnsigned<std::uint64_t>(p_refBuffer,
		p_refResponse.ullResponderProcessEpoch);
	AppendInt64(p_refBuffer, p_refResponse.llRespondedAtMs);
	AppendUnsigned<std::uint64_t>(p_refBuffer, p_refResponse.ullQueueCapacity);
	AppendUnsigned<std::uint64_t>(p_refBuffer, p_refResponse.ullQueueDepth);
	AppendUnsigned<std::uint64_t>(p_refBuffer, p_refResponse.ullReceivedCount);
	AppendUnsigned<std::uint64_t>(p_refBuffer,
		p_refResponse.ullValidationRejectedCount);
	AppendUnsigned<std::uint64_t>(p_refBuffer, p_refResponse.ullDuplicateCount);
	AppendUnsigned<std::uint64_t>(p_refBuffer, p_refResponse.ullOutOfOrderCount);
	AppendUnsigned<std::uint64_t>(p_refBuffer, p_refResponse.ullEpochResetCount);
	AppendUnsigned<std::uint64_t>(p_refBuffer, p_refResponse.ullSequenceGapCount);
	AppendUnsigned<std::uint64_t>(p_refBuffer,
		p_refResponse.ullCapacityDroppedCount);
	AppendUnsigned<std::uint64_t>(p_refBuffer, p_refResponse.ullFanoutCount);
	AppendUnsigned<std::uint64_t>(p_refBuffer, p_refResponse.ullPublishFailedCount);
	AppendUnsigned<std::uint32_t>(p_refBuffer,
		static_cast<std::uint32_t>(p_refResponse.aSource.size()));
	AppendUnsigned<std::uint32_t>(p_refBuffer, 0);
	for (const ST_EVENT_HEARTBEAT_SOURCE_STATUS& refSource :
		p_refResponse.aSource)
	{
		AppendUnsigned<std::uint16_t>(p_refBuffer, refSource.usPlatformVersion);
		AppendUnsigned<std::uint16_t>(p_refBuffer, 0);
		AppendUnsigned<std::uint32_t>(p_refBuffer,
			static_cast<std::uint32_t>(refSource.iSourceNo));
		AppendUnsigned<std::uint64_t>(p_refBuffer, refSource.ullSourceEpoch);
		AppendUnsigned<std::uint64_t>(p_refBuffer, refSource.ullLastSequence);
		AppendUnsigned<std::uint64_t>(p_refBuffer, refSource.ullGapCount);
	}
	return true;
}

bool DecodeEventHeartbeatResponse(const unsigned char* p_pBuffer,
	std::size_t p_szBufferLen, ST_EVENT_HEARTBEAT_RESPONSE& p_refResponse,
	std::string& p_refError)
{
	p_refResponse = ST_EVENT_HEARTBEAT_RESPONSE();
	p_refError.clear();
	if (p_pBuffer == nullptr || p_szBufferLen < EVENT_HEARTBEAT_RESPONSE_SIZE)
	{
		p_refError =
			"EVENT_HEARTBEAT_RESPONSE_TOO_SHORT: payload is shorter than the fixed response";
		return false;
	}
	std::size_t szOffset = 0;
	std::uint32_t uiMagic = 0;
	std::uint16_t usHeaderSize = 0;
	std::uint16_t usRequester = 0;
	std::uint16_t usResponder = 0;
	std::uint32_t uiFlags = 0;
	std::uint64_t ullRespondedAtMs = 0;
	std::uint32_t uiSourceCount = 0;
	std::uint32_t uiReserved = 0;
	bool bDecoded =
		ReadUnsigned<std::uint32_t>(p_pBuffer, p_szBufferLen, szOffset, uiMagic) &&
		ReadUnsigned<std::uint16_t>(p_pBuffer, p_szBufferLen, szOffset,
			p_refResponse.usVersion) &&
		ReadUnsigned<std::uint16_t>(p_pBuffer, p_szBufferLen, szOffset,
			usHeaderSize) &&
		ReadUnsigned<std::uint16_t>(p_pBuffer, p_szBufferLen, szOffset,
			usRequester) &&
		ReadUnsigned<std::uint16_t>(p_pBuffer, p_szBufferLen, szOffset,
			usResponder) &&
		ReadUnsigned<std::uint32_t>(p_pBuffer, p_szBufferLen, szOffset, uiFlags) &&
		ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen, szOffset,
			p_refResponse.ullRequestSequence) &&
		ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen, szOffset,
			p_refResponse.ullResponderProcessEpoch) &&
		ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen, szOffset,
			ullRespondedAtMs) &&
		ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen, szOffset,
			p_refResponse.ullQueueCapacity) &&
		ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen, szOffset,
			p_refResponse.ullQueueDepth) &&
		ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen, szOffset,
			p_refResponse.ullReceivedCount) &&
		ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen, szOffset,
			p_refResponse.ullValidationRejectedCount) &&
		ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen, szOffset,
			p_refResponse.ullDuplicateCount) &&
		ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen, szOffset,
			p_refResponse.ullOutOfOrderCount) &&
		ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen, szOffset,
			p_refResponse.ullEpochResetCount) &&
		ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen, szOffset,
			p_refResponse.ullSequenceGapCount) &&
		ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen, szOffset,
			p_refResponse.ullCapacityDroppedCount) &&
		ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen, szOffset,
			p_refResponse.ullFanoutCount) &&
		ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen, szOffset,
			p_refResponse.ullPublishFailedCount) &&
		ReadUnsigned<std::uint32_t>(p_pBuffer, p_szBufferLen, szOffset,
			uiSourceCount) &&
		ReadUnsigned<std::uint32_t>(p_pBuffer, p_szBufferLen, szOffset,
			uiReserved);
	if (!bDecoded || uiMagic != EVENT_HEARTBEAT_MAGIC ||
		usHeaderSize != EVENT_HEARTBEAT_RESPONSE_SIZE ||
		(uiFlags & ~EVENT_HEARTBEAT_FLAG_JETSTREAM_READY) != 0 ||
		uiReserved != 0 || uiSourceCount > EVENT_HEARTBEAT_MAX_SOURCES ||
		p_szBufferLen - szOffset !=
			static_cast<std::size_t>(uiSourceCount) * EVENT_HEARTBEAT_SOURCE_SIZE)
	{
		p_refError =
			"EVENT_HEARTBEAT_RESPONSE_DECODE_FAILED: header, flags, source count or length is invalid";
		return false;
	}
	p_refResponse.enRequesterPluginId = static_cast<EN_PLUGIN_ID>(usRequester);
	p_refResponse.enResponderPluginId = static_cast<EN_PLUGIN_ID>(usResponder);
	p_refResponse.bJetStreamReady =
		(uiFlags & EVENT_HEARTBEAT_FLAG_JETSTREAM_READY) != 0;
	p_refResponse.llRespondedAtMs = static_cast<std::int64_t>(ullRespondedAtMs);
	for (std::uint32_t uiIndex = 0; uiIndex < uiSourceCount; ++uiIndex)
	{
		ST_EVENT_HEARTBEAT_SOURCE_STATUS stSource;
		std::uint16_t usReserved = 0;
		std::uint32_t uiNo = 0;
		if (!ReadUnsigned<std::uint16_t>(p_pBuffer, p_szBufferLen, szOffset,
				stSource.usPlatformVersion) ||
			!ReadUnsigned<std::uint16_t>(p_pBuffer, p_szBufferLen, szOffset,
				usReserved) ||
			!ReadUnsigned<std::uint32_t>(p_pBuffer, p_szBufferLen, szOffset,
				uiNo) ||
			!ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen, szOffset,
				stSource.ullSourceEpoch) ||
			!ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen, szOffset,
				stSource.ullLastSequence) ||
			!ReadUnsigned<std::uint64_t>(p_pBuffer, p_szBufferLen, szOffset,
				stSource.ullGapCount) || usReserved != 0)
		{
			p_refError =
				"EVENT_HEARTBEAT_SOURCE_DECODE_FAILED: source item is truncated or reserved field is nonzero";
			return false;
		}
		stSource.iSourceNo = static_cast<std::int32_t>(uiNo);
		p_refResponse.aSource.push_back(stSource);
	}
	return szOffset == p_szBufferLen &&
		ValidateResponse(p_refResponse, p_refError);
}
