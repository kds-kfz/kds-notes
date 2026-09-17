#include "TradingTerminalProtocol.h"

#include <limits>

namespace
{
	static const std::uint32_t MT_TIME_STATE_MAGIC = 0x4D49544DU;
	static const std::uint32_t MT_TIME_REQUEST_MAGIC = 0x5154494DU;
	static const std::uint32_t MT_TIME_RESPONSE_MAGIC = 0x5254494DU;
	static const std::size_t MT_TIME_STATE_HEADER_SIZE = 70;
	static const std::size_t MT_TIME_REQUEST_SIZE = 16;
	static const std::size_t MT_TIME_RESPONSE_HEADER_SIZE = 12;
	static const std::size_t MT_TIME_MAX_STATE_COUNT = 1024;

	// 以小端顺序写入无符号整数，避免直接依赖编译器结构体布局。
	template <typename T>
	void WriteLittleEndian(std::vector<unsigned char>& p_refBuffer, T p_tValue)
	{
		for (std::size_t szIndex = 0; szIndex < sizeof(T); ++szIndex)
		{
			p_refBuffer.push_back(static_cast<unsigned char>((p_tValue >> (szIndex * 8)) & static_cast<T>(0xFF)));
		}
	}

	// 从缓冲区读取小端无符号整数，调用前已经校验缓冲区总长度。
	template <typename T>
	T ReadLittleEndian(const unsigned char* p_pBuffer)
	{
		T tValue = 0;
		for (std::size_t szIndex = 0; szIndex < sizeof(T); ++szIndex)
		{
			tValue |= static_cast<T>(p_pBuffer[szIndex]) << (szIndex * 8);
		}
		return tValue;
	}

	// 校验 UTF-8 文本并拒绝 NUL 和控制字符，避免时区 ID 在日志和 Windows API 边界被截断。
	bool IsValidUtf8Text(const std::string& p_refText)
	{
		std::size_t szIndex = 0;
		while (szIndex < p_refText.size())
		{
			const unsigned char chFirst = static_cast<unsigned char>(p_refText[szIndex]);
			if (chFirst <= 0x7FU)
			{
				if (chFirst < 0x20U || chFirst == 0x7FU)
				{
					return false;
				}
				++szIndex;
				continue;
			}
			std::size_t szContinuation = 0;
			unsigned char chSecondMin = 0x80U;
			unsigned char chSecondMax = 0xBFU;
			if (chFirst >= 0xC2U && chFirst <= 0xDFU)
			{
				szContinuation = 1;
			}
			else if (chFirst >= 0xE0U && chFirst <= 0xEFU)
			{
				szContinuation = 2;
				if (chFirst == 0xE0U)
				{
					chSecondMin = 0xA0U;
				}
				else if (chFirst == 0xEDU)
				{
					chSecondMax = 0x9FU;
				}
			}
			else if (chFirst >= 0xF0U && chFirst <= 0xF4U)
			{
				szContinuation = 3;
				if (chFirst == 0xF0U)
				{
					chSecondMin = 0x90U;
				}
				else if (chFirst == 0xF4U)
				{
					chSecondMax = 0x8FU;
				}
			}
			else
			{
				return false;
			}
			if (p_refText.size() - szIndex <= szContinuation)
			{
				return false;
			}
			const unsigned char chSecond = static_cast<unsigned char>(p_refText[szIndex + 1]);
			if (chSecond < chSecondMin || chSecond > chSecondMax)
			{
				return false;
			}
			for (std::size_t szOffset = 2; szOffset <= szContinuation; ++szOffset)
			{
				const unsigned char chCurrent = static_cast<unsigned char>(p_refText[szIndex + szOffset]);
				if (chCurrent < 0x80U || chCurrent > 0xBFU)
				{
					return false;
				}
			}
			szIndex += szContinuation + 1;
		}
		return true;
	}
}

ST_PLUGIN_NOTIFY_META::ST_PLUGIN_NOTIFY_META()
	: usVersion(TRADING_TERMINAL_PROTOCOL_VERSION)
	, usHeaderSize(static_cast<std::uint16_t>(TRADING_TERMINAL_NOTIFY_META_SIZE))
	, usSourcePlugin(EN_PLUGIN_ID_INVALID)
	, usNotifyMode(EN_PLUGIN_NOTIFY_MODE_INVALID)
	, usNotifyAction(EN_PLUGIN_NOTIFY_ACTION_INVALID)
	, usReserved(0)
	, uiPayloadLen(0)
	, ullSequence(0)
	, llTimestampMs(0)
{
}

ST_CLUSTER_FENCE::ST_CLUSTER_FENCE()
	: usVersion(CLUSTER_PROTOCOL_VERSION)
	, strOwnerInstanceId()
	, ullLeaseGeneration(0)
{
}

ST_CLUSTER_LEASE::ST_CLUSTER_LEASE()
	: usPlatformVersion(0)
	, iSourceNo(0)
	, enRole(EN_CLUSTER_ROLE_STANDBY)
	, strOwnerInstanceId()
	, ullLeaseGeneration(0)
	, llExpiresAtMs(0)
	, llLastRenewedMs(0)
	, bDataReady(false)
{
}

ST_CLUSTER_RUNTIME_METRIC::ST_CLUSTER_RUNTIME_METRIC()
	: strName()
	, ullValue(0)
{
}

ST_CLUSTER_INSTANCE::ST_CLUSTER_INSTANCE()
	: usVersion(CLUSTER_PROTOCOL_VERSION)
	, enPluginId(EN_PLUGIN_ID_INVALID)
	, enRole(EN_CLUSTER_ROLE_DISABLED)
	, strServiceName()
	, strInstanceId()
	, strNodeId()
	, strAdapterId()
	, bProcessReady(false)
	, llUpdatedAtMs(0)
	, strDetail()
{
}

ST_CLUSTER_STATE_RESPONSE::ST_CLUSTER_STATE_RESPONSE()
	: usVersion(CLUSTER_PROTOCOL_VERSION)
	, stInstance()
	, aLease()
	, aMetric()
{
}

ST_MT_TIME_SNAPSHOT_REQUEST::ST_MT_TIME_SNAPSHOT_REQUEST()
	: usVersion(MT_TIME_PROTOCOL_VERSION)
	, usPlatformVersion(0)
	, iSourceNo(0)
{
}

ST_MT_TIME_STATE::ST_MT_TIME_STATE()
	: usVersion(MT_TIME_PROTOCOL_VERSION)
	, usPlatformVersion(0)
	, iSourceNo(0)
	, strTimeZoneId()
	, iStandardOffsetSeconds(0)
	, iCurrentOffsetSeconds(0)
	, bDaylight(false)
	, enSyncState(EN_MT_TIME_SYNC_UNKNOWN)
	, enChangeReason(EN_MT_TIME_CHANGE_INITIAL)
	, ullAuthorityEpoch(0)
	, ullGeneration(0)
	, llEffectiveUtcMs(0)
	, llSampledUtcMs(0)
	, llValidUntilUtcMs(0)
{
}

ST_MT_TIME_SNAPSHOT_RESPONSE::ST_MT_TIME_SNAPSHOT_RESPONSE()
	: usVersion(MT_TIME_PROTOCOL_VERSION)
	, aState()
{
}

bool IsPluginFuncId(std::int64_t p_llFuncId)
{
	return p_llFuncId > PLUGIN_FUNC_COMMON_BEGIN && p_llFuncId <= PLUGIN_FUNC_DIAGNOSTIC_END &&
		(p_llFuncId % 10) != 0;
}

bool IsActiveMtTradeFuncId(std::int64_t p_llFuncId)
{
	switch (p_llFuncId)
	{
	case EN_PLUGIN_FUNC_TRADE_PLACE_ORDER:
	case EN_PLUGIN_FUNC_TRADE_PLACE_ORDER_DETAIL:
	case EN_PLUGIN_FUNC_TRADE_UPDATE_ORDER:
	case EN_PLUGIN_FUNC_TRADE_CANCEL_ORDER:
	case EN_PLUGIN_FUNC_TRADE_CREATE_ACCOUNT:
	case EN_PLUGIN_FUNC_TRADE_CLOSE_POSITION:
	case EN_PLUGIN_FUNC_TRADE_BATCH_CLOSE_POSITION:
	case EN_PLUGIN_FUNC_TRADE_MODIFY_POSITION:
	case EN_PLUGIN_FUNC_TRADE_MODIFY_BALANCE:
	case EN_PLUGIN_FUNC_TRADE_VERIFY_PASSWORD:
	case EN_PLUGIN_FUNC_TRADE_HOLIDAYS:
	case EN_PLUGIN_FUNC_TRADE_SYMBOL_SUSPENSIONS:
		return true;
	default:
		return false;
	}
}

bool IsActiveMtDeriveFuncId(std::int64_t p_llFuncId)
{
	return (p_llFuncId >= EN_PLUGIN_FUNC_DERIVE_TICK_APPEND &&
		p_llFuncId <= EN_PLUGIN_FUNC_DERIVE_STATUS) ||
		p_llFuncId == EN_PLUGIN_FUNC_DERIVE_M1_ARCHIVE_FETCH ||
		p_llFuncId == EN_PLUGIN_FUNC_DERIVE_M1_ARCHIVE_ACK;
}

bool IsPluginNotifyId(std::int64_t p_llNotifyId)
{
	return p_llNotifyId > PLUGIN_NOTIFY_BEGIN && p_llNotifyId <= PLUGIN_NOTIFY_END &&
		(p_llNotifyId % 10) != 0;
}

bool IsTransportControlNotifyId(std::int64_t p_llNotifyId)
{
	switch (p_llNotifyId)
	{
	case TRANSPORT_NOTIFY_CLIENT_ADDED:
	case TRANSPORT_NOTIFY_CLIENT_SUBSCRIBED:
	case TRANSPORT_NOTIFY_CLIENT_UNSUBSCRIBED:
	case TRANSPORT_NOTIFY_CLIENT_DELETED:
		return true;
	default:
		return false;
	}
}

bool IsPluginFuncAllowed(EN_PLUGIN_ID p_enPluginId, std::int64_t p_llFuncId)
{
	if (p_llFuncId > PLUGIN_FUNC_COMMON_BEGIN && p_llFuncId <= PLUGIN_FUNC_COMMON_END)
	{
		return p_enPluginId != EN_PLUGIN_ID_INVALID;
	}
	if (p_llFuncId > PLUGIN_FUNC_DIAGNOSTIC_BEGIN && p_llFuncId <= PLUGIN_FUNC_DIAGNOSTIC_END)
	{
		return p_enPluginId != EN_PLUGIN_ID_INVALID;
	}

	switch (p_enPluginId)
	{
	case EN_PLUGIN_ID_MT_GATEWAY_SERVICE:
		return p_llFuncId > PLUGIN_FUNC_MT_GATEWAY_SERVICE_BEGIN && p_llFuncId <= PLUGIN_FUNC_MT_GATEWAY_SERVICE_END;
	case EN_PLUGIN_ID_MT_QUOTE_SERVICE:
		return p_llFuncId > PLUGIN_FUNC_QUOTE_BEGIN && p_llFuncId <= PLUGIN_FUNC_QUOTE_END;
	case EN_PLUGIN_ID_MT_TRADE_SERVICE:
		return p_llFuncId > PLUGIN_FUNC_TRADE_BEGIN && p_llFuncId <= PLUGIN_FUNC_TRADE_END &&
			(p_llFuncId % 10) != 0;
	case EN_PLUGIN_ID_MT_QUERY_SERVICE:
		return p_llFuncId > PLUGIN_FUNC_QUERY_BEGIN && p_llFuncId <= PLUGIN_FUNC_QUERY_END &&
			(p_llFuncId % 10) != 0;
	case EN_PLUGIN_ID_MT_EVENT_SERVICE:
		return p_llFuncId > PLUGIN_FUNC_MT_EVENT_SERVICE_BEGIN && p_llFuncId <= PLUGIN_FUNC_MT_EVENT_SERVICE_END;
	case EN_PLUGIN_ID_MT_DERIVE_SERVICE:
		return p_llFuncId > PLUGIN_FUNC_MT_DERIVE_SERVICE_BEGIN && p_llFuncId <= PLUGIN_FUNC_MT_DERIVE_SERVICE_END;
	case EN_PLUGIN_ID_INVALID:
	default:
		return false;
	}
}

EN_PLUGIN_ID GetPluginIdByFuncId(std::int64_t p_llFuncId)
{
	if (p_llFuncId > PLUGIN_FUNC_MT_GATEWAY_SERVICE_BEGIN && p_llFuncId <= PLUGIN_FUNC_MT_GATEWAY_SERVICE_END)
	{
		return EN_PLUGIN_ID_MT_GATEWAY_SERVICE;
	}
	if (p_llFuncId > PLUGIN_FUNC_QUOTE_BEGIN && p_llFuncId <= PLUGIN_FUNC_QUOTE_END)
	{
		return EN_PLUGIN_ID_MT_QUOTE_SERVICE;
	}
	if (p_llFuncId > PLUGIN_FUNC_TRADE_BEGIN && p_llFuncId <= PLUGIN_FUNC_TRADE_END)
	{
		return EN_PLUGIN_ID_MT_TRADE_SERVICE;
	}
	if (p_llFuncId > PLUGIN_FUNC_QUERY_BEGIN && p_llFuncId <= PLUGIN_FUNC_QUERY_END)
	{
		return EN_PLUGIN_ID_MT_QUERY_SERVICE;
	}
	if (p_llFuncId > PLUGIN_FUNC_MT_EVENT_SERVICE_BEGIN && p_llFuncId <= PLUGIN_FUNC_MT_EVENT_SERVICE_END)
	{
		return EN_PLUGIN_ID_MT_EVENT_SERVICE;
	}
	if (p_llFuncId > PLUGIN_FUNC_MT_DERIVE_SERVICE_BEGIN && p_llFuncId <= PLUGIN_FUNC_MT_DERIVE_SERVICE_END)
	{
		return EN_PLUGIN_ID_MT_DERIVE_SERVICE;
	}
	return EN_PLUGIN_ID_INVALID;
}

const char* GetPluginName(EN_PLUGIN_ID p_enPluginId)
{
	switch (p_enPluginId)
	{
	case EN_PLUGIN_ID_MT_GATEWAY_SERVICE: return "MtGatewayService";
	case EN_PLUGIN_ID_MT_QUOTE_SERVICE: return "MtQuoteService";
	case EN_PLUGIN_ID_MT_TRADE_SERVICE: return "MtTradeService";
	case EN_PLUGIN_ID_MT_QUERY_SERVICE: return "MtQueryService";
	case EN_PLUGIN_ID_MT_EVENT_SERVICE: return "MtEventService";
	case EN_PLUGIN_ID_MT_DERIVE_SERVICE: return "MtDeriveService";
	case EN_PLUGIN_ID_INVALID:
	default: return "InvalidPlugin";
	}
}

const char* GetPluginFuncName(std::int64_t p_llFuncId)
{
	switch (p_llFuncId)
	{
	case EN_PLUGIN_FUNC_COMMON_HEALTH: return "COMMON_HEALTH";
	case EN_PLUGIN_FUNC_COMMON_VERSION: return "COMMON_VERSION";
	case EN_PLUGIN_FUNC_CLUSTER_STATE: return "CLUSTER_STATE";
	case EN_PLUGIN_FUNC_MT_GATEWAY_SERVICE_DEMO_RETIRED: return "MT_GATEWAY_SERVICE_DEMO_RETIRED";
	case EN_PLUGIN_FUNC_QUOTE_DEMO_RETIRED: return "QUOTE_DEMO_RETIRED";
	case EN_PLUGIN_FUNC_QUOTE_SNAPSHOT: return "QUOTE_SNAPSHOT";
	case EN_PLUGIN_FUNC_QUOTE_TIME_SNAPSHOT: return "QUOTE_TIME_SNAPSHOT";
	case EN_PLUGIN_FUNC_QUOTE_HEARTBEAT: return "QUOTE_HEARTBEAT";
	case EN_PLUGIN_FUNC_TRADE_DEMO_RETIRED: return "TRADE_DEMO_RETIRED";
	case EN_PLUGIN_FUNC_TRADE_PLACE_ORDER: return "TRADE_PLACE_ORDER";
	case EN_PLUGIN_FUNC_TRADE_PLACE_ORDER_DETAIL: return "TRADE_PLACE_ORDER_DETAIL";
	case EN_PLUGIN_FUNC_TRADE_UPDATE_ORDER: return "TRADE_UPDATE_ORDER";
	case EN_PLUGIN_FUNC_TRADE_CANCEL_ORDER: return "TRADE_CANCEL_ORDER";
	case EN_PLUGIN_FUNC_TRADE_CREATE_ACCOUNT: return "TRADE_CREATE_ACCOUNT";
	case EN_PLUGIN_FUNC_TRADE_CLOSE_POSITION: return "TRADE_CLOSE_POSITION";
	case EN_PLUGIN_FUNC_TRADE_BATCH_CLOSE_POSITION: return "TRADE_BATCH_CLOSE_POSITION";
	case EN_PLUGIN_FUNC_TRADE_MODIFY_POSITION: return "TRADE_MODIFY_POSITION";
	case EN_PLUGIN_FUNC_TRADE_MODIFY_BALANCE: return "TRADE_MODIFY_BALANCE";
	case EN_PLUGIN_FUNC_TRADE_VERIFY_PASSWORD: return "TRADE_VERIFY_PASSWORD";
	case EN_PLUGIN_FUNC_TRADE_HOLIDAYS: return "TRADE_HOLIDAYS";
	case EN_PLUGIN_FUNC_TRADE_SYMBOL_SUSPENSIONS: return "TRADE_SYMBOL_SUSPENSIONS";
	case EN_PLUGIN_FUNC_QUERY_DEMO_RETIRED: return "QUERY_DEMO_RETIRED";
	case EN_PLUGIN_FUNC_QUERY_SERVER_INFO: return "QUERY_SERVER_INFO";
	case EN_PLUGIN_FUNC_QUERY_SYMBOLS: return "QUERY_SYMBOLS";
	case EN_PLUGIN_FUNC_QUERY_SYMBOL_RATES: return "QUERY_SYMBOL_RATES";
	case EN_PLUGIN_FUNC_QUERY_VOLUME_RANK: return "QUERY_VOLUME_RANK";
	case EN_PLUGIN_FUNC_QUERY_QUOTES: return "QUERY_QUOTES";
	case EN_PLUGIN_FUNC_QUERY_BARS: return "QUERY_BARS";
	case EN_PLUGIN_FUNC_QUERY_ACCOUNTS: return "QUERY_ACCOUNTS";
	case EN_PLUGIN_FUNC_QUERY_ACCOUNT_SYMBOLS: return "QUERY_ACCOUNT_SYMBOLS";
	case EN_PLUGIN_FUNC_QUERY_ACCOUNT_SYMBOL_TRADING: return "QUERY_ACCOUNT_SYMBOL_TRADING";
	case EN_PLUGIN_FUNC_QUERY_ORDERS: return "QUERY_ORDERS";
	case EN_PLUGIN_FUNC_QUERY_POSITIONS: return "QUERY_POSITIONS";
	case EN_PLUGIN_FUNC_QUERY_DEALS: return "QUERY_DEALS";
	case EN_PLUGIN_FUNC_QUERY_WATCHLIST: return "QUERY_WATCHLIST";
	case EN_PLUGIN_FUNC_QUERY_CHART: return "QUERY_CHART";
	case EN_PLUGIN_FUNC_QUERY_DERIVE_STATE_SNAPSHOT: return "QUERY_DERIVE_STATE_SNAPSHOT";
	case EN_PLUGIN_FUNC_QUERY_M1_BACKFILL: return "QUERY_M1_BACKFILL";
	case EN_PLUGIN_FUNC_QUERY_MT5_PROFIT_GROUP_QUOTE: return "QUERY_MT5_PROFIT_GROUP_QUOTE";
	case EN_PLUGIN_FUNC_MT_EVENT_SERVICE_DEMO_RETIRED: return "MT_EVENT_SERVICE_DEMO_RETIRED";
	case EN_PLUGIN_FUNC_RELIABLE_EVENT_APPEND: return "RELIABLE_EVENT_APPEND";
	case EN_PLUGIN_FUNC_RELIABLE_EVENT_FETCH: return "RELIABLE_EVENT_FETCH";
	case EN_PLUGIN_FUNC_RELIABLE_EVENT_ACK: return "RELIABLE_EVENT_ACK";
	case EN_PLUGIN_FUNC_RELIABLE_EVENT_STATUS: return "RELIABLE_EVENT_STATUS";
	case EN_PLUGIN_FUNC_EVENT_HEARTBEAT: return "EVENT_HEARTBEAT";
	case EN_PLUGIN_FUNC_MT_DERIVE_SERVICE_DEMO_RETIRED: return "MT_DERIVE_SERVICE_DEMO_RETIRED";
	case EN_PLUGIN_FUNC_DERIVE_TICK_APPEND: return "DERIVE_TICK_APPEND";
	case EN_PLUGIN_FUNC_DERIVE_PROFIT_DEMAND: return "DERIVE_PROFIT_DEMAND";
	case EN_PLUGIN_FUNC_DERIVE_PROFIT_SNAPSHOT: return "DERIVE_PROFIT_SNAPSHOT";
	case EN_PLUGIN_FUNC_DERIVE_M1_SNAPSHOT: return "DERIVE_M1_SNAPSHOT";
	case EN_PLUGIN_FUNC_DERIVE_STATUS: return "DERIVE_STATUS";
	case EN_PLUGIN_FUNC_DERIVE_M1_ARCHIVE_FETCH: return "DERIVE_M1_ARCHIVE_FETCH";
	case EN_PLUGIN_FUNC_DERIVE_M1_ARCHIVE_ACK: return "DERIVE_M1_ARCHIVE_ACK";
	case EN_PLUGIN_FUNC_DIAGNOSTIC_ECHO: return "DIAGNOSTIC_ECHO";
	default: return IsPluginFuncId(p_llFuncId) ? "PLUGIN_FUNC_RESERVED" : "PLUGIN_FUNC_INVALID";
	}
}

const char* GetPluginNotifyName(std::int64_t p_llNotifyId)
{
	switch (p_llNotifyId)
	{
	case EN_PLUGIN_NOTIFY_SERVICE_STATE: return "SERVICE_STATE";
	case EN_PLUGIN_NOTIFY_MARKET_TICK: return "MARKET_TICK";
	case EN_PLUGIN_NOTIFY_ORDER_CHANGED: return "ORDER_CHANGED";
	case EN_PLUGIN_NOTIFY_DEAL_CHANGED: return "DEAL_CHANGED";
	case EN_PLUGIN_NOTIFY_POSITION_CHANGED: return "POSITION_CHANGED";
	case EN_PLUGIN_NOTIFY_MARGIN_CHANGED: return "MARGIN_CHANGED";
	case EN_PLUGIN_NOTIFY_TRADE_SOURCE_STATE: return "TRADE_SOURCE_STATE";
	case EN_PLUGIN_NOTIFY_SYMBOL_CHANGED: return "SYMBOL_CHANGED";
	case EN_PLUGIN_NOTIFY_USER_CHANGED: return "USER_CHANGED";
	case EN_PLUGIN_NOTIFY_GROUP_CHANGED: return "GROUP_CHANGED";
	case EN_PLUGIN_NOTIFY_SERVER_TIME_CHANGED: return "SERVER_TIME_CHANGED";
	case EN_PLUGIN_NOTIFY_HOLIDAY_CHANGED: return "HOLIDAY_CHANGED";
	case EN_PLUGIN_NOTIFY_SYMBOL_SUSPENSION_CHANGED: return "SYMBOL_SUSPENSION_CHANGED";
	case EN_PLUGIN_NOTIFY_PROFIT_CHANGED: return "PROFIT_CHANGED";
	case EN_PLUGIN_NOTIFY_KLINE_CHANGED: return "KLINE_CHANGED";
	case EN_PLUGIN_NOTIFY_DIAGNOSTIC_TEST: return "DIAGNOSTIC_TEST";
	default: return IsPluginNotifyId(p_llNotifyId) ? "PLUGIN_NOTIFY_RESERVED" : "PLUGIN_NOTIFY_INVALID";
	}
}

bool EncodePluginNotify(const ST_PLUGIN_NOTIFY_META& p_refMeta, const unsigned char* p_pPayload,
	std::size_t p_szPayloadLen, std::vector<unsigned char>& p_refBuffer, std::string& p_refError)
{
	p_refBuffer.clear();
	p_refError.clear();
	if (p_szPayloadLen > 0 && p_pPayload == nullptr)
	{
		p_refError = "PROTOCOL_INVALID_PAYLOAD: payload pointer is null while length is non-zero";
		return false;
	}
	if (p_szPayloadLen > (std::numeric_limits<std::uint32_t>::max)())
	{
		p_refError = "PROTOCOL_PAYLOAD_TOO_LARGE: payload length exceeds uint32 range";
		return false;
	}
	if (p_refMeta.usVersion != TRADING_TERMINAL_PROTOCOL_VERSION)
	{
		p_refError = "PROTOCOL_VERSION_UNSUPPORTED: notification metadata version is not supported";
		return false;
	}
	if (p_refMeta.usSourcePlugin < EN_PLUGIN_ID_MT_GATEWAY_SERVICE || p_refMeta.usSourcePlugin > EN_PLUGIN_ID_MT_DERIVE_SERVICE)
	{
		p_refError = "PROTOCOL_SOURCE_INVALID: source plugin identifier is invalid";
		return false;
	}
	if (p_refMeta.usNotifyMode < EN_PLUGIN_NOTIFY_MODE_BEST_EFFORT ||
		p_refMeta.usNotifyMode > EN_PLUGIN_NOTIFY_MODE_CONTROL)
	{
		p_refError = "PROTOCOL_NOTIFY_MODE_INVALID: notification mode is outside the registered range";
		return false;
	}
	if (p_refMeta.usNotifyAction < EN_PLUGIN_NOTIFY_ACTION_CREATED ||
		p_refMeta.usNotifyAction > EN_PLUGIN_NOTIFY_ACTION_RESET)
	{
		p_refError = "PROTOCOL_NOTIFY_ACTION_INVALID: notification action is outside the registered range";
		return false;
	}

	p_refBuffer.reserve(TRADING_TERMINAL_NOTIFY_META_SIZE + p_szPayloadLen);
	WriteLittleEndian<std::uint16_t>(p_refBuffer, p_refMeta.usVersion);
	WriteLittleEndian<std::uint16_t>(p_refBuffer, static_cast<std::uint16_t>(TRADING_TERMINAL_NOTIFY_META_SIZE));
	WriteLittleEndian<std::uint16_t>(p_refBuffer, p_refMeta.usSourcePlugin);
	WriteLittleEndian<std::uint16_t>(p_refBuffer, p_refMeta.usNotifyMode);
	WriteLittleEndian<std::uint16_t>(p_refBuffer, p_refMeta.usNotifyAction);
	WriteLittleEndian<std::uint16_t>(p_refBuffer, 0);
	WriteLittleEndian<std::uint32_t>(p_refBuffer, static_cast<std::uint32_t>(p_szPayloadLen));
	WriteLittleEndian<std::uint64_t>(p_refBuffer, p_refMeta.ullSequence);
	WriteLittleEndian<std::uint64_t>(p_refBuffer, static_cast<std::uint64_t>(p_refMeta.llTimestampMs));
	if (p_szPayloadLen > 0)
	{
		p_refBuffer.insert(p_refBuffer.end(), p_pPayload, p_pPayload + p_szPayloadLen);
	}
	return true;
}

bool DecodePluginNotify(const unsigned char* p_pBuffer, std::size_t p_szBufferLen,
	ST_PLUGIN_NOTIFY_META& p_refMeta, const unsigned char*& p_refPayload, std::size_t& p_refPayloadLen,
	std::string& p_refError)
{
	p_refPayload = nullptr;
	p_refPayloadLen = 0;
	p_refError.clear();
	if (p_pBuffer == nullptr || p_szBufferLen < TRADING_TERMINAL_NOTIFY_META_SIZE)
	{
		p_refError = "PROTOCOL_HEADER_TRUNCATED: notification buffer is shorter than metadata header";
		return false;
	}

	p_refMeta.usVersion = ReadLittleEndian<std::uint16_t>(p_pBuffer);
	p_refMeta.usHeaderSize = ReadLittleEndian<std::uint16_t>(p_pBuffer + 2);
	p_refMeta.usSourcePlugin = ReadLittleEndian<std::uint16_t>(p_pBuffer + 4);
	p_refMeta.usNotifyMode = ReadLittleEndian<std::uint16_t>(p_pBuffer + 6);
	p_refMeta.usNotifyAction = ReadLittleEndian<std::uint16_t>(p_pBuffer + 8);
	p_refMeta.usReserved = ReadLittleEndian<std::uint16_t>(p_pBuffer + 10);
	p_refMeta.uiPayloadLen = ReadLittleEndian<std::uint32_t>(p_pBuffer + 12);
	p_refMeta.ullSequence = ReadLittleEndian<std::uint64_t>(p_pBuffer + 16);
	p_refMeta.llTimestampMs = static_cast<std::int64_t>(ReadLittleEndian<std::uint64_t>(p_pBuffer + 24));

	if (p_refMeta.usVersion != TRADING_TERMINAL_PROTOCOL_VERSION ||
		p_refMeta.usHeaderSize != TRADING_TERMINAL_NOTIFY_META_SIZE)
	{
		p_refError = "PROTOCOL_HEADER_INVALID: notification version or header size is invalid";
		return false;
	}
	if (p_refMeta.usReserved != 0)
	{
		p_refError = "PROTOCOL_RESERVED_FIELD_INVALID: notification reserved field must be zero";
		return false;
	}
	if (p_refMeta.usSourcePlugin < EN_PLUGIN_ID_MT_GATEWAY_SERVICE ||
		p_refMeta.usSourcePlugin > EN_PLUGIN_ID_MT_DERIVE_SERVICE)
	{
		p_refError = "PROTOCOL_SOURCE_INVALID: source plugin identifier is invalid";
		return false;
	}
	if (p_refMeta.usNotifyMode < EN_PLUGIN_NOTIFY_MODE_BEST_EFFORT ||
		p_refMeta.usNotifyMode > EN_PLUGIN_NOTIFY_MODE_CONTROL)
	{
		p_refError = "PROTOCOL_NOTIFY_MODE_INVALID: notification mode is outside the registered range";
		return false;
	}
	if (p_refMeta.usNotifyAction < EN_PLUGIN_NOTIFY_ACTION_CREATED ||
		p_refMeta.usNotifyAction > EN_PLUGIN_NOTIFY_ACTION_RESET)
	{
		p_refError = "PROTOCOL_NOTIFY_ACTION_INVALID: notification action is outside the registered range";
		return false;
	}
	const std::size_t szExpectedLen = TRADING_TERMINAL_NOTIFY_META_SIZE + static_cast<std::size_t>(p_refMeta.uiPayloadLen);
	if (szExpectedLen != p_szBufferLen)
	{
		p_refError = "PROTOCOL_LENGTH_MISMATCH: notification payload length does not match buffer size";
		return false;
	}
	p_refPayload = p_pBuffer + TRADING_TERMINAL_NOTIFY_META_SIZE;
	p_refPayloadLen = p_refMeta.uiPayloadLen;
	return true;
}

bool ValidateMtTimeState(const ST_MT_TIME_STATE& p_refState,
	std::string& p_refError)
{
	p_refError.clear();
	if (p_refState.usVersion != MT_TIME_PROTOCOL_VERSION)
	{
		p_refError = "MT_TIME_VERSION_UNSUPPORTED: time protocol version must be 1";
		return false;
	}
	if ((p_refState.usPlatformVersion != 4 && p_refState.usPlatformVersion != 5) ||
		p_refState.iSourceNo <= 0)
	{
		p_refError = "MT_TIME_SOURCE_INVALID: platform version must be 4/5 and source No must be positive";
		return false;
	}
	if (p_refState.strTimeZoneId.empty() ||
		p_refState.strTimeZoneId.size() > MT_TIME_MAX_TIME_ZONE_ID_BYTES ||
		!IsValidUtf8Text(p_refState.strTimeZoneId))
	{
		p_refError = "MT_TIME_ZONE_INVALID: Windows time zone id must be valid UTF-8 between 1 and 128 bytes";
		return false;
	}
	static const std::int32_t iMaximumOffset = 24 * 60 * 60;
	if (p_refState.iStandardOffsetSeconds < -iMaximumOffset ||
		p_refState.iStandardOffsetSeconds > iMaximumOffset ||
		p_refState.iCurrentOffsetSeconds < -iMaximumOffset ||
		p_refState.iCurrentOffsetSeconds > iMaximumOffset ||
		p_refState.iStandardOffsetSeconds % 60 != 0 ||
		p_refState.iCurrentOffsetSeconds % 60 != 0)
	{
		p_refError = "MT_TIME_OFFSET_INVALID: standard and current offsets must be minute-aligned within plus or minus 24 hours";
		return false;
	}
	if (p_refState.enSyncState < EN_MT_TIME_SYNC_SYNCING ||
		p_refState.enSyncState > EN_MT_TIME_SYNC_MISMATCH ||
		p_refState.enChangeReason < EN_MT_TIME_CHANGE_INITIAL ||
		p_refState.enChangeReason > EN_MT_TIME_CHANGE_RECONNECT)
	{
		p_refError = "MT_TIME_STATE_INVALID: sync state or change reason is outside the published range";
		return false;
	}
	if (p_refState.ullAuthorityEpoch == 0 || p_refState.ullGeneration == 0 ||
		p_refState.llEffectiveUtcMs <= 0 || p_refState.llSampledUtcMs <= 0 ||
		p_refState.llValidUntilUtcMs <= p_refState.llSampledUtcMs ||
		p_refState.llEffectiveUtcMs > p_refState.llSampledUtcMs)
	{
		p_refError = "MT_TIME_GENERATION_INVALID: epoch, generation and UTC validity timestamps are inconsistent";
		return false;
	}
	return true;
}

bool EncodeMtTimeState(const ST_MT_TIME_STATE& p_refState,
	std::vector<unsigned char>& p_refBuffer, std::string& p_refError)
{
	p_refBuffer.clear();
	if (!ValidateMtTimeState(p_refState, p_refError))
	{
		return false;
	}
	p_refBuffer.reserve(MT_TIME_STATE_HEADER_SIZE + p_refState.strTimeZoneId.size());
	WriteLittleEndian<std::uint32_t>(p_refBuffer, MT_TIME_STATE_MAGIC);
	WriteLittleEndian<std::uint16_t>(p_refBuffer, p_refState.usVersion);
	WriteLittleEndian<std::uint16_t>(p_refBuffer,
		static_cast<std::uint16_t>(MT_TIME_STATE_HEADER_SIZE));
	WriteLittleEndian<std::uint16_t>(p_refBuffer, p_refState.usPlatformVersion);
	p_refBuffer.push_back(static_cast<unsigned char>(p_refState.enSyncState));
	p_refBuffer.push_back(p_refState.bDaylight ? 1U : 0U);
	WriteLittleEndian<std::uint16_t>(p_refBuffer,
		static_cast<std::uint16_t>(p_refState.enChangeReason));
	WriteLittleEndian<std::uint32_t>(p_refBuffer,
		static_cast<std::uint32_t>(p_refState.iSourceNo));
	WriteLittleEndian<std::uint32_t>(p_refBuffer,
		static_cast<std::uint32_t>(p_refState.iStandardOffsetSeconds));
	WriteLittleEndian<std::uint32_t>(p_refBuffer,
		static_cast<std::uint32_t>(p_refState.iCurrentOffsetSeconds));
	WriteLittleEndian<std::uint32_t>(p_refBuffer,
		static_cast<std::uint32_t>(p_refState.strTimeZoneId.size()));
	WriteLittleEndian<std::uint64_t>(p_refBuffer, p_refState.ullAuthorityEpoch);
	WriteLittleEndian<std::uint64_t>(p_refBuffer, p_refState.ullGeneration);
	WriteLittleEndian<std::uint64_t>(p_refBuffer,
		static_cast<std::uint64_t>(p_refState.llEffectiveUtcMs));
	WriteLittleEndian<std::uint64_t>(p_refBuffer,
		static_cast<std::uint64_t>(p_refState.llSampledUtcMs));
	WriteLittleEndian<std::uint64_t>(p_refBuffer,
		static_cast<std::uint64_t>(p_refState.llValidUntilUtcMs));
	p_refBuffer.insert(p_refBuffer.end(), p_refState.strTimeZoneId.begin(),
		p_refState.strTimeZoneId.end());
	return true;
}

bool DecodeMtTimeState(const unsigned char* p_pBuffer,
	std::size_t p_szBufferLen, ST_MT_TIME_STATE& p_refState,
	std::string& p_refError)
{
	p_refState = ST_MT_TIME_STATE();
	p_refError.clear();
	if (p_pBuffer == nullptr || p_szBufferLen < MT_TIME_STATE_HEADER_SIZE)
	{
		p_refError = "MT_TIME_STATE_TRUNCATED: payload is shorter than the fixed time state header";
		return false;
	}
	const std::uint32_t uiMagic = ReadLittleEndian<std::uint32_t>(p_pBuffer);
	const std::uint16_t usHeaderSize = ReadLittleEndian<std::uint16_t>(p_pBuffer + 6);
	const std::uint32_t uiSourceNo = ReadLittleEndian<std::uint32_t>(p_pBuffer + 14);
	const std::uint32_t uiTimeZoneLen = ReadLittleEndian<std::uint32_t>(p_pBuffer + 26);
	if (uiMagic != MT_TIME_STATE_MAGIC || usHeaderSize != MT_TIME_STATE_HEADER_SIZE)
	{
		p_refError = "MT_TIME_STATE_HEADER_INVALID: magic or header size is invalid";
		return false;
	}
	if (uiSourceNo > static_cast<std::uint32_t>((std::numeric_limits<std::int32_t>::max)()) ||
		uiTimeZoneLen == 0 || uiTimeZoneLen > MT_TIME_MAX_TIME_ZONE_ID_BYTES ||
		p_szBufferLen - MT_TIME_STATE_HEADER_SIZE != uiTimeZoneLen)
	{
		p_refError = "MT_TIME_STATE_LENGTH_INVALID: source No or time zone length does not match the payload";
		return false;
	}
	p_refState.usVersion = ReadLittleEndian<std::uint16_t>(p_pBuffer + 4);
	p_refState.usPlatformVersion = ReadLittleEndian<std::uint16_t>(p_pBuffer + 8);
	p_refState.enSyncState = static_cast<EN_MT_TIME_SYNC_STATE>(p_pBuffer[10]);
	if (p_pBuffer[11] > 1U)
	{
		p_refError = "MT_TIME_DAYLIGHT_INVALID: daylight flag must be 0 or 1";
		return false;
	}
	p_refState.bDaylight = p_pBuffer[11] == 1U;
	p_refState.enChangeReason = static_cast<EN_MT_TIME_CHANGE_REASON>(
		ReadLittleEndian<std::uint16_t>(p_pBuffer + 12));
	p_refState.iSourceNo = static_cast<std::int32_t>(uiSourceNo);
	p_refState.iStandardOffsetSeconds = static_cast<std::int32_t>(
		ReadLittleEndian<std::uint32_t>(p_pBuffer + 18));
	p_refState.iCurrentOffsetSeconds = static_cast<std::int32_t>(
		ReadLittleEndian<std::uint32_t>(p_pBuffer + 22));
	p_refState.ullAuthorityEpoch = ReadLittleEndian<std::uint64_t>(p_pBuffer + 30);
	p_refState.ullGeneration = ReadLittleEndian<std::uint64_t>(p_pBuffer + 38);
	p_refState.llEffectiveUtcMs = static_cast<std::int64_t>(
		ReadLittleEndian<std::uint64_t>(p_pBuffer + 46));
	p_refState.llSampledUtcMs = static_cast<std::int64_t>(
		ReadLittleEndian<std::uint64_t>(p_pBuffer + 54));
	p_refState.llValidUntilUtcMs = static_cast<std::int64_t>(
		ReadLittleEndian<std::uint64_t>(p_pBuffer + 62));
	p_refState.strTimeZoneId.assign(
		reinterpret_cast<const char*>(p_pBuffer + MT_TIME_STATE_HEADER_SIZE),
		static_cast<std::size_t>(uiTimeZoneLen));
	return ValidateMtTimeState(p_refState, p_refError);
}

bool EncodeMtTimeSnapshotRequest(const ST_MT_TIME_SNAPSHOT_REQUEST& p_refRequest,
	std::vector<unsigned char>& p_refBuffer, std::string& p_refError)
{
	p_refBuffer.clear();
	p_refError.clear();
	if (p_refRequest.usVersion != MT_TIME_PROTOCOL_VERSION ||
		!((p_refRequest.usPlatformVersion == 0 && p_refRequest.iSourceNo == 0) ||
		((p_refRequest.usPlatformVersion == 4 || p_refRequest.usPlatformVersion == 5) &&
		p_refRequest.iSourceNo > 0)))
	{
		p_refError = "MT_TIME_SNAPSHOT_REQUEST_INVALID: use Version=0,No=0 for all nodes or Version=4/5 with positive No";
		return false;
	}
	p_refBuffer.reserve(MT_TIME_REQUEST_SIZE);
	WriteLittleEndian<std::uint32_t>(p_refBuffer, MT_TIME_REQUEST_MAGIC);
	WriteLittleEndian<std::uint16_t>(p_refBuffer, p_refRequest.usVersion);
	WriteLittleEndian<std::uint16_t>(p_refBuffer,
		static_cast<std::uint16_t>(MT_TIME_REQUEST_SIZE));
	WriteLittleEndian<std::uint16_t>(p_refBuffer, p_refRequest.usPlatformVersion);
	WriteLittleEndian<std::uint16_t>(p_refBuffer, 0);
	WriteLittleEndian<std::uint32_t>(p_refBuffer,
		static_cast<std::uint32_t>(p_refRequest.iSourceNo));
	return true;
}

bool DecodeMtTimeSnapshotRequest(const unsigned char* p_pBuffer,
	std::size_t p_szBufferLen, ST_MT_TIME_SNAPSHOT_REQUEST& p_refRequest,
	std::string& p_refError)
{
	p_refRequest = ST_MT_TIME_SNAPSHOT_REQUEST();
	p_refError.clear();
	if (p_pBuffer == nullptr || p_szBufferLen != MT_TIME_REQUEST_SIZE)
	{
		p_refError = "MT_TIME_SNAPSHOT_REQUEST_LENGTH_INVALID: request must be exactly 16 bytes";
		return false;
	}
	const std::uint32_t uiMagic = ReadLittleEndian<std::uint32_t>(p_pBuffer);
	const std::uint16_t usSize = ReadLittleEndian<std::uint16_t>(p_pBuffer + 6);
	const std::uint16_t usReserved = ReadLittleEndian<std::uint16_t>(p_pBuffer + 10);
	const std::uint32_t uiSourceNo = ReadLittleEndian<std::uint32_t>(p_pBuffer + 12);
	if (uiMagic != MT_TIME_REQUEST_MAGIC || usSize != MT_TIME_REQUEST_SIZE ||
		usReserved != 0 ||
		uiSourceNo > static_cast<std::uint32_t>((std::numeric_limits<std::int32_t>::max)()))
	{
		p_refError = "MT_TIME_SNAPSHOT_REQUEST_HEADER_INVALID: magic, size, reserved field or source No is invalid";
		return false;
	}
	p_refRequest.usVersion = ReadLittleEndian<std::uint16_t>(p_pBuffer + 4);
	p_refRequest.usPlatformVersion = ReadLittleEndian<std::uint16_t>(p_pBuffer + 8);
	p_refRequest.iSourceNo = static_cast<std::int32_t>(uiSourceNo);
	std::vector<unsigned char> aUnused;
	return EncodeMtTimeSnapshotRequest(p_refRequest, aUnused, p_refError);
}

bool EncodeMtTimeSnapshotResponse(const ST_MT_TIME_SNAPSHOT_RESPONSE& p_refResponse,
	std::vector<unsigned char>& p_refBuffer, std::string& p_refError)
{
	p_refBuffer.clear();
	p_refError.clear();
	if (p_refResponse.usVersion != MT_TIME_PROTOCOL_VERSION ||
		p_refResponse.aState.size() > MT_TIME_MAX_STATE_COUNT)
	{
		p_refError = "MT_TIME_SNAPSHOT_RESPONSE_INVALID: version or state count is invalid";
		return false;
	}
	WriteLittleEndian<std::uint32_t>(p_refBuffer, MT_TIME_RESPONSE_MAGIC);
	WriteLittleEndian<std::uint16_t>(p_refBuffer, p_refResponse.usVersion);
	WriteLittleEndian<std::uint16_t>(p_refBuffer,
		static_cast<std::uint16_t>(MT_TIME_RESPONSE_HEADER_SIZE));
	WriteLittleEndian<std::uint32_t>(p_refBuffer,
		static_cast<std::uint32_t>(p_refResponse.aState.size()));
	for (const ST_MT_TIME_STATE& refState : p_refResponse.aState)
	{
		std::vector<unsigned char> aState;
		if (!EncodeMtTimeState(refState, aState, p_refError))
		{
			return false;
		}
		WriteLittleEndian<std::uint32_t>(p_refBuffer,
			static_cast<std::uint32_t>(aState.size()));
		p_refBuffer.insert(p_refBuffer.end(), aState.begin(), aState.end());
	}
	return true;
}

bool DecodeMtTimeSnapshotResponse(const unsigned char* p_pBuffer,
	std::size_t p_szBufferLen, ST_MT_TIME_SNAPSHOT_RESPONSE& p_refResponse,
	std::string& p_refError)
{
	p_refResponse = ST_MT_TIME_SNAPSHOT_RESPONSE();
	p_refError.clear();
	if (p_pBuffer == nullptr || p_szBufferLen < MT_TIME_RESPONSE_HEADER_SIZE)
	{
		p_refError = "MT_TIME_SNAPSHOT_RESPONSE_TRUNCATED: response is shorter than its header";
		return false;
	}
	const std::uint32_t uiMagic = ReadLittleEndian<std::uint32_t>(p_pBuffer);
	const std::uint16_t usVersion = ReadLittleEndian<std::uint16_t>(p_pBuffer + 4);
	const std::uint16_t usHeaderSize = ReadLittleEndian<std::uint16_t>(p_pBuffer + 6);
	const std::uint32_t uiCount = ReadLittleEndian<std::uint32_t>(p_pBuffer + 8);
	if (uiMagic != MT_TIME_RESPONSE_MAGIC || usVersion != MT_TIME_PROTOCOL_VERSION ||
		usHeaderSize != MT_TIME_RESPONSE_HEADER_SIZE || uiCount > MT_TIME_MAX_STATE_COUNT)
	{
		p_refError = "MT_TIME_SNAPSHOT_RESPONSE_HEADER_INVALID: magic, version, header size or count is invalid";
		return false;
	}
	std::size_t szOffset = MT_TIME_RESPONSE_HEADER_SIZE;
	for (std::uint32_t uiIndex = 0; uiIndex < uiCount; ++uiIndex)
	{
		if (szOffset > p_szBufferLen || p_szBufferLen - szOffset < sizeof(std::uint32_t))
		{
			p_refError = "MT_TIME_SNAPSHOT_ITEM_TRUNCATED: item length is missing";
			return false;
		}
		const std::uint32_t uiLength = ReadLittleEndian<std::uint32_t>(p_pBuffer + szOffset);
		szOffset += sizeof(std::uint32_t);
		if (uiLength < MT_TIME_STATE_HEADER_SIZE || szOffset > p_szBufferLen ||
			p_szBufferLen - szOffset < uiLength)
		{
			p_refError = "MT_TIME_SNAPSHOT_ITEM_LENGTH_INVALID: item exceeds the remaining response";
			return false;
		}
		ST_MT_TIME_STATE stState;
		if (!DecodeMtTimeState(p_pBuffer + szOffset, uiLength, stState, p_refError))
		{
			return false;
		}
		p_refResponse.aState.push_back(stState);
		szOffset += uiLength;
	}
	if (szOffset != p_szBufferLen)
	{
		p_refError = "MT_TIME_SNAPSHOT_RESPONSE_TRAILING_DATA: response contains unconsumed bytes";
		return false;
	}
	return true;
}
