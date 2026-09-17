#include "MtDeriveTickConsumer.h"

#include "nats/nats.h"
#include "Log.h"

#include <chrono>
#include <exception>

// 单来源运行时由 Consumer 独占；JetStream 保证同一 Subject 的消息顺序。
struct ST_MT_DERIVE_TICK_SOURCE_RUNTIME
{
	std::uint16_t usVersion;             // MT 平台版本。
	std::int32_t iNo;                    // 同平台节点编号。
	std::string strSubject;              // 当前来源精确 Subject。
	std::string strDurable;              // 与业务分片绑定的稳定 durable 名称。
	natsSubscription* pSubscription;     // 当前进程 pull subscription。
	std::thread clWorker;                // 当前来源唯一 Fetch/Apply/ACK 线程。
	std::uint64_t ullMaxLeaseGeneration; // 已按流顺序接受的最大 fencing 代次。
	std::string strLastError;            // 最近已记录错误，用于日志去重。

	ST_MT_DERIVE_TICK_SOURCE_RUNTIME()
		: usVersion(0)
		, iNo(0)
		, strSubject()
		, strDurable()
		, pSubscription(nullptr)
		, clWorker()
		, ullMaxLeaseGeneration(0)
		, strLastError()
	{
	}
};

CMtDeriveTickConsumer::CMtDeriveTickConsumer()
	: m_stConfig()
	, m_fnIsOwner()
	, m_fnApply()
	, m_bStopping(true)
	, m_clWaitMutex()
	, m_clWaitCondition()
	, m_pConnection(nullptr)
	, m_pJetStream(nullptr)
	, m_aRuntime()
{
}

CMtDeriveTickConsumer::~CMtDeriveTickConsumer()
{
	Stop();
}

bool CMtDeriveTickConsumer::Start(
	const ST_MT_DERIVE_TICK_STREAM_CONFIG& p_refConfig,
	const std::vector<ST_MT_DERIVE_SOURCE_CONFIG>& p_refSource,
	const PFN_MT_DERIVE_TICK_IS_OWNER& p_refIsOwner,
	const PFN_MT_DERIVE_TICK_APPLY& p_refApply,
	std::string& p_refError)
{
	Stop();
	p_refError.clear();
	m_stConfig = p_refConfig;
	if (!m_stConfig.bEnable)
	{
		m_bStopping.store(false);
		return true;
	}
	if (m_stConfig.strNatsUrl.empty() || m_stConfig.strStream.empty() ||
		m_stConfig.strSubjectPrefix.empty() || !p_refIsOwner || !p_refApply)
	{
		p_refError =
			"DERIVE_TICK_CONSUMER_CONFIG_INVALID: NATS, stream, subject and callbacks are required";
		return false;
	}
	m_fnIsOwner = p_refIsOwner;
	m_fnApply = p_refApply;
	natsStatus enStatus = natsConnection_ConnectTo(
		&m_pConnection, m_stConfig.strNatsUrl.c_str());
	if (enStatus == NATS_OK)
	{
		enStatus = natsConnection_JetStream(
			&m_pJetStream, m_pConnection, nullptr);
	}
	if (enStatus != NATS_OK || m_pJetStream == nullptr)
	{
		p_refError = BuildNatsError("connect/JetStream",
			static_cast<int>(enStatus), 0);
		Stop();
		return false;
	}
	if (!ValidateStream(p_refError))
	{
		Stop();
		return false;
	}

	// 第一步：先建立全部 durable，保证线程启动后不修改来源容器和指针地址。
	for (const ST_MT_DERIVE_SOURCE_CONFIG& refSource : p_refSource)
	{
		if (!refSource.bEnable)
		{
			continue;
		}
		std::unique_ptr<ST_MT_DERIVE_TICK_SOURCE_RUNTIME> ptrRuntime(
			new ST_MT_DERIVE_TICK_SOURCE_RUNTIME());
		ptrRuntime->usVersion = refSource.usVersion;
		ptrRuntime->iNo = refSource.iNo;
		ptrRuntime->strSubject = m_stConfig.strSubjectPrefix +
			".v" + std::to_string(refSource.usVersion) +
			".n" + std::to_string(refSource.iNo);
		ptrRuntime->strDurable = "mt-derive-v" +
			std::to_string(refSource.usVersion) + "-n" +
			std::to_string(refSource.iNo);
		if (!CreateSubscription(*ptrRuntime, p_refError))
		{
			Stop();
			return false;
		}
		m_aRuntime.push_back(std::move(ptrRuntime));
	}
	if (m_aRuntime.empty())
	{
		p_refError =
			"DERIVE_TICK_CONSUMER_SOURCE_EMPTY: no enabled Version+No is configured";
		Stop();
		return false;
	}

	// 第二步：全部 durable 成功后再启动线程，失败路径不会留下部分消费实例。
	m_bStopping.store(false);
	try
	{
		for (const std::unique_ptr<ST_MT_DERIVE_TICK_SOURCE_RUNTIME>& ptrRuntime :
			m_aRuntime)
		{
			ptrRuntime->clWorker = std::thread(
				&CMtDeriveTickConsumer::SourceThread,
				this, ptrRuntime.get());
		}
	}
	catch (const std::exception& p_refException)
	{
		p_refError = std::string(
			"DERIVE_TICK_CONSUMER_THREAD_FAILED: detail=") +
			p_refException.what();
		Stop();
		return false;
	}
	return true;
}

void CMtDeriveTickConsumer::Stop()
{
	m_bStopping.store(true);
	m_clWaitCondition.notify_all();
	for (const std::unique_ptr<ST_MT_DERIVE_TICK_SOURCE_RUNTIME>& ptrRuntime :
		m_aRuntime)
	{
		if (ptrRuntime && ptrRuntime->clWorker.joinable() &&
			ptrRuntime->clWorker.get_id() != std::this_thread::get_id())
		{
			ptrRuntime->clWorker.join();
		}
	}
	for (const std::unique_ptr<ST_MT_DERIVE_TICK_SOURCE_RUNTIME>& ptrRuntime :
		m_aRuntime)
	{
		if (ptrRuntime && ptrRuntime->pSubscription != nullptr)
		{
			natsSubscription_Destroy(ptrRuntime->pSubscription);
			ptrRuntime->pSubscription = nullptr;
		}
	}
	m_aRuntime.clear();
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
	m_fnIsOwner = PFN_MT_DERIVE_TICK_IS_OWNER();
	m_fnApply = PFN_MT_DERIVE_TICK_APPLY();
}

void CMtDeriveTickConsumer::SourceThread(
	ST_MT_DERIVE_TICK_SOURCE_RUNTIME* p_pRuntime)
{
	if (p_pRuntime == nullptr)
	{
		return;
	}
	while (!m_bStopping.load())
	{
		// Standby 不发起 Fetch；租约转移后未 ACK 消息由同一业务 durable 的新 Owner 继续处理。
		if (!m_fnIsOwner ||
			!m_fnIsOwner(p_pRuntime->usVersion, p_pRuntime->iNo))
		{
			WaitRetry();
			continue;
		}
		natsMsgList stMessages;
		stMessages.Msgs = nullptr;
		stMessages.Count = 0;
		jsErrCode enJetStreamError = static_cast<jsErrCode>(0);
		const natsStatus enStatus = natsSubscription_Fetch(
			&stMessages, p_pRuntime->pSubscription,
			static_cast<int>(m_stConfig.uiFetchBatch),
			static_cast<std::int64_t>(m_stConfig.uiFetchWaitMs),
			&enJetStreamError);
		if (enStatus == NATS_TIMEOUT)
		{
			natsMsgList_Destroy(&stMessages);
			continue;
		}
		if (enStatus != NATS_OK)
		{
			const std::string strError = BuildNatsError(
				"natsSubscription_Fetch", static_cast<int>(enStatus),
				static_cast<int>(enJetStreamError));
			if (p_pRuntime->strLastError != strError)
			{
				p_pRuntime->strLastError = strError;
				MT_WARN("derive Tick stream fetch failed,version=%u,no=%d,detail=%s",
					static_cast<unsigned int>(p_pRuntime->usVersion),
					p_pRuntime->iNo, strError.c_str());
			}
			natsMsgList_Destroy(&stMessages);
			WaitRetry();
			continue;
		}

		for (int iIndex = 0; iIndex < stMessages.Count; ++iIndex)
		{
			natsMsg* pMessage = stMessages.Msgs[iIndex];
			if (pMessage == nullptr || m_bStopping.load() ||
				!m_fnIsOwner(p_pRuntime->usVersion, p_pRuntime->iNo))
			{
				if (pMessage != nullptr)
				{
					natsMsg_Nak(pMessage, nullptr);
				}
				continue;
			}
			ST_QUOTE_BINARY_TICK stTick;
			std::string strError;
			const char* pData = natsMsg_GetData(pMessage);
			const int iDataLen = natsMsg_GetDataLength(pMessage);
			bool bApplied = iDataLen > 0 && pData != nullptr &&
				DecodeQuoteBinaryTick(
					reinterpret_cast<const unsigned char*>(pData),
					static_cast<std::size_t>(iDataLen), stTick, strError) &&
				stTick.usPlatformVersion == p_pRuntime->usVersion &&
				stTick.iSourceNo == p_pRuntime->iNo;
			if (bApplied)
			{
				bApplied = m_fnApply(stTick, strError);
			}
			if (!bApplied)
			{
				natsMsg_NakWithDelay(pMessage,
					static_cast<std::int64_t>(m_stConfig.uiRetryMs) *
						1000000LL, nullptr);
				if (p_pRuntime->strLastError != strError)
				{
					p_pRuntime->strLastError = strError;
					MT_WARN("derive Tick stream apply retry,version=%u,no=%d,detail=%s",
						static_cast<unsigned int>(p_pRuntime->usVersion),
						p_pRuntime->iNo, strError.c_str());
				}
				continue;
			}
			enJetStreamError = static_cast<jsErrCode>(0);
			const natsStatus enAckStatus = natsMsg_AckSync(
				pMessage, nullptr, &enJetStreamError);
			if (enAckStatus != NATS_OK)
			{
				const std::string strAckError = BuildNatsError(
					"natsMsg_AckSync", static_cast<int>(enAckStatus),
					static_cast<int>(enJetStreamError));
				p_pRuntime->strLastError = strAckError;
				MT_WARN("derive Tick stream ACK failed,version=%u,no=%d,detail=%s",
					static_cast<unsigned int>(p_pRuntime->usVersion),
					p_pRuntime->iNo, strAckError.c_str());
			}
			else
			{
				p_pRuntime->strLastError.clear();
			}
		}
		natsMsgList_Destroy(&stMessages);
	}
}

bool CMtDeriveTickConsumer::CreateSubscription(
	ST_MT_DERIVE_TICK_SOURCE_RUNTIME& p_refRuntime,
	std::string& p_refError)
{
	jsSubOptions stOptions;
	jsSubOptions_Init(&stOptions);
	stOptions.Stream = m_stConfig.strStream.c_str();
	stOptions.Config.Durable = p_refRuntime.strDurable.c_str();
	stOptions.Config.AckPolicy = js_AckExplicit;
	stOptions.Config.DeliverPolicy = js_DeliverAll;
	stOptions.Config.MaxDeliver =
		static_cast<std::int64_t>(m_stConfig.uiMaxDeliver);
	stOptions.Config.AckWait =
		static_cast<std::int64_t>(m_stConfig.uiFetchWaitMs) *
		1000000LL * 30LL;
	jsErrCode enJetStreamError = static_cast<jsErrCode>(0);
	const natsStatus enStatus = js_PullSubscribe(
		&p_refRuntime.pSubscription,
		m_pJetStream, p_refRuntime.strSubject.c_str(),
		p_refRuntime.strDurable.c_str(), nullptr,
		&stOptions, &enJetStreamError);
	if (enStatus != NATS_OK || p_refRuntime.pSubscription == nullptr)
	{
		p_refError = BuildNatsError("js_PullSubscribe",
			static_cast<int>(enStatus),
			static_cast<int>(enJetStreamError));
		return false;
	}
	return true;
}

bool CMtDeriveTickConsumer::ValidateStream(std::string& p_refError)
{
	jsStreamInfo* pInfo = nullptr;
	jsErrCode enJetStreamError = static_cast<jsErrCode>(0);
	const natsStatus enStatus = js_GetStreamInfo(&pInfo,
		m_pJetStream, m_stConfig.strStream.c_str(), nullptr,
		&enJetStreamError);
	const std::string strExpected = m_stConfig.strSubjectPrefix + ".>";
	if (enStatus != NATS_OK || pInfo == nullptr ||
		pInfo->Config == nullptr ||
		pInfo->Config->Storage != js_FileStorage ||
		pInfo->Config->SubjectsLen != 1 ||
		pInfo->Config->Subjects == nullptr ||
		pInfo->Config->Subjects[0] == nullptr ||
		strExpected != pInfo->Config->Subjects[0])
	{
		if (pInfo != nullptr)
		{
			jsStreamInfo_Destroy(pInfo);
		}
		p_refError = enStatus == NATS_OK ?
			"DERIVE_TICK_STREAM_CONFIG_MISMATCH: stream storage or subject differs from Quote" :
			BuildNatsError("js_GetStreamInfo", static_cast<int>(enStatus),
				static_cast<int>(enJetStreamError));
		return false;
	}
	jsStreamInfo_Destroy(pInfo);
	return true;
}

void CMtDeriveTickConsumer::WaitRetry()
{
	std::unique_lock<std::mutex> clLock(m_clWaitMutex);
	m_clWaitCondition.wait_for(clLock,
		std::chrono::milliseconds(m_stConfig.uiRetryMs),
		[this]() { return m_bStopping.load(); });
}

std::string CMtDeriveTickConsumer::BuildNatsError(
	const char* p_szOperation, int p_iStatus,
	int p_iJetStreamError)
{
	return std::string("DERIVE_TICK_STREAM_NATS_FAILED: operation=") +
		(p_szOperation != nullptr ? p_szOperation : "unknown") +
		", status=" + std::to_string(p_iStatus) +
		", detail=" + natsStatus_GetText(
			static_cast<natsStatus>(p_iStatus)) +
		", jetStreamError=" + std::to_string(p_iJetStreamError);
}
