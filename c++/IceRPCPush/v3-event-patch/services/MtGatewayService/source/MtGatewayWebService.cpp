#include "MtGatewayWebService.h"

#include "CodeMsg.h"
#include "DeriveBinaryProtocol.h"
#include "MtGatewayApiCatalog.h"
#include "MtGatewayAuthService.h"
#include "MtGatewayBackendPool.h"
#include "MtGatewayRequestAdapter.h"
#include "MtGatewayWebProtocol.h"
#include "QuoteBinaryProtocol.h"
#include "QuoteSnapshotBinaryProtocol.h"

#include <atomic>
#include <charconv>
#include <chrono>
#include <cstring>
#include <exception>
#include <limits>
#include <memory>
#include <new>
#include <sstream>

namespace
{
	static const std::size_t WEB_TRACE_ID_MAX_LENGTH = 128U;

	// 初始快照异步回调保存连接代次、Topic 和查询协议，晚到回调不会命中新连接。
	struct ST_MT_GATEWAY_SNAPSHOT_CALLBACK
	{
		ST_MT_GATEWAY_WEB_TARGET stTarget;         // 订阅发起时的连接代次。
		EN_MT_GATEWAY_WEB_TOPIC enTopic;           // 本次快照对应的单个 Topic。
		std::int32_t iVersion;                     // 快照来源平台版本。
		std::int32_t iNo;                          // 快照来源节点编号。
		std::uint32_t uiAttempt;                   // 当前订阅快照尝试号，旧回调据此失效。
		ST_MT_GATEWAY_API_DESCRIPTOR stApi;        // 查询应答解码所需的领域信息。
		bool bOpenPriceBootstrap;                  // true 表示 Quote 的前置 1185，不直接向客户端发送。
		std::set<std::int64_t> setLogin;           // 串联后续 1122 时恢复原订阅账号条件。
		std::set<std::string> setSymbol;           // 串联后续 1122 时恢复原订阅品种条件。

		ST_MT_GATEWAY_SNAPSHOT_CALLBACK()
			: stTarget()
			, enTopic(EN_MT_GATEWAY_WEB_TOPIC_NONE)
			, iVersion(0)
			, iNo(0)
			, uiAttempt(1)
			, stApi()
			, bOpenPriceBootstrap(false)
			, setLogin()
			, setSymbol()
		{
		}
	};

	// 返回当前系统 Unix 毫秒时间戳。
	std::int64_t GetTimestampMs()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
	}

	// 为当前 Gateway 进程生成非零启动代次，租约键据此隔离快速重启的旧 Revision。
	std::uint64_t GenerateGatewayEpoch()
	{
		static std::atomic<std::uint64_t> s_ullEpoch(
			static_cast<std::uint64_t>(
				GetTimestampMs()) << 16U);
		const std::uint64_t ullValue =
			s_ullEpoch.fetch_add(1) + 1U;
		return ullValue != 0 ? ullValue : 1U;
	}

	// 构造固定 Web Code/Msg/FuncId/Data 包装。
	std::string BuildWebResponse(std::int32_t p_iFuncId, int p_iCode,
		const std::string& p_refMessage, const nlohmann::json& p_refData)
	{
		const int iExternalCode = NormalizeV2ExternalErrorCode(p_iCode);
		const char* pExternalMessage = GetExternalErrorMsg(iExternalCode);
		nlohmann::json clResponse = nlohmann::json::object();
		clResponse["FuncId"] = p_iFuncId;
		clResponse["Code"] = iExternalCode;
		clResponse["Msg"] = pExternalMessage != nullptr ?
			pExternalMessage : "unknown error";
		clResponse["Data"] = p_refData;
		return clResponse.dump();
	}

	// 严格读取 int64 数值，拒绝浮点、溢出和其他 JSON 类型。
	bool ReadInt64(const nlohmann::json& p_refObject, const char* p_szName,
		std::int64_t& p_refValue, bool p_bRequired, std::string& p_refError)
	{
		p_refValue = 0;
		const nlohmann::json::const_iterator it = p_refObject.find(p_szName);
		if (it == p_refObject.end())
		{
			if (p_bRequired)
			{
				p_refError = std::string("INVALID_REQUEST: missing integer field ") +
					p_szName;
				return false;
			}
			return true;
		}
		if (it->is_number_unsigned())
		{
			const std::uint64_t ullValue = it->get<std::uint64_t>();
			if (ullValue <= static_cast<std::uint64_t>(
				(std::numeric_limits<std::int64_t>::max)()))
			{
				p_refValue = static_cast<std::int64_t>(ullValue);
				return true;
			}
		}
		else if (it->is_number_integer())
		{
			p_refValue = it->get<std::int64_t>();
			return true;
		}
		p_refError = std::string("INVALID_REQUEST: field must be int64: ") +
			p_szName;
		return false;
	}

	// 严格读取 uint64 数值，允许正整型 JSON 表示。
	bool ReadUInt64(const nlohmann::json& p_refObject, const char* p_szName,
		std::uint64_t& p_refValue, bool p_bRequired, std::string& p_refError)
	{
		p_refValue = 0;
		const nlohmann::json::const_iterator it = p_refObject.find(p_szName);
		if (it == p_refObject.end())
		{
			if (p_bRequired)
			{
				p_refError = std::string("INVALID_REQUEST: missing unsigned field ") +
					p_szName;
				return false;
			}
			return true;
		}
		if (it->is_number_unsigned())
		{
			p_refValue = it->get<std::uint64_t>();
			return true;
		}
		if (it->is_number_integer())
		{
			const std::int64_t llValue = it->get<std::int64_t>();
			if (llValue >= 0)
			{
				p_refValue = static_cast<std::uint64_t>(llValue);
				return true;
			}
		}
		else if (it->is_string())
		{
			const std::string strValue = it->get<std::string>();
			std::uint64_t ullValue = 0;
			const std::from_chars_result stResult = std::from_chars(
				strValue.data(), strValue.data() + strValue.size(), ullValue);
			if (!strValue.empty() && stResult.ec == std::errc() &&
				stResult.ptr == strValue.data() + strValue.size())
			{
				p_refValue = ullValue;
				return true;
			}
		}
		p_refError = std::string("INVALID_REQUEST: field must be uint64: ") +
			p_szName;
		return false;
	}

	// 读取可选字符串，字段存在但类型非法时返回失败。
	bool ReadString(const nlohmann::json& p_refObject, const char* p_szName,
		std::string& p_refValue, bool p_bRequired, std::string& p_refError)
	{
		p_refValue.clear();
		const nlohmann::json::const_iterator it = p_refObject.find(p_szName);
		if (it == p_refObject.end())
		{
			if (p_bRequired)
			{
				p_refError = std::string("INVALID_REQUEST: missing string field ") +
					p_szName;
				return false;
			}
			return true;
		}
		if (!it->is_string())
		{
			p_refError = std::string("INVALID_REQUEST: field must be string: ") +
				p_szName;
			return false;
		}
		p_refValue = it->get<std::string>();
		return true;
	}

	// 将逗号分隔字符串拆成非空项，兼容 V2 SymbolIds 文本格式。
	void SplitCommaSeparated(const std::string& p_refText,
		std::set<std::string>& p_refValues)
	{
		std::stringstream clStream(p_refText);
		std::string strItem;
		while (std::getline(clStream, strItem, ','))
		{
			if (!strItem.empty())
			{
				p_refValues.insert(strItem);
			}
		}
	}

	// 将 1211 Quote Binary 转换为 V2 Web QUOTE Data 字段。
	// 输入已经由 ClientData 解码，函数继续校验两层来源字段，禁止错误节点数据进入订阅分发。
	bool DecodeQuoteForWeb(
		const ST_PLUGIN_NOTIFY_META& p_refMeta,
		const ST_CLIENT_DATA_BINARY_EVENT& p_refEvent,
		double p_dM1OpenPrice,
		nlohmann::json& p_refData,
		std::uint64_t& p_refSourceEpoch,
		std::string& p_refError)
	{
		p_refData = nlohmann::json::object();
		p_refSourceEpoch = 0;
		if (p_refMeta.usSourcePlugin !=
				EN_PLUGIN_ID_MT_EVENT_SERVICE ||
			p_refMeta.usNotifyMode !=
				EN_PLUGIN_NOTIFY_MODE_BEST_EFFORT ||
			p_refMeta.usNotifyAction !=
				EN_PLUGIN_NOTIFY_ACTION_UPDATED)
		{
			p_refError =
				"WEB_QUOTE_META_INVALID: source must be MtEventService with BEST_EFFORT and UPDATED";
			return false;
		}
		if (!p_refEvent.refData ||
			p_refEvent.refData->enType !=
				EN_PLUGIN_BINARY_VALUE_BYTES)
		{
			p_refError =
				"WEB_QUOTE_DATA_INVALID: Quote ClientData must contain BYTES";
			return false;
		}

		ST_QUOTE_BINARY_TICK stTick;
		if (!DecodeQuoteBinaryTick(
			p_refEvent.refData->aByteValue.data(),
			p_refEvent.refData->aByteValue.size(),
			stTick, p_refError))
		{
			p_refError = "WEB_QUOTE_DECODE_FAILED: " +
				p_refError;
			return false;
		}
		if (p_refEvent.ullTopic !=
				EN_CLIENT_DATA_BINARY_TOPIC_QUOTE ||
			p_refEvent.iSourceVersion !=
				stTick.usPlatformVersion ||
			p_refEvent.iSourceNo != stTick.iSourceNo ||
			p_refEvent.strSymbol != stTick.strSymbol ||
			p_refEvent.ullSequence !=
				stTick.ullIngressSequence ||
			p_refEvent.llTimestampMs !=
				stTick.llIngressTimeMs)
		{
			p_refError =
				"WEB_QUOTE_METADATA_MISMATCH: ClientData and Quote fields are inconsistent";
			return false;
		}

		// V2 行情推送的公共字段保持原名称；Volume 表示当前 Tick 数量，固定为 1。
		p_refData["Version"] =
			stTick.usPlatformVersion;
		p_refData["No"] = stTick.iSourceNo;
		p_refData["Time"] = stTick.llServerTime;
		p_refData["SymbolId"] = stTick.strSymbol;
		p_refData["Bid"] = stTick.dBid;
		p_refData["Ask"] = stTick.dAsk;
		p_refData["Volume"] = 1;
		p_refData["SourceEpoch"] = stTick.ullSourceEpoch;
		p_refData["Sequence"] = stTick.ullIngressSequence;
		const double dOpenPrice = p_dM1OpenPrice > 0.0 ?
			p_dM1OpenPrice : 0.0;
		p_refData["OpenPrice"] = dOpenPrice;
		p_refData["ClosePrice"] = 0.0;
		p_refData["LowPrice"] = 0.0;
		p_refData["HighPrice"] = 0.0;
		p_refData["PrevClosedPrice"] = 0.0;
		p_refSourceEpoch = stTick.ullSourceEpoch;
		if (stTick.usPlatformVersion == 5)
		{
			static const std::uint64_t MT5_TICK_FLAG_VOLUME =
				0x00000010ULL;
			p_refData["Last"] = stTick.dLast;
			p_refData["DateTimeMsc"] =
				stTick.llServerTimeMsc;
			p_refData["Flags"] = stTick.ullFlags;
			p_refData["VolumeExt"] =
				(stTick.ullFlags & MT5_TICK_FLAG_VOLUME) != 0 ?
				stTick.ullVolumeExt : 0;
		}
		return true;
	}

	// 将单根权威 M1 转换为与 /bars M1 一致的 Web 字段；latest_quote 不参与本转换。
	nlohmann::json BuildM1BarForWeb(const ST_DERIVE_M1_BAR& p_refBar)
	{
		nlohmann::json clBar = nlohmann::json::object();
		clBar["Version"] = p_refBar.usPlatformVersion;
		clBar["No"] = p_refBar.iSourceNo;
		clBar["SymbolId"] = p_refBar.strSymbol;
		clBar["Time"] = p_refBar.llMinute;
		clBar["Open"] = p_refBar.dOpen;
		clBar["High"] = p_refBar.dHigh;
		clBar["Low"] = p_refBar.dLow;
		clBar["Close"] = p_refBar.dClose;
		clBar["Volume"] = p_refBar.ullTickVolume;
		clBar["VolumeExt"] = p_refBar.ullRealVolume;
		clBar["SourceEpoch"] = p_refBar.ullSourceEpoch;
		clBar["FirstSequence"] = p_refBar.ullFirstSequence;
		clBar["LastSequence"] = p_refBar.ullLastSequence;
		return clBar;
	}

	// Profit 快照可按 Login 过滤；0 表示完整快照，实时 1251 使用单 Login 避免无关账号数据进入消息正文。
	nlohmann::json BuildProfitForWeb(
		const ST_DERIVE_PROFIT_SNAPSHOT_RESPONSE& p_refProfit,
		std::int64_t p_llLogin)
	{
		nlohmann::json clProfit = nlohmann::json::object();
		clProfit["State"] = static_cast<unsigned int>(p_refProfit.enState);
		clProfit["EventSequence"] = p_refProfit.ullEventSequence;
		clProfit["Accounts"] = nlohmann::json::array();
		for (const ST_DERIVE_ACCOUNT_PROFIT& refAccount :
			p_refProfit.aAccount)
		{
			if (p_llLogin > 0 && refAccount.llLogin != p_llLogin)
			{
				continue;
			}
			nlohmann::json clItem = nlohmann::json::object();
			clItem["Version"] = refAccount.usPlatformVersion;
			clItem["No"] = refAccount.iSourceNo;
			clItem["Login"] = refAccount.llLogin;
			clItem["StateVersion"] = refAccount.ullStateVersion;
			clItem["Balance"] = refAccount.dBalance;
			clItem["Credit"] = refAccount.dCredit;
			clItem["Profit"] = refAccount.dProfit;
			clItem["Floating"] = refAccount.dFloating;
			clItem["Equity"] = refAccount.dEquity;
			clItem["Margin"] = refAccount.dMargin;
			clItem["MarginFree"] = refAccount.dMarginFree;
			clItem["MarginLevel"] = refAccount.dMarginLevel;
			clItem["Timestamp"] = refAccount.llTimestampMs;
			clProfit["Accounts"].push_back(clItem);
		}
		clProfit["Positions"] = nlohmann::json::array();
		for (const ST_DERIVE_POSITION_PROFIT& refPosition :
			p_refProfit.aPosition)
		{
			if (p_llLogin > 0 && refPosition.llLogin != p_llLogin)
			{
				continue;
			}
			nlohmann::json clItem = nlohmann::json::object();
			clItem["Version"] = refPosition.usPlatformVersion;
			clItem["No"] = refPosition.iSourceNo;
			clItem["Login"] = refPosition.llLogin;
			clItem["Position"] = refPosition.ullPositionId;
			clItem["StateVersion"] = refPosition.ullStateVersion;
			clItem["Symbol"] = refPosition.strSymbol;
			clItem["Side"] = refPosition.usSide;
			clItem["Volume"] = refPosition.dVolume;
			clItem["OpenPrice"] = refPosition.dOpenPrice;
			clItem["CurrentPrice"] = refPosition.dCurrentPrice;
			clItem["Profit"] = refPosition.dProfit;
			clItem["Storage"] = refPosition.dStorage;
			clItem["Commission"] = refPosition.dCommission;
			clItem["Tax"] = refPosition.dTax;
			clItem["Timestamp"] = refPosition.llTimestampMs;
			clProfit["Positions"].push_back(clItem);
		}
		return clProfit;
	}

	// 读取 Login 和 Logins，失败时返回 V2 对应字段错误码。
	bool ReadLogins(const nlohmann::json& p_refRequest,
		std::set<std::int64_t>& p_refLogins, int& p_refCode)
	{
		p_refLogins.clear();
		p_refCode = EN_TERMINAL_ERROR_OK;
		const nlohmann::json::const_iterator itLogin =
			p_refRequest.find("Login");
		if (itLogin != p_refRequest.end() && !itLogin->is_null())
		{
			std::int64_t llLogin = 0;
			if (itLogin->is_number_unsigned() &&
				itLogin->get<std::uint64_t>() <= static_cast<std::uint64_t>(
					(std::numeric_limits<std::int64_t>::max)()))
			{
				llLogin = static_cast<std::int64_t>(
					itLogin->get<std::uint64_t>());
			}
			else if (itLogin->is_number_integer())
			{
				llLogin = itLogin->get<std::int64_t>();
			}
			if (llLogin <= 0)
			{
				p_refCode = EN_V2_WEB_LOGIN_INVALID;
				return false;
			}
			p_refLogins.insert(llLogin);
		}
		const nlohmann::json::const_iterator itLogins =
			p_refRequest.find("Logins");
		if (itLogins != p_refRequest.end() && !itLogins->is_null())
		{
			if (!itLogins->is_array())
			{
				p_refCode = EN_V2_WEB_LOGINS_NOT_ARRAY;
				return false;
			}
			for (const nlohmann::json& refLogin : *itLogins)
			{
				std::int64_t llLogin = 0;
				if (refLogin.is_number_unsigned() &&
					refLogin.get<std::uint64_t>() <=
					static_cast<std::uint64_t>(
						(std::numeric_limits<std::int64_t>::max)()))
				{
					llLogin = static_cast<std::int64_t>(
						refLogin.get<std::uint64_t>());
				}
				else if (refLogin.is_number_integer())
				{
					llLogin = refLogin.get<std::int64_t>();
				}
				if (llLogin <= 0)
				{
					p_refCode = EN_V2_WEB_LOGINS_ELEMENT_INVALID;
					return false;
				}
				p_refLogins.insert(llLogin);
			}
		}
		return true;
	}

	// V2 的 SymbolId/SymbolIds 只接受字符串，SymbolIds 使用逗号分隔。
	bool ReadSymbols(const nlohmann::json& p_refRequest,
		std::set<std::string>& p_refSymbols, int& p_refCode)
	{
		p_refSymbols.clear();
		p_refCode = EN_TERMINAL_ERROR_OK;
		const nlohmann::json::const_iterator itSingle =
			p_refRequest.find("SymbolId");
		if (itSingle != p_refRequest.end() && !itSingle->is_null())
		{
			if (!itSingle->is_string())
			{
				p_refCode = EN_V2_WEB_SYMBOL_ID_INVALID;
				return false;
			}
			const std::string strSymbol = itSingle->get<std::string>();
			if (!strSymbol.empty())
			{
				p_refSymbols.insert(strSymbol);
			}
		}
		const nlohmann::json::const_iterator it =
			p_refRequest.find("SymbolIds");
		if (it == p_refRequest.end() || it->is_null())
		{
			return true;
		}
		if (!it->is_string())
		{
			p_refCode = EN_V2_WEB_SYMBOL_ID_INVALID;
			return false;
		}
		SplitCommaSeparated(it->get<std::string>(), p_refSymbols);
		return true;
	}

	// 将 Topic 映射到 MtQueryService 初始快照功能号。
	std::int64_t GetSnapshotFuncId(EN_MT_GATEWAY_WEB_TOPIC p_enTopic)
	{
		switch (p_enTopic)
		{
		case EN_MT_GATEWAY_WEB_TOPIC_QUOTE:
			return EN_PLUGIN_FUNC_QUOTE_SNAPSHOT;
		case EN_MT_GATEWAY_WEB_TOPIC_SYMBOL:
			return EN_PLUGIN_FUNC_QUERY_SYMBOLS;
		case EN_MT_GATEWAY_WEB_TOPIC_PROFIT:
			return EN_PLUGIN_FUNC_DERIVE_PROFIT_SNAPSHOT;
		case EN_MT_GATEWAY_WEB_TOPIC_USER:
			return EN_PLUGIN_FUNC_QUERY_ACCOUNTS;
		case EN_MT_GATEWAY_WEB_TOPIC_ORDER:
			return EN_PLUGIN_FUNC_QUERY_ORDERS;
		case EN_MT_GATEWAY_WEB_TOPIC_POSITION:
			return EN_PLUGIN_FUNC_QUERY_POSITIONS;
		case EN_MT_GATEWAY_WEB_TOPIC_DEAL:
			return EN_PLUGIN_FUNC_QUERY_DEALS;
		case EN_MT_GATEWAY_WEB_TOPIC_KLINE:
			return EN_PLUGIN_FUNC_DERIVE_M1_SNAPSHOT;
		default:
			return 0;
		}
	}
}

CMtGatewayWebService& CMtGatewayWebService::GetInstance()
{
	static CMtGatewayWebService s_clInstance;
	return s_clInstance;
}

CMtGatewayWebService::CMtGatewayWebService()
	: m_clMutex()
	, m_clCondition()
	, m_stConfig()
	, m_pServer(nullptr)
	, m_hCloudNetApi(nullptr)
	, m_pAuthService(nullptr)
	, m_pBackendPool(nullptr)
	, m_bConfigured(false)
	, m_bRunning(false)
	, m_bSweepStop(false)
	, m_bProfitDemandDirty(false)
	, m_iActiveCallbacks(0)
	, m_clSweepThread()
	, m_strGatewayInstance()
	, m_ullGatewayEpoch(0)
	, m_ullProfitRevision(0)
	, m_mapLastProfitDemand()
	, m_clQuoteSequenceMutex()
	, m_mapQuoteSequence()
	, m_mapQuoteInterval()
	, m_mapOpenPrice()
	, m_clSessionManager()
	, m_clPushDispatcher()
{
}

CMtGatewayWebService::~CMtGatewayWebService()
{
	Stop();
}

bool CMtGatewayWebService::SynchronizeConfig(
	const ST_MT_GATEWAY_WEB_CONFIG& p_refConfig, std::string& p_refError)
{
	std::lock_guard<std::mutex> clLock(m_clMutex);
	p_refError.clear();
	if (m_bRunning)
	{
		p_refError = "WEB_CONFIG_SYNC_REJECTED: WebSocket service is already running";
		return false;
	}
	m_stConfig = p_refConfig;
	{
		std::lock_guard<std::mutex> clSequenceLock(
			m_clQuoteSequenceMutex);
		m_mapQuoteInterval.clear();
		for (const ST_MT_GATEWAY_PUSH_POLICY& refPolicy :
			p_refConfig.aPushPolicy)
		{
			m_mapQuoteInterval[std::make_pair(
				static_cast<std::int32_t>(refPolicy.usVersion),
				refPolicy.iNo)] = refPolicy.uiQuoteMinIntervalMs;
		}
	}
	m_bConfigured = true;
	return true;
}

bool CMtGatewayWebService::Start(const std::string& p_refExeDirectory,
	const ST_LOG_CONFIG& p_refLogConfig, HCLOUD_NET_API p_hCloudNetApi,
	const CMtGatewayAuthService* p_pAuthService,
	CMtGatewayBackendPool* p_pBackendPool, std::string& p_refError)
{
	Stop();
	p_refError.clear();
	ST_MT_GATEWAY_WEB_CONFIG stConfig;
	{
		std::lock_guard<std::mutex> clLock(m_clMutex);
		if (!m_bConfigured || p_hCloudNetApi == nullptr ||
			p_pAuthService == nullptr || p_pBackendPool == nullptr)
		{
			p_refError = "WEB_START_INVALID_STATE: config, CloudNetDataApi, auth and backend pool are required";
			return false;
		}
		stConfig = m_stConfig;
	}
	if (!stConfig.bEnable)
	{
		return true;
	}

	ST_LOG_CONFIG stWebLog = p_refLogConfig;
	stWebLog.strName = stConfig.strLogName;
	if (!InitializeLog(CWebLog::GetInstance(), p_refExeDirectory,
		stWebLog, p_refError))
	{
		CWebLog::Release();
		return false;
	}
	CSocketServer* pServer = CreateWebSockInstance();
	if (pServer == nullptr)
	{
		CWebLog::Release();
		p_refError = "WEB_SERVER_CREATE_FAILED: CreateWebSockInstance returned null";
		return false;
	}
	m_clSessionManager.Reset(stConfig.uiMaxConnections,
		stConfig.uiSnapshotBufferCapacity);
	{
		std::lock_guard<std::mutex> clSequenceLock(
			m_clQuoteSequenceMutex);
		m_mapQuoteSequence.clear();
		m_mapOpenPrice.clear();
	}
	{
		std::lock_guard<std::mutex> clLock(m_clMutex);
		m_pServer = pServer;
		m_hCloudNetApi = p_hCloudNetApi;
		m_pAuthService = p_pAuthService;
		m_pBackendPool = p_pBackendPool;
		m_bRunning = true;
		m_bSweepStop = false;
		m_bProfitDemandDirty = true;
		m_strGatewayInstance =
			p_pAuthService->GetInstanceId();
		m_ullGatewayEpoch =
			GenerateGatewayEpoch();
		m_ullProfitRevision = 0;
		m_mapLastProfitDemand.clear();
	}

	// 第一步：先启动发送线程，再开放连接，保证首条订阅快照有可用队列。
	if (!m_clPushDispatcher.Start(stConfig, PushSendCallback,
		PushCloseCallback, this, p_refError))
	{
		Stop();
		return false;
	}
	try
	{
		m_clSweepThread = std::thread(
			&CMtGatewayWebService::SweepThread, this);
	}
	catch (const std::exception& p_refException)
	{
		p_refError = std::string("WEB_SWEEP_THREAD_FAILED: detail=") +
			p_refException.what();
		Stop();
		return false;
	}

	// 第二步：连接治理组件完成后再启动 SocketServer。
	char szError[1024] = {0};
	if (!pServer->CreateWebSock(stConfig.strBindIp.c_str(),
		stConfig.usPort, stConfig.uiRecvBufferBytes,
		stConfig.uiMaxConnections, stConfig.uiAcceptBacklog,
		WebNotifyCallback, stConfig.uiIoThreads,
		stConfig.uiQueueCapacity, szError, nullptr))
	{
		p_refError = std::string("WEB_SERVER_START_FAILED: detail=") + szError;
		Stop();
		return false;
	}
	WEB_INFO(
		"web service started,ip=%s,port=%hu,maxConnections=%u,sendThreads=%u,quoteQueue=%u,profitQueue=%u,normalQueue=%u,idleTimeoutSec=%d",
		stConfig.strBindIp.c_str(), stConfig.usPort,
		stConfig.uiMaxConnections, stConfig.uiSendThreads,
		stConfig.uiQuoteQueueCapacity, stConfig.uiProfitQueueCapacity,
		stConfig.uiNormalQueueCapacity, stConfig.iIdleTimeoutSec);
	return true;
}

void CMtGatewayWebService::Stop()
{
	CSocketServer* pServer = nullptr;
	{
		std::lock_guard<std::mutex> clLock(m_clMutex);
		m_bRunning = false;
		m_bSweepStop = true;
		pServer = m_pServer;
	}
	m_clCondition.notify_all();

	// 第一步：停止接收新推送并排空队列，发送回调仍可使用尚未释放的 SocketServer。
	m_clPushDispatcher.Stop();
	if (pServer != nullptr)
	{
		pServer->StopWebSock();
	}
	if (m_clSweepThread.joinable() &&
		m_clSweepThread.get_id() != std::this_thread::get_id())
	{
		m_clSweepThread.join();
	}

	// 第二步：等待底层连接回调退出，再清空会话并向 Derive 删除本实例全部 Profit 租约。
	{
		std::unique_lock<std::mutex> clLock(m_clMutex);
		m_clCondition.wait(clLock,
			[this]() { return m_iActiveCallbacks == 0; });
	}
	m_clSessionManager.Reset(0);
	{
		std::lock_guard<std::mutex> clSequenceLock(
			m_clQuoteSequenceMutex);
		m_mapQuoteSequence.clear();
		m_mapOpenPrice.clear();
	}
	SynchronizeProfitDemands();
	{
		std::lock_guard<std::mutex> clLock(m_clMutex);
		m_pServer = nullptr;
		m_hCloudNetApi = nullptr;
		m_pAuthService = nullptr;
		m_pBackendPool = nullptr;
		m_strGatewayInstance.clear();
		m_ullGatewayEpoch = 0;
		m_ullProfitRevision = 0;
		m_mapLastProfitDemand.clear();
		m_bProfitDemandDirty = false;
	}
	if (pServer != nullptr)
	{
		DelWebSockInstance(pServer);
	}
	CWebLog::Release();
}

void CMtGatewayWebService::PublishPluginNotify(
	EN_PLUGIN_NOTIFY_ID p_enNotifyId,
	const ST_PLUGIN_NOTIFY_META& p_refMeta,
	const unsigned char* p_pPayload,
	std::size_t p_szPayloadLen) noexcept
{
	try
	{
		PublishPluginNotifyCore(p_enNotifyId, p_refMeta,
			p_pPayload, p_szPayloadLen);
	}
	catch (const std::exception& p_refException)
	{
		try
		{
			WEB_ERROR(
				"web notify callback exception,notifyId=%lld,payloadLen=%zu,detail=%s",
				static_cast<long long>(p_enNotifyId),
				p_szPayloadLen, p_refException.what());
		}
		catch (...)
		{
		}
	}
	catch (...)
	{
		try
		{
			WEB_ERROR(
				"web notify callback exception,notifyId=%lld,payloadLen=%zu,detail=unknown",
				static_cast<long long>(p_enNotifyId),
				p_szPayloadLen);
		}
		catch (...)
		{
		}
	}
}

void CMtGatewayWebService::PublishPluginNotifyCore(
	EN_PLUGIN_NOTIFY_ID p_enNotifyId,
	const ST_PLUGIN_NOTIFY_META& p_refMeta,
	const unsigned char* p_pPayload, std::size_t p_szPayloadLen)
{
	const std::int32_t iWebFuncId =
		GetMtGatewayWebFuncId(p_enNotifyId);
	if (iWebFuncId == 0)
	{
		return;
	}
	bool bTokenMode = false;
	{
		std::lock_guard<std::mutex> clLock(m_clMutex);
		if (!m_bRunning)
		{
			return;
		}
		bTokenMode = m_pAuthService != nullptr &&
			m_pAuthService->IsWebTokenMode();
	}
	if (p_refMeta.usSourcePlugin !=
			EN_PLUGIN_ID_MT_EVENT_SERVICE ||
		p_refMeta.ullSequence == 0 ||
		p_refMeta.llTimestampMs <= 0)
	{
		WEB_WARN(
			"web notify source metadata invalid,notifyId=%lld,sourcePlugin=%u,outerSequence=%llu,outerTimestampMs=%lld",
			static_cast<long long>(p_enNotifyId),
			static_cast<unsigned int>(p_refMeta.usSourcePlugin),
			static_cast<unsigned long long>(p_refMeta.ullSequence),
			static_cast<long long>(p_refMeta.llTimestampMs));
		return;
	}

	// Derive 1251/1252 使用专用 Binary 协议，不是 ClientData 字段树；一次解码后按过滤键共享编码正文。
	if (p_enNotifyId == EN_PLUGIN_NOTIFY_PROFIT_CHANGED)
	{
		ST_DERIVE_PROFIT_SNAPSHOT_RESPONSE stProfit;
		std::string strError;
		if (!DecodeDeriveProfitSnapshotResponse(p_pPayload,
				p_szPayloadLen, stProfit, strError) ||
			stProfit.iCode != EN_TERMINAL_ERROR_OK ||
			stProfit.enState != EN_DERIVE_SERVICE_STATE_READY ||
			stProfit.ullEventSequence != p_refMeta.ullSequence)
		{
			WEB_WARN(
				"web profit conversion failed,sequence=%llu,detail=%s",
				static_cast<unsigned long long>(p_refMeta.ullSequence),
				strError.empty() ?
					"profit state or event watermark mismatch" : strError.c_str());
			return;
		}
		std::map<std::int64_t, std::pair<std::uint16_t, std::int32_t>> mapLogin;
		for (const ST_DERIVE_ACCOUNT_PROFIT& refAccount : stProfit.aAccount)
		{
			mapLogin[refAccount.llLogin] = std::make_pair(
				refAccount.usPlatformVersion, refAccount.iSourceNo);
		}
		for (const ST_DERIVE_POSITION_PROFIT& refPosition : stProfit.aPosition)
		{
			mapLogin[refPosition.llLogin] = std::make_pair(
				refPosition.usPlatformVersion, refPosition.iSourceNo);
		}
		for (const auto& refLogin : mapLogin)
		{
			ST_CLIENT_DATA_BINARY_EVENT stEvent;
			stEvent.ullTopic = EN_CLIENT_DATA_BINARY_TOPIC_PROFIT;
			stEvent.iSourceVersion = refLogin.second.first;
			stEvent.iSourceNo = refLogin.second.second;
			stEvent.llLogin = refLogin.first;
			stEvent.ullSequence = p_refMeta.ullSequence;
			stEvent.llTimestampMs = p_refMeta.llTimestampMs;
			nlohmann::json clData = nlohmann::json::object();
			clData["Topic"] = stEvent.ullTopic;
			clData["EventType"] = static_cast<int>(p_refMeta.usNotifyAction);
			clData["Version"] = stEvent.iSourceVersion;
			clData["No"] = stEvent.iSourceNo;
			clData["Login"] = stEvent.llLogin;
			clData["Symbol"] = "";
			clData["Sequence"] = stEvent.ullSequence;
			clData["Timestamp"] = stEvent.llTimestampMs;
			clData["Data"] = BuildProfitForWeb(stProfit, refLogin.first);
			const auto refMessage = std::make_shared<const std::string>(
				BuildWebResponse(EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION,
					EN_TERMINAL_ERROR_OK, "OK", clData));
			ST_MT_GATEWAY_PUSH_CONTEXT stContext;
			stContext.usVersion = refLogin.second.first;
			stContext.iNo = refLogin.second.second;
			stContext.ullSequence = p_refMeta.ullSequence;
			DispatchIncrement(EN_MT_GATEWAY_WEB_TOPIC_PROFIT,
				stEvent, refMessage, 0, stContext);
		}
		return;
	}
	if (p_enNotifyId == EN_PLUGIN_NOTIFY_KLINE_CHANGED)
	{
		ST_DERIVE_M1_SNAPSHOT_RESPONSE stM1;
		std::string strError;
		if (!DecodeDeriveM1SnapshotResponse(p_pPayload,
				p_szPayloadLen, stM1, strError) ||
			stM1.iCode != EN_TERMINAL_ERROR_OK ||
			stM1.enState != EN_DERIVE_SERVICE_STATE_READY ||
			stM1.aBar.empty() ||
			stM1.ullEventSequence != p_refMeta.ullSequence)
		{
			WEB_WARN(
				"web M1 conversion failed,sequence=%llu,detail=%s",
				static_cast<unsigned long long>(p_refMeta.ullSequence),
				strError.empty() ?
					"M1 state or event watermark mismatch" : strError.c_str());
			return;
		}
		for (const ST_DERIVE_M1_BAR& refBar : stM1.aBar)
		{
			UpdateOpenPriceFromM1(refBar);
			ST_CLIENT_DATA_BINARY_EVENT stEvent;
			stEvent.ullTopic = EN_CLIENT_DATA_BINARY_TOPIC_KLINE;
			stEvent.iSourceVersion = refBar.usPlatformVersion;
			stEvent.iSourceNo = refBar.iSourceNo;
			stEvent.strSymbol = refBar.strSymbol;
			stEvent.ullSequence = p_refMeta.ullSequence;
			stEvent.llTimestampMs = p_refMeta.llTimestampMs;
			nlohmann::json clData = nlohmann::json::object();
			clData["Topic"] = stEvent.ullTopic;
			clData["EventType"] = static_cast<int>(p_refMeta.usNotifyAction);
			clData["Version"] = stEvent.iSourceVersion;
			clData["No"] = stEvent.iSourceNo;
			clData["Login"] = 0;
			clData["Symbol"] = stEvent.strSymbol;
			clData["Sequence"] = stEvent.ullSequence;
			clData["Timestamp"] = stEvent.llTimestampMs;
			clData["Data"] = BuildM1BarForWeb(refBar);
			const auto refMessage = std::make_shared<const std::string>(
				BuildWebResponse(EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION,
					EN_TERMINAL_ERROR_OK, "OK", clData));
			ST_MT_GATEWAY_PUSH_CONTEXT stContext;
			stContext.usVersion = refBar.usPlatformVersion;
			stContext.iNo = refBar.iSourceNo;
			stContext.strSymbol = refBar.strSymbol;
			stContext.ullSequence = p_refMeta.ullSequence;
			DispatchIncrement(EN_MT_GATEWAY_WEB_TOPIC_KLINE,
				stEvent, refMessage, refBar.ullSourceEpoch, stContext);
		}
		return;
	}

	// 第一步：12xx 外层通知只负责类型，正文必须是 ClientData Binary 契约。
	ST_CLIENT_DATA_BINARY_EVENT stEvent;
	std::string strError;
	if (!DecodeClientDataBinaryEvent(p_pPayload,
		p_szPayloadLen, stEvent, strError))
	{
		WEB_WARN(
			"web notify decode failed,notifyId=%lld,sequence=%llu,payloadLen=%zu,detail=%s",
			static_cast<long long>(p_enNotifyId),
			static_cast<unsigned long long>(p_refMeta.ullSequence),
			p_szPayloadLen, strError.c_str());
		return;
	}
	if (p_enNotifyId != EN_PLUGIN_NOTIFY_MARKET_TICK &&
		(p_refMeta.ullSequence != stEvent.ullSequence ||
			p_refMeta.llTimestampMs != stEvent.llTimestampMs))
	{
		WEB_WARN(
			"web notify metadata mismatch,notifyId=%lld,outerSequence=%llu,eventSequence=%llu",
			static_cast<long long>(p_enNotifyId),
			static_cast<unsigned long long>(
				p_refMeta.ullSequence),
			static_cast<unsigned long long>(
				stEvent.ullSequence));
		return;
	}
	nlohmann::json clBusinessData;
	std::uint64_t ullSourceEpoch = 0;
	if (p_enNotifyId == EN_PLUGIN_NOTIFY_MARKET_TICK)
	{
		const double dM1OpenPrice = GetOpenPrice(
			static_cast<std::uint16_t>(stEvent.iSourceVersion),
			stEvent.iSourceNo, stEvent.strSymbol);
		if (!DecodeQuoteForWeb(p_refMeta, stEvent,
			dM1OpenPrice, clBusinessData, ullSourceEpoch, strError))
		{
			WEB_WARN(
				"web quote conversion failed,sequence=%llu,payloadLen=%zu,detail=%s",
				static_cast<unsigned long long>(
					p_refMeta.ullSequence),
				p_szPayloadLen, strError.c_str());
			return;
		}
	}
	else if (!stEvent.refData ||
		!CMtGatewayRequestAdapter::BinaryValueToJson(
			*stEvent.refData, clBusinessData, strError))
	{
		WEB_WARN(
			"web notify data conversion failed,notifyId=%lld,detail=%s",
			static_cast<long long>(p_enNotifyId), strError.c_str());
		return;
	}

	// 第二步：10004 只广播交易源状态；10002 必须严格匹配已登记 Topic。
	if (iWebFuncId == EN_MT_GATEWAY_WEB_FUNC_SOURCE_RESYNC)
	{
		nlohmann::json clData = nlohmann::json::object();
		clData["Version"] = stEvent.iSourceVersion;
		clData["No"] = stEvent.iSourceNo;
		clData["Sequence"] = stEvent.ullSequence;
		clData["Timestamp"] = stEvent.llTimestampMs;
		clData["Data"] = clBusinessData;
		const std::shared_ptr<const std::string> refMessage =
			std::make_shared<const std::string>(BuildWebResponse(
				EN_MT_GATEWAY_WEB_FUNC_SOURCE_RESYNC,
				EN_TERMINAL_ERROR_OK, "OK", clData));
		m_clPushDispatcher.EnqueueBatch(
			EN_MT_GATEWAY_WEB_TOPIC_USER,
			m_clSessionManager.GetAuthenticatedTargets(bTokenMode),
			refMessage, ST_MT_GATEWAY_PUSH_CONTEXT());
		return;
	}

	const EN_MT_GATEWAY_WEB_TOPIC enExpectedTopic =
		GetMtGatewayWebTopic(p_enNotifyId);
	if (enExpectedTopic == EN_MT_GATEWAY_WEB_TOPIC_NONE ||
		stEvent.ullTopic != static_cast<std::uint64_t>(enExpectedTopic))
	{
		WEB_WARN(
			"web notify topic mismatch,notifyId=%lld,expected=%llu,actual=%llu",
			static_cast<long long>(p_enNotifyId),
			static_cast<unsigned long long>(enExpectedTopic),
			static_cast<unsigned long long>(stEvent.ullTopic));
		return;
	}
	nlohmann::json clData = nlohmann::json::object();
	clData["Topic"] = stEvent.ullTopic;
	clData["EventType"] = static_cast<int>(p_refMeta.usNotifyAction);
	clData["Version"] = stEvent.iSourceVersion;
	clData["No"] = stEvent.iSourceNo;
	clData["Login"] = stEvent.llLogin;
	clData["Symbol"] = stEvent.strSymbol;
	clData["Sequence"] = stEvent.ullSequence;
	clData["Timestamp"] = stEvent.llTimestampMs;
	clData["Data"] = clBusinessData;
	const std::shared_ptr<const std::string> refMessage =
		std::make_shared<const std::string>(BuildWebResponse(
			EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION,
			EN_TERMINAL_ERROR_OK, "OK", clData));

	// 第三步：一次事件只编码一次，冻结目标代次后按固定 Sender 批量入队。
	ST_MT_GATEWAY_PUSH_CONTEXT stContext;
	stContext.usVersion = static_cast<std::uint16_t>(stEvent.iSourceVersion);
	stContext.iNo = stEvent.iSourceNo;
	stContext.strSymbol = stEvent.strSymbol;
	stContext.ullSequence = stEvent.ullSequence;
	if (p_enNotifyId == EN_PLUGIN_NOTIFY_MARKET_TICK)
	{
		const int iContinuity = CheckQuoteContinuity(
			static_cast<std::uint16_t>(stEvent.iSourceVersion),
			stEvent.iSourceNo, ullSourceEpoch, stEvent.ullSequence);
		if (iContinuity == 2)
		{
			WEB_WARN(
				"web stale Quote ignored,version=%d,no=%d,epoch=%llu,sequence=%llu",
				stEvent.iSourceVersion, stEvent.iSourceNo,
				static_cast<unsigned long long>(ullSourceEpoch),
				static_cast<unsigned long long>(stEvent.ullSequence));
			return;
		}
		if (iContinuity == 1)
		{
			const std::uint32_t uiInterval = GetQuoteMinIntervalMs(
				static_cast<std::uint16_t>(stEvent.iSourceVersion),
				stEvent.iSourceNo);
			WEB_WARN(
				"web Quote continuity gap,version=%d,no=%d,epoch=%llu,sequence=%llu,mode=%s",
				stEvent.iSourceVersion, stEvent.iSourceNo,
				static_cast<unsigned long long>(ullSourceEpoch),
				static_cast<unsigned long long>(stEvent.ullSequence),
				uiInterval == 0 ? "raw_tick" : "latest_quote");
			if (uiInterval == 0)
			{
				for (const ST_MT_GATEWAY_WEB_TARGET& refTarget :
					m_clSessionManager.GetQuoteSourceTargets(
						stEvent.iSourceVersion, stEvent.iSourceNo))
				{
					CloseTarget(refTarget,
						"resync_required_quote_sequence_gap");
				}
				return;
			}
			std::vector<ST_MT_GATEWAY_SNAPSHOT_TASK> aTask;
			m_clSessionManager.BeginQuoteSourceResync(
				stEvent.iSourceVersion, stEvent.iSourceNo, aTask);
			// 缺口后的当前 Tick 先进入新屏障缓冲，再提交 1122，避免请求提交窗口丢增量。
			DispatchIncrement(enExpectedTopic, stEvent,
				refMessage, ullSourceEpoch, stContext);
			for (const ST_MT_GATEWAY_SNAPSHOT_TASK& refTask : aTask)
			{
				SubmitSnapshotTask(refTask);
			}
			return;
		}
	}
	DispatchIncrement(enExpectedTopic, stEvent,
		refMessage, ullSourceEpoch, stContext);
}

std::uint32_t CMtGatewayWebService::GetQuoteMinIntervalMs(
	std::uint16_t p_usVersion, std::int32_t p_iNo) const
{
	std::lock_guard<std::mutex> clLock(m_clQuoteSequenceMutex);
	const auto it = m_mapQuoteInterval.find(std::make_pair(
		static_cast<std::int32_t>(p_usVersion), p_iNo));
	return it != m_mapQuoteInterval.end() ? it->second : 0U;
}

int CMtGatewayWebService::CheckQuoteContinuity(
	std::uint16_t p_usVersion, std::int32_t p_iNo,
	std::uint64_t p_ullSourceEpoch,
	std::uint64_t p_ullSequence)
{
	const std::pair<std::int32_t, std::int32_t> stKey =
		std::make_pair(static_cast<std::int32_t>(p_usVersion), p_iNo);
	std::lock_guard<std::mutex> clLock(m_clQuoteSequenceMutex);
	const auto it = m_mapQuoteSequence.find(stKey);
	if (it == m_mapQuoteSequence.end())
	{
		m_mapQuoteSequence[stKey] =
			std::make_pair(p_ullSourceEpoch, p_ullSequence);
		return 0;
	}
	if (it->second.first != p_ullSourceEpoch)
	{
		it->second = std::make_pair(p_ullSourceEpoch, p_ullSequence);
		return 1;
	}
	if (p_ullSequence <= it->second.second)
	{
		return 2;
	}
	const bool bGap = p_ullSequence != it->second.second + 1U;
	it->second.second = p_ullSequence;
	return bGap ? 1 : 0;
}

CMtGatewayWebService::ST_WEB_OPEN_PRICE_STATE::
	ST_WEB_OPEN_PRICE_STATE()
	: uiServerDate(0)
	, llFirstMinute(0)
	, dOpenPrice(0.0)
{
}

void CMtGatewayWebService::UpdateOpenPriceFromM1(
	const ST_DERIVE_M1_BAR& p_refBar)
{
	if ((p_refBar.usPlatformVersion != 4U &&
		 p_refBar.usPlatformVersion != 5U) ||
		p_refBar.iSourceNo <= 0 || p_refBar.strSymbol.empty() ||
		p_refBar.uiServerDate == 0 || p_refBar.llMinute <= 0 ||
		p_refBar.dOpen <= 0.0 ||
		(p_refBar.uiFlags & DERIVE_M1_FLAG_OPEN_PRICE_CONFIRMED) == 0)
	{
		return;
	}
	std::lock_guard<std::mutex> clLock(m_clQuoteSequenceMutex);
	ST_WEB_OPEN_PRICE_STATE& refState =
		m_mapOpenPrice[std::make_pair(
			static_cast<std::int32_t>(p_refBar.usPlatformVersion),
			p_refBar.iSourceNo)][p_refBar.strSymbol];
	if (p_refBar.uiServerDate > refState.uiServerDate ||
		(p_refBar.uiServerDate == refState.uiServerDate &&
		 (refState.llFirstMinute == 0 ||
		  p_refBar.llMinute < refState.llFirstMinute)))
	{
		refState.uiServerDate = p_refBar.uiServerDate;
		refState.llFirstMinute = p_refBar.llMinute;
		refState.dOpenPrice = p_refBar.dOpen;
	}
}

double CMtGatewayWebService::GetOpenPrice(
	std::uint16_t p_usVersion, std::int32_t p_iNo,
	const std::string& p_refSymbol) const
{
	std::lock_guard<std::mutex> clLock(m_clQuoteSequenceMutex);
	const auto itSource = m_mapOpenPrice.find(std::make_pair(
		static_cast<std::int32_t>(p_usVersion), p_iNo));
	if (itSource == m_mapOpenPrice.end())
	{
		return 0.0;
	}
	const auto itSymbol = itSource->second.find(p_refSymbol);
	return itSymbol == itSource->second.end() ?
		0.0 : itSymbol->second.dOpenPrice;
}

void CMtGatewayWebService::DispatchIncrement(
	EN_MT_GATEWAY_WEB_TOPIC p_enTopic,
	const ST_CLIENT_DATA_BINARY_EVENT& p_refEvent,
	const std::shared_ptr<const std::string>& p_refMessage,
	std::uint64_t p_ullSourceEpoch,
	const ST_MT_GATEWAY_PUSH_CONTEXT& p_refContext)
{
	std::vector<ST_MT_GATEWAY_WEB_TARGET> aLive;
	std::vector<ST_MT_GATEWAY_WEB_TARGET> aResync;
	m_clSessionManager.RouteIncrement(p_refEvent, p_refMessage,
		p_ullSourceEpoch, aLive, aResync);
	for (const ST_MT_GATEWAY_WEB_TARGET& refTarget : aResync)
	{
		CloseTarget(refTarget,
			"resync_required_snapshot_buffer_overflow");
	}
	if (!m_clPushDispatcher.EnqueueBatch(p_enTopic, aLive,
			p_refMessage, p_refContext))
	{
		WEB_WARN("web push batch rejected,topic=%s,messageLen=%zu",
			GetMtGatewayWebTopicName(p_enTopic),
			p_refMessage ? p_refMessage->size() : 0U);
	}
}

ST_MT_GATEWAY_WEB_USER_STATS
CMtGatewayWebService::GetUserStats() const
{
	return m_clSessionManager.GetUserStats();
}

void CMtGatewayWebService::WebNotifyCallback(
	void* p_pServerHandle, void* p_pClientHandle,
	WebSockNotifyType p_enType, const void* p_pData, int p_iDataLen,
	const char* p_szClientIp, unsigned short p_usClientPort,
	void* p_pErrorData)
{
	CMtGatewayWebService& refService = GetInstance();
	CSocketServer* pServer = nullptr;
	if (!refService.BeginCallback(pServer))
	{
		return;
	}
	(void)pServer;
	try
	{
		refService.HandleNotify(p_pServerHandle, p_pClientHandle,
			p_enType, p_pData, p_iDataLen, p_szClientIp,
			p_usClientPort, p_pErrorData);
	}
	catch (const std::exception& p_refException)
	{
		WEB_ERROR(
			"web callback exception,detail=%s", p_refException.what());
	}
	catch (...)
	{
		WEB_ERROR(
			"web callback exception,detail=unknown exception");
	}
	refService.EndCallback();
}

void CMtGatewayWebService::SnapshotResultCallback(
	const ST_CLOUD_NET_BINARY_RESULT* p_pResult, void* p_pParam)
{
	std::unique_ptr<ST_MT_GATEWAY_SNAPSHOT_CALLBACK> ptrCallback(
		static_cast<ST_MT_GATEWAY_SNAPSHOT_CALLBACK*>(p_pParam));
	if (!ptrCallback)
	{
		return;
	}
	try
	{
		if (ptrCallback->bOpenPriceBootstrap)
		{
			GetInstance().HandleOpenPriceBootstrapResult(
				ptrCallback->stTarget, ptrCallback->iVersion,
				ptrCallback->iNo, ptrCallback->uiAttempt,
				ptrCallback->setLogin, ptrCallback->setSymbol,
				p_pResult);
		}
		else
		{
			GetInstance().HandleSnapshotResult(ptrCallback->stTarget,
				ptrCallback->enTopic, ptrCallback->iVersion,
				ptrCallback->iNo, ptrCallback->uiAttempt,
				ptrCallback->stApi, p_pResult);
		}
	}
	catch (const std::exception& p_refException)
	{
		try
		{
			WEB_ERROR(
				"web snapshot callback exception,topic=%s,detail=%s",
				GetMtGatewayWebTopicName(ptrCallback->enTopic),
				p_refException.what());
		}
		catch (...)
		{
		}
	}
	catch (...)
	{
		try
		{
			WEB_ERROR(
				"web snapshot callback exception,topic=%s,detail=unknown",
				GetMtGatewayWebTopicName(ptrCallback->enTopic));
		}
		catch (...)
		{
		}
	}
}

bool CMtGatewayWebService::PushSendCallback(
	const ST_MT_GATEWAY_WEB_TARGET& p_refTarget,
	const std::string& p_refMessage, void* p_pContext)
{
	CMtGatewayWebService* pService =
		static_cast<CMtGatewayWebService*>(p_pContext);
	return pService != nullptr &&
		pService->SendPush(p_refTarget, p_refMessage);
}

void CMtGatewayWebService::PushCloseCallback(
	const ST_MT_GATEWAY_WEB_TARGET& p_refTarget,
	const char* p_szReason, void* p_pContext)
{
	CMtGatewayWebService* pService =
		static_cast<CMtGatewayWebService*>(p_pContext);
	if (pService != nullptr)
	{
		pService->CloseTarget(p_refTarget, p_szReason);
	}
}

void CMtGatewayWebService::HandleNotify(
	void* p_pServerHandle, void* p_pClientHandle,
	WebSockNotifyType p_enType, const void* p_pData, int p_iDataLen,
	const char* p_szClientIp, unsigned short p_usClientPort,
	void* p_pErrorData)
{
	const char* pClientIp = p_szClientIp != nullptr ? p_szClientIp : "";
	if (p_enType == enWebConnect)
	{
		std::uint64_t ullGeneration = 0;
		if (!m_clSessionManager.AddConnection(p_pServerHandle,
			p_pClientHandle, pClientIp, p_usClientPort, ullGeneration))
		{
			ST_MT_GATEWAY_WEB_TARGET stTarget;
			stTarget.stKey.pServerHandle = p_pServerHandle;
			stTarget.stKey.pClientHandle = p_pClientHandle;
			CloseTarget(stTarget, "web_connection_limit_reached");
			return;
		}
		WEB_INFO(
			"web client connected,server=%p,client=%p,generation=%llu,ip=%s,port=%hu",
			p_pServerHandle, p_pClientHandle,
			static_cast<unsigned long long>(ullGeneration),
			pClientIp, p_usClientPort);
		return;
	}

	ST_MT_GATEWAY_WEB_TARGET stTarget;
	const bool bKnown = m_clSessionManager.ResolveTarget(
		p_pServerHandle, p_pClientHandle, stTarget);
	if (p_enType == enWebClose || p_enType == enWebError)
	{
		if (bKnown)
		{
			m_clPushDispatcher.RemoveTarget(stTarget);
			m_clSessionManager.RemoveConnection(
				p_pServerHandle, p_pClientHandle);
			MarkProfitDemandDirty();
		}
		if (p_enType == enWebClose)
		{
			WEB_INFO("web client disconnected,server=%p,client=%p,generation=%llu,ip=%s,port=%hu,type=%d,detail=%s",
				p_pServerHandle, p_pClientHandle,
				static_cast<unsigned long long>(stTarget.ullGeneration),
				pClientIp, p_usClientPort, static_cast<int>(p_enType),
				p_pErrorData != nullptr ?
				static_cast<const char*>(p_pErrorData) : "");
		}
		else
		{
			WEB_WARN("web client disconnected,server=%p,client=%p,generation=%llu,ip=%s,port=%hu,type=%d,detail=%s",
				p_pServerHandle, p_pClientHandle,
				static_cast<unsigned long long>(stTarget.ullGeneration),
				pClientIp, p_usClientPort, static_cast<int>(p_enType),
				p_pErrorData != nullptr ?
				static_cast<const char*>(p_pErrorData) : "");
		}
		return;
	}
	if (p_enType == enWebData && bKnown)
	{
		HandleData(stTarget, p_pData, p_iDataLen);
	}
}

void CMtGatewayWebService::HandleData(
	const ST_MT_GATEWAY_WEB_TARGET& p_refTarget,
	const void* p_pData, int p_iDataLen)
{
	// 第一步：校验服务状态、连接代次和消息长度，错误消息不进入订阅处理。
	bool bRunning = false;
	{
		std::lock_guard<std::mutex> clLock(m_clMutex);
		bRunning = m_bRunning;
	}
	if (!bRunning || !m_clSessionManager.IsCurrent(p_refTarget))
	{
		return;
	}
	if (p_pData == nullptr || p_iDataLen <= 0 ||
		p_iDataLen > m_stConfig.iMaxMessageBytes)
	{
		SendControlResponse(p_refTarget, 0,
			EN_V2_WEB_NEED_JSON, "",
			nlohmann::json::object());
		return;
	}

	// 第二步：只解析一次 JSON 并读取数字 FuncId，未知协议不进入任何业务队列。
	const char* pData = static_cast<const char*>(p_pData);
	nlohmann::json clRequest = nlohmann::json::parse(
		pData, pData + p_iDataLen, nullptr, false);
	std::string strError;
	std::int64_t llFuncId = 0;
	if (!clRequest.is_object())
	{
		SendControlResponse(p_refTarget, 0,
			EN_V2_WEB_NEED_JSON, "", nlohmann::json::object());
		return;
	}
	if (!ReadInt64(clRequest, "FuncId", llFuncId, true, strError) ||
		llFuncId < (std::numeric_limits<std::int32_t>::min)() ||
		llFuncId > (std::numeric_limits<std::int32_t>::max)())
	{
		SendControlResponse(p_refTarget, 0,
			EN_V2_WEB_FUNC_ID_INVALID, "", nlohmann::json::object());
		return;
	}
	switch (static_cast<std::int32_t>(llFuncId))
	{
	case EN_MT_GATEWAY_WEB_FUNC_HEARTBEAT:
		HandleHeartbeat(p_refTarget, clRequest);
		break;
	case EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION:
		HandleSubscription(p_refTarget, clRequest);
		break;
	case EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION_STATE:
		HandleSubscriptionState(p_refTarget, clRequest);
		break;
	case EN_MT_GATEWAY_WEB_FUNC_SOURCE_RESYNC:
		SendControlResponse(p_refTarget,
			EN_MT_GATEWAY_WEB_FUNC_SOURCE_RESYNC,
			EN_V2_WEB_ACTION_UNSUPPORTED, "",
			nlohmann::json::object());
		break;
	default:
		SendControlResponse(p_refTarget,
			static_cast<std::int32_t>(llFuncId),
			EN_V2_WEB_ACTION_UNSUPPORTED, "",
			nlohmann::json::object());
		break;
	}
}

void CMtGatewayWebService::HandleHeartbeat(
	const ST_MT_GATEWAY_WEB_TARGET& p_refTarget,
	const nlohmann::json& p_refRequest)
{
	std::string strError;
	std::int64_t llType = 0;
	std::int64_t llTimestamp = 0;
	if (!ReadInt64(p_refRequest, "Type", llType, true, strError) ||
		llType != EN_MT_GATEWAY_WEB_HEARTBEAT_REQUEST)
	{
		SendControlResponse(p_refTarget,
			EN_MT_GATEWAY_WEB_FUNC_HEARTBEAT,
			EN_V2_WEB_TYPE_INVALID, "",
			nlohmann::json::object());
		return;
	}
	if (!ReadInt64(p_refRequest, "Timestamp", llTimestamp, true, strError))
	{
		SendControlResponse(p_refTarget,
			EN_MT_GATEWAY_WEB_FUNC_HEARTBEAT,
			EN_V2_WEB_TIMESTAMP_INVALID, "",
			nlohmann::json::object());
		return;
	}

	// 第一步：Token 模式首个有效心跳必须验证并绑定 UserID，后续心跳不得更换主体。
	const CMtGatewayAuthService* pAuthService = nullptr;
	{
		std::lock_guard<std::mutex> clLock(m_clMutex);
		pAuthService = m_pAuthService;
	}
	if (pAuthService == nullptr)
	{
		SendControlResponse(p_refTarget,
			EN_MT_GATEWAY_WEB_FUNC_HEARTBEAT,
			EN_TERMINAL_ERROR_MT_SYSTEM_NOT_READY, "",
			nlohmann::json::object());
		return;
	}
	std::uint64_t ullBoundUser = 0;
	const bool bAlreadyAuthenticated =
		m_clSessionManager.IsAuthenticated(p_refTarget,
			pAuthService->IsWebTokenMode(), ullBoundUser);
	if (!bAlreadyAuthenticated ||
		p_refRequest.find("UserID") != p_refRequest.end() ||
		p_refRequest.find("Token") != p_refRequest.end())
	{
		std::string strUserId;
		std::string strToken;
		if (!ReadString(p_refRequest, "UserID", strUserId,
				pAuthService->IsWebTokenMode(), strError) ||
			!ReadString(p_refRequest, "Token", strToken,
				pAuthService->IsWebTokenMode(), strError) ||
			!pAuthService->ValidateWebPrincipal(strUserId,
				strToken, ullBoundUser, strError))
		{
			SendControlResponse(p_refTarget,
				EN_MT_GATEWAY_WEB_FUNC_HEARTBEAT,
				EN_TERMINAL_ERROR_MT_INVALID_TOKEN, "",
				nlohmann::json::object());
			return;
		}
		if (!m_clSessionManager.BindPrincipal(p_refTarget,
				ullBoundUser, pAuthService->IsWebTokenMode(), strError))
		{
			SendControlResponse(p_refTarget,
				EN_MT_GATEWAY_WEB_FUNC_HEARTBEAT,
				EN_TERMINAL_ERROR_MT_TOKEN_LOGIN_MISMATCH, "",
				nlohmann::json::object());
			return;
		}
	}

	// 第二步：只有完全合法的心跳才刷新活动时间并回显 Timestamp、Seq 和 TraceId。
	nlohmann::json clData = nlohmann::json::object();
	clData["Type"] = EN_MT_GATEWAY_WEB_HEARTBEAT_RESPONSE;
	clData["Timestamp"] = llTimestamp;
	clData["ServerTimestamp"] = GetTimestampMs();
	const nlohmann::json::const_iterator itSeq = p_refRequest.find("Seq");
	if (itSeq != p_refRequest.end() && !itSeq->is_null())
	{
		if (!itSeq->is_number_integer() && !itSeq->is_number_unsigned())
		{
			SendControlResponse(p_refTarget,
				EN_MT_GATEWAY_WEB_FUNC_HEARTBEAT,
				EN_V2_WEB_SEQ_INVALID, "",
				nlohmann::json::object());
			return;
		}
		clData["Seq"] = *itSeq;
	}
	std::string strTraceId;
	const nlohmann::json::const_iterator itTraceId =
		p_refRequest.find("TraceId");
	if (itTraceId != p_refRequest.end() && !itTraceId->is_null() &&
		(!itTraceId->is_string() ||
		itTraceId->get<std::string>().size() > WEB_TRACE_ID_MAX_LENGTH))
	{
		SendControlResponse(p_refTarget,
			EN_MT_GATEWAY_WEB_FUNC_HEARTBEAT,
			EN_V2_WEB_TRACE_ID_INVALID, "",
			nlohmann::json::object());
		return;
	}
	if (itTraceId != p_refRequest.end() && itTraceId->is_string())
	{
		strTraceId = itTraceId->get<std::string>();
	}
	if (!strTraceId.empty())
	{
		clData["TraceId"] = strTraceId;
	}
	m_clSessionManager.Touch(p_refTarget);
	SendControlResponse(p_refTarget,
		EN_MT_GATEWAY_WEB_FUNC_HEARTBEAT,
		EN_TERMINAL_ERROR_OK, "", clData);
}

void CMtGatewayWebService::HandleSubscription(
	const ST_MT_GATEWAY_WEB_TARGET& p_refTarget,
	const nlohmann::json& p_refRequest)
{
	const CMtGatewayAuthService* pAuthService = nullptr;
	{
		std::lock_guard<std::mutex> clLock(m_clMutex);
		pAuthService = m_pAuthService;
	}
	const bool bTokenMode = pAuthService != nullptr &&
		pAuthService->IsWebTokenMode();
	std::uint64_t ullPrincipal = 0;
	if (!m_clSessionManager.IsAuthenticated(
		p_refTarget, bTokenMode, ullPrincipal))
	{
		SendControlResponse(p_refTarget,
			EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION,
			EN_TERMINAL_ERROR_MT_INVALID_TOKEN, "",
			nlohmann::json::object());
		return;
	}

	// 第一步：解析订阅动作、Topic、数据源和过滤条件，拒绝协议外 Topic。
	std::string strError;
	std::uint64_t ullSubType = 0;
	std::uint64_t ullTopicMask = 0;
	std::uint64_t ullVersion = 5;
	std::uint64_t ullNo = 1;
	std::set<std::int64_t> setLogins;
	std::set<std::string> setSymbols;
	int iFieldCode = EN_TERMINAL_ERROR_OK;
	if (!ReadUInt64(p_refRequest, "SubType", ullSubType, true, strError) ||
		(ullSubType != EN_MT_GATEWAY_WEB_SUBSCRIBE &&
		ullSubType != EN_MT_GATEWAY_WEB_UNSUBSCRIBE))
	{
		SendControlResponse(p_refTarget,
			EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION,
			EN_V2_WEB_SUB_TYPE_INVALID, "",
			nlohmann::json::object());
		return;
	}
	if (!ReadUInt64(p_refRequest, "Topic", ullTopicMask, true, strError))
	{
		SendControlResponse(p_refTarget,
			EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION,
			EN_V2_WEB_TOPIC_INVALID, "", nlohmann::json::object());
		return;
	}
	if (ullTopicMask == 0 ||
		(ullTopicMask & ~MT_GATEWAY_WEB_TOPIC_ALL) != 0)
	{
		SendControlResponse(p_refTarget,
			EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION,
			EN_V2_WEB_TOPIC_NOT_ALLOWED, "", nlohmann::json::object());
		return;
	}
	const nlohmann::json::const_iterator itVersion =
		p_refRequest.find("Version");
	if (itVersion != p_refRequest.end() && !itVersion->is_null() &&
		(!ReadUInt64(p_refRequest, "Version", ullVersion, true, strError) ||
		(ullVersion != 4 && ullVersion != 5)))
	{
		SendControlResponse(p_refTarget,
			EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION,
			EN_V2_WEB_VERSION_INVALID, "", nlohmann::json::object());
		return;
	}
	const nlohmann::json::const_iterator itNo = p_refRequest.find("No");
	if (itNo != p_refRequest.end() && !itNo->is_null() &&
		(!ReadUInt64(p_refRequest, "No", ullNo, true, strError) ||
		ullNo == 0 || ullNo > static_cast<std::uint64_t>(
			(std::numeric_limits<std::int32_t>::max)())))
	{
		SendControlResponse(p_refTarget,
			EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION,
			EN_V2_WEB_NO_INVALID, "", nlohmann::json::object());
		return;
	}
	if (!ReadLogins(p_refRequest, setLogins, iFieldCode) ||
		!ReadSymbols(p_refRequest, setSymbols, iFieldCode))
	{
		SendControlResponse(p_refTarget,
			EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION,
			iFieldCode, "", nlohmann::json::object());
		return;
	}
	const std::uint64_t ullUserTopics =
		EN_MT_GATEWAY_WEB_TOPIC_PROFIT |
		EN_MT_GATEWAY_WEB_TOPIC_ORDER |
		EN_MT_GATEWAY_WEB_TOPIC_POSITION |
		EN_MT_GATEWAY_WEB_TOPIC_DEAL |
		EN_MT_GATEWAY_WEB_TOPIC_USER;
	if (ullSubType == EN_MT_GATEWAY_WEB_SUBSCRIBE &&
		(ullTopicMask & ullUserTopics) != 0 && setLogins.empty())
	{
		SendControlResponse(p_refTarget,
			EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION,
			EN_V2_WEB_NEED_LOGIN, "", nlohmann::json::object());
		return;
	}
	const std::int32_t iVersion = static_cast<std::int32_t>(ullVersion);
	const std::int32_t iNo = static_cast<std::int32_t>(ullNo);

	// 第二步：写入连接代次对应的订阅表，Token 模式强制 Login 与主体一致。
	if (!m_clSessionManager.UpdateSubscription(p_refTarget,
		static_cast<EN_MT_GATEWAY_WEB_SUB_TYPE>(ullSubType),
		ullTopicMask, setLogins, setSymbols,
		iVersion, iNo, bTokenMode, strError))
	{
		int iCode = EN_TERMINAL_ERROR_MT_PARAM;
		if (strError.find("LOGIN_MISMATCH") == 0)
		{
			iCode = EN_TERMINAL_ERROR_MT_TOKEN_LOGIN_MISMATCH;
		}
		else if (strError.find("WEB_SESSION_NOT_FOUND") == 0)
		{
			iCode = EN_V2_WEB_USER_NOT_FOUND;
		}
		else if (strError.find("WEB_SUBSCRIPTION_LOGIN_MISSING") == 0)
		{
			iCode = EN_V2_WEB_NEED_LOGIN;
		}
		SendControlResponse(p_refTarget,
			EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION,
			iCode, "", nlohmann::json::object());
		return;
	}
	if ((ullTopicMask &
		EN_MT_GATEWAY_WEB_TOPIC_PROFIT) != 0)
	{
		// Web 回调只置脏并唤醒后台线程，避免同步 Ice 调用阻塞 SocketServer I/O。
		MarkProfitDemandDirty();
	}
	nlohmann::json clData = nlohmann::json::object();
	clData["SubType"] = ullSubType;
	clData["Topic"] = ullTopicMask;
	clData["Version"] = iVersion;
	clData["No"] = iNo;
	clData["Logins"] = nlohmann::json::array();
	for (const std::int64_t llLogin : setLogins)
	{
		clData["Logins"].push_back(llLogin);
	}
	clData["SymbolIds"] = nlohmann::json::array();
	for (const std::string& refSymbol : setSymbols)
	{
		clData["SymbolIds"].push_back(refSymbol);
	}
	m_clSessionManager.Touch(p_refTarget);
	SendControlResponse(p_refTarget,
		EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION,
		EN_TERMINAL_ERROR_OK, "", clData);

	// 第三步：订阅成功后异步向 Query 获取初始快照，实时事件继续来自 MtEventService。
	if (ullSubType == EN_MT_GATEWAY_WEB_SUBSCRIBE)
	{
		SubmitInitialSnapshots(p_refTarget, ullTopicMask,
			setLogins, setSymbols, iVersion, iNo);
	}
}

void CMtGatewayWebService::HandleSubscriptionState(
	const ST_MT_GATEWAY_WEB_TARGET& p_refTarget,
	const nlohmann::json& p_refRequest)
{
	const CMtGatewayAuthService* pAuthService = nullptr;
	{
		std::lock_guard<std::mutex> clLock(m_clMutex);
		pAuthService = m_pAuthService;
	}
	const bool bTokenMode = pAuthService != nullptr &&
		pAuthService->IsWebTokenMode();
	std::uint64_t ullPrincipal = 0;
	if (!m_clSessionManager.IsAuthenticated(
		p_refTarget, bTokenMode, ullPrincipal))
	{
		SendControlResponse(p_refTarget,
			EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION_STATE,
			EN_TERMINAL_ERROR_MT_INVALID_TOKEN, "",
			nlohmann::json::object());
		return;
	}
	std::string strError;
	std::uint64_t ullVersion = 0;
	std::uint64_t ullNo = 0;
	if (!ReadUInt64(p_refRequest, "Version", ullVersion, true, strError) ||
		(ullVersion != 4 && ullVersion != 5))
	{
		SendControlResponse(p_refTarget,
			EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION_STATE,
			EN_V2_WEB_VERSION_INVALID, "", nlohmann::json::object());
		return;
	}
	if (!ReadUInt64(p_refRequest, "No", ullNo, true, strError) ||
		ullNo == 0 || ullNo > static_cast<std::uint64_t>(
			(std::numeric_limits<std::int32_t>::max)()))
	{
		SendControlResponse(p_refTarget,
			EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION_STATE,
			EN_V2_WEB_NO_INVALID, "", nlohmann::json::object());
		return;
	}
	std::set<std::int64_t> setFilterLogins;
	int iFieldCode = EN_TERMINAL_ERROR_OK;
	if (!ReadLogins(p_refRequest, setFilterLogins, iFieldCode))
	{
		SendControlResponse(p_refTarget,
			EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION_STATE,
			iFieldCode, "", nlohmann::json::object());
		return;
	}
	const std::int32_t iVersion = static_cast<std::int32_t>(ullVersion);
	const std::int32_t iNo = static_cast<std::int32_t>(ullNo);

	// 返回本连接的不可变订阅快照，查询过程中不持有会话表锁。
	nlohmann::json clData = nlohmann::json::object();
	clData["Version"] = iVersion;
	clData["No"] = iNo;
	clData["Subscriptions"] = nlohmann::json::array();
	for (const ST_MT_GATEWAY_WEB_SUBSCRIPTION& refSubscription :
		m_clSessionManager.GetSubscriptions(p_refTarget))
	{
		if (refSubscription.iVersion != iVersion ||
			refSubscription.iNo != iNo)
		{
			continue;
		}
		if (!setFilterLogins.empty() && !refSubscription.setLogin.empty())
		{
			bool bMatched = false;
			for (const std::int64_t llLogin : refSubscription.setLogin)
			{
				if (setFilterLogins.find(llLogin) != setFilterLogins.end())
				{
					bMatched = true;
					break;
				}
			}
			if (!bMatched)
			{
				continue;
			}
		}
		nlohmann::json clItem = nlohmann::json::object();
		clItem["Topic"] = refSubscription.ullTopic;
		clItem["Logins"] = nlohmann::json::array();
		for (const std::int64_t llLogin : refSubscription.setLogin)
		{
			clItem["Logins"].push_back(llLogin);
		}
		clItem["SymbolIds"] = nlohmann::json::array();
		for (const std::string& refSymbol : refSubscription.setSymbol)
		{
			clItem["SymbolIds"].push_back(refSymbol);
		}
		clData["Subscriptions"].push_back(clItem);
	}
	m_clSessionManager.Touch(p_refTarget);
	SendControlResponse(p_refTarget,
		EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION_STATE,
		EN_TERMINAL_ERROR_OK, "", clData);
}

void CMtGatewayWebService::SubmitInitialSnapshots(
	const ST_MT_GATEWAY_WEB_TARGET& p_refTarget,
	std::uint64_t p_ullTopicMask,
	const std::set<std::int64_t>& p_refLogins,
	const std::set<std::string>& p_refSymbols,
	std::int32_t p_iVersion, std::int32_t p_iNo,
	std::uint32_t p_uiSnapshotAttempt,
	bool p_bOpenPriceReady)
{
	HCLOUD_NET_API hCloudNetApi = nullptr;
	CMtGatewayBackendPool* pBackendPool = nullptr;
	{
		std::lock_guard<std::mutex> clLock(m_clMutex);
		hCloudNetApi = m_hCloudNetApi;
		pBackendPool = m_pBackendPool;
	}
	if (hCloudNetApi == nullptr || pBackendPool == nullptr)
	{
		return;
	}

	for (std::uint64_t ullTopic = 1; ullTopic <=
		EN_MT_GATEWAY_WEB_TOPIC_KLINE; ullTopic <<= 1U)
	{
		if ((p_ullTopicMask & ullTopic) == 0)
		{
			continue;
		}
		const EN_MT_GATEWAY_WEB_TOPIC enTopic =
			static_cast<EN_MT_GATEWAY_WEB_TOPIC>(ullTopic);
		std::int64_t llFuncId = GetSnapshotFuncId(enTopic);
		if (llFuncId == 0)
		{
			continue;
		}

		ST_MT_GATEWAY_API_DESCRIPTOR stApi;
		stApi.enRouteKind = EN_MT_GATEWAY_ROUTE_COMPATIBILITY_API;
		stApi.llFuncId = llFuncId;
		stApi.bIdempotent = true;
		std::vector<unsigned char> aPayload;
		std::string strError;
		if (enTopic == EN_MT_GATEWAY_WEB_TOPIC_QUOTE)
		{
			// Quote 必须先拉取 1185 恢复确认 OpenPrice，再用 1122 的 SourceEpoch 和高水位建立屏障。
			llFuncId = p_bOpenPriceReady ?
				EN_PLUGIN_FUNC_QUOTE_SNAPSHOT :
				EN_PLUGIN_FUNC_DERIVE_M1_SNAPSHOT;
			stApi.enTargetPlugin = p_bOpenPriceReady ?
				EN_PLUGIN_ID_MT_QUOTE_SERVICE :
				EN_PLUGIN_ID_MT_DERIVE_SERVICE;
			stApi.enDomain =
				EN_PLUGIN_BINARY_DOMAIN_INVALID;
			stApi.strLogicalService = p_bOpenPriceReady ?
				"MtQuoteService" : "MtDeriveService";
			if (p_bOpenPriceReady)
			{
				ST_QUOTE_SNAPSHOT_BINARY_REQUEST stRequest;
				stRequest.usPlatformVersion =
					static_cast<std::uint16_t>(p_iVersion);
				stRequest.iSourceNo = p_iNo;
				if (!EncodeQuoteSnapshotBinaryRequest(stRequest,
						aPayload, strError))
				{
					continue;
				}
			}
			else
			{
				ST_DERIVE_M1_SNAPSHOT_REQUEST stRequest;
				stRequest.usPlatformVersion =
					static_cast<std::uint16_t>(p_iVersion);
				stRequest.iSourceNo = p_iNo;
				stRequest.uiMaxBars = static_cast<std::uint32_t>(
					DERIVE_BINARY_MAX_SNAPSHOT_ITEMS);
				stRequest.bOpenPriceOnly = true;
				if (!EncodeDeriveM1SnapshotRequest(stRequest,
						aPayload, strError))
				{
					continue;
				}
			}
		}
		else if (enTopic == EN_MT_GATEWAY_WEB_TOPIC_PROFIT ||
			enTopic == EN_MT_GATEWAY_WEB_TOPIC_KLINE)
		{
			// Profit/Kline 初始快照使用 Derive 原生 1184/1185 契约，禁止从 latest_quote 推导权威状态。
			stApi.enTargetPlugin =
				EN_PLUGIN_ID_MT_DERIVE_SERVICE;
			stApi.enDomain =
				EN_PLUGIN_BINARY_DOMAIN_INVALID;
			stApi.strLogicalService =
				"MtDeriveService";
			if (enTopic == EN_MT_GATEWAY_WEB_TOPIC_PROFIT)
			{
				ST_DERIVE_PROFIT_SNAPSHOT_REQUEST stRequest;
				stRequest.usPlatformVersion =
					static_cast<std::uint16_t>(p_iVersion);
				stRequest.iSourceNo = p_iNo;
				stRequest.aLogin.assign(p_refLogins.begin(), p_refLogins.end());
				if (!EncodeDeriveProfitSnapshotRequest(stRequest,
						aPayload, strError))
				{
					WEB_WARN(
						"web profit snapshot encode failed,version=%d,no=%d,detail=%s",
						p_iVersion, p_iNo, strError.c_str());
					continue;
				}
			}
			else
			{
				ST_DERIVE_M1_SNAPSHOT_REQUEST stRequest;
				stRequest.usPlatformVersion =
					static_cast<std::uint16_t>(p_iVersion);
				stRequest.iSourceNo = p_iNo;
				stRequest.uiMaxBars = 10000;
				if (p_refSymbols.size() == 1U)
				{
					stRequest.strSymbol = *p_refSymbols.begin();
				}
				if (!EncodeDeriveM1SnapshotRequest(stRequest,
						aPayload, strError))
				{
					WEB_WARN(
						"web M1 snapshot encode failed,version=%d,no=%d,detail=%s",
						p_iVersion, p_iNo, strError.c_str());
					continue;
				}
			}
		}
		else
		{
			// 其他 Topic 继续构造 Query Binary 字段对象，快照请求不携带 JSON 文本。
			stApi.enTargetPlugin =
				EN_PLUGIN_ID_MT_QUERY_SERVICE;
			stApi.enDomain =
				EN_PLUGIN_BINARY_DOMAIN_QUERY;
			stApi.strLogicalService =
				"MtQueryService";
			nlohmann::json clRequest =
				nlohmann::json::object();
			clRequest["Version"] = p_iVersion;
			clRequest["No"] = p_iNo;
			if (!p_refLogins.empty())
			{
				clRequest["Logins"] =
					nlohmann::json::array();
				for (const std::int64_t llLogin :
					p_refLogins)
				{
					clRequest["Logins"].
						push_back(llLogin);
				}
				if (p_refLogins.size() == 1U)
				{
					clRequest["Login"] =
						*p_refLogins.begin();
				}
			}
			if (!p_refSymbols.empty())
			{
				clRequest["SymbolIds"] =
					nlohmann::json::array();
				for (const std::string& refSymbol :
					p_refSymbols)
				{
					clRequest["SymbolIds"].
						push_back(refSymbol);
				}
			}
			CMtGatewayRequestAdapter clAdapter;
			if (!clAdapter.EncodeRequest(stApi,
					clRequest, aPayload,
					strError))
			{
				WEB_WARN(
					"web snapshot encode failed,topic=%s,detail=%s",
					GetMtGatewayWebTopicName(
						enTopic),
					strError.c_str());
				continue;
			}
		}
		std::string strConnection;
		std::vector<std::pair<std::uint16_t, std::int32_t>> aShard;
		const bool bRequireOwner =
			enTopic == EN_MT_GATEWAY_WEB_TOPIC_QUOTE ||
			enTopic == EN_MT_GATEWAY_WEB_TOPIC_PROFIT ||
			enTopic == EN_MT_GATEWAY_WEB_TOPIC_KLINE;
		if (bRequireOwner)
		{
			aShard.push_back(std::make_pair(
				static_cast<std::uint16_t>(p_iVersion), p_iNo));
		}
		if (!pBackendPool->SelectReady(
			stApi.strLogicalService,
			std::string(), aShard, bRequireOwner, strConnection))
		{
			WEB_WARN(
				"web snapshot backend unavailable,topic=%s",
				GetMtGatewayWebTopicName(enTopic));
			continue;
		}

		// 第二步：提交异步查询；回调上下文冻结连接代次，断线后结果自动丢弃。
		ST_MT_GATEWAY_SNAPSHOT_CALLBACK* pCallback =
			new (std::nothrow) ST_MT_GATEWAY_SNAPSHOT_CALLBACK;
		if (pCallback == nullptr)
		{
			continue;
		}
		pCallback->stTarget = p_refTarget;
		pCallback->enTopic = enTopic;
		pCallback->iVersion = p_iVersion;
		pCallback->iNo = p_iNo;
		pCallback->uiAttempt = p_uiSnapshotAttempt;
		pCallback->stApi = stApi;
		pCallback->bOpenPriceBootstrap =
			enTopic == EN_MT_GATEWAY_WEB_TOPIC_QUOTE &&
			!p_bOpenPriceReady;
		pCallback->setLogin = p_refLogins;
		pCallback->setSymbol = p_refSymbols;
		ST_CLOUD_NET_BINARY_CALL stCall;
		stCall.lSynId = GetTimestampMs();
		stCall.lFuncId = llFuncId;
		stCall.stPayload.pBuffer =
			aPayload.empty() ? nullptr : aPayload.data();
		stCall.stPayload.iLen =
			static_cast<int>(aPayload.size());
		const long long llResult = CallBinaryAsync(hCloudNetApi,
			strConnection.c_str(), &stCall,
			SnapshotResultCallback, pCallback);
		if (llResult < 0)
		{
			delete pCallback;
			const char* pError = GetLastErrorDetail(hCloudNetApi);
			WEB_WARN(
				"web snapshot submit failed,topic=%s,connection=%s,code=%d,detail=%s",
				GetMtGatewayWebTopicName(enTopic), strConnection.c_str(),
				GetLastErrorCode(hCloudNetApi),
				pError != nullptr ? pError : "unknown");
		}
	}
}

void CMtGatewayWebService::HandleOpenPriceBootstrapResult(
	const ST_MT_GATEWAY_WEB_TARGET& p_refTarget,
	std::int32_t p_iVersion, std::int32_t p_iNo,
	std::uint32_t p_uiSnapshotAttempt,
	const std::set<std::int64_t>& p_refLogins,
	const std::set<std::string>& p_refSymbols,
	const ST_CLOUD_NET_BINARY_RESULT* p_pResult)
{
	if (!m_clSessionManager.IsSnapshotAttemptCurrent(p_refTarget,
			EN_MT_GATEWAY_WEB_TOPIC_QUOTE, p_iVersion, p_iNo,
			p_uiSnapshotAttempt))
	{
		return;
	}
	ST_DERIVE_M1_SNAPSHOT_RESPONSE stM1;
	std::string strError;
	if (p_pResult == nullptr || p_pResult->iErrorCode != 0 ||
		p_pResult->lRetVal < 0 || p_pResult->stPayload.iLen < 0 ||
		!DecodeDeriveM1SnapshotResponse(p_pResult->stPayload.pBuffer,
			static_cast<std::size_t>(p_pResult->stPayload.iLen),
			stM1, strError) || stM1.iCode != EN_TERMINAL_ERROR_OK ||
		stM1.enState != EN_DERIVE_SERVICE_STATE_READY)
	{
		RetrySnapshot(p_refTarget, EN_MT_GATEWAY_WEB_TOPIC_QUOTE,
			p_iVersion, p_iNo, "open_price_snapshot_failed");
		return;
	}
	for (const ST_DERIVE_M1_BAR& refBar : stM1.aBar)
	{
		if (refBar.usPlatformVersion == p_iVersion &&
			refBar.iSourceNo == p_iNo)
		{
			UpdateOpenPriceFromM1(refBar);
		}
	}
	SubmitInitialSnapshots(p_refTarget,
		EN_MT_GATEWAY_WEB_TOPIC_QUOTE, p_refLogins, p_refSymbols,
		p_iVersion, p_iNo, p_uiSnapshotAttempt, true);
}

void CMtGatewayWebService::SubmitSnapshotTask(
	const ST_MT_GATEWAY_SNAPSHOT_TASK& p_refTask)
{
	const ST_MT_GATEWAY_WEB_SUBSCRIPTION& refSubscription =
		p_refTask.stSubscription;
	SubmitInitialSnapshots(p_refTask.stTarget,
		refSubscription.ullTopic, refSubscription.setLogin,
		refSubscription.setSymbol, refSubscription.iVersion,
		refSubscription.iNo, refSubscription.uiSnapshotAttempt);
}

void CMtGatewayWebService::RetrySnapshot(
	const ST_MT_GATEWAY_WEB_TARGET& p_refTarget,
	EN_MT_GATEWAY_WEB_TOPIC p_enTopic,
	std::int32_t p_iVersion, std::int32_t p_iNo,
	const char* p_szReason)
{
	ST_MT_GATEWAY_SNAPSHOT_TASK stTask;
	std::string strError;
	if (!m_clSessionManager.PrepareSnapshotRetry(p_refTarget,
			p_enTopic, p_iVersion, p_iNo, stTask, strError))
	{
		if (m_clSessionManager.IsCurrent(p_refTarget))
		{
			WEB_WARN(
				"web snapshot retry exhausted,topic=%s,version=%d,no=%d,reason=%s,detail=%s",
				GetMtGatewayWebTopicName(p_enTopic), p_iVersion, p_iNo,
				p_szReason != nullptr ? p_szReason : "unknown",
				strError.c_str());
			CloseTarget(p_refTarget,
				"resync_required_snapshot_retry_exhausted");
		}
		return;
	}
	WEB_WARN(
		"web snapshot retry submitted,topic=%s,version=%d,no=%d,attempt=%u,reason=%s",
		GetMtGatewayWebTopicName(p_enTopic), p_iVersion, p_iNo,
		stTask.stSubscription.uiSnapshotAttempt,
		p_szReason != nullptr ? p_szReason : "unknown");
	SubmitSnapshotTask(stTask);
}

void CMtGatewayWebService::HandleSnapshotResult(
	const ST_MT_GATEWAY_WEB_TARGET& p_refTarget,
	EN_MT_GATEWAY_WEB_TOPIC p_enTopic,
	std::int32_t p_iVersion, std::int32_t p_iNo,
	std::uint32_t p_uiSnapshotAttempt,
	const ST_MT_GATEWAY_API_DESCRIPTOR& p_refApi,
	const ST_CLOUD_NET_BINARY_RESULT* p_pResult)
{
	if (!m_clSessionManager.IsSnapshotAttemptCurrent(p_refTarget,
			p_enTopic, p_iVersion, p_iNo, p_uiSnapshotAttempt))
	{
		return;
	}
	if (p_pResult == nullptr || p_pResult->iErrorCode != 0 ||
		p_pResult->lRetVal < 0 || p_pResult->stPayload.iLen < 0)
	{
		RetrySnapshot(p_refTarget, p_enTopic,
			p_iVersion, p_iNo, "snapshot_rpc_failed");
		return;
	}
	if (p_enTopic == EN_MT_GATEWAY_WEB_TOPIC_QUOTE)
	{
		ST_QUOTE_SNAPSHOT_BINARY_RESPONSE stQuote;
		std::string strError;
		if (!DecodeQuoteSnapshotBinaryResponse(
				p_pResult->stPayload.pBuffer,
				static_cast<std::size_t>(p_pResult->stPayload.iLen),
				stQuote, strError) ||
			stQuote.usPlatformVersion != p_iVersion ||
			stQuote.iSourceNo != p_iNo)
		{
			RetrySnapshot(p_refTarget, p_enTopic,
				p_iVersion, p_iNo, "snapshot_decode_failed");
			return;
		}
		std::set<std::string> setSymbol;
		for (const ST_MT_GATEWAY_WEB_SUBSCRIPTION& refSubscription :
			m_clSessionManager.GetSubscriptions(p_refTarget))
		{
			if (refSubscription.ullTopic ==
					EN_MT_GATEWAY_WEB_TOPIC_QUOTE &&
				refSubscription.iVersion == p_iVersion &&
				refSubscription.iNo == p_iNo &&
				refSubscription.uiSnapshotAttempt == p_uiSnapshotAttempt)
			{
				setSymbol = refSubscription.setSymbol;
				break;
			}
		}
		if (!m_clSessionManager.BeginSnapshotDelivery(
				p_refTarget, p_enTopic, p_iVersion, p_iNo,
				stQuote.ullHighWatermark, stQuote.ullSourceEpoch,
				p_uiSnapshotAttempt, strError))
		{
			return;
		}
		nlohmann::json clQuote = nlohmann::json::array();
		for (const ST_QUOTE_BINARY_TICK& refTick : stQuote.aTick)
		{
			if (!setSymbol.empty() &&
				setSymbol.find(refTick.strSymbol) == setSymbol.end())
			{
				continue;
			}
			nlohmann::json clItem = nlohmann::json::object();
			clItem["Version"] = refTick.usPlatformVersion;
			clItem["No"] = refTick.iSourceNo;
			clItem["Symbol"] = refTick.strSymbol;
			clItem["Bid"] = refTick.dBid;
			clItem["Ask"] = refTick.dAsk;
			clItem["Last"] = refTick.dLast;
			clItem["Volume"] = refTick.ullVolume;
			clItem["VolumeExt"] = refTick.ullVolumeExt;
			clItem["Flags"] = refTick.ullFlags;
			clItem["Time"] = refTick.llServerTime;
			clItem["TimeMsc"] = refTick.llServerTimeMsc;
			const double dM1OpenPrice = GetOpenPrice(
				refTick.usPlatformVersion, refTick.iSourceNo,
				refTick.strSymbol);
			clItem["OpenPrice"] = dM1OpenPrice > 0.0 ? dM1OpenPrice : 0.0;
			clItem["ClosePrice"] = 0.0;
			clItem["LowPrice"] = 0.0;
			clItem["HighPrice"] = 0.0;
			clItem["PrevClosedPrice"] = 0.0;
			clQuote.push_back(clItem);
		}
		nlohmann::json clData = nlohmann::json::object();
		clData["Topic"] = static_cast<std::uint64_t>(p_enTopic);
		clData["Snapshot"] = true;
		clData["SourceEpoch"] = stQuote.ullSourceEpoch;
		clData["Sequence"] = stQuote.ullHighWatermark;
		clData["Data"] = clQuote;
		if (!m_clPushDispatcher.Enqueue(p_enTopic, p_refTarget,
				BuildWebResponse(EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION,
					EN_TERMINAL_ERROR_OK, "OK", clData)))
		{
			CloseTarget(p_refTarget,
				"resync_required_snapshot_enqueue_failed");
			return;
		}
		CompleteSnapshotBarrier(p_refTarget, p_enTopic,
			p_iVersion, p_iNo, stQuote.ullHighWatermark,
			stQuote.ullSourceEpoch, p_uiSnapshotAttempt);
		return;
	}
	if (p_enTopic == EN_MT_GATEWAY_WEB_TOPIC_KLINE)
	{
		ST_DERIVE_M1_SNAPSHOT_RESPONSE stM1;
		std::string strError;
		if (!DecodeDeriveM1SnapshotResponse(
				p_pResult->stPayload.pBuffer,
				static_cast<std::size_t>(p_pResult->stPayload.iLen),
				stM1, strError))
		{
			WEB_WARN("web M1 snapshot decode failed,detail=%s",
				strError.c_str());
			RetrySnapshot(p_refTarget, p_enTopic,
				p_iVersion, p_iNo, "snapshot_decode_failed");
			return;
		}
		if (stM1.iCode != EN_TERMINAL_ERROR_OK ||
			stM1.enState != EN_DERIVE_SERVICE_STATE_READY)
		{
			RetrySnapshot(p_refTarget, p_enTopic,
				p_iVersion, p_iNo, "snapshot_backend_not_ready");
			return;
		}
		// 1185 按分钟升序返回；跨 Quote 重启时旧分钟仍可保留，屏障必须使用最新分钟的当前 Epoch。
		const std::uint64_t ullEpoch = stM1.aBar.empty() ? 0U :
			stM1.aBar.back().ullSourceEpoch;
		if (!m_clSessionManager.BeginSnapshotDelivery(
				p_refTarget, p_enTopic, p_iVersion, p_iNo,
				stM1.ullEventSequence, ullEpoch,
				p_uiSnapshotAttempt, strError))
		{
			return;
		}
		nlohmann::json clM1 = nlohmann::json::object();
		clM1["State"] = static_cast<unsigned int>(stM1.enState);
		clM1["EventSequence"] = stM1.ullEventSequence;
		clM1["Bars"] = nlohmann::json::array();
		for (const ST_DERIVE_M1_BAR& refBar : stM1.aBar)
		{
			UpdateOpenPriceFromM1(refBar);
			clM1["Bars"].push_back(BuildM1BarForWeb(refBar));
		}
		nlohmann::json clData = nlohmann::json::object();
		clData["Topic"] = static_cast<std::uint64_t>(p_enTopic);
		clData["Snapshot"] = true;
		clData["Sequence"] = stM1.ullEventSequence;
		clData["Data"] = clM1;
		if (!m_clPushDispatcher.Enqueue(p_enTopic, p_refTarget,
				BuildWebResponse(EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION,
					stM1.iCode, stM1.strMessage, clData)))
		{
			CloseTarget(p_refTarget,
				"resync_required_snapshot_enqueue_failed");
			return;
		}
		CompleteSnapshotBarrier(p_refTarget, p_enTopic,
			p_iVersion, p_iNo, stM1.ullEventSequence, ullEpoch,
			p_uiSnapshotAttempt);
		return;
	}
	if (p_enTopic ==
		EN_MT_GATEWAY_WEB_TOPIC_PROFIT)
	{
		// Profit 快照使用 Derive 1184 原生应答，显式转换为 Web JSON；BOOTSTRAPPING 也要返回明确 Code/Msg。
		ST_DERIVE_PROFIT_SNAPSHOT_RESPONSE stProfit;
		std::string strError;
		if (!DecodeDeriveProfitSnapshotResponse(
				p_pResult->stPayload.pBuffer,
				static_cast<std::size_t>(
					p_pResult->stPayload.iLen),
				stProfit, strError))
		{
			WEB_WARN(
				"web profit snapshot decode failed,detail=%s",
				strError.c_str());
			RetrySnapshot(p_refTarget, p_enTopic,
				p_iVersion, p_iNo, "snapshot_decode_failed");
			return;
		}
		if (stProfit.iCode != EN_TERMINAL_ERROR_OK ||
			stProfit.enState != EN_DERIVE_SERVICE_STATE_READY)
		{
			RetrySnapshot(p_refTarget, p_enTopic,
				p_iVersion, p_iNo, "snapshot_backend_not_ready");
			return;
		}
		if (!m_clSessionManager.BeginSnapshotDelivery(
				p_refTarget, p_enTopic, p_iVersion, p_iNo,
				stProfit.ullEventSequence, 0,
				p_uiSnapshotAttempt, strError))
		{
			return;
		}
		nlohmann::json clProfit =
			nlohmann::json::object();
		clProfit["State"] =
			static_cast<unsigned int>(
				stProfit.enState);
		clProfit["EventSequence"] = stProfit.ullEventSequence;
		clProfit["Accounts"] =
			nlohmann::json::array();
		for (const ST_DERIVE_ACCOUNT_PROFIT& refAccount :
			stProfit.aAccount)
		{
			nlohmann::json clItem =
				nlohmann::json::object();
			clItem["Version"] =
				refAccount.usPlatformVersion;
			clItem["No"] = refAccount.iSourceNo;
			clItem["Login"] = refAccount.llLogin;
			clItem["StateVersion"] =
				refAccount.ullStateVersion;
			clItem["Balance"] = refAccount.dBalance;
			clItem["Credit"] = refAccount.dCredit;
			clItem["Profit"] = refAccount.dProfit;
			clItem["Floating"] =
				refAccount.dFloating;
			clItem["Equity"] = refAccount.dEquity;
			clItem["Margin"] = refAccount.dMargin;
			clItem["MarginFree"] =
				refAccount.dMarginFree;
			clItem["MarginLevel"] =
				refAccount.dMarginLevel;
			clItem["Timestamp"] =
				refAccount.llTimestampMs;
			clProfit["Accounts"].push_back(
				clItem);
		}
		clProfit["Positions"] =
			nlohmann::json::array();
		for (const ST_DERIVE_POSITION_PROFIT& refPosition :
			stProfit.aPosition)
		{
			nlohmann::json clItem =
				nlohmann::json::object();
			clItem["Version"] =
				refPosition.usPlatformVersion;
			clItem["No"] = refPosition.iSourceNo;
			clItem["Login"] = refPosition.llLogin;
			clItem["Position"] =
				refPosition.ullPositionId;
			clItem["StateVersion"] =
				refPosition.ullStateVersion;
			clItem["Symbol"] =
				refPosition.strSymbol;
			clItem["Side"] = refPosition.usSide;
			clItem["Volume"] =
				refPosition.dVolume;
			clItem["OpenPrice"] =
				refPosition.dOpenPrice;
			clItem["CurrentPrice"] =
				refPosition.dCurrentPrice;
			clItem["Profit"] =
				refPosition.dProfit;
			clItem["Storage"] =
				refPosition.dStorage;
			clItem["Commission"] =
				refPosition.dCommission;
			clItem["Tax"] = refPosition.dTax;
			clItem["Timestamp"] =
				refPosition.llTimestampMs;
			clProfit["Positions"].push_back(
				clItem);
		}
		nlohmann::json clData =
			nlohmann::json::object();
		clData["Topic"] =
			static_cast<std::uint64_t>(
				p_enTopic);
		clData["Snapshot"] = true;
		clData["Data"] = clProfit;
		const std::string strMessage =
			BuildWebResponse(
				EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION,
				stProfit.iCode,
				stProfit.strMessage, clData);
		if (!m_clPushDispatcher.Enqueue(
				p_enTopic, p_refTarget, strMessage))
		{
			CloseTarget(p_refTarget,
				"resync_required_snapshot_enqueue_failed");
			return;
		}
		CompleteSnapshotBarrier(p_refTarget, p_enTopic,
			p_iVersion, p_iNo, stProfit.ullEventSequence, 0,
			p_uiSnapshotAttempt);
		return;
	}
	CMtGatewayRequestAdapter clAdapter;
	ST_MT_GATEWAY_API_RESPONSE stResponse;
	std::string strError;
	if (!clAdapter.DecodeResponse(p_refApi,
		p_pResult->stPayload.pBuffer,
		static_cast<std::size_t>(p_pResult->stPayload.iLen),
		stResponse, strError))
	{
		RetrySnapshot(p_refTarget, p_enTopic,
			p_iVersion, p_iNo, "snapshot_decode_failed");
		return;
	}
	if (stResponse.iCode != EN_TERMINAL_ERROR_OK)
	{
		RetrySnapshot(p_refTarget, p_enTopic,
			p_iVersion, p_iNo, "snapshot_backend_not_ready");
		return;
	}
	if (!m_clSessionManager.BeginSnapshotDelivery(
			p_refTarget, p_enTopic, p_iVersion, p_iNo,
			0, 0, p_uiSnapshotAttempt, strError))
	{
		return;
	}
	nlohmann::json clData = nlohmann::json::object();
	clData["Topic"] = static_cast<std::uint64_t>(p_enTopic);
	clData["Snapshot"] = true;
	clData["Data"] = stResponse.clData;
	const std::string strMessage = BuildWebResponse(
		EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION,
		stResponse.iCode, stResponse.strMessage, clData);
	if (!m_clPushDispatcher.Enqueue(p_enTopic, p_refTarget, strMessage))
	{
		CloseTarget(p_refTarget,
			"resync_required_snapshot_enqueue_failed");
		return;
	}
	// Query 快照尚无统一事件水位，先发送快照再完整回放缓冲，仍可避免旧快照覆盖新增量。
	CompleteSnapshotBarrier(p_refTarget, p_enTopic,
		p_iVersion, p_iNo, 0, 0, p_uiSnapshotAttempt);
}

void CMtGatewayWebService::CompleteSnapshotBarrier(
	const ST_MT_GATEWAY_WEB_TARGET& p_refTarget,
	EN_MT_GATEWAY_WEB_TOPIC p_enTopic,
	std::int32_t p_iVersion, std::int32_t p_iNo,
	std::uint64_t p_ullWatermark,
	std::uint64_t p_ullSourceEpoch,
	std::uint32_t p_uiSnapshotAttempt)
{
	for (;;)
	{
		std::vector<ST_MT_GATEWAY_PENDING_INCREMENT> aPending;
		bool bLive = false;
		std::string strError;
		if (!m_clSessionManager.DrainSnapshotBuffer(p_refTarget,
				p_enTopic, p_iVersion, p_iNo, p_ullWatermark,
				p_ullSourceEpoch, p_uiSnapshotAttempt,
				aPending, bLive, strError))
		{
			WEB_WARN("web snapshot barrier failed,topic=%s,detail=%s",
				GetMtGatewayWebTopicName(p_enTopic), strError.c_str());
			CloseTarget(p_refTarget,
				"resync_required_snapshot_barrier_failed");
			return;
		}
		for (const ST_MT_GATEWAY_PENDING_INCREMENT& refPending : aPending)
		{
			ST_MT_GATEWAY_PUSH_CONTEXT stContext;
			stContext.usVersion = static_cast<std::uint16_t>(
				refPending.iVersion);
			stContext.iNo = refPending.iNo;
			stContext.strSymbol = refPending.strSymbol;
			stContext.ullSequence = refPending.ullSequence;
			const std::vector<ST_MT_GATEWAY_WEB_TARGET> aTarget(
				1U, p_refTarget);
			if (!m_clPushDispatcher.EnqueueBatch(p_enTopic,
					aTarget, refPending.refMessage, stContext))
			{
				CloseTarget(p_refTarget,
					"resync_required_snapshot_replay_failed");
				return;
			}
		}
		if (bLive)
		{
			return;
		}
	}
}

bool CMtGatewayWebService::SendControlResponse(
	const ST_MT_GATEWAY_WEB_TARGET& p_refTarget,
	std::int32_t p_iFuncId, int p_iCode,
	const std::string& p_refMessage, const nlohmann::json& p_refData)
{
	// 控制应答也进入连接固定 Sender，避免与异步快照或实时增量并发写同一 Socket。
	return m_clPushDispatcher.Enqueue(
		EN_MT_GATEWAY_WEB_TOPIC_NONE, p_refTarget,
		BuildWebResponse(p_iFuncId, p_iCode,
			p_refMessage, p_refData));
}

bool CMtGatewayWebService::SendPush(
	const ST_MT_GATEWAY_WEB_TARGET& p_refTarget,
	const std::string& p_refMessage)
{
	CSocketServer* pServer = nullptr;
	{
		std::lock_guard<std::mutex> clLock(m_clMutex);
		pServer = m_pServer;
	}
	if (pServer == nullptr ||
		!m_clSessionManager.IsCurrent(p_refTarget) ||
		!pServer->WebSockIsAlive(p_refTarget.stKey.pServerHandle,
			p_refTarget.stKey.pClientHandle))
	{
		return false;
	}
	return pServer->WebSockSend(
		p_refTarget.stKey.pServerHandle,
		p_refTarget.stKey.pClientHandle,
		p_refMessage.c_str(),
		static_cast<int>(p_refMessage.size()));
}

void CMtGatewayWebService::CloseTarget(
	const ST_MT_GATEWAY_WEB_TARGET& p_refTarget, const char* p_szReason)
{
	// 异步快照和 Sender 回调可能晚于句柄复用；旧代次不得删除或关闭新会话。
	if (!m_clSessionManager.IsCurrent(p_refTarget))
	{
		return;
	}
	CSocketServer* pServer = nullptr;
	{
		std::lock_guard<std::mutex> clLock(m_clMutex);
		pServer = m_pServer;
	}
	if (pServer == nullptr)
	{
		return;
	}
	const char* pReason = p_szReason != nullptr ?
		p_szReason : "web_connection_closed";
	if (!m_clSessionManager.RemoveConnection(p_refTarget))
	{
		return;
	}
	m_clPushDispatcher.RemoveTarget(p_refTarget);
	MarkProfitDemandDirty();
	pServer->WebSockClose(p_refTarget.stKey.pServerHandle,
		p_refTarget.stKey.pClientHandle, pReason,
		static_cast<int>(std::strlen(pReason)));
}

void CMtGatewayWebService::SweepThread()
{
	std::chrono::steady_clock::time_point clNextProfitRenew =
		std::chrono::steady_clock::now();
	for (;;)
	{
		{
			std::unique_lock<std::mutex> clLock(m_clMutex);
			if (m_clCondition.wait_for(clLock,
				std::chrono::seconds(
					m_stConfig.iSweepIntervalSec),
				[this]()
				{
					return m_bSweepStop ||
						m_bProfitDemandDirty;
				}) && m_bSweepStop)
			{
				break;
			}
		}
		const std::chrono::steady_clock::time_point clNow =
			std::chrono::steady_clock::now();
		bool bDemandDirty = false;
		unsigned int uiRenewMs = 0;
		{
			std::lock_guard<std::mutex> clLock(
				m_clMutex);
			bDemandDirty = m_bProfitDemandDirty;
			m_bProfitDemandDirty = false;
			uiRenewMs =
				m_stConfig.uiProfitLeaseRenewMs;
		}
		if (bDemandDirty ||
			clNow >= clNextProfitRenew)
		{
			SynchronizeProfitDemands();
			clNextProfitRenew =
				clNow + std::chrono::milliseconds(
					uiRenewMs);
		}
		for (const ST_MT_GATEWAY_WEB_TARGET& refTarget :
			m_clSessionManager.GetIdleTargets(
				clNow,
				m_stConfig.iIdleTimeoutSec))
		{
			WEB_WARN(
				"web idle connection closed,generation=%llu,timeoutSec=%d",
				static_cast<unsigned long long>(refTarget.ullGeneration),
				m_stConfig.iIdleTimeoutSec);
			CloseTarget(refTarget, "web_idle_timeout");
		}
		for (const ST_MT_GATEWAY_SNAPSHOT_TASK& refTask :
			m_clSessionManager.GetSnapshotFailureTasks(
				clNow, m_stConfig.uiSnapshotTimeoutMs))
		{
			WEB_WARN(
				"web snapshot attempt timed out,generation=%llu,topic=%s,attempt=%u,timeoutMs=%u",
				static_cast<unsigned long long>(
					refTask.stTarget.ullGeneration),
				GetMtGatewayWebTopicName(
					static_cast<EN_MT_GATEWAY_WEB_TOPIC>(
						refTask.stSubscription.ullTopic)),
				refTask.stSubscription.uiSnapshotAttempt,
				m_stConfig.uiSnapshotTimeoutMs);
			RetrySnapshot(refTask.stTarget,
				static_cast<EN_MT_GATEWAY_WEB_TOPIC>(
					refTask.stSubscription.ullTopic),
				refTask.stSubscription.iVersion,
				refTask.stSubscription.iNo,
				"snapshot_timeout_or_buffer_overflow");
		}
	}
}

void CMtGatewayWebService::SynchronizeProfitDemands()
{
	HCLOUD_NET_API hCloudNetApi = nullptr;
	ST_MT_GATEWAY_WEB_CONFIG stConfig;
	std::string strGatewayInstance;
	std::uint64_t ullGatewayEpoch = 0;
	std::map<std::pair<std::int32_t, std::int32_t>,
		std::set<std::int64_t>> mapPrevious;
	{
		std::lock_guard<std::mutex> clLock(
			m_clMutex);
		hCloudNetApi = m_hCloudNetApi;
		stConfig = m_stConfig;
		strGatewayInstance =
			m_strGatewayInstance;
		ullGatewayEpoch =
			m_ullGatewayEpoch;
		mapPrevious =
			m_mapLastProfitDemand;
	}
	if (hCloudNetApi == nullptr ||
		strGatewayInstance.empty() ||
		ullGatewayEpoch == 0)
	{
		return;
	}

	// 第一步：从会话表重建绝对 Login 集合，并合并上轮来源以便为已经消失的订阅发送 DELETE。
	std::map<std::pair<std::int32_t, std::int32_t>,
		std::set<std::int64_t>> mapDemand;
	m_clSessionManager.GetProfitDemands(mapDemand);
	std::set<std::pair<std::int32_t, std::int32_t>>
		setSource;
	for (const std::pair<const std::pair<
		std::int32_t, std::int32_t>,
		std::set<std::int64_t>>& refItem :
		mapDemand)
	{
		setSource.insert(refItem.first);
	}
	for (const std::pair<const std::pair<
		std::int32_t, std::int32_t>,
		std::set<std::int64_t>>& refItem :
		mapPrevious)
	{
		setSource.insert(refItem.first);
	}

	// 第二步：每个 Version+No 使用独立 REPLACE/DELETE；失败键保留上轮状态并在下一次续约重试。
	for (const std::pair<std::int32_t,
		std::int32_t>& refSource : setSource)
	{
		const std::map<std::pair<std::int32_t,
			std::int32_t>, std::set<std::int64_t>>::
			const_iterator itDemand =
			mapDemand.find(refSource);
		ST_DERIVE_PROFIT_DEMAND_REQUEST stRequest;
		stRequest.enOperation =
			itDemand != mapDemand.end() ?
			EN_DERIVE_PROFIT_DEMAND_REPLACE :
			EN_DERIVE_PROFIT_DEMAND_DELETE;
		stRequest.usPlatformVersion =
			static_cast<std::uint16_t>(
				refSource.first);
		stRequest.iSourceNo =
			refSource.second;
		stRequest.strGatewayInstance =
			strGatewayInstance;
		stRequest.ullGatewayEpoch =
			ullGatewayEpoch;
		{
			std::lock_guard<std::mutex> clLock(
				m_clMutex);
			++m_ullProfitRevision;
			if (m_ullProfitRevision == 0)
			{
				++m_ullProfitRevision;
			}
			stRequest.ullRevision =
				m_ullProfitRevision;
		}
		stRequest.uiLeaseMs =
			stRequest.enOperation ==
				EN_DERIVE_PROFIT_DEMAND_REPLACE ?
			stConfig.uiProfitLeaseMs : 0;
		if (itDemand != mapDemand.end())
		{
			stRequest.aLogin.assign(
				itDemand->second.begin(),
				itDemand->second.end());
		}
		std::vector<unsigned char> aRequest;
		std::string strError;
		if (!EncodeDeriveProfitDemandRequest(
				stRequest, aRequest, strError))
		{
			WEB_ERROR(
				"profit demand encode failed,version=%d,no=%d,revision=%llu,detail=%s",
				refSource.first, refSource.second,
				static_cast<unsigned long long>(
					stRequest.ullRevision),
				strError.c_str());
			continue;
		}
		ST_CLOUD_NET_BINARY_CALL stCall;
		stCall.lSynId =
			static_cast<long long>(
				stRequest.ullRevision);
		stCall.lFuncId =
			EN_PLUGIN_FUNC_DERIVE_PROFIT_DEMAND;
		stCall.stPayload.pBuffer =
			aRequest.data();
		stCall.stPayload.iLen =
			static_cast<int>(aRequest.size());
		ST_CLOUD_NET_BINARY_RESULT* pResult =
			CallBinarySync(hCloudNetApi,
				"MtDeriveService", &stCall);
		const std::unique_ptr<
			ST_CLOUD_NET_BINARY_RESULT,
			void (*)(ST_CLOUD_NET_BINARY_RESULT*)>
			clResult(pResult, FreeBinaryResult);
		ST_DERIVE_PROFIT_DEMAND_RESPONSE stResponse;
		if (pResult == nullptr ||
			pResult->iErrorCode != 0 ||
			pResult->lRetVal < 0 ||
			pResult->stPayload.iLen <= 0 ||
			pResult->stPayload.pBuffer == nullptr ||
			!DecodeDeriveProfitDemandResponse(
				pResult->stPayload.pBuffer,
				static_cast<std::size_t>(
					pResult->stPayload.iLen),
				stResponse, strError) ||
			stResponse.iCode !=
				EN_TERMINAL_ERROR_OK)
		{
			if (pResult == nullptr)
			{
				const char* pError =
					GetLastErrorDetail(
						hCloudNetApi);
				strError =
					"PROFIT_DEMAND_CALL_FAILED: code=" +
					std::to_string(
						GetLastErrorCode(
							hCloudNetApi)) +
					", detail=" +
					(pError != nullptr ?
						pError : "unknown");
			}
			else if (strError.empty())
			{
				strError =
					"PROFIT_DEMAND_REMOTE_FAILED: code=" +
					std::to_string(
						stResponse.iCode) +
					", detail=" +
					(stResponse.strMessage.empty() ?
						pResult->szErrInfo :
						stResponse.strMessage);
			}
			WEB_WARN(
				"profit demand sync failed,version=%d,no=%d,revision=%llu,operation=%u,detail=%s",
				refSource.first, refSource.second,
				static_cast<unsigned long long>(
					stRequest.ullRevision),
				static_cast<unsigned int>(
					stRequest.enOperation),
				strError.c_str());
			continue;
		}

		// 第三步：只在远端明确接受后更新本地成功快照；DELETE 成功立即移除旧来源。
		std::lock_guard<std::mutex> clLock(
			m_clMutex);
		if (itDemand != mapDemand.end())
		{
			m_mapLastProfitDemand[refSource] =
				itDemand->second;
		}
		else
		{
			m_mapLastProfitDemand.erase(
				refSource);
		}
	}
}

void CMtGatewayWebService::MarkProfitDemandDirty()
{
	{
		std::lock_guard<std::mutex> clLock(
			m_clMutex);
		m_bProfitDemandDirty = true;
	}
	m_clCondition.notify_all();
}

bool CMtGatewayWebService::BeginCallback(CSocketServer*& p_refServer)
{
	std::lock_guard<std::mutex> clLock(m_clMutex);
	p_refServer = m_pServer;
	if (m_pServer == nullptr)
	{
		return false;
	}
	++m_iActiveCallbacks;
	return true;
}

void CMtGatewayWebService::EndCallback()
{
	std::lock_guard<std::mutex> clLock(m_clMutex);
	if (m_iActiveCallbacks > 0)
	{
		--m_iActiveCallbacks;
	}
	if (m_iActiveCallbacks == 0)
	{
		m_clCondition.notify_all();
	}
}
