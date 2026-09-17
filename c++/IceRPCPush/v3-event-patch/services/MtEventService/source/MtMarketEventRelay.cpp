#include "MtMarketEventRelay.h"

#include "ClientDataBinaryProtocol.h"
#include "Log.h"
#include "QuoteBinaryProtocol.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <exception>
#include <map>
#include <thread>
#include <utility>

namespace
{
	// 已校验项拥有来源元数据和完整正文副本，离开 CloudNet 回调后不引用底层缓冲区。
	struct ST_MT_MARKET_EVENT_ITEM
	{
		ST_PLUGIN_NOTIFY_META stMeta;        // Quote 写入的原始通知元数据。
		std::vector<unsigned char> aPayload; // 完整 ClientData Binary 正文。

		ST_MT_MARKET_EVENT_ITEM()
			: stMeta()
			, aPayload()
		{
		}
	};

	// 分片内逐来源连续性状态，只在持有当前分片锁时读写。
	struct ST_MT_MARKET_SOURCE_WATERMARK
	{
		std::uint64_t ullSourceEpoch; // 最近接受的来源代次。
		std::uint64_t ullLastSequence;// 最近接受的入口序号。
		std::uint64_t ullGapCount;    // 当前进程观察到的缺口累计数。

		ST_MT_MARKET_SOURCE_WATERMARK()
			: ullSourceEpoch(0)
			, ullLastSequence(0)
			, ullGapCount(0)
		{
		}
	};

	// 返回当前 Unix 毫秒，用于高频诊断限频。
	std::int64_t GetNowMs()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
	}

	// 严格校验 Quote 1211 的通知元数据、ClientData 外层和 Quote v1 正文。
	bool ValidateMarketEvent(const ST_PLUGIN_NOTIFY_META& p_refMeta,
		const unsigned char* p_pPayload, std::size_t p_szPayloadLen,
		std::uint16_t& p_refVersion, std::int32_t& p_refNo,
		std::uint64_t& p_refSourceEpoch, std::uint64_t& p_refSequence,
		std::string& p_refError)
	{
		if (p_refMeta.usSourcePlugin != EN_PLUGIN_ID_MT_QUOTE_SERVICE ||
			p_refMeta.usNotifyMode != EN_PLUGIN_NOTIFY_MODE_BEST_EFFORT ||
			p_refMeta.usNotifyAction != EN_PLUGIN_NOTIFY_ACTION_UPDATED)
		{
			p_refError =
				"EVENT_QUOTE_META_INVALID: source must be MtQuoteService with BEST_EFFORT and UPDATED";
			return false;
		}
		if (p_refMeta.ullSequence == 0 || p_refMeta.llTimestampMs <= 0 ||
			p_refMeta.uiPayloadLen != p_szPayloadLen)
		{
			p_refError =
				"EVENT_QUOTE_META_SEQUENCE_INVALID: sequence, timestamp and payload length must be valid";
			return false;
		}

		ST_CLIENT_DATA_BINARY_EVENT stEvent;
		if (!DecodeClientDataBinaryEvent(p_pPayload,
			p_szPayloadLen, stEvent, p_refError))
		{
			p_refError = "EVENT_QUOTE_CLIENT_DATA_INVALID: " + p_refError;
			return false;
		}
		if (stEvent.ullTopic != EN_CLIENT_DATA_BINARY_TOPIC_QUOTE ||
			!stEvent.refData ||
			stEvent.refData->enType != EN_PLUGIN_BINARY_VALUE_BYTES)
		{
			p_refError =
				"EVENT_QUOTE_TOPIC_INVALID: 1211 requires Quote topic and BYTES data";
			return false;
		}

		ST_QUOTE_BINARY_TICK stTick;
		if (!DecodeQuoteBinaryTick(stEvent.refData->aByteValue.data(),
			stEvent.refData->aByteValue.size(), stTick, p_refError))
		{
			p_refError = "EVENT_QUOTE_PAYLOAD_INVALID: " + p_refError;
			return false;
		}
		if (stEvent.iSourceVersion != stTick.usPlatformVersion ||
			stEvent.iSourceNo != stTick.iSourceNo ||
			stEvent.strSymbol != stTick.strSymbol ||
			stEvent.ullSequence != stTick.ullIngressSequence ||
			stEvent.llTimestampMs != stTick.llIngressTimeMs ||
			p_refMeta.ullSequence != stEvent.ullSequence ||
			p_refMeta.llTimestampMs != stEvent.llTimestampMs)
		{
			p_refError =
				"EVENT_QUOTE_METADATA_MISMATCH: notify metadata, ClientData and Quote v1 fields are inconsistent";
			return false;
		}
		p_refVersion = stTick.usPlatformVersion;
		p_refNo = stTick.iSourceNo;
		p_refSourceEpoch = stTick.ullSourceEpoch;
		p_refSequence = stTick.ullIngressSequence;
		return true;
	}
}

// 单个分片拥有独立锁、FIFO、来源水位、计数和工作线程，结构不可复制。
struct ST_MT_MARKET_EVENT_SHARD
{
	std::mutex clMutex;                  // 保护队列、水位、计数和停止标记。
	std::condition_variable clCondition; // 唤醒当前分片工作线程。
	std::deque<ST_MT_MARKET_EVENT_ITEM> deqEvent; // 当前分片 FIFO。
	std::thread clWorker;                // 当前分片唯一发布线程。
	std::size_t szCapacity;              // 分片容量，0 表示不主动容量淘汰。
	std::map<std::pair<std::uint16_t, std::int32_t>,
		ST_MT_MARKET_SOURCE_WATERMARK> mapSequence; // 来源到连续性水位。
	std::uint64_t ullDuplicateCount;     // 当前分片重复序号数量。
	std::uint64_t ullOutOfOrderCount;    // 当前分片回退序号数量。
	std::uint64_t ullEpochResetCount;    // 当前分片 Epoch 变化数量。
	std::uint64_t ullSequenceGapCount;   // 当前分片序号缺口数量。
	std::uint64_t ullCapacityDroppedCount; // 当前分片淘汰最旧 Tick 数量。
	std::uint64_t ullFanoutCount;        // 当前分片成功扇出数量。
	std::uint64_t ullPublishFailedCount; // 当前分片发布失败数量。
	bool bStopRequested;                 // 排空队列后退出线程。

	ST_MT_MARKET_EVENT_SHARD()
		: clMutex()
		, clCondition()
		, deqEvent()
		, clWorker()
		, szCapacity(0)
		, mapSequence()
		, ullDuplicateCount(0)
		, ullOutOfOrderCount(0)
		, ullEpochResetCount(0)
		, ullSequenceGapCount(0)
		, ullCapacityDroppedCount(0)
		, ullFanoutCount(0)
		, ullPublishFailedCount(0)
		, bStopRequested(false)
	{
	}
};

ST_MT_MARKET_EVENT_DIAGNOSTICS::ST_MT_MARKET_EVENT_DIAGNOSTICS()
	: ullQueueCapacity(0)
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

IMtMarketEventRelaySink::~IMtMarketEventRelaySink()
{
}

CMtMarketEventRelay::CMtMarketEventRelay()
	: m_clLifecycleMutex()
	, m_stConfig()
	, m_pSink(nullptr)
	, m_aShard()
	, m_bAccepting(false)
	, m_llLastDropLogMs(0)
	, m_ullReceivedCount(0)
	, m_ullValidationRejectedCount(0)
{
}

CMtMarketEventRelay::~CMtMarketEventRelay()
{
	Stop();
}

bool CMtMarketEventRelay::Start(
	const ST_MT_MARKET_EVENT_CONFIG& p_refConfig,
	IMtMarketEventRelaySink* p_pSink, std::string& p_refError)
{
	Stop();
	p_refError.clear();
	if (p_pSink == nullptr || p_refConfig.uiWorkerThreads == 0 ||
		(p_refConfig.szQueueCapacity != 0 &&
		 p_refConfig.szQueueCapacity < 128))
	{
		p_refError =
			"EVENT_RELAY_START_INVALID: sink, workerThreads or queueCapacity is invalid";
		return false;
	}

	// 第一步：创建固定分片；正容量余数平均分配，零容量原样传递给全部分片。
	{
		std::lock_guard<std::mutex> clLifecycleLock(m_clLifecycleMutex);
		m_stConfig = p_refConfig;
		m_pSink = p_pSink;
		m_ullReceivedCount.store(0);
		m_ullValidationRejectedCount.store(0);
		m_llLastDropLogMs.store(0);
		for (unsigned int uiIndex = 0;
			uiIndex < p_refConfig.uiWorkerThreads; ++uiIndex)
		{
			std::unique_ptr<ST_MT_MARKET_EVENT_SHARD> ptrShard(
				new ST_MT_MARKET_EVENT_SHARD);
			if (p_refConfig.szQueueCapacity != 0)
			{
				ptrShard->szCapacity = p_refConfig.szQueueCapacity /
					p_refConfig.uiWorkerThreads +
					(uiIndex < p_refConfig.szQueueCapacity %
					 p_refConfig.uiWorkerThreads ? 1U : 0U);
			}
			m_aShard.push_back(std::move(ptrShard));
		}
	}

	// 第二步：全部线程创建成功后开放 Submit；异常路径由 Stop 回收已创建线程。
	try
	{
		for (const std::unique_ptr<ST_MT_MARKET_EVENT_SHARD>& refShard :
			m_aShard)
		{
			refShard->clWorker = std::thread(
				&CMtMarketEventRelay::Worker, this, refShard.get());
		}
	}
	catch (const std::exception& p_refException)
	{
		p_refError = std::string(
			"EVENT_RELAY_THREAD_START_FAILED: detail=") +
			p_refException.what();
		Stop();
		return false;
	}
	m_bAccepting.store(true);
	return true;
}

void CMtMarketEventRelay::Stop()
{
	m_bAccepting.store(false);
	{
		// 第一步：生命周期锁阻止已通过初始检查的 Submit 访问正在停止的分片。
		std::lock_guard<std::mutex> clLifecycleLock(m_clLifecycleMutex);
		for (const std::unique_ptr<ST_MT_MARKET_EVENT_SHARD>& refShard :
			m_aShard)
		{
			std::lock_guard<std::mutex> clQueueLock(refShard->clMutex);
			refShard->bStopRequested = true;
			refShard->clCondition.notify_all();
		}
	}

	// 第二步：Worker 排空队列期间 Sink 保持有效，全部 join 后再释放非拥有指针。
	for (const std::unique_ptr<ST_MT_MARKET_EVENT_SHARD>& refShard : m_aShard)
	{
		if (refShard->clWorker.joinable() &&
			refShard->clWorker.get_id() != std::this_thread::get_id())
		{
			refShard->clWorker.join();
		}
	}
	{
		std::lock_guard<std::mutex> clLifecycleLock(m_clLifecycleMutex);
		m_aShard.clear();
		m_pSink = nullptr;
	}
}

bool CMtMarketEventRelay::Submit(
	const ST_PLUGIN_NOTIFY_META& p_refMeta,
	const unsigned char* p_pPayload, std::size_t p_szPayloadLen,
	std::string& p_refError)
{
	p_refError.clear();
	if ((p_pPayload == nullptr && p_szPayloadLen != 0) ||
		!m_bAccepting.load())
	{
		p_refError =
			"EVENT_RELAY_NOT_ACCEPTING: relay is stopped or payload pointer is invalid";
		return false;
	}
	m_ullReceivedCount.fetch_add(1);

	// 第一步：回调线程完成三层校验，非法行情不会占用中继队列。
	std::uint16_t usVersion = 0;
	std::int32_t iNo = 0;
	std::uint64_t ullSourceEpoch = 0;
	std::uint64_t ullSequence = 0;
	if (!ValidateMarketEvent(p_refMeta, p_pPayload, p_szPayloadLen,
		usVersion, iNo, ullSourceEpoch, ullSequence, p_refError))
	{
		m_ullValidationRejectedCount.fetch_add(1);
		return false;
	}
	ST_MT_MARKET_EVENT_ITEM stItem;
	stItem.stMeta = p_refMeta;
	stItem.aPayload.assign(p_pPayload, p_pPayload + p_szPayloadLen);

	// 第二步：生命周期锁固定分片数组；分片锁串行提交同一来源连续性和 FIFO。
	std::lock_guard<std::mutex> clLifecycleLock(m_clLifecycleMutex);
	if (!m_bAccepting.load() || m_aShard.empty())
	{
		p_refError =
			"EVENT_RELAY_STOPPING: relay stopped while notification was being validated";
		return false;
	}
	ST_MT_MARKET_EVENT_SHARD* pShard =
		m_aShard[SelectShard(usVersion, iNo)].get();
	std::lock_guard<std::mutex> clQueueLock(pShard->clMutex);
	if (pShard->bStopRequested)
	{
		p_refError = "EVENT_RELAY_STOPPING: target shard is stopping";
		return false;
	}

	const std::pair<std::uint16_t, std::int32_t> stSourceKey =
		std::make_pair(usVersion, iNo);
	ST_MT_MARKET_SOURCE_WATERMARK& refWatermark =
		pShard->mapSequence[stSourceKey];
	if (refWatermark.ullSourceEpoch == ullSourceEpoch &&
		refWatermark.ullLastSequence != 0)
	{
		if (ullSequence == refWatermark.ullLastSequence)
		{
			++pShard->ullDuplicateCount;
			p_refError =
				"EVENT_RELAY_QUOTE_DUPLICATE: duplicate source sequence was rejected";
			return false;
		}
		if (ullSequence < refWatermark.ullLastSequence)
		{
			++pShard->ullOutOfOrderCount;
			p_refError =
				"EVENT_RELAY_QUOTE_OUT_OF_ORDER: source sequence moved backwards";
			return false;
		}
		if (ullSequence != refWatermark.ullLastSequence + 1U)
		{
			++pShard->ullSequenceGapCount;
			++refWatermark.ullGapCount;
			if (ShouldLogDrop(GetNowMs()))
			{
				MT_WARN(
					"market event source sequence gap,version=%u,no=%d,epoch=%llu,lastSequence=%llu,currentSequence=%llu",
					static_cast<unsigned int>(usVersion), iNo,
					static_cast<unsigned long long>(ullSourceEpoch),
					static_cast<unsigned long long>(refWatermark.ullLastSequence),
					static_cast<unsigned long long>(ullSequence));
			}
		}
	}
	else if (refWatermark.ullSourceEpoch != 0)
	{
		++pShard->ullEpochResetCount;
	}
	refWatermark.ullSourceEpoch = ullSourceEpoch;
	refWatermark.ullLastSequence = ullSequence;

	// 第三步：正容量满载时淘汰最旧 Tick；容量零从不执行容量淘汰。
	if (pShard->szCapacity != 0 &&
		pShard->deqEvent.size() >= pShard->szCapacity)
	{
		pShard->deqEvent.pop_front();
		++pShard->ullCapacityDroppedCount;
		if (ShouldLogDrop(GetNowMs()))
		{
			MT_WARN(
				"market event queue dropped oldest quote,version=%u,no=%d,shardCapacity=%zu,shardDropped=%llu",
				static_cast<unsigned int>(usVersion), iNo, pShard->szCapacity,
				static_cast<unsigned long long>(
					pShard->ullCapacityDroppedCount));
		}
	}
	pShard->deqEvent.push_back(std::move(stItem));
	pShard->clCondition.notify_one();
	return true;
}

void CMtMarketEventRelay::GetDiagnostics(
	ST_MT_MARKET_EVENT_DIAGNOSTICS& p_refDiagnostics) const
{
	p_refDiagnostics = ST_MT_MARKET_EVENT_DIAGNOSTICS();
	p_refDiagnostics.ullQueueCapacity = m_stConfig.szQueueCapacity;
	p_refDiagnostics.ullReceivedCount = m_ullReceivedCount.load();
	p_refDiagnostics.ullValidationRejectedCount =
		m_ullValidationRejectedCount.load();
	std::lock_guard<std::mutex> clLifecycleLock(m_clLifecycleMutex);
	for (const std::unique_ptr<ST_MT_MARKET_EVENT_SHARD>& refShard : m_aShard)
	{
		std::lock_guard<std::mutex> clQueueLock(refShard->clMutex);
		p_refDiagnostics.ullQueueDepth += refShard->deqEvent.size();
		p_refDiagnostics.ullDuplicateCount += refShard->ullDuplicateCount;
		p_refDiagnostics.ullOutOfOrderCount += refShard->ullOutOfOrderCount;
		p_refDiagnostics.ullEpochResetCount += refShard->ullEpochResetCount;
		p_refDiagnostics.ullSequenceGapCount += refShard->ullSequenceGapCount;
		p_refDiagnostics.ullCapacityDroppedCount +=
			refShard->ullCapacityDroppedCount;
		p_refDiagnostics.ullFanoutCount += refShard->ullFanoutCount;
		p_refDiagnostics.ullPublishFailedCount +=
			refShard->ullPublishFailedCount;
		for (const auto& refWatermark : refShard->mapSequence)
		{
			ST_EVENT_HEARTBEAT_SOURCE_STATUS stSource;
			stSource.usPlatformVersion = refWatermark.first.first;
			stSource.iSourceNo = refWatermark.first.second;
			stSource.ullSourceEpoch = refWatermark.second.ullSourceEpoch;
			stSource.ullLastSequence = refWatermark.second.ullLastSequence;
			stSource.ullGapCount = refWatermark.second.ullGapCount;
			p_refDiagnostics.aSource.push_back(stSource);
		}
	}
	std::sort(p_refDiagnostics.aSource.begin(),
		p_refDiagnostics.aSource.end(),
		[](const ST_EVENT_HEARTBEAT_SOURCE_STATUS& p_refLeft,
			const ST_EVENT_HEARTBEAT_SOURCE_STATUS& p_refRight)
		{
			return std::make_pair(p_refLeft.usPlatformVersion, p_refLeft.iSourceNo) <
				std::make_pair(p_refRight.usPlatformVersion, p_refRight.iSourceNo);
		});
}

void CMtMarketEventRelay::Worker(ST_MT_MARKET_EVENT_SHARD* p_pShard)
{
	if (p_pShard == nullptr)
	{
		return;
	}
	for (;;)
	{
		ST_MT_MARKET_EVENT_ITEM stItem;
		{
			std::unique_lock<std::mutex> clQueueLock(p_pShard->clMutex);
			p_pShard->clCondition.wait(clQueueLock,
				[p_pShard]()
				{
					return p_pShard->bStopRequested ||
						!p_pShard->deqEvent.empty();
				});
			if (p_pShard->deqEvent.empty())
			{
				if (p_pShard->bStopRequested)
				{
					break;
				}
				continue;
			}
			stItem = std::move(p_pShard->deqEvent.front());
			p_pShard->deqEvent.pop_front();
		}

		// Sink 异常和发布失败只影响当前在线 Tick，后续 Tick 继续处理并由计数暴露缺口。
		bool bPublished = false;
		std::string strError;
		try
		{
			bPublished = m_pSink != nullptr &&
				m_pSink->OnMarketEventReady(
					stItem.stMeta, stItem.aPayload, strError);
		}
		catch (const std::exception& p_refException)
		{
			strError = p_refException.what();
		}
		catch (...)
		{
			strError = "unknown exception";
		}
		{
			std::lock_guard<std::mutex> clQueueLock(p_pShard->clMutex);
			if (bPublished)
			{
				++p_pShard->ullFanoutCount;
			}
			else
			{
				++p_pShard->ullPublishFailedCount;
			}
		}
		if (!bPublished && ShouldLogDrop(GetNowMs()))
		{
			MT_ERROR(
				"market event local fanout failed,sourceSequence=%llu,payloadLen=%zu,detail=%s",
				static_cast<unsigned long long>(stItem.stMeta.ullSequence),
				stItem.aPayload.size(),
				strError.empty() ? "PublishPluginNotification returned false" :
					strError.c_str());
		}
	}
}

std::size_t CMtMarketEventRelay::SelectShard(
	std::uint16_t p_usVersion, std::int32_t p_iNo) const
{
	const std::uint64_t ullKey =
		static_cast<std::uint64_t>(p_usVersion) * 1315423911ULL +
		static_cast<std::uint32_t>(p_iNo);
	return static_cast<std::size_t>(ullKey % m_aShard.size());
}

bool CMtMarketEventRelay::ShouldLogDrop(std::int64_t p_llNowMs)
{
	const std::int64_t llInterval = static_cast<std::int64_t>(
		m_stConfig.uiDropLogIntervalMs);
	std::int64_t llPrevious = m_llLastDropLogMs.load();
	while (llPrevious == 0 || p_llNowMs - llPrevious >= llInterval)
	{
		if (m_llLastDropLogMs.compare_exchange_weak(llPrevious, p_llNowMs))
		{
			return true;
		}
	}
	return false;
}
