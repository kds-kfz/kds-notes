#include "ClientDataBinaryProtocol.h"
#include "CPostgreSqlPool.h"
#include "ChartConfigRepository.h"
#include "ChartDrawingRepository.h"
#include "ChartIndicatorRepository.h"
#include "ClusterBinaryProtocol.h"
#include "CodeMsg.h"
#include "DeriveBinaryProtocol.h"
#include "EventHeartbeatBinaryProtocol.h"
#include "MtGatewayApiCatalog.h"
#include "MtGatewayBackendPool.h"
#include "MtGatewayPushDispatcher.h"
#include "MtGatewayRequestAdapter.h"
#include "MtGatewayServiceMonitor.h"
#include "MtGatewayWebSessionManager.h"
#include "MtGatewayWebProtocol.h"
#include "MtDeriveConfig.h"
#include "MtDeriveDeadLetter.h"
#include "MtDeriveM1Engine.h"
#include "MtDeriveProfitEngine.h"
#include "MtDeriveProfitLease.h"
#include "MtDeriveStateStore.h"
#include "MtDeriveTickStore.h"
#include "MtDealerStateStore.h"
#include "MtEventConfig.h"
#include "MtMarketEventRelay.h"
#include "MtQueryBinaryJsonBridge.h"
#include "MtQueryBarHistoryStore.h"
#include "MtQueryConfig.h"
#include "MtQueryDispatcher.h"
#include "MtQueryM1ArchiveConsumer.h"
#include "MtQueryQuoteCache.h"
#include "MtQueryTimeZone.h"
#include "MtQuoteConfig.h"
#include "MtQuoteIngressQueue.h"
#include "ServerTimeUtcUtil.h"
#include "SymbolSessionUtcUtil.h"
#include "MtTradeDispatcher.h"
#include "MtTradeIdempotencyStore.h"
#include "MtTradeNodeManager.h"
#include "MtTradeOutbox.h"
#include "MarketStateCache.h"
#include "PluginBinaryProtocol.h"
#include "QueryBinaryProtocol.h"
#include "QuoteBinaryProtocol.h"
#include "QuoteHeartbeatBinaryProtocol.h"
#include "QuoteSnapshotBinaryProtocol.h"
#include "ReliableEventBinaryProtocol.h"
#include "TradeBinaryProtocol.h"
#include "WatchListRepository.h"
#include "AccountTradeStatusUtil.h"
#include "LocalRecordFileHeader.h"
#include "Mt4PositionProfitUtil.h"
#include "Mt5PositionProfitUtil.h"
#include "Mt5SymbolSessionUtil.h"
#include "Mt5ProfitGroupQuoteService.h"
#include "RealtimeTick.h"

#include <atomic>
#include <cstdio>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "nlohmann/json.hpp"

// TestLogStubs.cpp 提供线程安全的 1103 模拟状态，不访问真实 CloudNetDataApi。
void SetClusterProbeState(const char* p_szConnection,
	int p_iMode, std::int32_t p_iNo);

namespace
{
	// 测试目录守卫只删除当前测试创建的临时子目录，提前返回时也不会遗留恢复文件。
	class CTestDirectoryGuard
	{
	public:
		explicit CTestDirectoryGuard(
			const std::filesystem::path& p_refPath)
			: m_clPath(p_refPath)
		{
			std::error_code stError;
			std::filesystem::remove_all(
				m_clPath, stError);
		}

		~CTestDirectoryGuard()
		{
			std::error_code stError;
			std::filesystem::remove_all(
				m_clPath, stError);
		}

	private:
		std::filesystem::path m_clPath; // 当前测试独占的临时目录。
	};

	// 测试替身只验证 Dispatcher 与 ClientData 边界，不访问 PostgreSQL 或 Repository。
	class CTestClientDataService :
		public IMtQueryClientDataService
	{
	public:
		CTestClientDataService()
			: bDispatched(false)
		{
		}

		// 记录 1165/1166 转发并返回固定对象；测试线程单线程调用。
		virtual bool Dispatch(std::int64_t p_llFuncId,
			std::int64_t p_llRouteCode,
			const ST_PLUGIN_BINARY_VALUE& p_refRequest,
			std::int32_t& p_refCode,
			std::string& p_refMessage,
			std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& p_refData,
			std::string& p_refError) override
		{
			bDispatched =
				p_llFuncId ==
					EN_PLUGIN_FUNC_QUERY_WATCHLIST &&
				p_llRouteCode ==
					EN_PLUGIN_WATCHLIST_ROUTE_SYMBOLS &&
				p_refRequest.enType ==
					EN_PLUGIN_BINARY_VALUE_OBJECT;
			p_refCode = EN_TERMINAL_ERROR_OK;
			p_refMessage = "OK";
			p_refError.clear();
			p_refData = CreatePluginBinaryValue(
				EN_PLUGIN_BINARY_VALUE_OBJECT);
			return p_refData != nullptr;
		}

		// 返回 READY 状态，验证 1152 不再硬编码数据库状态。
		virtual void GetStatus(
			ST_MT_QUERY_DATABASE_STATUS& p_refStatus) const override
		{
			p_refStatus.enState =
				EN_MT_QUERY_DATABASE_READY;
			p_refStatus.strDetail =
				"POSTGRES_SCHEMA_READY: test";
			p_refStatus.uiTotalConnections = 2;
			p_refStatus.uiIdleConnections = 1;
		}

		bool bDispatched; // 是否收到预期的 WatchList 调用。
	};

	// 递归检查对象和数组字段，用于确认可靠事件中没有嵌套敏感数据。
	bool ContainsBinaryField(
		const ST_PLUGIN_BINARY_VALUE& p_refValue,
		const std::string& p_refName)
	{
		if (p_refValue.enType ==
			EN_PLUGIN_BINARY_VALUE_OBJECT)
		{
			for (const ST_PLUGIN_BINARY_FIELD& refField :
				p_refValue.aObjectField)
			{
				if (refField.strName == p_refName)
				{
					return true;
				}
				if (refField.refValue &&
					ContainsBinaryField(
						*refField.refValue,
						p_refName))
				{
					return true;
				}
			}
		}
		else if (p_refValue.enType ==
			EN_PLUGIN_BINARY_VALUE_ARRAY)
		{
			for (const std::shared_ptr<
				ST_PLUGIN_BINARY_VALUE>& refItem :
				p_refValue.aArrayValue)
			{
				if (refItem &&
					ContainsBinaryField(*refItem,
						p_refName))
				{
					return true;
				}
			}
		}
		return false;
	}

	// 为测试节点补齐五个固定交易角色；每个 count 都代表一个独立测试 Adapter。
	void AddTradeTestConnections(
		ST_MT_TRADE_SOURCE_CONFIG& p_refSource)
	{
		const EN_MT_TRADE_CONNECTION_ROLE aRole[] =
		{
			EN_MT_TRADE_CONNECTION_ROLE_TRADE_COMMAND,
			EN_MT_TRADE_CONNECTION_ROLE_TRADE_PRECHECK,
			EN_MT_TRADE_CONNECTION_ROLE_STATE_PUMPING,
			EN_MT_TRADE_CONNECTION_ROLE_CONFIG_PUMPING,
			EN_MT_TRADE_CONNECTION_ROLE_PUMPING_QUERY
		};
		for (const EN_MT_TRADE_CONNECTION_ROLE enRole : aRole)
		{
			ST_MT_TRADE_CONNECTION_CONFIG stConnection;
			stConnection.enRole = enRole;
			stConnection.strId = GetMtTradeConnectionRoleName(enRole);
			if (enRole == EN_MT_TRADE_CONNECTION_ROLE_STATE_PUMPING)
			{
				stConnection.uiModeMask =
					EN_MT_TRADE_CONNECTION_MODE_ORDER |
					EN_MT_TRADE_CONNECTION_MODE_DEAL |
					EN_MT_TRADE_CONNECTION_MODE_POSITION |
					EN_MT_TRADE_CONNECTION_MODE_ACCOUNT |
					EN_MT_TRADE_CONNECTION_MODE_SOURCE_STATUS;
			}
			else if (enRole ==
				EN_MT_TRADE_CONNECTION_ROLE_CONFIG_PUMPING)
			{
				stConnection.uiModeMask =
					EN_MT_TRADE_CONNECTION_MODE_USER |
					EN_MT_TRADE_CONNECTION_MODE_SYMBOL |
					EN_MT_TRADE_CONNECTION_MODE_GROUP;
			}
			p_refSource.aConnection.push_back(stConnection);
		}
	}

	// 为测试查询节点建立相互隔离的普通池和历史池配置。
	void AddQueryTestConnections(
		ST_MT_QUERY_SOURCE_CONFIG& p_refSource)
	{
		ST_MT_QUERY_CONNECTION_CONFIG stOrdinary;
		stOrdinary.strId = "ordinary-test";
		stOrdinary.enRole =
			EN_MT_QUERY_CONNECTION_ROLE_ORDINARY_QUERY;
		p_refSource.aConnection.push_back(stOrdinary);
		ST_MT_QUERY_CONNECTION_CONFIG stHistory;
		stHistory.strId = "history-test";
		stHistory.enRole =
			EN_MT_QUERY_CONNECTION_ROLE_HISTORY_QUERY;
		p_refSource.aConnection.push_back(stHistory);
	}

	// outbox 测试接收器只解码可靠事件，不访问 NATS 或 CloudNetDataApi。
	class CTestTradeOutboxSink :
		public IMtTradeOutboxSink,
		public IMtTradeFenceProvider
	{
	public:
		CTestTradeOutboxSink()
			: uiDelivered(0)
			, bEnvelopeValid(true)
			, bSensitiveDataAbsent(true)
		{
		}

		// 投递线程调用；每个报文必须是 Reliable Event Binary 对象。
		virtual bool DeliverReliableEvent(
			const std::vector<unsigned char>& p_refRequest,
			std::string& p_refError) override
		{
			ST_PLUGIN_BINARY_DOCUMENT stDocument;
			if (!DecodeReliableEventBinaryDocument(
					p_refRequest.data(),
					p_refRequest.size(), stDocument,
					p_refError) ||
				!stDocument.refRoot ||
				FindPluginBinaryField(
					*stDocument.refRoot,
					"EventId") == nullptr ||
				FindPluginBinaryField(
					*stDocument.refRoot,
					"SourceSequence") == nullptr)
			{
				bEnvelopeValid.store(false);
				return false;
			}
			const ST_PLUGIN_BINARY_VALUE* pPayload =
				FindPluginBinaryField(
					*stDocument.refRoot, "Payload");
			if (pPayload != nullptr &&
				pPayload->enType ==
					EN_PLUGIN_BINARY_VALUE_BYTES)
			{
				ST_PLUGIN_BINARY_DOCUMENT stPayload;
				std::string strPayloadError;
				if (!DecodeTradeBinaryDocument(
						pPayload->aByteValue.data(),
						pPayload->aByteValue.size(),
						stPayload,
						strPayloadError) ||
					!stPayload.refRoot ||
					ContainsBinaryField(
						*stPayload.refRoot,
						"Password"))
				{
					bSensitiveDataAbsent.store(false);
				}
			}
			uiDelivered.fetch_add(1U);
			return true;
		}

		// 单元测试不启动集群，返回空 token 以覆盖 v1 滚动兼容路径。
		virtual bool GetTradeFence(std::uint16_t p_usVersion,
			std::int32_t p_iNo, ST_CLUSTER_FENCE& p_refFence,
			std::string& p_refError) override
		{
			(void)p_usVersion;
			(void)p_iNo;
			p_refFence = ST_CLUSTER_FENCE();
			p_refError.clear();
			return true;
		}

		std::atomic<unsigned int> uiDelivered; // 成功接收的持久化事件数。
		std::atomic<bool> bEnvelopeValid;      // 可靠事件信封是否可完整解码。
		std::atomic<bool> bSensitiveDataAbsent;// 事件正文是否已移除嵌套 Password。
	};

	// 批量调度测试 Adapter 根据 Reject 字段产生成功或明确业务拒绝。
	class CTestPartialTradeAdapter :
		public IMtTradeAdapter
	{
	public:
		CTestPartialTradeAdapter()
			: m_bStarted(false)
			, m_ullNextOrder(70000U)
		{
		}

		virtual ~CTestPartialTradeAdapter()
		{
			Stop();
		}

		// 测试 Adapter 不产生 SDK 回调，只验证节点管理器能够注入事件边界。
		virtual void SetEventSink(
			IMtTradeEventSink* p_pEventSink) override
		{
			(void)p_pEventSink;
		}

		// 测试 Adapter 接受当前节点时间状态，不参与真实服务器时间换算。
		virtual bool UpdateTimeState(
			const ST_MT_TIME_STATE& p_refState,
			std::string& p_refError) override
		{
			(void)p_refState;
			p_refError.clear();
			return true;
		}

		virtual bool Start(
			const ST_MT_TRADE_SOURCE_CONFIG& p_refSource,
			const ST_MT_TRADE_CONNECTION_CONFIG& p_refConnection,
			std::size_t p_szConnectionIndex,
			const ST_MT_TRADE_SERVICE_CONFIG& p_refService,
			std::string& p_refError) override
		{
			(void)p_refSource;
			(void)p_refConnection;
			(void)p_szConnectionIndex;
			(void)p_refService;
			p_refError.clear();
			m_bStarted = true;
			return true;
		}

		virtual void Stop() override
		{
			m_bStarted = false;
		}

		// 测试替身没有外部 Dealer 状态；启动后对账直接成功，未启动时返回明确失败。
		virtual bool ReconcileBeforeReady(
			std::string& p_refError) override
		{
			p_refError.clear();
			if (!m_bStarted)
			{
				p_refError =
					"TRADE_TEST_RECONCILE_NOT_READY";
				return false;
			}
			return true;
		}

		virtual bool Execute(std::int64_t p_llFuncId,
			const ST_PLUGIN_BINARY_VALUE& p_refRequest,
			ST_MT_TRADE_EXECUTION_RESULT& p_refResult,
			std::string& p_refError) override
		{
			(void)p_llFuncId;
			p_refError.clear();
			p_refResult =
				ST_MT_TRADE_EXECUTION_RESULT();
			p_refResult.refData =
				CreatePluginBinaryValue(
					EN_PLUGIN_BINARY_VALUE_OBJECT);
			if (!m_bStarted ||
				!p_refResult.refData)
			{
				p_refError =
					"TRADE_TEST_ADAPTER_NOT_READY";
				return false;
			}
			const ST_PLUGIN_BINARY_VALUE* pReject =
				FindPluginBinaryField(p_refRequest,
					"Reject");
			if (pReject != nullptr &&
				pReject->enType ==
					EN_PLUGIN_BINARY_VALUE_BOOL &&
				pReject->ucBoolValue != 0U)
			{
				p_refResult.iCode =
					EN_TERMINAL_ERROR_MT_TRADE_CHECK_FAILED;
				p_refResult.strMessage =
					"TRADE_TEST_REJECTED";
				return true;
			}
			ST_PLUGIN_BINARY_FIELD stOrder;
			stOrder.strName = "OrderId";
			stOrder.refValue =
				CreatePluginBinaryValue(
					EN_PLUGIN_BINARY_VALUE_UINT64);
			stOrder.refValue->ullUIntValue =
				++m_ullNextOrder;
			p_refResult.refData->aObjectField.push_back(
				stOrder);
			p_refResult.iCode =
				EN_TERMINAL_ERROR_OK;
			p_refResult.strMessage = "OK";
			p_refResult.bSideEffectCommitted = true;
			return true;
		}

		virtual bool Precheck(
			const ST_PLUGIN_BINARY_VALUE& p_refRequest,
			ST_MT_TRADE_EXECUTION_RESULT& p_refResult,
			std::string& p_refError) override
		{
			const bool bResult = Execute(
				EN_PLUGIN_FUNC_TRADE_PLACE_ORDER_DETAIL,
				p_refRequest, p_refResult,
				p_refError);
			p_refResult.bSideEffectCommitted = false;
			return bResult;
		}

		virtual void GetStatus(
			ST_MT_TRADE_NODE_STATUS& p_refStatus) const override
		{
			p_refStatus =
				ST_MT_TRADE_NODE_STATUS();
			p_refStatus.enState = m_bStarted ?
				EN_MT_TRADE_NODE_READY :
				EN_MT_TRADE_NODE_STOPPED;
			p_refStatus.strDetail = m_bStarted ?
				"TRADE_TEST_READY" :
				"TRADE_TEST_STOPPED";
		}

	private:
		bool m_bStarted;              // 测试节点生命周期。
		std::uint64_t m_ullNextOrder; // 测试订单号。
	};

	// 并发测试状态由两个节点 Adapter 共享，用于观测不同幂等分片是否真正重叠执行。
	struct ST_CONCURRENT_TRADE_STATE
	{
		std::atomic<unsigned int> uiActive;    // 当前进入 Adapter 的调用数。
		std::atomic<unsigned int> uiMaxActive; // 测试期间观察到的最大并发数。
		std::atomic<unsigned int> uiCallCount; // Adapter 实际执行总数。

		ST_CONCURRENT_TRADE_STATE()
			: uiActive(0)
			, uiMaxActive(0)
			, uiCallCount(0)
		{
		}
	};

	// 延迟 Adapter 不访问 SDK，只为幂等锁粒度测试提供可观测并发窗口。
	class CTestConcurrentTradeAdapter :
		public IMtTradeAdapter
	{
	public:
		explicit CTestConcurrentTradeAdapter(
			ST_CONCURRENT_TRADE_STATE* p_pState)
			: m_pState(p_pState)
			, m_bStarted(false)
		{
		}

		virtual ~CTestConcurrentTradeAdapter()
		{
			Stop();
		}

		virtual void SetEventSink(
			IMtTradeEventSink* p_pEventSink) override
		{
			(void)p_pEventSink;
		}

		virtual bool UpdateTimeState(
			const ST_MT_TIME_STATE& p_refState,
			std::string& p_refError) override
		{
			(void)p_refState;
			p_refError.clear();
			return true;
		}

		virtual bool Start(
			const ST_MT_TRADE_SOURCE_CONFIG& p_refSource,
			const ST_MT_TRADE_CONNECTION_CONFIG& p_refConnection,
			std::size_t p_szConnectionIndex,
			const ST_MT_TRADE_SERVICE_CONFIG& p_refService,
			std::string& p_refError) override
		{
			(void)p_refSource;
			(void)p_refConnection;
			(void)p_szConnectionIndex;
			(void)p_refService;
			p_refError.clear();
			m_bStarted = true;
			return m_pState != nullptr;
		}

		virtual void Stop() override
		{
			m_bStarted = false;
		}

		// 并发替身不维护 Dealer 状态；只验证 Owner 接管前的对账契约被完整实现。
		virtual bool ReconcileBeforeReady(
			std::string& p_refError) override
		{
			p_refError.clear();
			if (!m_bStarted || m_pState == nullptr)
			{
				p_refError =
					"TRADE_CONCURRENCY_TEST_RECONCILE_NOT_READY";
				return false;
			}
			return true;
		}

		virtual bool Execute(std::int64_t p_llFuncId,
			const ST_PLUGIN_BINARY_VALUE& p_refRequest,
			ST_MT_TRADE_EXECUTION_RESULT& p_refResult,
			std::string& p_refError) override
		{
			(void)p_llFuncId;
			(void)p_refRequest;
			p_refError.clear();
			if (!m_bStarted || m_pState == nullptr)
			{
				p_refError =
					"TRADE_CONCURRENCY_TEST_ADAPTER_NOT_READY";
				return false;
			}
			const unsigned int uiActive =
				m_pState->uiActive.fetch_add(1U) + 1U;
			unsigned int uiMaximum =
				m_pState->uiMaxActive.load();
			while (uiMaximum < uiActive &&
				!m_pState->uiMaxActive.compare_exchange_weak(
					uiMaximum, uiActive))
			{
			}
			m_pState->uiCallCount.fetch_add(1U);
			std::this_thread::sleep_for(
				std::chrono::milliseconds(100));
			m_pState->uiActive.fetch_sub(1U);
			p_refResult = ST_MT_TRADE_EXECUTION_RESULT();
			p_refResult.iCode = EN_TERMINAL_ERROR_OK;
			p_refResult.strMessage = "OK";
			p_refResult.refData = CreatePluginBinaryValue(
				EN_PLUGIN_BINARY_VALUE_OBJECT);
			p_refResult.bSideEffectCommitted = false;
			return p_refResult.refData != nullptr;
		}

		virtual bool Precheck(
			const ST_PLUGIN_BINARY_VALUE& p_refRequest,
			ST_MT_TRADE_EXECUTION_RESULT& p_refResult,
			std::string& p_refError) override
		{
			return Execute(
				EN_PLUGIN_FUNC_TRADE_PLACE_ORDER_DETAIL,
				p_refRequest, p_refResult, p_refError);
		}

		virtual void GetStatus(
			ST_MT_TRADE_NODE_STATUS& p_refStatus) const override
		{
			p_refStatus = ST_MT_TRADE_NODE_STATUS();
			p_refStatus.enState = m_bStarted ?
				EN_MT_TRADE_NODE_READY :
				EN_MT_TRADE_NODE_STOPPED;
			p_refStatus.strDetail = m_bStarted ?
				"TRADE_CONCURRENCY_TEST_READY" :
				"TRADE_CONCURRENCY_TEST_STOPPED";
		}

	private:
		ST_CONCURRENT_TRADE_STATE* m_pState; // 测试函数拥有的并发计数器。
		bool m_bStarted;                     // Adapter 是否已开放调用。
	};

	// 测试失败时打印稳定英文信息，便于构建脚本直接定位失败断言。
	bool Check(bool p_bCondition, const char* p_szMessage)
	{
		if (!p_bCondition)
		{
			std::fprintf(stderr, "TEST_FAILED: %s\n",
				p_szMessage != nullptr ? p_szMessage : "unknown");
		}
		return p_bCondition;
	}

	// 为每个测试补充稳定名称，避免内部错误文本为空时无法定位失败用例。
	bool RunTest(const char* p_szName,
		bool (*p_pTest)())
	{
		if (p_pTest != nullptr && p_pTest())
		{
			return true;
		}
		std::fprintf(stderr,
			"TEST_CASE_FAILED: %s\n",
			p_szName != nullptr ? p_szName :
				"unknown");
		return false;
	}

	// 推送测试上下文记录发送次数和正文对象地址，用于确认多个目标共享同一消息缓冲。
	struct ST_PUSH_TEST_CONTEXT
	{
		std::mutex clMutex;                    // 保护回调结果。
		std::condition_variable clCondition;   // 等待全部异步发送完成。
		std::vector<const std::string*> aMessageAddress; // 每次回调收到的正文对象地址。
		std::vector<std::string> aMessage;       // 每次实际发送的正文，用于验证合并只保留最新值。
		std::vector<std::uint64_t> aGeneration;  // 每次发送的连接代次。
		unsigned int uiCloseCount;              // 非预期关闭回调次数。

		ST_PUSH_TEST_CONTEXT()
			: clMutex()
			, clCondition()
			, aMessageAddress()
			, aMessage()
			, aGeneration()
			, uiCloseCount(0)
		{
		}
	};

	// Dispatcher 工作线程调用；只记录不可变正文地址，不执行真实 Socket 发送。
	bool PushTestSend(
		const ST_MT_GATEWAY_WEB_TARGET& p_refTarget,
		const std::string& p_refMessage, void* p_pContext)
	{
		(void)p_refTarget;
		ST_PUSH_TEST_CONTEXT* pContext =
			static_cast<ST_PUSH_TEST_CONTEXT*>(p_pContext);
		if (pContext == nullptr || p_refMessage.empty())
		{
			return false;
		}
		{
			std::lock_guard<std::mutex> clLock(
				pContext->clMutex);
			pContext->aMessageAddress.push_back(
				&p_refMessage);
			pContext->aMessage.push_back(p_refMessage);
			pContext->aGeneration.push_back(
				p_refTarget.ullGeneration);
		}
		pContext->clCondition.notify_all();
		return true;
	}

	// 测试中发送始终成功；进入关闭回调即记录异常路径。
	void PushTestClose(
		const ST_MT_GATEWAY_WEB_TARGET& p_refTarget,
		const char* p_szReason, void* p_pContext)
	{
		(void)p_refTarget;
		(void)p_szReason;
		ST_PUSH_TEST_CONTEXT* pContext =
			static_cast<ST_PUSH_TEST_CONTEXT*>(p_pContext);
		if (pContext != nullptr)
		{
			std::lock_guard<std::mutex> clLock(
				pContext->clMutex);
			++pContext->uiCloseCount;
		}
	}

	// 校验反向索引覆盖通配、账号、品种、组合过滤及替换、退订和连接删除。
	bool TestGatewaySubscriptionIndex()
	{
		CMtGatewayWebSessionManager clManager;
		clManager.Reset(16U);
		std::vector<ST_MT_GATEWAY_WEB_TARGET> aTarget(4U);
		for (std::size_t szIndex = 0; szIndex < aTarget.size(); ++szIndex)
		{
			std::uint64_t ullGeneration = 0;
			void* pServer = reinterpret_cast<void*>(1U);
			void* pClient = reinterpret_cast<void*>(szIndex + 10U);
			if (!Check(clManager.AddConnection(pServer, pClient,
					"127.0.0.1", static_cast<unsigned short>(
						2000U + szIndex), ullGeneration),
					"failed to add indexed Web session"))
			{
				return false;
			}
			aTarget[szIndex].stKey.pServerHandle = pServer;
			aTarget[szIndex].stKey.pClientHandle = pClient;
			aTarget[szIndex].ullGeneration = ullGeneration;
		}

		std::string strError;
		const std::set<std::int64_t> setEmptyLogin;
		const std::set<std::string> setEmptySymbol;
		const std::set<std::int64_t> setLogin = { 10001 };
		const std::set<std::string> setEurUsd = { "EURUSD" };
		if (!Check(clManager.UpdateSubscription(aTarget[0],
				EN_MT_GATEWAY_WEB_SUBSCRIBE,
				EN_MT_GATEWAY_WEB_TOPIC_QUOTE, setEmptyLogin,
				setEmptySymbol, 5, 1, false, strError),
				strError.c_str()) ||
			!Check(clManager.UpdateSubscription(aTarget[1],
				EN_MT_GATEWAY_WEB_SUBSCRIBE,
				EN_MT_GATEWAY_WEB_TOPIC_QUOTE, setEmptyLogin,
				setEurUsd, 5, 1, false, strError),
				strError.c_str()) ||
			!Check(clManager.UpdateSubscription(aTarget[2],
				EN_MT_GATEWAY_WEB_SUBSCRIBE,
				EN_MT_GATEWAY_WEB_TOPIC_QUOTE, setLogin,
				setEurUsd, 5, 1, false, strError),
				strError.c_str()) ||
			!Check(clManager.UpdateSubscription(aTarget[3],
				EN_MT_GATEWAY_WEB_SUBSCRIBE,
				EN_MT_GATEWAY_WEB_TOPIC_ORDER, setLogin,
				setEmptySymbol, 5, 1, false, strError),
				strError.c_str()))
		{
			return false;
		}

		ST_CLIENT_DATA_BINARY_EVENT stEvent;
		stEvent.ullTopic = EN_MT_GATEWAY_WEB_TOPIC_QUOTE;
		stEvent.iSourceVersion = 5;
		stEvent.iSourceNo = 1;
		stEvent.llLogin = 10001;
		stEvent.strSymbol = "EURUSD";
		if (!Check(clManager.FindTargets(stEvent).size() == 3U,
				"combined Quote index returned duplicate or missing targets"))
		{
			return false;
		}
		stEvent.llLogin = 20002;
		if (!Check(clManager.FindTargets(stEvent).size() == 2U,
				"Login-filtered Quote index mismatch"))
		{
			return false;
		}
		stEvent.llLogin = 10001;
		stEvent.strSymbol = "GBPUSD";
		if (!Check(clManager.FindTargets(stEvent).size() == 1U,
				"Symbol-filtered Quote index mismatch"))
		{
			return false;
		}

		const std::set<std::string> setGbpUsd = { "GBPUSD" };
		if (!Check(clManager.UpdateSubscription(aTarget[1],
				EN_MT_GATEWAY_WEB_SUBSCRIBE,
				EN_MT_GATEWAY_WEB_TOPIC_QUOTE, setEmptyLogin,
				setGbpUsd, 5, 1, false, strError), strError.c_str()))
		{
			return false;
		}
		stEvent.strSymbol = "EURUSD";
		if (!Check(clManager.FindTargets(stEvent).size() == 2U,
				"subscription replacement left a stale index entry") ||
			!Check(clManager.UpdateSubscription(aTarget[0],
				EN_MT_GATEWAY_WEB_UNSUBSCRIBE,
				EN_MT_GATEWAY_WEB_TOPIC_QUOTE, setEmptyLogin,
				setEmptySymbol, 5, 1, false, strError), strError.c_str()) ||
			!Check(clManager.FindTargets(stEvent).size() == 1U,
				"unsubscribe did not remove wildcard index") ||
			!Check(clManager.RemoveConnection(
				aTarget[2].stKey.pServerHandle,
				aTarget[2].stKey.pClientHandle),
				"connection removal failed") ||
			!Check(clManager.FindTargets(stEvent).empty(),
				"connection removal left indexed subscriptions"))
		{
			return false;
		}

		stEvent.ullTopic = EN_MT_GATEWAY_WEB_TOPIC_ORDER;
		stEvent.llLogin = 10001;
		stEvent.strSymbol.clear();
		return Check(clManager.FindTargets(stEvent).size() == 1U,
			"user Topic Login index mismatch");
	}

	// 校验同一次扇出使用共享正文对象，队列线程不会为每个连接复制完整字符串。
	bool TestGatewaySharedPushPayload()
	{
		ST_MT_GATEWAY_WEB_CONFIG stConfig;
		stConfig.uiSendThreads = 1U;
		stConfig.uiQuoteQueueCapacity = 16U;
		stConfig.uiProfitQueueCapacity = 16U;
		stConfig.uiNormalQueueCapacity = 16U;
		stConfig.uiMaxPendingBytes = 1024U * 1024U;
		stConfig.iSlowClientCloseCount = 2;
		ST_PUSH_TEST_CONTEXT stContext;
		CMtGatewayPushDispatcher clDispatcher;
		std::string strError;
		if (!Check(clDispatcher.Start(stConfig,
				PushTestSend, PushTestClose,
				&stContext, strError), strError.c_str()))
		{
			return false;
		}
		const std::shared_ptr<const std::string> refMessage =
			std::make_shared<const std::string>(
				"{\"FuncId\":10002,\"Code\":0}");
		ST_MT_GATEWAY_WEB_TARGET stFirst;
		stFirst.stKey.pServerHandle = reinterpret_cast<void*>(1U);
		stFirst.stKey.pClientHandle = reinterpret_cast<void*>(20U);
		stFirst.ullGeneration = 1U;
		ST_MT_GATEWAY_WEB_TARGET stSecond = stFirst;
		stSecond.stKey.pClientHandle = reinterpret_cast<void*>(21U);
		stSecond.ullGeneration = 2U;
		if (!Check(clDispatcher.Enqueue(
				EN_MT_GATEWAY_WEB_TOPIC_QUOTE,
				stFirst, refMessage),
				"first shared push enqueue failed") ||
			!Check(clDispatcher.Enqueue(
				EN_MT_GATEWAY_WEB_TOPIC_QUOTE,
				stSecond, refMessage),
				"second shared push enqueue failed"))
		{
			clDispatcher.Stop();
			return false;
		}
		{
			std::unique_lock<std::mutex> clLock(
				stContext.clMutex);
			stContext.clCondition.wait_for(clLock,
				std::chrono::seconds(2),
				[&stContext]()
				{
					return stContext.aMessageAddress.size() == 2U;
				});
		}
		clDispatcher.Stop();
		return Check(stContext.aMessageAddress.size() == 2U,
				"shared push callbacks did not complete") &&
			Check(stContext.aMessageAddress[0] == refMessage.get() &&
				stContext.aMessageAddress[1] == refMessage.get(),
				"push fanout copied the immutable message") &&
			Check(stContext.uiCloseCount == 0U,
				"shared push unexpectedly closed a connection");
	}

	// 正间隔行情跨多个 fanout 分块时必须整体替换，不能只留下最后 1024 目标中的最后一块。
	bool TestGatewayLatestQuoteBatchReplacement()
	{
		ST_MT_GATEWAY_WEB_CONFIG stConfig;
		stConfig.uiSendThreads = 1U;
		stConfig.uiQuoteQueueCapacity = 32U;
		stConfig.uiProfitQueueCapacity = 16U;
		stConfig.uiNormalQueueCapacity = 16U;
		stConfig.uiMaxPendingBytes = 1024U * 1024U;
		stConfig.uiFanoutBatchTargets = 2U;
		ST_MT_GATEWAY_PUSH_POLICY stPolicy;
		stPolicy.usVersion = 5U;
		stPolicy.iNo = 1;
		stPolicy.uiQuoteMinIntervalMs = 100U;
		stConfig.aPushPolicy.push_back(stPolicy);
		ST_PUSH_TEST_CONTEXT stContext;
		CMtGatewayPushDispatcher clDispatcher;
		std::string strError;
		if (!Check(clDispatcher.Start(stConfig,
				PushTestSend, PushTestClose,
				&stContext, strError), strError.c_str()))
		{
			return false;
		}
		std::vector<ST_MT_GATEWAY_WEB_TARGET> aTarget(5U);
		for (std::size_t szIndex = 0; szIndex < aTarget.size(); ++szIndex)
		{
			aTarget[szIndex].stKey.pServerHandle =
				reinterpret_cast<void*>(1U);
			aTarget[szIndex].stKey.pClientHandle =
				reinterpret_cast<void*>(100U + szIndex);
			aTarget[szIndex].ullGeneration = szIndex + 1U;
		}
		ST_MT_GATEWAY_PUSH_CONTEXT stPushContext;
		stPushContext.usVersion = 5U;
		stPushContext.iNo = 1;
		stPushContext.strSymbol = "EURUSD";
		stPushContext.ullSequence = 1U;
		if (!Check(clDispatcher.EnqueueBatch(
				EN_MT_GATEWAY_WEB_TOPIC_QUOTE, aTarget,
				std::make_shared<const std::string>("old"),
				stPushContext), "old latest Quote batch enqueue failed"))
		{
			clDispatcher.Stop();
			return false;
		}
		stPushContext.ullSequence = 2U;
		if (!Check(clDispatcher.EnqueueBatch(
				EN_MT_GATEWAY_WEB_TOPIC_QUOTE, aTarget,
				std::make_shared<const std::string>("new"),
				stPushContext), "new latest Quote batch enqueue failed"))
		{
			clDispatcher.Stop();
			return false;
		}
		{
			std::unique_lock<std::mutex> clLock(stContext.clMutex);
			stContext.clCondition.wait_for(clLock,
				std::chrono::seconds(2), [&stContext]()
				{
					return stContext.aMessage.size() >= 5U;
				});
		}
		clDispatcher.Stop();
		std::set<std::uint64_t> setGeneration;
		for (std::size_t szIndex = 0;
			szIndex < stContext.aMessage.size(); ++szIndex)
		{
			if (!Check(stContext.aMessage[szIndex] == "new",
					"latest Quote emitted a replaced payload"))
			{
				return false;
			}
			setGeneration.insert(stContext.aGeneration[szIndex]);
		}
		return Check(stContext.aMessage.size() == 5U &&
				setGeneration.size() == 5U,
				"latest Quote batch replacement lost or duplicated targets") &&
			Check(stContext.uiCloseCount == 0U,
				"latest Quote replacement unexpectedly closed a target");
	}

	// 校验 1122 快照水位屏障、回放到 LIVE、来源重建和“初次加一次重试”的固定上限。
	bool TestGatewaySnapshotBarrierAndRetry()
	{
		CMtGatewayWebSessionManager clManager;
		clManager.Reset(4U, 4U);
		ST_MT_GATEWAY_WEB_TARGET stTarget;
		std::uint64_t ullGeneration = 0;
		stTarget.stKey.pServerHandle = reinterpret_cast<void*>(1U);
		stTarget.stKey.pClientHandle = reinterpret_cast<void*>(200U);
		if (!Check(clManager.AddConnection(stTarget.stKey.pServerHandle,
				stTarget.stKey.pClientHandle, "127.0.0.1", 3000U,
				ullGeneration), "snapshot test connection add failed"))
		{
			return false;
		}
		stTarget.ullGeneration = ullGeneration;
		std::string strError;
		const std::set<std::int64_t> setLogin;
		const std::set<std::string> setSymbol = { "EURUSD" };
		if (!Check(clManager.UpdateSubscription(stTarget,
				EN_MT_GATEWAY_WEB_SUBSCRIBE,
				EN_MT_GATEWAY_WEB_TOPIC_QUOTE, setLogin,
				setSymbol, 5, 1, false, strError), strError.c_str()))
		{
			return false;
		}
		ST_CLIENT_DATA_BINARY_EVENT stEvent;
		stEvent.ullTopic = EN_MT_GATEWAY_WEB_TOPIC_QUOTE;
		stEvent.iSourceVersion = 5;
		stEvent.iSourceNo = 1;
		stEvent.strSymbol = "EURUSD";
		stEvent.ullSequence = 11U;
		const auto refMessage =
			std::make_shared<const std::string>("tick-11");
		std::vector<ST_MT_GATEWAY_WEB_TARGET> aLive;
		std::vector<ST_MT_GATEWAY_WEB_TARGET> aResync;
		clManager.RouteIncrement(stEvent, refMessage,
			7U, aLive, aResync);
		if (!Check(aLive.empty() && aResync.empty(),
				"snapshot-loading increment escaped the barrier") ||
			!Check(clManager.BeginSnapshotDelivery(stTarget,
				EN_MT_GATEWAY_WEB_TOPIC_QUOTE, 5, 1,
				10U, 7U, 1U, strError), strError.c_str()))
		{
			return false;
		}
		std::vector<ST_MT_GATEWAY_PENDING_INCREMENT> aPending;
		bool bLive = false;
		if (!Check(clManager.DrainSnapshotBuffer(stTarget,
				EN_MT_GATEWAY_WEB_TOPIC_QUOTE, 5, 1,
				10U, 7U, 1U, aPending, bLive, strError),
				strError.c_str()) ||
			!Check(!bLive && aPending.size() == 1U &&
				aPending[0].ullSequence == 11U,
				"snapshot watermark replay mismatch") ||
			!Check(clManager.DrainSnapshotBuffer(stTarget,
				EN_MT_GATEWAY_WEB_TOPIC_QUOTE, 5, 1,
				10U, 7U, 1U, aPending, bLive, strError),
				strError.c_str()) ||
			!Check(bLive && aPending.empty(),
				"snapshot barrier did not switch atomically to LIVE"))
		{
			return false;
		}
		std::vector<ST_MT_GATEWAY_SNAPSHOT_TASK> aTask;
		clManager.BeginQuoteSourceResync(5, 1, aTask);
		if (!Check(aTask.size() == 1U &&
				aTask[0].stSubscription.uiSnapshotAttempt == 1U,
				"latest Quote source resync task mismatch"))
		{
			return false;
		}
		ST_MT_GATEWAY_SNAPSHOT_TASK stRetry;
		if (!Check(clManager.PrepareSnapshotRetry(stTarget,
				EN_MT_GATEWAY_WEB_TOPIC_QUOTE, 5, 1,
				stRetry, strError), strError.c_str()) ||
			!Check(stRetry.stSubscription.uiSnapshotAttempt == 2U,
				"snapshot retry attempt did not advance to two"))
		{
			return false;
		}
		return Check(!clManager.PrepareSnapshotRetry(stTarget,
			EN_MT_GATEWAY_WEB_TOPIC_QUOTE, 5, 1,
			stRetry, strError) &&
			strError.find("RETRY_EXHAUSTED") != std::string::npos,
			"snapshot retry exceeded the one-retry contract");
	}

	// Web Kline 复用 10002，Topic 位固定为 128，全掩码必须覆盖到 255。
	bool TestGatewayWebKlineTopicContract()
	{
		return Check(EN_MT_GATEWAY_WEB_TOPIC_KLINE == 128,
				"Web Kline Topic must be 128") &&
			Check(MT_GATEWAY_WEB_TOPIC_ALL == 255,
				"Web full Topic mask must be 255") &&
			Check(GetMtGatewayWebTopic(
				EN_PLUGIN_NOTIFY_KLINE_CHANGED) ==
				EN_MT_GATEWAY_WEB_TOPIC_KLINE,
				"1252 must map to Web Kline Topic") &&
			Check(GetMtGatewayWebFuncId(
				EN_PLUGIN_NOTIFY_KLINE_CHANGED) ==
				EN_MT_GATEWAY_WEB_FUNC_SUBSCRIPTION,
				"1252 must continue using Web FuncId 10002");
	}

	// 在已编码测试报文的固定位置写入 uint32 小端值，用于构造畸形外部计数。
	bool WriteUint32At(std::vector<unsigned char>& p_refBuffer,
		std::size_t p_szOffset, std::uint32_t p_uiValue)
	{
		if (p_szOffset > p_refBuffer.size() ||
			p_refBuffer.size() - p_szOffset < sizeof(p_uiValue))
		{
			return false;
		}
		for (std::size_t szIndex = 0;
			szIndex < sizeof(p_uiValue); ++szIndex)
		{
			p_refBuffer[p_szOffset + szIndex] =
				static_cast<unsigned char>(
					(p_uiValue >> (szIndex * 8U)) & 0xFFU);
		}
		return true;
	}

	// 创建包含整数、字符串和嵌入零字节字段的对象，验证协议不依赖 JSON 文本。
	ST_PLUGIN_BINARY_VALUE BuildProtocolObject()
	{
		ST_PLUGIN_BINARY_VALUE stRoot;
		stRoot.enType = EN_PLUGIN_BINARY_VALUE_OBJECT;

		ST_PLUGIN_BINARY_FIELD stLogin;
		stLogin.strName = "Login";
		stLogin.refValue = CreatePluginBinaryValue(
			EN_PLUGIN_BINARY_VALUE_INT64);
		stLogin.refValue->llIntValue = 10001;
		stRoot.aObjectField.push_back(stLogin);

		ST_PLUGIN_BINARY_FIELD stSymbol;
		stSymbol.strName = "Symbol";
		stSymbol.refValue = CreatePluginBinaryValue(
			EN_PLUGIN_BINARY_VALUE_STRING);
		stSymbol.refValue->strStringValue = "EURUSD";
		stRoot.aObjectField.push_back(stSymbol);

		ST_PLUGIN_BINARY_FIELD stRaw;
		stRaw.strName = "Raw";
		stRaw.refValue = CreatePluginBinaryValue(
			EN_PLUGIN_BINARY_VALUE_BYTES);
		stRaw.refValue->aByteValue = {0x11U, 0x00U, 0x22U};
		stRoot.aObjectField.push_back(stRaw);
		return stRoot;
	}

	// 校验 28 个兼容接口和健康接口都由正式 API 目录提供，不再登记 Demo URL。
	bool TestApiCatalog()
	{
		CMtGatewayApiCatalog clCatalog;
		std::string strError;
		if (!Check(clCatalog.RegisterPublishedApis(strError),
			strError.c_str()) ||
			!Check(clCatalog.GetCompatibilityRouteCount() == 30U,
			"compatibility route count must be 30") ||
			!Check(clCatalog.GetRouteCount() == 32U,
			"total route count must be 32"))
		{
			return false;
		}

		ST_MT_GATEWAY_API_DESCRIPTOR stDescriptor;
		if (!Check(clCatalog.Resolve("POST", "/server_info",
			stDescriptor), "server_info route is missing") ||
			!Check(stDescriptor.llFuncId ==
			EN_PLUGIN_FUNC_QUERY_SERVER_INFO,
			"server_info function identifier is invalid") ||
			!Check(stDescriptor.bIdempotent &&
			!stDescriptor.bWriteOperation,
			"query retry policy is invalid"))
		{
			return false;
		}
		if (!Check(clCatalog.Resolve("POST", "/place_order",
			stDescriptor), "place_order route is missing") ||
			!Check(stDescriptor.llFuncId ==
			EN_PLUGIN_FUNC_TRADE_PLACE_ORDER,
			"place_order function identifier is invalid") ||
			!Check(!stDescriptor.bIdempotent &&
			stDescriptor.bWriteOperation,
			"trade retry policy is invalid"))
		{
			return false;
		}
		if (!Check(!clCatalog.Resolve("POST",
			"/api/v1/demo/query", stDescriptor),
			"retired Demo URL must not be registered"))
		{
			return false;
		}
		return Check(clCatalog.Resolve("GET", "/health/ready",
			stDescriptor), "ready health route is missing");
	}

	// 校验 V2 平仓接口的数组根节点兼容边界，避免批量平仓接口被误改为相同语义。
	bool TestGatewayClosePositionRequestShape()
	{
		CMtGatewayApiCatalog clCatalog;
		CMtGatewayRequestAdapter clAdapter;
		ST_MT_GATEWAY_API_DESCRIPTOR stCloseDescriptor;
		ST_MT_GATEWAY_API_DESCRIPTOR stBatchDescriptor;
		std::string strError;
		if (!Check(clCatalog.RegisterPublishedApis(strError),
				strError.c_str()) ||
			!Check(clCatalog.Resolve("POST", "/close_position",
				stCloseDescriptor), "close_position route is missing") ||
			!Check(clCatalog.Resolve("POST", "/batch_close_position",
				stBatchDescriptor), "batch_close_position route is missing"))
		{
			return false;
		}

		nlohmann::json clCloseBody = nlohmann::json::array(
			{ nlohmann::json{
				{ "Version", 5 },
				{ "No", 1 },
				{ "Login", 10001 },
				{ "PositionId", 90001 } } });
		if (!Check(clAdapter.ValidateRequest(stCloseDescriptor,
				clCloseBody, strError), strError.c_str()) ||
			!Check(!clAdapter.ValidateRequest(stCloseDescriptor,
				nlohmann::json::array(), strError),
				"close_position must reject an empty array") ||
			!Check(!clAdapter.ValidateRequest(stCloseDescriptor,
				nlohmann::json::object(), strError),
				"close_position must reject an object root"))
		{
			return false;
		}

		nlohmann::json clInvalidItem = nlohmann::json::array();
		clInvalidItem.push_back(1);
		if (!Check(!clAdapter.ValidateRequest(stCloseDescriptor,
				clInvalidItem, strError),
				"close_position must reject non-object items"))
		{
			return false;
		}

		nlohmann::json clOversized = nlohmann::json::array();
		for (std::size_t szIndex = 0; szIndex < 1001U; ++szIndex)
		{
			clOversized.push_back(nlohmann::json::object());
		}
		if (!Check(!clAdapter.ValidateRequest(stCloseDescriptor,
				clOversized, strError),
				"close_position must reject more than 1000 items"))
		{
			return false;
		}

		const nlohmann::json clBatchBody =
		{
			{ "Version", 5 },
			{ "No", 1 },
			{ "Login", 10001 },
			{ "CloseType", 1 }
		};
		return Check(clAdapter.ValidateRequest(stBatchDescriptor,
				clBatchBody, strError), strError.c_str()) &&
			Check(!clAdapter.ValidateRequest(stBatchDescriptor,
				clCloseBody, strError),
				"batch_close_position must reject an array root");
	}

	// 校验 30 个 V2 HTTP 路由均有独立契约，并固定市场状态、server_info、bars、分页和 ClientData 边界。
	bool TestGatewayCompatibilityContracts()
	{
		struct ST_COMPATIBILITY_SAMPLE
		{
			const char* szPath;   // API 目录中的固定 URL。
			nlohmann::json clBody;// 能通过 V2 参数规则的最小请求。
		};
		const std::vector<ST_COMPATIBILITY_SAMPLE> aSamples =
		{
			{ "/server_info", { { "Version", 0 } } },
			{ "/symbols", nlohmann::json::object() },
			{ "/symbols/rates", { { "Login", 10001 } } },
			{ "/symbols/volume_rank", nlohmann::json::object() },
			{ "/quotes", nlohmann::json::object() },
			{ "/bars", { { "Version", 5 }, { "No", 1 }, { "SymbolIds", "EURUSD" }, { "Interval", 1 }, { "From", 1000 }, { "To", 2000 } } },
			{ "/accounts", nlohmann::json::object() },
			{ "/accounts/symbols", { { "Logins", nlohmann::json::array({ 10001 }) } } },
			{ "/accounts/symbols/trading", { { "Login", 10001 }, { "Symbols", "EURUSD" } } },
			{ "/accounts/orders", { { "Login", 10001 } } },
			{ "/accounts/positions", { { "Login", 10001 } } },
			{ "/accounts/deals", { { "Login", 10001 } } },
			{ "/holidays", { { "Version", 5 }, { "No", 1 }, { "From", 1000 }, { "To", 2000 }, { "SymbolIds", "EURUSD" } } },
			{ "/symbols/suspensions", { { "Version", 5 }, { "No", 1 }, { "SymbolIds", "EURUSD" }, { "OnlyRestricted", true } } },
			{ "/place_order", nlohmann::json::array({ { { "Symbol", "EURUSD" }, { "Volume", 100 }, { "Type", 0 } } }) },
			{ "/place_order/trading", nlohmann::json::array({ { { "Symbol", "EURUSD" }, { "Volume", 100 }, { "Type", 0 } } }) },
			{ "/update_order", { { "OrderId", 90001 } } },
			{ "/cancel_order", { { "OrderId", 90001 } } },
			{ "/accounts/create", nlohmann::json::object() },
			{ "/close_position", nlohmann::json::array({ { { "PositionId", 80001 } } }) },
			{ "/batch_close_position", { { "Login", 10001 }, { "CloseType", 1 } } },
			{ "/modify_position", { { "PositionId", 80001 } } },
			{ "/modify_balance", { { "Login", 10001 }, { "Amount", 10.0 } } },
			{ "/verify_password", { { "Login", 10001 }, { "Password", "test" } } },
			{ "/watchlist/symbols", { { "Version", 5 }, { "No", 1 }, { "Login", 10001 }, { "OptType", 4 } } },
			{ "/watchlist/sections", { { "Version", 5 }, { "No", 1 }, { "Login", 10001 }, { "OptType", 4 } } },
			{ "/chart/drawings", { { "Version", 5 }, { "No", 1 }, { "Login", 10001 }, { "OptType", 4 }, { "SymbolId", "EURUSD" } } },
			{ "/chart/drawings/sync", { { "Version", 5 }, { "No", 1 }, { "Login", 10001 }, { "OptType", 2 }, { "SymbolId", "EURUSD" }, { "Interval", 1 } } },
			{ "/chart/indicators", { { "Version", 5 }, { "No", 1 }, { "Login", 10001 }, { "OptType", 4 } } },
			{ "/chart/config", { { "Version", 5 }, { "No", 1 }, { "Login", 10001 }, { "OptType", 4 }, { "ConfigName", nlohmann::json::array() } } }
		};

		CMtGatewayApiCatalog clCatalog;
		CMtGatewayRequestAdapter clAdapter;
		std::string strError;
		if (!Check(clCatalog.RegisterPublishedApis(strError),
				strError.c_str()) ||
			!Check(aSamples.size() == 30U,
				"compatibility sample count must be 30"))
		{
			return false;
		}

		// Holiday 和停牌接口保持 V2 的严格 PascalCase、节点范围和查询边界。
		ST_MT_GATEWAY_API_DESCRIPTOR stHolidays;
		ST_MT_GATEWAY_API_DESCRIPTOR stSuspensions;
		if (!Check(clCatalog.Resolve("POST", "/holidays", stHolidays),
				"holidays route is missing") ||
			!Check(!clAdapter.ValidateRequest(stHolidays,
				{ { "Version", 5 }, { "No", 1 }, { "From", 1000 } }, strError),
				"holidays must require To") ||
			!Check(!clAdapter.ValidateRequest(stHolidays,
				{ { "Version", 5 }, { "No", 1 }, { "From", 2000 }, { "To", 1000 } }, strError),
				"holidays must reject a reversed range") ||
			!Check(clCatalog.Resolve("POST", "/symbols/suspensions", stSuspensions),
				"symbols/suspensions route is missing") ||
			!Check(!clAdapter.ValidateRequest(stSuspensions,
				{ { "Version", 5 }, { "No", 1 }, { "OnlyRestricted", 1 } }, strError),
				"symbols/suspensions must require a boolean OnlyRestricted"))
		{
			return false;
		}
		for (const ST_COMPATIBILITY_SAMPLE& refSample : aSamples)
		{
			ST_MT_GATEWAY_API_DESCRIPTOR stDescriptor;
			if (!Check(clCatalog.Resolve("POST", refSample.szPath,
					stDescriptor), refSample.szPath) ||
				!Check(clAdapter.ValidateRequest(stDescriptor,
					refSample.clBody, strError),
					(strError + ", path=" + refSample.szPath).c_str()))
			{
				return false;
			}
		}

		// 第一步：server_info 只接受 Version=0/4/5，Version=0 时禁止指定具体 No。
		ST_MT_GATEWAY_API_DESCRIPTOR stServerInfo;
		if (!Check(clCatalog.Resolve("POST", "/server_info",
				stServerInfo), "server_info route is missing") ||
			!Check(!clAdapter.ValidateRequest(stServerInfo,
				nlohmann::json::object(), strError),
				"server_info must require Version") ||
			!Check(clAdapter.ValidateRequest(stServerInfo,
				{ { "Version", 4 }, { "No", 1 }, { "WebUserStats", true } }, strError),
				strError.c_str()) ||
			!Check(clAdapter.ValidateRequest(stServerInfo,
				{ { "Version", 5 } }, strError), strError.c_str()) ||
			!Check(!clAdapter.ValidateRequest(stServerInfo,
				{ { "Version", 3 } }, strError),
				"server_info must reject unsupported Version") ||
			!Check(!clAdapter.ValidateRequest(stServerInfo,
				{ { "Version", 0 }, { "No", 1 } }, strError),
				"server_info Version=0 must reject a positive No") ||
			!Check(!clAdapter.ValidateRequest(stServerInfo,
				{ { "Version", 5 }, { "WebUserStats", 1 } }, strError),
				"server_info WebUserStats must be boolean") ||
			!Check(!clAdapter.ValidateRequest(stServerInfo,
				{ { "Version", 5 }, { "Diagnostics", true } }, strError),
				"server_info must reject unknown fields"))
		{
			return false;
		}

		// 第二步：bars 三种场景互斥，尾部条数和时间范围必须完整且在边界内。
		ST_MT_GATEWAY_API_DESCRIPTOR stBars;
		if (!Check(clCatalog.Resolve("POST", "/bars", stBars),
				"bars route is missing") ||
			!Check(clAdapter.ValidateRequest(stBars,
				{ { "Version", 5 }, { "No", 1 }, { "SymbolIds", "EURUSD" }, { "Interval", 1 }, { "OffsetTime", 0 }, { "ItemNum", 0 } }, strError),
				strError.c_str()) ||
			!Check(clAdapter.ValidateRequest(stBars,
				{ { "Version", 4 }, { "No", 1 }, { "SymbolIds", "EURUSD" }, { "Interval", 240 }, { "Offset", 0 }, { "ItemNum", 100 } }, strError),
				strError.c_str()) ||
			!Check(!clAdapter.ValidateRequest(stBars,
				{ { "No", 1 }, { "SymbolIds", "EURUSD" }, { "Interval", 1 }, { "From", 1000 }, { "To", 2000 } }, strError),
				"bars must require Version") ||
			!Check(!clAdapter.ValidateRequest(stBars,
				{ { "Version", 5 }, { "SymbolIds", "EURUSD" }, { "Interval", 1 }, { "From", 1000 }, { "To", 2000 } }, strError),
				"bars must require No") ||
			!Check(!clAdapter.ValidateRequest(stBars,
				{ { "Version", 4 }, { "No", 1 }, { "SymbolIds", "EURUSD" }, { "Interval", 2 }, { "From", 1000 }, { "To", 2000 } }, strError),
				"MT4 bars must reject an MT5-only period") ||
			!Check(clAdapter.ValidateRequest(stBars,
				{ { "Version", 5 }, { "No", 1 }, { "SymbolIds", "EURUSD" }, { "Interval", 2 }, { "From", 1000 }, { "To", 2000 } }, strError),
				strError.c_str()) ||
			!Check(!clAdapter.ValidateRequest(stBars,
				{ { "Version", 5 }, { "No", 1 }, { "SymbolIds", "EURUSD" }, { "Interval", 1 }, { "From", 1000 } }, strError),
				"bars must reject half of a range") ||
			!Check(!clAdapter.ValidateRequest(stBars,
				{ { "Version", 5 }, { "No", 1 }, { "SymbolIds", "EURUSD" }, { "Interval", 1 }, { "From", 1000 }, { "To", 2000 }, { "OffsetTime", 0 }, { "ItemNum", 1 } }, strError),
				"bars must reject mixed query scenes") ||
			!Check(!clAdapter.ValidateRequest(stBars,
				{ { "Version", 5 }, { "No", 1 }, { "SymbolIds", "EURUSD" }, { "Interval", 1 }, { "OffsetTime", 1 }, { "ItemNum", 0 } }, strError),
				"bars ItemNum=0 must require a zero tail anchor") ||
			!Check(!clAdapter.ValidateRequest(stBars,
				{ { "Version", 5 }, { "No", 1 }, { "SymbolIds", "EURUSD" }, { "Interval", 1 }, { "OffsetTime", 0 }, { "ItemNum", 201 } }, strError),
				"bars must reject an oversized ItemNum"))
		{
			return false;
		}

		// 第三步：账号分页只允许 MT5，Limit 可以单独出现，Offset 不能脱离 Limit。
		ST_MT_GATEWAY_API_DESCRIPTOR stOrders;
		if (!Check(clCatalog.Resolve("POST", "/accounts/orders",
				stOrders), "accounts/orders route is missing") ||
			!Check(clAdapter.ValidateRequest(stOrders,
				{ { "Version", 5 }, { "Login", 10001 }, { "Limit", 100 } }, strError),
				strError.c_str()) ||
			!Check(!clAdapter.ValidateRequest(stOrders,
				{ { "Version", 5 }, { "Login", 10001 }, { "Offset", 0 } }, strError),
				"Offset without Limit must be rejected") ||
			!Check(!clAdapter.ValidateRequest(stOrders,
				{ { "Version", 4 }, { "Login", 10001 }, { "Limit", 100 } }, strError),
				"MT4 account pagination must be rejected") ||
			!Check(!clAdapter.ValidateRequest(stOrders,
				{ { "Version", 5 }, { "Login", 10001 }, { "Limit", 501 } }, strError),
				"account page limit above 500 must be rejected"))
		{
			return false;
		}

		// 第四步：批量、ClientData 和嵌套 Content 保持 V2 的类型及字段边界。
		ST_MT_GATEWAY_API_DESCRIPTOR stAccountSymbols;
		ST_MT_GATEWAY_API_DESCRIPTOR stBatchClose;
		ST_MT_GATEWAY_API_DESCRIPTOR stDrawing;
		if (!Check(clCatalog.Resolve("POST", "/accounts/symbols",
				stAccountSymbols), "accounts/symbols route is missing") ||
			!Check(!clAdapter.ValidateRequest(stAccountSymbols,
				{ { "Logins", nlohmann::json::array() } }, strError),
				"accounts/symbols must reject empty Logins") ||
			!Check(clCatalog.Resolve("POST", "/batch_close_position",
				stBatchClose), "batch_close_position route is missing") ||
			!Check(!clAdapter.ValidateRequest(stBatchClose,
				{ { "Login", 10001 }, { "CloseType", 4 } }, strError),
				"batch_close_position must reject CloseType outside 1-3") ||
			!Check(clCatalog.Resolve("POST", "/chart/drawings",
				stDrawing), "chart/drawings route is missing") ||
			!Check(clAdapter.ValidateRequest(stDrawing,
				{ { "Version", 5 }, { "No", 1 }, { "Login", 10001 }, { "OptType", 1 }, { "SymbolId", "EURUSD" }, { "Interval", 1 }, { "DrawingId", "D1" }, { "ToolType", "trendLine" }, { "Content", { { "lowerCamelField", 1 } } } }, strError),
				strError.c_str()) ||
			!Check(!clAdapter.ValidateRequest(stDrawing,
				{ { "Version", 5 }, { "No", 1 }, { "Login", 10001 }, { "OptType", 4 }, { "symbolId", "EURUSD" } }, strError),
				"top-level fields must remain PascalCase"))
		{
			return false;
		}
		return true;
	}

	// 校验查询/交易领域隔离、尾随截断检测和嵌入零字节保存。
	bool TestBinaryDomains()
	{
		const ST_PLUGIN_BINARY_VALUE stRoot = BuildProtocolObject();
		std::vector<unsigned char> aPayload;
		std::string strError;
		if (!Check(EncodeQueryBinaryRequest(stRoot,
			aPayload, strError), strError.c_str()))
		{
			return false;
		}

		ST_PLUGIN_BINARY_DOCUMENT stDocument;
		if (!Check(DecodeQueryBinaryDocument(aPayload.data(),
			aPayload.size(), stDocument, strError), strError.c_str()) ||
			!Check(stDocument.refRoot != nullptr,
			"query root is null"))
		{
			return false;
		}
		const ST_PLUGIN_BINARY_VALUE* pRaw =
			FindPluginBinaryField(*stDocument.refRoot, "Raw");
		if (!Check(pRaw != nullptr &&
			pRaw->aByteValue.size() == 3U &&
			pRaw->aByteValue[1] == 0U,
			"embedded zero byte was not preserved"))
		{
			return false;
		}
		if (!Check(!DecodeTradeBinaryDocument(aPayload.data(),
			aPayload.size(), stDocument, strError),
			"trade decoder must reject query domain"))
		{
			return false;
		}
		aPayload.pop_back();
		return Check(!DecodeQueryBinaryDocument(aPayload.data(),
			aPayload.size(), stDocument, strError),
			"query decoder must reject truncated payload");
	}

	// 校验正式交易编号、退役 Demo 编号以及显式零值在 Trade Binary 中保持不变。
	bool TestTradeProtocolRegistry()
	{
		if (!Check(std::string(GetPluginFuncName(
				EN_PLUGIN_FUNC_TRADE_DEMO_RETIRED)) ==
					"TRADE_DEMO_RETIRED",
				"1131 must remain a retired protocol identifier") ||
			!Check(EN_PLUGIN_FUNC_TRADE_PLACE_ORDER == 1132 &&
				EN_PLUGIN_FUNC_TRADE_VERIFY_PASSWORD == 1142 &&
				EN_PLUGIN_FUNC_TRADE_HOLIDAYS == 1143 &&
				EN_PLUGIN_FUNC_TRADE_SYMBOL_SUSPENSIONS == 1144,
				"trade protocol range is invalid") ||
			!Check(!IsActiveMtTradeFuncId(
					EN_PLUGIN_FUNC_TRADE_DEMO_RETIRED) &&
				IsActiveMtTradeFuncId(
					EN_PLUGIN_FUNC_TRADE_PLACE_ORDER) &&
				IsActiveMtTradeFuncId(
					EN_PLUGIN_FUNC_TRADE_VERIFY_PASSWORD) &&
				IsActiveMtTradeFuncId(
					EN_PLUGIN_FUNC_TRADE_HOLIDAYS) &&
				IsActiveMtTradeFuncId(
					EN_PLUGIN_FUNC_TRADE_SYMBOL_SUSPENSIONS) &&
				!IsActiveMtTradeFuncId(
					EN_PLUGIN_FUNC_TRADE_MODIFY_POSITION + 1) &&
				!IsActiveMtTradeFuncId(
					EN_PLUGIN_FUNC_TRADE_SYMBOL_SUSPENSIONS + 1),
				"active trade protocol registry accepted a retired or reserved identifier"))
		{
			return false;
		}

		ST_PLUGIN_BINARY_VALUE stRoot;
		stRoot.enType = EN_PLUGIN_BINARY_VALUE_OBJECT;
		ST_PLUGIN_BINARY_FIELD stVersion;
		stVersion.strName = "Version";
		stVersion.refValue = CreatePluginBinaryValue(
			EN_PLUGIN_BINARY_VALUE_INT64);
		stVersion.refValue->llIntValue = 5;
		stRoot.aObjectField.push_back(stVersion);
		ST_PLUGIN_BINARY_FIELD stNo;
		stNo.strName = "No";
		stNo.refValue = CreatePluginBinaryValue(
			EN_PLUGIN_BINARY_VALUE_INT64);
		stNo.refValue->llIntValue = 1;
		stRoot.aObjectField.push_back(stNo);
		ST_PLUGIN_BINARY_FIELD stPosition;
		stPosition.strName = "PositionId";
		stPosition.refValue = CreatePluginBinaryValue(
			EN_PLUGIN_BINARY_VALUE_UINT64);
		stPosition.refValue->ullUIntValue = 90001U;
		stRoot.aObjectField.push_back(stPosition);
		ST_PLUGIN_BINARY_FIELD stPriceSl;
		stPriceSl.strName = "PriceSL";
		stPriceSl.refValue = CreatePluginBinaryValue(
			EN_PLUGIN_BINARY_VALUE_DOUBLE);
		stPriceSl.refValue->dDoubleValue = 0.0;
		stRoot.aObjectField.push_back(stPriceSl);

		std::vector<unsigned char> aPayload;
		std::string strError;
		if (!Check(EncodeTradeBinaryRequest(stRoot,
				aPayload, strError), strError.c_str()))
		{
			return false;
		}
		ST_PLUGIN_BINARY_DOCUMENT stDocument;
		if (!Check(DecodeTradeBinaryDocument(
				aPayload.data(), aPayload.size(),
				stDocument, strError), strError.c_str()) ||
			!Check(stDocument.refRoot != nullptr,
				"trade root is null"))
		{
			return false;
		}
		const ST_PLUGIN_BINARY_VALUE* pPriceSl =
			FindPluginBinaryField(*stDocument.refRoot,
				"PriceSL");
		return Check(pPriceSl != nullptr &&
			pPriceSl->enType ==
				EN_PLUGIN_BINARY_VALUE_DOUBLE &&
			pPriceSl->dDoubleValue == 0.0,
			"explicit zero PriceSL was not preserved");
	}

	// 校验可靠事件领域与 Trade/Query 隔离，并拒绝尾随数据。
	bool TestReliableEventProtocol()
	{
		ST_PLUGIN_BINARY_VALUE stRoot;
		stRoot.enType = EN_PLUGIN_BINARY_VALUE_OBJECT;
		ST_PLUGIN_BINARY_FIELD stEventId;
		stEventId.strName = "EventId";
		stEventId.refValue = CreatePluginBinaryValue(
			EN_PLUGIN_BINARY_VALUE_STRING);
		stEventId.refValue->strStringValue =
			"5-1-epoch-0000000001";
		stRoot.aObjectField.push_back(stEventId);
		ST_PLUGIN_BINARY_FIELD stEnvelope;
		stEnvelope.strName = "Envelope";
		stEnvelope.refValue = CreatePluginBinaryValue(
			EN_PLUGIN_BINARY_VALUE_BYTES);
		stEnvelope.refValue->aByteValue =
			{0x12U, 0x00U, 0x21U};
		stRoot.aObjectField.push_back(stEnvelope);

		std::vector<unsigned char> aPayload;
		std::string strError;
		if (!EncodeReliableEventBinaryRequest(
				stRoot, aPayload, strError))
		{
			return Check(false,
				("reliable encode failed: " +
					strError).c_str());
		}
		ST_PLUGIN_BINARY_DOCUMENT stDocument;
		if (!DecodeReliableEventBinaryDocument(
				aPayload.data(), aPayload.size(),
				stDocument, strError))
		{
			return Check(false,
				("reliable decode failed: " +
					strError).c_str());
		}
		if (!Check(!DecodeTradeBinaryDocument(
				aPayload.data(), aPayload.size(),
				stDocument, strError),
				"trade decoder must reject reliable event domain"))
		{
			return false;
		}
		if (!Check(!DecodeQueryBinaryDocument(
				aPayload.data(), aPayload.size(),
				stDocument, strError),
				"query decoder must reject reliable event domain"))
		{
			return false;
		}
		aPayload.push_back(0x7FU);
		return Check(!DecodeReliableEventBinaryDocument(
			aPayload.data(), aPayload.size(),
			stDocument, strError),
			"reliable event decoder must reject trailing data");
	}

	// 校验 ClientRequestId 结果在重启后仍命中，并拒绝同键不同请求摘要。
	bool TestTradeIdempotencyPersistence()
	{
		const std::int64_t llUnique =
			std::chrono::duration_cast<
				std::chrono::microseconds>(
				std::chrono::steady_clock::now().
					time_since_epoch()).count();
		const std::filesystem::path clDirectory =
			std::filesystem::temp_directory_path() /
			("TradingTerminalV3_Idempotency_" +
				std::to_string(llUnique));
		std::error_code clFileError;
		std::filesystem::remove_all(clDirectory,
			clFileError);

		CMtTradeIdempotencyStore clStore;
		std::string strError;
		if (!Check(clStore.Initialize(
				clDirectory.u8string(), 72U, strError),
				strError.c_str()))
		{
			return false;
		}
		const std::vector<unsigned char> aRequest =
			{0x11U, 0x32U, 0x00U, 0x01U};
		const std::uint64_t ullDigest =
			CMtTradeIdempotencyStore::ComputeDigest(
				aRequest.data(), aRequest.size());
		ST_MT_TRADE_IDEMPOTENCY_RESULT stSaved;
		stSaved.iCode = EN_TERMINAL_ERROR_OK;
		stSaved.strMessage = "OK";
		stSaved.refData = CreatePluginBinaryValue(
			EN_PLUGIN_BINARY_VALUE_OBJECT);
		ST_PLUGIN_BINARY_FIELD stOrder;
		stOrder.strName = "OrderId";
		stOrder.refValue = CreatePluginBinaryValue(
			EN_PLUGIN_BINARY_VALUE_UINT64);
		stOrder.refValue->ullUIntValue = 80001U;
		stSaved.refData->aObjectField.push_back(stOrder);
		if (!Check(clStore.Save(
				"5:1:1132:test-request", ullDigest,
				stSaved, strError), strError.c_str()))
		{
			clStore.Stop();
			std::filesystem::remove_all(clDirectory,
				clFileError);
			return false;
		}
		ST_MT_TRADE_IDEMPOTENCY_RESULT stLoaded;
		if (!Check(clStore.Lookup(
				"5:1:1132:test-request", ullDigest,
				stLoaded, strError) ==
					EN_MT_TRADE_IDEMPOTENCY_HIT,
				"idempotency result must be a hit") ||
			!Check(stLoaded.refData != nullptr &&
				FindPluginBinaryField(*stLoaded.refData,
					"OrderId") != nullptr,
				"idempotency result Data is missing") ||
			!Check(clStore.Lookup(
				"5:1:1132:test-request",
				ullDigest + 1U, stLoaded, strError) ==
					EN_MT_TRADE_IDEMPOTENCY_CONFLICT,
				"idempotency digest conflict was not rejected"))
		{
			clStore.Stop();
			std::filesystem::remove_all(clDirectory,
				clFileError);
			return false;
		}
		clStore.Stop();
		strError.clear();
		if (!Check(clStore.Initialize(
				clDirectory.u8string(), 72U, strError),
				strError.c_str()) ||
			!Check(clStore.Lookup(
				"5:1:1132:test-request", ullDigest,
				stLoaded, strError) ==
					EN_MT_TRADE_IDEMPOTENCY_HIT,
				"idempotency result must survive restart"))
		{
			clStore.Stop();
			std::filesystem::remove_all(clDirectory,
				clFileError);
			return false;
		}
		clStore.Stop();
		std::filesystem::remove_all(clDirectory,
			clFileError);
		return Check(!clFileError,
			"idempotency test directory cleanup failed");
	}

	// 校验 outbox 文件跨重启恢复、SourceSequence 高水位和确认后删除语义。
	bool TestTradeOutboxRecovery()
	{
		const std::int64_t llUnique =
			std::chrono::duration_cast<
				std::chrono::microseconds>(
				std::chrono::steady_clock::now().
					time_since_epoch()).count();
		const std::filesystem::path clDirectory =
			std::filesystem::temp_directory_path() /
			("TradingTerminalV3_Outbox_" +
				std::to_string(llUnique));
		std::error_code clFileError;
		std::filesystem::remove_all(clDirectory,
			clFileError);

		ST_MT_TRADE_SERVICE_CONFIG stConfig;
		stConfig.strOutboxPath =
			clDirectory.u8string();
		stConfig.uiOutboxRetryMs = 10U;
		stConfig.uiOutboxBatchSize = 8U;
		CMtTradeOutbox clOutbox;
		std::string strError;
		if (!Check(clOutbox.Initialize(stConfig,
				strError), strError.c_str()))
		{
			return false;
		}
		ST_MT_TRADE_EVENT stEvent;
		stEvent.enNotifyId =
			EN_PLUGIN_NOTIFY_ORDER_CHANGED;
		stEvent.enAction =
			EN_PLUGIN_NOTIFY_ACTION_CREATED;
		stEvent.refPayload = CreatePluginBinaryValue(
			EN_PLUGIN_BINARY_VALUE_OBJECT);
		ST_PLUGIN_BINARY_FIELD stOrder;
		stOrder.strName = "OrderId";
		stOrder.refValue = CreatePluginBinaryValue(
			EN_PLUGIN_BINARY_VALUE_UINT64);
		stOrder.refValue->ullUIntValue = 91001U;
		stEvent.refPayload->aObjectField.push_back(
			stOrder);
		std::vector<ST_MT_TRADE_EVENT> aEvent;
		aEvent.push_back(stEvent);
		std::vector<std::string> aEventId;
		if (!Check(clOutbox.Append(5, 1, aEvent,
				aEventId, strError), strError.c_str()) ||
			!Check(aEventId.size() == 1U,
				"outbox event id was not returned"))
		{
			clOutbox.Stop();
			std::filesystem::remove_all(clDirectory,
				clFileError);
			return false;
		}
		ST_MT_TRADE_OUTBOX_STATUS stBeforeRestart;
		clOutbox.GetStatus(stBeforeRestart);
		if (!Check(stBeforeRestart.ullLastSequence == 1U &&
				stBeforeRestart.szPendingCount == 1U,
				"outbox initial sequence or pending count is invalid"))
		{
			clOutbox.Stop();
			std::filesystem::remove_all(clDirectory,
				clFileError);
			return false;
		}
		const std::uint64_t ullFirstEpoch =
			stBeforeRestart.ullSourceEpoch;
		clOutbox.Stop();

		strError.clear();
		if (!Check(clOutbox.Initialize(stConfig,
				strError), strError.c_str()))
		{
			std::filesystem::remove_all(clDirectory,
				clFileError);
			return false;
		}
		ST_MT_TRADE_OUTBOX_STATUS stRecovered;
		clOutbox.GetStatus(stRecovered);
		if (!Check(stRecovered.ullLastSequence == 1U &&
				stRecovered.szPendingCount == 1U &&
				stRecovered.ullSourceEpoch !=
					ullFirstEpoch,
				"outbox restart recovery state is invalid"))
		{
			clOutbox.Stop();
			std::filesystem::remove_all(clDirectory,
				clFileError);
			return false;
		}
		CTestTradeOutboxSink clSink;
		ST_CLUSTER_FENCE stFence;
		if (!Check(clOutbox.ActivateShard(5U, 1,
				stFence, strError), strError.c_str()) ||
			!Check(clOutbox.Start(&clSink, &clSink,
				strError), strError.c_str()))
		{
			clOutbox.Stop();
			std::filesystem::remove_all(clDirectory,
				clFileError);
			return false;
		}
		for (unsigned int uiWait = 0;
			uiWait < 200U; ++uiWait)
		{
			clOutbox.GetStatus(stRecovered);
			if (stRecovered.szPendingCount == 0U)
			{
				break;
			}
			std::this_thread::sleep_for(
				std::chrono::milliseconds(10));
		}
		clOutbox.GetStatus(stRecovered);
		clOutbox.Stop();
		const bool bRecovered =
			stRecovered.szPendingCount == 0U &&
			stRecovered.ullDeliveredCount == 1U &&
			clSink.uiDelivered.load() == 1U &&
			clSink.bEnvelopeValid.load();
		std::filesystem::remove_all(clDirectory,
			clFileError);
		return Check(bRecovered,
				"outbox restart delivery did not drain exactly one event") &&
			Check(!clFileError,
				"outbox test directory cleanup failed");
	}

	// 校验 MT5 Dealer 状态的提交、终态单调性、超时未知、对账终态和损坏文件拒绝。
	bool TestMtDealerStatePersistence()
	{
		const std::int64_t llUnique =
			std::chrono::duration_cast<std::chrono::microseconds>(
				std::chrono::steady_clock::now().time_since_epoch()).count();
		const std::filesystem::path clDirectory =
			std::filesystem::temp_directory_path() /
			("TradingTerminalV3_Dealer_" +
				std::to_string(llUnique));
		CTestDirectoryGuard clDirectoryGuard(clDirectory);
		CMtDealerStateStore clStore;
		std::string strError;
		if (!Check(clStore.Initialize(clDirectory.u8string(),
				5U, 1, 1000U, 1U, 1000U,
				strError), strError.c_str()))
		{
			return false;
		}

		ST_MT_DEALER_RECORD stConfirmed;
		stConfirmed.usVersion = 5U;
		stConfirmed.iNo = 1;
		stConfirmed.uiRequestId = 7001U;
		stConfirmed.enState = EN_MT_DEALER_STATE_SUBMITTED;
		stConfirmed.ullLogin = 90001U;
		stConfirmed.uiAction = 200U;
		stConfirmed.strClientRequestId = "dealer-test-7001";
		stConfirmed.strDetail = "MT5_DEALER_REQUEST_SUBMITTED";
		if (!Check(clStore.SaveSubmitted(stConfirmed,
				strError), strError.c_str()))
		{
			return false;
		}
		stConfirmed.enState = EN_MT_DEALER_STATE_CONFIRMED;
		stConfirmed.ullOrder = 80001U;
		stConfirmed.ullDeal = 81001U;
		stConfirmed.iRetCode = 10009;
		stConfirmed.strDetail = "MT5_DEALER_ANSWER_CONFIRMED";
		if (!Check(clStore.SaveState(stConfirmed,
				strError), strError.c_str()))
		{
			return false;
		}
		ST_MT_DEALER_RECORD stLateSubmitted = stConfirmed;
		stLateSubmitted.enState = EN_MT_DEALER_STATE_SUBMITTED;
		stLateSubmitted.ullOrder = 0;
		stLateSubmitted.ullDeal = 0;
		if (!Check(clStore.SaveSubmitted(stLateSubmitted,
				strError), strError.c_str()))
		{
			return false;
		}

		ST_MT_DEALER_RECORD stUnknown;
		stUnknown.usVersion = 5U;
		stUnknown.iNo = 1;
		stUnknown.uiRequestId = 7002U;
		stUnknown.enState = EN_MT_DEALER_STATE_SUBMITTED;
		stUnknown.ullLogin = 90002U;
		stUnknown.llSubmittedTimeMs =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count() -
			2000LL;
		stUnknown.strDetail = "MT5_DEALER_REQUEST_SUBMITTED";
		if (!Check(clStore.SaveSubmitted(stUnknown,
				strError), strError.c_str()))
		{
			return false;
		}
		std::vector<ST_MT_DEALER_RECORD> aUnresolved;
		const std::int64_t llNowMs =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count();
		if (!Check(clStore.CollectUnresolved(llNowMs,
				aUnresolved, strError), strError.c_str()) ||
			!Check(aUnresolved.size() == 1U &&
				aUnresolved[0].uiRequestId == 7002U &&
				aUnresolved[0].enState ==
					EN_MT_DEALER_STATE_TIMED_OUT_UNKNOWN,
				"Dealer timeout state was not persisted"))
		{
			return false;
		}
		stUnknown = aUnresolved[0];
		stUnknown.enState = EN_MT_DEALER_STATE_RECONCILED;
		stUnknown.ullOrder = 80002U;
		stUnknown.iRetCode = 10008;
		stUnknown.strDetail = "MT5_DEALER_REQUEST_RECONCILED";
		if (!Check(clStore.SaveState(stUnknown,
				strError), strError.c_str()))
		{
			return false;
		}
		ST_MT_DEALER_RECORD stKeepUnknown;
		stKeepUnknown.usVersion = 5U;
		stKeepUnknown.iNo = 1;
		stKeepUnknown.uiRequestId = 7003U;
		stKeepUnknown.enState = EN_MT_DEALER_STATE_SUBMITTED;
		stKeepUnknown.ullLogin = 90003U;
		stKeepUnknown.llSubmittedTimeMs = llNowMs - 2000LL;
		stKeepUnknown.strDetail = "MT5_DEALER_REQUEST_SUBMITTED";
		std::size_t szRemoved = 0;
		ST_MT_DEALER_STATE_STATS stStats;
		if (!Check(clStore.SaveSubmitted(stKeepUnknown,
				strError), strError.c_str()) ||
			!Check(clStore.CollectUnresolved(llNowMs,
				aUnresolved, strError), strError.c_str()) ||
			!Check(clStore.CleanupExpiredTerminal(
				llNowMs + 2LL * 60LL * 60LL * 1000LL,
				szRemoved, strError), strError.c_str()) ||
			!Check(szRemoved == 2U,
				"Dealer cleanup did not remove both expired terminal records"))
		{
			return false;
		}
		clStore.GetStats(stStats);
		if (!Check(stStats.ullTimedOutUnknown == 1U &&
				stStats.ullConfirmed == 0U &&
				stStats.ullReconciled == 0U,
			"Dealer cleanup removed UNKNOWN or retained expired terminal state"))
		{
			return false;
		}
		clStore.Stop();
		if (!Check(clStore.Initialize(clDirectory.u8string(),
				5U, 1, 1000U, 1U, 1000U,
				strError), strError.c_str()) ||
			!Check(clStore.CollectUnresolved(llNowMs,
				aUnresolved, strError), strError.c_str()) ||
			!Check(aUnresolved.size() == 1U &&
				aUnresolved[0].uiRequestId == 7003U &&
				aUnresolved[0].enState ==
					EN_MT_DEALER_STATE_TIMED_OUT_UNKNOWN,
				"Dealer UNKNOWN state did not survive cleanup and restart"))
		{
			return false;
		}
		clStore.Stop();
		{
			std::ofstream clCorrupt(
				clDirectory / "request_7999.dealer",
				std::ios::binary | std::ios::trunc);
			clCorrupt.write("bad", 3);
		}
		strError.clear();
		return Check(!clStore.Initialize(clDirectory.u8string(),
				5U, 1, 1000U, 1U, 1000U,
				strError),
			"Dealer store must reject a corrupted state file") &&
			Check(strError.find("DEALER_STATE_") == 0,
				"Dealer corrupted-file error detail is missing");
	}

	// 校验 Version+No 路由、Mock 执行、只读预检和未知节点错误隔离。
	bool TestTradeNodeRouting()
	{
		ST_MT_TRADE_SERVICE_CONFIG stConfig;
		stConfig.bAllowMockAdapter = true;
		ST_MT_TRADE_SOURCE_CONFIG stMt4;
		stMt4.bEnable = true;
		stMt4.bMock = true;
		stMt4.usVersion = 4;
		stMt4.iNo = 1;
		stMt4.strName = "MT4-Test-1";
		stMt4.szMaxInflight = 8U;
		AddTradeTestConnections(stMt4);
		stConfig.aSource.push_back(stMt4);
		ST_MT_TRADE_SOURCE_CONFIG stMt5;
		stMt5.bEnable = true;
		stMt5.bMock = true;
		stMt5.usVersion = 5;
		stMt5.iNo = 2;
		stMt5.strName = "MT5-Test-2";
		stMt5.szMaxInflight = 8U;
		AddTradeTestConnections(stMt5);
		stConfig.aSource.push_back(stMt5);

		CMtTradeNodeManager clManager;
		std::string strError;
		if (!Check(clManager.Configure(stConfig,
				strError), strError.c_str()) ||
			!Check(clManager.Start(strError),
				strError.c_str()) ||
			!Check(clManager.ActivateShard(4U, 1,
				strError), strError.c_str()) ||
			!Check(clManager.ReconcileShard(4U, 1,
				strError), strError.c_str()) ||
			!Check(clManager.ActivateShard(5U, 2,
				strError), strError.c_str()) ||
			!Check(clManager.ReconcileShard(5U, 2,
				strError), strError.c_str()))
		{
			return false;
		}
		ST_PLUGIN_BINARY_VALUE stRequest;
		stRequest.enType =
			EN_PLUGIN_BINARY_VALUE_OBJECT;
		ST_MT_TRADE_EXECUTION_RESULT stResult;
		if (!Check(clManager.Execute(4, 1,
				EN_PLUGIN_FUNC_TRADE_PLACE_ORDER,
				stRequest, stResult, strError),
				strError.c_str()) ||
			!Check(stResult.iCode ==
					EN_TERMINAL_ERROR_OK &&
				stResult.bSideEffectCommitted &&
				stResult.refData != nullptr &&
				FindPluginBinaryField(
					*stResult.refData,
					"OrderId") != nullptr,
				"MT4 mock route result is invalid"))
		{
			clManager.Stop();
			return false;
		}
		if (!Check(clManager.Precheck(5, 2,
				stRequest, stResult, strError),
				strError.c_str()) ||
			!Check(stResult.iCode ==
					EN_TERMINAL_ERROR_OK &&
				!stResult.bSideEffectCommitted,
				"MT5 mock precheck result is invalid"))
		{
			clManager.Stop();
			return false;
		}
		if (!Check(clManager.Execute(5, 99,
				EN_PLUGIN_FUNC_TRADE_CANCEL_ORDER,
				stRequest, stResult, strError),
				strError.c_str()) ||
			!Check(stResult.iCode ==
				EN_TERMINAL_ERROR_TRADE_NODE_NOT_FOUND,
				"unknown Version+No was not isolated"))
		{
			clManager.Stop();
			return false;
		}
		std::vector<std::pair<
			ST_MT_TRADE_SOURCE_CONFIG,
			ST_MT_TRADE_NODE_STATUS>> aStatus;
		clManager.GetStatus(aStatus);
		clManager.Stop();
		return Check(aStatus.size() == 2U &&
			aStatus[0].second.enState ==
				EN_MT_TRADE_NODE_READY &&
			aStatus[1].second.enState ==
				EN_MT_TRADE_NODE_READY &&
			aStatus[0].second.szConnectionCount == 1U &&
			aStatus[1].second.szConnectionCount == 1U &&
			aStatus[0].second.szReadyConnectionCount == 1U &&
			aStatus[1].second.szReadyConnectionCount == 1U &&
			aStatus[0].second.szPrecheckConnectionCount == 1U &&
			aStatus[1].second.szPrecheckConnectionCount == 1U &&
			aStatus[0].second.aRoleReady.size() == 5U &&
			aStatus[1].second.aRoleReady.size() == 5U,
			"trade node status did not report all isolated role connections");
	}

	// 校验批量部分成功、成功项 outbox 持久化以及嵌套敏感字段递归清理。
	bool TestTradeDispatcherPartialBatch()
	{
		const std::int64_t llUnique =
			std::chrono::duration_cast<
				std::chrono::microseconds>(
				std::chrono::steady_clock::now().
					time_since_epoch()).count();
		const std::filesystem::path clRoot =
			std::filesystem::temp_directory_path() /
			("TradingTerminalV3_Dispatcher_" +
				std::to_string(llUnique));
		std::error_code clFileError;
		std::filesystem::remove_all(clRoot,
			clFileError);

		ST_MT_TRADE_SERVICE_CONFIG stConfig;
		stConfig.strIdempotencyPath =
			(clRoot / "idempotency").u8string();
		stConfig.strOutboxPath =
			(clRoot / "outbox").u8string();
		stConfig.uiOutboxRetryMs = 10U;
		stConfig.uiOutboxBatchSize = 8U;
		CMtTradeIdempotencyStore clIdempotency;
		CMtTradeOutbox clOutbox;
		CMtTradeNodeManager clManager;
		CMtTradeDispatcher clDispatcher;
		std::string strError;
		if (!Check(clIdempotency.Initialize(
				stConfig.strIdempotencyPath, 72U,
				strError), strError.c_str()) ||
			!Check(clOutbox.Initialize(stConfig,
				strError), strError.c_str()))
		{
			return false;
		}
		ST_MT_TRADE_SOURCE_CONFIG stSource;
		stSource.bEnable = true;
		stSource.bMock = true;
		stSource.usVersion = 5;
		stSource.iNo = 1;
		stSource.strName = "MT5-Partial-Test";
		stSource.szMaxInflight = 8U;
		if (!Check(clManager.RegisterAdapterForTest(
				stSource,
				std::unique_ptr<IMtTradeAdapter>(
					new CTestPartialTradeAdapter()),
				strError), strError.c_str()) ||
			!Check(clManager.Start(strError),
				strError.c_str()) ||
			!Check(clManager.ActivateShard(5U, 1,
				strError), strError.c_str()) ||
			!Check(clManager.ReconcileShard(5U, 1,
				strError), strError.c_str()) ||
			!Check(clDispatcher.Initialize(&stConfig,
				&clManager, &clIdempotency,
				&clOutbox, strError),
				strError.c_str()))
		{
			clManager.Stop();
			clOutbox.Stop();
			clIdempotency.Stop();
			std::filesystem::remove_all(clRoot,
				clFileError);
			return false;
		}

		const auto fnMakeItem =
			[](bool p_bReject)
		{
			std::shared_ptr<ST_PLUGIN_BINARY_VALUE>
				refItem = CreatePluginBinaryValue(
					EN_PLUGIN_BINARY_VALUE_OBJECT);
			ST_PLUGIN_BINARY_FIELD stVersion;
			stVersion.strName = "Version";
			stVersion.refValue =
				CreatePluginBinaryValue(
					EN_PLUGIN_BINARY_VALUE_INT64);
			stVersion.refValue->llIntValue = 5;
			refItem->aObjectField.push_back(
				stVersion);
			ST_PLUGIN_BINARY_FIELD stNo;
			stNo.strName = "No";
			stNo.refValue =
				CreatePluginBinaryValue(
					EN_PLUGIN_BINARY_VALUE_INT64);
			stNo.refValue->llIntValue = 1;
			refItem->aObjectField.push_back(stNo);
			ST_PLUGIN_BINARY_FIELD stReject;
			stReject.strName = "Reject";
			stReject.refValue =
				CreatePluginBinaryValue(
					EN_PLUGIN_BINARY_VALUE_BOOL);
			stReject.refValue->ucBoolValue =
				p_bReject ? 1U : 0U;
			refItem->aObjectField.push_back(
				stReject);
			ST_PLUGIN_BINARY_FIELD stCredentials;
			stCredentials.strName = "Credentials";
			stCredentials.refValue =
				CreatePluginBinaryValue(
					EN_PLUGIN_BINARY_VALUE_OBJECT);
			ST_PLUGIN_BINARY_FIELD stPassword;
			stPassword.strName = "Password";
			stPassword.refValue =
				CreatePluginBinaryValue(
					EN_PLUGIN_BINARY_VALUE_STRING);
			stPassword.refValue->strStringValue =
				"must-not-enter-outbox";
			stCredentials.refValue->
				aObjectField.push_back(stPassword);
			refItem->aObjectField.push_back(
				stCredentials);
			return refItem;
		};
		ST_PLUGIN_BINARY_VALUE stBatch;
		stBatch.enType =
			EN_PLUGIN_BINARY_VALUE_ARRAY;
		stBatch.aArrayValue.push_back(
			fnMakeItem(false));
		stBatch.aArrayValue.push_back(
			fnMakeItem(true));
		std::int32_t iCode =
			EN_TERMINAL_ERROR_INTERNAL_ERROR;
		std::string strMessage;
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE>
			refData;
		if (!Check(clDispatcher.Dispatch(
				EN_PLUGIN_FUNC_TRADE_PLACE_ORDER,
				stBatch, iCode, strMessage, refData,
				strError), strError.c_str()) ||
			!Check(iCode == EN_TERMINAL_ERROR_OK &&
				strMessage.find("PARTIAL_SUCCESS") !=
					std::string::npos &&
				refData != nullptr,
				"trade batch did not report partial success"))
		{
			clManager.Stop();
			clOutbox.Stop();
			clIdempotency.Stop();
			std::filesystem::remove_all(clRoot,
				clFileError);
			return false;
		}
		const ST_PLUGIN_BINARY_VALUE* pSuccess =
			FindPluginBinaryField(*refData,
				"SuccessCount");
		const ST_PLUGIN_BINARY_VALUE* pFailed =
			FindPluginBinaryField(*refData,
				"FailedCount");
		if (!Check(pSuccess != nullptr &&
				pSuccess->llIntValue == 1 &&
				pFailed != nullptr &&
				pFailed->llIntValue == 1,
				"trade batch result counts are invalid"))
		{
			clManager.Stop();
			clOutbox.Stop();
			clIdempotency.Stop();
			std::filesystem::remove_all(clRoot,
				clFileError);
			return false;
		}
		ST_MT_TRADE_OUTBOX_STATUS stStatus;
		clOutbox.GetStatus(stStatus);
		const ST_PLUGIN_BINARY_VALUE* pResults =
			FindPluginBinaryField(*refData,
				"Results");
		const ST_PLUGIN_BINARY_VALUE* pFirstResult =
			pResults != nullptr &&
				pResults->enType ==
					EN_PLUGIN_BINARY_VALUE_ARRAY &&
				!pResults->aArrayValue.empty() &&
				pResults->aArrayValue.front() ?
			pResults->aArrayValue.front().get() :
			nullptr;
		const ST_PLUGIN_BINARY_VALUE* pFirstData =
			pFirstResult != nullptr ?
				FindPluginBinaryField(*pFirstResult,
					"Data") : nullptr;
		const ST_PLUGIN_BINARY_VALUE* pEventPersisted =
			pFirstData != nullptr ?
				FindPluginBinaryField(*pFirstData,
					"EventPersisted") : nullptr;
		const ST_PLUGIN_BINARY_VALUE* pEventState =
			pFirstData != nullptr ?
				FindPluginBinaryField(*pFirstData,
					"EventState") : nullptr;
		clManager.Stop();
		clOutbox.Stop();
		clIdempotency.Stop();
		const bool bValid =
			stStatus.szPendingCount == 0U &&
			stStatus.ullDeliveredCount == 0U &&
			pEventPersisted != nullptr &&
			pEventPersisted->enType ==
				EN_PLUGIN_BINARY_VALUE_BOOL &&
			pEventPersisted->ucBoolValue == 0U &&
			pEventState != nullptr &&
			pEventState->enType ==
				EN_PLUGIN_BINARY_VALUE_STRING &&
			pEventState->strStringValue ==
				"AWAITING_AUTHORITATIVE_CALLBACK";
		std::filesystem::remove_all(clRoot,
			clFileError);
		return Check(bValid,
				"trade command fabricated an authoritative state event") &&
			Check(!clFileError,
				"dispatcher test directory cleanup failed");
	}

	// 校验不同幂等分片可以并发执行，而相同键的并发重试仍只进入一次 Adapter。
	bool TestTradeIdempotencyLockShards()
	{
		const std::filesystem::path clRoot =
			std::filesystem::temp_directory_path() /
			"TradingTerminalV3_TradeLockShards";
		CTestDirectoryGuard clDirectoryGuard(clRoot);
		ST_MT_TRADE_SERVICE_CONFIG stConfig;
		stConfig.strIdempotencyPath =
			(clRoot / "idempotency").u8string();
		stConfig.strOutboxPath =
			(clRoot / "outbox").u8string();
		stConfig.uiOutboxRetryMs = 10U;
		stConfig.uiOutboxBatchSize = 8U;
		CMtTradeIdempotencyStore clIdempotency;
		CMtTradeOutbox clOutbox;
		CMtTradeNodeManager clManager;
		CMtTradeDispatcher clDispatcher;
		ST_CONCURRENT_TRADE_STATE stState;
		std::string strError;
		if (!Check(clIdempotency.Initialize(
				stConfig.strIdempotencyPath, 72U,
				strError), strError.c_str()) ||
			!Check(clOutbox.Initialize(stConfig,
				strError), strError.c_str()))
		{
			return false;
		}
		for (std::int32_t iNo = 1; iNo <= 2; ++iNo)
		{
			ST_MT_TRADE_SOURCE_CONFIG stSource;
			stSource.bEnable = true;
			stSource.bMock = true;
			stSource.usVersion = 5;
			stSource.iNo = iNo;
			stSource.strName =
				"MT5-Concurrency-" + std::to_string(iNo);
			stSource.szMaxInflight = 8U;
			if (!Check(clManager.RegisterAdapterForTest(
					stSource,
					std::unique_ptr<IMtTradeAdapter>(
						new CTestConcurrentTradeAdapter(
							&stState)), strError),
					strError.c_str()))
			{
				clOutbox.Stop();
				clIdempotency.Stop();
				return false;
			}
		}
		if (!Check(clManager.Start(strError), strError.c_str()) ||
			!Check(clManager.ActivateShard(5U, 1,
				strError), strError.c_str()) ||
			!Check(clManager.ReconcileShard(5U, 1,
				strError), strError.c_str()) ||
			!Check(clManager.ActivateShard(5U, 2,
				strError), strError.c_str()) ||
			!Check(clManager.ReconcileShard(5U, 2,
				strError), strError.c_str()) ||
			!Check(clDispatcher.Initialize(&stConfig,
				&clManager, &clIdempotency,
				&clOutbox, strError), strError.c_str()))
		{
			clManager.Stop();
			clOutbox.Stop();
			clIdempotency.Stop();
			return false;
		}

		const auto fnBuildRequest = [](
			std::int32_t p_iNo,
			const std::string& p_refRequestId)
		{
			ST_PLUGIN_BINARY_VALUE stRequest;
			stRequest.enType =
				EN_PLUGIN_BINARY_VALUE_OBJECT;
			const auto fnAddInt64 = [&stRequest](
				const char* p_szName,
				std::int64_t p_llValue)
			{
				ST_PLUGIN_BINARY_FIELD stField;
				stField.strName = p_szName;
				stField.refValue = CreatePluginBinaryValue(
					EN_PLUGIN_BINARY_VALUE_INT64);
				stField.refValue->llIntValue = p_llValue;
				stRequest.aObjectField.push_back(stField);
			};
			fnAddInt64("Version", 5);
			fnAddInt64("No", p_iNo);
			ST_PLUGIN_BINARY_FIELD stRequestId;
			stRequestId.strName = "ClientRequestId";
			stRequestId.refValue = CreatePluginBinaryValue(
				EN_PLUGIN_BINARY_VALUE_STRING);
			stRequestId.refValue->strStringValue =
				p_refRequestId;
			stRequest.aObjectField.push_back(stRequestId);
			return stRequest;
		};

		// 第一步：选择两个明确映射到不同固定分片的键，避免测试受偶然哈希碰撞影响。
		const std::string strFirstId = "parallel-a";
		const std::string strFirstKey =
			"5:1:1134:" + strFirstId;
		const std::uint64_t ullFirstShard =
			CMtTradeIdempotencyStore::ComputeDigest(
				reinterpret_cast<const unsigned char*>(
					strFirstKey.data()), strFirstKey.size()) % 64U;
		std::string strSecondId;
		for (unsigned int uiIndex = 0; uiIndex < 256U; ++uiIndex)
		{
			const std::string strCandidate =
				"parallel-b-" + std::to_string(uiIndex);
			const std::string strKey =
				"5:2:1134:" + strCandidate;
			const std::uint64_t ullShard =
				CMtTradeIdempotencyStore::ComputeDigest(
					reinterpret_cast<const unsigned char*>(
						strKey.data()), strKey.size()) % 64U;
			if (ullShard != ullFirstShard)
			{
				strSecondId = strCandidate;
				break;
			}
		}
		if (!Check(!strSecondId.empty(),
				"failed to select distinct idempotency lock shards"))
		{
			clManager.Stop();
			clOutbox.Stop();
			clIdempotency.Stop();
			return false;
		}

		const ST_PLUGIN_BINARY_VALUE stFirstRequest =
			fnBuildRequest(1, strFirstId);
		const ST_PLUGIN_BINARY_VALUE stSecondRequest =
			fnBuildRequest(2, strSecondId);
		std::atomic<bool> bFirstSucceeded(false);
		std::atomic<bool> bSecondSucceeded(false);
		const auto fnDispatch = [&clDispatcher](
			const ST_PLUGIN_BINARY_VALUE& p_refRequest,
			std::atomic<bool>& p_refSucceeded)
		{
			std::int32_t iCode =
				EN_TERMINAL_ERROR_INTERNAL_ERROR;
			std::string strMessage;
			std::string strDispatchError;
			std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refData;
			const bool bDispatched = clDispatcher.Dispatch(
				EN_PLUGIN_FUNC_TRADE_UPDATE_ORDER,
				p_refRequest, iCode, strMessage,
				refData, strDispatchError);
			p_refSucceeded.store(bDispatched &&
				iCode == EN_TERMINAL_ERROR_OK);
			if (!p_refSucceeded.load())
			{
				std::fprintf(stderr,
					"TRADE_LOCK_DISPATCH_DIAGNOSTIC: dispatched=%d,code=%d,message=%s,error=%s\n",
					bDispatched ? 1 : 0, iCode,
					strMessage.c_str(), strDispatchError.c_str());
			}
		};
		std::thread clFirst(fnDispatch,
			std::cref(stFirstRequest),
			std::ref(bFirstSucceeded));
		std::thread clSecond(fnDispatch,
			std::cref(stSecondRequest),
			std::ref(bSecondSucceeded));
		clFirst.join();
		clSecond.join();
		const bool bParallel = bFirstSucceeded.load() &&
			bSecondSucceeded.load() &&
			stState.uiMaxActive.load() >= 2U;
		if (!bParallel)
		{
			std::fprintf(stderr,
				"TRADE_LOCK_SHARD_DIAGNOSTIC: first=%d,second=%d,maxActive=%u,calls=%u,firstShard=%llu\n",
				bFirstSucceeded.load() ? 1 : 0,
				bSecondSucceeded.load() ? 1 : 0,
				stState.uiMaxActive.load(),
				stState.uiCallCount.load(),
				static_cast<unsigned long long>(ullFirstShard));
		}

		// 第二步：同一键并发两次，首次结果落盘前后都不得产生第二次 Adapter 调用。
		stState.uiMaxActive.store(0U);
		const unsigned int uiCallsBefore =
			stState.uiCallCount.load();
		const ST_PLUGIN_BINARY_VALUE stDuplicateRequest =
			fnBuildRequest(1, "same-key");
		std::atomic<bool> bDuplicateFirst(false);
		std::atomic<bool> bDuplicateSecond(false);
		std::thread clDuplicateFirst(fnDispatch,
			std::cref(stDuplicateRequest),
			std::ref(bDuplicateFirst));
		std::thread clDuplicateSecond(fnDispatch,
			std::cref(stDuplicateRequest),
			std::ref(bDuplicateSecond));
		clDuplicateFirst.join();
		clDuplicateSecond.join();
		const bool bSingleExecution =
			bDuplicateFirst.load() &&
			bDuplicateSecond.load() &&
			stState.uiCallCount.load() ==
				uiCallsBefore + 1U;
		clManager.Stop();
		clOutbox.Stop();
		clIdempotency.Stop();
		return Check(bParallel,
				"different idempotency shards did not execute concurrently") &&
			Check(bSingleExecution,
				"same idempotency key executed the Adapter more than once");
	}

	// 校验短报文中的恶意超大对象/数组计数在 reserve 前被拒绝。
	bool TestMalformedContainerCounts()
	{
		std::string strError;
		ST_PLUGIN_BINARY_VALUE stEmptyObject;
		stEmptyObject.enType = EN_PLUGIN_BINARY_VALUE_OBJECT;
		std::vector<unsigned char> aPayload;
		if (!Check(EncodeQueryBinaryRequest(stEmptyObject,
			aPayload, strError), strError.c_str()) ||
			!Check(WriteUint32At(aPayload, 28U, 0xFFFFFFFFU),
			"failed to patch object field count"))
		{
			return false;
		}
		ST_PLUGIN_BINARY_DOCUMENT stDocument;
		if (!Check(!DecodeQueryBinaryDocument(aPayload.data(),
			aPayload.size(), stDocument, strError),
			"huge object field count must be rejected") ||
			!Check(strError.find("BINARY_OBJECT_FIELD_COUNT_INVALID") !=
			std::string::npos,
			"huge object field count returned unexpected error"))
		{
			return false;
		}

		ST_PLUGIN_BINARY_VALUE stRoot;
		stRoot.enType = EN_PLUGIN_BINARY_VALUE_OBJECT;
		ST_PLUGIN_BINARY_FIELD stItems;
		stItems.strName = "Items";
		stItems.refValue = CreatePluginBinaryValue(
			EN_PLUGIN_BINARY_VALUE_ARRAY);
		stRoot.aObjectField.push_back(stItems);
		if (!Check(EncodeQueryBinaryRequest(stRoot,
			aPayload, strError), strError.c_str()) ||
			!Check(WriteUint32At(aPayload, 49U, 0xFFFFFFFFU),
			"failed to patch array item count"))
		{
			return false;
		}
		return Check(!DecodeQueryBinaryDocument(aPayload.data(),
			aPayload.size(), stDocument, strError),
			"huge array item count must be rejected") &&
			Check(strError.find("BINARY_ARRAY_ITEM_COUNT_INVALID") !=
			std::string::npos,
			"huge array item count returned unexpected error");
	}

	// 构造可复用的 Quote Tick v1 测试数据；调用方只覆盖本场景关注的字段。
	void SetTestQuoteDefaults(ST_QUOTE_BINARY_TICK& p_refTick)
	{
		p_refTick = ST_QUOTE_BINARY_TICK();
		p_refTick.usPlatformVersion = 5;
		p_refTick.iSourceNo = 1;
		p_refTick.ullSourceEpoch = 7001U;
		p_refTick.ullIngressSequence = 99U;
		p_refTick.strSymbol = "EURUSD";
		p_refTick.dBid = 1.12345;
		p_refTick.dAsk = 1.12355;
		p_refTick.dLast = 1.12350;
		p_refTick.ullVolume = 10U;
		p_refTick.ullVolumeExt = 100000U;
		p_refTick.ullFlags = 0x10U;
		p_refTick.llServerTime = 1710000000LL;
		p_refTick.llServerTimeMsc = 1710000000123LL;
		p_refTick.llIngressTimeMs = 1710000000456LL;
	}

	// 校验 1211 的 ClientData 外层和 Quote Tick v1 正文能够无损往返。
	bool TestQuoteClientDataEvent()
	{
		ST_QUOTE_BINARY_TICK stTick;
		SetTestQuoteDefaults(stTick);
		std::vector<unsigned char> aQuotePayload;
		std::string strError;
		if (!Check(EncodeQuoteBinaryTick(stTick,
			aQuotePayload, strError), strError.c_str()) ||
			!Check(aQuotePayload.size() == QUOTE_BINARY_HEADER_SIZE +
				stTick.strSymbol.size(),
				"Quote Tick Binary v1 fixed header length mismatch"))
		{
			return false;
		}

		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refQuote =
			CreatePluginBinaryValue(EN_PLUGIN_BINARY_VALUE_BYTES);
		if (!Check(refQuote != nullptr, "failed to allocate Quote BYTES"))
		{
			return false;
		}
		refQuote->aByteValue = aQuotePayload;
		ST_CLIENT_DATA_BINARY_EVENT stEvent;
		stEvent.ullTopic = EN_CLIENT_DATA_BINARY_TOPIC_QUOTE;
		stEvent.iSourceVersion = stTick.usPlatformVersion;
		stEvent.iSourceNo = stTick.iSourceNo;
		stEvent.strSymbol = stTick.strSymbol;
		stEvent.ullSequence = stTick.ullIngressSequence;
		stEvent.llTimestampMs = stTick.llIngressTimeMs;
		stEvent.refData = refQuote;
		std::vector<unsigned char> aPayload;
		if (!Check(EncodeClientDataBinaryEvent(stEvent,
			aPayload, strError), strError.c_str()))
		{
			return false;
		}

		ST_CLIENT_DATA_BINARY_EVENT stDecodedEvent;
		ST_QUOTE_BINARY_TICK stDecodedTick;
		return Check(DecodeClientDataBinaryEvent(aPayload.data(),
			aPayload.size(), stDecodedEvent, strError), strError.c_str()) &&
			Check(stDecodedEvent.refData != nullptr &&
				stDecodedEvent.refData->enType == EN_PLUGIN_BINARY_VALUE_BYTES,
				"decoded Quote Data is not BYTES") &&
			Check(DecodeQuoteBinaryTick(
				stDecodedEvent.refData->aByteValue.data(),
				stDecodedEvent.refData->aByteValue.size(),
				stDecodedTick, strError), strError.c_str()) &&
			Check(stDecodedTick.strSymbol == stTick.strSymbol &&
				stDecodedTick.ullSourceEpoch == stTick.ullSourceEpoch &&
				stDecodedTick.ullIngressSequence == stTick.ullIngressSequence &&
				stDecodedTick.llServerTimeMsc == stTick.llServerTimeMsc &&
				stDecodedTick.dBid == stTick.dBid,
				"Quote Tick Binary v1 ClientData round trip mismatch");
	}

	// 校验 Quote Tick Binary v1 的唯一布局和全部关键输入边界。
	bool TestQuoteBinaryV1()
	{
		ST_QUOTE_BINARY_TICK stTick;
		SetTestQuoteDefaults(stTick);
		std::vector<unsigned char> aPayload;
		std::string strError;
		ST_QUOTE_BINARY_TICK stDecoded;
		if (!Check(EncodeQuoteBinaryTick(stTick, aPayload, strError),
				strError.c_str()) ||
			!Check(aPayload.size() == QUOTE_BINARY_HEADER_SIZE +
				stTick.strSymbol.size(),
				"Quote Tick Binary v1 payload length mismatch") ||
			!Check(DecodeQuoteBinaryTick(aPayload.data(), aPayload.size(),
				stDecoded, strError), strError.c_str()) ||
			!Check(stDecoded.usVersion == 1U &&
				stDecoded.usPlatformVersion == 5U &&
				stDecoded.iSourceNo == 1 &&
				stDecoded.ullSourceEpoch == 7001U &&
				stDecoded.ullIngressSequence == 99U &&
				stDecoded.llServerTime == 1710000000LL &&
				stDecoded.llServerTimeMsc == 1710000000123LL &&
				stDecoded.llIngressTimeMs == 1710000000456LL,
				"Quote Tick Binary v1 round trip mismatch"))
		{
			return false;
		}

		ST_QUOTE_BINARY_TICK stInvalid = stTick;
		stInvalid.usVersion = 3U;
		if (!Check(!EncodeQuoteBinaryTick(stInvalid, aPayload, strError),
				"old Quote protocol version must be rejected by encoder"))
		{
			return false;
		}
		if (!Check(EncodeQuoteBinaryTick(stTick, aPayload, strError),
				strError.c_str()))
		{
			return false;
		}
		for (unsigned char chVersion = 3U; chVersion <= 6U; ++chVersion)
		{
			std::vector<unsigned char> aOld = aPayload;
			aOld[4] = chVersion;
			if (!Check(!DecodeQuoteBinaryTick(aOld.data(), aOld.size(),
					stDecoded, strError),
					"old Quote development payload must be rejected"))
			{
				return false;
			}
		}

		stInvalid = stTick;
		stInvalid.strSymbol.assign("\xC0\xAF", 2);
		if (!Check(!EncodeQuoteBinaryTick(stInvalid, aPayload, strError),
				"invalid UTF-8 Quote symbol must be rejected"))
		{
			return false;
		}
		stInvalid = stTick;
		stInvalid.strSymbol.assign("EUR\0USD", 7);
		if (!Check(!EncodeQuoteBinaryTick(stInvalid, aPayload, strError),
				"embedded NUL Quote symbol must be rejected"))
		{
			return false;
		}
		stInvalid = stTick;
		stInvalid.dBid = (std::numeric_limits<double>::infinity)();
		if (!Check(!EncodeQuoteBinaryTick(stInvalid, aPayload, strError),
				"non-finite Quote price must be rejected"))
		{
			return false;
		}
		stInvalid = stTick;
		stInvalid.llServerTimeMsc += 1000LL;
		if (!Check(!EncodeQuoteBinaryTick(stInvalid, aPayload, strError),
				"inconsistent raw server time must be rejected"))
		{
			return false;
		}

		if (!Check(EncodeQuoteBinaryTick(stTick, aPayload, strError),
				strError.c_str()))
		{
			return false;
		}
		std::vector<unsigned char> aTruncated = aPayload;
		aTruncated.pop_back();
		std::vector<unsigned char> aTrailing = aPayload;
		aTrailing.push_back(0U);
		std::vector<unsigned char> aReserved = aPayload;
		aReserved[10] = 1U;
		return Check(!DecodeQuoteBinaryTick(aTruncated.data(),
			aTruncated.size(), stDecoded, strError),
			"truncated Quote v1 payload must be rejected") &&
			Check(!DecodeQuoteBinaryTick(aTrailing.data(),
				aTrailing.size(), stDecoded, strError),
				"Quote v1 payload with trailing bytes must be rejected") &&
			Check(!DecodeQuoteBinaryTick(aReserved.data(),
				aReserved.size(), stDecoded, strError),
				"Quote v1 payload with nonzero reserved field must be rejected");
	}

	// 校验 1124 请求/应答身份、进程代次、逐来源诊断和畸形正文边界。
	bool TestQuoteHeartbeatProtocol()
	{
		ST_QUOTE_HEARTBEAT_REQUEST stRequest;
		stRequest.enSenderPluginNo = EN_SERVICE_HEARTBEAT_PLUGIN_DERIVE;
		stRequest.enTargetPluginNo = EN_SERVICE_HEARTBEAT_PLUGIN_QUOTE;
		stRequest.ullRequestSequence = 7U;
		stRequest.ullSenderProcessEpoch = 7001U;
		stRequest.llSentAtMs = 1710000000000LL;
		std::vector<unsigned char> aPayload;
		std::string strError;
		ST_QUOTE_HEARTBEAT_REQUEST stDecodedRequest;
		if (!Check(EncodeQuoteHeartbeatRequest(stRequest,
				aPayload, strError), strError.c_str()) ||
			!Check(DecodeQuoteHeartbeatRequest(aPayload.data(),
				aPayload.size(), stDecodedRequest, strError),
				strError.c_str()) ||
			!Check(stDecodedRequest.ullRequestSequence == 7U &&
				stDecodedRequest.ullSenderProcessEpoch == 7001U,
				"Quote heartbeat request round trip mismatch"))
		{
			return false;
		}

		ST_QUOTE_HEARTBEAT_RESPONSE stResponse;
		stResponse.enSenderPluginNo = EN_SERVICE_HEARTBEAT_PLUGIN_QUOTE;
		stResponse.enTargetPluginNo = EN_SERVICE_HEARTBEAT_PLUGIN_DERIVE;
		stResponse.ullRequestSequence = stRequest.ullRequestSequence;
		stResponse.ullSenderProcessEpoch = 8001U;
		stResponse.llRespondedAtMs = 1710000000010LL;
		ST_QUOTE_HEARTBEAT_SOURCE_STATE stSource;
		stSource.usPlatformVersion = 5;
		stSource.usConnectionState = 2;
		stSource.iSourceNo = 1;
		stSource.uiActivePriority = 1U;
		stSource.uiQueueDepth = 12U;
		stSource.ullSourceEpoch = 9001U;
		stSource.ullLastSequence = 99U;
		stSource.llLastTickTimeMs = 1710000000009LL;
		stSource.ullReconnectCount = 2U;
		stSource.ullDroppedCount = 3U;
		stSource.ullPublishFailedCount = 4U;
		stResponse.aSource.push_back(stSource);
		ST_QUOTE_HEARTBEAT_RESPONSE stDecodedResponse;
		if (!Check(EncodeQuoteHeartbeatResponse(stResponse,
				aPayload, strError), strError.c_str()) ||
			!Check(DecodeQuoteHeartbeatResponse(aPayload.data(),
				aPayload.size(), stDecodedResponse, strError),
				strError.c_str()) ||
			!Check(stDecodedResponse.ullRequestSequence == 7U &&
				stDecodedResponse.aSource.size() == 1U &&
				stDecodedResponse.aSource[0].ullSourceEpoch == 9001U &&
				stDecodedResponse.aSource[0].ullPublishFailedCount == 4U,
				"Quote heartbeat response round trip mismatch"))
		{
			return false;
		}

		ST_QUOTE_HEARTBEAT_REQUEST stInvalidRequest = stRequest;
		stInvalidRequest.enSenderPluginNo = EN_SERVICE_HEARTBEAT_PLUGIN_QUOTE;
		if (!Check(!EncodeQuoteHeartbeatRequest(stInvalidRequest,
				aPayload, strError),
				"Quote heartbeat must reject the wrong request direction"))
		{
			return false;
		}
		stInvalidRequest = stRequest;
		stInvalidRequest.ullRequestSequence = 0U;
		if (!Check(!EncodeQuoteHeartbeatRequest(stInvalidRequest,
				aPayload, strError),
				"Quote heartbeat must reject a zero request sequence"))
		{
			return false;
		}
		ST_QUOTE_HEARTBEAT_RESPONSE stInvalidResponse = stResponse;
		stInvalidResponse.aSource.push_back(stSource);
		if (!Check(!EncodeQuoteHeartbeatResponse(stInvalidResponse,
				aPayload, strError),
				"Quote heartbeat must reject duplicate sources"))
		{
			return false;
		}
		stInvalidResponse = stResponse;
		stInvalidResponse.aSource[0].usConnectionState = 5U;
		if (!Check(!EncodeQuoteHeartbeatResponse(stInvalidResponse,
				aPayload, strError),
				"Quote heartbeat must reject an invalid connection state"))
		{
			return false;
		}
		if (!Check(EncodeQuoteHeartbeatResponse(stResponse,
				aPayload, strError), strError.c_str()))
		{
			return false;
		}
		aPayload.pop_back();
		return Check(!DecodeQuoteHeartbeatResponse(aPayload.data(),
			aPayload.size(), stDecodedResponse, strError),
			"Quote heartbeat must reject a truncated response");
	}

	// 校验 1176 请求/应答身份、队列计数、JetStream 状态和逐来源水位。
	bool TestEventHeartbeatProtocol()
	{
		ST_EVENT_HEARTBEAT_REQUEST stRequest;
		stRequest.enSenderPluginId = EN_PLUGIN_ID_MT_DERIVE_SERVICE;
		stRequest.ullRequestSequence = 17U;
		stRequest.ullSenderProcessEpoch = 7001U;
		stRequest.llSentAtMs = 1710000000000LL;
		std::vector<unsigned char> aPayload;
		std::string strError;
		ST_EVENT_HEARTBEAT_REQUEST stDecodedRequest;
		if (!Check(EncodeEventHeartbeatRequest(stRequest,
				aPayload, strError), strError.c_str()) ||
			!Check(aPayload.size() == 40U,
				"Event heartbeat request length mismatch") ||
			!Check(DecodeEventHeartbeatRequest(aPayload.data(),
				aPayload.size(), stDecodedRequest, strError),
				strError.c_str()) ||
			!Check(stDecodedRequest.enSenderPluginId ==
					EN_PLUGIN_ID_MT_DERIVE_SERVICE &&
				stDecodedRequest.enTargetPluginId ==
					EN_PLUGIN_ID_MT_EVENT_SERVICE &&
				stDecodedRequest.ullRequestSequence == 17U,
				"Event heartbeat request round trip mismatch"))
		{
			return false;
		}

		ST_EVENT_HEARTBEAT_RESPONSE stResponse;
		stResponse.enRequesterPluginId = stRequest.enSenderPluginId;
		stResponse.bJetStreamReady = true;
		stResponse.ullRequestSequence = stRequest.ullRequestSequence;
		stResponse.ullResponderProcessEpoch = 8001U;
		stResponse.llRespondedAtMs = 1710000000010LL;
		stResponse.ullQueueCapacity = 0U;
		stResponse.ullQueueDepth = 12U;
		stResponse.ullReceivedCount = 100U;
		stResponse.ullValidationRejectedCount = 2U;
		stResponse.ullDuplicateCount = 3U;
		stResponse.ullOutOfOrderCount = 4U;
		stResponse.ullEpochResetCount = 5U;
		stResponse.ullSequenceGapCount = 6U;
		stResponse.ullCapacityDroppedCount = 7U;
		stResponse.ullFanoutCount = 88U;
		stResponse.ullPublishFailedCount = 9U;
		ST_EVENT_HEARTBEAT_SOURCE_STATUS stSource;
		stSource.usPlatformVersion = 5U;
		stSource.iSourceNo = 1;
		stSource.ullSourceEpoch = 9001U;
		stSource.ullLastSequence = 99U;
		stSource.ullGapCount = 6U;
		stResponse.aSource.push_back(stSource);
		ST_EVENT_HEARTBEAT_RESPONSE stDecodedResponse;
		if (!Check(EncodeEventHeartbeatResponse(stResponse,
				aPayload, strError), strError.c_str()) ||
			!Check(aPayload.size() == 168U,
				"Event heartbeat response length mismatch") ||
			!Check(DecodeEventHeartbeatResponse(aPayload.data(),
				aPayload.size(), stDecodedResponse, strError),
				strError.c_str()) ||
			!Check(stDecodedResponse.bJetStreamReady &&
				stDecodedResponse.ullRequestSequence == 17U &&
				stDecodedResponse.ullFanoutCount == 88U &&
				stDecodedResponse.aSource.size() == 1U &&
				stDecodedResponse.aSource[0].ullLastSequence == 99U,
				"Event heartbeat response round trip mismatch"))
		{
			return false;
		}

		ST_EVENT_HEARTBEAT_REQUEST stInvalidRequest = stRequest;
		stInvalidRequest.enSenderPluginId = EN_PLUGIN_ID_MT_EVENT_SERVICE;
		if (!Check(!EncodeEventHeartbeatRequest(stInvalidRequest,
				aPayload, strError),
				"Event heartbeat must reject Event as requester"))
		{
			return false;
		}
		ST_EVENT_HEARTBEAT_RESPONSE stInvalidResponse = stResponse;
		stInvalidResponse.aSource[0].ullSourceEpoch = 0U;
		if (!Check(!EncodeEventHeartbeatResponse(stInvalidResponse,
				aPayload, strError),
				"Event heartbeat must reject an invalid source watermark"))
		{
			return false;
		}
		if (!Check(EncodeEventHeartbeatResponse(stResponse,
				aPayload, strError), strError.c_str()))
		{
			return false;
		}
		aPayload.push_back(0U);
		return Check(!DecodeEventHeartbeatResponse(aPayload.data(),
			aPayload.size(), stDecodedResponse, strError),
			"Event heartbeat must reject trailing bytes");
	}

	// Relay 假发布端可阻塞工作线程，便于确定性检查无限队列和淘汰最旧语义。
	class CTestMarketEventRelaySink : public IMtMarketEventRelaySink
	{
	public:
		explicit CTestMarketEventRelaySink(bool p_bBlocked = false,
			bool p_bPublishSuccess = true)
			: m_clMutex()
			, m_clCondition()
			, m_bBlocked(p_bBlocked)
			, m_bPublishSuccess(p_bPublishSuccess)
			, m_aSequence()
			, m_aPayload()
		{
		}

		bool OnMarketEventReady(const ST_PLUGIN_NOTIFY_META& p_refSourceMeta,
			const std::vector<unsigned char>& p_refPayload,
			std::string& p_refError) override
		{
			std::unique_lock<std::mutex> clLock(m_clMutex);
			m_aSequence.push_back(p_refSourceMeta.ullSequence);
			m_aPayload.push_back(p_refPayload);
			m_clCondition.notify_all();
			m_clCondition.wait(clLock, [this]() { return !m_bBlocked; });
			if (!m_bPublishSuccess)
			{
				p_refError = "TEST_EVENT_RELAY_PUBLISH_FAILED";
			}
			return m_bPublishSuccess;
		}

		bool WaitForCount(std::size_t p_szCount)
		{
			std::unique_lock<std::mutex> clLock(m_clMutex);
			return m_clCondition.wait_for(clLock,
				std::chrono::seconds(5),
				[this, p_szCount]()
				{
					return m_aSequence.size() >= p_szCount;
				});
		}

		void Release()
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			m_bBlocked = false;
			m_clCondition.notify_all();
		}

		void GetSequences(std::vector<std::uint64_t>& p_refSequence) const
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			p_refSequence = m_aSequence;
		}

		bool FirstPayloadEquals(
			const std::vector<unsigned char>& p_refPayload) const
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			return !m_aPayload.empty() && m_aPayload.front() == p_refPayload;
		}

	private:
		mutable std::mutex m_clMutex;
		std::condition_variable m_clCondition;
		bool m_bBlocked;
		bool m_bPublishSuccess;
		std::vector<std::uint64_t> m_aSequence;
		std::vector<std::vector<unsigned char>> m_aPayload;
	};

	// 构造 Quote v1、ClientData 和通知元数据三层一致的 1211 测试报文。
	bool BuildEventRelayPayload(std::uint64_t p_ullEpoch,
		std::uint64_t p_ullSequence, std::int32_t p_iSourceNo,
		std::vector<unsigned char>& p_refPayload,
		ST_PLUGIN_NOTIFY_META& p_refMeta, std::string& p_refError)
	{
		ST_QUOTE_BINARY_TICK stTick;
		SetTestQuoteDefaults(stTick);
		stTick.iSourceNo = p_iSourceNo;
		stTick.ullSourceEpoch = p_ullEpoch;
		stTick.ullIngressSequence = p_ullSequence;
		stTick.llIngressTimeMs += static_cast<std::int64_t>(p_ullSequence);
		std::vector<unsigned char> aQuotePayload;
		if (!EncodeQuoteBinaryTick(stTick, aQuotePayload, p_refError))
		{
			return false;
		}
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refQuote =
			CreatePluginBinaryValue(EN_PLUGIN_BINARY_VALUE_BYTES);
		if (!refQuote)
		{
			p_refError = "TEST_EVENT_RELAY_ALLOCATION_FAILED";
			return false;
		}
		refQuote->aByteValue = aQuotePayload;
		ST_CLIENT_DATA_BINARY_EVENT stEvent;
		stEvent.ullTopic = EN_CLIENT_DATA_BINARY_TOPIC_QUOTE;
		stEvent.iSourceVersion = stTick.usPlatformVersion;
		stEvent.iSourceNo = stTick.iSourceNo;
		stEvent.strSymbol = stTick.strSymbol;
		stEvent.ullSequence = stTick.ullIngressSequence;
		stEvent.llTimestampMs = stTick.llIngressTimeMs;
		stEvent.refData = refQuote;
		if (!EncodeClientDataBinaryEvent(stEvent, p_refPayload, p_refError))
		{
			return false;
		}
		p_refMeta = ST_PLUGIN_NOTIFY_META();
		p_refMeta.usSourcePlugin = EN_PLUGIN_ID_MT_QUOTE_SERVICE;
		p_refMeta.usNotifyMode = EN_PLUGIN_NOTIFY_MODE_BEST_EFFORT;
		p_refMeta.usNotifyAction = EN_PLUGIN_NOTIFY_ACTION_UPDATED;
		p_refMeta.uiPayloadLen = static_cast<std::uint32_t>(p_refPayload.size());
		p_refMeta.ullSequence = stEvent.ullSequence;
		p_refMeta.llTimestampMs = stEvent.llTimestampMs;
		return true;
	}

	// 校验 Event 1211 三层校验、连续性、固定来源顺序和 queueCapacity 边界。
	bool TestEventMarketRelay()
	{
		ST_MT_MARKET_EVENT_CONFIG stConfig;
		stConfig.uiWorkerThreads = 2U;
		stConfig.szQueueCapacity = 0U;
		CTestMarketEventRelaySink clSink;
		CMtMarketEventRelay clRelay;
		std::string strError;
		if (!Check(clRelay.Start(stConfig, &clSink, strError),
				strError.c_str()))
		{
			return false;
		}
		std::vector<unsigned char> aPayload;
		ST_PLUGIN_NOTIFY_META stMeta;
		if (!Check(BuildEventRelayPayload(100U, 1U, 1,
				aPayload, stMeta, strError), strError.c_str()) ||
			!Check(clRelay.Submit(stMeta, aPayload.data(),
				aPayload.size(), strError), strError.c_str()) ||
			!Check(!clRelay.Submit(stMeta, aPayload.data(),
				aPayload.size(), strError),
				"Event relay must reject a duplicate sequence"))
		{
			clRelay.Stop();
			return false;
		}
		const std::vector<unsigned char> aFirstPayload = aPayload;
		if (!Check(BuildEventRelayPayload(100U, 3U, 1,
				aPayload, stMeta, strError), strError.c_str()) ||
			!Check(clRelay.Submit(stMeta, aPayload.data(),
				aPayload.size(), strError), strError.c_str()) ||
			!Check(BuildEventRelayPayload(100U, 2U, 1,
				aPayload, stMeta, strError), strError.c_str()) ||
			!Check(!clRelay.Submit(stMeta, aPayload.data(),
				aPayload.size(), strError),
				"Event relay must reject an out-of-order sequence") ||
			!Check(BuildEventRelayPayload(101U, 1U, 1,
				aPayload, stMeta, strError), strError.c_str()) ||
			!Check(clRelay.Submit(stMeta, aPayload.data(),
				aPayload.size(), strError), strError.c_str()))
		{
			clRelay.Stop();
			return false;
		}
		ST_PLUGIN_NOTIFY_META stInvalidMeta = stMeta;
		stInvalidMeta.usSourcePlugin = EN_PLUGIN_ID_MT_TRADE_SERVICE;
		if (!Check(!clRelay.Submit(stInvalidMeta, aPayload.data(),
				aPayload.size(), strError),
				"Event relay must reject a non-Quote source") ||
			!Check(clSink.WaitForCount(3U),
				"Event relay did not fan out accepted ticks"))
		{
			clRelay.Stop();
			return false;
		}
		ST_MT_MARKET_EVENT_DIAGNOSTICS stDiagnostics;
		for (unsigned int uiWait = 0; uiWait < 500U; ++uiWait)
		{
			clRelay.GetDiagnostics(stDiagnostics);
			if (stDiagnostics.ullFanoutCount == 3U)
			{
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		std::vector<std::uint64_t> aSequence;
		clSink.GetSequences(aSequence);
		const bool bUnlimitedOk =
			stDiagnostics.ullReceivedCount == 6U &&
			stDiagnostics.ullValidationRejectedCount == 1U &&
			stDiagnostics.ullDuplicateCount == 1U &&
			stDiagnostics.ullOutOfOrderCount == 1U &&
			stDiagnostics.ullEpochResetCount == 1U &&
			stDiagnostics.ullSequenceGapCount == 1U &&
			stDiagnostics.ullCapacityDroppedCount == 0U &&
			stDiagnostics.ullFanoutCount == 3U &&
			stDiagnostics.aSource.size() == 1U &&
			stDiagnostics.aSource[0].ullSourceEpoch == 101U &&
			stDiagnostics.aSource[0].ullLastSequence == 1U &&
			aSequence == std::vector<std::uint64_t>({1U, 3U, 1U}) &&
			clSink.FirstPayloadEquals(aFirstPayload);
		clRelay.Stop();
		if (!Check(bUnlimitedOk,
				"Event relay validation, order or unlimited diagnostics mismatch"))
		{
			return false;
		}

		stConfig.uiWorkerThreads = 1U;
		stConfig.szQueueCapacity = 128U;
		CTestMarketEventRelaySink clBlockedSink(true);
		if (!Check(clRelay.Start(stConfig, &clBlockedSink, strError),
				strError.c_str()) ||
			!Check(BuildEventRelayPayload(200U, 1U, 2,
				aPayload, stMeta, strError), strError.c_str()) ||
			!Check(clRelay.Submit(stMeta, aPayload.data(),
				aPayload.size(), strError), strError.c_str()) ||
			!Check(clBlockedSink.WaitForCount(1U),
				"Event relay blocked sink was not entered"))
		{
			clBlockedSink.Release();
			clRelay.Stop();
			return false;
		}
		for (std::uint64_t ullSequence = 2U;
			ullSequence <= 130U; ++ullSequence)
		{
			if (!BuildEventRelayPayload(200U, ullSequence, 2,
				aPayload, stMeta, strError) ||
				!clRelay.Submit(stMeta, aPayload.data(),
					aPayload.size(), strError))
			{
				clBlockedSink.Release();
				clRelay.Stop();
				return Check(false, strError.c_str());
			}
		}
		clRelay.GetDiagnostics(stDiagnostics);
		const bool bDroppedOldest =
			stDiagnostics.ullQueueDepth == 128U &&
			stDiagnostics.ullCapacityDroppedCount == 1U;
		clBlockedSink.Release();
		const bool bDrained = clBlockedSink.WaitForCount(129U);
		clBlockedSink.GetSequences(aSequence);
		for (unsigned int uiWait = 0; uiWait < 500U; ++uiWait)
		{
			clRelay.GetDiagnostics(stDiagnostics);
			if (stDiagnostics.ullFanoutCount == 129U)
			{
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		clRelay.Stop();
		if (!Check(bDroppedOldest && bDrained &&
			aSequence.size() == 129U && aSequence.front() == 1U &&
			aSequence[1] == 3U && aSequence.back() == 130U &&
			stDiagnostics.ullFanoutCount == 129U,
			"Event bounded relay must evict only the oldest queued Tick"))
		{
			return false;
		}

		stConfig.szQueueCapacity = 0U;
		CTestMarketEventRelaySink clFailedSink(false, false);
		if (!Check(clRelay.Start(stConfig, &clFailedSink, strError),
				strError.c_str()) ||
			!Check(BuildEventRelayPayload(300U, 1U, 3,
				aPayload, stMeta, strError), strError.c_str()) ||
			!Check(clRelay.Submit(stMeta, aPayload.data(),
				aPayload.size(), strError), strError.c_str()) ||
			!Check(clFailedSink.WaitForCount(1U),
				"Event relay publish-failure sink was not called"))
		{
			clRelay.Stop();
			return false;
		}
		for (unsigned int uiWait = 0; uiWait < 500U; ++uiWait)
		{
			clRelay.GetDiagnostics(stDiagnostics);
			if (stDiagnostics.ullPublishFailedCount == 1U)
			{
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		clRelay.Stop();
		return Check(stDiagnostics.ullPublishFailedCount == 1U &&
			stDiagnostics.ullFanoutCount == 0U,
			"Event relay publish failure diagnostics mismatch");
	}

	// 校验 1122 单节点快照的空结果、多 Tick 往返以及高水位和长度边界。
	bool TestQuoteSnapshotProtocol()
	{
		ST_QUOTE_SNAPSHOT_BINARY_REQUEST stRequest;
		stRequest.usPlatformVersion = 5;
		stRequest.iSourceNo = 2;
		std::vector<unsigned char> aPayload;
		std::string strError;
		if (!Check(EncodeQuoteSnapshotBinaryRequest(stRequest,
				aPayload, strError), strError.c_str()) ||
			!Check(aPayload.size() == QUOTE_SNAPSHOT_REQUEST_SIZE,
				"snapshot request length mismatch"))
		{
			return false;
		}
		ST_QUOTE_SNAPSHOT_BINARY_REQUEST stDecodedRequest;
		if (!Check(DecodeQuoteSnapshotBinaryRequest(aPayload.data(),
				aPayload.size(), stDecodedRequest, strError),
				strError.c_str()) ||
			!Check(stDecodedRequest.usPlatformVersion == 5 &&
				stDecodedRequest.iSourceNo == 2,
				"snapshot request round trip mismatch"))
		{
			return false;
		}
		aPayload.push_back(0U);
		if (!Check(!DecodeQuoteSnapshotBinaryRequest(aPayload.data(),
				aPayload.size(), stDecodedRequest, strError),
				"snapshot request trailing data must be rejected"))
		{
			return false;
		}

		ST_QUOTE_SNAPSHOT_BINARY_RESPONSE stResponse;
		stResponse.usPlatformVersion = 5;
		stResponse.iSourceNo = 2;
		stResponse.ullSourceEpoch = 7002U;
		stResponse.ullHighWatermark = 102U;
		for (std::uint64_t ullIndex = 0; ullIndex < 2U; ++ullIndex)
		{
			ST_QUOTE_BINARY_TICK stTick;
			SetTestQuoteDefaults(stTick);
			stTick.usPlatformVersion = 5;
			stTick.iSourceNo = 2;
			stTick.ullSourceEpoch = 7002U;
			stTick.strSymbol = ullIndex == 0U ? "EURUSD" : "USDJPY";
			stTick.dBid = 1.1 + static_cast<double>(ullIndex);
			stTick.dAsk = stTick.dBid + 0.0001;
			stTick.llServerTime = 1710000000;
			stTick.llServerTimeMsc = 1710000000000;
			stTick.llIngressTimeMs = 1710000000100;
			stTick.ullIngressSequence = 101U + ullIndex;
			stResponse.aTick.push_back(stTick);
		}
		if (!Check(EncodeQuoteSnapshotBinaryResponse(stResponse,
				aPayload, strError), strError.c_str()))
		{
			return false;
		}
		ST_QUOTE_SNAPSHOT_BINARY_RESPONSE stDecodedResponse;
		if (!Check(DecodeQuoteSnapshotBinaryResponse(aPayload.data(),
				aPayload.size(), stDecodedResponse, strError),
				strError.c_str()) ||
			!Check(stDecodedResponse.ullHighWatermark == 102U &&
				stDecodedResponse.ullSourceEpoch == 7002U &&
				stDecodedResponse.aTick.size() == 2U &&
				stDecodedResponse.aTick[1].strSymbol == "USDJPY",
				"snapshot response round trip mismatch"))
		{
			return false;
		}
		ST_QUOTE_SNAPSHOT_BINARY_RESPONSE stInvalid = stResponse;
		stInvalid.aTick[1].ullIngressSequence = 103U;
		if (!Check(!EncodeQuoteSnapshotBinaryResponse(stInvalid,
				aPayload, strError),
				"snapshot tick above high watermark must be rejected"))
		{
			return false;
		}
		stInvalid = stResponse;
		stInvalid.aTick[1].strSymbol = stInvalid.aTick[0].strSymbol;
		if (!Check(!EncodeQuoteSnapshotBinaryResponse(stInvalid,
				aPayload, strError),
				"snapshot duplicate symbol must be rejected"))
		{
			return false;
		}
		if (!Check(EncodeQuoteSnapshotBinaryResponse(stResponse,
				aPayload, strError), strError.c_str()))
		{
			return false;
		}
		aPayload.pop_back();
		return Check(!DecodeQuoteSnapshotBinaryResponse(aPayload.data(),
			aPayload.size(), stDecodedResponse, strError),
			"truncated snapshot response must be rejected");
	}

	// 构造 Query 冷启动测试 Tick；序号和市场时间由调用方控制。
	ST_QUOTE_BINARY_TICK BuildQueryTick(std::uint64_t p_ullSequence,
		std::int64_t p_llMarketTime, double p_dPrice,
		std::uint64_t p_ullSourceEpoch = 7003U)
	{
		ST_QUOTE_BINARY_TICK stTick;
		SetTestQuoteDefaults(stTick);
		stTick.usPlatformVersion = 5;
		stTick.iSourceNo = 1;
		stTick.ullSourceEpoch = p_ullSourceEpoch;
		stTick.strSymbol = "EURUSD";
		stTick.dBid = p_dPrice;
		stTick.dAsk = p_dPrice + 0.0001;
		stTick.dLast = p_dPrice;
		stTick.ullVolumeExt = 1;
		stTick.llServerTime = p_llMarketTime;
		stTick.llServerTimeMsc = p_llMarketTime * 1000;
		stTick.llIngressTimeMs = p_llMarketTime * 1000;
		stTick.ullIngressSequence = p_ullSequence;
		return stTick;
	}

	// 验证 Query 先缓冲 1211、后安装 1122，并只应用快照高水位之后的严格递增 Tick。
	bool TestQueryQuoteBootstrap()
	{
		ST_MT_QUERY_SERVICE_CONFIG stConfig;
		stConfig.strInstanceId = "Query-Test-01";
		stConfig.szBootstrapBufferCapacity = 128;
		stConfig.szMinuteRetention = 120;
		ST_MT_QUERY_SOURCE_CONFIG stSource;
		stSource.bEnable = true;
		stSource.usVersion = 5;
		stSource.iNo = 1;
		stSource.strName = "MT5-1";
		stConfig.aSource.push_back(stSource);

		CMtQueryQuoteCache clCache;
		std::string strError;
		if (!Check(clCache.Configure(stConfig, strError),
			strError.c_str()))
		{
			return false;
		}

		// Query 不允许用进程所在机器的时区代替 MT 权威状态；测试显式安装 UTC 状态后再验证 K 线。
		const std::int64_t llNowMs =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count();
		ST_MT_TIME_STATE stTime;
		stTime.usPlatformVersion = 5;
		stTime.iSourceNo = 1;
		stTime.strTimeZoneId = "UTC";
		stTime.iStandardOffsetSeconds = 0;
		stTime.iCurrentOffsetSeconds = 0;
		stTime.bDaylight = false;
		stTime.enSyncState = EN_MT_TIME_SYNC_READY;
		stTime.enChangeReason = EN_MT_TIME_CHANGE_SDK_UPDATE;
		stTime.ullAuthorityEpoch = 1U;
		stTime.ullGeneration = 1U;
		stTime.llEffectiveUtcMs = llNowMs - 1000;
		stTime.llSampledUtcMs = llNowMs;
		stTime.llValidUntilUtcMs = llNowMs + 60000;
		if (!Check(clCache.UpdateTimeState(stTime, strError),
				strError.c_str()) ||
			!Check(clCache.OnIncrement(BuildQueryTick(
				101U, 1710000001, 1.1010), strError),
				strError.c_str()) ||
			!Check(clCache.OnIncrement(BuildQueryTick(
				102U, 1710000061, 1.1020), strError),
				strError.c_str()))
		{
			return false;
		}

		ST_QUOTE_SNAPSHOT_BINARY_RESPONSE stSnapshot;
		stSnapshot.usPlatformVersion = 5;
		stSnapshot.iSourceNo = 1;
		stSnapshot.ullSourceEpoch = 7003U;
		stSnapshot.ullHighWatermark = 100U;
		stSnapshot.aTick.push_back(BuildQueryTick(
			100U, 1709999941, 1.1000));
		if (!Check(clCache.InstallSnapshot(stSnapshot,
				strError), strError.c_str()))
		{
			return false;
		}
		std::vector<ST_QUOTE_BINARY_TICK> aTick;
		ST_MT_QUERY_QUOTE_SOURCE_STATUS stStatus;
		const std::set<std::string> setEmpty;
		if (!Check(clCache.GetQuotes(5, 1, setEmpty,
				aTick, stStatus, strError), strError.c_str()) ||
			!Check(aTick.size() == 1U &&
				aTick[0].ullIngressSequence == 102U &&
				stStatus.ullHighWatermark == 102U &&
				stStatus.bSnapshotReady,
				"Query snapshot merge high watermark mismatch"))
		{
			return false;
		}
		if (!Check(clCache.OnIncrement(BuildQueryTick(
				102U, 1710000061, 9.9999), strError),
				strError.c_str()) ||
			!Check(clCache.GetQuotes(5, 1, setEmpty,
				aTick, stStatus, strError), strError.c_str()) ||
			!Check(aTick[0].dLast == 1.1020,
				"duplicate Query Quote sequence must be ignored"))
		{
			return false;
		}
		if (!Check(clCache.OnIncrement(BuildQueryTick(
				1U, 1710000121, 1.2000, 7004U), strError),
				strError.c_str()) ||
			!Check(clCache.OnIncrement(BuildQueryTick(
				103U, 1710000181, 9.9999, 7003U), strError),
				strError.c_str()) ||
			!Check(clCache.GetQuotes(5, 1, setEmpty,
				aTick, stStatus, strError), strError.c_str()) ||
			!Check(aTick[0].dLast == 1.2000 &&
				stStatus.ullSourceEpoch == 7004U &&
				stStatus.ullHighWatermark == 1U,
				"Query must reset sequence comparison on a new SourceEpoch and reject retired epoch ticks"))
		{
			return false;
		}

		std::vector<ST_MT_QUERY_BAR> aBars;
		if (!Check(!clCache.GetBars(5, 1, "EURUSD", 1,
				0, 0, 10U, aBars, strError),
				"Query must not generate M1 from Best Effort 1211"))
		{
			return false;
		}
		ST_DERIVE_M1_SNAPSHOT_RESPONSE stM1Snapshot;
		stM1Snapshot.enState =
			EN_DERIVE_SERVICE_STATE_READY;
		stM1Snapshot.iCode = 0;
		stM1Snapshot.strMessage = "OK";
		for (std::uint64_t ullIndex = 0;
			ullIndex < 2U; ++ullIndex)
		{
			ST_DERIVE_M1_BAR stBar;
			stBar.usPlatformVersion = 5;
			stBar.iSourceNo = 1;
			stBar.ullSourceEpoch = 7003U;
			stBar.strSymbol = "EURUSD";
			stBar.llMinute =
				1710000000 +
				static_cast<std::int64_t>(
					ullIndex * 60U);
			stBar.uiServerDate = 20240309U;
			stBar.uiFlags = DERIVE_M1_FLAG_OPEN_PRICE_CONFIRMED;
			stBar.dOpen = 1.1010 +
				static_cast<double>(ullIndex) *
				0.0010;
			stBar.dHigh = stBar.dOpen;
			stBar.dLow = stBar.dOpen;
			stBar.dClose = stBar.dOpen;
			stBar.ullTickVolume = 1;
			stBar.ullRealVolume = 1;
			stBar.ullFirstSequence =
				101U + ullIndex;
			stBar.ullLastSequence =
				101U + ullIndex;
			stM1Snapshot.aBar.push_back(stBar);
		}
		if (!Check(clCache.InstallM1Snapshot(
				5, 1, stM1Snapshot, strError),
				strError.c_str()) ||
			!Check(clCache.GetBars(5, 1, "EURUSD", 1,
				0, 0, 10U, aBars, strError),
				strError.c_str()) ||
			!Check(aBars.size() == 2U &&
				std::abs(aBars[0].dClose - 1.1010) <
					0.0000001 &&
				std::abs(aBars[1].dClose - 1.1020) <
					0.0000001,
				"Query Derive M1 snapshot mismatch") ||
			!Check(clCache.GetQuotes(5, 1, setEmpty,
				aTick, stStatus, strError), strError.c_str()) ||
			!Check(aTick.size() == 1U &&
				std::abs(clCache.GetOpenPrice(5, 1, "EURUSD") -
					1.1010) < 0.0000001,
				"Query did not expose confirmed M1 OpenPrice"))
		{
			return false;
		}
		ST_DERIVE_M1_SNAPSHOT_RESPONSE stM1Update;
		stM1Update.enState =
			EN_DERIVE_SERVICE_STATE_READY;
		stM1Update.iCode = 0;
		stM1Update.strMessage = "OK";
		ST_DERIVE_M1_BAR stUpdatedBar =
			stM1Snapshot.aBar.back();
		stUpdatedBar.dHigh = 1.1030;
		stUpdatedBar.dClose = 1.1030;
		stUpdatedBar.ullTickVolume = 2;
		stUpdatedBar.ullRealVolume = 2;
		stUpdatedBar.ullLastSequence = 103U;
		stM1Update.aBar.push_back(stUpdatedBar);
		if (!Check(clCache.OnM1Increment(
				stM1Update, strError),
				strError.c_str()) ||
			!Check(clCache.GetBars(5, 1, "EURUSD", 1,
				0, 0, 10U, aBars, strError),
				strError.c_str()) ||
			!Check(aBars.back().dClose == 1.1030 &&
				aBars.back().ullTickVolume == 2U,
				"Query Derive M1 increment mismatch"))
		{
			return false;
		}

		CMtQueryDispatcher clDispatcher;
		CMtQueryNodeManager clNodeManager;
		if (!Check(clDispatcher.Initialize(&stConfig,
				&clCache, &clNodeManager, strError),
				strError.c_str()))
		{
			return false;
		}
		ST_PLUGIN_BINARY_VALUE stRequest;
		stRequest.enType = EN_PLUGIN_BINARY_VALUE_OBJECT;
		std::int32_t iCode = 0;
		std::string strMessage;
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refData;
		if (!Check(clDispatcher.Dispatch(
				EN_PLUGIN_FUNC_QUERY_QUOTES, 0, stRequest,
				iCode, strMessage, refData, strError),
				strError.c_str()) ||
			!Check(refData != nullptr &&
				refData->enType ==
					EN_PLUGIN_BINARY_VALUE_ARRAY &&
				refData->aArrayValue.size() == 1U,
				"Query quotes dispatcher response mismatch"))
		{
			return false;
		}
		if (!Check(clDispatcher.Dispatch(
				EN_PLUGIN_FUNC_QUERY_SYMBOLS, 0, stRequest,
				iCode, strMessage, refData, strError),
				strError.c_str()) ||
			!Check(iCode ==
				EN_TERMINAL_ERROR_MT_NO_AVAILABLE_SERVER,
				"uninitialized MT adapter must preserve V2 error code"))
		{
			return false;
		}
		if (!Check(clDispatcher.Dispatch(
				EN_PLUGIN_FUNC_QUERY_WATCHLIST,
				EN_PLUGIN_WATCHLIST_ROUTE_SYMBOLS,
				stRequest, iCode, strMessage, refData,
				strError), strError.c_str()) ||
			!Check(iCode ==
				EN_TERMINAL_ERROR_POSTGRES_DISABLED,
				"disabled PostgreSQL must preserve V2 error code"))
		{
			return false;
		}

		// Query 响应允许数组根，网关解码后才能保持 V2 Data 数组语义。
		std::vector<unsigned char> aResponse;
		if (!Check(EncodeQueryBinaryResponse(0, "OK",
				*refData, aResponse, strError),
				strError.c_str()))
		{
			return false;
		}
		ST_PLUGIN_BINARY_DOCUMENT stDocument;
		return Check(DecodeQueryBinaryDocument(aResponse.data(),
			aResponse.size(), stDocument, strError),
			strError.c_str());
	}

	// 校验 1152 只返回 V2 ServerVersion/Mt4Managers/Mt5Managers，并按 Version/No 和时间状态过滤。
	bool TestQueryServerInfoCompatibility()
	{
		ST_MT_QUERY_SERVICE_CONFIG stConfig;
		stConfig.strInstanceId = "Query-Test-Server-Info";
		ST_MT_QUERY_SOURCE_CONFIG stMt4;
		stMt4.bEnable = true;
		stMt4.bMock = true;
		stMt4.usVersion = 4;
		stMt4.iNo = 1;
		stMt4.strName = "MT4-A";
		AddQueryTestConnections(stMt4);
		ST_MT_QUERY_SOURCE_CONFIG stMt5;
		stMt5.bEnable = true;
		stMt5.bMock = true;
		stMt5.usVersion = 5;
		stMt5.iNo = 2;
		stMt5.strName = "MT5-B";
		AddQueryTestConnections(stMt5);
		stConfig.aSource.push_back(stMt4);
		stConfig.aSource.push_back(stMt5);

		CMtQueryQuoteCache clCache;
		CMtQueryNodeManager clNodeManager;
		CMtQueryDispatcher clDispatcher;
		std::string strError;
		if (!Check(clCache.Configure(stConfig, strError),
				strError.c_str()) ||
			!Check(clDispatcher.Initialize(&stConfig, &clCache,
				&clNodeManager, strError, nullptr, "1.2.3.4"),
				strError.c_str()))
		{
			return false;
		}
		const std::int64_t llNowMs =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count();
		const auto fnBuildTimeState =
			[llNowMs](std::uint16_t p_usVersion,
				std::int32_t p_iNo, std::uint64_t p_ullGeneration)
			{
				ST_MT_TIME_STATE stState;
				stState.usPlatformVersion = p_usVersion;
				stState.iSourceNo = p_iNo;
				stState.strTimeZoneId = "China Standard Time";
				stState.iStandardOffsetSeconds = 28800;
				stState.iCurrentOffsetSeconds = 28800;
				stState.bDaylight = false;
				stState.enSyncState = EN_MT_TIME_SYNC_READY;
				stState.enChangeReason = EN_MT_TIME_CHANGE_SDK_UPDATE;
				stState.ullAuthorityEpoch = 100U + p_ullGeneration;
				stState.ullGeneration = p_ullGeneration;
				stState.llEffectiveUtcMs = llNowMs - 1000;
				stState.llSampledUtcMs = llNowMs;
				stState.llValidUntilUtcMs = llNowMs + 60000;
				return stState;
			};
		if (!Check(clCache.UpdateTimeState(
				fnBuildTimeState(4, 1, 1U), strError),
				strError.c_str()))
		{
			return false;
		}

		const auto fnAddInt = [](ST_PLUGIN_BINARY_VALUE& p_refRequest,
			const char* p_szName, std::int64_t p_llValue)
			{
				ST_PLUGIN_BINARY_FIELD stField;
				stField.strName = p_szName;
				stField.refValue = CreatePluginBinaryValue(
					EN_PLUGIN_BINARY_VALUE_INT64);
				stField.refValue->llIntValue = p_llValue;
				p_refRequest.aObjectField.push_back(stField);
			};
		const auto fnAddBool = [](ST_PLUGIN_BINARY_VALUE& p_refRequest,
			const char* p_szName, bool p_bValue)
			{
				ST_PLUGIN_BINARY_FIELD stField;
				stField.strName = p_szName;
				stField.refValue = CreatePluginBinaryValue(
					EN_PLUGIN_BINARY_VALUE_BOOL);
				stField.refValue->ucBoolValue = p_bValue ? 1U : 0U;
				p_refRequest.aObjectField.push_back(stField);
			};

		// 第一步：Version=0 选择全部节点，任一选中节点时间未就绪时返回 V2 20008。
		ST_PLUGIN_BINARY_VALUE stAllRequest;
		stAllRequest.enType = EN_PLUGIN_BINARY_VALUE_OBJECT;
		fnAddInt(stAllRequest, "Version", 0);
		std::int32_t iCode = EN_TERMINAL_ERROR_INTERNAL_ERROR;
		std::string strMessage;
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refData;
		if (!Check(clDispatcher.Dispatch(
				EN_PLUGIN_FUNC_QUERY_SERVER_INFO, 0,
				stAllRequest, iCode, strMessage, refData,
				strError), strError.c_str()) ||
			!Check(iCode == EN_TERMINAL_ERROR_MT_SYSTEM_NOT_READY,
				"server_info must wait for every selected time state"))
		{
			return false;
		}
		if (!Check(clCache.UpdateTimeState(
				fnBuildTimeState(5, 2, 2U), strError),
				strError.c_str()) ||
			!Check(clDispatcher.Dispatch(
				EN_PLUGIN_FUNC_QUERY_SERVER_INFO, 0,
				stAllRequest, iCode, strMessage, refData,
				strError), strError.c_str()) ||
			!Check(iCode == EN_TERMINAL_ERROR_OK && refData != nullptr,
				"server_info all-platform request failed"))
		{
			return false;
		}
		const ST_PLUGIN_BINARY_VALUE* pVersion =
			FindPluginBinaryField(*refData, "ServerVersion");
		const ST_PLUGIN_BINARY_VALUE* pMt4 =
			FindPluginBinaryField(*refData, "Mt4Managers");
		const ST_PLUGIN_BINARY_VALUE* pMt5 =
			FindPluginBinaryField(*refData, "Mt5Managers");
		if (!Check(refData->aObjectField.size() == 3U &&
				pVersion != nullptr &&
				pVersion->enType == EN_PLUGIN_BINARY_VALUE_STRING &&
				pVersion->strStringValue == "1.2.3.4" &&
				pMt4 != nullptr && pMt5 != nullptr &&
				pMt4->enType == EN_PLUGIN_BINARY_VALUE_ARRAY &&
				pMt5->enType == EN_PLUGIN_BINARY_VALUE_ARRAY &&
				pMt4->aArrayValue.size() == 1U &&
				pMt5->aArrayValue.size() == 1U,
				"server_info external response shape is not V2 compatible"))
		{
			return false;
		}
		const ST_PLUGIN_BINARY_VALUE& refMt5Item =
			*pMt5->aArrayValue.front();
		if (!Check(refMt5Item.aObjectField.size() == 3U &&
				FindPluginBinaryField(refMt5Item, "Id") != nullptr &&
				FindPluginBinaryField(refMt5Item, "Name") != nullptr &&
				FindPluginBinaryField(refMt5Item,
					"ServerTimeOffset") != nullptr,
				"server_info manager item leaked or omitted fields"))
		{
			return false;
		}

		// 第二步：Version+No 精确筛选，WebUserStats 只由 Gateway 按请求选项补充。
		ST_PLUGIN_BINARY_VALUE stMt5Request;
		stMt5Request.enType = EN_PLUGIN_BINARY_VALUE_OBJECT;
		fnAddInt(stMt5Request, "Version", 5);
		fnAddInt(stMt5Request, "No", 2);
		fnAddBool(stMt5Request, "WebUserStats", true);
		if (!Check(clDispatcher.Dispatch(
				EN_PLUGIN_FUNC_QUERY_SERVER_INFO, 0,
				stMt5Request, iCode, strMessage, refData,
				strError), strError.c_str()) ||
			!Check(iCode == EN_TERMINAL_ERROR_OK &&
				FindPluginBinaryField(*refData,
					"WebUserStats") == nullptr &&
				FindPluginBinaryField(*refData,
					"Mt4Managers")->aArrayValue.empty() &&
				FindPluginBinaryField(*refData,
					"Mt5Managers")->aArrayValue.size() == 1U,
				"server_info Version/No filter or Web stats boundary mismatch"))
		{
			return false;
		}

		// 第三步：直接 Binary 调用仍拒绝缺失 Version 和未发布字段。
		ST_PLUGIN_BINARY_VALUE stInvalid;
		stInvalid.enType = EN_PLUGIN_BINARY_VALUE_OBJECT;
		if (!Check(clDispatcher.Dispatch(
				EN_PLUGIN_FUNC_QUERY_SERVER_INFO, 0,
				stInvalid, iCode, strMessage, refData,
				strError), strError.c_str()) ||
			!Check(iCode == EN_TERMINAL_ERROR_MT_PARAM,
				"server_info direct call must require Version"))
		{
			return false;
		}
		fnAddInt(stInvalid, "Version", 5);
		fnAddInt(stInvalid, "Diagnostics", 1);
		return Check(clDispatcher.Dispatch(
				EN_PLUGIN_FUNC_QUERY_SERVER_INFO, 0,
				stInvalid, iCode, strMessage, refData,
				strError), strError.c_str()) &&
			Check(iCode == EN_TERMINAL_ERROR_MT_PARAM,
				"server_info direct call must reject diagnostics fields");
	}

	// 验证 Windows 动态时区按目标日期应用 DST，并保持 MT4/MT5 周线起点差异。
	bool TestQueryTimeZoneDst()
	{
		const auto fnUtc = [](int p_iYear, int p_iMonth,
			int p_iDay, int p_iHour, int p_iMinute)
		{
			tm stTime = {};
			stTime.tm_year = p_iYear - 1900;
			stTime.tm_mon = p_iMonth - 1;
			stTime.tm_mday = p_iDay;
			stTime.tm_hour = p_iHour;
			stTime.tm_min = p_iMinute;
			return static_cast<std::int64_t>(_mkgmtime64(&stTime));
		};
		const std::string strZone = "Pacific Standard Time";
		std::string strError;
		std::int32_t iBeforeOffset = 0;
		std::int32_t iAfterOffset = 0;
		const std::int64_t llBefore = fnUtc(2024, 3, 10, 9, 30);
		const std::int64_t llAfter = fnUtc(2024, 3, 10, 10, 30);
		if (!Check(CMtQueryTimeZone::ResolveUtcOffsetSeconds(
				strZone, llBefore, iBeforeOffset, strError),
				strError.c_str()) ||
			!Check(CMtQueryTimeZone::ResolveUtcOffsetSeconds(
				strZone, llAfter, iAfterOffset, strError),
				strError.c_str()) ||
			!Check(iBeforeOffset == -8 * 60 * 60 &&
				iAfterOffset == -7 * 60 * 60,
				"Pacific DST offsets are incorrect"))
		{
			return false;
		}

		// 春季 02:30 不存在；秋季 01:30 重复时固定选择较晚的标准时间 09:30 UTC。
		std::int64_t llUtc = 0;
		if (!Check(!CMtQueryTimeZone::ServerEpochToUtc(strZone,
				fnUtc(2024, 3, 10, 2, 30), llUtc, strError),
				"DST skipped local time must be rejected") ||
			!Check(CMtQueryTimeZone::ServerEpochToUtc(strZone,
				fnUtc(2024, 11, 3, 1, 30), llUtc, strError),
				strError.c_str()) ||
			!Check(llUtc == fnUtc(2024, 11, 3, 9, 30),
				"DST repeated local time must select standard time"))
		{
			return false;
		}

		// 实际 1244 切换日志优先于 Windows 推算：Broker 可在标准规则之外继续使用标准偏移。
		ST_MT_QUERY_TIME_TRANSITION stActualTransition;
		stActualTransition.llEffectiveUtcMs =
			fnUtc(2024, 3, 10, 0, 0) * 1000LL;
		stActualTransition.iOffsetSeconds = -8 * 60 * 60;
		stActualTransition.ullAuthorityEpoch = 900U;
		stActualTransition.ullGeneration = 2U;
		std::vector<ST_MT_QUERY_TIME_TRANSITION> aActualTransition(
			1U, stActualTransition);
		std::int64_t llActualServer = 0;
		std::int64_t llActualUtc = 0;
		if (!Check(CMtQueryTimeZone::UtcToServerEpoch(strZone,
				aActualTransition, fnUtc(2024, 3, 10, 20, 0),
				llActualServer, strError), strError.c_str()) ||
			!Check(llActualServer == fnUtc(2024, 3, 10, 12, 0),
				"actual 1244 offset did not override Windows DST") ||
			!Check(CMtQueryTimeZone::ServerEpochToUtc(strZone,
				aActualTransition, llActualServer, llActualUtc,
				strError), strError.c_str()) ||
			!Check(llActualUtc == fnUtc(2024, 3, 10, 20, 0),
				"actual 1244 reverse conversion mismatch"))
		{
			return false;
		}

		// DST 切换日相邻 D1 起点相差 23 小时，证明没有复用当前固定偏移。
		std::int64_t llDayBefore = 0;
		std::int64_t llDayAfter = 0;
		if (!Check(CMtQueryTimeZone::ResolveBarBucket(5U,
				strZone, fnUtc(2024, 3, 10, 20, 0), 1440,
				llDayBefore, strError), strError.c_str()) ||
			!Check(CMtQueryTimeZone::ResolveBarBucket(5U,
				strZone, fnUtc(2024, 3, 11, 20, 0), 1440,
				llDayAfter, strError), strError.c_str()) ||
			!Check(llDayAfter - llDayBefore == 23 * 60 * 60,
				"DST daily buckets must preserve a 23-hour transition day"))
		{
			return false;
		}

		std::int64_t llMt4Week = 0;
		std::int64_t llMt5Week = 0;
		return Check(CMtQueryTimeZone::ResolveBarBucket(4U,
				strZone, fnUtc(2024, 3, 13, 20, 0), 10080,
				llMt4Week, strError), strError.c_str()) &&
			Check(CMtQueryTimeZone::ResolveBarBucket(5U,
				strZone, fnUtc(2024, 3, 13, 20, 0), 10080,
				llMt5Week, strError), strError.c_str()) &&
			Check(llMt4Week == fnUtc(2024, 3, 11, 7, 0) &&
				llMt5Week == fnUtc(2024, 3, 10, 8, 0),
				"MT4 Monday and MT5 Sunday week starts differ");
	}

	// 验证 /bars 的文件校验、损坏恢复、停机重开及“历史+实时”覆盖优先级。
	bool TestQueryBarsHistoryClosure()
	{
		const std::filesystem::path clDirectory =
			std::filesystem::temp_directory_path() /
			"MtQueryBarsHistoryClosure";
		CTestDirectoryGuard clGuard(clDirectory);
		std::error_code stFsError;
		std::filesystem::create_directories(clDirectory,
			stFsError);
		if (!Check(!stFsError,
			"failed to create Query bars test directory"))
		{
			return false;
		}

		CMtQueryBarHistoryStore clStore;
		std::string strError;
		if (!Check(clStore.Initialize(clDirectory.u8string(),
				strError), strError.c_str()))
		{
			return false;
		}
		ST_MT_QUERY_BAR_HISTORY_KEY stKey;
		stKey.usVersion = 5;
		stKey.iNo = 1;
		stKey.strSymbol = "EURUSD";
		stKey.llIntervalMinutes = 1;
		// 第一步：同业务键回填租约必须非阻塞互斥，释放后允许下一请求立即取得。
		bool bFirstLease = false;
		bool bSecondLease = false;
		bool bThirdLease = false;
		if (!Check(clStore.TryBeginBackfill(stKey, bFirstLease,
				strError), strError.c_str()) ||
			!Check(bFirstLease,
				"Query first bar backfill lease was not acquired") ||
			!Check(clStore.TryBeginBackfill(stKey, bSecondLease,
				strError), strError.c_str()) ||
			!Check(!bSecondLease,
				"Query duplicate bar backfill lease must not wait or acquire"))
		{
			clStore.EndBackfill(stKey);
			clStore.Stop();
			return false;
		}
		clStore.EndBackfill(stKey);
		if (!Check(clStore.TryBeginBackfill(stKey, bThirdLease,
				strError), strError.c_str()) ||
			!Check(bThirdLease,
				"Query released bar backfill lease was not reusable"))
		{
			clStore.Stop();
			return false;
		}
		clStore.EndBackfill(stKey);
		// 第二步：租约续期不改变业务代次；Generation、offset 或 DST 规则变化必须拒绝旧查询结果。
		ST_MT_QUERY_TIME_CONTEXT stExpectedTime;
		stExpectedTime.bReady = true;
		stExpectedTime.strTimeZoneId = "China Standard Time";
		stExpectedTime.iOffsetSeconds = 28800;
		stExpectedTime.ullAuthorityEpoch = 7U;
		stExpectedTime.ullGeneration = 9U;
		stExpectedTime.llValidUntilUtcMs = 1000;
		ST_MT_QUERY_TIME_CONTEXT stCurrentTime = stExpectedTime;
		stCurrentTime.llValidUntilUtcMs = 2000;
		if (!Check(IsMtQueryTimeContextSameGeneration(
				stExpectedTime, stCurrentTime),
				"Query time lease renewal must keep the same generation"))
		{
			clStore.Stop();
			return false;
		}
		++stCurrentTime.ullGeneration;
		if (!Check(!IsMtQueryTimeContextSameGeneration(
				stExpectedTime, stCurrentTime),
				"Query changed time generation must invalidate old bars"))
		{
			clStore.Stop();
			return false;
		}
		std::vector<ST_MT_QUERY_BAR> aSeed;
		for (std::uint64_t ullIndex = 0; ullIndex < 3U; ++ullIndex)
		{
			ST_MT_QUERY_BAR stBar;
			stBar.llDateTime = 1710000000 +
				static_cast<std::int64_t>(ullIndex * 60U);
			stBar.dOpen = 1.1000 + static_cast<double>(ullIndex) * 0.0010;
			stBar.dHigh = stBar.dOpen + 0.0002;
			stBar.dLow = stBar.dOpen - 0.0002;
			stBar.dClose = stBar.dOpen + 0.0001;
			stBar.ullTickVolume = 10U + ullIndex;
			aSeed.push_back(stBar);
		}
		std::vector<ST_MT_QUERY_BAR> aSelected;
		CMtQueryBarHistoryStore::SelectBars(aSeed,
			aSeed[1].llDateTime, aSeed[2].llDateTime,
			0U, 0U, aSelected);
		if (!Check(aSelected.size() == 2U &&
				aSelected.front().llDateTime == aSeed[1].llDateTime,
				"Query bar range selection mismatch"))
		{
			return false;
		}
		CMtQueryBarHistoryStore::SelectBars(aSeed,
			0, 0, 1U, 1U, aSelected);
		if (!Check(aSelected.size() == 1U &&
				aSelected.front().llDateTime == aSeed[1].llDateTime,
				"Query bar compatible Offset selection mismatch"))
		{
			return false;
		}
		ST_MT_QUERY_BAR_COVERAGE_RANGE stCoverageFirst;
		stCoverageFirst.llFrom = aSeed.front().llDateTime;
		stCoverageFirst.llTo = aSeed[1].llDateTime - 1;
		ST_MT_QUERY_BAR_COVERAGE_RANGE stCoverageSecond;
		stCoverageSecond.llFrom = aSeed[1].llDateTime;
		stCoverageSecond.llTo = aSeed.back().llDateTime + 59;
		std::vector<ST_MT_QUERY_BAR_COVERAGE_RANGE> aCoverageSeed;
		aCoverageSeed.push_back(stCoverageFirst);
		aCoverageSeed.push_back(stCoverageSecond);
		if (!Check(clStore.EnqueueMerge(stKey, aSeed,
				aCoverageSeed,
				strError), strError.c_str()) ||
			!Check(clStore.Flush(5000U, strError),
				strError.c_str()))
		{
			return false;
		}
		std::vector<ST_MT_QUERY_BAR_COVERAGE_RANGE> aCoverageRead;
		EN_MT_QUERY_BAR_HISTORY_READ_STATE enCoverageRead =
			EN_MT_QUERY_BAR_HISTORY_MISSING;
		if (!Check(clStore.ReadCoverage(stKey, aCoverageRead,
				enCoverageRead, strError), strError.c_str()) ||
			!Check(enCoverageRead == EN_MT_QUERY_BAR_HISTORY_READY &&
				aCoverageRead.size() == 1U &&
				aCoverageRead[0].llFrom == aSeed.front().llDateTime &&
				aCoverageRead[0].llTo == aSeed.back().llDateTime + 59,
				"Query bar coverage merge or persistence mismatch"))
		{
			return false;
		}
		std::vector<ST_MT_QUERY_BAR_COVERAGE_RANGE> aMissing;
		CMtQueryBarHistoryStore::FindUncovered(aCoverageRead,
			aSeed.front().llDateTime - 60,
			aSeed.back().llDateTime + 119, aMissing);
		if (!Check(aMissing.size() == 2U &&
				aMissing[0].llFrom == aSeed.front().llDateTime - 60 &&
				aMissing[0].llTo == aSeed.front().llDateTime - 1 &&
				aMissing[1].llFrom == aSeed.back().llDateTime + 60 &&
				aMissing[1].llTo == aSeed.back().llDateTime + 119,
				"Query bar uncovered range calculation mismatch"))
		{
			return false;
		}
		// coverage 可以独立于 Bars 保存，证明成功空区间不会被下一请求重复回源。
		ST_MT_QUERY_BAR_COVERAGE_RANGE stEmptyCoverage;
		stEmptyCoverage.llFrom = aSeed.back().llDateTime + 60;
		stEmptyCoverage.llTo = aSeed.back().llDateTime + 119;
		std::vector<ST_MT_QUERY_BAR_COVERAGE_RANGE> aEmptyCoverage(
			1U, stEmptyCoverage);
		const std::vector<ST_MT_QUERY_BAR> aNoBars;
		if (!Check(clStore.EnqueueMerge(stKey, aNoBars,
				aEmptyCoverage, strError), strError.c_str()) ||
			!Check(clStore.Flush(5000U, strError), strError.c_str()) ||
			!Check(clStore.ReadCoverage(stKey, aCoverageRead,
				enCoverageRead, strError), strError.c_str()) ||
			!Check(aCoverageRead.size() == 1U &&
				aCoverageRead[0].llTo == stEmptyCoverage.llTo,
				"Query empty history coverage was not persisted"))
		{
			return false;
		}
		std::vector<ST_MT_QUERY_BAR> aRead;
		EN_MT_QUERY_BAR_HISTORY_READ_STATE enRead =
			EN_MT_QUERY_BAR_HISTORY_MISSING;
		if (!Check(clStore.Read(stKey, aRead, enRead,
				strError), strError.c_str()) ||
			!Check(enRead == EN_MT_QUERY_BAR_HISTORY_READY &&
				aRead.size() == 3U,
				"Query bar history initial read mismatch"))
		{
			return false;
		}
		std::vector<ST_MT_QUERY_BAR> aStreamSelected;
		if (!Check(clStore.ReadSelected(stKey,
				aSeed[1].llDateTime, aSeed[2].llDateTime, 0U,
				aStreamSelected, enRead, strError), strError.c_str()) ||
			!Check(aStreamSelected.size() == 2U &&
				aStreamSelected.front().llDateTime ==
					aSeed[1].llDateTime,
				"Query streaming range read mismatch") ||
			!Check(clStore.ReadSelected(stKey, 0, 0, 2U,
				aStreamSelected, enRead, strError), strError.c_str()) ||
			!Check(aStreamSelected.size() == 2U &&
				aStreamSelected.front().llDateTime ==
					aSeed[1].llDateTime,
				"Query streaming tail read mismatch"))
		{
			return false;
		}
		ST_MT_QUERY_BAR_STORE_STATS stStoreStats;
		clStore.GetStatus(stStoreStats);
		if (!Check(stStoreStats.szLockShards == 256U &&
				stStoreStats.szPendingTasks == 0U &&
				stStoreStats.szPendingBars == 0U &&
				stStoreStats.szPendingBytes == 0U,
			"Query history queue or fixed lock shard metrics mismatch"))
		{
			return false;
		}
		std::vector<ST_MT_QUERY_BAR> aOversized(262145U,
			aSeed.front());
		if (!Check(!clStore.EnqueueMerge(stKey, aOversized,
				strError) && strError.find("QUEUE_BAR_LIMIT") !=
					std::string::npos,
			"Query history queue must reject more than 262144 Bars before copying"))
		{
			return false;
		}
		// 保存侧必须依次覆盖首次写入、顺序追加和重叠修复，旧历史始终由固定缓冲流式处理。
		ST_MT_QUERY_BAR_HISTORY_KEY stStreamKey = stKey;
		stStreamKey.strSymbol = "STREAMUSD";
		std::vector<ST_MT_QUERY_BAR> aStreamSeed;
		aStreamSeed.reserve(4096U);
		for (std::size_t szIndex = 0; szIndex < 4096U; ++szIndex)
		{
			ST_MT_QUERY_BAR stBar = aSeed.front();
			stBar.llDateTime = 1720000000LL +
				static_cast<std::int64_t>(szIndex * 60U);
			stBar.dOpen += static_cast<double>(szIndex) * 0.000001;
			stBar.dHigh = stBar.dOpen + 0.0002;
			stBar.dLow = stBar.dOpen - 0.0002;
			stBar.dClose = stBar.dOpen + 0.0001;
			stBar.ullTickVolume = 100U + szIndex;
			aStreamSeed.push_back(stBar);
		}
		const std::vector<ST_MT_QUERY_BAR_COVERAGE_RANGE> aNoCoverage;
		if (!Check(clStore.CommitArchive(stStreamKey, aStreamSeed,
				aNoCoverage, strError), strError.c_str()))
		{
			return false;
		}
		std::vector<ST_MT_QUERY_BAR> aStreamAppend;
		for (std::size_t szIndex = 0; szIndex < 2U; ++szIndex)
		{
			ST_MT_QUERY_BAR stBar = aStreamSeed.back();
			stBar.llDateTime += static_cast<std::int64_t>(
				(szIndex + 1U) * 60U);
			stBar.dClose += static_cast<double>(szIndex + 1U) * 0.001;
			stBar.ullTickVolume += szIndex + 1U;
			aStreamAppend.push_back(stBar);
		}
		if (!Check(clStore.CommitArchive(stStreamKey, aStreamAppend,
				aNoCoverage, strError), strError.c_str()))
		{
			return false;
		}
		ST_MT_QUERY_BAR stStreamRepair = aStreamSeed[2048U];
		stStreamRepair.dClose = 9.8765;
		stStreamRepair.ullSourceEpoch = 88U;
		stStreamRepair.ullLastSequence = 9001U;
		const std::vector<ST_MT_QUERY_BAR> aStreamRepair(
			1U, stStreamRepair);
		if (!Check(clStore.CommitArchive(stStreamKey, aStreamRepair,
				aNoCoverage, strError), strError.c_str()))
		{
			return false;
		}
		std::vector<ST_MT_QUERY_BAR> aStreamVerify;
		if (!Check(clStore.ReadSelected(stStreamKey,
				stStreamRepair.llDateTime, stStreamRepair.llDateTime,
				0U, aStreamVerify, enRead, strError), strError.c_str()) ||
			!Check(aStreamVerify.size() == 1U &&
				aStreamVerify[0].dClose == 9.8765 &&
				aStreamVerify[0].ullLastSequence == 9001U,
				"Query streaming repair did not preserve newer Bar") ||
			!Check(clStore.ReadSelected(stStreamKey, 0, 0, 2U,
				aStreamVerify, enRead, strError), strError.c_str()) ||
			!Check(aStreamVerify.size() == 2U &&
				aStreamVerify.back().llDateTime ==
					aStreamAppend.back().llDateTime,
				"Query streaming append tail mismatch"))
		{
			return false;
		}
		// 第一步：同一分钟的 Derive 记录具有来源代次和序号，应覆盖无序号的 MT 历史记录。
		ST_MT_QUERY_BAR stRealtime = aSeed.back();
		stRealtime.dClose = 1.2345;
		stRealtime.ullSourceEpoch = 77U;
		stRealtime.ullLastSequence = 900U;
		std::vector<ST_MT_QUERY_BAR> aRealtime(1U, stRealtime);
		CMtQueryBarHistoryStore::MergeBars(aRead, aRealtime);
		if (!Check(aRead.back().dClose == 1.2345,
			"Derive sequence did not override MT history") ||
			!Check(clStore.EnqueueMerge(stKey, aRealtime,
				strError), strError.c_str()) ||
			!Check(clStore.Flush(5000U, strError),
				strError.c_str()))
		{
			return false;
		}
		// 第二步：篡改文件正文必须触发隔离，正式路径消失后允许 MT/Derive 数据重新创建。
		const std::filesystem::path clFile = clDirectory /
			"bars" / "mt5" / "no1" / "1" / "EURUSD.bars";
		const std::filesystem::path clCoverageFile =
			std::filesystem::path(clFile.u8string() + ".coverage");
		{
			std::fstream clCorrupt(clFile, std::ios::binary |
				std::ios::in | std::ios::out);
			if (!Check(static_cast<bool>(clCorrupt),
				"Query bar history file is missing"))
			{
				return false;
			}
			clCorrupt.seekg(-1, std::ios::end);
			char chValue = 0;
			clCorrupt.read(&chValue, 1);
			chValue ^= static_cast<char>(0x5A);
			clCorrupt.seekp(-1, std::ios::end);
			clCorrupt.write(&chValue, 1);
			clCorrupt.flush();
		}
		aRead.clear();
		if (!Check(clStore.Read(stKey, aRead, enRead,
				strError), strError.c_str()) ||
			!Check(enRead == EN_MT_QUERY_BAR_HISTORY_CORRUPT &&
				aRead.empty() && !std::filesystem::exists(clFile) &&
				!std::filesystem::exists(clCoverageFile),
				"corrupt Query history was not quarantined") ||
			!Check(clStore.EnqueueMerge(stKey, aRealtime,
				aEmptyCoverage,
				strError), strError.c_str()) ||
			!Check(clStore.Flush(5000U, strError),
				strError.c_str()))
		{
			return false;
		}
		clStore.Stop();

		// 第三步：新 Store 实例应读回恢复文件，证明停机排空和重启加载闭环。
		if (!Check(clStore.Initialize(clDirectory.u8string(),
				strError), strError.c_str()) ||
			!Check(clStore.Read(stKey, aRead, enRead,
				strError), strError.c_str()) ||
			!Check(enRead == EN_MT_QUERY_BAR_HISTORY_READY &&
				aRead.size() == 1U && aRead[0].dClose == 1.2345,
				"Query history restart recovery mismatch") ||
			!Check(clStore.ReadCoverage(stKey, aCoverageRead,
				enCoverageRead, strError), strError.c_str()) ||
			!Check(enCoverageRead == EN_MT_QUERY_BAR_HISTORY_READY &&
				aCoverageRead.size() == 1U,
				"Query coverage restart recovery mismatch"))
		{
			return false;
		}
		// 第四步：通过唯一的新模式链执行 1157，最终结果合并历史文件和 Derive M1，且不泄露内部成交量字段。
		ST_MT_QUERY_SERVICE_CONFIG stConfig;
		stConfig.strInstanceId = "Query-Bars-Test";
		stConfig.strCachePath = clDirectory.u8string();
		stConfig.bAllowMockAdapter = true;
		stConfig.bAllowDegradedStart = false;
		stConfig.szBootstrapBufferCapacity = 128U;
		stConfig.szMinuteRetention = 120U;
		ST_MT_QUERY_SOURCE_CONFIG stSource;
		stSource.bEnable = true;
		stSource.bMock = true;
		stSource.usVersion = 5;
		stSource.iNo = 1;
		stSource.strName = "MT5-Mock";
		AddQueryTestConnections(stSource);
		stConfig.aSource.push_back(stSource);
		CMtQueryQuoteCache clCache;
		CMtQueryNodeManager clNodeManager;
		CMtQueryDispatcher clDispatcher;
		if (!Check(clCache.Configure(stConfig, strError),
				strError.c_str()) ||
			!Check(clNodeManager.Configure(stConfig, strError),
				strError.c_str()) ||
			!Check(clNodeManager.Start(strError), strError.c_str()))
		{
			clStore.Stop();
			return false;
		}
		const std::int64_t llNowMs =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count();
		ST_MT_TIME_STATE stTime;
		stTime.usPlatformVersion = 5;
		stTime.iSourceNo = 1;
		stTime.strTimeZoneId = "China Standard Time";
		stTime.iStandardOffsetSeconds = 28800;
		stTime.iCurrentOffsetSeconds = 28800;
		stTime.enSyncState = EN_MT_TIME_SYNC_READY;
		stTime.enChangeReason = EN_MT_TIME_CHANGE_SDK_UPDATE;
		stTime.ullAuthorityEpoch = 99U;
		stTime.ullGeneration = 1U;
		stTime.llEffectiveUtcMs = llNowMs - 1000;
		stTime.llSampledUtcMs = llNowMs;
		stTime.llValidUntilUtcMs = llNowMs + 60000;
		if (!Check(clNodeManager.UpdateTimeState(stTime,
				strError), strError.c_str()) ||
			!Check(clCache.UpdateTimeState(stTime, strError),
				strError.c_str()))
		{
			clNodeManager.Stop();
			clStore.Stop();
			return false;
		}
		ST_DERIVE_M1_SNAPSHOT_RESPONSE stSnapshot;
		stSnapshot.enState = EN_DERIVE_SERVICE_STATE_READY;
		stSnapshot.iCode = 0;
		stSnapshot.strMessage = "OK";
		ST_DERIVE_M1_BAR stM1;
		stM1.usPlatformVersion = 5;
		stM1.iSourceNo = 1;
		stM1.ullSourceEpoch = 88U;
		stM1.strSymbol = "EURUSD";
		stM1.llMinute = stRealtime.llDateTime;
		stM1.dOpen = 1.2300;
		stM1.dHigh = 1.2400;
		stM1.dLow = 1.2200;
		stM1.dClose = 1.2399;
		stM1.ullTickVolume = 88U;
		stM1.ullRealVolume = 66U;
		stM1.ullFirstSequence = 901U;
		stM1.ullLastSequence = 999U;
		stSnapshot.aBar.push_back(stM1);
		if (!Check(clCache.InstallM1Snapshot(5, 1,
				stSnapshot, strError), strError.c_str()) ||
			!Check(clDispatcher.Initialize(&stConfig, &clCache,
				&clNodeManager, strError, nullptr, "1.0.0.1",
				&clStore), strError.c_str()))
		{
			clNodeManager.Stop();
			clStore.Stop();
			return false;
		}
		ST_PLUGIN_BINARY_VALUE stRequest;
		stRequest.enType = EN_PLUGIN_BINARY_VALUE_OBJECT;
		const auto fnAddInt = [&stRequest](const char* p_szName,
			std::int64_t p_llValue)
		{
			ST_PLUGIN_BINARY_FIELD stField;
			stField.strName = p_szName;
			stField.refValue = CreatePluginBinaryValue(
				EN_PLUGIN_BINARY_VALUE_INT64);
			stField.refValue->llIntValue = p_llValue;
			stRequest.aObjectField.push_back(stField);
		};
		const auto fnAddString = [&stRequest](const char* p_szName,
			const char* p_szValue)
		{
			ST_PLUGIN_BINARY_FIELD stField;
			stField.strName = p_szName;
			stField.refValue = CreatePluginBinaryValue(
				EN_PLUGIN_BINARY_VALUE_STRING);
			stField.refValue->strStringValue = p_szValue;
			stRequest.aObjectField.push_back(stField);
		};
		fnAddInt("Version", 5);
		fnAddInt("No", 1);
		fnAddString("SymbolIds", "EURUSD");
		fnAddInt("Interval", 1);
		fnAddInt("From", stRealtime.llDateTime);
		fnAddInt("To", stRealtime.llDateTime + 59);
		std::int32_t iCode = EN_TERMINAL_ERROR_INTERNAL_ERROR;
		std::string strMessage;
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refData;
		const bool bDispatched = clDispatcher.Dispatch(
			EN_PLUGIN_FUNC_QUERY_BARS, 0, stRequest,
			iCode, strMessage, refData, strError);
		if (!Check(bDispatched, strError.c_str()) ||
			!Check(clStore.Flush(5000U, strError), strError.c_str()))
		{
			clNodeManager.Stop();
			clStore.Stop();
			return false;
		}
		// 第五步：部分确认和 MT Common error 都映射为 20008，多品种结果不得保留先前成功项。
		const auto fnBuildRangeRequest = [](const char* p_szSymbols,
			std::int64_t p_llFrom, std::int64_t p_llTo)
		{
			ST_PLUGIN_BINARY_VALUE stValue;
			stValue.enType = EN_PLUGIN_BINARY_VALUE_OBJECT;
			const auto fnInt = [&stValue](const char* p_szName,
				std::int64_t p_llValue)
			{
				ST_PLUGIN_BINARY_FIELD stField;
				stField.strName = p_szName;
				stField.refValue = CreatePluginBinaryValue(
					EN_PLUGIN_BINARY_VALUE_INT64);
				stField.refValue->llIntValue = p_llValue;
				stValue.aObjectField.push_back(stField);
			};
			const auto fnString = [&stValue](const char* p_szName,
				const char* p_szValue)
			{
				ST_PLUGIN_BINARY_FIELD stField;
				stField.strName = p_szName;
				stField.refValue = CreatePluginBinaryValue(
					EN_PLUGIN_BINARY_VALUE_STRING);
				stField.refValue->strStringValue = p_szValue;
				stValue.aObjectField.push_back(stField);
			};
			fnInt("Version", 5);
			fnInt("No", 1);
			fnString("SymbolIds", p_szSymbols);
			fnInt("Interval", 1);
			fnInt("From", p_llFrom);
			fnInt("To", p_llTo);
			return stValue;
		};
		ST_PLUGIN_BINARY_VALUE stPartialRequest = fnBuildRangeRequest(
			"EURUSD,__MOCK_PARTIAL__", stRealtime.llDateTime,
			stRealtime.llDateTime + 59);
		std::int32_t iPartialCode = EN_TERMINAL_ERROR_INTERNAL_ERROR;
		std::string strPartialMessage;
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refPartialData;
		const bool bPartialDispatched = clDispatcher.Dispatch(
			EN_PLUGIN_FUNC_QUERY_BARS, 0, stPartialRequest,
			iPartialCode, strPartialMessage, refPartialData, strError);
		ST_PLUGIN_BINARY_VALUE stCommonRequest = fnBuildRangeRequest(
			"__MOCK_COMMON_ERROR__", stRealtime.llDateTime,
			stRealtime.llDateTime + 59);
		std::int32_t iCommonCode = EN_TERMINAL_ERROR_INTERNAL_ERROR;
		std::string strCommonMessage;
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refCommonData;
		const bool bCommonDispatched = clDispatcher.Dispatch(
			EN_PLUGIN_FUNC_QUERY_BARS, 0, stCommonRequest,
			iCommonCode, strCommonMessage, refCommonData, strError);
		if (!Check(bPartialDispatched, strError.c_str()) ||
			!Check(iPartialCode == EN_TERMINAL_ERROR_MT_SYSTEM_NOT_READY &&
				refPartialData && refPartialData->aArrayValue.empty(),
				"Query partial bars must return 20008 without multi-symbol partial data") ||
			!Check(bCommonDispatched, strError.c_str()) ||
			!Check(iCommonCode == EN_TERMINAL_ERROR_MT_SYSTEM_NOT_READY &&
				refCommonData && refCommonData->aArrayValue.empty(),
				"Query MT Common error must map to 20008"))
		{
			clNodeManager.Stop();
			clStore.Stop();
			return false;
		}
		// 第六步：停止 MT 历史池后重复同一区间，持久化 coverage 应让请求完全由缓存完成。
		clNodeManager.Stop();
		std::int32_t iCachedCode = EN_TERMINAL_ERROR_INTERNAL_ERROR;
		std::string strCachedMessage;
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refCachedData;
		const bool bCachedDispatched = clDispatcher.Dispatch(
			EN_PLUGIN_FUNC_QUERY_BARS, 0, stRequest,
			iCachedCode, strCachedMessage, refCachedData, strError);
		CMtQueryNodeManager clReloadedNodeManager;
		const bool bTimeJournalReloaded =
			clReloadedNodeManager.Configure(stConfig, strError);
		clReloadedNodeManager.Stop();
		clStore.Stop();
		if (!Check(bCachedDispatched, strError.c_str()) ||
			!Check(iCode == EN_TERMINAL_ERROR_OK && refData &&
				refData->aArrayValue.size() == 1U,
				"Query bars dispatcher response missing") ||
			!Check(iCachedCode == EN_TERMINAL_ERROR_OK &&
				refCachedData && refCachedData->aArrayValue.size() == 1U,
				"Query bars cache-first request unexpectedly required MT") ||
			!Check(bTimeJournalReloaded,
				"Query 1244 transition journal restart load failed"))
		{
			return false;
		}
		const ST_PLUGIN_BINARY_VALUE* pBars =
			FindPluginBinaryField(*refData->aArrayValue[0], "Bars");
		const ST_PLUGIN_BINARY_VALUE* pItem =
			pBars && !pBars->aArrayValue.empty() ?
				pBars->aArrayValue[0].get() : nullptr;
		const ST_PLUGIN_BINARY_VALUE* pClose = pItem ?
			FindPluginBinaryField(*pItem, "Close") : nullptr;
		return Check(pItem != nullptr && pClose != nullptr &&
			pClose->enType == EN_PLUGIN_BINARY_VALUE_DOUBLE &&
			pClose->dDoubleValue == 1.2399 &&
			FindPluginBinaryField(*pItem, "Value") != nullptr &&
			FindPluginBinaryField(*pItem, "TickVolume") == nullptr &&
			FindPluginBinaryField(*pItem, "RealVolume") == nullptr,
			"Query bars V2 fields or Derive overlap priority mismatch");
	}

	// 校验非行情 ClientData 仍要求 OBJECT，避免 Quote BYTES 规则影响后续订单和配置事件。
	bool TestObjectClientDataEvent()
	{
		ST_CLIENT_DATA_BINARY_EVENT stEvent;
		stEvent.ullTopic =
			EN_CLIENT_DATA_BINARY_TOPIC_SYMBOL;
		stEvent.iSourceVersion = 5;
		stEvent.iSourceNo = 1;
		stEvent.strSymbol = "EURUSD";
		stEvent.ullSequence = 100U;
		stEvent.llTimestampMs = 1710000000500;
		stEvent.refData = std::shared_ptr<ST_PLUGIN_BINARY_VALUE>(
			new ST_PLUGIN_BINARY_VALUE(BuildProtocolObject()));
		std::vector<unsigned char> aPayload;
		std::string strError;
		if (!Check(EncodeClientDataBinaryEvent(stEvent,
			aPayload, strError), strError.c_str()))
		{
			return false;
		}
		ST_CLIENT_DATA_BINARY_EVENT stDecoded;
		return Check(DecodeClientDataBinaryEvent(aPayload.data(),
			aPayload.size(), stDecoded, strError),
			strError.c_str()) &&
			Check(stDecoded.strSymbol == "EURUSD" &&
				stDecoded.ullSequence == 100U &&
				stDecoded.refData != nullptr &&
				stDecoded.refData->enType ==
					EN_PLUGIN_BINARY_VALUE_OBJECT,
			"object client data event mismatch");
	}

	// 校验可靠 Trade 信封只在在线扇出时转换为 ClientData，过滤字段和来源水位必须保持一致。
	bool TestReliableTradeClientDataEvent()
	{
		const ST_PLUGIN_BINARY_VALUE stTradeData =
			BuildProtocolObject();
		std::vector<unsigned char> aTradePayload;
		std::string strError;
		if (!Check(EncodeTradeBinaryRequest(stTradeData,
			aTradePayload, strError), strError.c_str()))
		{
			return false;
		}

		ST_PLUGIN_BINARY_VALUE stEnvelope;
		stEnvelope.enType = EN_PLUGIN_BINARY_VALUE_OBJECT;
		const auto fnAddInt64 = [&stEnvelope](
			const char* p_szName, std::int64_t p_llValue)
			{
				ST_PLUGIN_BINARY_FIELD stField;
				stField.strName = p_szName;
				stField.refValue = CreatePluginBinaryValue(
					EN_PLUGIN_BINARY_VALUE_INT64);
				stField.refValue->llIntValue = p_llValue;
				stEnvelope.aObjectField.push_back(stField);
			};
		const auto fnAddUInt64 = [&stEnvelope](
			const char* p_szName, std::uint64_t p_ullValue)
			{
				ST_PLUGIN_BINARY_FIELD stField;
				stField.strName = p_szName;
				stField.refValue = CreatePluginBinaryValue(
					EN_PLUGIN_BINARY_VALUE_UINT64);
				stField.refValue->ullUIntValue = p_ullValue;
				stEnvelope.aObjectField.push_back(stField);
			};
		fnAddInt64("Version", 5);
		fnAddInt64("No", 2);
		fnAddInt64("NotifyId",
			EN_PLUGIN_NOTIFY_ORDER_CHANGED);
		fnAddUInt64("SourceSequence", 77U);
		fnAddInt64("CreatedTimeMs", 1710000000600LL);
		ST_PLUGIN_BINARY_FIELD stPayload;
		stPayload.strName = "Payload";
		stPayload.refValue = CreatePluginBinaryValue(
			EN_PLUGIN_BINARY_VALUE_BYTES);
		stPayload.refValue->aByteValue = aTradePayload;
		stEnvelope.aObjectField.push_back(stPayload);

		std::vector<unsigned char> aClientData;
		if (!Check(EncodeReliableTradeEventAsClientData(
				stEnvelope, aClientData, strError),
			strError.c_str()))
		{
			return false;
		}
		ST_CLIENT_DATA_BINARY_EVENT stDecoded;
		if (!Check(DecodeClientDataBinaryEvent(
				aClientData.data(), aClientData.size(),
				stDecoded, strError), strError.c_str()) ||
			!Check(stDecoded.ullTopic ==
					EN_CLIENT_DATA_BINARY_TOPIC_ORDER &&
				stDecoded.iSourceVersion == 5 &&
				stDecoded.iSourceNo == 2 &&
				stDecoded.llLogin == 10001 &&
				stDecoded.strSymbol == "EURUSD" &&
				stDecoded.ullSequence == 77U &&
				stDecoded.llTimestampMs == 1710000000600LL,
				"reliable order event metadata was not preserved"))
		{
			return false;
		}

		// 1244 必须继续传完整 Reliable 信封，防止公共时间同步误收 ClientData 文档。
		ST_PLUGIN_BINARY_VALUE* pNotifyId =
			const_cast<ST_PLUGIN_BINARY_VALUE*>(
				FindPluginBinaryField(stEnvelope,
					"NotifyId"));
		if (!Check(pNotifyId != nullptr,
			"reliable event NotifyId field is missing"))
		{
			return false;
		}
		pNotifyId->llIntValue =
			EN_PLUGIN_NOTIFY_SERVER_TIME_CHANGED;
		return Check(!EncodeReliableTradeEventAsClientData(
			stEnvelope, aClientData, strError),
			"1244 time event must not be converted to ClientData");
	}

	// 验证可靠归档将 MT4/MT5 M1 确定性提交到全部周期，跨日 W1/MN1 不覆盖且重投不重复累计。
	bool TestQueryM1ArchiveConsumer()
	{
		const std::filesystem::path clRoot =
			std::filesystem::temp_directory_path() /
			"TradingTerminalQueryM1Archive";
		CTestDirectoryGuard clGuard(clRoot);
		ST_MT_QUERY_SERVICE_CONFIG stConfig;
		stConfig.strInstanceId = "Query-Archive-Test";
		stConfig.strCachePath = clRoot.u8string();
		const auto fnAddSource = [&stConfig](std::uint16_t p_usVersion,
			std::int32_t p_iNo)
		{
			ST_MT_QUERY_SOURCE_CONFIG stSource;
			stSource.bEnable = true;
			stSource.bMock = true;
			stSource.usVersion = p_usVersion;
			stSource.iNo = p_iNo;
			stSource.strName = p_usVersion == 4U ? "MT4-Archive" :
				"MT5-Archive";
			AddQueryTestConnections(stSource);
			stConfig.aSource.push_back(stSource);
		};
		fnAddSource(5U, 1);
		fnAddSource(4U, 2);
		CMtQueryNodeManager clNodeManager;
		CMtQueryBarHistoryStore clStore;
		std::string strError;
		if (!Check(clNodeManager.Configure(stConfig, strError),
				strError.c_str()) ||
			!Check(clNodeManager.Start(strError), strError.c_str()) ||
			!Check(clStore.Initialize(clRoot.u8string(), strError),
				strError.c_str()))
		{
			clNodeManager.Stop();
			return false;
		}
		const std::int64_t llNowMs =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count();
		const auto fnInstallTime = [&clNodeManager, &strError, llNowMs](
			std::uint16_t p_usVersion, std::int32_t p_iNo,
			std::uint64_t p_ullGeneration, std::int64_t p_llValidUntil)
		{
			ST_MT_TIME_STATE stTime;
			stTime.usPlatformVersion = p_usVersion;
			stTime.iSourceNo = p_iNo;
			stTime.strTimeZoneId = "UTC";
			stTime.iStandardOffsetSeconds = 0;
			stTime.iCurrentOffsetSeconds = 0;
			stTime.enSyncState = EN_MT_TIME_SYNC_READY;
			stTime.ullAuthorityEpoch = 8200U + p_usVersion;
			stTime.ullGeneration = p_ullGeneration;
			stTime.llEffectiveUtcMs = llNowMs - 60000LL;
			stTime.llSampledUtcMs = llNowMs - 1000LL;
			stTime.llValidUntilUtcMs = p_llValidUntil;
			return clNodeManager.UpdateTimeState(stTime, strError);
		};
		if (!Check(fnInstallTime(5U, 1, 1U,
				llNowMs + 600000LL), strError.c_str()) ||
			!Check(fnInstallTime(4U, 2, 1U,
				llNowMs + 600000LL), strError.c_str()))
		{
			clStore.Stop();
			clNodeManager.Stop();
			return false;
		}
		ST_MT_QUERY_M1_ARCHIVE_CONFIG stArchiveConfig;
		stArchiveConfig.bEnable = false;
		CMtQueryM1ArchiveConsumer clConsumer;
		if (!Check(clConsumer.Start(stArchiveConfig,
				&clNodeManager, &clStore,
				PFN_MT_QUERY_M1_ARCHIVE_FETCH(),
				PFN_MT_QUERY_M1_ARCHIVE_ACK(), strError),
			strError.c_str()))
		{
			clStore.Stop();
			clNodeManager.Stop();
			return false;
		}
		const auto fnUtc = [](int p_iYear, int p_iMonth,
			int p_iDay, int p_iHour, int p_iMinute)
		{
			tm stTime = {};
			stTime.tm_year = p_iYear - 1900;
			stTime.tm_mon = p_iMonth - 1;
			stTime.tm_mday = p_iDay;
			stTime.tm_hour = p_iHour;
			stTime.tm_min = p_iMinute;
			return static_cast<std::int64_t>(_mkgmtime64(&stTime));
		};
		const std::int64_t llDay1 = fnUtc(2024, 1, 1, 0, 0);
		const std::int64_t llDay2 = fnUtc(2024, 1, 2, 0, 0);
		const std::int64_t llDay3 = fnUtc(2024, 1, 3, 0, 0);
		const auto fnBatch = [](std::uint16_t p_usVersion,
			std::int32_t p_iNo, const char* p_szId,
			std::uint32_t p_uiDate, std::int64_t p_llDay,
			double p_dPrice, std::uint64_t p_ullVolume,
			bool p_bEmpty)
		{
			ST_DERIVE_M1_ARCHIVE_BATCH stBatch;
			stBatch.strArchiveId = p_szId;
			stBatch.usPlatformVersion = p_usVersion;
			stBatch.iSourceNo = p_iNo;
			stBatch.strSymbol = "EURUSD";
			stBatch.uiServerDate = p_uiDate;
			stBatch.llFromMinute = p_llDay;
			stBatch.llToMinute = p_llDay + 86340LL;
			stBatch.bConfirmedEmpty = p_bEmpty;
			stBatch.ullTimeAuthorityEpoch = 8200U + p_usVersion;
			stBatch.ullTimeGeneration = 1U;
			stBatch.strOwnerInstanceId = "derive-owner-1";
			stBatch.ullLeaseGeneration = 9U;
			if (!p_bEmpty)
			{
				ST_DERIVE_M1_BAR stBar;
				stBar.usPlatformVersion = p_usVersion;
				stBar.iSourceNo = p_iNo;
				stBar.strSymbol = "EURUSD";
				stBar.llMinute = p_llDay + 60LL;
				stBar.dOpen = p_dPrice;
				stBar.dHigh = p_dPrice + 0.2;
				stBar.dLow = p_dPrice - 0.1;
				stBar.dClose = p_dPrice + 0.1;
				stBar.ullTickVolume = p_ullVolume;
				stBar.ullRealVolume = p_ullVolume * 2U;
				// 纯 MT 回填没有 Quote Epoch/Sequence，0/0 必须仍可可靠归档。
				stBatch.aBar.push_back(stBar);
			}
			return stBatch;
		};
		ST_QUERY_M1_BACKFILL_REQUEST stPartialRequest;
		stPartialRequest.usPlatformVersion = 5U;
		stPartialRequest.iSourceNo = 1;
		stPartialRequest.uiServerDate = 20240101U;
		stPartialRequest.strSymbol = "EURUSD";
		stPartialRequest.llFromMinute = 0;
		stPartialRequest.llToMinute = llDay1 + 3600LL;
		stPartialRequest.ullTimeAuthorityEpoch = 8205U;
		stPartialRequest.ullTimeGeneration = 1U;
		stPartialRequest.strOwnerInstanceId = "derive-owner-1";
		stPartialRequest.ullLeaseGeneration = 9U;
		std::vector<unsigned char> aProtocol;
		ST_QUERY_M1_BACKFILL_REQUEST stDecodedRequest;
		if (!Check(EncodeQueryM1BackfillRequest(stPartialRequest,
				aProtocol, strError), strError.c_str()) ||
			!Check(DecodeQueryM1BackfillRequest(aProtocol.data(),
				aProtocol.size(), stDecodedRequest, strError),
				strError.c_str()) ||
			!Check(stDecodedRequest.llFromMinute == 0 &&
				stDecodedRequest.llToMinute == llDay1 + 3600LL,
				"1168 partial Broker-day sentinel changed"))
		{
			clConsumer.Stop();
			clStore.Stop();
			clNodeManager.Stop();
			return false;
		}
		const ST_DERIVE_M1_ARCHIVE_BATCH stDay1 = fnBatch(
			5U, 1, "mt5-day1", 20240101U, llDay1, 1.0, 10U, false);
		const ST_DERIVE_M1_ARCHIVE_BATCH stDay2 = fnBatch(
			5U, 1, "mt5-day2", 20240102U, llDay2, 2.0, 20U, false);
		const ST_DERIVE_M1_ARCHIVE_BATCH stDay3 = fnBatch(
			5U, 1, "mt5-day3-empty", 20240103U, llDay3, 0.0, 0U, true);
		if (!Check(clConsumer.ProcessBatch(stDay1, strError),
				strError.c_str()) ||
			!Check(clConsumer.ProcessBatch(stDay2, strError),
				strError.c_str()) ||
			!Check(clConsumer.ProcessBatch(stDay2, strError),
				"duplicate archive batch must be idempotent") ||
			!Check(clConsumer.ProcessBatch(stDay3, strError),
				strError.c_str()))
		{
			clConsumer.Stop();
			clStore.Stop();
			clNodeManager.Stop();
			return false;
		}
		const std::int64_t aMt5Period[] =
			{1, 2, 3, 4, 5, 6, 10, 12, 15, 20, 30, 60,
			 120, 180, 240, 360, 480, 720, 1440, 10080, 43200};
		for (const std::int64_t llPeriod : aMt5Period)
		{
			ST_MT_QUERY_BAR_HISTORY_KEY stKey;
			stKey.usVersion = 5U;
			stKey.iNo = 1;
			stKey.strSymbol = "EURUSD";
			stKey.llIntervalMinutes = llPeriod;
			std::vector<ST_MT_QUERY_BAR> aBars;
			EN_MT_QUERY_BAR_HISTORY_READ_STATE enState =
				EN_MT_QUERY_BAR_HISTORY_MISSING;
			if (!Check(clStore.Read(stKey, aBars, enState,
					strError), strError.c_str()) ||
				!Check(enState == EN_MT_QUERY_BAR_HISTORY_READY &&
					!aBars.empty(),
					"MT5 archive period was not persisted"))
			{
				clConsumer.Stop();
				clStore.Stop();
				clNodeManager.Stop();
				return false;
			}
		}
		ST_MT_QUERY_BAR_HISTORY_KEY stWeekKey;
		stWeekKey.usVersion = 5U;
		stWeekKey.iNo = 1;
		stWeekKey.strSymbol = "EURUSD";
		stWeekKey.llIntervalMinutes = 10080;
		std::vector<ST_MT_QUERY_BAR> aWeek;
		EN_MT_QUERY_BAR_HISTORY_READ_STATE enWeekState =
			EN_MT_QUERY_BAR_HISTORY_MISSING;
		if (!Check(clStore.Read(stWeekKey, aWeek, enWeekState,
				strError), strError.c_str()) ||
			!Check(aWeek.size() == 1U &&
				aWeek[0].dOpen == 1.0 &&
				aWeek[0].dClose == 2.1 &&
				aWeek[0].ullTickVolume == 30U &&
				aWeek[0].ullRealVolume == 60U,
				"W1 cross-day aggregation was overwritten or double-counted"))
		{
			clConsumer.Stop();
			clStore.Stop();
			clNodeManager.Stop();
			return false;
		}
		ST_MT_QUERY_BAR_HISTORY_KEY stM1Key = stWeekKey;
		stM1Key.llIntervalMinutes = 1;
		std::vector<ST_MT_QUERY_BAR_COVERAGE_RANGE> aCoverage;
		EN_MT_QUERY_BAR_HISTORY_READ_STATE enCoverage =
			EN_MT_QUERY_BAR_HISTORY_MISSING;
		if (!Check(clStore.ReadCoverage(stM1Key, aCoverage,
				enCoverage, strError), strError.c_str()) ||
			!Check(aCoverage.size() == 1U &&
				aCoverage[0].llFrom == llDay1 &&
				aCoverage[0].llTo == llDay3 + 86399LL,
				"confirmed empty archive day did not advance coverage"))
		{
			clConsumer.Stop();
			clStore.Stop();
			clNodeManager.Stop();
			return false;
		}
		const ST_DERIVE_M1_ARCHIVE_BATCH stMt4 = fnBatch(
			4U, 2, "mt4-day1", 20240101U, llDay1, 1.5, 15U, false);
		if (!Check(clConsumer.ProcessBatch(stMt4, strError),
			strError.c_str()))
		{
			clConsumer.Stop();
			clStore.Stop();
			clNodeManager.Stop();
			return false;
		}
		const std::int64_t aMt4Period[] =
			{1, 5, 15, 30, 60, 240, 1440, 10080, 43200};
		for (const std::int64_t llPeriod : aMt4Period)
		{
			ST_MT_QUERY_BAR_HISTORY_KEY stKey;
			stKey.usVersion = 4U;
			stKey.iNo = 2;
			stKey.strSymbol = "EURUSD";
			stKey.llIntervalMinutes = llPeriod;
			std::vector<ST_MT_QUERY_BAR> aBars;
			EN_MT_QUERY_BAR_HISTORY_READ_STATE enState =
				EN_MT_QUERY_BAR_HISTORY_MISSING;
			if (!Check(clStore.Read(stKey, aBars, enState,
					strError), strError.c_str()) ||
				!Check(enState == EN_MT_QUERY_BAR_HISTORY_READY &&
					!aBars.empty(),
					"MT4 archive period was not persisted"))
			{
				clConsumer.Stop();
				clStore.Stop();
				clNodeManager.Stop();
				return false;
			}
		}
		clConsumer.Stop();
		clStore.Stop();
		clNodeManager.Stop();
		return true;
	}

	// 校验 1181 退役登记、1182-1186 活动范围以及 Derive 固定小端契约。
	bool TestDeriveProtocol()
	{
		if (!Check(std::string(GetPluginFuncName(
				EN_PLUGIN_FUNC_MT_DERIVE_SERVICE_DEMO_RETIRED)) ==
					"MT_DERIVE_SERVICE_DEMO_RETIRED",
				"1181 must remain retired") ||
			!Check(!IsActiveMtDeriveFuncId(
					EN_PLUGIN_FUNC_MT_DERIVE_SERVICE_DEMO_RETIRED) &&
				IsActiveMtDeriveFuncId(
					EN_PLUGIN_FUNC_DERIVE_TICK_APPEND) &&
				IsActiveMtDeriveFuncId(
					EN_PLUGIN_FUNC_DERIVE_STATUS),
				"Derive active protocol range is invalid"))
		{
			return false;
		}

		ST_DERIVE_TICK_BATCH_REQUEST stBatch;
		stBatch.usPlatformVersion = 5;
		stBatch.iSourceNo = 1;
		stBatch.ullSourceEpoch = 9001U;
		stBatch.ullFirstSequence = 1U;
		const double aPrice[] = {100.0, 110.0, 90.0, 101.0};
		for (std::size_t szIndex = 0;
			szIndex < _countof(aPrice); ++szIndex)
		{
			ST_QUOTE_BINARY_TICK stTick;
			SetTestQuoteDefaults(stTick);
			stTick.usPlatformVersion = 5;
			stTick.iSourceNo = 1;
			stTick.ullSourceEpoch = stBatch.ullSourceEpoch;
			stTick.strSymbol = "TEST";
			stTick.dBid = aPrice[szIndex];
			stTick.dAsk = aPrice[szIndex] + 0.1;
			stTick.llServerTimeMsc =
				1710000000000 + szIndex;
			stTick.llServerTime = 1710000000;
			stTick.llIngressTimeMs =
				1710000000100 + szIndex;
			stTick.ullIngressSequence =
				1U + szIndex;
			stBatch.aTick.push_back(stTick);
		}
		std::vector<unsigned char> aPayload;
		std::string strError;
		if (!Check(EncodeDeriveTickBatchRequest(stBatch,
				aPayload, strError), strError.c_str()))
		{
			return false;
		}
		ST_DERIVE_TICK_BATCH_REQUEST stDecodedBatch;
		if (!Check(DecodeDeriveTickBatchRequest(
				aPayload.data(), aPayload.size(),
				stDecodedBatch, strError),
				strError.c_str()) ||
			!Check(stDecodedBatch.aTick.size() == 4U &&
				stDecodedBatch.aTick[3].dBid == 101.0 &&
				stDecodedBatch.ullSourceEpoch == 9001U,
				"Derive Tick batch round trip mismatch"))
		{
			return false;
		}
		aPayload.pop_back();
		if (!Check(!DecodeDeriveTickBatchRequest(
				aPayload.data(), aPayload.size(),
				stDecodedBatch, strError),
				"truncated Derive Tick batch must be rejected"))
		{
			return false;
		}

		ST_DERIVE_PROFIT_DEMAND_REQUEST stDemand;
		stDemand.usPlatformVersion = 5;
		stDemand.iSourceNo = 1;
		stDemand.strGatewayInstance =
			"MtGatewayService-01";
		stDemand.ullGatewayEpoch = 5001U;
		stDemand.ullRevision = 8U;
		stDemand.uiLeaseMs = 60000U;
		stDemand.aLogin.push_back(10001);
		stDemand.aLogin.push_back(10002);
		if (!Check(EncodeDeriveProfitDemandRequest(
				stDemand, aPayload, strError),
				strError.c_str()))
		{
			return false;
		}
		ST_DERIVE_PROFIT_DEMAND_REQUEST stDecodedDemand;
		if (!Check(DecodeDeriveProfitDemandRequest(
				aPayload.data(), aPayload.size(),
				stDecodedDemand, strError),
				strError.c_str()) ||
			!Check(stDecodedDemand.ullRevision == 8U &&
				stDecodedDemand.aLogin.size() == 2U,
				"Derive demand round trip mismatch"))
		{
			return false;
		}
		stDemand.aLogin.push_back(10001);
		if (!Check(!EncodeDeriveProfitDemandRequest(
				stDemand, aPayload, strError),
				"duplicate Derive demand Login must be rejected"))
		{
			return false;
		}

		ST_DERIVE_M1_SNAPSHOT_REQUEST stM1Request;
		stM1Request.usPlatformVersion = 5U;
		stM1Request.iSourceNo = 1;
		stM1Request.strSymbol = "TEST";
		stM1Request.uiMaxBars = 100U;
		stM1Request.bOpenPriceOnly = true;
		if (!Check(EncodeDeriveM1SnapshotRequest(
				stM1Request, aPayload, strError), strError.c_str()))
		{
			return false;
		}
		ST_DERIVE_M1_SNAPSHOT_REQUEST stDecodedM1Request;
		if (!Check(DecodeDeriveM1SnapshotRequest(
				aPayload.data(), aPayload.size(),
				stDecodedM1Request, strError), strError.c_str()) ||
			!Check(stDecodedM1Request.bOpenPriceOnly &&
				stDecodedM1Request.strSymbol == "TEST" &&
				stDecodedM1Request.uiMaxBars == 100U,
				"Derive OpenPrice-only request round trip mismatch"))
		{
			return false;
		}

		ST_DERIVE_M1_SNAPSHOT_RESPONSE stM1;
		stM1.enState = EN_DERIVE_SERVICE_STATE_READY;
		stM1.ullEventSequence = 77U;
		ST_DERIVE_M1_BAR stBar;
		stBar.usPlatformVersion = 5;
		stBar.iSourceNo = 1;
		stBar.ullSourceEpoch = 9001U;
		stBar.strSymbol = "TEST";
		stBar.llMinute = 1710000000;
		stBar.dOpen = 100.0;
		stBar.dHigh = 110.0;
		stBar.dLow = 90.0;
		stBar.dClose = 101.0;
		stBar.ullTickVolume = 4U;
		stBar.ullRealVolume = 0U;
		stBar.ullFirstSequence = 1U;
		stBar.ullLastSequence = 4U;
		stM1.aBar.push_back(stBar);
		if (!Check(EncodeDeriveM1SnapshotResponse(
				stM1, aPayload, strError),
				strError.c_str()))
		{
			return false;
		}
		ST_DERIVE_M1_SNAPSHOT_RESPONSE stDecodedM1;
		if (!Check(DecodeDeriveM1SnapshotResponse(
				aPayload.data(), aPayload.size(),
				stDecodedM1, strError), strError.c_str()) ||
			!Check(stDecodedM1.aBar.size() == 1U &&
				stDecodedM1.ullEventSequence == 77U &&
				stDecodedM1.aBar[0].dOpen == 100.0 &&
				stDecodedM1.aBar[0].dHigh == 110.0 &&
				stDecodedM1.aBar[0].dLow == 90.0 &&
				stDecodedM1.aBar[0].dClose == 101.0 &&
				stDecodedM1.aBar[0].ullTickVolume == 4U,
				"Derive M1 round trip mismatch"))
		{
			return false;
		}

		ST_DERIVE_STATUS_RESPONSE stStatus;
		stStatus.enState = EN_DERIVE_SERVICE_STATE_READY;
		stStatus.ullArchiveSpoolBytes = 4096U;
		stStatus.ullArchiveOldestTaskAgeMs = 1200U;
		stStatus.ullArchivePublishedCount = 8U;
		stStatus.ullArchiveConsumerAckCount = 7U;
		stStatus.uiArchivePendingCount = 2U;
		stStatus.uiBackfillInProgressCount = 1U;
		ST_DERIVE_SOURCE_STATUS stSourceStatus;
		stSourceStatus.usPlatformVersion = 5U;
		stSourceStatus.iSourceNo = 1;
		stSourceStatus.enState = EN_DERIVE_SERVICE_STATE_READY;
		stSourceStatus.uiSymbolCount = 3U;
		stSourceStatus.llArchiveWatermark = 1710000000LL;
		stSourceStatus.uiArchivePendingCount = 1U;
		stSourceStatus.uiBackfillPendingCount = 2U;
		stStatus.aSource.push_back(stSourceStatus);
		if (!Check(EncodeDeriveStatusResponse(stStatus,
				aPayload, strError), strError.c_str()))
		{
			return false;
		}
		ST_DERIVE_STATUS_RESPONSE stDecodedStatus;
		return Check(DecodeDeriveStatusResponse(aPayload.data(),
				aPayload.size(), stDecodedStatus, strError),
			strError.c_str()) &&
			Check(stDecodedStatus.ullArchiveSpoolBytes == 4096U &&
				stDecodedStatus.ullArchiveConsumerAckCount == 7U &&
				stDecodedStatus.uiArchivePendingCount == 2U &&
				stDecodedStatus.aSource.size() == 1U &&
				stDecodedStatus.aSource[0].llArchiveWatermark ==
					1710000000LL &&
				stDecodedStatus.aSource[0].uiBackfillPendingCount == 2U,
				"Derive 1186 archive diagnostics round trip mismatch");
	}

	// 验证 .MIN 双日槽、历史槽保护、DST 双 occurrence、晚到 Tick 修复归档和损坏拒绝。
	bool TestDeriveMinuteWorkspaceArchive()
	{
		const std::filesystem::path clRoot =
			std::filesystem::temp_directory_path() /
			"TradingTerminalDeriveMinuteWorkspace";
		CTestDirectoryGuard clGuard(clRoot);
		ST_MT_DERIVE_SERVICE_CONFIG stConfig;
		stConfig.strInstanceId = "Derive-Minute-Test";
		stConfig.stTick.strCachePath = clRoot.u8string();
		stConfig.stTick.szMinuteRetention = 2880U;
		ST_MT_DERIVE_SOURCE_CONFIG stSource;
		stSource.bEnable = true;
		stSource.usVersion = 5U;
		stSource.iNo = 1;
		stConfig.aSource.push_back(stSource);
		CMtDeriveM1Engine clEngine;
		std::string strError;
		if (!Check(clEngine.Configure(stConfig, strError),
			strError.c_str()))
		{
			return false;
		}
		const std::int64_t llNowMs =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count();
		ST_MT_TIME_STATE stTimeState;
		stTimeState.usPlatformVersion = 5U;
		stTimeState.iSourceNo = 1;
		stTimeState.strTimeZoneId = "UTC";
		stTimeState.iStandardOffsetSeconds = 0;
		stTimeState.iCurrentOffsetSeconds = 0;
		stTimeState.enSyncState = EN_MT_TIME_SYNC_READY;
		stTimeState.enChangeReason = EN_MT_TIME_CHANGE_SDK_UPDATE;
		stTimeState.ullAuthorityEpoch = 8102U;
		stTimeState.ullGeneration = 3U;
		stTimeState.llEffectiveUtcMs = llNowMs - 1000LL;
		stTimeState.llSampledUtcMs = llNowMs;
		stTimeState.llValidUntilUtcMs = llNowMs + 86400000LL;
		if (!Check(clEngine.UpdateTimeState(stTimeState, strError),
				strError.c_str()))
		{
			return false;
		}
		const auto fnUpdateOffset = [&clEngine, &stTimeState, &strError](
			std::int32_t p_iOffsetSeconds, std::uint64_t p_ullGeneration)
		{
			stTimeState.iCurrentOffsetSeconds = p_iOffsetSeconds;
			stTimeState.ullGeneration = p_ullGeneration;
			return clEngine.UpdateTimeState(stTimeState, strError);
		};
		const auto fnUtc = [](int p_iYear, int p_iMonth,
			int p_iDay, int p_iHour, int p_iMinute)
		{
			tm stTime = {};
			stTime.tm_year = p_iYear - 1900;
			stTime.tm_mon = p_iMonth - 1;
			stTime.tm_mday = p_iDay;
			stTime.tm_hour = p_iHour;
			stTime.tm_min = p_iMinute;
			return static_cast<std::int64_t>(_mkgmtime64(&stTime));
		};
		const auto fnTick = [](std::uint64_t p_ullSequence,
			std::int64_t p_llUtc, std::int64_t p_llServer,
			const char* p_szSymbol, double p_dPrice)
		{
			ST_QUOTE_BINARY_TICK stTick;
			SetTestQuoteDefaults(stTick);
			stTick.usPlatformVersion = 5U;
			stTick.iSourceNo = 1;
			stTick.ullSourceEpoch = 9102U;
			stTick.strSymbol = p_szSymbol;
			stTick.dBid = p_dPrice;
			stTick.dAsk = p_dPrice + 0.1;
			stTick.dLast = p_dPrice;
			stTick.ullVolumeExt = 2U;
			stTick.llServerTime = p_llServer;
			stTick.llServerTimeMsc = p_llServer * 1000LL;
			stTick.llIngressTimeMs = p_llUtc * 1000LL + 10LL;
			stTick.ullIngressSequence = p_ullSequence;
			return stTick;
		};
		const std::int64_t llDay1 = fnUtc(2024, 1, 1, 0, 0);
		const std::int64_t llDay2 = fnUtc(2024, 1, 2, 0, 0);
		const std::int64_t llDay3 = fnUtc(2024, 1, 3, 0, 0);
		ST_DERIVE_M1_BAR stChanged;
		const std::vector<std::string> aAuthoritySymbol =
			{ "EURUSD", "DST" };
		if (!Check(clEngine.ReconcileSymbols(5U, 1,
				aAuthoritySymbol, strError), strError.c_str()) ||
			!Check(clEngine.GetSymbolCount(5U, 1) == 2U,
				"Derive authoritative symbol reconcile count mismatch") ||
			!Check(!clEngine.ApplyTick(fnTick(900U, llDay1 + 60,
				llDay1 + 60, "UNKNOWN", 9.0), stChanged, strError) &&
				strError.find("SYMBOL_NOT_FOUND") != std::string::npos,
				"unknown Tick must not implicitly create a reconciled symbol") ||
			!Check(clEngine.MarkSymbolDeleted(5U, 1, "DST", strError),
				strError.c_str()) ||
			!Check(clEngine.GetSymbolCount(5U, 1) == 1U &&
				!clEngine.ApplyTick(fnTick(901U, llDay1 + 120,
					llDay1 + 120, "DST", 9.1), stChanged, strError),
				"deleted symbol accepted a Tick or remained active") ||
			!Check(clEngine.MarkSymbolActive(5U, 1, "DST", strError),
				strError.c_str()) ||
			!Check(clEngine.GetSymbolCount(5U, 1) == 2U,
				"re-enabled symbol did not reopen its existing mapping"))
		{
			return false;
		}
		if (!Check(clEngine.ApplyTick(fnTick(1U,
				llDay1 + 23 * 3600 + 58 * 60,
				llDay1 + 23 * 3600 + 58 * 60,
				"EURUSD", 1.10), stChanged, strError),
			strError.c_str()) ||
			!Check(clEngine.ApplyTick(fnTick(2U,
				llDay2 + 60, llDay2 + 60,
				"EURUSD", 1.20), stChanged, strError),
			strError.c_str()))
		{
			return false;
		}
		std::vector<ST_DERIVE_MINUTE_ARCHIVE_CANDIDATE> aArchive;
		clEngine.ListArchiveCandidates(aArchive);
		if (!Check(aArchive.size() == 1U &&
				aArchive[0].uiServerDate == 20240101U,
			"Derive first Broker day did not enter history slot"))
		{
			return false;
		}
		if (!Check(!clEngine.ApplyTick(fnTick(3U,
				llDay3 + 60, llDay3 + 60,
				"EURUSD", 1.30), stChanged, strError) &&
				strError.find("UNARCHIVED_SLOT_BLOCKED") !=
					std::string::npos,
			"Derive unarchived history slot did not block overwrite"))
		{
			return false;
		}
		const auto fnConfirmDay = [&clEngine, &strError](
			std::uint32_t p_uiDate, std::int64_t p_llDay)
		{
			ST_QUERY_M1_BACKFILL_RESPONSE stBackfill;
			stBackfill.enState = EN_DERIVE_SERVICE_STATE_READY;
			stBackfill.iCode = 0;
			stBackfill.usPlatformVersion = 5U;
			stBackfill.iSourceNo = 1;
			stBackfill.uiServerDate = p_uiDate;
			stBackfill.strSymbol = "EURUSD";
			stBackfill.llConfirmedFromMinute = p_llDay;
			stBackfill.llConfirmedToMinute = p_llDay + 86340LL;
			stBackfill.bConfirmedEmpty = true;
			stBackfill.bFullServerDay = true;
			stBackfill.ullTimeAuthorityEpoch = 8102U;
			stBackfill.ullTimeGeneration = 3U;
			return clEngine.MergeBackfill(stBackfill, strError);
		};
		if (!Check(fnConfirmDay(20240101U, llDay1),
			strError.c_str()))
		{
			return false;
		}
		ST_DERIVE_M1_SNAPSHOT_REQUEST stOpenPriceRequest;
		stOpenPriceRequest.usPlatformVersion = 5U;
		stOpenPriceRequest.iSourceNo = 1;
		stOpenPriceRequest.strSymbol = "EURUSD";
		stOpenPriceRequest.uiMaxBars = 1U;
		stOpenPriceRequest.bOpenPriceOnly = true;
		ST_DERIVE_M1_SNAPSHOT_RESPONSE stOpenPriceSnapshot;
		if (!Check(clEngine.Snapshot(stOpenPriceRequest,
				stOpenPriceSnapshot, strError), strError.c_str()) ||
			!Check(stOpenPriceSnapshot.aBar.size() == 1U &&
				stOpenPriceSnapshot.aBar[0].strSymbol == "EURUSD" &&
				stOpenPriceSnapshot.aBar[0].uiServerDate == 20240101U &&
				stOpenPriceSnapshot.aBar[0].dOpen == 1.10 &&
				(stOpenPriceSnapshot.aBar[0].uiFlags &
					DERIVE_M1_FLAG_OPEN_PRICE_CONFIRMED) != 0,
				"Derive OpenPrice-only snapshot did not return the confirmed first M1"))
		{
			return false;
		}
		stOpenPriceRequest.strSymbol = "DST";
		if (!Check(clEngine.Snapshot(stOpenPriceRequest,
				stOpenPriceSnapshot, strError), strError.c_str()) ||
			!Check(stOpenPriceSnapshot.aBar.empty(),
				"Derive OpenPrice-only snapshot returned an unconfirmed symbol"))
		{
			return false;
		}
		clEngine.ListArchiveCandidates(aArchive);
		ST_DERIVE_M1_ARCHIVE_BATCH stBatch;
		if (!Check(aArchive.size() == 1U &&
				clEngine.BuildArchiveBatch(aArchive[0], llDay1,
					llDay1 + 86340LL, "m1-test-day1", "owner-1",
					7U, stBatch, strError), strError.c_str()) ||
			!Check(stBatch.aBar.size() == 1U &&
				stBatch.strArchiveId.find("m1-test-day1-g") == 0 &&
				(stBatch.aBar[0].uiFlags &
					DERIVE_M1_FLAG_OPEN_PRICE_CONFIRMED) != 0,
				"Derive deterministic archive generation or content mismatch") ||
			!Check(clEngine.MarkArchiveSpooled(aArchive[0],
				llDay1 + 86340LL, strError), strError.c_str()) ||
			!Check(clEngine.ApplyTick(fnTick(3U,
				llDay3 + 60, llDay3 + 60,
				"EURUSD", 1.30), stChanged, strError),
				strError.c_str()))
		{
			return false;
		}

		// 第二天归档后收到晚到 Tick，历史槽必须重新进入待归档并产生新代次。
		if (!Check(fnConfirmDay(20240102U, llDay2),
				strError.c_str()))
		{
			return false;
		}
		clEngine.ListArchiveCandidates(aArchive);
		if (!Check(aArchive.size() == 1U &&
				clEngine.MarkArchiveSpooled(aArchive[0],
					llDay2 + 86340LL, strError), strError.c_str()) ||
			!Check(clEngine.ApplyTick(fnTick(50U,
				llDay2 + 60, llDay2 + 60,
				"EURUSD", 1.25), stChanged, strError),
				strError.c_str()))
		{
			return false;
		}
		clEngine.ListArchiveCandidates(aArchive);
		if (!Check(aArchive.size() == 1U &&
				aArchive[0].uiServerDate == 20240102U,
			"late Tick did not reopen archived history slot"))
		{
			return false;
		}

		// 同一个 Broker 01:30 对应两个 UTC 分钟时必须占用两个 occurrence。
		const std::int64_t llRepeatedServer = llDay3 + 90 * 60;
		if (!Check(fnUpdateOffset(-7 * 3600, 4U), strError.c_str()) ||
			!Check(clEngine.ApplyTick(fnTick(60U,
				llDay3 + 8 * 3600 + 30 * 60,
				llRepeatedServer, "DST", 2.0),
				stChanged, strError), strError.c_str()) ||
			!Check(fnUpdateOffset(-8 * 3600, 5U), strError.c_str()) ||
			!Check(clEngine.ApplyTick(fnTick(61U,
				llDay3 + 9 * 3600 + 30 * 60,
				llRepeatedServer, "DST", 2.1),
				stChanged, strError), strError.c_str()))
		{
			return false;
		}
		ST_DERIVE_M1_SNAPSHOT_REQUEST stRequest;
		stRequest.usPlatformVersion = 5U;
		stRequest.iSourceNo = 1;
		stRequest.strSymbol = "DST";
		stRequest.uiMaxBars = 10U;
		ST_DERIVE_M1_SNAPSHOT_RESPONSE stSnapshot;
		if (!Check(clEngine.Snapshot(stRequest, stSnapshot,
				strError), strError.c_str()) ||
			!Check(stSnapshot.aBar.size() == 2U &&
				stSnapshot.aBar[0].llMinute !=
					stSnapshot.aBar[1].llMinute,
				"Derive DST repeated hour did not preserve two occurrences"))
		{
			return false;
		}
		std::vector<ST_DERIVE_MINUTE_BACKFILL_CANDIDATE> aCurrent;
		clEngine.ListCurrentBackfillCandidates(aCurrent);
		ST_DERIVE_MINUTE_SOURCE_DIAGNOSTICS stDiagnostics;
		clEngine.GetSourceDiagnostics(5U, 1, stDiagnostics);
		if (!Check(!aCurrent.empty() &&
				stDiagnostics.uiArchivePendingCount >= 1U &&
				stDiagnostics.uiBackfillPendingCount >= 1U,
			"Derive current-day or 1186 source diagnostics missing"))
		{
			return false;
		}
		if (!Check(clEngine.FlushPersistent(strError),
			strError.c_str()))
		{
			return false;
		}
		clEngine.Clear();
		std::filesystem::path clMinuteFile;
		std::error_code stFsError;
		for (std::filesystem::recursive_directory_iterator it(clRoot,
			stFsError); !stFsError &&
			it != std::filesystem::recursive_directory_iterator(); ++it)
		{
			if (it->is_regular_file() && it->path().extension() == ".MIN")
			{
				clMinuteFile = it->path();
				break;
			}
		}
		if (!Check(!clMinuteFile.empty(),
			"Derive .MIN mapping file was not created"))
		{
			return false;
		}
		{
			std::fstream clFile(clMinuteFile,
				std::ios::binary | std::ios::in | std::ios::out);
			char chValue = 0;
			clFile.seekg(8, std::ios::beg);
			clFile.read(&chValue, 1);
			chValue = static_cast<char>(chValue ^ 0x5A);
			clFile.seekp(8, std::ios::beg);
			clFile.write(&chValue, 1);
		}
		CMtDeriveM1Engine clCorrupt;
		return Check(!clCorrupt.Configure(stConfig, strError),
			"corrupt Derive .MIN mapping must block startup");
	}

	// 验证 TickStore 连续 ACK、重复/缺口、检查点恢复、WAL 重放和 Profit 租约边界。
	bool TestDeriveRuntime()
	{
		const std::filesystem::path clTestPath =
			std::filesystem::temp_directory_path() /
			("TradingTerminalDeriveTest_" +
				std::to_string(
					static_cast<long long>(
						std::chrono::steady_clock::now().
							time_since_epoch().count())));
		CTestDirectoryGuard clDirectoryGuard(
			clTestPath);
		ST_MT_DERIVE_SERVICE_CONFIG stConfig;
		stConfig.strInstanceId = "Derive-Test-01";
		stConfig.stTick.strCachePath =
			clTestPath.u8string();
		stConfig.stTick.ullMaxWalBytes =
			64ULL * 1024ULL * 1024ULL;
		stConfig.stTick.uiMaxBatchTicks = 64;
		stConfig.stTick.szMinuteRetention = 120;
		ST_MT_DERIVE_SOURCE_CONFIG stSource;
		stSource.bEnable = true;
		stSource.usVersion = 5;
		stSource.iNo = 1;
		stConfig.aSource.push_back(stSource);

		CMtDeriveM1Engine clM1;
		CMtDeriveTickStore clStore;
		std::string strError;
		if (!Check(clM1.Configure(stConfig,
				strError), strError.c_str()))
		{
			return false;
		}
		const std::int64_t llNowMs =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count();
		ST_MT_TIME_STATE stTimeState;
		stTimeState.usPlatformVersion = 5U;
		stTimeState.iSourceNo = 1;
		stTimeState.strTimeZoneId = "UTC";
		stTimeState.iStandardOffsetSeconds = 0;
		stTimeState.iCurrentOffsetSeconds = 0;
		stTimeState.enSyncState = EN_MT_TIME_SYNC_READY;
		stTimeState.enChangeReason = EN_MT_TIME_CHANGE_SDK_UPDATE;
		stTimeState.ullAuthorityEpoch = 8005U;
		stTimeState.ullGeneration = 7U;
		stTimeState.llEffectiveUtcMs = llNowMs - 1000LL;
		stTimeState.llSampledUtcMs = llNowMs;
		stTimeState.llValidUntilUtcMs = llNowMs + 86400000LL;
		if (!Check(clM1.UpdateTimeState(stTimeState, strError),
				strError.c_str()) ||
			!Check(clM1.ReconcileSymbols(5U, 1,
				std::vector<std::string>({"TEST"}), strError),
				strError.c_str()) ||
			!Check(clStore.Configure(stConfig,
				&clM1, strError),
				strError.c_str()))
		{
			return false;
		}
		ST_DERIVE_TICK_BATCH_REQUEST stBatch;
		stBatch.usPlatformVersion = 5;
		stBatch.iSourceNo = 1;
		stBatch.ullSourceEpoch = 9101U;
		stBatch.ullFirstSequence = 1U;
		const double aPrice[] =
			{100.0, 110.0, 90.0, 101.0};
		for (std::size_t szIndex = 0;
			szIndex < _countof(aPrice);
			++szIndex)
		{
			ST_QUOTE_BINARY_TICK stTick;
			SetTestQuoteDefaults(stTick);
			stTick.usPlatformVersion = 5;
			stTick.iSourceNo = 1;
			stTick.ullSourceEpoch =
				stBatch.ullSourceEpoch;
			stTick.strSymbol = "TEST";
			stTick.dBid = aPrice[szIndex];
			stTick.dAsk = aPrice[szIndex] + 0.1;
			stTick.dLast = aPrice[szIndex];
			stTick.llServerTime = 1710000000;
			stTick.llServerTimeMsc =
				1710000000000 +
				static_cast<std::int64_t>(
					szIndex);
			stTick.llIngressTimeMs =
				1710000000100 +
				static_cast<std::int64_t>(
					szIndex);
			stTick.ullIngressSequence =
				1U + szIndex;
			stBatch.aTick.push_back(stTick);
		}
		ST_DERIVE_TICK_BATCH_RESPONSE stResponse;
		std::vector<ST_DERIVE_M1_BAR> aChanged;
		if (!Check(clStore.Append(stBatch,
				stResponse, aChanged,
				strError), strError.c_str()) ||
			!Check(stResponse.iCode == 0 &&
				stResponse.ullAckSequence == 4U &&
				stResponse.uiAcceptedCount == 4U,
				"Derive TickStore ACK mismatch"))
		{
			return false;
		}
		ST_DERIVE_TICK_BATCH_RESPONSE stDuplicate;
		if (!Check(clStore.Append(stBatch,
				stDuplicate, aChanged,
				strError), strError.c_str()) ||
			!Check(stDuplicate.iCode == 0 &&
				stDuplicate.uiAcceptedCount == 0U &&
				stDuplicate.ullAckSequence == 4U,
				"Derive duplicate batch must be idempotent"))
		{
			return false;
		}
		clStore.Stop();
		clM1.Clear();

		// 在旧单文件和后续物理段分别追加序号 5/6，模拟跨段 ACK 前崩溃；Configure 必须顺序重放并清理覆盖段。
		ST_QUOTE_BINARY_TICK stReplay =
			stBatch.aTick.back();
		stReplay.dBid = 105.0;
		stReplay.dAsk = 105.1;
		stReplay.dLast = 105.0;
		stReplay.ullIngressSequence = 5U;
		stReplay.llIngressTimeMs += 1;
		const std::filesystem::path clWalPath =
			clTestPath / "wal" / "v5_n1.wal";
		const auto fnAppendWalRecord = [&strError](
			const std::filesystem::path& p_refPath,
			const ST_QUOTE_BINARY_TICK& p_refTick)
		{
			std::vector<unsigned char> aEncoded;
			if (!EncodeQuoteBinaryTick(p_refTick, aEncoded, strError))
			{
				return false;
			}
			std::ofstream clWal(p_refPath,
				std::ios::binary | std::ios::app);
			const std::uint32_t uiLength =
				static_cast<std::uint32_t>(
					aEncoded.size());
			const unsigned char aLength[4] =
			{
				static_cast<unsigned char>(
					uiLength & 0xFFU),
				static_cast<unsigned char>(
					(uiLength >> 8U) & 0xFFU),
				static_cast<unsigned char>(
					(uiLength >> 16U) & 0xFFU),
				static_cast<unsigned char>(
					(uiLength >> 24U) & 0xFFU)
			};
			clWal.write(
				reinterpret_cast<const char*>(
					aLength), sizeof(aLength));
			clWal.write(
				reinterpret_cast<const char*>(
					aEncoded.data()),
				static_cast<std::streamsize>(
					aEncoded.size()));
			return static_cast<bool>(clWal);
		};
		const std::filesystem::path clSegmentPath =
			clTestPath / "wal" / "v5_n1.wal.999.seg";
		ST_QUOTE_BINARY_TICK stReplaySecond = stReplay;
		stReplaySecond.dBid = 106.0;
		stReplaySecond.dAsk = 106.1;
		stReplaySecond.dLast = 106.0;
		stReplaySecond.ullIngressSequence = 6U;
		++stReplaySecond.llIngressTimeMs;
		if (!Check(fnAppendWalRecord(clWalPath, stReplay),
				"Derive legacy replay WAL fixture write failed") ||
			!Check(fnAppendWalRecord(clSegmentPath, stReplaySecond),
				"Derive segmented replay WAL fixture write failed"))
		{
			return false;
		}

		CMtDeriveM1Engine clRestoredM1;
		CMtDeriveTickStore clRestoredStore;
		if (!Check(clRestoredM1.Configure(
				stConfig, strError), strError.c_str()) ||
			!Check(clRestoredM1.UpdateTimeState(
				stTimeState, strError), strError.c_str()) ||
			!Check(clRestoredM1.ReconcileSymbols(5U, 1,
				std::vector<std::string>({"TEST"}), strError),
				strError.c_str()) ||
			!Check(clRestoredStore.Configure(
				stConfig, &clRestoredM1,
				strError), strError.c_str()))
		{
			return false;
		}
		ST_DERIVE_M1_SNAPSHOT_REQUEST stSnapshotRequest;
		stSnapshotRequest.usPlatformVersion = 5;
		stSnapshotRequest.iSourceNo = 1;
		stSnapshotRequest.uiMaxBars = 100;
		ST_DERIVE_M1_SNAPSHOT_RESPONSE stSnapshot;
		if (!Check(clRestoredM1.Snapshot(
				stSnapshotRequest, stSnapshot,
				strError), strError.c_str()) ||
			!Check(stSnapshot.aBar.size() == 1U &&
				stSnapshot.aBar[0].dOpen == 100.0 &&
				stSnapshot.aBar[0].dHigh == 110.0 &&
				stSnapshot.aBar[0].dLow == 90.0 &&
				stSnapshot.aBar[0].dClose == 106.0 &&
				stSnapshot.aBar[0].ullTickVolume == 6U,
				"Derive WAL replay M1 mismatch"))
		{
			return false;
		}
		if (!Check(!std::filesystem::exists(clWalPath) &&
				!std::filesystem::exists(clSegmentPath),
			"checkpoint-covered WAL segments were not removed"))
		{
			return false;
		}
		ST_DERIVE_TICK_BATCH_REQUEST stGap;
		stGap.usPlatformVersion = 5;
		stGap.iSourceNo = 1;
		stGap.ullSourceEpoch = 9101U;
		stGap.ullFirstSequence = 8U;
		ST_QUOTE_BINARY_TICK stGapTick = stReplaySecond;
		stGapTick.ullIngressSequence = 8U;
		stGap.aTick.push_back(stGapTick);
		if (!Check(clRestoredStore.Append(stGap,
				stResponse, aChanged,
				strError), strError.c_str()) ||
			!Check(stResponse.iCode ==
				EN_TERMINAL_ERROR_DERIVE_SEQUENCE_GAP &&
				stResponse.ullAckSequence == 6U,
				"Derive sequence gap must not advance ACK"))
		{
			return false;
		}

		// Profit 租约验证 stale Revision、全局容量和到期清理。
		CMtDeriveProfitLease clLease;
		ST_MT_DERIVE_PROFIT_CONFIG stProfitConfig;
		stProfitConfig.uiDefaultLeaseMs = 5000;
		stProfitConfig.uiMaxLeaseMs = 60000;
		stProfitConfig.szMaxDemandLogins = 2;
		if (!Check(clLease.Configure(
				stProfitConfig, strError),
				strError.c_str()))
		{
			return false;
		}
		ST_DERIVE_PROFIT_DEMAND_REQUEST stDemand;
		stDemand.usPlatformVersion = 5;
		stDemand.iSourceNo = 1;
		stDemand.strGatewayInstance = "Gateway-A";
		stDemand.ullGatewayEpoch = 1001U;
		stDemand.ullRevision = 3U;
		stDemand.uiLeaseMs = 5000;
		stDemand.aLogin = {10001, 10002};
		ST_DERIVE_PROFIT_DEMAND_RESPONSE stDemandResponse;
		if (!Check(clLease.Apply(stDemand, 100000,
				stDemandResponse, strError),
				strError.c_str()) ||
			!Check(stDemandResponse.iCode == 0 &&
				clLease.GetActiveLoginCount(
					100001) == 2U,
				"Derive lease replace mismatch"))
		{
			return false;
		}
		stDemand.ullRevision = 2U;
		if (!Check(clLease.Apply(stDemand, 100002,
				stDemandResponse, strError),
				strError.c_str()) ||
			!Check(stDemandResponse.iCode ==
				EN_TERMINAL_ERROR_DERIVE_LEASE_STALE,
				"Derive stale lease revision must be rejected"))
		{
			return false;
		}
		ST_DERIVE_PROFIT_DEMAND_REQUEST stOverflow =
			stDemand;
		stOverflow.strGatewayInstance = "Gateway-B";
		stOverflow.ullGatewayEpoch = 2001U;
		stOverflow.ullRevision = 1U;
		stOverflow.aLogin = {10003};
		if (!Check(clLease.Apply(stOverflow, 100003,
				stDemandResponse, strError),
				strError.c_str()) ||
			!Check(stDemandResponse.iCode ==
				EN_TERMINAL_ERROR_DERIVE_QUEUE_BUSY,
				"Derive lease capacity must be enforced") ||
			!Check(clLease.GetActiveLoginCount(
				105001) == 0U,
				"Derive expired lease must be removed"))
		{
			return false;
		}
		return true;
	}

	// 校验 Derive 在 1167 安装前缓冲可靠增量、按快照水位回放并拒绝伪造命令事件。
	bool TestDeriveReliableStateBootstrap()
	{
		const auto fnAddField =
			[](ST_PLUGIN_BINARY_VALUE& p_refObject,
				const char* p_szName,
				const std::shared_ptr<
					ST_PLUGIN_BINARY_VALUE>& p_refValue)
			{
				ST_PLUGIN_BINARY_FIELD stField;
				stField.strName = p_szName;
				stField.refValue = p_refValue;
				p_refObject.aObjectField.push_back(
					stField);
			};
		const auto fnAddInt64 =
			[&fnAddField](ST_PLUGIN_BINARY_VALUE& p_refObject,
				const char* p_szName,
				std::int64_t p_llValue)
			{
				std::shared_ptr<ST_PLUGIN_BINARY_VALUE>
					refValue = CreatePluginBinaryValue(
						EN_PLUGIN_BINARY_VALUE_INT64);
				refValue->llIntValue = p_llValue;
				fnAddField(p_refObject, p_szName,
					refValue);
			};
		const auto fnAddUInt64 =
			[&fnAddField](ST_PLUGIN_BINARY_VALUE& p_refObject,
				const char* p_szName,
				std::uint64_t p_ullValue)
			{
				std::shared_ptr<ST_PLUGIN_BINARY_VALUE>
					refValue = CreatePluginBinaryValue(
						EN_PLUGIN_BINARY_VALUE_UINT64);
				refValue->ullUIntValue = p_ullValue;
				fnAddField(p_refObject, p_szName,
					refValue);
			};
		const auto fnAddDouble =
			[&fnAddField](ST_PLUGIN_BINARY_VALUE& p_refObject,
				const char* p_szName, double p_dValue)
			{
				std::shared_ptr<ST_PLUGIN_BINARY_VALUE>
					refValue = CreatePluginBinaryValue(
						EN_PLUGIN_BINARY_VALUE_DOUBLE);
				refValue->dDoubleValue = p_dValue;
				fnAddField(p_refObject, p_szName,
					refValue);
			};
		const auto fnAddString =
			[&fnAddField](ST_PLUGIN_BINARY_VALUE& p_refObject,
				const char* p_szName,
				const std::string& p_refValue)
			{
				std::shared_ptr<ST_PLUGIN_BINARY_VALUE>
					refValue = CreatePluginBinaryValue(
						EN_PLUGIN_BINARY_VALUE_STRING);
				refValue->strStringValue =
					p_refValue;
				fnAddField(p_refObject, p_szName,
					refValue);
			};
		const auto fnBuildUserPayload =
			[&fnAddInt64, &fnAddUInt64,
				&fnAddString](std::int64_t p_llVersion,
					std::int64_t p_llNo,
					std::uint64_t p_ullLogin,
					bool p_bSchema)
			{
				ST_PLUGIN_BINARY_VALUE stPayload;
				stPayload.enType =
					EN_PLUGIN_BINARY_VALUE_OBJECT;
				if (p_bSchema)
				{
					fnAddInt64(stPayload,
						"SchemaVersion", 1);
				}
				fnAddInt64(stPayload, "Version",
					p_llVersion);
				fnAddInt64(stPayload, "No", p_llNo);
				fnAddUInt64(stPayload, "Login",
					p_ullLogin);
				fnAddString(stPayload, "Group",
					"demo");
				fnAddString(stPayload, "Currency",
					"USD");
				return stPayload;
			};
		const auto fnBuildEnvelope =
			[&fnAddField, &fnAddInt64, &fnAddUInt64,
				&fnAddString](
					const ST_PLUGIN_BINARY_VALUE& p_refPayload,
					std::int64_t p_llVersion,
					std::int64_t p_llNo,
					std::uint64_t p_ullSourceSequence,
					std::vector<unsigned char>& p_refEnvelope,
					std::string& p_refError)
			{
				std::vector<unsigned char> aPayload;
				if (!EncodeTradeBinaryRequest(
						p_refPayload, aPayload,
						p_refError))
				{
					return false;
				}
				ST_PLUGIN_BINARY_VALUE stEnvelope;
				stEnvelope.enType =
					EN_PLUGIN_BINARY_VALUE_OBJECT;
				fnAddString(stEnvelope, "EventId",
					"derive-state-test-" +
					std::to_string(
						p_ullSourceSequence));
				fnAddInt64(stEnvelope, "Version",
					p_llVersion);
				fnAddInt64(stEnvelope, "No",
					p_llNo);
				fnAddInt64(stEnvelope, "NotifyId",
					EN_PLUGIN_NOTIFY_USER_CHANGED);
				fnAddInt64(stEnvelope, "Action",
					EN_PLUGIN_NOTIFY_ACTION_UPDATED);
				fnAddUInt64(stEnvelope, "SourceEpoch",
					1U);
				fnAddUInt64(stEnvelope,
					"SourceSequence",
					p_ullSourceSequence);
				fnAddInt64(stEnvelope,
					"CreatedTimeMs", 1);
				std::shared_ptr<ST_PLUGIN_BINARY_VALUE>
					refBytes = CreatePluginBinaryValue(
						EN_PLUGIN_BINARY_VALUE_BYTES);
				refBytes->aByteValue = aPayload;
				fnAddField(stEnvelope, "Payload",
					refBytes);
				return EncodeReliableEventBinaryRequest(
					stEnvelope, p_refEnvelope,
					p_refError);
			};

		ST_MT_DERIVE_SOURCE_CONFIG stSource;
		stSource.bEnable = true;
		stSource.usVersion = 5;
		stSource.iNo = 1;
		std::vector<ST_MT_DERIVE_SOURCE_CONFIG>
			aSource(1U, stSource);
		CMtDeriveStateStore clStore;
		std::string strError;
		if (!Check(clStore.Configure(aSource, 1U,
				strError), strError.c_str()))
		{
			return false;
		}

		// 第一个增量先于快照到达，必须被深拷贝缓冲且不能提前标记来源 READY。
		const ST_PLUGIN_BINARY_VALUE stUser =
			fnBuildUserPayload(5, 1, 10001U,
				true);
		std::vector<unsigned char> aEnvelope;
		if (!Check(fnBuildEnvelope(stUser, 5, 1,
				11U, aEnvelope, strError),
				strError.c_str()) ||
			!Check(!clStore.ApplyReliableEnvelope(
				11U, aEnvelope, strError) &&
				strError.find(
					"DERIVE_RELIABLE_RETRYABLE:") == 0,
				"derive pre-snapshot event was incorrectly acknowledged"))
		{
			return false;
		}
		ST_MT_DERIVE_STATE_STATUS stStatus;
		clStore.GetStatus(stStatus);
		if (!Check(!stStatus.bReady &&
				stStatus.szBufferedEventCount == 1U,
				"derive state did not buffer the pre-snapshot event"))
		{
			return false;
		}

		// 快照水位为 10，安装后必须回放 StreamSequence=11，并保留一个账户实体。
		ST_PLUGIN_BINARY_VALUE stSnapshot;
		stSnapshot.enType =
			EN_PLUGIN_BINARY_VALUE_OBJECT;
		fnAddInt64(stSnapshot, "Version", 5);
		fnAddInt64(stSnapshot, "No", 1);
		fnAddUInt64(stSnapshot, "SnapshotId", 7U);
		fnAddUInt64(stSnapshot, "EventSequence", 10U);
		static const char* s_aArrayField[] =
		{
			"Accounts", "Positions", "Symbols",
			"Groups", "Rates"
		};
		for (const char* pField : s_aArrayField)
		{
			std::shared_ptr<ST_PLUGIN_BINARY_VALUE>
				refArray = CreatePluginBinaryValue(
					EN_PLUGIN_BINARY_VALUE_ARRAY);
			fnAddField(stSnapshot, pField, refArray);
		}

		// 构造一份可计算的 MT5 权威状态，验证收益引擎不会依赖仓库内部对象或模拟零值。
		ST_PLUGIN_BINARY_VALUE* pAccounts =
			const_cast<ST_PLUGIN_BINARY_VALUE*>(
				FindPluginBinaryField(stSnapshot,
					"Accounts"));
		ST_PLUGIN_BINARY_VALUE* pPositions =
			const_cast<ST_PLUGIN_BINARY_VALUE*>(
				FindPluginBinaryField(stSnapshot,
					"Positions"));
		ST_PLUGIN_BINARY_VALUE* pSymbols =
			const_cast<ST_PLUGIN_BINARY_VALUE*>(
				FindPluginBinaryField(stSnapshot,
					"Symbols"));
		ST_PLUGIN_BINARY_VALUE* pRates =
			const_cast<ST_PLUGIN_BINARY_VALUE*>(
				FindPluginBinaryField(stSnapshot,
					"Rates"));
		if (!Check(pAccounts != nullptr &&
			pPositions != nullptr && pSymbols != nullptr &&
			pRates != nullptr,
			"derive snapshot arrays were not created"))
		{
			return false;
		}
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refAccount =
			CreatePluginBinaryValue(
				EN_PLUGIN_BINARY_VALUE_OBJECT);
		fnAddUInt64(*refAccount, "Login", 10001U);
		fnAddDouble(*refAccount, "Balance", 1000.0);
		fnAddDouble(*refAccount, "Credit", 0.0);
		fnAddDouble(*refAccount, "Profit", 0.0);
		fnAddDouble(*refAccount, "Floating", 0.0);
		fnAddDouble(*refAccount, "Equity", 1000.0);
		fnAddDouble(*refAccount, "Margin", 100.0);
		fnAddDouble(*refAccount, "MarginFree", 900.0);
		pAccounts->aArrayValue.push_back(refAccount);
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refSecondAccount =
			CreatePluginBinaryValue(
				EN_PLUGIN_BINARY_VALUE_OBJECT);
		fnAddUInt64(*refSecondAccount, "Login", 10002U);
		fnAddDouble(*refSecondAccount, "Balance", 2000.0);
		fnAddDouble(*refSecondAccount, "Credit", 0.0);
		fnAddDouble(*refSecondAccount, "Profit", 0.0);
		fnAddDouble(*refSecondAccount, "Floating", 0.0);
		fnAddDouble(*refSecondAccount, "Equity", 2000.0);
		fnAddDouble(*refSecondAccount, "Margin", 0.0);
		fnAddDouble(*refSecondAccount, "MarginFree", 2000.0);
		pAccounts->aArrayValue.push_back(refSecondAccount);

		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refPosition =
			CreatePluginBinaryValue(
				EN_PLUGIN_BINARY_VALUE_OBJECT);
		fnAddUInt64(*refPosition, "Login", 10001U);
		fnAddUInt64(*refPosition, "Position", 70001U);
		fnAddString(*refPosition, "Symbol", "EURUSD");
		fnAddInt64(*refPosition, "Action", 0);
		fnAddUInt64(*refPosition, "Volume", 10000U);
		fnAddDouble(*refPosition, "PriceOpen", 1.1000);
		fnAddDouble(*refPosition, "PriceCurrent", 1.2000);
		fnAddDouble(*refPosition, "Profit", 10000.0);
		fnAddDouble(*refPosition, "RateProfit", 1.0);
		pPositions->aArrayValue.push_back(refPosition);

		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refSymbol =
			CreatePluginBinaryValue(
				EN_PLUGIN_BINARY_VALUE_OBJECT);
		fnAddString(*refSymbol, "Symbol", "EURUSD");
		fnAddDouble(*refSymbol, "ContractSize", 100000.0);
		fnAddInt64(*refSymbol, "CalcMode", 0);
		pSymbols->aArrayValue.push_back(refSymbol);

		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refRate =
			CreatePluginBinaryValue(
				EN_PLUGIN_BINARY_VALUE_OBJECT);
		fnAddString(*refRate, "Symbol", "EURUSD");
		fnAddDouble(*refRate, "Bid", 1.2000);
		fnAddDouble(*refRate, "Ask", 1.2002);
		pRates->aArrayValue.push_back(refRate);
		if (!Check(clStore.InstallSnapshot(5, 1,
				stSnapshot, strError),
				strError.c_str()))
		{
			return false;
		}
		clStore.GetStatus(stStatus);
		if (!Check(stStatus.bReady &&
				stStatus.szBufferedEventCount == 0U &&
				stStatus.szAccountCount == 2U &&
				stStatus.ullLastReliableSequence == 11U,
				"derive state snapshot waterline replay is invalid"))
		{
			return false;
		}
		ST_MT_DERIVE_SOURCE_STATE_SNAPSHOT stStateSnapshot;
		if (!Check(clStore.SnapshotSource(5, 1,
				stStateSnapshot, strError),
				strError.c_str()) ||
			!Check(stStateSnapshot.bReady &&
				stStateSnapshot.aAccount.size() == 2U &&
				stStateSnapshot.aPosition.size() == 1U,
				"derive deep-copy source snapshot mismatch"))
		{
			return false;
		}

		CMtDeriveProfitLease clProfitLease;
		ST_MT_DERIVE_PROFIT_CONFIG stProfitConfig;
		stProfitConfig.uiWorkerThreads = 1U;
		stProfitConfig.szQueueCapacity = 1U;
		stProfitConfig.uiMergeIntervalMs = 100U;
		stProfitConfig.uiFullReconcileMs = 1000U;
		stProfitConfig.uiDefaultLeaseMs = 5000U;
		stProfitConfig.uiMaxLeaseMs = 60000U;
		stProfitConfig.szMaxDemandLogins = 10U;
		if (!Check(clProfitLease.Configure(
				stProfitConfig, strError),
				strError.c_str()))
		{
			return false;
		}
		CMtDeriveProfitEngine clProfitEngine;
		CMt5ProfitGroupQuoteService& refGroupQuoteService =
			CMt5ProfitGroupQuoteService::GetInstance();
		refGroupQuoteService.Stop();
		std::vector<ST_MT5_PROFIT_GROUP_QUOTE_NODE_CONFIG>
			aDisabledGroupQuoteConfig;
		if (!Check(refGroupQuoteService.Configure(
				aDisabledGroupQuoteConfig, strError),
				strError.c_str()))
		{
			return false;
		}
		if (!Check(clProfitEngine.Configure(
				stProfitConfig, aSource, &clStore,
				&clProfitLease, &refGroupQuoteService,
				strError),
				strError.c_str()))
		{
			return false;
		}
		ST_DERIVE_PROFIT_SNAPSHOT_REQUEST stProfitRequest;
		stProfitRequest.usPlatformVersion = 5;
		stProfitRequest.iSourceNo = 1;
		stProfitRequest.aLogin.push_back(10001);
		ST_DERIVE_PROFIT_SNAPSHOT_RESPONSE stProfitResponse;
		if (!Check(clProfitEngine.Snapshot(
				stProfitRequest, stProfitResponse,
				strError), strError.c_str()) ||
			!Check(stProfitResponse.iCode == 0 &&
				stProfitResponse.aPosition.size() == 1U &&
				stProfitResponse.aAccount.size() == 1U,
				"derive profit snapshot item count mismatch") ||
			!Check(std::fabs(
				stProfitResponse.aPosition[0].dProfit -
				10000.0) < 0.0001,
				"derive MT5 position profit mismatch") ||
			!Check(std::fabs(
				stProfitResponse.aAccount[0].dEquity -
				11000.0) < 0.0001,
				"derive account equity patch mismatch"))
		{
			return false;
		}

		// 0=实时必须由 Tick 立即唤醒固定 Worker；发布失败需清除摘要，使相同业务状态可再次发送。
		const std::int64_t llNowMs =
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count();
		ST_DERIVE_PROFIT_DEMAND_REQUEST stDemand;
		stDemand.usPlatformVersion = 5;
		stDemand.iSourceNo = 1;
		stDemand.strGatewayInstance = "Gateway-Profit-Test";
		stDemand.ullGatewayEpoch = 1U;
		stDemand.ullRevision = 1U;
		stDemand.uiLeaseMs = 5000U;
		stDemand.aLogin = {10001, 10002};
		ST_DERIVE_PROFIT_DEMAND_RESPONSE stDemandResponse;
		if (!Check(clProfitLease.Apply(stDemand, llNowMs,
				stDemandResponse, strError), strError.c_str()) ||
			!Check(stDemandResponse.iCode == EN_TERMINAL_ERROR_OK,
				"derive realtime profit lease was rejected"))
		{
			return false;
		}
		std::mutex clPublishMutex;
		std::condition_variable clPublishCondition;
		unsigned int uiPublishCount = 0;
		bool bReleaseSecondPublish = false;
		if (!Check(clProfitEngine.Start(
				[&clPublishMutex, &clPublishCondition,
					&uiPublishCount, &bReleaseSecondPublish](
						const ST_DERIVE_PROFIT_SNAPSHOT_RESPONSE&,
						std::string& p_refPublishError)
				{
					std::unique_lock<std::mutex> clLock(clPublishMutex);
					++uiPublishCount;
					clPublishCondition.notify_all();
					if (uiPublishCount == 1U)
					{
						p_refPublishError =
							"EXPECTED_TEST_PUBLISH_FAILURE";
						return false;
					}
					if (uiPublishCount == 2U)
					{
						clPublishCondition.wait(clLock,
							[&bReleaseSecondPublish]()
							{
								return bReleaseSecondPublish;
							});
					}
					return true;
				}, strError), strError.c_str()))
		{
			return false;
		}
		ST_DERIVE_TICK_BATCH_REQUEST stProfitTicks;
		stProfitTicks.usPlatformVersion = 5;
		stProfitTicks.iSourceNo = 1;
		stProfitTicks.ullSourceEpoch = 9001U;
		stProfitTicks.ullFirstSequence = 1U;
		ST_QUOTE_BINARY_TICK stProfitTick;
		stProfitTick.usPlatformVersion = 5;
		stProfitTick.iSourceNo = 1;
		stProfitTick.strSymbol = "EURUSD";
		stProfitTick.dBid = 1.2000;
		stProfitTick.dAsk = 1.2002;
		stProfitTick.llIngressTimeMs = llNowMs;
		stProfitTick.ullSourceEpoch = 9001U;
		stProfitTick.ullIngressSequence = 1U;
		stProfitTicks.aTick.push_back(stProfitTick);
		bool bRealtimeAccepted = clProfitEngine.OnTicks(
			stProfitTicks, strError);
		{
			std::unique_lock<std::mutex> clLock(clPublishMutex);
			clPublishCondition.wait_for(clLock,
				std::chrono::seconds(1), [&uiPublishCount]()
				{
					return uiPublishCount >= 1U;
				});
		}
		stProfitTicks.ullFirstSequence = 2U;
		stProfitTicks.aTick[0].ullIngressSequence = 2U;
		bool bRetryAccepted = clProfitEngine.OnTicks(
			stProfitTicks, strError);
		bool bSecondPublishEntered = false;
		{
			std::unique_lock<std::mutex> clLock(clPublishMutex);
			bSecondPublishEntered = clPublishCondition.wait_for(clLock,
				std::chrono::seconds(1), [&uiPublishCount]()
				{
					return uiPublishCount >= 2U;
				});
		}

		// 第二次发送阻塞时，容量为一的分片会溢出；全量核对完成前不得恢复 READY。
		bool bStateChangeAccepted = false;
		bool bDegradedObserved = false;
		if (bSecondPublishEntered)
		{
			bStateChangeAccepted =
				clProfitEngine.OnAuthoritativeStateChanged(5, 1, strError);
			bDegradedObserved = !clProfitEngine.IsReady();
		}
		{
			std::lock_guard<std::mutex> clLock(clPublishMutex);
			bReleaseSecondPublish = true;
			clPublishCondition.notify_all();
		}
		bool bRecovered = false;
		for (unsigned int uiWait = 0; uiWait < 200U; ++uiWait)
		{
			if (clProfitEngine.IsReady())
			{
				bRecovered = true;
				break;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
		clProfitEngine.Stop();
		if (!Check(bRealtimeAccepted && bRetryAccepted &&
				bSecondPublishEntered && uiPublishCount >= 2U,
				"derive realtime profit did not retry a failed unchanged notification") ||
			!Check(bStateChangeAccepted && bDegradedObserved,
				"derive profit queue overflow did not enter DEGRADED") ||
			!Check(bRecovered,
				"derive profit full reconciliation did not restore READY"))
		{
			return false;
		}

		// 缺少 SchemaVersion 的命令请求不是权威状态，必须拒绝且不能改变已安装状态。
		const ST_PLUGIN_BINARY_VALUE stCommand =
			fnBuildUserPayload(5, 1, 10002U,
				false);
		if (!Check(fnBuildEnvelope(stCommand, 5, 1,
				12U, aEnvelope, strError),
				strError.c_str()) ||
			!Check(!clStore.ApplyReliableEnvelope(
				12U, aEnvelope, strError) &&
				strError.find(
					"DERIVE_STATE_FIELD_MISSING") !=
					std::string::npos,
				"derive state accepted a command-derived event"))
		{
			return false;
		}

		// 新仓库容量为一条；第二条快照前增量必须返回可重试错误，不能被消费者送入 DLQ。
		CMtDeriveStateStore clLimitedStore;
		if (!Check(clLimitedStore.Configure(
				aSource, 1U, strError),
				strError.c_str()) ||
			!Check(fnBuildEnvelope(stUser, 5, 1,
				21U, aEnvelope, strError),
				strError.c_str()) ||
			!Check(!clLimitedStore.ApplyReliableEnvelope(
				21U, aEnvelope, strError) &&
				strError.find(
					"DERIVE_RELIABLE_RETRYABLE:") == 0,
				"derive buffered event did not remain unacknowledged") ||
			!Check(fnBuildEnvelope(stUser, 5, 1,
				22U, aEnvelope, strError),
				strError.c_str()) ||
			!Check(!clLimitedStore.ApplyReliableEnvelope(
				22U, aEnvelope, strError) &&
				strError.find(
					"DERIVE_RELIABLE_RETRYABLE:") == 0,
				"derive bootstrap capacity error is not retryable"))
		{
			return false;
		}

		// 人工重放默认遵守快照水位和实体 SourceSequence，只有显式 allow-stale 才能越过保护。
		bool bReplayApplied = false;
		if (!Check(fnBuildEnvelope(stUser, 5, 1,
				10U, aEnvelope, strError), strError.c_str()) ||
			!Check(clStore.ReplayReliableEnvelope(13U,
				aEnvelope, false, bReplayApplied, strError) &&
				!bReplayApplied,
				"derive replay did not suppress an older entity SourceSequence") ||
			!Check(fnBuildEnvelope(stUser, 5, 1,
				12U, aEnvelope, strError), strError.c_str()) ||
			!Check(clStore.ReplayReliableEnvelope(14U,
				aEnvelope, false, bReplayApplied, strError) &&
				bReplayApplied,
				"derive replay did not apply a newer entity SourceSequence") ||
			!Check(fnBuildEnvelope(stUser, 5, 1,
				9U, aEnvelope, strError), strError.c_str()) ||
			!Check(!clStore.ReplayReliableEnvelope(9U,
				aEnvelope, false, bReplayApplied, strError) &&
				strError.find("DERIVE_REPLAY_STALE_SNAPSHOT") == 0,
				"derive replay accepted an event covered by the snapshot") ||
			!Check(clStore.ReplayReliableEnvelope(15U,
				aEnvelope, true, bReplayApplied, strError) &&
				bReplayApplied,
				"derive explicit stale replay was not applied"))
		{
			return false;
		}
		return true;
	}

	// 验证 DLQ 顺序读取、EventId 校验、重放请求校验和及损坏拒绝。
	bool TestDeriveDeadLetterOperations()
	{
		const std::filesystem::path clRoot =
			std::filesystem::temp_directory_path() /
			"mt-derive-dlq-protocol-test";
		CTestDirectoryGuard clGuard(clRoot);
		std::error_code clDirectoryError;
		std::filesystem::create_directories(clRoot,
			clDirectoryError);
		if (!Check(!clDirectoryError,
			"derive DLQ test directory create failed"))
		{
			return false;
		}

		ST_PLUGIN_BINARY_VALUE stRoot;
		stRoot.enType = EN_PLUGIN_BINARY_VALUE_OBJECT;
		ST_PLUGIN_BINARY_FIELD stEventId;
		stEventId.strName = "EventId";
		stEventId.refValue = CreatePluginBinaryValue(
			EN_PLUGIN_BINARY_VALUE_STRING);
		stEventId.refValue->strStringValue =
			"derive-dlq-test-event";
		stRoot.aObjectField.push_back(stEventId);
		std::vector<unsigned char> aEnvelope;
		std::string strError;
		if (!Check(EncodeReliableEventBinaryRequest(
				stRoot, aEnvelope, strError), strError.c_str()))
		{
			return false;
		}
		const std::string strDlq =
			(clRoot / "dead-letter.bin").u8string();
		std::vector<ST_MT_DERIVE_DEAD_LETTER_RECORD> aRecord;
		if (!Check(AppendMtDeriveDeadLetter(strDlq,
				77U, aEnvelope, "invalid payload", strError),
				strError.c_str()) ||
			!Check(ReadMtDeriveDeadLetters(strDlq,
				aRecord, strError), strError.c_str()) ||
			!Check(aRecord.size() == 1U &&
				aRecord[0].strEventId == "derive-dlq-test-event" &&
				aRecord[0].ullStreamSequence == 77U,
				"derive DLQ record round trip mismatch"))
		{
			return false;
		}

		ST_MT_DERIVE_REPLAY_REQUEST stRequest;
		stRequest.strRequestId = "derive-dlq-test-request";
		stRequest.strEventId = aRecord[0].strEventId;
		stRequest.strOperator = "protocol-test";
		stRequest.strReason = "verify replay file";
		stRequest.ullStreamSequence = aRecord[0].ullStreamSequence;
		stRequest.aEnvelope = aRecord[0].aEnvelope;
		std::string strRequestPath;
		ST_MT_DERIVE_REPLAY_REQUEST stDecoded;
		if (!Check(WriteMtDeriveReplayRequest(
				(clRoot / "replay").u8string(), stRequest,
				strRequestPath, strError), strError.c_str()) ||
			!Check(ReadMtDeriveReplayRequest(strRequestPath,
				stDecoded, strError), strError.c_str()) ||
			!Check(stDecoded.strEventId == stRequest.strEventId &&
				stDecoded.aEnvelope == stRequest.aEnvelope,
				"derive replay request round trip mismatch") ||
			!Check(AppendMtDeriveReplayAudit(
				(clRoot / "audit.jsonl").u8string(),
				"TEST", "PASSED", stDecoded, "",
				strError), strError.c_str()))
		{
			return false;
		}
		{
			std::fstream clFile(std::filesystem::u8path(
				strRequestPath), std::ios::binary |
				std::ios::in | std::ios::out);
			if (!Check(static_cast<bool>(clFile),
				"derive replay request reopen failed"))
			{
				return false;
			}
			clFile.seekg(-1, std::ios::end);
			char chCorrupt = 0;
			clFile.read(&chCorrupt, 1);
			chCorrupt = static_cast<char>(chCorrupt ^ 0x5A);
			clFile.seekp(-1, std::ios::end);
			clFile.write(&chCorrupt, 1);
		}
		return Check(!ReadMtDeriveReplayRequest(
			strRequestPath, stDecoded, strError),
			"derive replay request checksum corruption was accepted");
	}

	// 校验 1167 参数、汇率分页字段以及未接入权威 MT 状态时拒绝伪造全量快照。
	bool TestQueryDeriveStateSnapshot()
	{
		ST_MT_QUERY_SERVICE_CONFIG stConfig;
		stConfig.strInstanceId = "Query-Test-Derive-State";
		ST_MT_QUERY_SOURCE_CONFIG stSource;
		stSource.bEnable = true;
		stSource.usVersion = 5;
		stSource.iNo = 1;
		stSource.strName = "MT5-1";
		stSource.bMock = true;
		AddQueryTestConnections(stSource);
		stConfig.aSource.push_back(stSource);
		CMtQueryQuoteCache clCache;
		std::string strError;
		if (!Check(clCache.Configure(stConfig, strError),
			strError.c_str()))
		{
			return false;
		}
		ST_QUOTE_SNAPSHOT_BINARY_RESPONSE stSnapshot;
		stSnapshot.usPlatformVersion = 5;
		stSnapshot.iSourceNo = 1;
		stSnapshot.ullSourceEpoch = 9001U;
		stSnapshot.ullHighWatermark = 2U;
		ST_QUOTE_BINARY_TICK stFirst =
			BuildQueryTick(1U, 1710000001,
				1.1010, 9001U);
		ST_QUOTE_BINARY_TICK stSecond = stFirst;
		stSecond.strSymbol = "USDJPY";
		stSecond.ullIngressSequence = 2U;
		stSecond.dBid = 151.1;
		stSecond.dAsk = 151.2;
		stSnapshot.aTick.push_back(stFirst);
		stSnapshot.aTick.push_back(stSecond);
		if (!Check(clCache.InstallSnapshot(stSnapshot,
			strError), strError.c_str()))
		{
			return false;
		}

		CMtQueryDispatcher clDispatcher;
		CMtQueryNodeManager clNodeManager;
		if (!Check(clNodeManager.Configure(stConfig,
				strError), strError.c_str()) ||
			!Check(clNodeManager.Start(strError),
				strError.c_str()) ||
			!Check(clDispatcher.Initialize(&stConfig,
				&clCache, &clNodeManager, strError),
				strError.c_str()))
		{
			return false;
		}
		ST_PLUGIN_BINARY_VALUE stRequest;
		stRequest.enType =
			EN_PLUGIN_BINARY_VALUE_OBJECT;
		const auto fnAddUInt =
			[&stRequest](const char* p_szName,
				std::uint64_t p_ullValue)
			{
				ST_PLUGIN_BINARY_FIELD stField;
				stField.strName = p_szName;
				stField.refValue =
					CreatePluginBinaryValue(
						EN_PLUGIN_BINARY_VALUE_UINT64);
				stField.refValue->ullUIntValue =
					p_ullValue;
				stRequest.aObjectField.push_back(
					stField);
			};
		fnAddUInt("Version", 5U);
		fnAddUInt("No", 1U);
		fnAddUInt("EntityMask",
			EN_DERIVE_STATE_ENTITY_RATE);
		fnAddUInt("Cursor", 0U);
		fnAddUInt("Limit", 1U);
		std::int32_t iCode =
			EN_TERMINAL_ERROR_INTERNAL_ERROR;
		std::string strMessage;
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refData;
		if (!Check(clDispatcher.Dispatch(
				EN_PLUGIN_FUNC_QUERY_DERIVE_STATE_SNAPSHOT,
				0, stRequest, iCode, strMessage,
				refData, strError), strError.c_str()) ||
			!Check(iCode == EN_TERMINAL_ERROR_OK &&
				refData != nullptr,
				"1167 rate snapshot must succeed"))
		{
			return false;
		}
		const ST_PLUGIN_BINARY_VALUE* pRates =
			FindPluginBinaryField(*refData, "Rates");
		const ST_PLUGIN_BINARY_VALUE* pHasMore =
			FindPluginBinaryField(*refData, "HasMore");
		if (!Check(pRates != nullptr &&
				pRates->enType ==
					EN_PLUGIN_BINARY_VALUE_ARRAY &&
				pRates->aArrayValue.size() == 1U &&
				pHasMore != nullptr &&
				pHasMore->enType ==
					EN_PLUGIN_BINARY_VALUE_BOOL &&
				pHasMore->ucBoolValue == 1U,
				"1167 rate pagination response mismatch"))
		{
			return false;
		}
		for (ST_PLUGIN_BINARY_FIELD& refField :
			stRequest.aObjectField)
		{
			if (refField.strName == "EntityMask")
			{
				refField.refValue->ullUIntValue =
					EN_DERIVE_STATE_ENTITY_ALL;
			}
		}
		return Check(clDispatcher.Dispatch(
				EN_PLUGIN_FUNC_QUERY_DERIVE_STATE_SNAPSHOT,
				0, stRequest, iCode, strMessage,
				refData, strError), strError.c_str()) &&
			Check(iCode == EN_TERMINAL_ERROR_OK &&
				refData != nullptr &&
				FindPluginBinaryField(*refData,
					"Accounts") != nullptr &&
				FindPluginBinaryField(*refData,
					"Positions") != nullptr,
				"1167 full authoritative snapshot is missing entity arrays");
	}

	// 校验 Binary/JSON 只在数据库边界转换，并验证 Dispatcher 注入 ClientData 后执行真实转发。
	bool TestQueryClientDataBoundary()
	{
		ST_PLUGIN_BINARY_VALUE stRequest;
		stRequest.enType = EN_PLUGIN_BINARY_VALUE_OBJECT;
		ST_PLUGIN_BINARY_FIELD stVersion;
		stVersion.strName = "Version";
		stVersion.refValue = CreatePluginBinaryValue(
			EN_PLUGIN_BINARY_VALUE_INT64);
		stVersion.refValue->llIntValue = 5;
		stRequest.aObjectField.push_back(stVersion);
		ST_PLUGIN_BINARY_FIELD stLogin;
		stLogin.strName = "Login";
		stLogin.refValue = CreatePluginBinaryValue(
			EN_PLUGIN_BINARY_VALUE_UINT64);
		stLogin.refValue->ullUIntValue = 10001U;
		stRequest.aObjectField.push_back(stLogin);

		nlohmann::json jsonRequest;
		std::string strError;
		if (!Check(ConvertPluginBinaryToJson(stRequest,
				jsonRequest, strError), strError.c_str()) ||
			!Check(jsonRequest.at("Version").get<int>() == 5 &&
				jsonRequest.at("Login").get<std::uint64_t>() ==
					10001U,
				"Binary to JSON database boundary mismatch"))
		{
			return false;
		}
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refRoundTrip;
		if (!Check(ConvertJsonToPluginBinary(jsonRequest,
				refRoundTrip, strError), strError.c_str()) ||
			!Check(refRoundTrip != nullptr &&
				refRoundTrip->enType ==
					EN_PLUGIN_BINARY_VALUE_OBJECT,
				"JSON to Binary database boundary mismatch"))
		{
			return false;
		}
		ST_PLUGIN_BINARY_VALUE stBytes;
		stBytes.enType = EN_PLUGIN_BINARY_VALUE_BYTES;
		stBytes.aByteValue = {0x01U};
		if (!Check(!ConvertPluginBinaryToJson(stBytes,
				jsonRequest, strError),
				"database bridge must reject raw bytes") ||
			!Check(strError.find("BYTES_UNSUPPORTED") !=
				std::string::npos,
				"raw bytes rejection detail is missing"))
		{
			return false;
		}

		ST_MT_QUERY_SERVICE_CONFIG stConfig;
		stConfig.strInstanceId = "Query-Test-ClientData";
		ST_MT_QUERY_SOURCE_CONFIG stSource;
		stSource.bEnable = true;
		stSource.usVersion = 5;
		stSource.iNo = 1;
		stSource.strName = "MT5-1";
		stConfig.aSource.push_back(stSource);
		CMtQueryQuoteCache clCache;
		if (!Check(clCache.Configure(stConfig, strError),
			strError.c_str()))
		{
			return false;
		}
		CTestClientDataService clClientData;
		CMtQueryDispatcher clDispatcher;
		CMtQueryNodeManager clNodeManager;
		if (!Check(clDispatcher.Initialize(&stConfig,
				&clCache, &clNodeManager, strError,
				&clClientData),
				strError.c_str()))
		{
			return false;
		}
		std::int32_t iCode = EN_TERMINAL_ERROR_INTERNAL_ERROR;
		std::string strMessage;
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refData;
		if (!Check(clDispatcher.Dispatch(
				EN_PLUGIN_FUNC_QUERY_WATCHLIST,
				EN_PLUGIN_WATCHLIST_ROUTE_SYMBOLS,
				stRequest, iCode, strMessage, refData,
				strError), strError.c_str()) ||
			!Check(clClientData.bDispatched &&
				iCode == EN_TERMINAL_ERROR_OK,
				"ClientData dispatcher injection failed"))
		{
			return false;
		}
		return true;
	}

	// 直接加载 dev/test 配置，验证五节点凭据、固定角色和物理连接数能够通过生产解析器。
	bool TestServiceConnectionConfigs()
	{
		const std::filesystem::path clRoot =
			std::filesystem::path(__FILE__).parent_path().
				parent_path().parent_path().parent_path();
		const std::string strExeDirectory =
			(clRoot / "bin" / "x64vc14").u8string();
		const std::uint32_t uiStateModes =
			EN_MT_TRADE_CONNECTION_MODE_ORDER |
			EN_MT_TRADE_CONNECTION_MODE_DEAL |
			EN_MT_TRADE_CONNECTION_MODE_POSITION |
			EN_MT_TRADE_CONNECTION_MODE_ACCOUNT |
			EN_MT_TRADE_CONNECTION_MODE_SOURCE_STATUS;
		const std::uint32_t uiConfigModes =
			EN_MT_TRADE_CONNECTION_MODE_USER |
			EN_MT_TRADE_CONNECTION_MODE_SYMBOL |
			EN_MT_TRADE_CONNECTION_MODE_GROUP;
		for (const char* pEnvironment : { "dev", "test" })
		{
			const std::string strSuffix = std::string("_") +
				pEnvironment + ".xml";
			std::string strError;
			ST_MT_QUOTE_SERVICE_CONFIG stQuote;
			if (!LoadMtQuoteServiceConfig(
					(clRoot / "conf" /
						("MtQuoteService" + strSuffix)).u8string(),
					strExeDirectory, stQuote, strError))
			{
				return Check(false, strError.c_str());
			}
			if (!Check(stQuote.aSource.size() == 5U,
					"Quote config must contain five V2 nodes"))
			{
				return false;
			}
			for (const ST_MT_QUOTE_SOURCE_CONFIG& refSource :
				stQuote.aSource)
			{
				if (!Check(refSource.aConnection.size() == 1U &&
					!refSource.bSourceFailoverEnable &&
					refSource.szQueueCapacity == 0U &&
					refSource.aConnection[0].uiPriority == 1U &&
					refSource.aConnection[0].uiCount == 1U,
					"Quote phase one source must use one priority-1 QUOTE_PUMPING connection without failover"))
				{
					return false;
				}
			}

			ST_MT_TRADE_SERVICE_CONFIG stTrade;
			strError.clear();
			if (!LoadMtTradeServiceConfig(
					(clRoot / "conf" /
						("MtTradeService" + strSuffix)).u8string(),
					strExeDirectory, stTrade, strError))
			{
				return Check(false, strError.c_str());
			}
			if (!Check(stTrade.aSource.size() == 5U,
					"Trade config must contain five V2 nodes"))
			{
				return false;
			}
			for (const ST_MT_TRADE_SOURCE_CONFIG& refSource :
				stTrade.aSource)
			{
				std::uint32_t uiPumpingModes = 0U;
				unsigned int uiCommandCount = 0U;
				unsigned int uiPrecheckCount = 0U;
				for (const ST_MT_TRADE_CONNECTION_CONFIG&
					refConnection : refSource.aConnection)
				{
					uiPumpingModes |= refConnection.uiModeMask;
					if (refConnection.enRole ==
						EN_MT_TRADE_CONNECTION_ROLE_TRADE_COMMAND)
					{
						uiCommandCount = refConnection.uiCount;
					}
					else if (refConnection.enRole ==
						EN_MT_TRADE_CONNECTION_ROLE_TRADE_PRECHECK)
					{
						uiPrecheckCount = refConnection.uiCount;
					}
				}
				if (!Check(refSource.aConnection.size() == 5U &&
					uiCommandCount == 6U && uiPrecheckCount == 6U &&
					uiPumpingModes == (uiStateModes | uiConfigModes),
					"Trade source role configuration is invalid"))
				{
					return false;
				}
			}

			ST_MT_QUERY_SERVICE_CONFIG stQuery;
			strError.clear();
			if (!LoadMtQueryServiceConfig(
					(clRoot / "conf" /
						("MtQueryService" + strSuffix)).u8string(),
					strExeDirectory, stQuery, strError))
			{
				return Check(false, strError.c_str());
			}
			if (!Check(stQuery.aSource.size() == 5U &&
					stQuery.stDatabase.bEnable &&
					!stQuery.stDatabase.strPassword.empty(),
					"Query config or PostgreSQL direct credentials are invalid"))
			{
				return false;
			}
			const unsigned int uiExpectedTimeBootstrapTimeoutMs =
				std::string(pEnvironment) == "dev" ? 5000U : 60000U;
			if (!Check(stQuery.uiTimeBootstrapTimeoutMs ==
					uiExpectedTimeBootstrapTimeoutMs,
				"Query time bootstrap timeout configuration mismatch"))
			{
				return false;
			}
			for (const ST_MT_QUERY_SOURCE_CONFIG& refSource :
				stQuery.aSource)
			{
				if (!Check(refSource.aConnection.size() == 2U &&
					refSource.aConnection[0].uiCount == 6U &&
					refSource.aConnection[1].uiCount == 6U,
					"Query ordinary/history pool configuration is invalid"))
				{
					return false;
				}
			}
		}

		// 复制开发模板只替换非敏感超时字段，验证解析器拒绝范围外值；临时目录由守卫立即清理。
		const std::filesystem::path clInvalidRoot =
			std::filesystem::temp_directory_path() /
			"MtQueryTimeBootstrapConfigTests";
		CTestDirectoryGuard clInvalidGuard(clInvalidRoot);
		std::error_code stDirectoryError;
		std::filesystem::create_directories(clInvalidRoot,
			stDirectoryError);
		if (!Check(!stDirectoryError,
			"Query invalid config test directory create failed"))
		{
			return false;
		}
		const std::filesystem::path clDevConfig =
			clRoot / "conf" / "MtQueryService_dev.xml";
		std::ifstream clInput(clDevConfig, std::ios::binary);
		const std::string strTemplate(
			(std::istreambuf_iterator<char>(clInput)),
			std::istreambuf_iterator<char>());
		if (!Check(clInput.good() || clInput.eof(),
			"Query dev config read failed"))
		{
			return false;
		}
		const auto fnRejectTimeout = [&](const char* p_szValue) -> bool
		{
			std::string strInvalid = strTemplate;
			const std::string strExpected =
				"timeBootstrapTimeoutMs=\"5000\"";
			const std::size_t szPosition =
				strInvalid.find(strExpected);
			if (szPosition == std::string::npos)
			{
				return false;
			}
			strInvalid.replace(szPosition, strExpected.size(),
				"timeBootstrapTimeoutMs=\"" +
				std::string(p_szValue) + "\"");
			const std::filesystem::path clInvalidPath =
				clInvalidRoot / (std::string("query-") +
					p_szValue + ".xml");
			std::ofstream clOutput(clInvalidPath,
				std::ios::binary | std::ios::trunc);
			clOutput.write(strInvalid.data(),
				static_cast<std::streamsize>(strInvalid.size()));
			clOutput.close();
			ST_MT_QUERY_SERVICE_CONFIG stInvalid;
			std::string strError;
			return !LoadMtQueryServiceConfig(
				clInvalidPath.u8string(), strExeDirectory,
				stInvalid, strError) &&
				strError.find("timeBootstrapTimeoutMs") !=
					std::string::npos;
		};
		return Check(fnRejectTimeout("999"),
			"Query time bootstrap timeout below minimum was accepted") &&
			Check(fnRejectTimeout("300001"),
				"Query time bootstrap timeout above maximum was accepted");
	}

	// 校验第一阶段容量和单连接约束，同时证明第二阶段主备代码仍可由显式开关配置。
	bool TestQuotePhaseOneConfigBoundaries()
	{
		const std::filesystem::path clRoot =
			std::filesystem::temp_directory_path() /
			"MtQuotePhaseOneConfigTests";
		CTestDirectoryGuard clGuard(clRoot);
		std::error_code stDirectoryError;
		std::filesystem::create_directories(clRoot, stDirectoryError);
		if (!Check(!stDirectoryError,
			"Quote config test directory create failed"))
		{
			return false;
		}

		const auto fnWriteConfig = [&clRoot](const char* p_szName,
			const char* p_szQueueCapacity, bool p_bFailover,
			bool p_bSecondConnection, bool p_bDuplicatePriority,
			std::filesystem::path& p_refPath) -> bool
		{
			p_refPath = clRoot / p_szName;
			std::string strXml =
				"<TradingTerminal><MtQuoteService>"
				"<Runtime mt4DllPath=\"mtmanapi64.dll\" "
				"mt5DllDirectory=\".\" allowDegradedStart=\"0\" "
				"dropLogIntervalMs=\"1000\"/>"
				"<Sources><Source enable=\"1\" version=\"5\" no=\"1\" "
				"name=\"MT5-TEST\" address=\"127.0.0.1:1\" login=\"1\" "
				"password=\"test\" queueCapacity=\"" +
				std::string(p_szQueueCapacity) +
				"\" sourceFailoverEnable=\"" +
				(p_bFailover ? std::string("1") : std::string("0")) +
				"\"><Connections>"
				"<Connection id=\"quote-primary\" priority=\"1\" "
				"role=\"QUOTE_PUMPING\" modes=\"quote\" count=\"1\"/>";
			if (p_bSecondConnection)
			{
				strXml +=
					"<Connection id=\"quote-secondary\" priority=\"" +
					(p_bDuplicatePriority ? std::string("1") :
						std::string("2")) +
					"\" role=\"QUOTE_PUMPING\" modes=\"quote\" count=\"1\"/>";
			}
			strXml +=
				"</Connections></Source></Sources>"
				"</MtQuoteService></TradingTerminal>";
			std::ofstream clOutput(p_refPath,
				std::ios::binary | std::ios::trunc);
			clOutput.write(strXml.data(),
				static_cast<std::streamsize>(strXml.size()));
			return static_cast<bool>(clOutput);
		};

		const std::string strExeDirectory = clRoot.u8string();
		ST_MT_QUOTE_SERVICE_CONFIG stConfig;
		std::string strError;
		std::filesystem::path clPath;
		if (!Check(fnWriteConfig("queue-0.xml", "0", false,
				false, false, clPath), "Quote queue-0 fixture write failed") ||
			!Check(LoadMtQuoteServiceConfig(clPath.u8string(),
				strExeDirectory, stConfig, strError), strError.c_str()) ||
			!Check(stConfig.aSource[0].szQueueCapacity == 0U,
				"Quote queueCapacity=0 was not preserved"))
		{
			return false;
		}
		if (!Check(fnWriteConfig("queue-128.xml", "128", false,
				false, false, clPath), "Quote queue-128 fixture write failed") ||
			!Check(LoadMtQuoteServiceConfig(clPath.u8string(),
				strExeDirectory, stConfig, strError), strError.c_str()) ||
			!Check(stConfig.aSource[0].szQueueCapacity == 128U,
				"Quote queueCapacity=128 was not preserved"))
		{
			return false;
		}
		if (!Check(fnWriteConfig("queue-127.xml", "127", false,
				false, false, clPath), "Quote queue-127 fixture write failed") ||
			!Check(!LoadMtQuoteServiceConfig(clPath.u8string(),
				strExeDirectory, stConfig, strError) &&
				strError.find("QUOTE_CONFIG_QUEUE_CAPACITY_INVALID") !=
					std::string::npos,
				"Quote must reject queueCapacity between 1 and 127"))
		{
			return false;
		}
		if (!Check(fnWriteConfig("single-mode-two.xml", "0", false,
				true, false, clPath), "Quote single-mode fixture write failed") ||
			!Check(!LoadMtQuoteServiceConfig(clPath.u8string(),
				strExeDirectory, stConfig, strError) &&
				strError.find("QUOTE_CONFIG_FAILOVER_INVALID") !=
					std::string::npos,
				"Quote phase one must reject a second physical connection"))
		{
			return false;
		}
		if (!Check(fnWriteConfig("failover-two.xml", "0", true,
				true, false, clPath), "Quote failover fixture write failed") ||
			!Check(LoadMtQuoteServiceConfig(clPath.u8string(),
				strExeDirectory, stConfig, strError), strError.c_str()))
		{
			return false;
		}
		return Check(fnWriteConfig("priority-duplicate.xml", "0", true,
				true, true, clPath), "Quote duplicate-priority fixture write failed") &&
			Check(!LoadMtQuoteServiceConfig(clPath.u8string(),
				strExeDirectory, stConfig, strError) &&
				strError.find("QUOTE_CONFIG_CONNECTION_DUPLICATE") !=
					std::string::npos,
				"Quote failover must reject duplicate priorities");
	}

	// 写入最小 Event 第一阶段配置，测试只使用本机占位 NATS URL，不建立网络连接。
	bool WriteEventConfigFixture(const std::filesystem::path& p_refPath,
		const char* p_szQueueCapacity, const char* p_szReplicas,
		const char* p_szBroadcastEnable)
	{
		std::ofstream clFile(p_refPath, std::ios::binary | std::ios::trunc);
		if (!clFile.is_open())
		{
			return false;
		}
		clFile << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
			"<Config><MtEventService>"
			"<MarketEvent workerThreads=\"4\" queueCapacity=\""
			<< p_szQueueCapacity << "\" dropLogIntervalMs=\"10000\"/>"
			"<Broadcast enable=\"" << p_szBroadcastEnable
			<< "\" subject=\"mt.event.broadcast\"/>"
			"<ReliableEvent enable=\"1\" url=\"nats://127.0.0.1:4222\" "
			"stream=\"MT_TRADING_EVENTS\" subjectPrefix=\"mt.trade\" replicas=\""
			<< p_szReplicas << "\"/>"
			"</MtEventService></Config>";
		return clFile.good();
	}

	// 校验 Event 第一阶段只接受容量 0 或 128-10000000，并固定单副本 JetStream。
	bool TestEventPhaseOneConfigBoundaries()
	{
		const std::filesystem::path clRoot =
			std::filesystem::temp_directory_path() /
			"MtEventPhaseOneConfigTests";
		CTestDirectoryGuard clGuard(clRoot);
		std::error_code stErrorCode;
		std::filesystem::create_directories(clRoot, stErrorCode);
		if (!Check(!stErrorCode, "Event config test directory creation failed"))
		{
			return false;
		}
		const std::filesystem::path clPath = clRoot / "MtEventService.xml";
		ST_MT_EVENT_SERVICE_CONFIG stConfig;
		std::string strError;
		if (!Check(WriteEventConfigFixture(clPath, "0", "1", "0"),
				"Event queueCapacity=0 fixture write failed") ||
			!Check(LoadMtEventServiceConfig(clPath.u8string(),
				stConfig, strError), strError.c_str()) ||
			!Check(stConfig.stMarketEvent.szQueueCapacity == 0U &&
				!stConfig.stBroadcast.bEnable &&
				stConfig.stReliableEvent.bEnable &&
				stConfig.stReliableEvent.uiReplicas == 1U,
				"Event first-phase config values mismatch") ||
			!Check(WriteEventConfigFixture(clPath, "128", "1", "0"),
				"Event bounded queue fixture write failed") ||
			!Check(LoadMtEventServiceConfig(clPath.u8string(),
				stConfig, strError) &&
				stConfig.stMarketEvent.szQueueCapacity == 128U,
				"Event queueCapacity=128 must be accepted"))
		{
			return false;
		}
		if (!Check(WriteEventConfigFixture(clPath, "127", "1", "0"),
				"Event invalid capacity fixture write failed") ||
			!Check(!LoadMtEventServiceConfig(clPath.u8string(),
				stConfig, strError) &&
				strError.find("EVENT_CONFIG_QUEUE_INVALID") !=
					std::string::npos,
				"Event queueCapacity below 128 must be rejected"))
		{
			return false;
		}
		return Check(WriteEventConfigFixture(clPath, "0", "3", "0"),
				"Event replicas fixture write failed") &&
			Check(!LoadMtEventServiceConfig(clPath.u8string(),
				stConfig, strError) &&
				strError.find("EVENT_CONFIG_REPLICAS_INVALID") !=
					std::string::npos,
				"Event first phase must reject replicas other than 1");
	}

	// 验证 Quote 实际 FIFO 在无限容量、正容量淘汰和跨 Epoch 排空时的顺序边界。
	bool TestQuoteIngressQueuePolicy()
	{
		CMtQuoteIngressQueue clUnlimited;
		for (std::uint64_t ullSequence = 1; ullSequence <= 3; ++ullSequence)
		{
			ST_QUOTE_BINARY_TICK stTick;
			stTick.ullSourceEpoch = 100;
			stTick.ullIngressSequence = ullSequence;
			if (!Check(!clUnlimited.Push(std::move(stTick), 0),
				"Quote queueCapacity=0 must not drop an accepted Tick"))
			{
				return false;
			}
		}
		if (!Check(clUnlimited.Size() == 3,
			"Quote unlimited queue size mismatch"))
		{
			return false;
		}
		for (std::uint64_t ullExpected = 1; ullExpected <= 3; ++ullExpected)
		{
			ST_QUOTE_BINARY_TICK stTick;
			if (!Check(clUnlimited.Pop(stTick) &&
					stTick.ullIngressSequence == ullExpected,
				"Quote unlimited queue must preserve FIFO order"))
			{
				return false;
			}
		}

		CMtQuoteIngressQueue clBounded;
		for (std::uint64_t ullSequence = 10; ullSequence <= 12; ++ullSequence)
		{
			ST_QUOTE_BINARY_TICK stTick;
			stTick.ullSourceEpoch = 200;
			stTick.ullIngressSequence = ullSequence;
			const bool bDropped = clBounded.Push(std::move(stTick), 2);
			if (!Check(bDropped == (ullSequence == 12),
				"Quote bounded queue drop decision mismatch"))
			{
				return false;
			}
		}
		ST_QUOTE_BINARY_TICK stFirst;
		ST_QUOTE_BINARY_TICK stSecond;
		if (!Check(clBounded.Pop(stFirst) && clBounded.Pop(stSecond) &&
				stFirst.ullIngressSequence == 11 &&
				stSecond.ullIngressSequence == 12,
			"Quote bounded queue must evict the oldest Tick"))
		{
			return false;
		}

		CMtQuoteIngressQueue clEpoch;
		ST_QUOTE_BINARY_TICK stOldEpoch;
		stOldEpoch.ullSourceEpoch = 300;
		stOldEpoch.ullIngressSequence = 7;
		ST_QUOTE_BINARY_TICK stNewEpoch;
		stNewEpoch.ullSourceEpoch = 301;
		stNewEpoch.ullIngressSequence = 1;
		clEpoch.Push(std::move(stOldEpoch), 0);
		clEpoch.Push(std::move(stNewEpoch), 0);
		if (!Check(clEpoch.Pop(stFirst) && clEpoch.Pop(stSecond) &&
				stFirst.ullSourceEpoch == 300 &&
				stSecond.ullSourceEpoch == 301,
			"Quote reconnect must drain the old Epoch before the new Epoch"))
		{
			return false;
		}
		return Check(clEpoch.Empty(),
			"Quote epoch queue must be empty after FIFO drain");
	}

	// 验证时间引导只要求启用节点 READY，且在超时后严格环境失败、开发环境明确降级。
	bool TestQueryTimeBootstrapDecision()
	{
		ST_MT_QUERY_SERVICE_CONFIG stConfig;
		if (!Check(stConfig.uiTimeBootstrapTimeoutMs == 30000U,
			"Query time bootstrap default timeout mismatch"))
		{
			return false;
		}
		ST_MT_QUERY_SOURCE_CONFIG stEnabled;
		stEnabled.bEnable = true;
		stEnabled.usVersion = 5U;
		stEnabled.iNo = 1;
		ST_MT_QUERY_SOURCE_CONFIG stDisabled;
		stDisabled.bEnable = false;
		stDisabled.usVersion = 4U;
		stDisabled.iNo = 2;
		stConfig.aSource.push_back(stEnabled);
		stConfig.aSource.push_back(stDisabled);
		ST_MT_TIME_STATE stReady;
		stReady.usPlatformVersion = 5U;
		stReady.iSourceNo = 1;
		stReady.enSyncState = EN_MT_TIME_SYNC_READY;
		stReady.llValidUntilUtcMs = 200000;
		std::vector<ST_MT_TIME_STATE> aState(1U, stReady);
		std::string strDetail;
		if (!Check(EvaluateMtQueryTimeBootstrap(stConfig,
				aState, 100000, false, strDetail) ==
				EN_MT_QUERY_TIME_BOOTSTRAP_READY,
			"Query ready time bootstrap decision mismatch"))
		{
			return false;
		}

		stConfig.aSource.push_back(ST_MT_QUERY_SOURCE_CONFIG());
		stConfig.aSource.back().bEnable = true;
		stConfig.aSource.back().usVersion = 4U;
		stConfig.aSource.back().iNo = 3;
		if (!Check(EvaluateMtQueryTimeBootstrap(stConfig,
				aState, 100000, false, strDetail) ==
				EN_MT_QUERY_TIME_BOOTSTRAP_WAITING,
			"Query missing time source must remain waiting"))
		{
			return false;
		}
		stConfig.bAllowDegradedStart = true;
		if (!Check(EvaluateMtQueryTimeBootstrap(stConfig,
				aState, 100000, true, strDetail) ==
				EN_MT_QUERY_TIME_BOOTSTRAP_DEGRADED,
			"Query dev timeout must enter degraded mode"))
		{
			return false;
		}
		stConfig.bAllowDegradedStart = false;
		if (!Check(EvaluateMtQueryTimeBootstrap(stConfig,
				aState, 100000, true, strDetail) ==
				EN_MT_QUERY_TIME_BOOTSTRAP_FAILED,
			"Query strict timeout must fail startup"))
		{
			return false;
		}
		stConfig.aSource.pop_back();
		stReady.llValidUntilUtcMs = 99999;
		aState[0] = stReady;
		return Check(EvaluateMtQueryTimeBootstrap(stConfig,
			aState, 100000, false, strDetail) ==
			EN_MT_QUERY_TIME_BOOTSTRAP_WAITING,
			"Query stale time source must not become ready");
	}

	bool TestMtTimeProtocol()
	{
		if (!Check(GetPluginFuncName(
				EN_PLUGIN_FUNC_QUOTE_TIME_SNAPSHOT) ==
			std::string("QUOTE_TIME_SNAPSHOT"),
			"1123 protocol name mismatch") ||
			!Check(GetPluginNotifyName(
				EN_PLUGIN_NOTIFY_SERVER_TIME_CHANGED) ==
			std::string("SERVER_TIME_CHANGED"),
			"1244 protocol name mismatch"))
		{
			return false;
		}

		ST_MT_TIME_STATE stState;
		stState.usPlatformVersion = 5;
		stState.iSourceNo = 2;
		stState.strTimeZoneId = "E. Europe Standard Time";
		stState.iStandardOffsetSeconds = 7200;
		stState.iCurrentOffsetSeconds = 10800;
		stState.bDaylight = true;
		stState.enSyncState = EN_MT_TIME_SYNC_READY;
		stState.enChangeReason = EN_MT_TIME_CHANGE_SDK_UPDATE;
		stState.ullAuthorityEpoch = 9001U;
		stState.ullGeneration = 7U;
		stState.llEffectiveUtcMs = 1710000000000LL;
		stState.llSampledUtcMs = 1710000001000LL;
		stState.llValidUntilUtcMs = 1710000181000LL;
		std::vector<unsigned char> aState;
		std::string strError;
		if (!Check(EncodeMtTimeState(stState, aState,
				strError), strError.c_str()))
		{
			return false;
		}
		ST_MT_TIME_STATE stDecoded;
		if (!Check(DecodeMtTimeState(aState.data(),
				aState.size(), stDecoded, strError),
			strError.c_str()) ||
			!Check(stDecoded.strTimeZoneId ==
					stState.strTimeZoneId &&
				stDecoded.iCurrentOffsetSeconds == 10800 &&
				stDecoded.ullAuthorityEpoch == 9001U &&
				stDecoded.ullGeneration == 7U,
				"time state round trip mismatch"))
		{
			return false;
		}

		ST_MT_TIME_SNAPSHOT_REQUEST stRequest;
		std::vector<unsigned char> aRequest;
		if (!Check(EncodeMtTimeSnapshotRequest(stRequest,
				aRequest, strError), strError.c_str()))
		{
			return false;
		}
		ST_MT_TIME_SNAPSHOT_REQUEST stDecodedRequest;
		if (!Check(DecodeMtTimeSnapshotRequest(aRequest.data(),
				aRequest.size(), stDecodedRequest, strError),
			strError.c_str()))
		{
			return false;
		}
		stRequest.usPlatformVersion = 5;
		stRequest.iSourceNo = 0;
		if (!Check(!EncodeMtTimeSnapshotRequest(stRequest,
				aRequest, strError),
			"partial Version+No time query must be rejected"))
		{
			return false;
		}

		ST_MT_TIME_SNAPSHOT_RESPONSE stResponse;
		stResponse.aState.push_back(stState);
		std::vector<unsigned char> aResponse;
		if (!Check(EncodeMtTimeSnapshotResponse(stResponse,
				aResponse, strError), strError.c_str()))
		{
			return false;
		}
		ST_MT_TIME_SNAPSHOT_RESPONSE stDecodedResponse;
		if (!Check(DecodeMtTimeSnapshotResponse(aResponse.data(),
				aResponse.size(), stDecodedResponse, strError),
			strError.c_str()) ||
			!Check(stDecodedResponse.aState.size() == 1U &&
				stDecodedResponse.aState[0].bDaylight,
				"time snapshot round trip mismatch"))
		{
			return false;
		}
		aResponse.pop_back();
		return Check(!DecodeMtTimeSnapshotResponse(
			aResponse.data(), aResponse.size(),
			stDecodedResponse, strError),
			"truncated time snapshot must be rejected");
	}

	// 验证传输层客户端控制通知不会被误当成 12xx 业务通知，同时拒绝相邻未登记编号。
	bool TestTransportControlNotifyIds()
	{
		return Check(IsTransportControlNotifyId(
				TRANSPORT_NOTIFY_CLIENT_ADDED),
			"transport client-added notify must be recognized") &&
			Check(IsTransportControlNotifyId(
				TRANSPORT_NOTIFY_CLIENT_SUBSCRIBED),
			"transport client-subscribed notify must be recognized") &&
			Check(IsTransportControlNotifyId(
				TRANSPORT_NOTIFY_CLIENT_UNSUBSCRIBED),
			"transport client-unsubscribed notify must be recognized") &&
			Check(IsTransportControlNotifyId(
				TRANSPORT_NOTIFY_CLIENT_DELETED),
			"transport client-deleted notify must be recognized") &&
			Check(!IsTransportControlNotifyId(
				TRANSPORT_NOTIFY_CLIENT_UNSUBSCRIBED + 1),
			"unregistered transport notify must remain invalid") &&
			Check(!IsTransportControlNotifyId(
				EN_PLUGIN_NOTIFY_MARKET_TICK),
			"business notify must not be classified as transport control");
	}

	// 验证 1103 集群状态固定小端协议的往返、角色字段和尾随字节拒绝规则。
	bool TestClusterStateProtocol()
	{
		ST_CLUSTER_STATE_RESPONSE stState;
		stState.stInstance.enPluginId = EN_PLUGIN_ID_MT_QUOTE_SERVICE;
		stState.stInstance.enRole = EN_CLUSTER_ROLE_OWNER;
		stState.stInstance.strServiceName = "MtQuoteService";
		stState.stInstance.strInstanceId = "MtQuoteService-01";
		stState.stInstance.strNodeId = "CloudNetNode1";
		stState.stInstance.strAdapterId = "MtQuoteServiceRPC-1";
		stState.stInstance.bProcessReady = true;
		stState.stInstance.llUpdatedAtMs = 1700000000000LL;
		stState.stInstance.strDetail = "OK";
		ST_CLUSTER_LEASE stLease;
		stLease.usPlatformVersion = 5;
		stLease.iSourceNo = 2;
		stLease.enRole = EN_CLUSTER_ROLE_OWNER;
		stLease.strOwnerInstanceId = "MtQuoteService-01";
		stLease.ullLeaseGeneration = 42;
		stLease.llExpiresAtMs = 1700000015000LL;
		stLease.llLastRenewedMs = 1700000005000LL;
		stLease.bDataReady = true;
		stState.aLease.push_back(stLease);
		ST_CLUSTER_RUNTIME_METRIC stMetric;
		stMetric.strName = "process.private_bytes";
		stMetric.ullValue = 123456789U;
		stState.aMetric.push_back(stMetric);

		std::vector<unsigned char> aPayload;
		std::string strError;
		if (!Check(EncodeClusterStateResponse(stState, aPayload,
				strError), strError.c_str()))
		{
			return false;
		}
		ST_CLUSTER_STATE_RESPONSE stDecoded;
		if (!Check(DecodeClusterStateResponse(aPayload.data(),
				aPayload.size(), stDecoded, strError), strError.c_str()) ||
			!Check(stDecoded.stInstance.strInstanceId ==
					"MtQuoteService-01" && stDecoded.aLease.size() == 1U &&
					stDecoded.aLease[0].ullLeaseGeneration == 42U &&
					stDecoded.aLease[0].bDataReady &&
					stDecoded.aMetric.size() == 1U &&
					stDecoded.aMetric[0].strName ==
						"process.private_bytes" &&
					stDecoded.aMetric[0].ullValue == 123456789U,
				"cluster state round trip mismatch"))
		{
			return false;
		}
		std::vector<unsigned char> aTrailing = aPayload;
		aTrailing.push_back(0U);
		if (!Check(!DecodeClusterStateResponse(aTrailing.data(),
			aTrailing.size(), stDecoded, strError),
			"cluster state trailing byte must be rejected"))
		{
			return false;
		}
		ST_CLUSTER_STATE_RESPONSE stLegacy = stState;
		stLegacy.usVersion = CLUSTER_PROTOCOL_VERSION_V1;
		stLegacy.stInstance.usVersion = CLUSTER_PROTOCOL_VERSION_V1;
		stLegacy.aMetric.clear();
		aPayload.clear();
		return Check(EncodeClusterStateResponse(stLegacy, aPayload,
				strError), strError.c_str()) &&
			Check(DecodeClusterStateResponse(aPayload.data(),
				aPayload.size(), stDecoded, strError), strError.c_str()) &&
			Check(stDecoded.usVersion == CLUSTER_PROTOCOL_VERSION_V1 &&
				stDecoded.aMetric.empty(),
				"cluster state v1 compatibility mismatch");
	}

	// 验证网关只路由到分片 Owner，并在探测失败后清除旧租约、接管后切换到新 Owner。
	bool TestGatewayOwnerAwareRouting()
	{
		SetClusterProbeState("MtQuoteService-1", 1, 1);
		SetClusterProbeState("MtQuoteService-2", 2, 1);
		SetClusterProbeState("MtQueryService-1", 0, 1);
		SetClusterProbeState("MtTradeService-1", 0, 1);
		CMtGatewayServiceMonitor clMonitor;
		ST_MT_GATEWAY_MONITOR_CONFIG stMonitorConfig;
		stMonitorConfig.bEnable = true;
		stMonitorConfig.iIntervalMs = 10;
		stMonitorConfig.iFailureThreshold = 1;
		stMonitorConfig.iRecoveryThreshold = 1;
		const std::vector<std::string> aService =
		{
			"MtQuoteService-1", "MtQuoteService-2",
			"MtQueryService-1", "MtTradeService-1"
		};
		std::string strError;
		if (!Check(clMonitor.Start(reinterpret_cast<HCLOUD_NET_API>(1),
				aService, stMonitorConfig, strError), strError.c_str()))
		{
			return false;
		}
		for (unsigned int uiWait = 0; uiWait < 100U &&
			!clMonitor.IsReady("MtTradeService-1"); ++uiWait)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
		std::vector<ST_MT_GATEWAY_BACKEND_CONFIG> aBackend;
		const auto fnAddBackend = [&aBackend](const char* p_szLogical,
			const char* p_szConnection)
		{
			ST_MT_GATEWAY_BACKEND_CONFIG stBackend;
			stBackend.bEnable = true;
			stBackend.strLogicalService = p_szLogical;
			stBackend.strConnectionName = p_szConnection;
			aBackend.push_back(stBackend);
		};
		fnAddBackend("MtQuoteService", "MtQuoteService-1");
		fnAddBackend("MtQuoteService", "MtQuoteService-2");
		fnAddBackend("MtQueryService", "MtQueryService-1");
		fnAddBackend("MtTradeService", "MtTradeService-1");
		CMtGatewayBackendPool clPool;
		std::string strConnection;
		const std::vector<std::pair<std::uint16_t, std::int32_t>> aShard =
		{
			std::make_pair(static_cast<std::uint16_t>(5U),
				static_cast<std::int32_t>(1))
		};
		if (!Check(clPool.Configure(aBackend, &clMonitor,
				strError), strError.c_str()) ||
			!Check(clPool.SelectReady("MtQuoteService", "",
				aShard, true, strConnection) &&
				strConnection == "MtQuoteService-1",
				"Gateway did not select the current Quote Owner"))
		{
			clMonitor.Stop();
			return false;
		}

		// 第一步：让旧 Owner 探测失败；一次失败即 DOWN，租约必须同步失效。
		SetClusterProbeState("MtQuoteService-1", -1, 1);
		for (unsigned int uiWait = 0; uiWait < 100U &&
			clMonitor.IsReady("MtQuoteService-1"); ++uiWait)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
		if (!Check(!clMonitor.IsOwner("MtQuoteService-1", 5U, 1),
				"Gateway retained a stale Owner lease after probe failure") ||
			!Check(!clPool.SelectReady("MtQuoteService", "",
				aShard, true, strConnection),
				"Gateway routed to a Standby while no Owner was available"))
		{
			clMonitor.Stop();
			return false;
		}

		// 第二步：备用实例成为 Owner 后，下一轮 1103 应使路由自动切换。
		SetClusterProbeState("MtQuoteService-2", 1, 1);
		for (unsigned int uiWait = 0; uiWait < 100U &&
			!clMonitor.IsOwner("MtQuoteService-2", 5U, 1); ++uiWait)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
		const bool bSelected = clPool.SelectReady("MtQuoteService", "",
			aShard, true, strConnection) &&
			strConnection == "MtQuoteService-2";
		clPool.Reset();
		clMonitor.Stop();
		return Check(bSelected,
			"Gateway did not switch routing to the recovered Quote Owner");
	}
}

bool TestV2ErrorCompatibilityMessages()
{
	struct ST_EXPECTED_CODE_MESSAGE
	{
		int iCode;
		const char* pMessage;
	};
	const ST_EXPECTED_CODE_MESSAGE aCompatibility[] =
	{
		{ 0, "success" },
		{ 20000, "sys error, such as memory allocate failed" },
		{ 20001, "param error" },
		{ 20002, "mt request api return no data" },
		{ 20003, "no match data" },
		{ 20004, "json param parse failed" },
		{ 20005, "no available mt service" },
		{ 20006, "business not support" },
		{ 20007, "no available db conn" },
		{ 20008, "system not ready" },
		{ 20009, "main/investor pwd same" },
		{ 20010, "invalid parameter" },
		{ 20011, "invalid token" },
		{ 20012, "symbol volume rank is querying" },
		{ 20013, "token login mismatch" },
		{ 20014, "place order trade check failed" },
		{ 20015, "Account disabled" },
		{ 21000, "error, need json format" },
		{ 21001, "error, action is not string" },
		{ 21002, "error, sub_list is not array" },
		{ 21003, "error, sub_list element is not string" },
		{ 21004, "error, need login(user id)" },
		{ 21005, "error, logins is not array" },
		{ 21006, "error, logins element is not number" },
		{ 21007, "error, action unsupported" },
		{ 21008, "error, user not found" },
		{ 21009, "error, SubType invalid" },
		{ 21010, "error, Topic invalid" },
		{ 21011, "error, SymbolId invalid" },
		{ 21012, "error, Login invalid" },
		{ 21013, "error, Topic not allowed" },
		{ 21014, "error, Version invalid" },
		{ 21015, "error, No invalid" },
		{ 21016, "error, Timestamp invalid" },
		{ 21017, "error, FuncId invalid" },
		{ 21018, "error, Type invalid" },
		{ 21019, "error, Seq invalid" },
		{ 21020, "error, TraceId invalid" },
		{ 21021, "http business queue full" },
		{ 21022, "mt manager connection busy" },
		{ 22000, "postgres pool is disabled" },
		{ 22001, "postgres pool config invalid" },
		{ 22002, "postgres connect failed" },
		{ 22003, "postgres pool is not initialized" },
		{ 22004, "acquire postgres connection timeout" },
		{ 22005, "postgres execute failed" },
		{ 22006, "postgres connection is broken" },
		{ 22007, "client data param invalid" },
		{ 22008, "client data db error" },
		{ 22009, "client data opt type invalid" },
		{ 22010, "client data account invalid" },
		{ 22011, "client data api invalid" },
		{ 22027, "watchlist item not found" },
		{ 22028, "watchlist conflict" },
		{ 22029, "watchlist only supports query" },
		{ 22030, "watchlist symbol ids invalid" },
		{ 22031, "watchlist group ids invalid" },
		{ 22032, "watchlist section id invalid" },
		{ 22033, "watchlist section name invalid" },
		{ 22034, "watchlist move group invalid" },
		{ 22035, "watchlist order invalid" },
		{ 22036, "watchlist default group delete denied" },
		{ 22037, "watchlist section not found" },
		{ 22038, "watchlist source group not found" },
		{ 22039, "watchlist target group not found" },
		{ 22040, "watchlist symbol not found" },
		{ 22041, "watchlist section name exists" },
		{ 22042, "watchlist section id exhausted" },
		{ 22053, "chart drawing symbol invalid" },
		{ 22054, "chart drawing interval invalid" },
		{ 22055, "chart drawing id invalid" },
		{ 22056, "chart drawing tool type invalid" },
		{ 22057, "chart drawing content invalid" },
		{ 22058, "chart drawing not found" },
		{ 22069, "chart indicator id invalid" },
		{ 22070, "chart indicator type invalid" },
		{ 22071, "chart indicator content invalid" },
		{ 22072, "chart indicator not found" },
		{ 22083, "chart config content invalid" },
		{ 22084, "chart config not found" }
	};
	for (const ST_EXPECTED_CODE_MESSAGE& refExpected : aCompatibility)
	{
		const char* pCompatibility =
			GetV2CompatibilityErrorMsg(refExpected.iCode);
		const char* pExternal = GetExternalErrorMsg(refExpected.iCode);
		if (!Check(pCompatibility != nullptr && pExternal != nullptr &&
				std::string(pCompatibility) == refExpected.pMessage &&
				std::string(pExternal) == refExpected.pMessage,
				"V2 compatibility full error table mismatch"))
		{
			return false;
		}
	}

	const ST_EXPECTED_CODE_MESSAGE aNetwork[] =
	{
		{ 23000, "business process failed" },
		{ 23001, "frame process failed" },
		{ 23002, "unknown abnormal" },
		{ 23003, "server stopped" },
		{ 23100, "user network disconnected" },
		{ 23101, "user not exist" },
		{ 23102, "user request overflow" },
		{ 23103, "user request length invalid" },
		{ 23104, "websocket connection count exceed limit" },
		{ 23105, "user network type invalid" },
		{ 23106, "user request buffer overflow" },
		{ 23107, "user response buffer overflow" }
	};
	for (const ST_EXPECTED_CODE_MESSAGE& refExpected : aNetwork)
	{
		const char* pNetwork = GetV2NetworkErrorMsg(refExpected.iCode);
		if (!Check(pNetwork != nullptr &&
				std::string(pNetwork) == refExpected.pMessage,
				"V2 network context error table mismatch"))
		{
			return false;
		}
	}

	struct ST_EXPECTED_CODE_NORMALIZATION
	{
		int iInputCode;
		int iOutputCode;
	};
	const ST_EXPECTED_CODE_NORMALIZATION aNormalization[] =
	{
		{ EN_TERMINAL_ERROR_INVALID_REQUEST, 20001 },
		{ EN_TERMINAL_ERROR_SERVICE_UNAVAILABLE, 20008 },
		{ EN_TERMINAL_ERROR_REQUEST_TIMEOUT, 20008 },
		{ EN_TERMINAL_ERROR_ASYNC_SUBMIT_FAILED, 20005 },
		{ EN_TERMINAL_ERROR_REMOTE_ERROR, 20005 },
		{ EN_TERMINAL_ERROR_PROTOCOL_ERROR, 20000 },
		{ EN_TERMINAL_ERROR_INTERNAL_ERROR, 20000 },
		{ EN_TERMINAL_ERROR_TRADE_NODE_UNAVAILABLE, 20005 },
		{ EN_TERMINAL_ERROR_TRADE_QUEUE_BUSY, 21022 },
		{ EN_TERMINAL_ERROR_DERIVE_BOOTSTRAPPING, 20008 },
		{ EN_TERMINAL_ERROR_MT_TIME_STALE, 20008 },
		{ EN_TERMINAL_ERROR_SERVICE_NOT_OWNER, 20008 }
	};
	for (const ST_EXPECTED_CODE_NORMALIZATION& refExpected : aNormalization)
	{
		if (!Check(NormalizeV2ExternalErrorCode(refExpected.iInputCode) ==
				refExpected.iOutputCode,
				"V3 to V2 external error normalization mismatch"))
		{
			return false;
		}
	}

	const char* pDeriveInternal = GetTerminalErrorMsg(
		EN_TERMINAL_ERROR_DERIVE_BOOTSTRAPPING);
	return Check(GetV2CompatibilityErrorMsg(
			EN_TERMINAL_ERROR_DERIVE_BOOTSTRAPPING) == nullptr &&
		pDeriveInternal != nullptr &&
		std::string(pDeriveInternal) == "DERIVE_STATE_BOOTSTRAPPING" &&
		GetV2CompatibilityErrorMsg(99999) == nullptr &&
		GetExternalErrorMsg(99999) == nullptr &&
		GetV2NetworkErrorMsg(99999) == nullptr,
		"Error protocol context isolation mismatch");
}

	// CV2Mt4ProfitTestDataSource：测试专用 Owner 适配器，验证公共函数不依赖 V2 全局单体。
	class CV2Mt4ProfitTestDataSource :
		public IV2Mt4PositionProfitDataSource
	{
	public:
		void GetTradesByLogin(int, int,
			std::vector<MT4TradeRecord>& p_refTrades) const override
		{
			p_refTrades = aTrade;
		}

		bool GetSymbolQuote(int, const std::string&,
			STQuoteData& p_refQuote) const override
		{
			p_refQuote = stQuote;
			return true;
		}

		bool GetSymbol(int, const std::string&,
			MT4ConSymbol& p_refSymbol) const override
		{
			p_refSymbol = stSymbol;
			return true;
		}

		bool StoreMarginLevel(int,
			const MT4MarginLevel& p_refMargin,
			const double* p_pLocalFloating) override
		{
			stStoredMargin = p_refMargin;
			bHasAnchor = p_pLocalFloating != nullptr;
			dAnchor = bHasAnchor ? *p_pLocalFloating : 0.0;
			return true;
		}

		std::vector<MT4TradeRecord> aTrade; // Owner 持有的持仓快照。
		STQuoteData stQuote;                // Owner 持有的行情快照。
		MT4ConSymbol stSymbol = {};         // Owner 持有的品种快照。
		MT4MarginLevel stStoredMargin = {}; // 最近一次写回的保证金。
		bool bHasAnchor = false;             // 是否写入本地浮动锚点。
		double dAnchor = 0.0;               // 最近写入的本地浮动值。
	};

	bool TestV2CompatibilityCommonUtilities()
	{
		// ClientData Owner 的返回码和公开成员函数必须保持 V2 的 int 契约，避免错误码被静默截断。
		static_assert(std::is_same<decltype(ST_POSTGRESQL_RESULT::nCode),
			int>::value, "V2 PostgreSQL result code type changed");
		static_assert(std::is_same<decltype(ST_WATCHLIST_RET::nCode),
			int>::value, "V2 WatchList result code type changed");
		static_assert(std::is_same<decltype(ST_CHART_CONFIG_RET::nCode),
			int>::value, "V2 Chart Config result code type changed");
		static_assert(std::is_same<decltype(ST_CHART_DRAWING_RET::nCode),
			int>::value, "V2 Chart Drawing result code type changed");
		static_assert(std::is_same<decltype(ST_CHART_INDICATOR_RET::nCode),
			int>::value, "V2 Chart Indicator result code type changed");
		static_assert(std::is_same<
			decltype(&CPostgreSqlPool::CConnectionGuard::GetCode),
			int (CPostgreSqlPool::CConnectionGuard::*)() const>::value,
			"V2 PostgreSQL connection guard signature changed");
		static_assert(std::is_same<decltype(&CWatchListRepository::Execute),
			ST_WATCHLIST_RET (CWatchListRepository::*)(
				E_WATCHLIST_API_TYPE, const ST_WATCHLIST_REQUEST&)>::value,
			"V2 WatchList repository signature changed");
		static_assert(std::is_same<decltype(&CChartConfigRepository::Execute),
			ST_CHART_CONFIG_RET (CChartConfigRepository::*)(
				const ST_CHART_CONFIG_REQUEST&)>::value,
			"V2 Chart Config repository signature changed");
		static_assert(std::is_same<decltype(&CChartDrawingRepository::Execute),
			ST_CHART_DRAWING_RET (CChartDrawingRepository::*)(
				const ST_CHART_DRAWING_REQUEST&)>::value,
			"V2 Chart Drawing repository signature changed");
		static_assert(std::is_same<decltype(&CChartIndicatorRepository::Execute),
			ST_CHART_INDICATOR_RET (CChartIndicatorRepository::*)(
				const ST_CHART_INDICATOR_REQUEST&)>::value,
			"V2 Chart Indicator repository signature changed");
		static_assert(std::is_same<decltype(&TryConvertServerTimeToUtc0),
			bool(*)(int64_t, int64_t, int64_t&)>::value,
			"V2 server seconds to UTC signature changed");
		static_assert(std::is_same<decltype(&TryConvertUtc0ToServerTime),
			bool(*)(int64_t, int64_t, int64_t&)>::value,
			"V2 UTC to server seconds signature changed");
		static_assert(std::is_same<decltype(&TryConvertServerTimeMscToUtc0),
			bool(*)(int64_t, int64_t, int64_t&)>::value,
			"V2 server milliseconds to UTC signature changed");
		static_assert(std::is_same<decltype(&TryConvertUtc0MscToServerTime),
			bool(*)(int64_t, int64_t, int64_t&)>::value,
			"V2 UTC to server milliseconds signature changed");
		static_assert(std::is_same<
			decltype(&ConvertServerSessionTimeToUtc),
			bool(*)(int, int, int64_t,
				ST_SYMBOL_SESSION_UTC_TIME&)>::value,
			"V2 server session time signature changed");
		static_assert(std::is_same<
			decltype(&ConvertServerSessionIntervalToUtc),
			bool(*)(int, int, int, int64_t,
				std::vector<ST_SYMBOL_SESSION_UTC_INTERVAL>&)>::value,
			"V2 server session interval signature changed");
		static_assert(std::is_same<
			decltype(&ConvertClientSymbolsSessionJsonToUtc),
			void(*)(nlohmann::json&, int64_t, int, int)>::value,
			"V2 symbols session JSON signature changed");
		static_assert(std::is_same<
			decltype(&ConvertClientAccountsSymbolsSessionJsonToUtc),
			void(*)(nlohmann::json&, int64_t, int, int)>::value,
			"V2 account symbols session JSON signature changed");
		static_assert(sizeof(ST_SYMBOL_SESSION_UTC_TIME) ==
			sizeof(int) * 2U,
			"V2 symbol session UTC time layout changed");
		static_assert(sizeof(ST_SYMBOL_SESSION_UTC_INTERVAL) ==
			sizeof(int) * 3U,
			"V2 symbol session UTC interval layout changed");
		static_assert(std::is_same<decltype(&GetMt4TradeProfitRate),
			double(*)(double)>::value,
			"V2 MT4 profit rate signature changed");
		static_assert(std::is_same<decltype(&GetMt5PositionProfitRate),
			double(*)(double)>::value,
			"V2 MT5 profit rate signature changed");
		static_assert(sizeof(STLocalRecordFileHeader) == 20U,
			"V2 local file header layout changed");

		int64_t i64UtcSeconds = 0;
		int64_t i64ServerSeconds = 0;
		int64_t i64UtcMilliseconds = 0;
		int64_t i64ServerMilliseconds = 0;
		int64_t i64OverflowResult = 99;
		if (!Check(TryConvertServerTimeToUtc0(1700007200LL, 7200LL,
				i64UtcSeconds) && i64UtcSeconds == 1700000000LL,
				"V2 server seconds to UTC mismatch") ||
			!Check(TryConvertUtc0ToServerTime(1700000000LL, 7200LL,
				i64ServerSeconds) && i64ServerSeconds == 1700007200LL,
				"V2 UTC to server seconds mismatch") ||
			!Check(TryConvertServerTimeMscToUtc0(1700007200123LL,
				7200LL, i64UtcMilliseconds) &&
				i64UtcMilliseconds == 1700000000123LL,
				"V2 server milliseconds to UTC mismatch") ||
			!Check(TryConvertUtc0MscToServerTime(1700000000123LL,
				7200LL, i64ServerMilliseconds) &&
				i64ServerMilliseconds == 1700007200123LL,
				"V2 UTC to server milliseconds mismatch") ||
			!Check(!TryConvertUtc0ToServerTime(
				(std::numeric_limits<int64_t>::max)(), 1LL,
				i64OverflowResult) && i64OverflowResult == 0,
				"V2 seconds overflow handling mismatch") ||
			!Check(!TryConvertServerTimeMscToUtc0(1LL,
				(std::numeric_limits<int64_t>::min)(),
				i64OverflowResult) && i64OverflowResult == 0,
				"V2 milliseconds offset overflow handling mismatch"))
		{
			return false;
		}

		ST_SYMBOL_SESSION_UTC_TIME stUtcSessionTime;
		std::vector<ST_SYMBOL_SESSION_UTC_INTERVAL> aUtcSession;
		if (!Check(ConvertServerSessionTimeToUtc(0, 60, 7200,
				stUtcSessionTime) &&
				stUtcSessionTime.iDayInWeek == 6 &&
				stUtcSessionTime.iMinuteOfDay == 1380,
				"V2 symbol session previous-week conversion mismatch") ||
			!Check(ConvertServerSessionTimeToUtc(6, 1380, -3600,
				stUtcSessionTime) &&
				stUtcSessionTime.iDayInWeek == 0 &&
				stUtcSessionTime.iMinuteOfDay == 0,
				"V2 symbol session next-week conversion mismatch") ||
			!Check(ConvertServerSessionIntervalToUtc(1, 60, 180,
				7200, aUtcSession) && aUtcSession.size() == 2U &&
				aUtcSession[0].iDayInWeek == 0 &&
				aUtcSession[0].iOpenMinute == 1380 &&
				aUtcSession[0].iCloseMinute == 1440 &&
				aUtcSession[1].iDayInWeek == 1 &&
				aUtcSession[1].iOpenMinute == 0 &&
				aUtcSession[1].iCloseMinute == 60,
				"V2 symbol session midnight split mismatch") ||
			!Check(!ConvertServerSessionTimeToUtc(7, 0, 0,
				stUtcSessionTime),
				"V2 symbol session invalid day accepted") ||
			!Check(!ConvertServerSessionIntervalToUtc(0, 0, 0,
				0, aUtcSession),
				"V2 zero-length symbol session accepted") ||
			!Check(!ConvertServerSessionTimeToUtc(0, 0, 86401,
				stUtcSessionTime),
				"V2 symbol session invalid offset accepted"))
		{
			return false;
		}

		nlohmann::json jsonSymbols = nlohmann::json::array(
			{
				{
					{ "SymbolId", "EURUSD" },
					{ "TimeStart", 1700007200LL },
					{ "TimeExpiration", 0LL },
					{ "Session", {
						{ "Trade", nlohmann::json::array({
							{
								{ "DayInWeek", 1 },
								{ "OpenHours", 1 },
								{ "OpenMinutes", 0 },
								{ "CloseHours", 3 },
								{ "CloseMinutes", 0 }
							},
							{
								{ "DayInWeek", 1 },
								{ "OpenHours", 1 },
								{ "OpenMinutes", 0 },
								{ "CloseHours", 3 },
								{ "CloseMinutes", 0 }
							}
						}) },
						{ "Quote", nlohmann::json::array() }
					} }
				}
			});
		ConvertClientSymbolsSessionJsonToUtc(jsonSymbols,
			7200, 5, 1);
		const nlohmann::json& refTradeSession =
			jsonSymbols[0]["Session"]["Trade"];
		if (!Check(jsonSymbols[0]["TimeStart"] == 1700000000LL &&
				jsonSymbols[0]["TimeExpiration"] == 0LL &&
				refTradeSession.size() == 2U &&
				refTradeSession[0]["DayInWeek"] == 0 &&
				refTradeSession[0]["Open"] == 1380 &&
				refTradeSession[0]["Close"] == 1440 &&
				refTradeSession[1]["DayInWeek"] == 1 &&
				refTradeSession[1]["Open"] == 0 &&
				refTradeSession[1]["Close"] == 60,
				"V2 symbols session JSON conversion mismatch"))
		{
			return false;
		}
		nlohmann::json jsonAccountSymbols = nlohmann::json::array(
			{
				{
					{ "Login", 10001 },
					{ "Symbols", jsonSymbols }
				}
			});
		ConvertClientAccountsSymbolsSessionJsonToUtc(
			jsonAccountSymbols, 0, 5, 1);
		if (!Check(jsonAccountSymbols[0]["Symbols"][0]
				["Session"]["Trade"].size() == 2U,
				"V2 account symbols session JSON traversal mismatch"))
		{
			return false;
		}

		STLocalRecordFileHeader stHeader;
		stHeader.uiRecordCount = 9U;
		InitLocalRecordFileHeader(stHeader, 0x12345678U, 3U,
			static_cast<std::uint32_t>(sizeof(stHeader)), 64U);
		if (!Check(stHeader.uiMagic == 0x12345678U &&
				stHeader.uiVersion == 3U && stHeader.uiHeaderSize == 20U &&
				stHeader.uiRecordSize == 64U && stHeader.uiRecordCount == 0U,
				"V2 local record header initialization mismatch") ||
			!Check(IsMt4KlinePeriod(1) && IsMt4KlinePeriod(1440) &&
				!IsMt4KlinePeriod(2) && IsMt5KlinePeriod(2) &&
				!IsMt5KlinePeriod(7),
				"V2 kline period set mismatch") ||
			!Check(MTHS::WstringToString(
					MTHS::StringToWstring("V2兼容")) == "V2兼容",
				"V2 UTF-8 conversion mismatch"))
		{
			return false;
		}

		UserRecord stMt4User = {};
		stMt4User.enable = 1;
		stMt4User.enable_read_only = 0;
		ConGroup stMt4Group = {};
		stMt4Group.enable = 1;
		strcpy_s(stMt4Group.currency, " usd ");
		const ST_ACCOUNT_TRADE_STATUS stMt4Status =
			BuildMt4AccountTradeStatus(&stMt4User, &stMt4Group);
		stMt4User.enable_read_only = 1;
		const ST_ACCOUNT_TRADE_STATUS stMt4ReadOnly =
			BuildMt4AccountTradeStatus(&stMt4User, &stMt4Group);

		MT5User stMt5User = {};
		stMt5User.Login = 90001U;
		stMt5User.Rights = IMTUser::USER_RIGHT_ENABLED;
		MT5ConGroup stMt5Group = {};
		stMt5Group.PermissionsFlags =
			IMTConGroup::PERMISSION_ENABLE_CONNECTION;
		strcpy_s(stMt5Group.Currency, "USDT");
		const ST_ACCOUNT_TRADE_STATUS stMt5Status =
			BuildMt5AccountTradeStatus(&stMt5User, &stMt5Group, false);
		const ST_ACCOUNT_TRADE_STATUS stMt5Archived =
			BuildMt5AccountTradeStatus(&stMt5User, &stMt5Group, true);

		CAccountArchiveStatusStore clArchiveStore;
		BindAccountArchiveStatusStore(&clArchiveStore);
		MarkAccountArchiveStatus(5, 71, 90001U, true);
		const bool bArchived = IsAccountArchived(5, 71, 90001U);
		MarkAccountArchiveStatus(5, 71, 90001U, false);
		const bool bCleared = !IsAccountArchived(5, 71, 90001U);
		BindAccountArchiveStatusStore(nullptr);
		if (!Check(stMt4Status.bCanTrade &&
				stMt4Status.iTradeCode == EN_ACCOUNT_TRADE_CODE_OK &&
				stMt4Status.iAccountType == EN_ACCOUNT_TYPE_USD &&
				!stMt4ReadOnly.bCanTrade &&
				stMt4ReadOnly.iTradeCode == EN_ACCOUNT_TRADE_CODE_READ_ONLY &&
				stMt5Status.bCanTrade &&
				stMt5Status.iAccountType == EN_ACCOUNT_TYPE_USDT &&
				stMt5Archived.iTradeCode == EN_ACCOUNT_TRADE_CODE_ARCHIVED &&
				bArchived && bCleared,
				"V2 account trade status mismatch"))
		{
			return false;
		}

		STQuoteData stQuote;
		stQuote.bHasBid = true;
		stQuote.bHasAsk = true;
		stQuote.dBid = 1.01;
		stQuote.dAsk = 1.02;
		MT5Position stPosition = {};
		stPosition.Action = 0U;
		stPosition.Volume = 10000U;
		stPosition.PriceOpen = 1.0;
		stPosition.ContractSize = 100000.0;
		stPosition.RateProfit = 2.0;
		MT5ConSymbol stMt5Symbol = {};
		stMt5Symbol.ContractSize = 100000.0;
		double dCurrentPrice = 0.0;
		double dProfit = 0.0;
		if (!Check(TryCalculateMt5PositionMarketProfit(stPosition,
					stQuote, &stMt5Symbol, dCurrentPrice, dProfit) &&
				std::fabs(dCurrentPrice - 1.01) < 1.0e-12 &&
				std::fabs(dProfit - 2000.0) < 1.0e-8,
				"V2 MT5 market profit mismatch"))
		{
			return false;
		}

		MT4TradeRecord stTrade = {};
		stTrade.login = 88;
		stTrade.cmd = MT4_OP_BUY;
		stTrade.volume = 100;
		stTrade.open_price = 1.0;
		stTrade.conv_rates[1] = 1.0;
		strcpy_s(stTrade.symbol, "EURUSD");
		MT4ConSymbol stMt4Symbol = {};
		stMt4Symbol.contract_size = 100000.0;
		double dMt4Price = 0.0;
		double dMt4Profit = 0.0;
		if (!Check(TryCalculateMt4TradeMarketProfit(stTrade, stMt4Symbol,
					stQuote, dMt4Price, dMt4Profit) &&
				std::fabs(dMt4Profit - 1000.0) < 1.0e-8,
				"V2 MT4 fallback profit mismatch"))
		{
			return false;
		}
		stTrade.close_price = 1.005;
		stTrade.profit = 500.0;
		if (!Check(TryCalculateMt4TradeMarketProfit(stTrade, stMt4Symbol,
					stQuote, dMt4Price, dMt4Profit) &&
				std::fabs(dMt4Profit - 1000.0) < 1.0e-8,
				"V2 MT4 anchored profit mismatch"))
		{
			return false;
		}

		CV2Mt4ProfitTestDataSource clDataSource;
		clDataSource.aTrade.push_back(stTrade);
		clDataSource.stQuote = stQuote;
		clDataSource.stSymbol = stMt4Symbol;
		BindV2Mt4PositionProfitDataSource(&clDataSource);
		MT4MarginLevel stMargin = {};
		stMargin.login = 88;
		ST_MT4_ACCOUNT_FLOATING_SUMMARY stSummary;
		const bool bStored = CacheMt4MarginLevelWithCurrentProfitAnchor(
			71, stMargin, &stSummary);
		BindV2Mt4PositionProfitDataSource(nullptr);
		MT5SymbolSessionInfo stSessionInfo;
		const bool bNullSessionRejected = !LoadMt5SymbolSessionsFromApi(
			nullptr, nullptr, stSessionInfo);
		return Check(bStored && clDataSource.bHasAnchor &&
			stSummary.szPositionCount == 1U && stSummary.iVolume == 100 &&
			std::fabs(clDataSource.dAnchor - 1000.0) < 1.0e-8 &&
			bNullSessionRejected && !HasMt5SymbolSessionData(stSessionInfo),
			"V2 owner adapter or MT5 session failure path mismatch");
	}

	// 验证 V2 同名 MT5 组报价组件的需求、样本、双候选、过期、繁忙、依赖核对和节点清理语义。
	bool TestMt5ProfitGroupQuoteService()
	{
		if (!Check(std::string(GetPluginFuncName(
				EN_PLUGIN_FUNC_QUERY_MT5_PROFIT_GROUP_QUOTE)) ==
				"QUERY_MT5_PROFIT_GROUP_QUOTE",
			"1169 MT5 group quote protocol name mismatch"))
		{
			return false;
		}
		CMt5ProfitGroupQuoteService& refService =
			CMt5ProfitGroupQuoteService::GetInstance();
		refService.Stop();
		ST_MT5_PROFIT_GROUP_QUOTE_NODE_CONFIG stConfig;
		stConfig.iNo = 701;
		stConfig.bEnabled = true;
		stConfig.uiRefreshMs = 100U;
		stConfig.uiStaleMs = 600U;
		stConfig.uiExpireMs = 1600U;
		stConfig.uiMaxQueriesPerSecond = 20U;
		std::string strError;
		if (!Check(refService.Configure({ stConfig },
				strError), strError.c_str()))
		{
			return false;
		}

		std::atomic<unsigned int> uiQueryCount(0U);
		std::atomic<unsigned int> uiChangedCount(0U);
		std::atomic<bool> bPoolBusy(false);
		std::atomic<bool> bDependencyValid(true);
		refService.Start(
			[&refService, &uiQueryCount, &bPoolBusy](
				const ST_MT5_PROFIT_GROUP_QUOTE_KEY& p_refKey,
				ST_MT5_PROFIT_GROUP_QUOTE_QUERY_RESULT& p_refResult)
			{
				p_refResult =
					ST_MT5_PROFIT_GROUP_QUOTE_QUERY_RESULT();
				if (bPoolBusy.load())
				{
					p_refResult.bQueryPoolBusy = true;
					p_refResult.strError =
						"EXPECTED_MANAGER_CONNECTION_BUSY";
					return false;
				}
				const unsigned int uiCall =
					uiQueryCount.fetch_add(1U) + 1U;
				const std::int64_t llTickMsc =
					1710000000000LL + uiCall;
				ST_REALTIME_TICK stTick;
				stTick.iVersion = 5;
				stTick.iNo = p_refKey.iNo;
				strncpy_s(stTick.szSymbol,
					sizeof(stTick.szSymbol),
					p_refKey.strSymbol.c_str(), _TRUNCATE);
				stTick.dBid = 1.2000;
				stTick.dAsk = 1.2002;
				stTick.i64MarketTimeMsc = llTickMsc;
				refService.OnRealtimeTick(stTick);
				p_refResult.dGlobalBid = stTick.dBid;
				p_refResult.dGlobalAsk = stTick.dAsk;
				p_refResult.dGroupBid = 1.2002;
				p_refResult.dGroupAsk = 1.2005;
				p_refResult.i64GlobalTickMsc = llTickMsc;
				p_refResult.i64GroupTickMsc = llTickMsc;
				p_refResult.iApiCallCount = 2;
				p_refResult.iRetCode = 0;
				p_refResult.ui64TimeAuthorityEpoch = 91U;
				p_refResult.ui64TimeGeneration = 7U;
				return true;
			},
			[&uiChangedCount](const std::vector<
				ST_MT5_PROFIT_GROUP_QUOTE_DEPENDENCY>& p_refDependency)
			{
				if (!p_refDependency.empty())
				{
					uiChangedCount.fetch_add(1U);
				}
			},
			[&bDependencyValid](
				const ST_MT5_PROFIT_GROUP_QUOTE_DEPENDENCY&)
			{
				return bDependencyValid.load();
			});

		ST_MT5_PROFIT_GROUP_QUOTE_REQUEST stRequest;
		stRequest.stKey.iNo = 701;
		stRequest.stKey.strGroup = "real\\test";
		stRequest.stKey.strSymbol = "EURUSD";
		stRequest.i64Login = 90001;
		stRequest.dPoint = 0.00001;
		stRequest.uiDigits = 5U;
		STQuoteData stRawQuote;
		stRawQuote.iVersion = 5;
		stRawQuote.iNo = 701;
		stRawQuote.strSymbolId = "EURUSD";
		stRawQuote.dBid = 1.2000;
		stRawQuote.dAsk = 1.2002;
		stRawQuote.bHasBid = true;
		stRawQuote.bHasAsk = true;
		STQuoteData stEffectiveQuote;
		const bool bInitialFallback =
			!refService.ResolveEffectiveQuote(stRequest,
				stRawQuote, stEffectiveQuote);

		// 两个不同 Tick 的一致候选均命中实时样本后，才允许发布组报价偏移。
		bool bReady = false;
		for (unsigned int uiWait = 0; uiWait < 250U; ++uiWait)
		{
			if (refService.ResolveEffectiveQuote(stRequest,
					stRawQuote, stEffectiveQuote))
			{
				bReady = true;
				break;
			}
			std::this_thread::sleep_for(
				std::chrono::milliseconds(20));
		}
		const bool bEffectivePrice = bReady &&
			std::fabs(stEffectiveQuote.dBid - 1.2002) < 1.0e-9 &&
			std::fabs(stEffectiveQuote.dAsk - 1.2005) < 1.0e-9;

		// 发布后切换为连接池繁忙，旧值先进入 stale，再硬过期并回退服务器快照。
		bPoolBusy.store(true);
		bool bStaleObserved = false;
		for (unsigned int uiWait = 0; uiWait < 120U; ++uiWait)
		{
			ST_MT5_PROFIT_GROUP_QUOTE_STATS stStats;
			refService.GetStats(stStats);
			if (stStats.szStaleCount > 0U)
			{
				bStaleObserved = true;
				break;
			}
			std::this_thread::sleep_for(
				std::chrono::milliseconds(20));
		}
		bool bExpiredFallback = false;
		for (unsigned int uiWait = 0; uiWait < 100U; ++uiWait)
		{
			if (!refService.ResolveEffectiveQuote(stRequest,
					stRawQuote, stEffectiveQuote))
			{
				ST_MT5_PROFIT_GROUP_QUOTE_STATS stStats;
				refService.GetStats(stStats);
				if (stStats.szExpiredCount > 0U)
				{
					bExpiredFallback = true;
					break;
				}
			}
			std::this_thread::sleep_for(
				std::chrono::milliseconds(20));
		}

		// 权威依赖失效后必须删除需求；重新登记后 ClearNode 必须清空该 No 的全部状态。
		bDependencyValid.store(false);
		refService.RequestReconcile();
		bool bReconciled = false;
		for (unsigned int uiWait = 0; uiWait < 100U; ++uiWait)
		{
			ST_MT5_PROFIT_GROUP_QUOTE_STATS stStats;
			refService.GetStats(stStats);
			if (stStats.szActiveKeyCount == 0U &&
				stStats.szDependencyCount == 0U)
			{
				bReconciled = true;
				break;
			}
			std::this_thread::sleep_for(
				std::chrono::milliseconds(20));
		}
		bDependencyValid.store(true);
		refService.ResolveEffectiveQuote(stRequest,
			stRawQuote, stEffectiveQuote);
		refService.ClearNode(701, "protocol test");
		ST_MT5_PROFIT_GROUP_QUOTE_STATS stFinalStats;
		refService.GetStats(stFinalStats);
		refService.Stop();
		return Check(bInitialFallback && bEffectivePrice &&
			uiQueryCount.load() >= 2U &&
			uiChangedCount.load() >= 1U &&
			bStaleObserved && bExpiredFallback && bReconciled &&
			stFinalStats.szActiveKeyCount == 0U &&
			stFinalStats.szDependencyCount == 0U &&
			stFinalStats.ui64NoConnectionCount > 0U,
			"MT5 group quote calibration lifecycle mismatch");
	}

	class CMarketStateTestRuntime : public IMarketStateRuntime
	{
	public:
		bool GetQuoteTime(int, int, const std::string&,
			std::int64_t& p_refTime) const override
		{
			p_refTime = static_cast<std::int64_t>(std::time(nullptr));
			return true;
		}
		bool IsTradeSessionOpen(int, int, const std::string&,
			std::int64_t, std::int64_t, bool& p_refOpen) const override
		{
			p_refOpen = true;
			return true;
		}
		bool GetSymbolPath(int, int, const std::string&,
			std::string& p_refPath) const override
		{
			p_refPath = "Forex\\Majors";
			return true;
		}
		bool TryGetServerTimeOffset(int, int,
			std::int64_t& p_refOffset) const override
		{
			p_refOffset = 0;
			return true;
		}
		int GetQuoteExpireSeconds(int, int) const override
		{
			return 60;
		}
	};

	bool TestMarketStatePersistence()
	{
		const std::filesystem::path clRoot =
			std::filesystem::temp_directory_path() /
			"MtGatewayServiceProtocolTests_MarketState";
		CTestDirectoryGuard clGuard(clRoot);
		CMarketStateTestRuntime clRuntime;
		BindMarketStateRuntime(&clRuntime);
		CMarketStateCache& refCache = GetMarketStateCache();
		refCache.Destroy();
		if (!Check(refCache.Initialize(clRoot.string()),
				"market state initialize failed") ||
			!Check(refCache.PreloadServer(5, 98701),
				"market state initial preload failed"))
		{
			BindMarketStateRuntime(nullptr);
			return false;
		}

		const std::int64_t llNow = static_cast<std::int64_t>(std::time(nullptr));
		std::time_t stNow = static_cast<std::time_t>(llNow);
		std::tm stUtc = {};
		gmtime_s(&stUtc, &stNow);
		ST_MARKET_HOLIDAY_RECORD stHoliday;
		stHoliday.strHolidayId = "protocol-test-holiday";
		stHoliday.bEnabled = true;
		stHoliday.strDescription = "protocol persistence test";
		stHoliday.bRecurringYearly = true;
		stHoliday.iMonth = stUtc.tm_mon + 1;
		stHoliday.iDay = stUtc.tm_mday;
		stHoliday.iWorkFrom = 0;
		stHoliday.iWorkTo = 0;
		stHoliday.vecSymbolRules = { "EURUSD" };
		stHoliday.enRuleMatchType = MARKET_HOLIDAY_RULE_SYMBOL;

		ST_SYMBOL_SUSPENSION_CONFIG stSuspension;
		stSuspension.iVersion = 5;
		stSuspension.iNo = 98701;
		stSuspension.strSymbolId = "EURUSD";
		stSuspension.i64RawTradeMode = 4;
		stSuspension.bRealtime = true;

		if (!Check(refCache.ReplaceHolidaySnapshot(5, 98701,
				{ stHoliday }, MARKET_STATE_SYNC_MT5_FULL, true),
				"market holiday replace failed") ||
			!Check(refCache.ReplaceSuspensionSnapshot(5, 98701,
				{ stSuspension }, MARKET_STATE_SYNC_MT5_FULL, true),
				"market suspension replace failed"))
		{
			refCache.Destroy();
			BindMarketStateRuntime(nullptr);
			return false;
		}

		std::vector<ST_MARKET_HOLIDAY_RECORD> aHoliday;
		std::vector<ST_SYMBOL_SUSPENSION_CONFIG> aSuspension;
		ST_MARKET_STATE_SNAPSHOT_META stHolidayMeta;
		ST_MARKET_STATE_SNAPSHOT_META stSuspensionMeta;
		if (!Check(refCache.GetHolidaySnapshot(5, 98701, aHoliday,
				stHolidayMeta, true) && aHoliday.size() == 1U &&
				stHolidayMeta.bLiveValidated,
				"live holiday snapshot mismatch") ||
			!Check(refCache.GetSuspensionSnapshot(5, 98701,
				aSuspension, stSuspensionMeta) && aSuspension.size() == 1U &&
				stSuspensionMeta.bLiveValidated,
				"live suspension snapshot mismatch"))
		{
			refCache.Destroy();
			BindMarketStateRuntime(nullptr);
			return false;
		}
		const std::uint64_t ullHolidayGeneration = stHolidayMeta.ui64Generation;
		const std::uint64_t ullSuspensionGeneration = stSuspensionMeta.ui64Generation;
		refCache.Destroy();

		const bool bReloaded = refCache.Initialize(clRoot.string()) &&
			refCache.PreloadServer(5, 98701) &&
			refCache.GetHolidaySnapshot(5, 98701, aHoliday, stHolidayMeta, false) &&
			refCache.GetSuspensionSnapshot(5, 98701, aSuspension, stSuspensionMeta);
		const bool bSafeDiskState = bReloaded &&
			stHolidayMeta.ui64Generation == ullHolidayGeneration &&
			stSuspensionMeta.ui64Generation == ullSuspensionGeneration &&
			stHolidayMeta.bLoadedFromDisk && stSuspensionMeta.bLoadedFromDisk &&
			!stHolidayMeta.bLiveValidated && !stSuspensionMeta.bLiveValidated &&
			stHolidayMeta.bIsStale && stSuspensionMeta.bIsStale &&
			!refCache.GetHolidaySnapshot(5, 98701, aHoliday,
				stHolidayMeta, true);
		refCache.Destroy();
		BindMarketStateRuntime(nullptr);
		return Check(bSafeDiskState,
			"disk market state must reload as stale and not live validated");
	}

int main()
{
	// 按 API、领域协议和事件协议顺序执行，任一失败立即返回非零。
	if (!RunTest("V2ErrorCompatibilityMessages",
			TestV2ErrorCompatibilityMessages) ||
		!RunTest("V2CompatibilityCommonUtilities",
			TestV2CompatibilityCommonUtilities) ||
		!RunTest("ApiCatalog", TestApiCatalog) ||
		!RunTest("GatewaySubscriptionIndex",
			TestGatewaySubscriptionIndex) ||
		!RunTest("GatewaySharedPushPayload",
			TestGatewaySharedPushPayload) ||
		!RunTest("GatewayLatestQuoteBatchReplacement",
			TestGatewayLatestQuoteBatchReplacement) ||
		!RunTest("GatewaySnapshotBarrierAndRetry",
			TestGatewaySnapshotBarrierAndRetry) ||
		!RunTest("GatewayWebKlineTopicContract",
			TestGatewayWebKlineTopicContract) ||
		!RunTest("GatewayClosePositionRequestShape",
			TestGatewayClosePositionRequestShape) ||
		!RunTest("GatewayCompatibilityContracts",
			TestGatewayCompatibilityContracts) ||
		!RunTest("BinaryDomains", TestBinaryDomains) ||
		!RunTest("TradeProtocolRegistry",
			TestTradeProtocolRegistry) ||
		!RunTest("MarketStatePersistence",
			TestMarketStatePersistence) ||
		!RunTest("ReliableEventProtocol",
			TestReliableEventProtocol) ||
		!RunTest("TradeIdempotencyPersistence",
			TestTradeIdempotencyPersistence) ||
		!RunTest("TradeOutboxRecovery",
			TestTradeOutboxRecovery) ||
		!RunTest("MtDealerStatePersistence",
			TestMtDealerStatePersistence) ||
		!RunTest("TradeNodeRouting",
			TestTradeNodeRouting) ||
		!RunTest("TradeDispatcherPartialBatch",
			TestTradeDispatcherPartialBatch) ||
		!RunTest("TradeIdempotencyLockShards",
			TestTradeIdempotencyLockShards) ||
		!RunTest("MalformedContainerCounts",
			TestMalformedContainerCounts) ||
		!RunTest("ServiceConnectionConfigs",
			TestServiceConnectionConfigs) ||
		!RunTest("QuotePhaseOneConfigBoundaries",
			TestQuotePhaseOneConfigBoundaries) ||
		!RunTest("EventPhaseOneConfigBoundaries",
			TestEventPhaseOneConfigBoundaries) ||
		!RunTest("QuoteIngressQueuePolicy",
			TestQuoteIngressQueuePolicy) ||
		!RunTest("QueryTimeBootstrapDecision",
			TestQueryTimeBootstrapDecision) ||
		!RunTest("MtTimeProtocol", TestMtTimeProtocol) ||
		!RunTest("ClusterStateProtocol", TestClusterStateProtocol) ||
		!RunTest("GatewayOwnerAwareRouting",
			TestGatewayOwnerAwareRouting) ||
		!RunTest("TransportControlNotifyIds",
			TestTransportControlNotifyIds) ||
		!RunTest("QuoteClientDataEvent",
			TestQuoteClientDataEvent) ||
		!RunTest("QuoteBinaryV1", TestQuoteBinaryV1) ||
		!RunTest("QuoteHeartbeatProtocol",
			TestQuoteHeartbeatProtocol) ||
		!RunTest("EventHeartbeatProtocol",
			TestEventHeartbeatProtocol) ||
		!RunTest("EventMarketRelay",
			TestEventMarketRelay) ||
		!RunTest("QuoteSnapshotProtocol",
			TestQuoteSnapshotProtocol) ||
		!RunTest("QueryQuoteBootstrap",
			TestQueryQuoteBootstrap) ||
		!RunTest("QueryServerInfoCompatibility",
			TestQueryServerInfoCompatibility) ||
		!RunTest("QueryTimeZoneDst",
			TestQueryTimeZoneDst) ||
		!RunTest("QueryBarsHistoryClosure",
			TestQueryBarsHistoryClosure) ||
		!RunTest("QueryM1ArchiveConsumer",
			TestQueryM1ArchiveConsumer) ||
		!RunTest("DeriveProtocol",
			TestDeriveProtocol) ||
		!RunTest("DeriveMinuteWorkspaceArchive",
			TestDeriveMinuteWorkspaceArchive) ||
		!RunTest("DeriveRuntime",
			TestDeriveRuntime) ||
		!RunTest("Mt5ProfitGroupQuoteService",
			TestMt5ProfitGroupQuoteService) ||
		!RunTest("DeriveReliableStateBootstrap",
			TestDeriveReliableStateBootstrap) ||
		!RunTest("DeriveDeadLetterOperations",
			TestDeriveDeadLetterOperations) ||
		!RunTest("QueryDeriveStateSnapshot",
			TestQueryDeriveStateSnapshot) ||
		!RunTest("ObjectClientDataEvent",
			TestObjectClientDataEvent) ||
		!RunTest("ReliableTradeClientDataEvent",
			TestReliableTradeClientDataEvent) ||
		!RunTest("QueryClientDataBoundary",
			TestQueryClientDataBoundary))
	{
		return 1;
	}
	std::printf("ALL_TESTS_PASSED\n");
	return 0;
}
