#include "MtDeriveServiceApp.h"

#include "CodeMsg.h"
#include "DeriveBinaryProtocol.h"
#include "Log.h"
#include "QueryBinaryProtocol.h"
#include "ReliableEventBinaryProtocol.h"
#include "TradeBinaryProtocol.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>
#include <map>
#include <set>
#include <tuple>
#include <vector>

namespace
{
	// 快照与增量按 Version+No 固定到同一提交锁，避免跨来源使用单把全局热锁。
	std::size_t SelectNotifyMutex(std::uint16_t p_usVersion,
		std::int32_t p_iNo)
	{
		const std::uint64_t ullKey =
			static_cast<std::uint64_t>(p_usVersion) * 1315423911ULL ^
			static_cast<std::uint32_t>(p_iNo);
		return static_cast<std::size_t>(ullKey % 64ULL);
	}

	static const char* MT_DERIVE_QUERY_CONNECTION =
		"MtQueryService";
	static const std::uint64_t MT_DERIVE_SNAPSHOT_PAGE_SIZE =
		1000U;
	static const std::size_t MT_DERIVE_SNAPSHOT_MAX_PAGES =
		100000U;

	// 创建派生恢复请求使用的受限 Binary 值；分配失败时返回详细英文错误。
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE> MakeValue(
		EN_PLUGIN_BINARY_VALUE_TYPE p_enType,
		std::string& p_refError)
	{
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refValue =
			CreatePluginBinaryValue(p_enType);
		if (!refValue)
		{
			p_refError =
				"DERIVE_SNAPSHOT_MEMORY_ERROR: failed to allocate Binary value";
		}
		return refValue;
	}

	// 向 Binary 对象追加已分配字段；字段和值均由对象树共享所有权。
	bool AddField(ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName,
		const std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& p_refValue,
		std::string& p_refError)
	{
		if (p_refObject.enType !=
				EN_PLUGIN_BINARY_VALUE_OBJECT ||
			p_szName == nullptr || p_szName[0] == 0 ||
			!p_refValue)
		{
			p_refError =
				"DERIVE_SNAPSHOT_FIELD_INVALID: object, name and value are required";
			return false;
		}
		ST_PLUGIN_BINARY_FIELD stField;
		stField.strName = p_szName;
		stField.refValue = p_refValue;
		p_refObject.aObjectField.push_back(stField);
		return true;
	}

	// 向 Binary 对象追加有符号整数。
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

	// 向 Binary 对象追加无符号整数。
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

	// 向内部 Query Binary 对象追加 UTF-8 字符串；字段和值由文档树共同持有。
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

	// 从响应对象读取有符号整数，兼容不超过 INT64_MAX 的无符号编码。
	bool ReadInt64(const ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName, std::int64_t& p_refValue,
		std::string& p_refError)
	{
		const ST_PLUGIN_BINARY_VALUE* pValue =
			FindPluginBinaryField(p_refObject, p_szName);
		if (pValue != nullptr &&
			pValue->enType ==
				EN_PLUGIN_BINARY_VALUE_INT64)
		{
			p_refValue = pValue->llIntValue;
			return true;
		}
		if (pValue != nullptr &&
			pValue->enType ==
				EN_PLUGIN_BINARY_VALUE_UINT64 &&
			pValue->ullUIntValue <=
				static_cast<std::uint64_t>(
					(std::numeric_limits<std::int64_t>::max)()))
		{
			p_refValue = static_cast<std::int64_t>(
				pValue->ullUIntValue);
			return true;
		}
		p_refError =
			std::string("DERIVE_SNAPSHOT_FIELD_INVALID: field=") +
			(p_szName != nullptr ? p_szName : "<null>") +
			", expected=int64";
		return false;
	}

	// 从响应对象读取无符号整数，兼容非负有符号编码。
	bool ReadUInt64(const ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName, std::uint64_t& p_refValue,
		std::string& p_refError)
	{
		const ST_PLUGIN_BINARY_VALUE* pValue =
			FindPluginBinaryField(p_refObject, p_szName);
		if (pValue != nullptr &&
			pValue->enType ==
				EN_PLUGIN_BINARY_VALUE_UINT64)
		{
			p_refValue = pValue->ullUIntValue;
			return true;
		}
		if (pValue != nullptr &&
			pValue->enType ==
				EN_PLUGIN_BINARY_VALUE_INT64 &&
			pValue->llIntValue >= 0)
		{
			p_refValue = static_cast<std::uint64_t>(
				pValue->llIntValue);
			return true;
		}
		p_refError =
			std::string("DERIVE_SNAPSHOT_FIELD_INVALID: field=") +
			(p_szName != nullptr ? p_szName : "<null>") +
			", expected=uint64";
		return false;
	}

	// 从内部 Query 响应读取有限浮点报价；禁止整数隐式转为价格。
	bool ReadDouble(const ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName, double& p_refValue,
		std::string& p_refError)
	{
		const ST_PLUGIN_BINARY_VALUE* pValue =
			FindPluginBinaryField(p_refObject, p_szName);
		if (pValue == nullptr ||
			pValue->enType != EN_PLUGIN_BINARY_VALUE_DOUBLE ||
			!std::isfinite(pValue->dDoubleValue))
		{
			p_refError =
				std::string("DERIVE_GROUP_QUOTE_FIELD_INVALID: field=") +
				(p_szName == nullptr ? "<null>" : p_szName) +
				", expected=finite double";
			return false;
		}
		p_refValue = pValue->dDoubleValue;
		return true;
	}

	// 从可靠事件对象读取非空字符串；只用于状态副作用路由，不保存字段树指针。
	bool ReadString(const ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName, std::string& p_refValue,
		std::string& p_refError)
	{
		const ST_PLUGIN_BINARY_VALUE* pValue =
			FindPluginBinaryField(p_refObject, p_szName);
		if (pValue == nullptr ||
			pValue->enType != EN_PLUGIN_BINARY_VALUE_STRING ||
			pValue->strStringValue.empty())
		{
			p_refError =
				std::string("DERIVE_GROUP_QUOTE_STATE_FIELD_INVALID: field=") +
				(p_szName != nullptr ? p_szName : "<null>") +
				", expected=non-empty string";
			return false;
		}
		p_refValue = pValue->strStringValue;
		return true;
	}

	// 从可靠事件信封复制正文；复制后的缓冲由当前调用栈拥有，可安全在解码器返回后使用。
	bool ReadBytes(const ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName,
		std::vector<unsigned char>& p_refValue,
		std::string& p_refError)
	{
		const ST_PLUGIN_BINARY_VALUE* pValue =
			FindPluginBinaryField(p_refObject, p_szName);
		if (pValue == nullptr ||
			pValue->enType != EN_PLUGIN_BINARY_VALUE_BYTES ||
			pValue->aByteValue.empty())
		{
			p_refError =
				std::string("DERIVE_GROUP_QUOTE_STATE_FIELD_INVALID: field=") +
				(p_szName != nullptr ? p_szName : "<null>") +
				", expected=non-empty bytes";
			return false;
		}
		p_refValue = pValue->aByteValue;
		return true;
	}

	// 从响应对象读取严格布尔字段，不接受整数替代。
	bool ReadBool(const ST_PLUGIN_BINARY_VALUE& p_refObject,
		const char* p_szName, bool& p_refValue,
		std::string& p_refError)
	{
		const ST_PLUGIN_BINARY_VALUE* pValue =
			FindPluginBinaryField(p_refObject, p_szName);
		if (pValue == nullptr ||
			pValue->enType !=
				EN_PLUGIN_BINARY_VALUE_BOOL ||
			pValue->ucBoolValue > 1U)
		{
			p_refError =
				std::string("DERIVE_SNAPSHOT_FIELD_INVALID: field=") +
				(p_szName != nullptr ? p_szName : "<null>") +
				", expected=bool";
			return false;
		}
		p_refValue = pValue->ucBoolValue != 0U;
		return true;
	}

	// 将后续页的五类实体追加到首个响应对象，聚合对象持有所有字段树的共享所有权。
	bool AppendSnapshotPage(
		ST_PLUGIN_BINARY_VALUE& p_refAggregate,
		const ST_PLUGIN_BINARY_VALUE& p_refPage,
		std::string& p_refError)
	{
		static const char* s_aField[] =
		{
			"Accounts", "Positions", "Symbols",
			"Groups", "Rates"
		};
		for (const char* pField : s_aField)
		{
			ST_PLUGIN_BINARY_VALUE* pTarget =
				const_cast<ST_PLUGIN_BINARY_VALUE*>(
					FindPluginBinaryField(
						p_refAggregate, pField));
			const ST_PLUGIN_BINARY_VALUE* pSource =
				FindPluginBinaryField(p_refPage,
					pField);
			if (pTarget == nullptr || pSource == nullptr ||
				pTarget->enType !=
					EN_PLUGIN_BINARY_VALUE_ARRAY ||
				pSource->enType !=
					EN_PLUGIN_BINARY_VALUE_ARRAY)
			{
				p_refError =
					std::string(
						"DERIVE_SNAPSHOT_ARRAY_INVALID: field=") +
					pField + ", expected=array";
				return false;
			}
			pTarget->aArrayValue.insert(
				pTarget->aArrayValue.end(),
				pSource->aArrayValue.begin(),
				pSource->aArrayValue.end());
		}
		return true;
	}
}

CMtDeriveServiceApp::CMtDeriveServiceApp()
	: CServiceApplication(EN_PLUGIN_ID_MT_DERIVE_SERVICE,
		"MtDeriveService")
	, m_stDeriveConfig()
	, m_clM1Engine()
	, m_clM1Archive()
	, m_clTickStore()
	, m_clTickConsumer()
	, m_clProfitLease()
	, m_clStateStore()
	, m_clGroupQuoteService()
	, m_clProfitEngine()
	, m_clReliableConsumer()
	, m_bStarted(false)
	, m_bBootstrapStopping(false)
	, m_clBootstrapMutex()
	, m_clBootstrapCondition()
	, m_clBootstrapThread()
	, m_clM1PublishMutex()
	, m_clM1PublishCondition()
	, m_clM1PublishThread()
	, m_bM1PublishStopping(false)
	, m_mapPendingM1()
	, m_mapLastM1Minute()
	, m_aProfitSnapshotMutex()
	, m_aM1SnapshotMutex()
	, m_ullProfitNotifySequence(0)
	, m_ullM1NotifySequence(0)
{
	// 构造阶段只初始化值对象，恢复文件和网络资源由公共生命周期按顺序创建。
}

CMtDeriveServiceApp::~CMtDeriveServiceApp()
{
	OnStopping();
}

bool CMtDeriveServiceApp::OnConfigure(
	std::string& p_refError)
{
	m_bStarted.store(false);
	m_bBootstrapStopping.store(false);
	m_bM1PublishStopping = false;
	m_mapPendingM1.clear();
	m_mapLastM1Minute.clear();
	m_ullProfitNotifySequence.store(0);
	m_ullM1NotifySequence.store(0);

	// 第一步：读取实例、来源、WAL、M1、收益租约和可靠消费者参数。
	if (!LoadMtDeriveServiceConfig(GetConfigPath(),
			GetExeDirectory(), m_stDeriveConfig,
			p_refError))
	{
		return false;
	}
	for (const ST_MT_DERIVE_SOURCE_CONFIG& refSource : m_stDeriveConfig.aSource)
	{
		if (refSource.bEnable && !RegisterClusterShard(
				refSource.usVersion, refSource.iNo, p_refError))
		{
			return false;
		}
	}

	// 第二步：先建立空 M1 来源槽，再恢复 TickStore 检查点和 WAL。
	// 恢复完成前 Ice 网络尚未启动，因此不会与 1182 请求并发。
	if (!m_clM1Engine.Configure(m_stDeriveConfig,
			p_refError))
	{
		return false;
	}
	if (!m_clTickStore.Configure(m_stDeriveConfig,
			&m_clM1Engine, p_refError))
	{
		m_clM1Engine.Clear();
		return false;
	}

	// 第三步：租约表不从磁盘恢复，Gateway 启动后必须用绝对集合重新登记。
	if (!m_clProfitLease.Configure(
			m_stDeriveConfig.stProfit, p_refError))
	{
		m_clTickStore.Stop();
		m_clM1Engine.Clear();
		return false;
	}
	if (!m_clStateStore.Configure(
			m_stDeriveConfig.aSource,
			m_stDeriveConfig.stReliable.
				szBootstrapBufferCapacity,
			p_refError))
	{
		m_clProfitLease.Clear();
		m_clTickStore.Stop();
		m_clM1Engine.Clear();
		return false;
	}
	std::vector<ST_MT5_PROFIT_GROUP_QUOTE_NODE_CONFIG>
		aGroupQuoteConfig;
	for (const ST_MT_DERIVE_SOURCE_CONFIG& refSource :
		m_stDeriveConfig.aSource)
	{
		if (!refSource.bEnable || refSource.usVersion != 5U)
		{
			continue;
		}
		ST_MT5_PROFIT_GROUP_QUOTE_NODE_CONFIG stConfig;
		stConfig.iNo = refSource.iNo;
		stConfig.bEnabled = refSource.bGroupQuoteEnabled;
		stConfig.uiRefreshMs = refSource.uiGroupQuoteRefreshMs;
		stConfig.uiStaleMs = refSource.uiGroupQuoteStaleMs;
		stConfig.uiExpireMs = refSource.uiGroupQuoteExpireMs;
		stConfig.uiMaxQueriesPerSecond =
			refSource.uiGroupQuoteMaxQps;
		aGroupQuoteConfig.push_back(stConfig);
	}
	if (!m_clGroupQuoteService.Configure(
			aGroupQuoteConfig, p_refError))
	{
		m_clStateStore.Clear();
		m_clProfitLease.Clear();
		m_clTickStore.Stop();
		m_clM1Engine.Clear();
		return false;
	}
	if (!m_clProfitEngine.Configure(
			m_stDeriveConfig.stProfit,
			m_stDeriveConfig.aSource,
			&m_clStateStore, &m_clProfitLease,
			&m_clGroupQuoteService,
			p_refError))
	{
		m_clStateStore.Clear();
		m_clProfitLease.Clear();
		m_clTickStore.Stop();
		m_clM1Engine.Clear();
		return false;
	}
	MT_INFO(
		"derive config loaded,instance=%s,sourceCount=%zu,cache=%s,reliableEnabled=%d",
		m_stDeriveConfig.strInstanceId.c_str(),
		m_stDeriveConfig.aSource.size(),
		m_stDeriveConfig.stTick.strCachePath.c_str(),
		m_stDeriveConfig.stReliable.bEnable ? 1 : 0);
	return true;
}

bool CMtDeriveServiceApp::OnStarted(
	std::string& p_refError)
{
	p_refError.clear();
	// 第一步：无损 Tick durable 必须先于收益和状态恢复启动；Standby 只建连接不 Fetch。
	if (!m_clTickConsumer.Start(
			m_stDeriveConfig.stTickStream,
			m_stDeriveConfig.aSource,
			[this](std::uint16_t p_usVersion, std::int32_t p_iNo)
			{
				return HasClusterLease(p_usVersion, p_iNo);
			},
			[this](const ST_QUOTE_BINARY_TICK& p_refTick,
				std::string& p_refApplyError)
			{
				return ApplyTickStreamTick(p_refTick, p_refApplyError);
			}, p_refError))
	{
		return false;
	}
	if (!m_clM1Archive.Start(m_stDeriveConfig.stM1Archive,
			&m_clM1Engine,
			[this](std::uint16_t p_usVersion, std::int32_t p_iNo)
			{
				return HasClusterLease(p_usVersion, p_iNo);
			},
			[this](std::uint16_t p_usVersion, std::int32_t p_iNo,
				ST_CLUSTER_FENCE& p_refFence, std::string& p_refFenceError)
			{
				return GetClusterFence(p_usVersion, p_iNo,
					p_refFence, p_refFenceError);
			},
			[this](const ST_QUERY_M1_BACKFILL_REQUEST& p_refRequest,
				ST_QUERY_M1_BACKFILL_RESPONSE& p_refResponse,
				std::string& p_refBackfillError)
			{
				return CallM1Backfill(p_refRequest, p_refResponse,
					p_refBackfillError);
			}, p_refError))
	{
		m_clTickConsumer.Stop();
		return false;
	}

	// 第二步：可靠消费者必须先于快照线程启动，确保快照生成期间的增量已经进入有界缓冲。
	if (!m_clReliableConsumer.Start(
			GetCloudNetApi(),
			m_stDeriveConfig.stReliable,
			[this](std::uint64_t p_ullSequence,
				const std::vector<unsigned char>& p_refEnvelope,
				std::string& p_refApplyError)
			{
				return ApplyReliableEvent(
					p_ullSequence, p_refEnvelope,
					p_refApplyError);
			},
			[this](std::uint64_t p_ullSequence,
				const std::vector<unsigned char>& p_refEnvelope,
				bool p_bAllowStale, bool& p_refApplied,
				std::string& p_refReplayError)
			{
				return m_clStateStore.ReplayReliableEnvelope(
					p_ullSequence, p_refEnvelope,
					p_bAllowStale, p_refApplied,
					p_refReplayError);
			},
			p_refError))
	{
		m_clM1Archive.Stop();
		m_clTickConsumer.Stop();
		return false;
	}

	// 第三步：可靠链路启用时创建唯一 1167 恢复线程；创建失败先回收消费者再返回。
	m_bBootstrapStopping.store(false);
	if (m_stDeriveConfig.stReliable.bEnable)
	{
		try
		{
			m_clBootstrapThread = std::thread(
				&CMtDeriveServiceApp::
					StateBootstrapThread, this);
		}
		catch (const std::exception& p_refException)
		{
			p_refError =
				std::string(
					"DERIVE_BOOTSTRAP_THREAD_START_FAILED: detail=") +
				p_refException.what();
			m_clReliableConsumer.Stop();
			m_clM1Archive.Stop();
			m_clTickConsumer.Stop();
			return false;
		}
		catch (...)
		{
			p_refError =
				"DERIVE_BOOTSTRAP_THREAD_START_FAILED: detail=unknown exception";
			m_clReliableConsumer.Stop();
			m_clM1Archive.Stop();
			m_clTickConsumer.Stop();
			return false;
		}
	}

	// 第四步：收益线程只读深拷贝权威状态；发布失败由下个周期重试，不能阻塞可靠消费。
	if (!m_clProfitEngine.Start(
			[this](const ST_DERIVE_PROFIT_SNAPSHOT_RESPONSE& p_refResponse,
				std::string& p_refPublishError)
			{
				return PublishProfitSnapshot(
					p_refResponse, p_refPublishError);
			}, p_refError))
	{
		m_bBootstrapStopping.store(true);
		m_clBootstrapCondition.notify_all();
		if (m_clBootstrapThread.joinable())
		{
			m_clBootstrapThread.join();
		}
		m_clReliableConsumer.Stop();
		m_clM1Archive.Stop();
		m_clTickConsumer.Stop();
		return false;
	}
	try
	{
		m_clGroupQuoteService.Start(
			[this](const ST_MT5_PROFIT_GROUP_QUOTE_KEY& p_refKey,
				ST_MT5_PROFIT_GROUP_QUOTE_QUERY_RESULT& p_refResult)
			{
				return CallMt5ProfitGroupQuote(p_refKey, p_refResult);
			},
			[this](const std::vector<
				ST_MT5_PROFIT_GROUP_QUOTE_DEPENDENCY>& p_refDependency)
			{
				m_clProfitEngine.OnGroupQuoteChanged(p_refDependency);
			},
			[this](const ST_MT5_PROFIT_GROUP_QUOTE_DEPENDENCY& p_refDependency)
			{
				return m_clProfitEngine.
					ValidateGroupQuoteDependency(p_refDependency);
			});
	}
	catch (const std::exception& p_refException)
	{
		p_refError = std::string(
			"DERIVE_GROUP_QUOTE_THREAD_START_FAILED: detail=") +
			p_refException.what();
		m_clProfitEngine.Stop();
		m_bBootstrapStopping.store(true);
		m_clBootstrapCondition.notify_all();
		if (m_clBootstrapThread.joinable())
		{
			m_clBootstrapThread.join();
		}
		m_clReliableConsumer.Stop();
		m_clM1Archive.Stop();
		m_clTickConsumer.Stop();
		return false;
	}
	catch (...)
	{
		p_refError =
			"DERIVE_GROUP_QUOTE_THREAD_START_FAILED: detail=unknown";
		m_clProfitEngine.Stop();
		m_bBootstrapStopping.store(true);
		m_clBootstrapCondition.notify_all();
		if (m_clBootstrapThread.joinable())
		{
			m_clBootstrapThread.join();
		}
		m_clReliableConsumer.Stop();
		m_clM1Archive.Stop();
		m_clTickConsumer.Stop();
		return false;
	}
	try
	{
		m_clM1PublishThread = std::thread(
			&CMtDeriveServiceApp::M1PublishThread, this);
	}
	catch (const std::exception& p_refException)
	{
		p_refError = std::string(
			"DERIVE_M1_PUBLISH_THREAD_START_FAILED: detail=") +
			p_refException.what();
		m_clGroupQuoteService.Stop();
		m_clProfitEngine.Stop();
		m_bBootstrapStopping.store(true);
		m_clBootstrapCondition.notify_all();
		if (m_clBootstrapThread.joinable())
		{
			m_clBootstrapThread.join();
		}
		m_clReliableConsumer.Stop();
		m_clM1Archive.Stop();
		m_clTickConsumer.Stop();
		return false;
	}
	catch (...)
	{
		p_refError =
			"DERIVE_M1_PUBLISH_THREAD_START_FAILED: detail=unknown";
		m_clGroupQuoteService.Stop();
		m_clProfitEngine.Stop();
		m_bBootstrapStopping.store(true);
		m_clBootstrapCondition.notify_all();
		if (m_clBootstrapThread.joinable())
		{
			m_clBootstrapThread.join();
		}
		m_clReliableConsumer.Stop();
		m_clM1Archive.Stop();
		m_clTickConsumer.Stop();
		return false;
	}

	// 第五步：后台组件均已取得自己的资源后才开放 1182-1188 请求。
	m_bStarted.store(true);
	if (!m_stDeriveConfig.stReliable.bEnable)
	{
		// 可靠状态关闭时只提供 M1；组件启动完成即可开放当前持有的来源。
		for (const ST_MT_DERIVE_SOURCE_CONFIG& refSource : m_stDeriveConfig.aSource)
		{
			if (refSource.bEnable &&
				HasClusterLease(refSource.usVersion, refSource.iNo) &&
				!SetClusterShardReady(refSource.usVersion, refSource.iNo,
					true, p_refError))
			{
				m_bStarted.store(false);
				m_clGroupQuoteService.Stop();
				m_clProfitEngine.Stop();
				m_clReliableConsumer.Stop();
				m_clM1Archive.Stop();
				m_clTickConsumer.Stop();
				return false;
			}
		}
	}
	MT_INFO(
		"derive components started,instance=%s,state=%s",
		m_stDeriveConfig.strInstanceId.c_str(),
		m_stDeriveConfig.stReliable.bEnable ?
			"BOOTSTRAPPING" : "M1_READY_PROFIT_DISABLED");
	return true;
}

void CMtDeriveServiceApp::OnStopping()
{
	// 第一步：先关闭请求闸门，后续在途请求由组件自己的互斥量完成或返回停止状态。
	m_bStarted.store(false);

	// 第二步：先停止 Tick durable，确保不再更新 M1/收益，再停止收益和可靠状态消费。
	m_clTickConsumer.Stop();
	m_clM1Archive.Stop();
	{
		std::lock_guard<std::mutex> clLock(m_clM1PublishMutex);
		m_bM1PublishStopping = true;
		m_clM1PublishCondition.notify_all();
	}
	if (m_clM1PublishThread.joinable() &&
		m_clM1PublishThread.get_id() != std::this_thread::get_id())
	{
		m_clM1PublishThread.join();
	}
	// 组报价线程可能正在回调 Profit；先停止新 1169 和回调，再等待 Profit Worker 退出。
	m_clGroupQuoteService.Stop();
	m_clProfitEngine.Stop();
	m_bBootstrapStopping.store(true);
	m_clBootstrapCondition.notify_all();
	m_clReliableConsumer.Stop();
	if (m_clBootstrapThread.joinable() &&
		m_clBootstrapThread.get_id() !=
			std::this_thread::get_id())
	{
		m_clBootstrapThread.join();
	}

	// 第三步：后台线程退出后清空状态；TickStore 不再持有 M1 指针后才能清空 M1。
	m_clStateStore.Clear();
	m_clTickStore.Stop();
	m_clM1Engine.Clear();
	m_clProfitLease.Clear();
	{
		std::lock_guard<std::mutex> clLock(m_clM1PublishMutex);
		m_mapPendingM1.clear();
		m_mapLastM1Minute.clear();
	}
}

bool CMtDeriveServiceApp::OnMtTimeStateChanged(
	const ST_MT_TIME_STATE& p_refState,
	std::string& p_refError)
{
	// 第一步：公共注册表尚未提交新值，此时先复制旧值用于判断是否跨越时间权威代次。
	bool bGenerationChanged = false;
	std::vector<ST_MT_TIME_STATE> aOldState;
	GetAllMtTimeStates(aOldState);
	for (const ST_MT_TIME_STATE& refOld : aOldState)
	{
		if (refOld.usPlatformVersion ==
				p_refState.usPlatformVersion &&
			refOld.iSourceNo == p_refState.iSourceNo)
		{
			bGenerationChanged =
				refOld.ullAuthorityEpoch !=
					p_refState.ullAuthorityEpoch ||
				refOld.ullGeneration !=
					p_refState.ullGeneration;
			break;
		}
	}

	// 第二步：M1 先接受新时间状态；失败时不得提前清除仍与公共注册表一致的组报价。
	if (!m_clM1Engine.UpdateTimeState(p_refState,
			p_refError) &&
		p_refError.find(
			"DERIVE_M1_TIME_SOURCE_NOT_CONFIGURED") ==
			std::string::npos)
	{
		return false;
	}
	p_refError.clear();
	if (bGenerationChanged &&
		p_refState.usPlatformVersion == 5U)
	{
		// 第三步：旧代次正在执行的 1169 即使随后返回，也会因节点 Epoch 和二次时间复核被丢弃。
		m_clGroupQuoteService.ClearNode(
			p_refState.iSourceNo,
			"time authority generation changed");
	}
	return true;
}

bool CMtDeriveServiceApp::HandleBusinessRequest(
	const ST_CLOUD_NET_BINARY_REQUEST* p_pRequest,
	ST_CLOUD_NET_BINARY_RESULT* p_pResult)
{
	if (p_pRequest == nullptr || p_pResult == nullptr ||
		!IsActiveMtDeriveFuncId(p_pRequest->lFuncId))
	{
		// 1181 仅保留 RETIRED 登记；公共健康、版本和 Echo 仍由基类处理。
		return false;
	}
	if (!m_bStarted.load())
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_DERIVE_BOOTSTRAPPING,
			"DERIVE_SERVICE_NOT_READY: components are starting or stopping");
		return true;
	}
	if (p_pRequest->stPayload.iLen < 0 ||
		(p_pRequest->stPayload.iLen > 0 &&
			p_pRequest->stPayload.pBuffer == nullptr))
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_INVALID_REQUEST,
			"DERIVE_REQUEST_BUFFER_INVALID: payload length or pointer is invalid");
		return true;
	}

	switch (p_pRequest->lFuncId)
	{
	case EN_PLUGIN_FUNC_DERIVE_TICK_APPEND:
		return HandleTickBatch(p_pRequest, p_pResult);
	case EN_PLUGIN_FUNC_DERIVE_PROFIT_DEMAND:
		return HandleProfitDemand(p_pRequest, p_pResult);
	case EN_PLUGIN_FUNC_DERIVE_PROFIT_SNAPSHOT:
		return HandleProfitSnapshot(p_pRequest, p_pResult);
	case EN_PLUGIN_FUNC_DERIVE_M1_SNAPSHOT:
		return HandleM1Snapshot(p_pRequest, p_pResult);
	case EN_PLUGIN_FUNC_DERIVE_STATUS:
		return HandleStatus(p_pRequest, p_pResult);
	case EN_PLUGIN_FUNC_DERIVE_M1_ARCHIVE_FETCH:
		return HandleM1ArchiveFetch(p_pRequest, p_pResult);
	case EN_PLUGIN_FUNC_DERIVE_M1_ARCHIVE_ACK:
		return HandleM1ArchiveAck(p_pRequest, p_pResult);
	default:
		return false;
	}
}

bool CMtDeriveServiceApp::HandleTickBatch(
	const ST_CLOUD_NET_BINARY_REQUEST* p_pRequest,
	ST_CLOUD_NET_BINARY_RESULT* p_pResult)
{
	// 第一步：严格解码连续 Tick 批次，协议层校验 Version、Epoch、序号和每条 Tick 来源。
	ST_DERIVE_TICK_BATCH_REQUEST stRequest;
	std::string strError;
	if (!DecodeDeriveTickBatchRequest(
			p_pRequest->stPayload.pBuffer,
			static_cast<std::size_t>(
				p_pRequest->stPayload.iLen),
			stRequest, strError))
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_PROTOCOL_ERROR,
			strError.c_str());
		return true;
	}
	if (!IsClusterOwner(stRequest.usPlatformVersion, stRequest.iSourceNo))
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_SERVICE_NOT_OWNER,
			"SERVICE_NOT_OWNER: MtDeriveService cannot append Tick data for a standby shard");
		return true;
	}

	// 第二步：在线入口必须先取得当前时间权威，避免冬夏令切换时把旧代次 Tick 写入 WAL。
	ST_MT_TIME_STATE stTimeState;
	if (!GetMtTimeState(stRequest.usPlatformVersion,
			stRequest.iSourceNo, stTimeState, strError))
	{
		const std::string strTimeError =
			"DERIVE_TICK_TIME_NOT_READY: version=" +
			std::to_string(stRequest.usPlatformVersion) +
			", no=" + std::to_string(stRequest.iSourceNo) +
			", detail=" + strError;
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_MT_TIME_NOT_READY,
			strTimeError.c_str());
		return true;
	}
	// 第三步：WAL 刷盘、M1 应用、检查点更新和 ACK 生成在来源锁内按固定顺序完成。
	ST_DERIVE_TICK_BATCH_RESPONSE stResponse;
	std::vector<ST_DERIVE_M1_BAR> aChangedBar;
	if (!m_clTickStore.Append(stRequest, stResponse,
			aChangedBar, strError))
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_INTERNAL_ERROR,
			strError.empty() ?
				"DERIVE_TICK_APPEND_FAILED: TickStore returned false" :
				strError.c_str());
		return true;
	}

	// 第三步：业务成功后发布批次最终 M1。发布失败不能回滚已经持久化并确认的 Tick。
	if (stResponse.iCode == EN_TERMINAL_ERROR_OK)
	{
		// 收益报价只在 Tick 已持久化并通过连续序号校验后更新，避免使用尚未取得所有权的数据。
		std::string strProfitError;
		if (!m_clProfitEngine.OnTicks(stRequest,
				strProfitError))
		{
			MT_WARN(
				"derive profit Tick update failed,version=%u,no=%d,firstSequence=%llu,detail=%s",
				static_cast<unsigned int>(
					stRequest.usPlatformVersion),
				stRequest.iSourceNo,
				static_cast<unsigned long long>(
					stRequest.ullFirstSequence),
				strProfitError.c_str());
		}
		ForwardGroupQuoteTicks(stRequest);
		PublishChangedM1(aChangedBar);
	}

	// 第四步：无论成功或可恢复的业务失败都返回 1182 应答，Quote 据 AckSequence 决定 WAL 删除边界。
	std::vector<unsigned char> aResponse;
	if (!EncodeDeriveTickBatchResponse(stResponse,
			aResponse, strError))
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_PROTOCOL_ERROR,
			strError.c_str());
		return true;
	}
	return SetEncodedResponse(aResponse,
		stResponse.iCode, p_pResult,
		"DERIVE_TICK_RESPONSE");
}

bool CMtDeriveServiceApp::ApplyTickStreamTick(
	const ST_QUOTE_BINARY_TICK& p_refTick,
	std::string& p_refError)
{
	p_refError.clear();
	if (!HasClusterLease(p_refTick.usPlatformVersion,
			p_refTick.iSourceNo))
	{
		p_refError =
			"SERVICE_NOT_OWNER: Derive Tick durable lost the Version+No lease";
		return false;
	}

	// 第一步：RECOVERING Owner 也必须校验当前时间权威，避免快照恢复期间生成错误分钟。
	ST_MT_TIME_STATE stTimeState;
	if (!GetMtTimeState(p_refTick.usPlatformVersion,
			p_refTick.iSourceNo, stTimeState, p_refError))
	{
		p_refError =
			"DERIVE_TICK_STREAM_TIME_NOT_READY: " + p_refError;
		return false;
	}
	// 第二步：单消息转换为连续批次，复用 TickStore 的 WAL、检查点和序号幂等规则。
	ST_DERIVE_TICK_BATCH_REQUEST stRequest;
	stRequest.usPlatformVersion = p_refTick.usPlatformVersion;
	stRequest.iSourceNo = p_refTick.iSourceNo;
	stRequest.ullSourceEpoch = p_refTick.ullSourceEpoch;
	stRequest.ullFirstSequence = p_refTick.ullIngressSequence;
	stRequest.aTick.push_back(p_refTick);
	ST_DERIVE_TICK_BATCH_RESPONSE stResponse;
	std::vector<ST_DERIVE_M1_BAR> aChangedBar;
	if (!m_clTickStore.Append(stRequest, stResponse,
			aChangedBar, p_refError))
	{
		p_refError = p_refError.empty() ?
			"DERIVE_TICK_STREAM_APPEND_FAILED: TickStore returned false" :
			p_refError;
		return false;
	}
	if (stResponse.iCode != EN_TERMINAL_ERROR_OK)
	{
		p_refError = stResponse.strMessage.empty() ?
			"DERIVE_TICK_STREAM_APPEND_REJECTED: TickStore returned a business error" :
			stResponse.strMessage;
		return false;
	}

	// 第三步：只有持久化成功的 Tick 才进入收益报价和权威 M1 发布，发布失败不回滚 Tick ACK。
	std::string strProfitError;
	if (!m_clProfitEngine.OnTicks(stRequest, strProfitError))
	{
		MT_WARN(
			"derive profit Tick stream update failed,version=%u,no=%d,sequence=%llu,detail=%s",
			static_cast<unsigned int>(p_refTick.usPlatformVersion),
			p_refTick.iSourceNo,
			static_cast<unsigned long long>(p_refTick.ullIngressSequence),
			strProfitError.c_str());
	}
	ForwardGroupQuoteTicks(stRequest);
	PublishChangedM1(aChangedBar);
	return true;
}

bool CMtDeriveServiceApp::HandleProfitDemand(
	const ST_CLOUD_NET_BINARY_REQUEST* p_pRequest,
	ST_CLOUD_NET_BINARY_RESULT* p_pResult)
{
	// 第一步：解码 Gateway 绝对账号集合，禁止使用不可靠的增量引用计数。
	ST_DERIVE_PROFIT_DEMAND_REQUEST stRequest;
	std::string strError;
	if (!DecodeDeriveProfitDemandRequest(
			p_pRequest->stPayload.pBuffer,
			static_cast<std::size_t>(
				p_pRequest->stPayload.iLen),
			stRequest, strError))
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_PROTOCOL_ERROR,
			strError.c_str());
		return true;
	}

	// 第二步：在租约表内原子执行 Revision 校验、容量检查和 REPLACE/DELETE。
	ST_DERIVE_PROFIT_DEMAND_RESPONSE stResponse;
	if (!m_clProfitLease.Apply(stRequest,
			GetTimestampMs(), stResponse, strError))
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_INTERNAL_ERROR,
			strError.empty() ?
				"DERIVE_PROFIT_DEMAND_FAILED: lease registry returned false" :
				strError.c_str());
		return true;
	}
	std::vector<unsigned char> aResponse;
	if (!EncodeDeriveProfitDemandResponse(stResponse,
			aResponse, strError))
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_PROTOCOL_ERROR,
			strError.c_str());
		return true;
	}
	return SetEncodedResponse(aResponse,
		stResponse.iCode, p_pResult,
		"DERIVE_PROFIT_DEMAND_RESPONSE");
}

bool CMtDeriveServiceApp::HandleProfitSnapshot(
	const ST_CLOUD_NET_BINARY_REQUEST* p_pRequest,
	ST_CLOUD_NET_BINARY_RESULT* p_pResult)
{
	// 第一步：即使收益状态尚未恢复，也必须校验请求，避免错误报文被误判为冷启动。
	ST_DERIVE_PROFIT_SNAPSHOT_REQUEST stRequest;
	std::string strError;
	if (!DecodeDeriveProfitSnapshotRequest(
			p_pRequest->stPayload.pBuffer,
			static_cast<std::size_t>(
				p_pRequest->stPayload.iLen),
			stRequest, strError))
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_PROTOCOL_ERROR,
			strError.c_str());
		return true;
	}

	// 第二步：收益引擎使用权威状态深拷贝和最新 Tick 计算；冷启动和缺汇率分别返回明确业务状态。
	ST_DERIVE_PROFIT_SNAPSHOT_RESPONSE stResponse;
	std::lock_guard<std::mutex> clSnapshotLock(
		m_aProfitSnapshotMutex[SelectNotifyMutex(
			stRequest.usPlatformVersion, stRequest.iSourceNo)]);
	if (!m_clProfitEngine.Snapshot(stRequest, stResponse, strError))
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_INTERNAL_ERROR,
			strError.empty() ?
				"DERIVE_PROFIT_SNAPSHOT_FAILED: profit engine returned false" :
				strError.c_str());
		return true;
	}
	// 快照水位与 1251 的外层序号使用同一计数器，Gateway 可据此丢弃并发重复增量。
	stResponse.ullEventSequence =
		m_ullProfitNotifySequence.load();

	std::vector<unsigned char> aResponse;
	if (!EncodeDeriveProfitSnapshotResponse(stResponse,
			aResponse, strError))
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_PROTOCOL_ERROR,
			strError.c_str());
		return true;
	}
	return SetEncodedResponse(aResponse,
		stResponse.iCode, p_pResult,
		"DERIVE_PROFIT_SNAPSHOT_RESPONSE");
}

bool CMtDeriveServiceApp::HandleM1Snapshot(
	const ST_CLOUD_NET_BINARY_REQUEST* p_pRequest,
	ST_CLOUD_NET_BINARY_RESULT* p_pResult)
{
	// 第一步：解码来源、品种、分钟范围和数量上限。
	ST_DERIVE_M1_SNAPSHOT_REQUEST stRequest;
	std::string strError;
	if (!DecodeDeriveM1SnapshotRequest(
			p_pRequest->stPayload.pBuffer,
			static_cast<std::size_t>(
				p_pRequest->stPayload.iLen),
			stRequest, strError))
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_PROTOCOL_ERROR,
			strError.c_str());
		return true;
	}

	// 第二步：在 M1 引擎锁内复制稳定排序快照，返回值不引用内部容器。
	ST_DERIVE_M1_SNAPSHOT_RESPONSE stResponse;
	std::lock_guard<std::mutex> clSnapshotLock(
		m_aM1SnapshotMutex[SelectNotifyMutex(
			stRequest.usPlatformVersion, stRequest.iSourceNo)]);
	if (!m_clM1Engine.Snapshot(stRequest, stResponse, strError))
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_INTERNAL_ERROR,
			strError.empty() ?
				"DERIVE_M1_SNAPSHOT_FAILED: M1 engine returned false" :
				strError.c_str());
		return true;
	}
	// 快照水位冻结为编码前已发布的 1252 序号，订阅屏障只回放更高序号。
	stResponse.ullEventSequence =
		m_ullM1NotifySequence.load();
	std::vector<unsigned char> aResponse;
	if (!EncodeDeriveM1SnapshotResponse(stResponse,
			aResponse, strError))
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_PROTOCOL_ERROR,
			strError.c_str());
		return true;
	}
	return SetEncodedResponse(aResponse,
		stResponse.iCode, p_pResult,
		"DERIVE_M1_SNAPSHOT_RESPONSE");
}

bool CMtDeriveServiceApp::HandleStatus(
	const ST_CLOUD_NET_BINARY_REQUEST* p_pRequest,
	ST_CLOUD_NET_BINARY_RESULT* p_pResult)
{
	if (p_pRequest->stPayload.iLen != 0)
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_INVALID_REQUEST,
			"DERIVE_STATUS_REQUEST_INVALID: 1186 payload must be empty");
		return true;
	}

	// 第一步：复制来源持久化状态和有效收益租约数量。
	ST_DERIVE_STATUS_RESPONSE stResponse;
	m_clTickStore.GetStatus(stResponse.aSource);
	for (ST_DERIVE_SOURCE_STATUS& refStatus : stResponse.aSource)
	{
		ST_DERIVE_MINUTE_SOURCE_DIAGNOSTICS stDiagnostics;
		m_clM1Engine.GetSourceDiagnostics(
			refStatus.usPlatformVersion, refStatus.iSourceNo,
			stDiagnostics);
		refStatus.llArchiveWatermark =
			stDiagnostics.llArchiveWatermark;
		refStatus.uiArchivePendingCount =
			stDiagnostics.uiArchivePendingCount;
		refStatus.uiBackfillPendingCount =
			stDiagnostics.uiBackfillPendingCount;
	}
	stResponse.ullActiveProfitLogins =
		m_clProfitLease.GetActiveLoginCount(
			GetTimestampMs());
	stResponse.ullArchiveSpoolBytes =
		m_clM1Archive.GetSpoolBytes();
	stResponse.ullArchiveOldestTaskAgeMs =
		m_clM1Archive.GetOldestTaskAgeMs();
	stResponse.ullArchivePublishedCount =
		m_clM1Archive.GetPublishedCount();
	stResponse.ullArchiveConsumerAckCount =
		m_clM1Archive.GetConsumerAckCount();
	stResponse.uiArchivePendingCount =
		m_clM1Archive.GetPendingCount();
	stResponse.uiBackfillInProgressCount =
		m_clM1Archive.GetBackfillInProgressCount();
	ST_MT_DERIVE_RELIABLE_STATUS stReliableStatus;
	ST_MT_DERIVE_STATE_STATUS stStateStatus;
	m_clReliableConsumer.GetStatus(stReliableStatus);
	m_clStateStore.GetStatus(stStateStatus);
	stResponse.ullReliableSequence =
		stReliableStatus.ullLastAckSequence;

	// 第二步：M1 可以独立 READY；可靠消费、快照恢复或收益引擎未完成时全局明确标记为 DEGRADED。
	stResponse.enState =
		EN_DERIVE_SERVICE_STATE_DEGRADED;
	stResponse.iCode =
		EN_TERMINAL_ERROR_DERIVE_BOOTSTRAPPING;
	stResponse.strMessage = m_stDeriveConfig.
		stReliable.bEnable ?
		("DERIVE_STATE_BOOTSTRAPPING: readySources=" +
			std::to_string(
				stStateStatus.szReadySourceCount) +
			", sourceCount=" +
			std::to_string(stStateStatus.szSourceCount) +
			", bufferedEvents=" +
			std::to_string(
				stStateStatus.szBufferedEventCount) +
			", reliableReady=" +
			std::to_string(
				stReliableStatus.bReady ? 1 : 0)) :
		"DERIVE_PARTIALLY_READY: reliable state consumer is disabled; lossless M1 remains available";
	if (stStateStatus.bReady &&
		m_clProfitEngine.IsReady())
	{
		stResponse.enState =
			EN_DERIVE_SERVICE_STATE_READY;
		stResponse.iCode = EN_TERMINAL_ERROR_OK;
		stResponse.strMessage =
			"OK";
	}
	for (const ST_DERIVE_SOURCE_STATUS& refStatus :
		stResponse.aSource)
	{
		if (refStatus.enState !=
			EN_DERIVE_SERVICE_STATE_READY)
		{
			stResponse.strMessage =
				"DERIVE_DEGRADED: one or more Tick sources are not ready";
			break;
		}
	}

	std::string strError;
	std::vector<unsigned char> aResponse;
	if (!EncodeDeriveStatusResponse(stResponse,
			aResponse, strError))
	{
		SetBinaryResultError(p_pResult,
			EN_TERMINAL_ERROR_PROTOCOL_ERROR,
			strError.c_str());
		return true;
	}
	return SetEncodedResponse(aResponse,
		stResponse.iCode, p_pResult,
		"DERIVE_STATUS_RESPONSE");
}

bool CMtDeriveServiceApp::HandleM1ArchiveFetch(
	const ST_CLOUD_NET_BINARY_REQUEST* p_pRequest,
	ST_CLOUD_NET_BINARY_RESULT* p_pResult)
{
	ST_DERIVE_M1_ARCHIVE_FETCH_REQUEST stRequest;
	std::string strError;
	if (!DecodeDeriveM1ArchiveFetchRequest(
			p_pRequest->stPayload.pBuffer,
			static_cast<std::size_t>(p_pRequest->stPayload.iLen),
			stRequest, strError))
	{
		SetBinaryResultError(p_pResult, EN_TERMINAL_ERROR_PROTOCOL_ERROR,
			strError.c_str());
		return true;
	}
	ST_DERIVE_M1_ARCHIVE_FETCH_RESPONSE stResponse;
	if (!m_clM1Archive.Fetch(stRequest, stResponse, strError))
	{
		SetBinaryResultError(p_pResult, EN_TERMINAL_ERROR_INTERNAL_ERROR,
			strError.c_str());
		return true;
	}
	std::vector<unsigned char> aResponse;
	if (!EncodeDeriveM1ArchiveFetchResponse(stResponse,
			aResponse, strError))
	{
		SetBinaryResultError(p_pResult, EN_TERMINAL_ERROR_PROTOCOL_ERROR,
			strError.c_str());
		return true;
	}
	return SetEncodedResponse(aResponse, stResponse.iCode,
		p_pResult, "DERIVE_M1_ARCHIVE_FETCH_RESPONSE");
}

bool CMtDeriveServiceApp::HandleM1ArchiveAck(
	const ST_CLOUD_NET_BINARY_REQUEST* p_pRequest,
	ST_CLOUD_NET_BINARY_RESULT* p_pResult)
{
	ST_DERIVE_M1_ARCHIVE_ACK_REQUEST stRequest;
	std::string strError;
	if (!DecodeDeriveM1ArchiveAckRequest(
			p_pRequest->stPayload.pBuffer,
			static_cast<std::size_t>(p_pRequest->stPayload.iLen),
			stRequest, strError))
	{
		SetBinaryResultError(p_pResult, EN_TERMINAL_ERROR_PROTOCOL_ERROR,
			strError.c_str());
		return true;
	}
	ST_DERIVE_M1_ARCHIVE_ACK_RESPONSE stResponse;
	if (!m_clM1Archive.Ack(stRequest, stResponse, strError))
	{
		SetBinaryResultError(p_pResult, EN_TERMINAL_ERROR_INTERNAL_ERROR,
			strError.c_str());
		return true;
	}
	std::vector<unsigned char> aResponse;
	if (!EncodeDeriveM1ArchiveAckResponse(stResponse,
			aResponse, strError))
	{
		SetBinaryResultError(p_pResult, EN_TERMINAL_ERROR_PROTOCOL_ERROR,
			strError.c_str());
		return true;
	}
	return SetEncodedResponse(aResponse, stResponse.iCode,
		p_pResult, "DERIVE_M1_ARCHIVE_ACK_RESPONSE");
}

void CMtDeriveServiceApp::ForwardGroupQuoteTicks(
	const ST_DERIVE_TICK_BATCH_REQUEST& p_refRequest)
{
	if (p_refRequest.usPlatformVersion != 5U ||
		!HasClusterLease(p_refRequest.usPlatformVersion,
			p_refRequest.iSourceNo))
	{
		return;
	}
	for (const ST_QUOTE_BINARY_TICK& refTick : p_refRequest.aTick)
	{
		if (refTick.strSymbol.empty() ||
			refTick.strSymbol.size() >= 128U)
		{
			continue;
		}
		ST_REALTIME_TICK stTick;
		stTick.iVersion = refTick.usPlatformVersion;
		stTick.iNo = refTick.iSourceNo;
		if (strncpy_s(stTick.szSymbol, sizeof(stTick.szSymbol),
				refTick.strSymbol.c_str(), _TRUNCATE) != 0)
		{
			continue;
		}
		stTick.dBid = refTick.dBid;
		stTick.dAsk = refTick.dAsk;
		stTick.dLast = refTick.dLast;
		stTick.ui64Volume = refTick.ullVolume;
		stTick.ui64VolumeExt = refTick.ullVolumeExt;
		stTick.ui64Flags = refTick.ullFlags;
		stTick.i64MarketTime = refTick.llServerTime;
		stTick.i64MarketTimeMsc = refTick.llServerTimeMsc;
		stTick.i64ServerTime = refTick.llServerTime;
		stTick.i64IngressTimeMs = refTick.llIngressTimeMs;
		stTick.ui64IngressSeq = refTick.ullIngressSequence;
		m_clGroupQuoteService.OnRealtimeTick(stTick);
	}
}

bool CMtDeriveServiceApp::CallM1Backfill(
	const ST_QUERY_M1_BACKFILL_REQUEST& p_refRequest,
	ST_QUERY_M1_BACKFILL_RESPONSE& p_refResponse,
	std::string& p_refError)
{
	p_refResponse = ST_QUERY_M1_BACKFILL_RESPONSE();
	p_refError.clear();
	if (GetCloudNetApi() == nullptr)
	{
		p_refError = "DERIVE_M1_BACKFILL_NETWORK_NOT_READY";
		return false;
	}
	std::vector<unsigned char> aRequest;
	if (!EncodeQueryM1BackfillRequest(p_refRequest,
			aRequest, p_refError) ||
		aRequest.size() > static_cast<std::size_t>(
			(std::numeric_limits<int>::max)()))
	{
		return false;
	}
	ST_CLOUD_NET_BINARY_CALL stCall;
	stCall.lSynId = GetTimestampMs();
	stCall.lFuncId = EN_PLUGIN_FUNC_QUERY_M1_BACKFILL;
	stCall.stPayload.pBuffer = aRequest.data();
	stCall.stPayload.iLen = static_cast<int>(aRequest.size());
	ST_CLOUD_NET_BINARY_RESULT* pResult = CallBinarySync(
		GetCloudNetApi(), MT_DERIVE_QUERY_CONNECTION, &stCall);
	if (pResult == nullptr)
	{
		p_refError = "DERIVE_M1_BACKFILL_CALL_FAILED: code=" +
			std::to_string(GetLastErrorCode(GetCloudNetApi())) +
			", detail=" + GetLastErrorDetail(GetCloudNetApi());
		return false;
	}
	const std::unique_ptr<ST_CLOUD_NET_BINARY_RESULT,
		void (*)(ST_CLOUD_NET_BINARY_RESULT*)> clResult(
		pResult, FreeBinaryResult);
	if (pResult->iErrorCode != 0 || pResult->lRetVal < 0 ||
		pResult->stPayload.iLen <= 0 ||
		pResult->stPayload.pBuffer == nullptr)
	{
		p_refError = "DERIVE_M1_BACKFILL_TRANSPORT_ERROR: networkCode=" +
			std::to_string(pResult->iErrorCode) + ", ret=" +
			std::to_string(pResult->lRetVal) + ", detail=" +
			pResult->szErrInfo;
		return false;
	}
	return DecodeQueryM1BackfillResponse(
		pResult->stPayload.pBuffer,
		static_cast<std::size_t>(pResult->stPayload.iLen),
		p_refResponse, p_refError);
}

bool CMtDeriveServiceApp::CallMt5ProfitGroupQuote(
	const ST_MT5_PROFIT_GROUP_QUOTE_KEY& p_refKey,
	ST_MT5_PROFIT_GROUP_QUOTE_QUERY_RESULT& p_refResult)
{
	p_refResult = ST_MT5_PROFIT_GROUP_QUOTE_QUERY_RESULT();
	if (p_refKey.iNo <= 0 || p_refKey.strGroup.empty() ||
		p_refKey.strSymbol.empty())
	{
		p_refResult.strError =
			"DERIVE_GROUP_QUOTE_KEY_INVALID: positive No, Group and Symbol are required";
		return false;
	}
	if (GetCloudNetApi() == nullptr ||
		!HasClusterLease(5U, p_refKey.iNo))
	{
		p_refResult.strError = GetCloudNetApi() == nullptr ?
			"DERIVE_GROUP_QUOTE_NETWORK_NOT_READY: CloudNet API is null" :
			"SERVICE_NOT_OWNER: Derive lost the MT5 shard lease before 1169";
		return false;
	}

	// 第一步：查询前固定本地时间权威代次；RPC 返回后必须与该副本逐字段复核。
	ST_MT_TIME_STATE stTimeBefore;
	std::string strError;
	if (!GetMtTimeState(5U, p_refKey.iNo,
			stTimeBefore, strError))
	{
		p_refResult.strError =
			"DERIVE_GROUP_QUOTE_TIME_NOT_READY: " + strError;
		return false;
	}

	// 第二步：1169 是内部 Query Binary 对象，固定只传四个白名单字段。
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refRequest =
		MakeValue(EN_PLUGIN_BINARY_VALUE_OBJECT, strError);
	if (!refRequest ||
		!AddInt64(*refRequest, "Version", 5, strError) ||
		!AddInt64(*refRequest, "No", p_refKey.iNo, strError) ||
		!AddString(*refRequest, "Group",
			p_refKey.strGroup, strError) ||
		!AddString(*refRequest, "Symbol",
			p_refKey.strSymbol, strError))
	{
		p_refResult.strError = strError;
		return false;
	}
	std::vector<unsigned char> aRequest;
	if (!EncodeQueryBinaryRequest(*refRequest,
			aRequest, strError) ||
		aRequest.empty() || aRequest.size() >
			static_cast<std::size_t>(
				(std::numeric_limits<int>::max)()))
	{
		p_refResult.strError = strError.empty() ?
			"DERIVE_GROUP_QUOTE_REQUEST_TOO_LARGE" : strError;
		return false;
	}

	// 第三步：校准线程同步调用普通查询池；结果缓冲在当前作用域结束时统一释放。
	ST_CLOUD_NET_BINARY_CALL stCall;
	stCall.lSynId = GetTimestampMs();
	stCall.lFuncId =
		EN_PLUGIN_FUNC_QUERY_MT5_PROFIT_GROUP_QUOTE;
	stCall.stPayload.pBuffer = aRequest.data();
	stCall.stPayload.iLen = static_cast<int>(aRequest.size());
	ST_CLOUD_NET_BINARY_RESULT* pResult = CallBinarySync(
		GetCloudNetApi(), MT_DERIVE_QUERY_CONNECTION, &stCall);
	if (pResult == nullptr)
	{
		p_refResult.strError =
			"DERIVE_GROUP_QUOTE_CALL_FAILED: code=" +
			std::to_string(GetLastErrorCode(GetCloudNetApi())) +
			", detail=" + GetLastErrorDetail(GetCloudNetApi());
		return false;
	}
	const std::unique_ptr<ST_CLOUD_NET_BINARY_RESULT,
		void (*)(ST_CLOUD_NET_BINARY_RESULT*)> clResult(
			pResult, FreeBinaryResult);
	if (pResult->iErrorCode != 0 || pResult->lRetVal < 0 ||
		pResult->stPayload.iLen <= 0 ||
		pResult->stPayload.pBuffer == nullptr)
	{
		p_refResult.strError =
			"DERIVE_GROUP_QUOTE_TRANSPORT_ERROR: networkCode=" +
			std::to_string(pResult->iErrorCode) + ", ret=" +
			std::to_string(pResult->lRetVal) + ", detail=" +
			pResult->szErrInfo;
		return false;
	}

	// 第四步：业务错误是可诊断失败，不读取 Data；普通池繁忙单独反馈给 V2 调度统计。
	ST_PLUGIN_BINARY_DOCUMENT stDocument;
	if (!DecodeQueryBinaryDocument(
			pResult->stPayload.pBuffer,
			static_cast<std::size_t>(
				pResult->stPayload.iLen),
			stDocument, strError) || !stDocument.refRoot)
	{
		p_refResult.strError = strError;
		return false;
	}
	if (stDocument.iCode != EN_TERMINAL_ERROR_OK)
	{
		p_refResult.bQueryPoolBusy =
			stDocument.iCode ==
			EN_TERMINAL_ERROR_MT_MANAGER_CONNECTION_BUSY;
		p_refResult.strError =
			"DERIVE_GROUP_QUOTE_REMOTE_ERROR: code=" +
			std::to_string(stDocument.iCode) + ", detail=" +
			stDocument.strMessage;
		return false;
	}
	if (stDocument.refRoot->enType !=
		EN_PLUGIN_BINARY_VALUE_OBJECT)
	{
		p_refResult.strError =
			"DERIVE_GROUP_QUOTE_RESPONSE_INVALID: Data must be an object";
		return false;
	}
	std::int64_t llGlobalTickMsc = 0;
	std::int64_t llGroupTickMsc = 0;
	std::int64_t llApiCallCount = 0;
	std::int64_t llRetCode = 0;
	if (!ReadDouble(*stDocument.refRoot, "GlobalBid",
			p_refResult.dGlobalBid, strError) ||
		!ReadDouble(*stDocument.refRoot, "GlobalAsk",
			p_refResult.dGlobalAsk, strError) ||
		!ReadDouble(*stDocument.refRoot, "GroupBid",
			p_refResult.dGroupBid, strError) ||
		!ReadDouble(*stDocument.refRoot, "GroupAsk",
			p_refResult.dGroupAsk, strError) ||
		!ReadInt64(*stDocument.refRoot, "GlobalTickMsc",
			llGlobalTickMsc, strError) ||
		!ReadInt64(*stDocument.refRoot, "GroupTickMsc",
			llGroupTickMsc, strError) ||
		!ReadInt64(*stDocument.refRoot, "ApiCallCount",
			llApiCallCount, strError) ||
		!ReadInt64(*stDocument.refRoot, "RetCode",
			llRetCode, strError) ||
		!ReadUInt64(*stDocument.refRoot, "TimeAuthorityEpoch",
			p_refResult.ui64TimeAuthorityEpoch, strError) ||
		!ReadUInt64(*stDocument.refRoot, "TimeGeneration",
			p_refResult.ui64TimeGeneration, strError) ||
		llApiCallCount < 0 ||
		llApiCallCount > (std::numeric_limits<int>::max)() ||
		llRetCode < (std::numeric_limits<int>::min)() ||
		llRetCode > (std::numeric_limits<int>::max)())
	{
		p_refResult.strError = strError.empty() ?
			"DERIVE_GROUP_QUOTE_RESPONSE_RANGE_INVALID" : strError;
		return false;
	}
	p_refResult.i64GlobalTickMsc = llGlobalTickMsc;
	p_refResult.i64GroupTickMsc = llGroupTickMsc;
	p_refResult.iApiCallCount =
		static_cast<int>(llApiCallCount);
	p_refResult.iRetCode = static_cast<int>(llRetCode);

	// 第五步：Owner、本地时间和 Query 使用的代次必须同时一致；任一变化都拒绝发布整次结果。
	ST_MT_TIME_STATE stTimeAfter;
	if (!HasClusterLease(5U, p_refKey.iNo) ||
		!GetMtTimeState(5U, p_refKey.iNo,
			stTimeAfter, strError) ||
		stTimeBefore.ullAuthorityEpoch !=
			stTimeAfter.ullAuthorityEpoch ||
		stTimeBefore.ullGeneration !=
			stTimeAfter.ullGeneration ||
		p_refResult.ui64TimeAuthorityEpoch !=
			stTimeBefore.ullAuthorityEpoch ||
		p_refResult.ui64TimeGeneration !=
			stTimeBefore.ullGeneration)
	{
		p_refResult.strError =
			"DERIVE_GROUP_QUOTE_FENCE_CHANGED: Owner or time generation changed during 1169";
		return false;
	}
	return true;
}

bool CMtDeriveServiceApp::ApplyReliableEvent(
	std::uint64_t p_ullStreamSequence,
	const std::vector<unsigned char>& p_refEnvelope,
	std::string& p_refError)
{
	// 可靠消费线程同步调用本函数；状态仓库完成协议校验、深拷贝和快照水位判断后才允许 ACK。
	std::uint16_t usVersion = 0;
	std::int32_t iNo = 0;
	bool bApplied = false;
	if (!m_clStateStore.ApplyReliableEnvelope(
		p_ullStreamSequence, p_refEnvelope,
		p_refError, &usVersion, &iNo, &bApplied))
	{
		return false;
	}
	// 状态事件可能在上次失败后按实体序号被判定为重复；仍必须执行全量幂等核对后才允许 ACK。
	if (!ReconcileMinuteSymbols(usVersion, iNo, p_refError))
	{
		return false;
	}
	// 组报价副作用必须在重复交付时也幂等执行，避免状态已提交而首次副作用失败后无法恢复。
	if (!ApplyGroupQuoteStateChange(p_refEnvelope,
			usVersion, iNo, p_refError))
	{
		return false;
	}
	if (bApplied)
	{
		std::string strProfitError;
		if (!m_clProfitEngine.OnAuthoritativeStateChanged(
				usVersion, iNo, strProfitError))
		{
			// 权威状态已经提交，收益线程的低频全量核对负责恢复，不能因此拒绝可靠事件 ACK。
			MT_WARN(
				"derive profit state-change refresh deferred,version=%u,no=%d,sequence=%llu,detail=%s",
				static_cast<unsigned int>(usVersion), iNo,
				static_cast<unsigned long long>(p_ullStreamSequence),
				strProfitError.c_str());
		}
	}
	return true;
}

bool CMtDeriveServiceApp::ApplyGroupQuoteStateChange(
	const std::vector<unsigned char>& p_refEnvelope,
	std::uint16_t p_usVersion, std::int32_t p_iNo,
	std::string& p_refError)
{
	p_refError.clear();
	if (p_usVersion != 5U)
	{
		return true;
	}

	// 第一步：状态仓库已验证同一信封；这里再次解码只为取得同名组件的精确失效维度。
	ST_PLUGIN_BINARY_DOCUMENT stEnvelope;
	if (!DecodeReliableEventBinaryDocument(
			p_refEnvelope.data(), p_refEnvelope.size(),
			stEnvelope, p_refError) || !stEnvelope.refRoot)
	{
		return false;
	}
	std::int64_t llNotifyId = 0;
	std::int64_t llAction = 0;
	std::vector<unsigned char> aPayload;
	if (!ReadInt64(*stEnvelope.refRoot, "NotifyId",
			llNotifyId, p_refError) ||
		!ReadInt64(*stEnvelope.refRoot, "Action",
			llAction, p_refError) ||
		!ReadBytes(*stEnvelope.refRoot, "Payload",
			aPayload, p_refError))
	{
		return false;
	}
	const EN_PLUGIN_NOTIFY_ID enNotifyId =
		static_cast<EN_PLUGIN_NOTIFY_ID>(llNotifyId);
	if (enNotifyId != EN_PLUGIN_NOTIFY_USER_CHANGED &&
		enNotifyId != EN_PLUGIN_NOTIFY_POSITION_CHANGED &&
		enNotifyId != EN_PLUGIN_NOTIFY_GROUP_CHANGED &&
		enNotifyId != EN_PLUGIN_NOTIFY_SYMBOL_CHANGED)
	{
		return true;
	}

	// 第二步：只解析需要的业务主键；失效函数不持有 StateStore 锁，可安全唤醒校准线程。
	ST_PLUGIN_BINARY_DOCUMENT stPayload;
	if (!DecodeTradeBinaryDocument(aPayload.data(),
			aPayload.size(), stPayload, p_refError) ||
		!stPayload.refRoot)
	{
		return false;
	}
	if (enNotifyId == EN_PLUGIN_NOTIFY_USER_CHANGED)
	{
		std::int64_t llLogin = 0;
		if (!ReadInt64(*stPayload.refRoot,
				"Login", llLogin, p_refError) || llLogin <= 0)
		{
			if (p_refError.empty())
			{
				p_refError =
					"DERIVE_GROUP_QUOTE_LOGIN_INVALID: Login must be positive";
			}
			return false;
		}
		m_clGroupQuoteService.RemoveLogin(p_iNo, llLogin);
	}
	else if (enNotifyId == EN_PLUGIN_NOTIFY_GROUP_CHANGED)
	{
		std::string strGroup;
		if (!ReadString(*stPayload.refRoot,
				"Group", strGroup, p_refError))
		{
			return false;
		}
		m_clGroupQuoteService.InvalidateGroup(
			p_iNo, strGroup);
	}
	else if (enNotifyId == EN_PLUGIN_NOTIFY_SYMBOL_CHANGED)
	{
		std::string strSymbol;
		if (!ReadString(*stPayload.refRoot,
				"Symbol", strSymbol, p_refError))
		{
			return false;
		}
		m_clGroupQuoteService.InvalidateSymbol(
			p_iNo, strSymbol);
	}
	else if (llAction ==
		EN_PLUGIN_NOTIFY_ACTION_DELETED)
	{
		// 持仓删除时先移除该 Login 的旧反向依赖；其余持仓会在下一轮 Profit 快照重新登记。
		std::int64_t llLogin = 0;
		if (!ReadInt64(*stPayload.refRoot,
				"Login", llLogin, p_refError) || llLogin <= 0)
		{
			if (p_refError.empty())
			{
				p_refError =
					"DERIVE_GROUP_QUOTE_LOGIN_INVALID: Login must be positive";
			}
			return false;
		}
		m_clGroupQuoteService.RemoveLogin(p_iNo, llLogin);
	}
	m_clGroupQuoteService.RequestReconcile();
	return true;
}

bool CMtDeriveServiceApp::ReconcileMinuteSymbols(
	std::uint16_t p_usVersion, std::int32_t p_iNo,
	std::string& p_refError)
{
	ST_MT_DERIVE_SOURCE_STATE_SNAPSHOT stState;
	if (!m_clStateStore.SnapshotSource(
			p_usVersion, p_iNo, stState, p_refError))
	{
		return false;
	}
	if (!stState.bReady)
	{
		p_refError =
			"DERIVE_MINUTE_SYMBOL_SNAPSHOT_NOT_READY: 1167 source is not installed";
		return false;
	}
	std::vector<std::string> aSymbol;
	for (const std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& refValue :
		stState.aSymbol)
	{
		if (!refValue)
		{
			p_refError = "DERIVE_MINUTE_SYMBOL_VALUE_NULL";
			return false;
		}
		const ST_PLUGIN_BINARY_VALUE* pSymbol =
			FindPluginBinaryField(*refValue, "Symbol");
		if (pSymbol == nullptr ||
			pSymbol->enType != EN_PLUGIN_BINARY_VALUE_STRING ||
			pSymbol->strStringValue.empty())
		{
			p_refError =
				"DERIVE_MINUTE_SYMBOL_FIELD_INVALID: Symbol must be a non-empty string";
			return false;
		}
		aSymbol.push_back(pSymbol->strStringValue);
	}
	return m_clM1Engine.ReconcileSymbols(
		p_usVersion, p_iNo, aSymbol, p_refError);
}

void CMtDeriveServiceApp::StateBootstrapThread()
{
	std::set<std::pair<std::uint16_t, std::int32_t>>
		setCompleted;
	std::map<std::pair<std::uint16_t, std::int32_t>,
		std::string> mapLastError;
	while (!m_bBootstrapStopping.load())
	{
		// 第一步：逐来源独立恢复，单个 MT 节点不可用不能阻止其他来源进入 READY。
		for (const ST_MT_DERIVE_SOURCE_CONFIG& refSource :
			m_stDeriveConfig.aSource)
		{
			if (m_bBootstrapStopping.load())
			{
				break;
			}
			const std::pair<std::uint16_t, std::int32_t>
				stSource(refSource.usVersion,
					refSource.iNo);
			if (!refSource.bEnable ||
				setCompleted.find(stSource) !=
					setCompleted.end())
			{
				continue;
			}
			std::string strError;
			if (LoadSourceSnapshot(refSource,
					strError))
			{
				setCompleted.insert(stSource);
				mapLastError.erase(stSource);
				if (HasClusterLease(refSource.usVersion, refSource.iNo) &&
					!SetClusterShardReady(refSource.usVersion, refSource.iNo,
						true, strError))
				{
					setCompleted.erase(stSource);
					mapLastError[stSource] = strError;
					continue;
				}
				MT_INFO(
					"derive state snapshot installed,version=%u,no=%d,ready=%zu",
					static_cast<unsigned int>(
						refSource.usVersion),
					refSource.iNo,
					setCompleted.size());
				continue;
			}

			// 依赖持续不可用时只记录错误变化，避免固定重试周期刷满日志。
			if (mapLastError[stSource] != strError)
			{
				mapLastError[stSource] = strError;
				MT_WARN(
					"derive state snapshot pending,version=%u,no=%d,detail=%s",
					static_cast<unsigned int>(
						refSource.usVersion),
					refSource.iNo,
					strError.c_str());
			}
		}

		ST_MT_DERIVE_STATE_STATUS stStatus;
		m_clStateStore.GetStatus(stStatus);
		if (stStatus.bReady)
		{
			MT_INFO(
				"derive authoritative state restored,sourceCount=%zu,accountCount=%zu,positionCount=%zu,symbolCount=%zu,groupCount=%zu,rateCount=%zu",
				stStatus.szSourceCount,
				stStatus.szAccountCount,
				stStatus.szPositionCount,
				stStatus.szSymbolCount,
				stStatus.szGroupCount,
				stStatus.szRateCount);
			break;
		}

		// 第二步：使用可中断等待，使右上角关闭、任务管理器结束前的控制台关闭流程或 Ctrl+C 能及时退出。
		std::unique_lock<std::mutex> clLock(
			m_clBootstrapMutex);
		m_clBootstrapCondition.wait_for(
			clLock,
			std::chrono::milliseconds(
				m_stDeriveConfig.stReliable.uiRetryMs),
			[this]()
			{
				return m_bBootstrapStopping.load();
			});
	}
}

bool CMtDeriveServiceApp::LoadSourceSnapshot(
	const ST_MT_DERIVE_SOURCE_CONFIG& p_refSource,
	std::string& p_refError)
{
	p_refError.clear();
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE>
		refAggregate;
	std::uint64_t ullCursor = 0;
	std::uint64_t ullSnapshotId = 0;
	std::uint64_t ullEventSequence = 0;
	for (std::size_t szPage = 0;
		szPage < MT_DERIVE_SNAPSHOT_MAX_PAGES;
		++szPage)
	{
		if (m_bBootstrapStopping.load())
		{
			p_refError =
				"DERIVE_SNAPSHOT_STOPPING: service is stopping";
			return false;
		}

		// 第一步：每页都校验来源、实体位和快照标识，跨页状态变化必须整轮重试。
		std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refPage;
		if (!CallSourceSnapshotPage(p_refSource,
				ullCursor, refPage, p_refError) ||
			!refPage ||
			refPage->enType !=
				EN_PLUGIN_BINARY_VALUE_OBJECT)
		{
			if (p_refError.empty())
			{
				p_refError =
					"DERIVE_SNAPSHOT_PAGE_INVALID: Query returned no object";
			}
			return false;
		}
		std::int64_t llVersion = 0;
		std::int64_t llNo = 0;
		std::uint64_t ullMask = 0;
		std::uint64_t ullPageSnapshotId = 0;
		std::uint64_t ullPageEventSequence = 0;
		std::uint64_t ullNextCursor = 0;
		bool bHasMore = false;
		if (!ReadInt64(*refPage, "Version",
				llVersion, p_refError) ||
			!ReadInt64(*refPage, "No",
				llNo, p_refError) ||
			!ReadUInt64(*refPage, "EntityMask",
				ullMask, p_refError) ||
			!ReadUInt64(*refPage, "SnapshotId",
				ullPageSnapshotId, p_refError) ||
			!ReadUInt64(*refPage, "EventSequence",
				ullPageEventSequence, p_refError) ||
			!ReadUInt64(*refPage, "NextCursor",
				ullNextCursor, p_refError) ||
			!ReadBool(*refPage, "HasMore",
				bHasMore, p_refError) ||
			llVersion != p_refSource.usVersion ||
			llNo != p_refSource.iNo ||
			ullMask != static_cast<std::uint64_t>(
				EN_DERIVE_STATE_ENTITY_ALL) ||
			ullPageSnapshotId == 0)
		{
			if (p_refError.empty())
			{
				p_refError =
					"DERIVE_SNAPSHOT_HEADER_MISMATCH: Query page does not match requested source";
			}
			return false;
		}

		if (!refAggregate)
		{
			refAggregate = refPage;
			ullSnapshotId = ullPageSnapshotId;
			ullEventSequence =
				ullPageEventSequence;
		}
		else
		{
			if (ullPageSnapshotId != ullSnapshotId ||
				ullPageEventSequence !=
					ullEventSequence ||
				!AppendSnapshotPage(*refAggregate,
					*refPage, p_refError))
			{
				if (p_refError.empty())
				{
					p_refError =
						"DERIVE_SNAPSHOT_CHANGED_DURING_PAGING: SnapshotId or EventSequence changed";
				}
				return false;
			}
		}

		// 第二步：最后一页安装完整快照；安装过程会合并高水位后的可靠增量。
		if (!bHasMore)
		{
			if (!m_clStateStore.InstallSnapshot(
					p_refSource.usVersion,
					p_refSource.iNo,
					*refAggregate, p_refError))
			{
				return false;
			}
			return ReconcileMinuteSymbols(
				p_refSource.usVersion,
				p_refSource.iNo, p_refError);
		}
		if (ullNextCursor <= ullCursor)
		{
			p_refError =
				"DERIVE_SNAPSHOT_CURSOR_NOT_ADVANCED: Query returned HasMore without a larger cursor";
			return false;
		}
		ullCursor = ullNextCursor;
	}
	p_refError =
		"DERIVE_SNAPSHOT_PAGE_LIMIT_EXCEEDED: response exceeded 100000 pages";
	return false;
}

bool CMtDeriveServiceApp::CallSourceSnapshotPage(
	const ST_MT_DERIVE_SOURCE_CONFIG& p_refSource,
	std::uint64_t p_ullCursor,
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE>& p_refPage,
	std::string& p_refError)
{
	p_refPage.reset();
	p_refError.clear();
	if (GetCloudNetApi() == nullptr)
	{
		p_refError =
			"DERIVE_SNAPSHOT_NETWORK_NOT_READY: CloudNet API is null";
		return false;
	}

	// 第一步：1167 请求固定拉取全部收益依赖实体，分页只由 Cursor 和 Limit 控制。
	std::shared_ptr<ST_PLUGIN_BINARY_VALUE> refRequest =
		MakeValue(EN_PLUGIN_BINARY_VALUE_OBJECT,
			p_refError);
	if (!refRequest ||
		!AddInt64(*refRequest, "Version",
			p_refSource.usVersion, p_refError) ||
		!AddInt64(*refRequest, "No",
			p_refSource.iNo, p_refError) ||
		!AddUInt64(*refRequest, "EntityMask",
			static_cast<std::uint64_t>(
				EN_DERIVE_STATE_ENTITY_ALL),
			p_refError) ||
		!AddUInt64(*refRequest, "Cursor",
			p_ullCursor, p_refError) ||
		!AddUInt64(*refRequest, "Limit",
			MT_DERIVE_SNAPSHOT_PAGE_SIZE,
			p_refError))
	{
		return false;
	}
	std::vector<unsigned char> aRequest;
	if (!EncodeQueryBinaryRequest(*refRequest,
			aRequest, p_refError) ||
		aRequest.size() >
			static_cast<std::size_t>(
				(std::numeric_limits<int>::max)()))
	{
		if (p_refError.empty())
		{
			p_refError =
				"DERIVE_SNAPSHOT_REQUEST_TOO_LARGE: encoded request exceeds int range";
		}
		return false;
	}

	// 第二步：同步 RPC 只在恢复线程执行，不占用 Ice 服务请求线程。
	ST_CLOUD_NET_BINARY_CALL stCall;
	stCall.lSynId = GetTimestampMs();
	stCall.lFuncId =
		EN_PLUGIN_FUNC_QUERY_DERIVE_STATE_SNAPSHOT;
	stCall.stPayload.pBuffer = aRequest.data();
	stCall.stPayload.iLen =
		static_cast<int>(aRequest.size());
	ST_CLOUD_NET_BINARY_RESULT* pResult =
		CallBinarySync(GetCloudNetApi(),
			MT_DERIVE_QUERY_CONNECTION, &stCall);
	if (pResult == nullptr)
	{
		p_refError =
			"DERIVE_SNAPSHOT_CALL_FAILED: conn=MtQueryService, code=" +
			std::to_string(GetLastErrorCode(
				GetCloudNetApi())) + ", detail=" +
			GetLastErrorDetail(GetCloudNetApi());
		return false;
	}
	const std::unique_ptr<ST_CLOUD_NET_BINARY_RESULT,
		void (*)(ST_CLOUD_NET_BINARY_RESULT*)> clResult(
			pResult, FreeBinaryResult);
	if (pResult->iErrorCode != 0 ||
		pResult->lRetVal < 0 ||
		pResult->stPayload.iLen <= 0 ||
		pResult->stPayload.pBuffer == nullptr)
	{
		p_refError =
			"DERIVE_SNAPSHOT_TRANSPORT_ERROR: conn=MtQueryService, networkCode=" +
			std::to_string(pResult->iErrorCode) +
			", ret=" +
			std::to_string(pResult->lRetVal) +
			", detail=" + pResult->szErrInfo;
		return false;
	}

	// 第三步：业务错误保留 Query 的详细英文原因，便于区分数据源未就绪和协议损坏。
	ST_PLUGIN_BINARY_DOCUMENT stDocument;
	if (!DecodeQueryBinaryDocument(
			pResult->stPayload.pBuffer,
			static_cast<std::size_t>(
				pResult->stPayload.iLen),
			stDocument, p_refError) ||
		!stDocument.refRoot)
	{
		return false;
	}
	if (stDocument.iCode !=
		EN_TERMINAL_ERROR_OK)
	{
		p_refError =
			"DERIVE_SNAPSHOT_REMOTE_ERROR: code=" +
			std::to_string(stDocument.iCode) +
			", detail=" + stDocument.strMessage;
		return false;
	}
	p_refPage = stDocument.refRoot;
	return true;
}

bool CMtDeriveServiceApp::SetEncodedResponse(
	const std::vector<unsigned char>& p_refPayload,
	std::int32_t p_iBusinessCode,
	ST_CLOUD_NET_BINARY_RESULT* p_pResult,
	const char* p_szContext)
{
	if (p_pResult == nullptr ||
		p_refPayload.size() >
			static_cast<std::size_t>(
				(std::numeric_limits<int>::max)()))
	{
		if (p_pResult != nullptr)
		{
			const std::string strError =
				std::string(p_szContext != nullptr ?
					p_szContext : "DERIVE_RESPONSE") +
				"_TOO_LARGE: encoded response exceeds int range";
			SetBinaryResultError(p_pResult,
				EN_TERMINAL_ERROR_DERIVE_SNAPSHOT_TOO_LARGE,
				strError.c_str());
		}
		return true;
	}
	p_pResult->lParam = p_iBusinessCode;
	p_pResult->wParam =
		static_cast<long long>(p_refPayload.size());
	SetBinaryResultPayload(p_pResult,
		p_refPayload.empty() ? nullptr :
			p_refPayload.data(),
		p_refPayload.size());
	return true;
}

bool CMtDeriveServiceApp::PublishProfitSnapshot(
	const ST_DERIVE_PROFIT_SNAPSHOT_RESPONSE& p_refResponse,
	std::string& p_refError)
{
	p_refError.clear();
	if (p_refResponse.iCode != EN_TERMINAL_ERROR_OK ||
		p_refResponse.enState != EN_DERIVE_SERVICE_STATE_READY)
	{
		p_refError =
			"DERIVE_PROFIT_NOTIFY_STATE_INVALID: only a READY successful snapshot may be published";
		return false;
	}
	// 空租约分片无需发送通知；Gateway 在 1184 中仍能得到合法空数组。
	if (p_refResponse.aAccount.empty() &&
		p_refResponse.aPosition.empty())
	{
		return true;
	}
	std::uint16_t usVersion = 0;
	std::int32_t iNo = 0;
	bool bSourceSet = false;
	for (const ST_DERIVE_ACCOUNT_PROFIT& refAccount : p_refResponse.aAccount)
	{
		if (!IsClusterOwner(refAccount.usPlatformVersion, refAccount.iSourceNo))
		{
			p_refError =
				"SERVICE_NOT_OWNER: profit snapshot contains a standby Version+No";
			return false;
		}
		if (!bSourceSet)
		{
			usVersion = refAccount.usPlatformVersion;
			iNo = refAccount.iSourceNo;
			bSourceSet = true;
		}
		else if (usVersion != refAccount.usPlatformVersion ||
			iNo != refAccount.iSourceNo)
		{
			p_refError =
				"DERIVE_PROFIT_NOTIFY_SOURCE_MIXED: one notification may contain only one Version+No";
			return false;
		}
	}
	for (const ST_DERIVE_POSITION_PROFIT& refPosition : p_refResponse.aPosition)
	{
		if (!IsClusterOwner(refPosition.usPlatformVersion, refPosition.iSourceNo))
		{
			p_refError =
				"SERVICE_NOT_OWNER: position profit contains a standby Version+No";
			return false;
		}
		if (!bSourceSet)
		{
			usVersion = refPosition.usPlatformVersion;
			iNo = refPosition.iSourceNo;
			bSourceSet = true;
		}
		else if (usVersion != refPosition.usPlatformVersion ||
			iNo != refPosition.iSourceNo)
		{
			p_refError =
				"DERIVE_PROFIT_NOTIFY_SOURCE_MIXED: one notification may contain only one Version+No";
			return false;
		}
	}
	std::lock_guard<std::mutex> clSnapshotLock(
		m_aProfitSnapshotMutex[SelectNotifyMutex(usVersion, iNo)]);
	std::uint64_t ullSequence =
		m_ullProfitNotifySequence.fetch_add(1) + 1U;
	if (ullSequence == 0)
	{
		ullSequence =
			m_ullProfitNotifySequence.fetch_add(1) + 1U;
	}
	ST_DERIVE_PROFIT_SNAPSHOT_RESPONSE stNotify =
		p_refResponse;
	stNotify.ullEventSequence = ullSequence;
	std::vector<unsigned char> aPayload;
	if (!EncodeDeriveProfitSnapshotResponse(
			stNotify, aPayload, p_refError))
	{
		return false;
	}
	// 1251 属于可合并收益通知；最新快照会覆盖同账号旧值，可靠交易状态仍由 122x/124x 单独承载。
	return PublishPluginNotification(
		EN_PLUGIN_NOTIFY_PROFIT_CHANGED,
		EN_PLUGIN_NOTIFY_MODE_BEST_EFFORT,
		EN_PLUGIN_NOTIFY_ACTION_UPDATED,
		ullSequence, GetTimestampMs(),
		aPayload.data(), aPayload.size(),
		p_refError);
}

void CMtDeriveServiceApp::PublishChangedM1(
	const std::vector<ST_DERIVE_M1_BAR>& p_refChangedBar)
{
	// 同一批次先按来源、品种和分钟保留最终状态；0=实时直接发布，正间隔进入最新槽。
	typedef std::tuple<std::uint16_t, std::int32_t,
		std::string, std::int64_t> BAR_KEY;
	std::map<BAR_KEY, ST_DERIVE_M1_BAR> mapChanged;
	for (const ST_DERIVE_M1_BAR& refBar :
		p_refChangedBar)
	{
		if (!IsClusterOwner(refBar.usPlatformVersion, refBar.iSourceNo))
		{
			continue;
		}
		mapChanged[std::make_tuple(
			refBar.usPlatformVersion,
			refBar.iSourceNo, refBar.strSymbol,
			refBar.llMinute)] = refBar;
	}
	std::vector<ST_DERIVE_M1_BAR> aImmediate;
	for (const auto& refItem : mapChanged)
	{
		const ST_DERIVE_M1_BAR& refBar = refItem.second;
		const std::uint32_t uiInterval = GetM1PublishCoalesceMs(
			refBar.usPlatformVersion, refBar.iSourceNo);
		if (uiInterval == 0)
		{
			aImmediate.push_back(refBar);
			continue;
		}
		const auto stKey = std::make_tuple(refBar.usPlatformVersion,
			refBar.iSourceNo, refBar.strSymbol);
		std::lock_guard<std::mutex> clLock(m_clM1PublishMutex);
		const auto itMinute = m_mapLastM1Minute.find(stKey);
		if (itMinute == m_mapLastM1Minute.end() ||
			itMinute->second != refBar.llMinute)
		{
			const auto itPending = m_mapPendingM1.find(stKey);
			if (itPending != m_mapPendingM1.end())
			{
				aImmediate.push_back(itPending->second.first);
				m_mapPendingM1.erase(itPending);
			}
			aImmediate.push_back(refBar);
			m_mapLastM1Minute[stKey] = refBar.llMinute;
			continue;
		}
		const auto itPending = m_mapPendingM1.find(stKey);
		if (itPending == m_mapPendingM1.end())
		{
			m_mapPendingM1[stKey] = std::make_pair(refBar,
				std::chrono::steady_clock::now() +
					std::chrono::milliseconds(uiInterval));
		}
		else
		{
			itPending->second.first = refBar;
		}
		m_clM1PublishCondition.notify_one();
	}
	for (const ST_DERIVE_M1_BAR& refBar : aImmediate)
	{
		PublishM1Bar(refBar);
	}
}

std::uint32_t CMtDeriveServiceApp::GetM1PublishCoalesceMs(
	std::uint16_t p_usVersion, std::int32_t p_iNo) const
{
	for (const ST_MT_DERIVE_SOURCE_CONFIG& refSource : m_stDeriveConfig.aSource)
	{
		if (refSource.usVersion == p_usVersion && refSource.iNo == p_iNo)
		{
			return refSource.uiM1PublishCoalesceMs;
		}
	}
	return 0;
}

void CMtDeriveServiceApp::PublishM1Bar(const ST_DERIVE_M1_BAR& p_refBar)
{
	std::lock_guard<std::mutex> clSnapshotLock(
		m_aM1SnapshotMutex[SelectNotifyMutex(
			p_refBar.usPlatformVersion, p_refBar.iSourceNo)]);
	ST_DERIVE_M1_SNAPSHOT_RESPONSE stNotify;
	stNotify.enState = EN_DERIVE_SERVICE_STATE_READY;
	stNotify.iCode = EN_TERMINAL_ERROR_OK;
	stNotify.strMessage = "OK";
	stNotify.aBar.push_back(p_refBar);
	std::uint64_t ullSequence = m_ullM1NotifySequence.fetch_add(1) + 1U;
	if (ullSequence == 0)
	{
		ullSequence = m_ullM1NotifySequence.fetch_add(1) + 1U;
	}
	stNotify.ullEventSequence = ullSequence;
	std::vector<unsigned char> aPayload;
	std::string strError;
	if (!EncodeDeriveM1SnapshotResponse(stNotify, aPayload, strError))
	{
		MT_ERROR(
			"derive M1 notify encode failed,version=%u,no=%d,symbol=%s,minute=%lld,detail=%s",
			static_cast<unsigned int>(p_refBar.usPlatformVersion),
			p_refBar.iSourceNo, p_refBar.strSymbol.c_str(),
			static_cast<long long>(p_refBar.llMinute), strError.c_str());
		return;
	}
	if (!PublishPluginNotification(EN_PLUGIN_NOTIFY_KLINE_CHANGED,
			EN_PLUGIN_NOTIFY_MODE_BEST_EFFORT,
			EN_PLUGIN_NOTIFY_ACTION_UPDATED, ullSequence, GetTimestampMs(),
			aPayload.data(), aPayload.size(), strError))
	{
		MT_ERROR(
			"derive M1 notify publish failed,version=%u,no=%d,symbol=%s,minute=%lld,sequence=%llu,detail=%s",
			static_cast<unsigned int>(p_refBar.usPlatformVersion),
			p_refBar.iSourceNo, p_refBar.strSymbol.c_str(),
			static_cast<long long>(p_refBar.llMinute),
			static_cast<unsigned long long>(ullSequence), strError.c_str());
	}
}

void CMtDeriveServiceApp::M1PublishThread()
{
	for (;;)
	{
		ST_DERIVE_M1_BAR stBar;
		{
			std::unique_lock<std::mutex> clLock(m_clM1PublishMutex);
			for (;;)
			{
				if (m_bM1PublishStopping && m_mapPendingM1.empty())
				{
					return;
				}
				if (m_mapPendingM1.empty())
				{
					m_clM1PublishCondition.wait(clLock);
					continue;
				}
				auto itSelected = m_mapPendingM1.begin();
				for (auto it = m_mapPendingM1.begin();
					it != m_mapPendingM1.end(); ++it)
				{
					if (it->second.second < itSelected->second.second)
					{
						itSelected = it;
					}
				}
				if (!m_bM1PublishStopping &&
					itSelected->second.second > std::chrono::steady_clock::now())
				{
					m_clM1PublishCondition.wait_until(clLock,
						itSelected->second.second);
					continue;
				}
				stBar = itSelected->second.first;
				m_mapPendingM1.erase(itSelected);
				break;
			}
		}
		PublishM1Bar(stBar);
	}
}

void CMtDeriveServiceApp::OnClusterRoleChanged(
	const ST_CLUSTER_LEASE& p_refLease)
{
	CServiceApplication::OnClusterRoleChanged(p_refLease);
	if (p_refLease.usPlatformVersion == 5U &&
		p_refLease.enRole != EN_CLUSTER_ROLE_OWNER)
	{
		// RECOVERING 表示新租约代次，Standby/Degraded 表示失租；两者都必须丢弃旧 Owner 校准状态。
		m_clGroupQuoteService.ClearNode(
			p_refLease.iSourceNo,
			p_refLease.enRole == EN_CLUSTER_ROLE_RECOVERING ?
				"cluster recovery generation changed" :
				"cluster owner lease lost");
	}
	if (!m_bStarted.load() ||
		p_refLease.enRole != EN_CLUSTER_ROLE_RECOVERING)
	{
		return;
	}
	ST_MT_DERIVE_STATE_STATUS stStatus;
	m_clStateStore.GetStatus(stStatus);
	if (m_stDeriveConfig.stReliable.bEnable && !stStatus.bReady)
	{
		// 1167 快照线程会在该来源安装完成后置 READY，此处保持 fail-closed。
		return;
	}
	std::string strError;
	if (!SetClusterShardReady(p_refLease.usPlatformVersion,
			p_refLease.iSourceNo, true, strError))
	{
		MT_ERROR(
			"derive shard activation failed,version=%u,no=%d,generation=%llu,detail=%s",
			static_cast<unsigned int>(p_refLease.usPlatformVersion),
			p_refLease.iSourceNo,
			static_cast<unsigned long long>(p_refLease.ullLeaseGeneration),
			strError.c_str());
	}
}
