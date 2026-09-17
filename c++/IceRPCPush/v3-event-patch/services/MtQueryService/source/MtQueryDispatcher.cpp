#include "MtQueryDispatcher.h"

#include "CodeMsg.h"
#include "Log.h"
#include "TradingTerminalProtocol.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <vector>

namespace
{
	bool ResolveBrokerDateMinute(std::int64_t p_llServerMinute,
		std::uint32_t& p_refDate, std::uint32_t& p_refMinuteIndex)
	{
		if (p_llServerMinute <= 0 || p_llServerMinute % 60LL != 0)
		{
			return false;
		}
		const __time64_t llRaw = static_cast<__time64_t>(
			p_llServerMinute);
		tm stServer = {};
		if (_gmtime64_s(&stServer, &llRaw) != 0)
		{
			return false;
		}
		p_refDate = static_cast<std::uint32_t>(
			(stServer.tm_year + 1900) * 10000 +
			(stServer.tm_mon + 1) * 100 + stServer.tm_mday);
		p_refMinuteIndex = static_cast<std::uint32_t>(
			stServer.tm_hour * 60 + stServer.tm_min);
		return p_refMinuteIndex < 1440U;
	}

	bool BuildBrokerDayServerRange(std::uint32_t p_uiServerDate,
		std::int64_t& p_refServerFrom,
		std::int64_t& p_refServerNextDay)
	{
		tm stDate = {};
		stDate.tm_year = static_cast<int>(p_uiServerDate / 10000U) - 1900;
		stDate.tm_mon = static_cast<int>((p_uiServerDate / 100U) % 100U) - 1;
		stDate.tm_mday = static_cast<int>(p_uiServerDate % 100U);
		const __time64_t llFrom = _mkgmtime64(&stDate);
		if (llFrom <= 0)
		{
			return false;
		}
		tm stVerify = {};
		if (_gmtime64_s(&stVerify, &llFrom) != 0 ||
			static_cast<std::uint32_t>((stVerify.tm_year + 1900) * 10000 +
				(stVerify.tm_mon + 1) * 100 + stVerify.tm_mday) !=
				p_uiServerDate)
		{
			return false;
		}
		p_refServerFrom = static_cast<std::int64_t>(llFrom);
		p_refServerNextDay = p_refServerFrom + 86400LL;
		return true;
	}

	// /bars 内部回填状态。该枚举只描述 V3 服务内部竞态，不作为新的外部协议字段发送。
	enum EN_MT_QUERY_BAR_BACKFILL_RESULT : std::uint16_t
	{
		EN_MT_QUERY_BAR_BACKFILL_NOT_NEEDED = 0,
		EN_MT_QUERY_BAR_BACKFILL_APPLIED = 1,
		EN_MT_QUERY_BAR_BACKFILL_IN_PROGRESS = 2,
		EN_MT_QUERY_BAR_BACKFILL_SOURCE_STALE = 3,
		EN_MT_QUERY_BAR_BACKFILL_GENERATION_CHANGED = 4,
		EN_MT_QUERY_BAR_BACKFILL_MT_TRANSIENT_ERROR = 5,
		EN_MT_QUERY_BAR_BACKFILL_STORAGE_ERROR = 6
	};

	const char* GetBarBackfillResultName(
		EN_MT_QUERY_BAR_BACKFILL_RESULT p_enResult)
	{
		switch (p_enResult)
		{
		case EN_MT_QUERY_BAR_BACKFILL_NOT_NEEDED:
			return "NOT_NEEDED";
		case EN_MT_QUERY_BAR_BACKFILL_APPLIED:
			return "APPLIED";
		case EN_MT_QUERY_BAR_BACKFILL_IN_PROGRESS:
			return "IN_PROGRESS";
		case EN_MT_QUERY_BAR_BACKFILL_SOURCE_STALE:
			return "SOURCE_STALE";
		case EN_MT_QUERY_BAR_BACKFILL_GENERATION_CHANGED:
			return "GENERATION_CHANGED";
		case EN_MT_QUERY_BAR_BACKFILL_MT_TRANSIENT_ERROR:
			return "MT_TRANSIENT_ERROR";
		case EN_MT_QUERY_BAR_BACKFILL_STORAGE_ERROR:
			return "STORAGE_ERROR";
		default:
			return "UNKNOWN";
		}
	}

	// 区分本地对象/文件损坏与可重试的 MT 状态；详细字符串只用于内部分类和日志，不暴露凭据。
	bool IsBarStorageFailureDetail(const std::string& p_refDetail)
	{
		static const char* s_aMarker[] =
		{
			"QUERY_BAR_STORE", "QUERY_BAR_COVERAGE",
			"QUERY_BARS_RESULT_INVALID", "QUERY_RESPONSE_MEMORY",
			"QUERY_MOCK_MEMORY"
		};
		for (const char* pMarker : s_aMarker)
		{
			if (p_refDetail.find(pMarker) != std::string::npos)
			{
				return true;
			}
		}
		return false;
	}

	// 轻量作用域清理只用于释放回填租约；析构不抛异常，覆盖所有提前返回路径。
	class CQueryScopeExit
	{
	public:
		explicit CQueryScopeExit(const std::function<void()>& p_refAction)
			: m_fnAction(p_refAction)
		{
		}

		~CQueryScopeExit()
		{
			if (m_fnAction)
			{
				m_fnAction();
			}
		}

	private:
		CQueryScopeExit(const CQueryScopeExit&) = delete;
		CQueryScopeExit& operator=(const CQueryScopeExit&) = delete;
		std::function<void()> m_fnAction; // 析构时执行一次的无异常清理动作。
	};

	// 创建值节点并统一返回内存错误，避免处理器解引用空 shared_ptr。
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE> MakeValue(
		EN_PLUGIN_BINARY_VALUE_TYPE p_enType,
		std::string& p_refError)
	{
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refValue =
			CreatePluginBinaryValue(p_enType);
		if (!refValue)
		{
			p_refError =
				"QUERY_RESPONSE_MEMORY_ERROR: failed to allocate Binary value";
		}
		return refValue;
	}

	// 向对象写入已经创建的值节点；字段名固定由服务端定义。
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
				"QUERY_RESPONSE_FIELD_INVALID: object, field name or value is invalid";
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
			MakeValue(EN_PLUGIN_BINARY_VALUE_INT64, p_refError);
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
			MakeValue(EN_PLUGIN_BINARY_VALUE_UINT64, p_refError);
		if (!refValue)
		{
			return false;
		}
		refValue->ullUIntValue = p_ullValue;
		return AddField(p_refObject, p_szName,
			refValue, p_refError);
	}

	bool AddDouble(ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName, double p_dValue,
		std::string& p_refError)
	{
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refValue =
			MakeValue(EN_PLUGIN_BINARY_VALUE_DOUBLE, p_refError);
		if (!refValue)
		{
			return false;
		}
		refValue->dDoubleValue = p_dValue;
		return AddField(p_refObject, p_szName,
			refValue, p_refError);
	}

	bool AddBool(ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName, bool p_bValue,
		std::string& p_refError)
	{
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refValue =
			MakeValue(EN_PLUGIN_BINARY_VALUE_BOOL, p_refError);
		if (!refValue)
		{
			return false;
		}
		refValue->ucBoolValue = p_bValue ? 1U : 0U;
		return AddField(p_refObject, p_szName,
			refValue, p_refError);
	}

	bool AddString(ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName, const std::string& p_refValueText,
		std::string& p_refError)
	{
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refValue =
			MakeValue(EN_PLUGIN_BINARY_VALUE_STRING, p_refError);
		if (!refValue)
		{
			return false;
		}
		refValue->strStringValue = p_refValueText;
		return AddField(p_refObject, p_szName,
			refValue, p_refError);
	}

	// 读取可选有符号整数；缺失时使用默认值，类型或范围不匹配时返回详细错误。
	bool ReadInt64(const ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName, std::int64_t p_llDefault,
		std::int64_t& p_refValue, std::string& p_refError)
	{
		p_refValue = p_llDefault;
		const ST_PLUGIN_BINARY_VALUE* pValue =
			FindPluginBinaryField(p_refObject, p_szName);
		if (pValue == nullptr)
		{
			return true;
		}
		if (pValue->enType == EN_PLUGIN_BINARY_VALUE_INT64)
		{
			p_refValue = pValue->llIntValue;
			return true;
		}
		if (pValue->enType == EN_PLUGIN_BINARY_VALUE_UINT64 &&
			pValue->ullUIntValue <=
			static_cast<std::uint64_t>(
				(std::numeric_limits<std::int64_t>::max)()))
		{
			p_refValue =
				static_cast<std::int64_t>(pValue->ullUIntValue);
			return true;
		}
		p_refError = std::string(
			"QUERY_REQUEST_FIELD_TYPE_INVALID: field=") +
			p_szName + ", expected=int64";
		return false;
	}

	// 读取可选无符号整数；分页 Cursor 和 EntityMask 不允许负值或浮点隐式转换。
	bool ReadUInt64(const ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName, std::uint64_t p_ullDefault,
		std::uint64_t& p_refValue, std::string& p_refError)
	{
		p_refValue = p_ullDefault;
		const ST_PLUGIN_BINARY_VALUE* pValue =
			FindPluginBinaryField(p_refObject, p_szName);
		if (pValue == nullptr)
		{
			return true;
		}
		if (pValue->enType == EN_PLUGIN_BINARY_VALUE_UINT64)
		{
			p_refValue = pValue->ullUIntValue;
			return true;
		}
		if (pValue->enType == EN_PLUGIN_BINARY_VALUE_INT64 &&
			pValue->llIntValue >= 0)
		{
			p_refValue = static_cast<std::uint64_t>(
				pValue->llIntValue);
			return true;
		}
		p_refError = std::string(
			"QUERY_REQUEST_FIELD_TYPE_INVALID: field=") +
			p_szName + ", expected=uint64";
		return false;
	}

	// 读取可选字符串；缺失时返回空字符串。
	bool ReadString(const ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName, std::string& p_refValue,
		std::string& p_refError)
	{
		p_refValue.clear();
		const ST_PLUGIN_BINARY_VALUE* pValue =
			FindPluginBinaryField(p_refObject, p_szName);
		if (pValue == nullptr)
		{
			return true;
		}
		if (pValue->enType != EN_PLUGIN_BINARY_VALUE_STRING)
		{
			p_refError = std::string(
				"QUERY_REQUEST_FIELD_TYPE_INVALID: field=") +
				p_szName + ", expected=string";
			return false;
		}
		p_refValue = pValue->strStringValue;
		return true;
	}

	// 将 V2 逗号分隔 SymbolIds 解析为去重集合，空字符串表示不过滤。
	std::set<std::string> ParseSymbolIds(
		const std::string& p_refText)
	{
		std::set<std::string> setResult;
		std::stringstream clStream(p_refText);
		std::string strItem;
		while (std::getline(clStream, strItem, ','))
		{
			const std::string::size_type szBegin =
				strItem.find_first_not_of(" \t\r\n");
			const std::string::size_type szEnd =
				strItem.find_last_not_of(" \t\r\n");
			if (szBegin != std::string::npos)
			{
				setResult.insert(strItem.substr(szBegin,
					szEnd - szBegin + 1));
			}
		}
		return setResult;
	}

	// 读取 SDK Adapter 返回的数值字段；价格允许 DOUBLE，成交量兼容安全整数。
	bool ReadBarDouble(const ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName, double& p_refValue,
		std::string& p_refError)
	{
		const ST_PLUGIN_BINARY_VALUE* pValue =
			FindPluginBinaryField(p_refObject, p_szName);
		if (pValue == nullptr)
		{
			p_refError = std::string(
				"QUERY_BARS_ADAPTER_FIELD_MISSING: field=") + p_szName;
			return false;
		}
		if (pValue->enType == EN_PLUGIN_BINARY_VALUE_DOUBLE)
		{
			p_refValue = pValue->dDoubleValue;
			return std::isfinite(p_refValue);
		}
		if (pValue->enType == EN_PLUGIN_BINARY_VALUE_INT64)
		{
			p_refValue = static_cast<double>(pValue->llIntValue);
			return true;
		}
		if (pValue->enType == EN_PLUGIN_BINARY_VALUE_UINT64)
		{
			p_refValue = static_cast<double>(pValue->ullUIntValue);
			return true;
		}
		p_refError = std::string(
			"QUERY_BARS_ADAPTER_FIELD_TYPE_INVALID: field=") + p_szName;
		return false;
	}

	// 将 HISTORY_QUERY Adapter 的按品种数组解码为内部 K 线；任何字段不完整都拒绝整包。
	bool DecodeAdapterBars(const ST_PLUGIN_BINARY_VALUE& p_refData,
		std::map<std::string, std::vector<ST_MT_QUERY_BAR>>& p_refBars,
		std::string& p_refError)
	{
		p_refBars.clear();
		if (p_refData.enType != EN_PLUGIN_BINARY_VALUE_ARRAY)
		{
			p_refError =
				"QUERY_BARS_ADAPTER_ROOT_INVALID: expected array";
			return false;
		}
		for (const std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& refSymbol :
			p_refData.aArrayValue)
		{
			if (!refSymbol || refSymbol->enType !=
				EN_PLUGIN_BINARY_VALUE_OBJECT)
			{
				p_refError =
					"QUERY_BARS_ADAPTER_SYMBOL_INVALID: expected object";
				return false;
			}
			const ST_PLUGIN_BINARY_VALUE* pSymbol =
				FindPluginBinaryField(*refSymbol, "SymbolId");
			const ST_PLUGIN_BINARY_VALUE* pArray =
				FindPluginBinaryField(*refSymbol, "Bars");
			if (pSymbol == nullptr || pSymbol->enType !=
					EN_PLUGIN_BINARY_VALUE_STRING ||
				pSymbol->strStringValue.empty() || pArray == nullptr ||
				pArray->enType != EN_PLUGIN_BINARY_VALUE_ARRAY)
			{
				p_refError =
					"QUERY_BARS_ADAPTER_SYMBOL_FIELDS_INVALID: SymbolId or Bars is invalid";
				return false;
			}
			std::vector<ST_MT_QUERY_BAR>& refBars =
				p_refBars[pSymbol->strStringValue];
			for (const std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& refItem :
				pArray->aArrayValue)
			{
				if (!refItem || refItem->enType !=
					EN_PLUGIN_BINARY_VALUE_OBJECT)
				{
					p_refError =
						"QUERY_BARS_ADAPTER_ITEM_INVALID: expected object";
					return false;
				}
				std::int64_t llDateTime = 0;
				double dValue = 0.0;
				ST_MT_QUERY_BAR stBar;
				if (!ReadInt64(*refItem, "DateTime", 0,
						llDateTime, p_refError) || llDateTime <= 0 ||
					!ReadBarDouble(*refItem, "Open", stBar.dOpen,
						p_refError) ||
					!ReadBarDouble(*refItem, "High", stBar.dHigh,
						p_refError) ||
					!ReadBarDouble(*refItem, "Low", stBar.dLow,
						p_refError) ||
					!ReadBarDouble(*refItem, "Close", stBar.dClose,
						p_refError) ||
					!ReadBarDouble(*refItem, "Value", dValue,
						p_refError) || dValue < 0.0 ||
					stBar.dHigh < stBar.dLow)
				{
					return false;
				}
				stBar.llDateTime = llDateTime;
				stBar.ullTickVolume = dValue >= static_cast<double>(
						(std::numeric_limits<std::uint64_t>::max)()) ?
					(std::numeric_limits<std::uint64_t>::max)() :
					static_cast<std::uint64_t>(dValue);
				stBar.ullRealVolume = 0;
				stBar.ullSourceEpoch = 0;
				stBar.ullLastSequence = 0;
				refBars.push_back(stBar);
			}
			std::vector<ST_MT_QUERY_BAR> aNormalized;
			CMtQueryBarHistoryStore::MergeBars(aNormalized, refBars);
			refBars.swap(aNormalized);
		}
		return true;
	}

	// 为单品种缺口构造明确的范围请求；不复制原请求中的分页字段，避免 Adapter 重复解释场景。
	bool BuildBarRangeRequest(std::uint16_t p_usVersion,
		std::int32_t p_iNo, const std::string& p_refSymbol,
		std::int64_t p_llInterval, std::int64_t p_llFrom,
		std::int64_t p_llTo, ST_PLUGIN_BINARY_VALUE& p_refRequest,
		std::string& p_refError)
	{
		p_refRequest = ST_PLUGIN_BINARY_VALUE();
		p_refRequest.enType = EN_PLUGIN_BINARY_VALUE_OBJECT;
		return AddInt64(p_refRequest, "Version", p_usVersion,
				p_refError) &&
			AddInt64(p_refRequest, "No", p_iNo, p_refError) &&
			AddString(p_refRequest, "SymbolIds", p_refSymbol,
				p_refError) &&
			AddInt64(p_refRequest, "Interval", p_llInterval,
				p_refError) &&
			AddInt64(p_refRequest, "From", p_llFrom, p_refError) &&
			AddInt64(p_refRequest, "To", p_llTo, p_refError);
	}

	// 通过 HISTORY_QUERY 池查询单个 UTC0 缺口；业务失败保留在 Result，基础设施失败通过 Error 返回。
	bool ExecuteBarRange(CMtQueryNodeManager* p_pNodeManager,
		std::uint16_t p_usVersion, std::int32_t p_iNo,
		const std::string& p_refSymbol, std::int64_t p_llInterval,
		const ST_MT_QUERY_BAR_COVERAGE_RANGE& p_refRange,
		ST_MT_QUERY_EXECUTION_RESULT& p_refResult,
		std::vector<ST_MT_QUERY_BAR>& p_refBars,
		bool& p_refTimeGenerationCurrent,
		std::string& p_refError)
	{
		p_refBars.clear();
		p_refTimeGenerationCurrent = false;
		ST_PLUGIN_BINARY_VALUE stRangeRequest;
		if (p_pNodeManager == nullptr ||
			!BuildBarRangeRequest(p_usVersion, p_iNo, p_refSymbol,
				p_llInterval, p_refRange.llFrom, p_refRange.llTo,
				stRangeRequest, p_refError) ||
			!p_pNodeManager->Execute(p_usVersion, p_iNo,
				EN_PLUGIN_FUNC_QUERY_BARS, stRangeRequest,
				p_refResult, p_refError,
				&p_refTimeGenerationCurrent))
		{
			return false;
		}
		if (p_refResult.iCode != EN_TERMINAL_ERROR_OK)
		{
			return true;
		}
		std::map<std::string, std::vector<ST_MT_QUERY_BAR>> mapBars;
		if (!p_refResult.refData ||
			!DecodeAdapterBars(*p_refResult.refData, mapBars,
				p_refError))
		{
			return false;
		}
		const auto iterBars = mapBars.find(p_refSymbol);
		if (iterBars != mapBars.end())
		{
			p_refBars = iterBars->second;
		}
		return true;
	}

	// 统计指定锚点之前可用于尾部分页的记录，避免 cache 已满足时仍访问 MT。
	std::size_t CountTailBars(const std::vector<ST_MT_QUERY_BAR>& p_refBars,
		std::int64_t p_llAnchorExclusive)
	{
		return static_cast<std::size_t>(std::count_if(
			p_refBars.begin(), p_refBars.end(),
			[p_llAnchorExclusive](const ST_MT_QUERY_BAR& p_refBar)
			{
				return p_llAnchorExclusive <= 0 ||
					p_refBar.llDateTime < p_llAnchorExclusive;
			}));
	}

	// V2 固定周期集合；MT5 额外支持非标准分钟和小时周期。
	bool IsSupportedBarsPeriod(std::uint16_t p_usVersion,
		std::int64_t p_llInterval)
	{
		static const std::set<std::int64_t> s_setMt4 =
			{ 1, 5, 15, 30, 60, 240, 1440, 10080, 43200 };
		static const std::set<std::int64_t> s_setMt5 =
			{ 1, 2, 3, 4, 5, 6, 10, 12, 15, 20, 30,
			  60, 120, 180, 240, 360, 480, 720,
			  1440, 10080, 43200 };
		return p_usVersion == 4U ?
			s_setMt4.count(p_llInterval) != 0U :
			(p_usVersion == 5U &&
			 s_setMt5.count(p_llInterval) != 0U);
	}

	std::int64_t GetNowMs()
	{
		return std::chrono::duration_cast<
			std::chrono::milliseconds>(
			std::chrono::system_clock::now().
				time_since_epoch()).count();
	}

	// 读取对象中的数组字段；1167 只接受 Adapter 明确登记的四类权威实体。
	const ST_PLUGIN_BINARY_VALUE* FindArrayField(
		const ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName, std::string& p_refError)
	{
		const ST_PLUGIN_BINARY_VALUE* pValue =
			FindPluginBinaryField(p_refObject, p_szName);
		if (pValue == nullptr || pValue->enType !=
			EN_PLUGIN_BINARY_VALUE_ARRAY)
		{
			p_refError = std::string(
				"QUERY_DERIVE_STATE_FIELD_INVALID: field=") +
				p_szName + ", expected=array";
			return nullptr;
		}
		return pValue;
	}

	// 使用稳定 FNV-1a 计算值树指纹，跨页期间任一权威字段变化都会生成新的 SnapshotId。
	void HashBytes(std::uint64_t& p_refHash,
		const void* p_pBuffer, std::size_t p_szLength)
	{
		const unsigned char* pBuffer =
			static_cast<const unsigned char*>(p_pBuffer);
		for (std::size_t szIndex = 0;
			szIndex < p_szLength; ++szIndex)
		{
			p_refHash ^= pBuffer[szIndex];
			p_refHash *= 1099511628211ULL;
		}
	}

	bool HashBinaryValue(const ST_PLUGIN_BINARY_VALUE& p_refValue,
		std::uint64_t& p_refHash, std::size_t p_szDepth)
	{
		if (p_szDepth > PLUGIN_BINARY_MAX_DEPTH)
		{
			return false;
		}
		const std::uint16_t usType =
			static_cast<std::uint16_t>(p_refValue.enType);
		HashBytes(p_refHash, &usType, sizeof(usType));
		switch (p_refValue.enType)
		{
		case EN_PLUGIN_BINARY_VALUE_NULL:
			return true;
		case EN_PLUGIN_BINARY_VALUE_INT64:
			HashBytes(p_refHash, &p_refValue.llIntValue,
				sizeof(p_refValue.llIntValue));
			return true;
		case EN_PLUGIN_BINARY_VALUE_UINT64:
			HashBytes(p_refHash, &p_refValue.ullUIntValue,
				sizeof(p_refValue.ullUIntValue));
			return true;
		case EN_PLUGIN_BINARY_VALUE_DOUBLE:
			HashBytes(p_refHash, &p_refValue.dDoubleValue,
				sizeof(p_refValue.dDoubleValue));
			return true;
		case EN_PLUGIN_BINARY_VALUE_BOOL:
			HashBytes(p_refHash, &p_refValue.ucBoolValue,
				sizeof(p_refValue.ucBoolValue));
			return true;
		case EN_PLUGIN_BINARY_VALUE_STRING:
			HashBytes(p_refHash,
				p_refValue.strStringValue.data(),
				p_refValue.strStringValue.size());
			return true;
		case EN_PLUGIN_BINARY_VALUE_BYTES:
			HashBytes(p_refHash,
				p_refValue.aByteValue.data(),
				p_refValue.aByteValue.size());
			return true;
		case EN_PLUGIN_BINARY_VALUE_ARRAY:
			for (const std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& refItem :
				p_refValue.aArrayValue)
			{
				if (!refItem || !HashBinaryValue(*refItem,
						p_refHash, p_szDepth + 1))
				{
					return false;
				}
			}
			return true;
		case EN_PLUGIN_BINARY_VALUE_OBJECT:
			for (const ST_PLUGIN_BINARY_FIELD& refField :
				p_refValue.aObjectField)
			{
				HashBytes(p_refHash, refField.strName.data(),
					refField.strName.size());
				if (!refField.refValue ||
					!HashBinaryValue(*refField.refValue,
						p_refHash, p_szDepth + 1))
				{
					return false;
				}
			}
			return true;
		default:
			return false;
		}
	}

	// 从一个实体数组复制全局游标覆盖的部分，返回已经遍历的全局记录数。
	void AppendSnapshotPage(
		const ST_PLUGIN_BINARY_VALUE& p_refSource,
		std::uint64_t p_ullCursor, std::uint64_t p_ullLimit,
		std::uint64_t& p_refVisited,
		std::uint64_t& p_refEmitted,
		ST_PLUGIN_BINARY_VALUE& p_refTarget)
	{
		for (const std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& refItem :
			p_refSource.aArrayValue)
		{
			if (p_refVisited >= p_ullCursor &&
				p_refEmitted < p_ullLimit)
			{
				p_refTarget.aArrayValue.push_back(refItem);
				++p_refEmitted;
			}
			++p_refVisited;
		}
	}
}

CMtQueryDispatcher::CMtQueryDispatcher()
	: m_pConfig(nullptr)
	, m_pQuoteCache(nullptr)
	, m_pBarHistoryStore(nullptr)
	, m_pNodeManager(nullptr)
	, m_pClientDataService(nullptr)
	, m_strProgramVersion("0.0.0.0")
{
}

CMtQueryDispatcher::~CMtQueryDispatcher()
{
	m_pConfig = nullptr;
	m_pQuoteCache = nullptr;
	m_pBarHistoryStore = nullptr;
	m_pNodeManager = nullptr;
	m_pClientDataService = nullptr;
	m_strProgramVersion.clear();
}

bool CMtQueryDispatcher::Initialize(
	const ST_MT_QUERY_SERVICE_CONFIG* p_pConfig,
	const CMtQueryQuoteCache* p_pQuoteCache,
	CMtQueryNodeManager* p_pNodeManager,
	std::string& p_refError,
	IMtQueryClientDataService* p_pClientDataService,
	const std::string& p_refProgramVersion,
	CMtQueryBarHistoryStore* p_pBarHistoryStore)
{
	p_refError.clear();
	if (p_pConfig == nullptr || p_pQuoteCache == nullptr ||
		p_pNodeManager == nullptr)
	{
		p_refError =
			"QUERY_DISPATCHER_DEPENDENCY_INVALID: config, Quote cache and node manager are required";
		return false;
	}
	m_pConfig = p_pConfig;
	m_pQuoteCache = p_pQuoteCache;
	m_pBarHistoryStore = p_pBarHistoryStore;
	m_pNodeManager = p_pNodeManager;
	m_pClientDataService = p_pClientDataService;
	m_strProgramVersion = p_refProgramVersion.empty() ?
		"0.0.0.0" : p_refProgramVersion;
	return true;
}

bool CMtQueryDispatcher::Dispatch(std::int64_t p_llFuncId,
	std::int64_t p_llRouteCode,
	const ST_PLUGIN_BINARY_VALUE& p_refRequest,
	std::int32_t& p_refCode, std::string& p_refMessage,
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& p_refData,
	std::string& p_refError) const
{
	p_refCode = EN_TERMINAL_ERROR_OK;
	p_refMessage = "OK";
	p_refData.reset();
	p_refError.clear();
	if (m_pConfig == nullptr || m_pQuoteCache == nullptr ||
		m_pNodeManager == nullptr ||
		p_refRequest.enType != EN_PLUGIN_BINARY_VALUE_OBJECT)
	{
		p_refError =
			"QUERY_DISPATCHER_STATE_INVALID: dispatcher is not initialized or request root is not an object";
		return false;
	}
	switch (p_llFuncId)
	{
	case EN_PLUGIN_FUNC_QUERY_SERVER_INFO:
		return HandleServerInfo(p_refRequest, p_refCode,
			p_refMessage, p_refData, p_refError);
	case EN_PLUGIN_FUNC_QUERY_QUOTES:
		return HandleQuotes(p_refRequest, p_refCode,
			p_refMessage, p_refData, p_refError);
	case EN_PLUGIN_FUNC_QUERY_BARS:
		return HandleBars(p_refRequest, p_refCode,
			p_refMessage, p_refData, p_refError);
	case EN_PLUGIN_FUNC_QUERY_DERIVE_STATE_SNAPSHOT:
		return HandleDeriveStateSnapshot(p_refRequest,
			p_refCode, p_refMessage, p_refData,
			p_refError);
	case EN_PLUGIN_FUNC_QUERY_MT5_PROFIT_GROUP_QUOTE:
		return HandleMt5ProfitGroupQuote(p_refRequest,
			p_refCode, p_refMessage, p_refData,
			p_refError);
	case EN_PLUGIN_FUNC_QUERY_SYMBOLS:
	case EN_PLUGIN_FUNC_QUERY_SYMBOL_RATES:
	case EN_PLUGIN_FUNC_QUERY_VOLUME_RANK:
	case EN_PLUGIN_FUNC_QUERY_ACCOUNTS:
	case EN_PLUGIN_FUNC_QUERY_ACCOUNT_SYMBOLS:
	case EN_PLUGIN_FUNC_QUERY_ACCOUNT_SYMBOL_TRADING:
	case EN_PLUGIN_FUNC_QUERY_ORDERS:
	case EN_PLUGIN_FUNC_QUERY_POSITIONS:
	case EN_PLUGIN_FUNC_QUERY_DEALS:
		return HandleMtQuery(p_llFuncId, p_refRequest,
			p_refCode, p_refMessage, p_refData, p_refError);
	case EN_PLUGIN_FUNC_QUERY_WATCHLIST:
	case EN_PLUGIN_FUNC_QUERY_CHART:
		return HandleClientData(p_llFuncId,
			p_llRouteCode, p_refRequest, p_refCode, p_refMessage,
			p_refData, p_refError);
	default:
		p_refError =
			"QUERY_FUNCTION_NOT_REGISTERED: function identifier has no Query handler";
		return false;
	}
}

bool CMtQueryDispatcher::HandleServerInfo(
	const ST_PLUGIN_BINARY_VALUE& p_refRequest,
	std::int32_t& p_refCode, std::string& p_refMessage,
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& p_refData,
	std::string& p_refError) const
{
	// 第一步：内部调用也必须执行完整字段校验，避免绕过 Gateway 后获得诊断数据或错误筛选。
	std::set<std::string> setField;
	for (const ST_PLUGIN_BINARY_FIELD& refField :
		p_refRequest.aObjectField)
	{
		if (refField.strName != "Version" &&
			refField.strName != "No" &&
			refField.strName != "WebUserStats")
		{
			p_refCode = EN_TERMINAL_ERROR_MT_PARAM;
			p_refMessage =
				"MT_PARAMETER_INVALID: server_info only supports Version, No and WebUserStats";
			p_refData = MakeValue(
				EN_PLUGIN_BINARY_VALUE_OBJECT, p_refError);
			return p_refData != nullptr;
		}
		if (!setField.insert(refField.strName).second)
		{
			p_refCode = EN_TERMINAL_ERROR_MT_PARAM;
			p_refMessage =
				"MT_PARAMETER_INVALID: server_info contains a duplicate field=" +
				refField.strName;
			p_refData = MakeValue(
				EN_PLUGIN_BINARY_VALUE_OBJECT, p_refError);
			return p_refData != nullptr;
		}
	}
	const ST_PLUGIN_BINARY_VALUE* pVersion =
		FindPluginBinaryField(p_refRequest, "Version");
	std::int64_t llVersion = 0;
	std::int64_t llNo = 0;
	const ST_PLUGIN_BINARY_VALUE* pWebUserStats =
		FindPluginBinaryField(p_refRequest, "WebUserStats");
	if (pVersion == nullptr ||
		!ReadInt64(p_refRequest, "Version", 0,
			llVersion, p_refError) ||
		!ReadInt64(p_refRequest, "No", 0,
			llNo, p_refError) ||
		(llVersion != 0 && llVersion != 4 && llVersion != 5) ||
		llNo < 0 ||
		llNo > (std::numeric_limits<std::int32_t>::max)() ||
		(llVersion == 0 && llNo != 0) ||
		(pWebUserStats != nullptr &&
			pWebUserStats->enType != EN_PLUGIN_BINARY_VALUE_BOOL))
	{
		p_refCode = EN_TERMINAL_ERROR_MT_PARAM;
		p_refMessage = p_refError.empty() ?
			"MT_PARAMETER_INVALID: Version is required and must be 0/4/5; No must be non-negative and zero when Version=0; WebUserStats must be boolean" :
			"MT_PARAMETER_INVALID: " + p_refError;
		p_refError.clear();
		p_refData = MakeValue(
			EN_PLUGIN_BINARY_VALUE_OBJECT, p_refError);
		return p_refData != nullptr;
	}

	// 第二步：复制动态时间状态；偏移未同步或已过期时保持 V2 的 SYSTEM_NOT_READY 语义。
	std::vector<ST_MT_QUERY_QUOTE_SOURCE_STATUS> aQuoteStatus;
	m_pQuoteCache->GetStatus(aQuoteStatus);
	p_refData = MakeValue(EN_PLUGIN_BINARY_VALUE_OBJECT,
		p_refError);
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refMt4 =
		MakeValue(EN_PLUGIN_BINARY_VALUE_ARRAY, p_refError);
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refMt5 =
		MakeValue(EN_PLUGIN_BINARY_VALUE_ARRAY, p_refError);
	if (!p_refData || !refMt4 || !refMt5 ||
		!AddString(*p_refData, "ServerVersion",
			m_strProgramVersion, p_refError))
	{
		return false;
	}
	for (const ST_MT_QUERY_SOURCE_CONFIG& refSource :
		m_pConfig->aSource)
	{
		if (!refSource.bEnable || refSource.aConnection.empty() ||
			(llVersion != 0 && refSource.usVersion != llVersion) ||
			(llNo > 0 && refSource.iNo != llNo))
		{
			continue;
		}
		const ST_MT_QUERY_QUOTE_SOURCE_STATUS* pTimeStatus = nullptr;
		for (const ST_MT_QUERY_QUOTE_SOURCE_STATUS& refStatus :
			aQuoteStatus)
		{
			if (refStatus.usVersion == refSource.usVersion &&
				refStatus.iNo == refSource.iNo)
			{
				pTimeStatus = &refStatus;
				break;
			}
		}
		if (pTimeStatus == nullptr || !pTimeStatus->bTimeReady)
		{
			p_refCode = EN_TERMINAL_ERROR_MT_SYSTEM_NOT_READY;
			p_refMessage =
				"MT_SYSTEM_NOT_READY: server time offset is unavailable, Version=" +
				std::to_string(refSource.usVersion) + ", No=" +
				std::to_string(refSource.iNo);
			p_refData = MakeValue(
				EN_PLUGIN_BINARY_VALUE_OBJECT, p_refError);
			return p_refData != nullptr;
		}

		// 第三步：外部节点结构严格限制为 Id/Name/ServerTimeOffset，不泄漏地址、账号和池状态。
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refManager =
			MakeValue(EN_PLUGIN_BINARY_VALUE_OBJECT, p_refError);
		if (!refManager ||
			!AddInt64(*refManager, "Id", refSource.iNo, p_refError) ||
			!AddString(*refManager, "Name", refSource.strName,
				p_refError) ||
			!AddInt64(*refManager, "ServerTimeOffset",
				pTimeStatus->iTimeOffsetSeconds, p_refError))
		{
			return false;
		}
		if (refSource.usVersion == 4)
		{
			refMt4->aArrayValue.push_back(refManager);
		}
		else
		{
			refMt5->aArrayValue.push_back(refManager);
		}
	}
	return AddField(*p_refData, "Mt4Managers", refMt4,
			p_refError) &&
		AddField(*p_refData, "Mt5Managers", refMt5,
			p_refError);
}

bool CMtQueryDispatcher::HandleQuotes(
	const ST_PLUGIN_BINARY_VALUE& p_refRequest,
	std::int32_t& p_refCode, std::string& p_refMessage,
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& p_refData,
	std::string& p_refError) const
{
	std::int64_t llVersion = 5;
	std::int64_t llNo = 1;
	std::string strSymbolIds;
	if (!ReadInt64(p_refRequest, "Version", 5,
			llVersion, p_refError) ||
		!ReadInt64(p_refRequest, "No", 1,
			llNo, p_refError) ||
		!ReadString(p_refRequest, "SymbolIds",
			strSymbolIds, p_refError) ||
		(llVersion != 4 && llVersion != 5) ||
		llNo <= 0 ||
		llNo > (std::numeric_limits<std::int32_t>::max)())
	{
		if (p_refError.empty())
		{
			p_refError =
				"QUERY_QUOTES_PARAMETER_INVALID: Version must be 4/5 and No must be positive";
		}
		return false;
	}

	std::vector<ST_QUOTE_BINARY_TICK> aTick;
	ST_MT_QUERY_QUOTE_SOURCE_STATUS stStatus;
	if (!m_pQuoteCache->GetQuotes(
			static_cast<std::uint16_t>(llVersion),
			static_cast<std::int32_t>(llNo),
			ParseSymbolIds(strSymbolIds), aTick, stStatus,
			p_refError))
	{
		p_refCode =
			EN_TERMINAL_ERROR_MT_SYSTEM_NOT_READY;
		p_refMessage = p_refError;
		p_refError.clear();
		p_refData = MakeValue(
			EN_PLUGIN_BINARY_VALUE_ARRAY, p_refError);
		return p_refData != nullptr;
	}
	if (stStatus.llLastIngressMs > 0 &&
		GetNowMs() - stStatus.llLastIngressMs >
			m_pConfig->uiQuoteStaleMs)
	{
		p_refCode =
			EN_TERMINAL_ERROR_MT_SYSTEM_NOT_READY;
		p_refMessage =
			"QUERY_QUOTE_STALE: latest Quote exceeds configured quoteStaleMs";
	}
	p_refData = MakeValue(EN_PLUGIN_BINARY_VALUE_ARRAY,
		p_refError);
	if (!p_refData)
	{
		return false;
	}
	for (const ST_QUOTE_BINARY_TICK& refTick : aTick)
	{
		const double dOpenPrice = m_pQuoteCache->GetOpenPrice(
			refTick.usPlatformVersion, refTick.iSourceNo,
			refTick.strSymbol);
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refItem =
			MakeValue(EN_PLUGIN_BINARY_VALUE_OBJECT,
				p_refError);
		if (!refItem ||
			!AddInt64(*refItem, "Version",
				refTick.usPlatformVersion, p_refError) ||
			!AddInt64(*refItem, "No",
				refTick.iSourceNo, p_refError) ||
			!AddString(*refItem, "Symbol",
				refTick.strSymbol, p_refError) ||
			!AddDouble(*refItem, "Bid",
				refTick.dBid, p_refError) ||
			!AddDouble(*refItem, "Ask",
				refTick.dAsk, p_refError) ||
			!AddDouble(*refItem, "Last",
				refTick.dLast, p_refError) ||
			!AddDouble(*refItem, "OpenPrice",
				dOpenPrice, p_refError) ||
			!AddDouble(*refItem, "ClosePrice",
				0.0, p_refError) ||
			!AddDouble(*refItem, "LowPrice",
				0.0, p_refError) ||
			!AddDouble(*refItem, "HighPrice",
				0.0, p_refError) ||
			!AddDouble(*refItem, "PrevClosedPrice",
				0.0, p_refError) ||
			!AddUInt64(*refItem, "Volume",
				refTick.ullVolume, p_refError) ||
			!AddUInt64(*refItem, "VolumeExt",
				refTick.ullVolumeExt, p_refError) ||
			!AddUInt64(*refItem, "Flags",
				refTick.ullFlags, p_refError) ||
			!AddInt64(*refItem, "Time",
				refTick.llServerTime, p_refError) ||
			!AddInt64(*refItem, "TimeMsc",
				refTick.llServerTimeMsc, p_refError))
		{
			return false;
		}
		p_refData->aArrayValue.push_back(refItem);
	}
	return true;
}

bool CMtQueryDispatcher::HandleBars(
	const ST_PLUGIN_BINARY_VALUE& p_refRequest,
	std::int32_t& p_refCode, std::string& p_refMessage,
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& p_refData,
	std::string& p_refError) const
{
	std::int64_t llVersion = 5;
	std::int64_t llNo = 1;
	std::int64_t llInterval = 0;
	std::int64_t llFrom = 0;
	std::int64_t llTo = 0;
	std::int64_t llOffset = 0;
	std::int64_t llOffsetTime = 0;
	std::int64_t llItemNum = 0;
	std::string strSymbolIds;
	const bool bHasFrom = FindPluginBinaryField(
		p_refRequest, "From") != nullptr;
	const bool bHasTo = FindPluginBinaryField(
		p_refRequest, "To") != nullptr;
	const bool bHasOffset = FindPluginBinaryField(
		p_refRequest, "Offset") != nullptr;
	const bool bHasOffsetTime = FindPluginBinaryField(
		p_refRequest, "OffsetTime") != nullptr;
	const bool bHasItemNum = FindPluginBinaryField(
		p_refRequest, "ItemNum") != nullptr;
	if (!ReadInt64(p_refRequest, "Version", 5,
			llVersion, p_refError) ||
		!ReadInt64(p_refRequest, "No", 1,
			llNo, p_refError) ||
		!ReadInt64(p_refRequest, "Interval", 0,
			llInterval, p_refError) ||
		!ReadInt64(p_refRequest, "From", 0,
			llFrom, p_refError) ||
		!ReadInt64(p_refRequest, "To", 0,
			llTo, p_refError) ||
		!ReadInt64(p_refRequest, "Offset", 0,
			llOffset, p_refError) ||
		!ReadInt64(p_refRequest, "OffsetTime", 0,
			llOffsetTime, p_refError) ||
		!ReadInt64(p_refRequest, "ItemNum", 0,
			llItemNum, p_refError) ||
		!ReadString(p_refRequest, "SymbolIds",
			strSymbolIds, p_refError) ||
		(llVersion != 4 && llVersion != 5) || llNo <= 0 ||
		!IsSupportedBarsPeriod(static_cast<std::uint16_t>(llVersion),
			llInterval) || strSymbolIds.empty() ||
		(bHasFrom != bHasTo) ||
		((bHasOffset ? 1 : 0) + (bHasOffsetTime ? 1 : 0) +
		 (bHasFrom ? 1 : 0) != 1) ||
		((bHasOffset || bHasOffsetTime) != bHasItemNum) ||
		(bHasFrom && (llFrom <= 0 || llTo < llFrom)) ||
		(bHasOffset && (llOffset < 0 || llItemNum < 0 ||
			llOffset > static_cast<std::int64_t>(UINT32_MAX) ||
			llItemNum > 200 ||
			llOffset > static_cast<std::int64_t>(UINT32_MAX) - llItemNum ||
			(llItemNum == 0 && llOffset != 0))) ||
		(bHasOffsetTime && (llOffsetTime < 0 || llItemNum < 0 ||
			llItemNum > 200 ||
			(llItemNum == 0 && llOffsetTime != 0))))
	{
		if (p_refError.empty())
		{
			p_refError =
				"QUERY_BARS_PARAMETER_INVALID: Version, No, SymbolIds, Interval and range/tail scene are invalid";
		}
		p_refCode = EN_TERMINAL_ERROR_MT_PARAM;
		p_refMessage = p_refError;
		p_refError.clear();
		p_refData = MakeValue(EN_PLUGIN_BINARY_VALUE_ARRAY,
			p_refError);
		return p_refData != nullptr;
	}
	const std::set<std::string> setSymbols =
		ParseSymbolIds(strSymbolIds);
	if (setSymbols.empty())
	{
		p_refCode = EN_TERMINAL_ERROR_MT_PARAM;
		p_refMessage =
			"QUERY_BARS_SYMBOLS_EMPTY: SymbolIds contains no valid symbol";
		p_refData = MakeValue(EN_PLUGIN_BINARY_VALUE_ARRAY,
			p_refError);
		return p_refData != nullptr;
	}
	if (m_pBarHistoryStore == nullptr)
	{
		p_refCode = EN_TERMINAL_ERROR_MT_SYSTEM;
		p_refMessage =
			"QUERY_BAR_STORE_NOT_INITIALIZED: history store is required for 1157";
		p_refData = MakeValue(EN_PLUGIN_BINARY_VALUE_ARRAY,
			p_refError);
		return p_refData != nullptr;
	}

	p_refData = MakeValue(EN_PLUGIN_BINARY_VALUE_ARRAY,
		p_refError);
	if (!p_refData)
	{
		return false;
	}
	// 统一结束失败：多品种任一窗口未覆盖时清空已生成结果，禁止把部分品种包装成成功。
	const auto fnFinishFailure = [&p_refCode, &p_refMessage,
		&p_refData, &p_refError](std::int32_t p_iCode,
		EN_MT_QUERY_BAR_BACKFILL_RESULT p_enResult,
		const std::string& p_refDetail, std::uint16_t p_usVersion,
		std::int32_t p_iNo, const std::string& p_refSymbol)
	{
		p_refCode = p_iCode;
		p_refMessage = p_refDetail;
		p_refData->aArrayValue.clear();
		p_refError.clear();
		MT_WARN(
			"query bars unresolved,version=%u,no=%d,symbol=%s,result=%s,code=%d,detail=%s",
			static_cast<unsigned int>(p_usVersion), p_iNo,
			p_refSymbol.c_str(), GetBarBackfillResultName(p_enResult),
			p_iCode, p_refDetail.c_str());
		return true;
	};
	for (const std::string& refSymbol : setSymbols)
	{
		// 第一步：先读取本地数据和 coverage；只有未覆盖区间才能进入 HISTORY_QUERY 池。
		ST_MT_QUERY_BAR_HISTORY_KEY stKey;
		stKey.usVersion = static_cast<std::uint16_t>(llVersion);
		stKey.iNo = static_cast<std::int32_t>(llNo);
		stKey.strSymbol = refSymbol;
		stKey.llIntervalMinutes = llInterval;
		const std::size_t szTailNeed = bHasFrom ? 0U :
			static_cast<std::size_t>(llItemNum +
				(bHasOffset ? llOffset : 0));
		const std::int64_t llStoreReadFrom = bHasFrom ? llFrom : 0;
		const std::int64_t llStoreReadTo = bHasFrom ? llTo :
			(bHasOffsetTime && llOffsetTime > 0 ? llOffsetTime - 1 : 0);
		std::vector<ST_MT_QUERY_BAR> aMerged;
		EN_MT_QUERY_BAR_HISTORY_READ_STATE enReadState =
			EN_MT_QUERY_BAR_HISTORY_MISSING;
		if (!m_pBarHistoryStore->ReadSelected(stKey,
			llStoreReadFrom, llStoreReadTo, szTailNeed,
			aMerged, enReadState, p_refError))
		{
			return fnFinishFailure(EN_TERMINAL_ERROR_MT_SYSTEM,
				EN_MT_QUERY_BAR_BACKFILL_STORAGE_ERROR,
				p_refError.empty() ?
					"QUERY_BAR_STORE_READ_FAILED: history file read failed" :
					p_refError,
				stKey.usVersion, stKey.iNo, refSymbol);
		}
		std::vector<ST_MT_QUERY_BAR_COVERAGE_RANGE> aCoverage;
		EN_MT_QUERY_BAR_HISTORY_READ_STATE enCoverageState =
			EN_MT_QUERY_BAR_HISTORY_MISSING;
		if (!m_pBarHistoryStore->ReadCoverage(stKey, aCoverage,
			enCoverageState, p_refError))
		{
			return fnFinishFailure(EN_TERMINAL_ERROR_MT_SYSTEM,
				EN_MT_QUERY_BAR_BACKFILL_STORAGE_ERROR,
				p_refError.empty() ?
					"QUERY_BAR_COVERAGE_READ_FAILED: coverage file read failed" :
					p_refError,
				stKey.usVersion, stKey.iNo, refSymbol);
		}
		if (enReadState == EN_MT_QUERY_BAR_HISTORY_CORRUPT ||
			enCoverageState == EN_MT_QUERY_BAR_HISTORY_CORRUPT)
		{
			return fnFinishFailure(EN_TERMINAL_ERROR_MT_SYSTEM,
				EN_MT_QUERY_BAR_BACKFILL_STORAGE_ERROR,
				"QUERY_BAR_STORE_CORRUPT: damaged history or coverage file was quarantined",
				stKey.usVersion, stKey.iNo, refSymbol);
		}

		// 第二步：先合并 Derive 实时 M1；缓存已经满足分页时不得访问 MT。
		std::vector<ST_MT_QUERY_BAR> aRealtime;
		std::vector<ST_MT_QUERY_BAR> aPersistBars;
		std::string strRealtimeError;
		const std::int64_t llRealtimeFrom = bHasFrom ? llFrom :
			(bHasOffsetTime && llOffsetTime > 0 ? 1 : 0);
		const std::int64_t llRealtimeTo = bHasFrom ? llTo :
			(bHasOffsetTime && llOffsetTime > 0 ? llOffsetTime - 1 : 0);
		if (m_pQuoteCache->GetBars(
				static_cast<std::uint16_t>(llVersion),
				static_cast<std::int32_t>(llNo), refSymbol,
				llInterval, llRealtimeFrom, llRealtimeTo,
				szTailNeed, aRealtime, strRealtimeError))
		{
			CMtQueryBarHistoryStore::MergeBars(aMerged,
				aRealtime);
			aPersistBars = aRealtime;
		}

		// 第三步：范围请求只查询 coverage 缺口；同业务键并发回填立即返回，不等待第二条历史连接。
		std::vector<ST_MT_QUERY_BAR_COVERAGE_RANGE> aNewCoverage;
		std::size_t szMtCalls = 0U;
		EN_MT_QUERY_BAR_BACKFILL_RESULT enBackfillResult =
			EN_MT_QUERY_BAR_BACKFILL_NOT_NEEDED;
		std::string strBackfillDetail;
		std::unique_ptr<CQueryScopeExit> ptrBackfillLease;
		// 失败后重新读取当前权威 coverage；其他有效任务已完成时继续成功，存储异常则升级为 20000。
		const auto fnRefreshCoverage = [this, &stKey, &aMerged,
			&aCoverage, &enBackfillResult, &strBackfillDetail,
			llStoreReadFrom, llStoreReadTo, szTailNeed,
			&p_refCode, &p_refMessage, &p_refError](
			const ST_MT_QUERY_BAR_COVERAGE_RANGE& p_refRange,
			bool& p_refCovered)
		{
			p_refCovered = false;
			std::vector<ST_MT_QUERY_BAR> aLatestBars;
			std::vector<ST_MT_QUERY_BAR_COVERAGE_RANGE> aLatestCoverage;
			EN_MT_QUERY_BAR_HISTORY_READ_STATE enLatestBars =
				EN_MT_QUERY_BAR_HISTORY_MISSING;
			EN_MT_QUERY_BAR_HISTORY_READ_STATE enLatestCoverage =
				EN_MT_QUERY_BAR_HISTORY_MISSING;
			if (!m_pBarHistoryStore->ReadSelected(stKey,
					llStoreReadFrom, llStoreReadTo, szTailNeed,
					aLatestBars, enLatestBars, p_refError) ||
				!m_pBarHistoryStore->ReadCoverage(stKey,
					aLatestCoverage, enLatestCoverage, p_refError) ||
				enLatestBars == EN_MT_QUERY_BAR_HISTORY_CORRUPT ||
				enLatestCoverage == EN_MT_QUERY_BAR_HISTORY_CORRUPT)
			{
				enBackfillResult =
					EN_MT_QUERY_BAR_BACKFILL_STORAGE_ERROR;
				p_refCode = EN_TERMINAL_ERROR_MT_SYSTEM;
				strBackfillDetail = p_refError.empty() ?
					"QUERY_BAR_STORE_REFRESH_FAILED: current coverage could not be reloaded" :
					p_refError;
				p_refMessage = strBackfillDetail;
				p_refError.clear();
				return false;
			}
			CMtQueryBarHistoryStore::MergeBars(aMerged, aLatestBars);
			CMtQueryBarHistoryStore::MergeCoverage(aCoverage,
				aLatestCoverage);
			std::vector<ST_MT_QUERY_BAR_COVERAGE_RANGE> aStillMissing;
			CMtQueryBarHistoryStore::FindUncovered(aCoverage,
				p_refRange.llFrom, p_refRange.llTo, aStillMissing);
			p_refCovered = aStillMissing.empty();
			return true;
		};
		const auto fnFetchMissing = [this, llVersion, llNo,
			llInterval, &refSymbol, &aMerged, &aPersistBars,
			&aCoverage, &aNewCoverage, &szMtCalls, &stKey,
			&ptrBackfillLease, &enBackfillResult,
			&strBackfillDetail, &fnRefreshCoverage,
			&p_refCode, &p_refMessage,
			&p_refError](
			const std::vector<ST_MT_QUERY_BAR_COVERAGE_RANGE>& p_refMissing)
		{
			if (p_refMissing.empty())
			{
				return true;
			}
			for (const ST_MT_QUERY_BAR_COVERAGE_RANGE& refRange :
				p_refMissing)
			{
				if (!ptrBackfillLease)
				{
					bool bAcquired = false;
					if (!m_pBarHistoryStore->TryBeginBackfill(
							stKey, bAcquired, p_refError))
					{
						enBackfillResult =
							EN_MT_QUERY_BAR_BACKFILL_STORAGE_ERROR;
						p_refCode = EN_TERMINAL_ERROR_MT_SYSTEM;
						strBackfillDetail = p_refError.empty() ?
							"QUERY_BAR_BACKFILL_COORDINATOR_FAILED: failed to acquire store state" :
							p_refError;
						p_refMessage = strBackfillDetail;
						p_refError.clear();
						return true;
					}
					if (!bAcquired)
					{
						bool bCovered = false;
						if (!fnRefreshCoverage(refRange, bCovered))
						{
							return true;
						}
						if (bCovered)
						{
							enBackfillResult =
								EN_MT_QUERY_BAR_BACKFILL_NOT_NEEDED;
							continue;
						}
						enBackfillResult =
							EN_MT_QUERY_BAR_BACKFILL_IN_PROGRESS;
						p_refCode = EN_TERMINAL_ERROR_MT_SYSTEM_NOT_READY;
						strBackfillDetail =
							"QUERY_BARS_BACKFILL_IN_PROGRESS: another request is filling the same Version+No+Symbol+Interval";
						p_refMessage = strBackfillDetail;
						return true;
					}
					ptrBackfillLease.reset(new CQueryScopeExit(
						[this, stKey]()
						{
							m_pBarHistoryStore->EndBackfill(stKey);
						}));
				}
				if (szMtCalls >= 64U)
				{
					enBackfillResult =
						EN_MT_QUERY_BAR_BACKFILL_SOURCE_STALE;
					p_refCode =
						EN_TERMINAL_ERROR_MT_MANAGER_CONNECTION_BUSY;
					strBackfillDetail =
						"QUERY_BARS_BACKFILL_LIMIT: request requires more than 64 MT history gap calls";
					p_refMessage = strBackfillDetail;
					return true;
				}
				ST_MT_QUERY_EXECUTION_RESULT stResult;
				std::vector<ST_MT_QUERY_BAR> aFetched;
				bool bTimeGenerationCurrent = false;
				if (!ExecuteBarRange(m_pNodeManager,
						static_cast<std::uint16_t>(llVersion),
						static_cast<std::int32_t>(llNo), refSymbol,
						llInterval, refRange, stResult, aFetched,
						bTimeGenerationCurrent, p_refError))
				{
					const std::string strExecuteError = p_refError;
					const bool bStorage =
						IsBarStorageFailureDetail(strExecuteError);
					bool bCovered = false;
					if (!bStorage &&
						!fnRefreshCoverage(refRange, bCovered))
					{
						return true;
					}
					if (bCovered)
					{
						enBackfillResult =
							EN_MT_QUERY_BAR_BACKFILL_NOT_NEEDED;
						p_refError.clear();
						MT_DEBUG(
							"query bars fallback covered,version=%lld,no=%lld,symbol=%s,from=%lld,to=%lld,result=SOURCE_STALE",
							static_cast<long long>(llVersion),
							static_cast<long long>(llNo), refSymbol.c_str(),
							static_cast<long long>(refRange.llFrom),
							static_cast<long long>(refRange.llTo));
						continue;
					}
					enBackfillResult = bStorage ?
						EN_MT_QUERY_BAR_BACKFILL_STORAGE_ERROR :
						EN_MT_QUERY_BAR_BACKFILL_SOURCE_STALE;
					p_refCode = bStorage ? EN_TERMINAL_ERROR_MT_SYSTEM :
						EN_TERMINAL_ERROR_MT_SYSTEM_NOT_READY;
					strBackfillDetail = strExecuteError.empty() ?
						"QUERY_BARS_BACKFILL_UNAVAILABLE: history source did not complete the request" :
						strExecuteError;
					p_refMessage = strBackfillDetail;
					p_refError.clear();
					return true;
				}
				++szMtCalls;
				if (stResult.iCode != EN_TERMINAL_ERROR_OK)
				{
					bool bCovered = false;
					if (!fnRefreshCoverage(refRange, bCovered))
					{
						return true;
					}
					if (bCovered)
					{
						enBackfillResult =
							EN_MT_QUERY_BAR_BACKFILL_NOT_NEEDED;
						MT_DEBUG(
							"query bars fallback covered,version=%lld,no=%lld,symbol=%s,from=%lld,to=%lld,result=MT_TRANSIENT_ERROR",
							static_cast<long long>(llVersion),
							static_cast<long long>(llNo), refSymbol.c_str(),
							static_cast<long long>(refRange.llFrom),
							static_cast<long long>(refRange.llTo));
						continue;
					}
					enBackfillResult =
						EN_MT_QUERY_BAR_BACKFILL_MT_TRANSIENT_ERROR;
					p_refCode = stResult.iCode ==
							EN_TERMINAL_ERROR_MT_MANAGER_CONNECTION_BUSY ?
							stResult.iCode :
							(stResult.iCode >= EN_TERMINAL_ERROR_MT_SYSTEM ?
								stResult.iCode :
								EN_TERMINAL_ERROR_MT_SYSTEM_NOT_READY);
					strBackfillDetail = stResult.strMessage.empty() ?
						"QUERY_BARS_MT_TRANSIENT_ERROR: MT history request failed" :
						stResult.strMessage;
					p_refMessage = strBackfillDetail;
					return true;
				}
				if (!bTimeGenerationCurrent)
				{
					bool bCovered = false;
					if (!fnRefreshCoverage(refRange, bCovered))
					{
						return true;
					}
					if (bCovered)
					{
						enBackfillResult =
							EN_MT_QUERY_BAR_BACKFILL_NOT_NEEDED;
						MT_DEBUG(
							"query bars fallback covered,version=%lld,no=%lld,symbol=%s,from=%lld,to=%lld,result=GENERATION_CHANGED",
							static_cast<long long>(llVersion),
							static_cast<long long>(llNo), refSymbol.c_str(),
							static_cast<long long>(refRange.llFrom),
							static_cast<long long>(refRange.llTo));
						continue;
					}
					enBackfillResult =
						EN_MT_QUERY_BAR_BACKFILL_GENERATION_CHANGED;
					p_refCode = EN_TERMINAL_ERROR_MT_SYSTEM_NOT_READY;
					strBackfillDetail =
						"QUERY_BARS_GENERATION_CHANGED: time authority changed during MT history request";
					p_refMessage = strBackfillDetail;
					MT_DEBUG(
						"query bars stale generation rejected,version=%lld,no=%lld,symbol=%s,from=%lld,to=%lld",
						static_cast<long long>(llVersion),
						static_cast<long long>(llNo), refSymbol.c_str(),
						static_cast<long long>(refRange.llFrom),
						static_cast<long long>(refRange.llTo));
					return true;
				}
				CMtQueryBarHistoryStore::MergeBars(aMerged, aFetched);
				CMtQueryBarHistoryStore::MergeBars(aPersistBars,
					aFetched);
				std::vector<ST_MT_QUERY_BAR_COVERAGE_RANGE> aConfirmed;
				for (const ST_MT_QUERY_CONFIRMED_RANGE& refConfirmed :
					stResult.aConfirmedRange)
				{
					ST_MT_QUERY_BAR_COVERAGE_RANGE stConfirmed;
					stConfirmed.llFrom = (std::max)(refRange.llFrom,
						refConfirmed.llFrom);
					stConfirmed.llTo = (std::min)(refRange.llTo,
						refConfirmed.llTo);
					if (stConfirmed.llTo >= stConfirmed.llFrom)
					{
						aConfirmed.push_back(stConfirmed);
					}
				}
				CMtQueryBarHistoryStore::MergeCoverage(aCoverage,
					aConfirmed);
				CMtQueryBarHistoryStore::MergeCoverage(aNewCoverage,
					aConfirmed);
				std::vector<ST_MT_QUERY_BAR_COVERAGE_RANGE> aStillMissing;
				CMtQueryBarHistoryStore::FindUncovered(aCoverage,
					refRange.llFrom, refRange.llTo, aStillMissing);
				if (!aStillMissing.empty())
				{
					bool bCovered = false;
					if (!fnRefreshCoverage(refRange, bCovered))
					{
						return true;
					}
					if (bCovered)
					{
						enBackfillResult =
							EN_MT_QUERY_BAR_BACKFILL_NOT_NEEDED;
						continue;
					}
					enBackfillResult =
						EN_MT_QUERY_BAR_BACKFILL_MT_TRANSIENT_ERROR;
					p_refCode = EN_TERMINAL_ERROR_MT_SYSTEM_NOT_READY;
					strBackfillDetail =
						"QUERY_BARS_BACKFILL_PARTIAL: MT did not confirm the complete requested window";
					p_refMessage = strBackfillDetail;
					return true;
				}
				enBackfillResult = EN_MT_QUERY_BAR_BACKFILL_APPLIED;
			}
			return true;
		};
		if (bHasFrom)
		{
			std::vector<ST_MT_QUERY_BAR_COVERAGE_RANGE> aMissing;
			CMtQueryBarHistoryStore::FindUncovered(aCoverage,
				llFrom, llTo, aMissing);
			if (!fnFetchMissing(aMissing))
			{
				return false;
			}
		}
		else if (llItemNum > 0)
		{
			// 尾部分页按固定页宽向过去扩展，最多 16 页和 64 次缺口调用，防止稀疏品种拖垮历史池。
			const std::size_t szOffset = bHasOffset ?
				static_cast<std::size_t>(llOffset) : 0U;
			const std::size_t szNeed = szOffset >
				(std::numeric_limits<std::size_t>::max)() -
					static_cast<std::size_t>(llItemNum) ?
				(std::numeric_limits<std::size_t>::max)() :
				szOffset + static_cast<std::size_t>(llItemNum);
			const std::int64_t llAnchorExclusive =
				bHasOffsetTime && llOffsetTime > 0 ? llOffsetTime : 0;
			std::int64_t llPageTo = llAnchorExclusive > 0 ?
				llAnchorExclusive - 1 : GetNowMs() / 1000LL;
			const std::int64_t llPageBars = (std::max)(
				512LL, static_cast<std::int64_t>(
					(std::min)(szNeed, static_cast<std::size_t>(5000U))) *
					4LL);
			const std::int64_t llIntervalSeconds = llInterval * 60LL;
			const std::int64_t llPageSpan = llPageBars >
				(std::numeric_limits<std::int64_t>::max)() /
					llIntervalSeconds ?
				(std::numeric_limits<std::int64_t>::max)() :
				llPageBars * llIntervalSeconds;
			for (std::size_t szPage = 0U; szPage < 16U &&
				CountTailBars(aMerged, llAnchorExclusive) < szNeed &&
				llPageTo > 0 && szMtCalls < 64U; ++szPage)
			{
				const std::int64_t llPageFrom = llPageTo >= llPageSpan ?
					llPageTo - llPageSpan + 1 : 1;
				std::vector<ST_MT_QUERY_BAR_COVERAGE_RANGE> aMissing;
				CMtQueryBarHistoryStore::FindUncovered(aCoverage,
					llPageFrom, llPageTo, aMissing);
				if (!fnFetchMissing(aMissing))
				{
					return false;
				}
				if (p_refCode != EN_TERMINAL_ERROR_OK || llPageFrom <= 1)
				{
					break;
				}
				llPageTo = llPageFrom - 1;
			}
		}
		// 第四步：先提交已确认的数据和 coverage；后续缺口失败不能丢弃前面已经完成的有效分段。
		std::string strPersistError;
		if (!m_pBarHistoryStore->EnqueueMerge(stKey, aPersistBars,
			aNewCoverage, strPersistError))
		{
			enBackfillResult = EN_MT_QUERY_BAR_BACKFILL_STORAGE_ERROR;
			p_refCode = EN_TERMINAL_ERROR_MT_SYSTEM;
			strBackfillDetail = strPersistError.empty() ?
				"QUERY_BAR_STORE_SUBMIT_FAILED: persistence queue rejected confirmed data" :
				strPersistError;
			p_refMessage = strBackfillDetail;
		}
		if (p_refCode != EN_TERMINAL_ERROR_OK)
		{
			return fnFinishFailure(p_refCode, enBackfillResult,
				strBackfillDetail.empty() ? p_refMessage : strBackfillDetail,
				stKey.usVersion, stKey.iNo, refSymbol);
		}

		// 第五步：最后执行 V2 范围、OffsetTime 或兼容 Offset 分页；三种请求格式共用新模式数据链，输出保持时间升序。
		std::vector<ST_MT_QUERY_BAR> aSelectable = aMerged;
		if (bHasOffsetTime && llOffsetTime > 0)
		{
			aSelectable.erase(std::remove_if(aSelectable.begin(),
				aSelectable.end(), [llOffsetTime](
					const ST_MT_QUERY_BAR& p_refBar)
				{
					return p_refBar.llDateTime >= llOffsetTime;
				}), aSelectable.end());
		}
		std::vector<ST_MT_QUERY_BAR> aBars;
		CMtQueryBarHistoryStore::SelectBars(aSelectable,
			bHasFrom ? llFrom : 0, bHasFrom ? llTo : 0,
			bHasOffset ? static_cast<std::size_t>(llOffset) : 0U,
			bHasFrom ? 0U : static_cast<std::size_t>(llItemNum),
			aBars);
		if (aBars.empty())
		{
			continue;
		}
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refSymbolBars =
			MakeValue(EN_PLUGIN_BINARY_VALUE_OBJECT,
				p_refError);
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refBars =
			MakeValue(EN_PLUGIN_BINARY_VALUE_ARRAY,
				p_refError);
		if (!refSymbolBars || !refBars ||
			!AddString(*refSymbolBars, "SymbolId",
				refSymbol, p_refError))
		{
			return false;
		}
		for (const ST_MT_QUERY_BAR& refBar : aBars)
		{
			std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refItem =
				MakeValue(EN_PLUGIN_BINARY_VALUE_OBJECT,
					p_refError);
			if (!refItem ||
				!AddInt64(*refItem, "DateTime",
					refBar.llDateTime, p_refError) ||
				!AddDouble(*refItem, "Open",
					refBar.dOpen, p_refError) ||
				!AddDouble(*refItem, "Close",
					refBar.dClose, p_refError) ||
				!AddDouble(*refItem, "High",
					refBar.dHigh, p_refError) ||
				!AddDouble(*refItem, "Low",
					refBar.dLow, p_refError) ||
				!AddDouble(*refItem, "Value",
					static_cast<double>(refBar.ullTickVolume),
					p_refError))
			{
				return false;
			}
			refBars->aArrayValue.push_back(refItem);
		}
		if (!AddField(*refSymbolBars, "Bars",
			refBars, p_refError))
		{
			return false;
		}
		p_refData->aArrayValue.push_back(refSymbolBars);
	}
	return true;
}

bool CMtQueryDispatcher::HandleDeriveStateSnapshot(
	const ST_PLUGIN_BINARY_VALUE& p_refRequest,
	std::int32_t& p_refCode, std::string& p_refMessage,
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& p_refData,
	std::string& p_refError) const
{
	// 第一步：严格读取来源、实体位、游标和页大小，避免大快照绕过 50 MiB 传输限制。
	std::int64_t llVersion = 0;
	std::int64_t llNo = 0;
	std::uint64_t ullEntityMask = 0;
	std::uint64_t ullCursor = 0;
	std::uint64_t ullLimit = 0;
	if (!ReadInt64(p_refRequest, "Version", 0,
			llVersion, p_refError) ||
		!ReadInt64(p_refRequest, "No", 0,
			llNo, p_refError) ||
		!ReadUInt64(p_refRequest, "EntityMask", 0,
			ullEntityMask, p_refError) ||
		!ReadUInt64(p_refRequest, "Cursor", 0,
			ullCursor, p_refError) ||
		!ReadUInt64(p_refRequest, "Limit", 1000,
			ullLimit, p_refError) ||
		(llVersion != 4 && llVersion != 5) ||
		llNo <= 0 ||
		ullEntityMask == 0 ||
		(ullEntityMask &
			~static_cast<std::uint64_t>(
				EN_DERIVE_STATE_ENTITY_ALL)) != 0 ||
		ullLimit == 0 || ullLimit > 5000)
	{
		if (p_refError.empty())
		{
			p_refError =
				"QUERY_DERIVE_SNAPSHOT_PARAMETER_INVALID: Version, No, EntityMask, Cursor or Limit is invalid";
		}
		return false;
	}

	// 第二步：使用一个普通连接租约捕获账户、持仓、品种和组，避免同一页混入不同连接时刻的数据。
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refState;
	if (!m_pNodeManager->CaptureDeriveState(
			static_cast<std::uint16_t>(llVersion),
			static_cast<std::int32_t>(llNo),
			refState, p_refError) || !refState ||
		refState->enType != EN_PLUGIN_BINARY_VALUE_OBJECT)
	{
		p_refCode = EN_TERMINAL_ERROR_MT_SYSTEM_NOT_READY;
		p_refMessage = p_refError.empty() ?
			"QUERY_DERIVE_SNAPSHOT_NOT_READY: authoritative MT state is unavailable" :
			p_refError;
		p_refError.clear();
		p_refData = MakeValue(
			EN_PLUGIN_BINARY_VALUE_OBJECT, p_refError);
		return p_refData != nullptr;
	}
	const ST_PLUGIN_BINARY_VALUE* pAccounts =
		FindArrayField(*refState, "Accounts", p_refError);
	const ST_PLUGIN_BINARY_VALUE* pPositions =
		FindArrayField(*refState, "Positions", p_refError);
	const ST_PLUGIN_BINARY_VALUE* pSymbols =
		FindArrayField(*refState, "Symbols", p_refError);
	const ST_PLUGIN_BINARY_VALUE* pGroups =
		FindArrayField(*refState, "Groups", p_refError);
	if (pAccounts == nullptr || pPositions == nullptr ||
		pSymbols == nullptr || pGroups == nullptr)
	{
		return false;
	}

	// 第三步：汇率边来自 1122+1211 最新快照，与 MT 状态共同参与快照指纹。
	std::vector<ST_QUOTE_BINARY_TICK> aTick;
	ST_MT_QUERY_QUOTE_SOURCE_STATUS stStatus;
	const std::set<std::string> setEmpty;
	if (!m_pQuoteCache->GetQuotes(
			static_cast<std::uint16_t>(llVersion),
			static_cast<std::int32_t>(llNo),
			setEmpty, aTick, stStatus, p_refError))
	{
		p_refCode =
			EN_TERMINAL_ERROR_MT_SYSTEM_NOT_READY;
		p_refMessage = p_refError;
		p_refError.clear();
		p_refData = MakeValue(
			EN_PLUGIN_BINARY_VALUE_OBJECT, p_refError);
		return p_refData != nullptr;
	}
	p_refData = MakeValue(
		EN_PLUGIN_BINARY_VALUE_OBJECT, p_refError);
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refAccounts =
		MakeValue(EN_PLUGIN_BINARY_VALUE_ARRAY, p_refError);
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refPositions =
		MakeValue(EN_PLUGIN_BINARY_VALUE_ARRAY, p_refError);
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refSymbols =
		MakeValue(EN_PLUGIN_BINARY_VALUE_ARRAY, p_refError);
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refGroups =
		MakeValue(EN_PLUGIN_BINARY_VALUE_ARRAY, p_refError);
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refRates =
		MakeValue(EN_PLUGIN_BINARY_VALUE_ARRAY, p_refError);
	if (!p_refData || !refAccounts || !refPositions ||
		!refSymbols || !refGroups || !refRates)
	{
		return false;
	}
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refAllRates =
		MakeValue(EN_PLUGIN_BINARY_VALUE_ARRAY, p_refError);
	if (!refAllRates)
	{
		return false;
	}
	for (const ST_QUOTE_BINARY_TICK& refTick : aTick)
	{
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refRate =
			MakeValue(EN_PLUGIN_BINARY_VALUE_OBJECT,
				p_refError);
		if (!refRate ||
			!AddString(*refRate, "Symbol",
				refTick.strSymbol, p_refError) ||
			!AddDouble(*refRate, "Bid",
				refTick.dBid, p_refError) ||
			!AddDouble(*refRate, "Ask",
				refTick.dAsk, p_refError) ||
			!AddInt64(*refRate, "TimestampMs",
				refTick.llIngressTimeMs, p_refError) ||
			!AddUInt64(*refRate, "SourceEpoch",
				refTick.ullSourceEpoch, p_refError) ||
			!AddUInt64(*refRate, "SourceSequence",
				refTick.ullIngressSequence,
				p_refError))
		{
			return false;
		}
		refAllRates->aArrayValue.push_back(refRate);
	}

	// 第四步：按固定实体顺序应用全局游标，未请求的实体不占用游标空间。
	std::uint64_t ullVisited = 0;
	std::uint64_t ullEmitted = 0;
	auto AppendEntity = [&](std::uint64_t p_ullMask,
		const ST_PLUGIN_BINARY_VALUE& p_refSource,
		ST_PLUGIN_BINARY_VALUE& p_refTarget)
	{
		if ((ullEntityMask & p_ullMask) != 0)
		{
			AppendSnapshotPage(p_refSource, ullCursor,
				ullLimit, ullVisited, ullEmitted,
				p_refTarget);
		}
	};
	AppendEntity(EN_DERIVE_STATE_ENTITY_ACCOUNT,
		*pAccounts, *refAccounts);
	AppendEntity(EN_DERIVE_STATE_ENTITY_POSITION,
		*pPositions, *refPositions);
	AppendEntity(EN_DERIVE_STATE_ENTITY_SYMBOL,
		*pSymbols, *refSymbols);
	AppendEntity(EN_DERIVE_STATE_ENTITY_GROUP,
		*pGroups, *refGroups);
	AppendEntity(EN_DERIVE_STATE_ENTITY_RATE,
		*refAllRates, *refRates);
	if (ullCursor > ullVisited)
	{
		p_refError =
			"QUERY_DERIVE_SNAPSHOT_CURSOR_INVALID: Cursor exceeds authoritative record count";
		return false;
	}

	// 第五步：SnapshotId 覆盖全部权威实体、Quote 代次和水位，跨页变化会迫使 Derive 整轮重试。
	std::uint64_t ullSnapshotId = 1469598103934665603ULL;
	if (!HashBinaryValue(*refState, ullSnapshotId, 0) ||
		!HashBinaryValue(*refAllRates, ullSnapshotId, 0))
	{
		p_refError =
			"QUERY_DERIVE_SNAPSHOT_HASH_FAILED: authoritative value tree is invalid";
		return false;
	}
	HashBytes(ullSnapshotId, &stStatus.ullSourceEpoch,
		sizeof(stStatus.ullSourceEpoch));
	HashBytes(ullSnapshotId, &stStatus.ullHighWatermark,
		sizeof(stStatus.ullHighWatermark));
	const std::uint64_t ullNextCursor =
		ullCursor + ullEmitted;
	if (!AddInt64(*p_refData, "Version",
			llVersion, p_refError) ||
		!AddInt64(*p_refData, "No",
			llNo, p_refError) ||
		!AddUInt64(*p_refData, "EntityMask",
			ullEntityMask, p_refError) ||
		!AddUInt64(*p_refData, "SnapshotId",
			ullSnapshotId, p_refError) ||
		!AddUInt64(*p_refData, "EventSequence",
			stStatus.ullHighWatermark, p_refError) ||
		!AddUInt64(*p_refData, "NextCursor",
			ullNextCursor, p_refError) ||
		!AddBool(*p_refData, "HasMore",
			ullNextCursor < ullVisited, p_refError) ||
		!AddField(*p_refData, "Accounts",
			refAccounts, p_refError) ||
		!AddField(*p_refData, "Positions",
			refPositions, p_refError) ||
		!AddField(*p_refData, "Symbols",
			refSymbols, p_refError) ||
		!AddField(*p_refData, "Groups",
			refGroups, p_refError) ||
		!AddField(*p_refData, "Rates",
			refRates, p_refError))
	{
		return false;
	}
	return true;
}

bool CMtQueryDispatcher::HandleMt5ProfitGroupQuote(
	const ST_PLUGIN_BINARY_VALUE& p_refRequest,
	std::int32_t& p_refCode, std::string& p_refMessage,
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& p_refData,
	std::string& p_refError) const
{
	p_refCode = EN_TERMINAL_ERROR_OK;
	p_refMessage = "OK";
	p_refError.clear();
	p_refData = MakeValue(EN_PLUGIN_BINARY_VALUE_OBJECT, p_refError);
	if (!p_refData || m_pNodeManager == nullptr)
	{
		if (p_refError.empty())
		{
			p_refError =
				"QUERY_MT5_GROUP_QUOTE_NODE_MANAGER_MISSING";
		}
		return false;
	}
	if (p_refRequest.enType != EN_PLUGIN_BINARY_VALUE_OBJECT)
	{
		p_refCode = EN_TERMINAL_ERROR_INVALID_REQUEST;
		p_refMessage =
			"QUERY_MT5_GROUP_QUOTE_REQUEST_INVALID: root must be an object";
		return true;
	}

	// 第一步：内部校准请求只允许四个固定字段，重复和额外字段都不能进入 MT SDK。
	static const char* const s_aAllowed[] =
	{
		"Version", "No", "Group", "Symbol"
	};
	std::set<std::string> setField;
	for (const ST_PLUGIN_BINARY_FIELD& refField :
		p_refRequest.aObjectField)
	{
		const bool bAllowed = std::find(
			s_aAllowed, s_aAllowed + _countof(s_aAllowed),
			refField.strName) !=
			s_aAllowed + _countof(s_aAllowed);
		if (!bAllowed || !setField.insert(refField.strName).second)
		{
			p_refCode = EN_TERMINAL_ERROR_INVALID_REQUEST;
			p_refMessage =
				"QUERY_MT5_GROUP_QUOTE_REQUEST_INVALID: duplicate or unknown field=" +
				refField.strName;
			return true;
		}
	}
	std::int64_t llVersion = 0;
	std::int64_t llNo = 0;
	std::string strGroup;
	std::string strSymbol;
	if (setField.size() != _countof(s_aAllowed) ||
		!ReadInt64(p_refRequest, "Version", 0,
			llVersion, p_refError) ||
		!ReadInt64(p_refRequest, "No", 0,
			llNo, p_refError) ||
		!ReadString(p_refRequest, "Group",
			strGroup, p_refError) ||
		!ReadString(p_refRequest, "Symbol",
			strSymbol, p_refError) ||
		llVersion != 5 || llNo <= 0 ||
		llNo > (std::numeric_limits<std::int32_t>::max)() ||
		strGroup.empty() || strGroup.size() > 128U ||
		strSymbol.empty() || strSymbol.size() > 128U)
	{
		p_refCode = EN_TERMINAL_ERROR_INVALID_REQUEST;
		p_refMessage = p_refError.empty() ?
			"QUERY_MT5_GROUP_QUOTE_REQUEST_INVALID: Version=5, positive No, Group and Symbol are required" :
			p_refError;
		p_refError.clear();
		return true;
	}

	// 第二步：查询前后复核 Query 本地时间代次；过期或切换结果不得返回给 Derive。
	ST_MT_QUERY_TIME_CONTEXT stTimeBefore;
	if (!m_pNodeManager->GetTimeContext(5U,
			static_cast<std::int32_t>(llNo), stTimeBefore,
			p_refError) || !stTimeBefore.bReady ||
		stTimeBefore.llValidUntilUtcMs <= GetNowMs())
	{
		p_refCode = EN_TERMINAL_ERROR_MT_SYSTEM_NOT_READY;
		p_refMessage =
			"MT_SYSTEM_NOT_READY: 1169 time authority is missing or stale";
		p_refError.clear();
		return true;
	}
	ST_MT_QUERY_EXECUTION_RESULT stResult;
	bool bTimeGenerationCurrent = false;
	if (!m_pNodeManager->Execute(5U,
			static_cast<std::int32_t>(llNo),
			EN_PLUGIN_FUNC_QUERY_MT5_PROFIT_GROUP_QUOTE,
			p_refRequest, stResult, p_refError,
			&bTimeGenerationCurrent) || !stResult.refData)
	{
		if (p_refError.empty())
		{
			p_refError =
				"QUERY_MT5_GROUP_QUOTE_EXECUTION_FAILED: adapter returned no result";
		}
		return false;
	}
	ST_MT_QUERY_TIME_CONTEXT stTimeAfter;
	if (!bTimeGenerationCurrent ||
		!m_pNodeManager->GetTimeContext(5U,
			static_cast<std::int32_t>(llNo), stTimeAfter,
			p_refError) ||
		!IsMtQueryTimeContextSameGeneration(
			stTimeBefore, stTimeAfter))
	{
		p_refCode = EN_TERMINAL_ERROR_MT_SYSTEM_NOT_READY;
		p_refMessage =
			"MT_SYSTEM_NOT_READY: 1169 time generation changed during TickLast";
		p_refError.clear();
		return true;
	}
	p_refCode = stResult.iCode;
	p_refMessage = stResult.strMessage.empty() ?
		(stResult.iCode == EN_TERMINAL_ERROR_OK ?
			"OK" : "MT5_GROUP_QUOTE_QUERY_FAILED") :
		stResult.strMessage;
	p_refData = stResult.refData;
	return true;
}

bool CMtQueryDispatcher::HandleM1Backfill(
	const ST_QUERY_M1_BACKFILL_REQUEST& p_refRequest,
	ST_QUERY_M1_BACKFILL_RESPONSE& p_refResponse,
	std::string& p_refError) const
{
	p_refResponse = ST_QUERY_M1_BACKFILL_RESPONSE();
	p_refResponse.usPlatformVersion =
		p_refRequest.usPlatformVersion;
	p_refResponse.iSourceNo = p_refRequest.iSourceNo;
	p_refResponse.uiServerDate = p_refRequest.uiServerDate;
	p_refResponse.strSymbol = p_refRequest.strSymbol;
	p_refResponse.llConfirmedFromMinute = p_refRequest.llFromMinute;
	p_refResponse.llConfirmedToMinute = p_refRequest.llToMinute;
	p_refResponse.ullTimeAuthorityEpoch =
		p_refRequest.ullTimeAuthorityEpoch;
	p_refResponse.ullTimeGeneration =
		p_refRequest.ullTimeGeneration;
	p_refError.clear();
	if (m_pNodeManager == nullptr)
	{
		p_refError =
			"QUERY_M1_BACKFILL_NODE_MANAGER_MISSING";
		return false;
	}

	// 第一步：回源前必须精确匹配 Derive 传入的时间代次，禁止使用当前 offset 猜测旧请求。
	ST_MT_QUERY_TIME_CONTEXT stTimeBefore;
	const std::int64_t llNowMs = GetNowMs();
	if (!m_pNodeManager->GetTimeContext(
			p_refRequest.usPlatformVersion,
			p_refRequest.iSourceNo, stTimeBefore,
			p_refError) || !stTimeBefore.bReady ||
		stTimeBefore.llValidUntilUtcMs <= llNowMs ||
		stTimeBefore.ullAuthorityEpoch !=
			p_refRequest.ullTimeAuthorityEpoch ||
		stTimeBefore.ullGeneration !=
			p_refRequest.ullTimeGeneration)
	{
		p_refResponse.enState =
			EN_DERIVE_SERVICE_STATE_BOOTSTRAPPING;
		p_refResponse.iCode =
			EN_TERMINAL_ERROR_MT_SYSTEM_NOT_READY;
		p_refResponse.strMessage =
			"MT_SYSTEM_NOT_READY: 1168 time authority is missing, stale or changed";
		p_refError.clear();
		return true;
	}
	std::int64_t llRequestFrom = p_refRequest.llFromMinute;
	std::int64_t llRequestTo = p_refRequest.llToMinute;
	std::int64_t llServerFrom = 0;
	std::int64_t llServerTo = 0;
	if (p_refRequest.bFullServerDay)
	{
		std::int64_t llServerNextDay = 0;
		if (!BuildBrokerDayServerRange(p_refRequest.uiServerDate,
				llServerFrom, llServerNextDay) ||
			!CMtQueryTimeZone::ServerEpochToUtc(
				stTimeBefore.strTimeZoneId, stTimeBefore.aTransition,
				llServerFrom, llRequestFrom, p_refError) ||
			!CMtQueryTimeZone::ServerEpochToUtc(
				stTimeBefore.strTimeZoneId, stTimeBefore.aTransition,
				llServerNextDay, llRequestTo, p_refError))
		{
			p_refResponse.enState = EN_DERIVE_SERVICE_STATE_DEGRADED;
			p_refResponse.iCode = EN_TERMINAL_ERROR_INVALID_REQUEST;
			p_refResponse.strMessage =
				"QUERY_M1_BACKFILL_SERVER_DATE_INVALID: Broker day cannot be resolved";
			p_refError.clear();
			return true;
		}
		llRequestTo -= 60LL;
		llServerTo = llServerNextDay - 60LL;
		p_refResponse.llConfirmedFromMinute = llRequestFrom;
		p_refResponse.llConfirmedToMinute = llRequestTo;
	}
	else
	{
		// 当前日主动回填允许 From=0，由 Query 使用持久化时区历史解析 Broker 日起点。
		// 这样 Derive 无需复制时区转换逻辑，DST 切换日也不会按固定 offset 猜测边界。
		if (llRequestFrom == 0)
		{
			std::int64_t llServerNextDay = 0;
			if (!BuildBrokerDayServerRange(p_refRequest.uiServerDate,
					llServerFrom, llServerNextDay) ||
				!CMtQueryTimeZone::ServerEpochToUtc(
					stTimeBefore.strTimeZoneId,
					stTimeBefore.aTransition, llServerFrom,
					llRequestFrom, p_refError))
			{
				p_refResponse.enState = EN_DERIVE_SERVICE_STATE_DEGRADED;
				p_refResponse.iCode = EN_TERMINAL_ERROR_INVALID_REQUEST;
				p_refResponse.strMessage =
					"QUERY_M1_BACKFILL_SERVER_DATE_INVALID: partial Broker day start cannot be resolved";
				p_refError.clear();
				return true;
			}
			p_refResponse.llConfirmedFromMinute = llRequestFrom;
		}
		else if (!CMtQueryTimeZone::UtcToServerEpoch(
				stTimeBefore.strTimeZoneId, stTimeBefore.aTransition,
				llRequestFrom, llServerFrom, p_refError))
		{
			return false;
		}
		if (!CMtQueryTimeZone::UtcToServerEpoch(
				stTimeBefore.strTimeZoneId, stTimeBefore.aTransition,
				llRequestTo, llServerTo, p_refError))
		{
			return false;
		}
	}
	std::uint32_t uiFromDate = 0;
	std::uint32_t uiToDate = 0;
	std::uint32_t uiFromMinute = 0;
	std::uint32_t uiToMinute = 0;
	if (!ResolveBrokerDateMinute(llServerFrom, uiFromDate,
			uiFromMinute) ||
		!ResolveBrokerDateMinute(llServerTo, uiToDate,
			uiToMinute) ||
		uiFromDate != p_refRequest.uiServerDate ||
		uiToDate != p_refRequest.uiServerDate)
	{
		p_refResponse.enState = EN_DERIVE_SERVICE_STATE_DEGRADED;
		p_refResponse.iCode = EN_TERMINAL_ERROR_INVALID_REQUEST;
		p_refResponse.strMessage =
			"QUERY_M1_BACKFILL_SERVER_DATE_MISMATCH: UTC range does not belong to requested Broker date";
		p_refError.clear();
		return true;
	}
	p_refResponse.bFullServerDay =
		uiFromMinute == 0U && uiToMinute == 1439U;

	// 第二步：强制周期为 M1 并直接借用 HISTORY_QUERY Adapter；本路径不检查本地 coverage。
	ST_MT_QUERY_BAR_COVERAGE_RANGE stRange;
	stRange.llFrom = llRequestFrom;
	stRange.llTo = llRequestTo;
	ST_MT_QUERY_EXECUTION_RESULT stResult;
	std::vector<ST_MT_QUERY_BAR> aBars;
	bool bTimeGenerationCurrent = false;
	if (!ExecuteBarRange(m_pNodeManager,
			p_refRequest.usPlatformVersion,
			p_refRequest.iSourceNo, p_refRequest.strSymbol,
			1, stRange, stResult, aBars,
			bTimeGenerationCurrent, p_refError))
	{
		return false;
	}
	if (stResult.iCode != EN_TERMINAL_ERROR_OK)
	{
		p_refResponse.enState =
			EN_DERIVE_SERVICE_STATE_DEGRADED;
		p_refResponse.iCode = stResult.iCode;
		p_refResponse.strMessage = stResult.strMessage;
		return true;
	}

	// 第三步：Adapter 必须明确确认完整闭区间；成功空区间同样是权威结果。
	std::vector<ST_MT_QUERY_BAR_COVERAGE_RANGE> aConfirmed;
	for (const ST_MT_QUERY_CONFIRMED_RANGE& refRange :
		stResult.aConfirmedRange)
	{
		ST_MT_QUERY_BAR_COVERAGE_RANGE stConfirmed;
		stConfirmed.llFrom = refRange.llFrom;
		stConfirmed.llTo = refRange.llTo;
		aConfirmed.push_back(stConfirmed);
	}
	std::vector<ST_MT_QUERY_BAR_COVERAGE_RANGE> aNormalized;
	CMtQueryBarHistoryStore::MergeCoverage(aNormalized,
		aConfirmed);
	std::vector<ST_MT_QUERY_BAR_COVERAGE_RANGE> aMissing;
	CMtQueryBarHistoryStore::FindUncovered(aNormalized,
		llRequestFrom, llRequestTo,
		aMissing);
	ST_MT_QUERY_TIME_CONTEXT stTimeAfter;
	if (!bTimeGenerationCurrent || !aMissing.empty() ||
		!m_pNodeManager->GetTimeContext(
			p_refRequest.usPlatformVersion,
			p_refRequest.iSourceNo, stTimeAfter,
			p_refError) ||
		!IsMtQueryTimeContextSameGeneration(
			stTimeBefore, stTimeAfter))
	{
		p_refResponse.enState =
			EN_DERIVE_SERVICE_STATE_BOOTSTRAPPING;
		p_refResponse.iCode =
			EN_TERMINAL_ERROR_MT_SYSTEM_NOT_READY;
		p_refResponse.strMessage = aMissing.empty() ?
			"MT_SYSTEM_NOT_READY: 1168 time generation changed during ChartRequest" :
			"MT_SYSTEM_NOT_READY: 1168 MT source did not confirm the complete range";
		p_refError.clear();
		return true;
	}
	if (aBars.size() > p_refRequest.uiMaxBars)
	{
		p_refResponse.enState =
			EN_DERIVE_SERVICE_STATE_DEGRADED;
		p_refResponse.iCode =
			EN_TERMINAL_ERROR_INVALID_REQUEST;
		p_refResponse.strMessage =
			"QUERY_M1_BACKFILL_BAR_LIMIT: MT result exceeds MaxBars";
		return true;
	}

	// 第四步：历史记录没有实时 SourceEpoch/Sequence；Derive 合并时让在线 Tick 覆盖重叠分钟。
	for (const ST_MT_QUERY_BAR& refBar : aBars)
	{
		ST_DERIVE_M1_BAR stBar;
		stBar.usPlatformVersion =
			p_refRequest.usPlatformVersion;
		stBar.iSourceNo = p_refRequest.iSourceNo;
		stBar.strSymbol = p_refRequest.strSymbol;
		stBar.llMinute = refBar.llDateTime;
		stBar.uiServerDate = p_refRequest.uiServerDate;
		stBar.dOpen = refBar.dOpen;
		stBar.dHigh = refBar.dHigh;
		stBar.dLow = refBar.dLow;
		stBar.dClose = refBar.dClose;
		stBar.ullTickVolume = refBar.ullTickVolume;
		stBar.ullRealVolume = refBar.ullRealVolume;
		p_refResponse.aBar.push_back(stBar);
		std::int64_t llServerMinute = 0;
		if (!CMtQueryTimeZone::UtcToServerEpoch(
				stTimeAfter.strTimeZoneId,
				stTimeAfter.aTransition,
				refBar.llDateTime, llServerMinute,
				p_refError))
		{
			return false;
		}
		p_refResponse.aServerMinute.push_back(
			llServerMinute);
	}
	p_refResponse.enState = EN_DERIVE_SERVICE_STATE_READY;
	p_refResponse.iCode = EN_TERMINAL_ERROR_OK;
	p_refResponse.strMessage = "OK";
	p_refResponse.bConfirmedEmpty =
		p_refResponse.aBar.empty();
	p_refResponse.ullTimeAuthorityEpoch =
		stTimeAfter.ullAuthorityEpoch;
	p_refResponse.ullTimeGeneration =
		stTimeAfter.ullGeneration;
	return true;
}

bool CMtQueryDispatcher::HandleMtQuery(
	std::int64_t p_llFuncId,
	const ST_PLUGIN_BINARY_VALUE& p_refRequest,
	std::int32_t& p_refCode,
	std::string& p_refMessage,
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& p_refData,
	std::string& p_refError) const
{
	std::int64_t llVersion = 5;
	std::int64_t llNo = 1;
	if (!ReadInt64(p_refRequest, "Version", 5,
			llVersion, p_refError) ||
		!ReadInt64(p_refRequest, "No", 1,
			llNo, p_refError) ||
		(llVersion != 4 && llVersion != 5) ||
		llNo <= 0 || llNo >
			(std::numeric_limits<std::int32_t>::max)())
	{
		if (p_refError.empty())
		{
			p_refError =
				"QUERY_MT_ROUTE_PARAMETER_INVALID: Version must be 4/5 and No must be positive";
		}
		return false;
	}
	ST_MT_QUERY_EXECUTION_RESULT stResult;
	if (!m_pNodeManager->Execute(
			static_cast<std::uint16_t>(llVersion),
			static_cast<std::int32_t>(llNo),
			p_llFuncId, p_refRequest,
			stResult, p_refError) || !stResult.refData)
	{
		if (p_refError.empty())
		{
			p_refError =
				"QUERY_MT_EXECUTION_FAILED: adapter returned no result";
		}
		return false;
	}
	p_refCode = stResult.iCode;
	p_refMessage = stResult.strMessage.empty() ?
		(stResult.iCode == EN_TERMINAL_ERROR_OK ?
			"OK" : "MT_QUERY_FAILED") :
		stResult.strMessage;
	p_refData = stResult.refData;
	return true;
}

bool CMtQueryDispatcher::HandleClientData(
	std::int64_t p_llFuncId, std::int64_t p_llRouteCode,
	const ST_PLUGIN_BINARY_VALUE& p_refRequest,
	std::int32_t& p_refCode, std::string& p_refMessage,
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& p_refData,
	std::string& p_refError) const
{
	const bool bRouteValid =
		(p_llFuncId == EN_PLUGIN_FUNC_QUERY_WATCHLIST &&
			(p_llRouteCode == EN_PLUGIN_WATCHLIST_ROUTE_SYMBOLS ||
				p_llRouteCode ==
					EN_PLUGIN_WATCHLIST_ROUTE_SECTIONS)) ||
		(p_llFuncId == EN_PLUGIN_FUNC_QUERY_CHART &&
			p_llRouteCode >= EN_PLUGIN_CHART_ROUTE_DRAWINGS &&
			p_llRouteCode <= EN_PLUGIN_CHART_ROUTE_CONFIG);
	if (!bRouteValid)
	{
		p_refCode =
			EN_TERMINAL_ERROR_CLIENT_DATA_API;
		p_refMessage =
			"CLIENT_DATA_ROUTE_INVALID: FuncId and RouteCode combination is not registered";
	}
	else if (m_pClientDataService != nullptr)
	{
		return m_pClientDataService->Dispatch(p_llFuncId,
			p_llRouteCode, p_refRequest,
			p_refCode, p_refMessage, p_refData, p_refError);
	}
	else if (!m_pConfig->stDatabase.bEnable)
	{
		p_refCode =
			EN_TERMINAL_ERROR_POSTGRES_DISABLED;
		p_refMessage =
			"POSTGRES_POOL_DISABLED: MtQueryService PostgreSQL enable=0";
	}
	else
	{
		p_refCode =
			EN_TERMINAL_ERROR_POSTGRES_NOT_INITIALIZED;
		p_refMessage =
			"POSTGRES_POOL_NOT_INITIALIZED: ClientData repository has not completed startup";
	}
	p_refData = MakeValue(EN_PLUGIN_BINARY_VALUE_OBJECT,
		p_refError);
	return p_refData != nullptr;
}
