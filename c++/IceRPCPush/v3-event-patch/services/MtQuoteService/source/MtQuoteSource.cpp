#include "MtQuoteSource.h"

#include "Log.h"

#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <Windows.h>

#include "MT4ManagerAPI.h"
#include "MT5APIManager.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <exception>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace
{
	// 返回本机 Unix 毫秒时间；只记录接入时刻和诊断水位，不修改 MT 原始服务器时间。
	std::int64_t GetNowMs()
	{
		return static_cast<std::int64_t>(
			std::chrono::duration_cast<std::chrono::milliseconds>(
				std::chrono::system_clock::now().time_since_epoch()).count());
	}

	// 仅接受有限正数买卖价；Quote 不使用业务规则补造缺失价格。
	bool IsValidBidAsk(double p_dBid, double p_dAsk)
	{
		return std::isfinite(p_dBid) && std::isfinite(p_dAsk) &&
			p_dBid > 0.0 && p_dAsk > 0.0;
	}

	// 读取 SDK 定长 ANSI 缓冲区，返回独立字符串以隔离回调生命周期。
	std::string ReadFixedText(const char* p_pText, std::size_t p_szCapacity)
	{
		if (p_pText == nullptr || p_szCapacity == 0)
		{
			return std::string();
		}
		std::size_t szLength = 0;
		while (szLength < p_szCapacity && p_pText[szLength] != '\0')
		{
			++szLength;
		}
		return std::string(p_pText, szLength);
	}

	// 将 MT5 UTF-16 字符串转换为 UTF-8；非法内容返回空字符串并丢弃当前 Tick。
	std::string WideToUtf8(LPCWSTR p_pText)
	{
		if (p_pText == nullptr || p_pText[0] == L'\0')
		{
			return std::string();
		}
		const int iLength = WideCharToMultiByte(CP_UTF8,
			WC_ERR_INVALID_CHARS, p_pText, -1, nullptr, 0,
			nullptr, nullptr);
		if (iLength <= 1)
		{
			return std::string();
		}
		std::vector<char> aBuffer(static_cast<std::size_t>(iLength));
		if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
			p_pText, -1, aBuffer.data(), iLength, nullptr, nullptr) <= 0)
		{
			return std::string();
		}
		return std::string(aBuffer.data());
	}

	// 将 UTF-8 配置转换为 MT5 SDK 使用的 UTF-16；配置校验失败不回显原值。
	std::wstring Utf8ToWide(const std::string& p_refText)
	{
		if (p_refText.empty())
		{
			return std::wstring();
		}
		const int iLength = MultiByteToWideChar(CP_UTF8,
			MB_ERR_INVALID_CHARS, p_refText.c_str(), -1,
			nullptr, 0);
		if (iLength <= 1)
		{
			return std::wstring();
		}
		std::vector<wchar_t> aBuffer(static_cast<std::size_t>(iLength));
		if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
			p_refText.c_str(), -1, aBuffer.data(), iLength) <= 0)
		{
			return std::wstring();
		}
		return std::wstring(aBuffer.data());
	}

	// 物理连接抽象只负责一个 SDK 会话；主备选择由逻辑源统一处理。
	class IMtQuotePhysicalConnection
	{
	public:
		virtual ~IMtQuotePhysicalConnection() = default;
		virtual bool Start(const ST_MT_QUOTE_SOURCE_CONFIG& p_refSource,
			const ST_MT_QUOTE_CONNECTION_CONFIG& p_refConnection,
			const ST_MT_QUOTE_SERVICE_CONFIG& p_refService,
			IMtQuoteConnectionSink* p_pSink,
			std::string& p_refError) = 0;
		virtual void Activate() = 0;
		virtual void Deactivate() = 0;
		virtual void Stop() = 0;
		virtual void GetStatus(ST_MT_QUOTE_CONNECTION_STATUS& p_refStatus) const = 0;
	};

	// MT4 单连接：独占 Factory/API、回调计数和重连线程，禁止承担主备选择或业务计算。
	class CMt4QuoteConnection final : public IMtQuotePhysicalConnection
	{
	public:
		CMt4QuoteConnection();
		virtual ~CMt4QuoteConnection();
		virtual bool Start(const ST_MT_QUOTE_SOURCE_CONFIG& p_refSource,
			const ST_MT_QUOTE_CONNECTION_CONFIG& p_refConnection,
			const ST_MT_QUOTE_SERVICE_CONFIG& p_refService,
			IMtQuoteConnectionSink* p_pSink,
			std::string& p_refError) override;
		virtual void Activate() override;
		virtual void Deactivate() override;
		virtual void Stop() override;
		virtual void GetStatus(ST_MT_QUOTE_CONNECTION_STATUS& p_refStatus) const override;

	private:
		static void __stdcall PumpingCallback(int p_iCode, int p_iType,
			void* p_pData, void* p_pParam);
		void HandlePumpingEvent(int p_iCode);
		void Worker();
		bool ConnectAndRegister();
		bool SubscribeAllSymbols();
		void DrainTicks();
		bool BeginCallback();
		void EndCallback();
		void SetState(EN_MT_QUOTE_CONNECTION_STATE p_enState);

	private:
		ST_MT_QUOTE_SOURCE_CONFIG m_stSource;       // 逻辑来源身份副本，不含业务状态。
		ST_MT_QUOTE_CONNECTION_CONFIG m_stConnection; // 当前物理连接凭据副本，仅传给 SDK。
		IMtQuoteConnectionSink* m_pSink;             // 非拥有回调目标，Stop 前保持有效。
		std::unique_ptr<CManagerFactory> m_ptrFactory; // 当前连接独占 MT4 Factory。
		CManagerInterface* m_pApi;                   // 当前连接独占 Pumping API。
		mutable std::mutex m_clApiMutex;             // 串行化 Connect、订阅、拉取和 Disconnect。
		mutable std::mutex m_clStateMutex;           // 保护条件、连接标记和回调计数。
		std::condition_variable m_clStateCondition;  // 唤醒连接、拉取、停机和回调退出等待。
		std::thread m_clWorker;                      // 当前物理连接唯一重连线程。
		std::atomic<bool> m_bStopping;               // 停机闸门先于 SDK 释放关闭。
		std::atomic<bool> m_bActive;                 // Owner 激活后才允许连接和回调。
		bool m_bStarted;                             // Factory/API/线程是否创建完成。
		bool m_bConnected;                           // Connect/Login 是否成功。
		bool m_bPumpingActive;                       // 是否收到 START_PUMPING。
		bool m_bSubscribePending;                    // 是否等待全品种订阅。
		bool m_bTickPending;                         // 是否等待拉取 SymbolInfoUpdated。
		bool m_bEverReady;                           // 是否至少一次进入 READY，用于重连计数。
		unsigned int m_uiActiveCallbacks;            // 尚未退出的 SDK 回调数量。
		std::atomic<EN_MT_QUOTE_CONNECTION_STATE> m_enState; // 无锁诊断状态。
		std::atomic<std::uint64_t> m_ullReconnectCount; // READY 后再次 READY 的累计次数。
		std::atomic<std::uint64_t> m_ullFailureCount; // 连接、登录、订阅失败累计次数。
		std::atomic<unsigned int> m_uiConsecutiveFailures; // 主备阈值使用的连续失败次数。
		std::atomic<std::int64_t> m_llLastTickTimeMs; // 最近有效 Tick 的本机时间。
	};

	CMt4QuoteConnection::CMt4QuoteConnection()
		: m_stSource()
		, m_stConnection()
		, m_pSink(nullptr)
		, m_ptrFactory()
		, m_pApi(nullptr)
		, m_clApiMutex()
		, m_clStateMutex()
		, m_clStateCondition()
		, m_clWorker()
		, m_bStopping(false)
		, m_bActive(false)
		, m_bStarted(false)
		, m_bConnected(false)
		, m_bPumpingActive(false)
		, m_bSubscribePending(false)
		, m_bTickPending(false)
		, m_bEverReady(false)
		, m_uiActiveCallbacks(0)
		, m_enState(EN_MT_QUOTE_CONNECTION_STOPPED)
		, m_ullReconnectCount(0)
		, m_ullFailureCount(0)
		, m_uiConsecutiveFailures(0)
		, m_llLastTickTimeMs(0)
	{
	}

	CMt4QuoteConnection::~CMt4QuoteConnection()
	{
		Stop();
	}

	bool CMt4QuoteConnection::Start(
		const ST_MT_QUOTE_SOURCE_CONFIG& p_refSource,
		const ST_MT_QUOTE_CONNECTION_CONFIG& p_refConnection,
		const ST_MT_QUOTE_SERVICE_CONFIG& p_refService,
		IMtQuoteConnectionSink* p_pSink,
		std::string& p_refError)
	{
		Stop();
		p_refError.clear();
		if (p_pSink == nullptr || p_refConnection.ullLogin >
			static_cast<std::uint64_t>((std::numeric_limits<int>::max)()))
		{
			p_refError =
				"MT4_QUOTE_START_INVALID: sink is null or login exceeds MT4 range";
			return false;
		}
		m_stSource = p_refSource;
		m_stConnection = p_refConnection;
		m_pSink = p_pSink;
		m_bStopping.store(false);
		m_ptrFactory.reset(new CManagerFactory(
			p_refService.strMt4DllPath.c_str()));
		if (!m_ptrFactory || !m_ptrFactory->IsValid())
		{
			p_refError = "MT4_SDK_LOAD_FAILED: Manager API DLL is unavailable";
			Stop();
			return false;
		}
		if (m_ptrFactory->WinsockStartup() != RET_OK)
		{
			p_refError = "MT4_WINSOCK_START_FAILED: WSAStartup failed";
			Stop();
			return false;
		}
		m_pApi = m_ptrFactory->Create(ManAPIVersion);
		if (m_pApi == nullptr)
		{
			p_refError = "MT4_API_CREATE_FAILED: Manager API factory returned null";
			Stop();
			return false;
		}
		try
		{
			m_bStarted = true;
			m_clWorker = std::thread(&CMt4QuoteConnection::Worker, this);
		}
		catch (const std::exception& p_refException)
		{
			p_refError = std::string("MT4_WORKER_START_FAILED: detail=") +
				p_refException.what();
			Stop();
			return false;
		}
		return true;
	}

	void CMt4QuoteConnection::Activate()
	{
		if (!m_bStarted || m_bStopping.load())
		{
			return;
		}
		m_bActive.store(true);
		SetState(EN_MT_QUOTE_CONNECTION_CONNECTING);
		m_clStateCondition.notify_all();
	}

	void CMt4QuoteConnection::Deactivate()
	{
		m_bActive.store(false);
		m_clStateCondition.notify_all();
		{
			std::lock_guard<std::mutex> clApiLock(m_clApiMutex);
			if (m_pApi != nullptr)
			{
				m_pApi->Disconnect();
			}
		}
		{
			std::lock_guard<std::mutex> clStateLock(m_clStateMutex);
			m_bConnected = false;
			m_bPumpingActive = false;
			m_bSubscribePending = false;
			m_bTickPending = false;
		}
		SetState(EN_MT_QUOTE_CONNECTION_STOPPED);
	}

	void CMt4QuoteConnection::Stop()
	{
		m_bStopping.store(true);
		m_bActive.store(false);
		m_clStateCondition.notify_all();
		{
			std::lock_guard<std::mutex> clApiLock(m_clApiMutex);
			if (m_pApi != nullptr)
			{
				m_pApi->Disconnect();
			}
		}
		if (m_clWorker.joinable() &&
			m_clWorker.get_id() != std::this_thread::get_id())
		{
			m_clWorker.join();
		}
		{
			std::unique_lock<std::mutex> clStateLock(m_clStateMutex);
			m_clStateCondition.wait(clStateLock,
				[this]() { return m_uiActiveCallbacks == 0; });
			m_bConnected = false;
			m_bPumpingActive = false;
			m_bSubscribePending = false;
			m_bTickPending = false;
		}
		if (m_pApi != nullptr)
		{
			m_pApi->Release();
			m_pApi = nullptr;
		}
		if (m_ptrFactory)
		{
			m_ptrFactory->WinsockCleanup();
			m_ptrFactory.reset();
		}
		m_pSink = nullptr;
		m_bStarted = false;
		m_enState.store(EN_MT_QUOTE_CONNECTION_STOPPED);
	}

	void CMt4QuoteConnection::GetStatus(
		ST_MT_QUOTE_CONNECTION_STATUS& p_refStatus) const
	{
		p_refStatus = ST_MT_QUOTE_CONNECTION_STATUS();
		p_refStatus.strConnectionId = m_stConnection.strId;
		p_refStatus.uiPriority = m_stConnection.uiPriority;
		p_refStatus.enState = m_enState.load();
		p_refStatus.ullReconnectCount = m_ullReconnectCount.load();
		p_refStatus.ullFailureCount = m_ullFailureCount.load();
		p_refStatus.llLastTickTimeMs = m_llLastTickTimeMs.load();
	}

	void __stdcall CMt4QuoteConnection::PumpingCallback(int p_iCode,
		int p_iType, void* p_pData, void* p_pParam)
	{
		(void)p_iType;
		(void)p_pData;
		CMt4QuoteConnection* pConnection =
			static_cast<CMt4QuoteConnection*>(p_pParam);
		if (pConnection == nullptr || !pConnection->BeginCallback())
		{
			return;
		}
		try
		{
			pConnection->HandlePumpingEvent(p_iCode);
		}
		catch (...)
		{
		}
		pConnection->EndCallback();
	}

	void CMt4QuoteConnection::HandlePumpingEvent(int p_iCode)
	{
		EN_MT_QUOTE_CONNECTION_STATE enNewState = m_enState.load();
		{
			std::lock_guard<std::mutex> clStateLock(m_clStateMutex);
			if (!m_bActive.load())
			{
				return;
			}
			if (p_iCode == PUMP_START_PUMPING)
			{
				m_bPumpingActive = true;
				m_bSubscribePending = true;
			}
			else if (p_iCode == PUMP_STOP_PUMPING)
			{
				m_bConnected = false;
				m_bPumpingActive = false;
				m_bSubscribePending = false;
				m_bTickPending = false;
				enNewState = EN_MT_QUOTE_CONNECTION_RECONNECTING;
			}
			else if (p_iCode == PUMP_UPDATE_BIDASK && m_bPumpingActive)
			{
				m_bTickPending = true;
			}
		}
		if (enNewState == EN_MT_QUOTE_CONNECTION_RECONNECTING)
		{
			SetState(enNewState);
		}
		m_clStateCondition.notify_all();
	}

	void CMt4QuoteConnection::Worker()
	{
		while (!m_bStopping.load())
		{
			if (!m_bActive.load())
			{
				std::unique_lock<std::mutex> clStateLock(m_clStateMutex);
				m_clStateCondition.wait(clStateLock,
					[this]() { return m_bStopping.load() || m_bActive.load(); });
				continue;
			}
			bool bConnected = false;
			{
				std::lock_guard<std::mutex> clStateLock(m_clStateMutex);
				bConnected = m_bConnected;
			}
			if (!bConnected)
			{
				SetState(m_bEverReady ?
					EN_MT_QUOTE_CONNECTION_RECONNECTING :
					EN_MT_QUOTE_CONNECTION_CONNECTING);
				if (!ConnectAndRegister())
				{
					m_ullFailureCount.fetch_add(1);
					const unsigned int uiFailures =
						m_uiConsecutiveFailures.fetch_add(1) + 1U;
					SetState(uiFailures >=
						m_stSource.uiSwitchFailureThreshold ?
						EN_MT_QUOTE_CONNECTION_DOWN :
						EN_MT_QUOTE_CONNECTION_RECONNECTING);
					std::unique_lock<std::mutex> clStateLock(m_clStateMutex);
					m_clStateCondition.wait_for(clStateLock,
						std::chrono::milliseconds(
							m_stConnection.uiReconnectIntervalMs),
						[this]() { return m_bStopping.load() || !m_bActive.load(); });
					continue;
				}
			}

			bool bSubscribe = false;
			bool bDrain = false;
			{
				std::unique_lock<std::mutex> clStateLock(m_clStateMutex);
				m_clStateCondition.wait_for(clStateLock,
					std::chrono::milliseconds(1000),
					[this]()
					{
						return m_bStopping.load() || !m_bActive.load() ||
							!m_bConnected || m_bSubscribePending ||
							m_bTickPending;
					});
				if (m_bStopping.load() || !m_bActive.load())
				{
					continue;
				}
				bSubscribe = m_bSubscribePending && m_bPumpingActive;
				bDrain = m_bTickPending && m_bPumpingActive;
				m_bTickPending = false;
			}
			bool bSubscribed = false;
			if (bSubscribe)
			{
				try
				{
					bSubscribed = SubscribeAllSymbols();
				}
				catch (...)
				{
					// 内存分配失败不得终止原连接重连线程，下一轮继续完成订阅。
					bSubscribed = false;
				}
			}
			if (bSubscribed)
			{
				{
					std::lock_guard<std::mutex> clStateLock(m_clStateMutex);
					m_bSubscribePending = false;
					if (m_bEverReady)
					{
						m_ullReconnectCount.fetch_add(1);
					}
					m_bEverReady = true;
					m_uiConsecutiveFailures.store(0);
				}
				SetState(EN_MT_QUOTE_CONNECTION_READY);
			}
			else if (bSubscribe)
			{
				m_ullFailureCount.fetch_add(1);
			}
			if (bDrain)
			{
				try
				{
					DrainTicks();
				}
				catch (...)
				{
					// 批量深拷贝失败只影响当前批次，Worker 必须继续接收后续 MT4 Tick。
					m_ullFailureCount.fetch_add(1);
				}
			}
		}
	}

	bool CMt4QuoteConnection::ConnectAndRegister()
	{
		if (m_bStopping.load() || !m_bActive.load() || m_pApi == nullptr)
		{
			return false;
		}
		std::lock_guard<std::mutex> clApiLock(m_clApiMutex);
		m_pApi->Disconnect();
		int iRet = m_pApi->Connect(m_stConnection.strAddress.c_str());
		if (iRet == RET_OK)
		{
			iRet = m_pApi->Login(static_cast<int>(m_stConnection.ullLogin),
				m_stConnection.strPassword.c_str());
		}
		if (iRet != RET_OK)
		{
			m_pApi->Disconnect();
			return false;
		}
		const int iFlags = CLIENT_FLAGS_HIDENEWS |
			CLIENT_FLAGS_HIDEMAIL | CLIENT_FLAGS_HIDEONLINE |
			CLIENT_FLAGS_HIDEUSERS;
		{
			std::lock_guard<std::mutex> clStateLock(m_clStateMutex);
			m_bConnected = true;
			m_bPumpingActive = false;
			m_bSubscribePending = false;
			m_bTickPending = false;
		}
		iRet = m_pApi->PumpingSwitchEx(PumpingCallback, iFlags, this);
		if (iRet != RET_OK)
		{
			{
				std::lock_guard<std::mutex> clStateLock(m_clStateMutex);
				m_bConnected = false;
			}
			// PumpingSwitchEx 失败后断开可能同步回调，禁止持有状态锁调用 SDK。
			m_pApi->Disconnect();
			return false;
		}
		return true;
	}

	bool CMt4QuoteConnection::SubscribeAllSymbols()
	{
		std::lock_guard<std::mutex> clApiLock(m_clApiMutex);
		if (m_pApi == nullptr || m_bStopping.load() || !m_bActive.load())
		{
			return false;
		}
		int iTotal = 0;
		ConSymbol* pSymbols = m_pApi->SymbolsGetAll(&iTotal);
		if (pSymbols == nullptr || iTotal <= 0)
		{
			if (pSymbols != nullptr)
			{
				m_pApi->MemFree(pSymbols);
			}
			return false;
		}
		int iSubscribed = 0;
		for (int iIndex = 0; iIndex < iTotal; ++iIndex)
		{
			const std::string strSymbol = ReadFixedText(
				pSymbols[iIndex].symbol, sizeof(pSymbols[iIndex].symbol));
			if (!strSymbol.empty() &&
				m_pApi->SymbolAdd(strSymbol.c_str()) == RET_OK)
			{
				++iSubscribed;
			}
		}
		m_pApi->MemFree(pSymbols);
		return iSubscribed > 0;
	}

	void CMt4QuoteConnection::DrainTicks()
	{
		std::vector<ST_QUOTE_BINARY_TICK> aTick;
		{
			std::lock_guard<std::mutex> clApiLock(m_clApiMutex);
			if (m_pApi == nullptr || m_bStopping.load() || !m_bActive.load())
			{
				return;
			}
			SymbolInfo aInfo[512] = {};
			int iCount = 0;
			while ((iCount = m_pApi->SymbolInfoUpdated(aInfo,
				static_cast<int>(_countof(aInfo)))) > 0)
			{
				for (int iIndex = 0; iIndex < iCount; ++iIndex)
				{
					if (!IsValidBidAsk(aInfo[iIndex].bid, aInfo[iIndex].ask))
					{
						continue;
					}
					ST_QUOTE_BINARY_TICK stTick;
					stTick.usPlatformVersion = 4;
					stTick.iSourceNo = m_stSource.iNo;
					stTick.strSymbol = ReadFixedText(aInfo[iIndex].symbol,
						sizeof(aInfo[iIndex].symbol));
					stTick.dBid = aInfo[iIndex].bid;
					stTick.dAsk = aInfo[iIndex].ask;
					stTick.llServerTime = static_cast<std::int64_t>(
						aInfo[iIndex].lasttime);
					stTick.llServerTimeMsc = stTick.llServerTime * 1000LL;
					stTick.llIngressTimeMs = GetNowMs();
					if (!stTick.strSymbol.empty() && stTick.llServerTime > 0)
					{
						aTick.push_back(stTick);
					}
				}
			}
		}
		for (const ST_QUOTE_BINARY_TICK& refTick : aTick)
		{
			m_llLastTickTimeMs.store(refTick.llIngressTimeMs);
			if (!m_bStopping.load() && m_bActive.load() && m_pSink != nullptr)
			{
				m_pSink->OnConnectionTick(m_stConnection.uiPriority, refTick);
			}
		}
	}

	bool CMt4QuoteConnection::BeginCallback()
	{
		std::lock_guard<std::mutex> clStateLock(m_clStateMutex);
		if (m_bStopping.load() || !m_bActive.load())
		{
			return false;
		}
		++m_uiActiveCallbacks;
		return true;
	}

	void CMt4QuoteConnection::EndCallback()
	{
		std::lock_guard<std::mutex> clStateLock(m_clStateMutex);
		if (m_uiActiveCallbacks > 0)
		{
			--m_uiActiveCallbacks;
		}
		m_clStateCondition.notify_all();
	}

	void CMt4QuoteConnection::SetState(
		EN_MT_QUOTE_CONNECTION_STATE p_enState)
	{
		const EN_MT_QUOTE_CONNECTION_STATE enPrevious =
			m_enState.exchange(p_enState);
		if (enPrevious != p_enState && m_pSink != nullptr)
		{
			m_pSink->OnConnectionState(m_stConnection.uiPriority, p_enState);
		}
	}

	// MT5 单连接：SDK 回调只深拷贝原始 Tick；不订阅时间、不缓存 TickStat、不计算业务字段。
	class CMt5QuoteConnection final : public IMtQuotePhysicalConnection,
		public IMTManagerSink, public IMTTickSink
	{
	public:
		CMt5QuoteConnection();
		virtual ~CMt5QuoteConnection();
		virtual bool Start(const ST_MT_QUOTE_SOURCE_CONFIG& p_refSource,
			const ST_MT_QUOTE_CONNECTION_CONFIG& p_refConnection,
			const ST_MT_QUOTE_SERVICE_CONFIG& p_refService,
			IMtQuoteConnectionSink* p_pSink,
			std::string& p_refError) override;
		virtual void Activate() override;
		virtual void Deactivate() override;
		virtual void Stop() override;
		virtual void GetStatus(ST_MT_QUOTE_CONNECTION_STATUS& p_refStatus) const override;
		virtual void OnConnect(void) override;
		virtual void OnDisconnect(void) override;
		virtual void OnTick(LPCWSTR p_pSymbol,
			const MTTickShort& p_refTick) override;
		virtual void OnTickStat(const MTTickStat& p_refTickStat) override;

	private:
		void Worker();
		bool Connect();
		bool SetupQuoteStream();
		bool BeginCallback();
		void EndCallback();
		void SetState(EN_MT_QUOTE_CONNECTION_STATE p_enState);

	private:
		ST_MT_QUOTE_SOURCE_CONFIG m_stSource;       // 逻辑来源身份副本。
		ST_MT_QUOTE_CONNECTION_CONFIG m_stConnection; // 当前物理连接凭据副本。
		IMtQuoteConnectionSink* m_pSink;             // 非拥有逻辑源回调。
		CMTManagerAPIFactory m_clFactory;            // 当前连接独占 MT5 Factory。
		IMTManagerAPI* m_pApi;                       // 当前连接独占 Pumping API。
		mutable std::mutex m_clApiMutex;             // 串行化 Connect/Selected/Disconnect。
		mutable std::mutex m_clStateMutex;           // 保护回调计数和连接设置状态。
		std::condition_variable m_clStateCondition;  // 唤醒连接、重连和停机。
		std::thread m_clWorker;                      // 当前物理连接唯一工作线程。
		std::atomic<bool> m_bStopping;               // 停机闸门。
		std::atomic<bool> m_bActive;                 // Owner 活动闸门。
		bool m_bStarted;                             // SDK 对象和线程是否已启动。
		bool m_bConnected;                           // 是否收到 OnConnect。
		bool m_bSetupPending;                        // 是否等待 SelectedAddAll。
		bool m_bEverReady;                           // 是否至少一次 READY。
		unsigned int m_uiActiveCallbacks;            // 尚未退出的 SDK 回调数量。
		std::atomic<EN_MT_QUOTE_CONNECTION_STATE> m_enState; // 当前 SDK 状态。
		std::atomic<std::uint64_t> m_ullReconnectCount; // 重连成功累计数。
		std::atomic<std::uint64_t> m_ullFailureCount; // 连接/订阅失败累计数。
		std::atomic<unsigned int> m_uiConsecutiveFailures; // 连续失败次数。
		std::atomic<std::int64_t> m_llLastTickTimeMs; // 最近有效 Tick 本机时间。
	};

	CMt5QuoteConnection::CMt5QuoteConnection()
		: m_stSource()
		, m_stConnection()
		, m_pSink(nullptr)
		, m_clFactory()
		, m_pApi(nullptr)
		, m_clApiMutex()
		, m_clStateMutex()
		, m_clStateCondition()
		, m_clWorker()
		, m_bStopping(false)
		, m_bActive(false)
		, m_bStarted(false)
		, m_bConnected(false)
		, m_bSetupPending(false)
		, m_bEverReady(false)
		, m_uiActiveCallbacks(0)
		, m_enState(EN_MT_QUOTE_CONNECTION_STOPPED)
		, m_ullReconnectCount(0)
		, m_ullFailureCount(0)
		, m_uiConsecutiveFailures(0)
		, m_llLastTickTimeMs(0)
	{
	}

	CMt5QuoteConnection::~CMt5QuoteConnection()
	{
		Stop();
	}

	bool CMt5QuoteConnection::Start(
		const ST_MT_QUOTE_SOURCE_CONFIG& p_refSource,
		const ST_MT_QUOTE_CONNECTION_CONFIG& p_refConnection,
		const ST_MT_QUOTE_SERVICE_CONFIG& p_refService,
		IMtQuoteConnectionSink* p_pSink,
		std::string& p_refError)
	{
		Stop();
		p_refError.clear();
		if (p_pSink == nullptr)
		{
			p_refError = "MT5_QUOTE_START_INVALID: sink is null";
			return false;
		}
		m_stSource = p_refSource;
		m_stConnection = p_refConnection;
		m_pSink = p_pSink;
		m_bStopping.store(false);
		const std::wstring wstrDllDirectory =
			Utf8ToWide(p_refService.strMt5DllDirectory);
		if (wstrDllDirectory.empty())
		{
			p_refError = "MT5_SDK_PATH_INVALID: DLL directory is not valid UTF-8";
			Stop();
			return false;
		}
		MTAPIRES iRet = m_clFactory.Initialize(wstrDllDirectory.c_str());
		if (iRet != MT_RET_OK)
		{
			p_refError = "MT5_SDK_LOAD_FAILED: Manager API initialization failed";
			Stop();
			return false;
		}
		std::uint32_t uiVersion = 0;
		iRet = m_clFactory.Version(uiVersion);
		if (iRet != MT_RET_OK || uiVersion < MTManagerAPIVersion)
		{
			p_refError = "MT5_SDK_VERSION_INVALID: Manager API is too old";
			Stop();
			return false;
		}
		iRet = m_clFactory.CreateManager(MTManagerAPIVersion, &m_pApi);
		if (iRet != MT_RET_OK || m_pApi == nullptr)
		{
			p_refError = "MT5_API_CREATE_FAILED: Manager API creation failed";
			Stop();
			return false;
		}
		if (m_pApi->Subscribe(this) != MT_RET_OK ||
			m_pApi->TickSubscribe(this) != MT_RET_OK)
		{
			p_refError = "MT5_SUBSCRIBE_FAILED: manager or tick subscription failed";
			Stop();
			return false;
		}
		try
		{
			m_bStarted = true;
			m_clWorker = std::thread(&CMt5QuoteConnection::Worker, this);
		}
		catch (const std::exception& p_refException)
		{
			p_refError = std::string("MT5_WORKER_START_FAILED: detail=") +
				p_refException.what();
			Stop();
			return false;
		}
		return true;
	}

	void CMt5QuoteConnection::Activate()
	{
		if (!m_bStarted || m_bStopping.load())
		{
			return;
		}
		m_bActive.store(true);
		SetState(EN_MT_QUOTE_CONNECTION_CONNECTING);
		m_clStateCondition.notify_all();
	}

	void CMt5QuoteConnection::Deactivate()
	{
		m_bActive.store(false);
		m_clStateCondition.notify_all();
		{
			std::lock_guard<std::mutex> clApiLock(m_clApiMutex);
			if (m_pApi != nullptr)
			{
				m_pApi->Disconnect();
			}
		}
		{
			std::lock_guard<std::mutex> clStateLock(m_clStateMutex);
			m_bConnected = false;
			m_bSetupPending = false;
		}
		SetState(EN_MT_QUOTE_CONNECTION_STOPPED);
	}

	void CMt5QuoteConnection::Stop()
	{
		m_bStopping.store(true);
		m_bActive.store(false);
		m_clStateCondition.notify_all();
		{
			std::lock_guard<std::mutex> clApiLock(m_clApiMutex);
			if (m_pApi != nullptr)
			{
				m_pApi->TickUnsubscribe(this);
				m_pApi->Unsubscribe(this);
				m_pApi->Disconnect();
			}
		}
		if (m_clWorker.joinable() &&
			m_clWorker.get_id() != std::this_thread::get_id())
		{
			m_clWorker.join();
		}
		{
			std::unique_lock<std::mutex> clStateLock(m_clStateMutex);
			m_clStateCondition.wait(clStateLock,
				[this]() { return m_uiActiveCallbacks == 0; });
			m_bConnected = false;
			m_bSetupPending = false;
		}
		if (m_pApi != nullptr)
		{
			m_pApi->Release();
			m_pApi = nullptr;
		}
		m_clFactory.Shutdown();
		m_pSink = nullptr;
		m_bStarted = false;
		m_enState.store(EN_MT_QUOTE_CONNECTION_STOPPED);
	}

	void CMt5QuoteConnection::GetStatus(
		ST_MT_QUOTE_CONNECTION_STATUS& p_refStatus) const
	{
		p_refStatus = ST_MT_QUOTE_CONNECTION_STATUS();
		p_refStatus.strConnectionId = m_stConnection.strId;
		p_refStatus.uiPriority = m_stConnection.uiPriority;
		p_refStatus.enState = m_enState.load();
		p_refStatus.ullReconnectCount = m_ullReconnectCount.load();
		p_refStatus.ullFailureCount = m_ullFailureCount.load();
		p_refStatus.llLastTickTimeMs = m_llLastTickTimeMs.load();
	}

	void CMt5QuoteConnection::OnConnect(void)
	{
		if (!BeginCallback())
		{
			return;
		}
		try
		{
			{
				std::lock_guard<std::mutex> clStateLock(m_clStateMutex);
				m_bConnected = true;
				m_bSetupPending = true;
			}
			m_clStateCondition.notify_all();
		}
		catch (...)
		{
			// SDK 回调边界不得传播异常；连接 Worker 会继续按状态对账。
		}
		EndCallback();
	}

	void CMt5QuoteConnection::OnDisconnect(void)
	{
		if (!BeginCallback())
		{
			return;
		}
		try
		{
			{
				std::lock_guard<std::mutex> clStateLock(m_clStateMutex);
				m_bConnected = false;
				m_bSetupPending = false;
			}
			SetState(EN_MT_QUOTE_CONNECTION_RECONNECTING);
			m_clStateCondition.notify_all();
		}
		catch (...)
		{
			// SDK 回调边界不得传播异常；连接 Worker 会继续执行原源重连。
		}
		EndCallback();
	}

	void CMt5QuoteConnection::OnTick(LPCWSTR p_pSymbol,
		const MTTickShort& p_refTick)
	{
		if (!BeginCallback())
		{
			return;
		}
		try
		{
			const std::string strSymbol = WideToUtf8(p_pSymbol);
			if (!strSymbol.empty() &&
				IsValidBidAsk(p_refTick.bid, p_refTick.ask) &&
				p_refTick.datetime > 0)
			{
				ST_QUOTE_BINARY_TICK stTick;
				stTick.usPlatformVersion = 5;
				stTick.iSourceNo = m_stSource.iNo;
				stTick.strSymbol = strSymbol;
				stTick.dBid = p_refTick.bid;
				stTick.dAsk = p_refTick.ask;
				stTick.dLast = std::isfinite(p_refTick.last) &&
					p_refTick.last > 0.0 ? p_refTick.last : 0.0;
				stTick.ullVolume = p_refTick.volume;
				stTick.ullVolumeExt = p_refTick.volume_ext;
				stTick.ullFlags = p_refTick.flags;
				stTick.llServerTime = p_refTick.datetime;
				stTick.llServerTimeMsc = p_refTick.datetime_msc > 0 ?
					static_cast<std::int64_t>(p_refTick.datetime_msc) :
					static_cast<std::int64_t>(p_refTick.datetime) * 1000LL;
				stTick.llIngressTimeMs = GetNowMs();
				m_llLastTickTimeMs.store(stTick.llIngressTimeMs);
				if (m_pSink != nullptr && m_bActive.load())
				{
					m_pSink->OnConnectionTick(
						m_stConnection.uiPriority, stTick);
				}
			}
		}
		catch (...)
		{
			// 内存分配或下游入队异常只丢弃当前 Tick，不得越过 SDK 回调边界。
		}
		EndCallback();
	}

	void CMt5QuoteConnection::OnTickStat(const MTTickStat& p_refTickStat)
	{
		// TickStat 是扩展业务行情，最终由 Derive/Query 负责；Quote 明确忽略且不记录逐 Tick 日志。
		(void)p_refTickStat;
	}

	void CMt5QuoteConnection::Worker()
	{
		while (!m_bStopping.load())
		{
			if (!m_bActive.load())
			{
				std::unique_lock<std::mutex> clStateLock(m_clStateMutex);
				m_clStateCondition.wait(clStateLock,
					[this]() { return m_bStopping.load() || m_bActive.load(); });
				continue;
			}
			bool bConnected = false;
			bool bSetupPending = false;
			{
				std::lock_guard<std::mutex> clStateLock(m_clStateMutex);
				bConnected = m_bConnected;
				bSetupPending = m_bSetupPending;
			}
			if (!bConnected)
			{
				SetState(m_bEverReady ?
					EN_MT_QUOTE_CONNECTION_RECONNECTING :
					EN_MT_QUOTE_CONNECTION_CONNECTING);
				bool bConnectSucceeded = false;
				try
				{
					bConnectSucceeded = Connect();
				}
				catch (...)
				{
					// 地址转换或 SDK 包装异常按连接失败处理，禁止终止重连线程。
					bConnectSucceeded = false;
				}
				if (!bConnectSucceeded)
				{
					m_ullFailureCount.fetch_add(1);
					const unsigned int uiFailures =
						m_uiConsecutiveFailures.fetch_add(1) + 1U;
					SetState(uiFailures >=
						m_stSource.uiSwitchFailureThreshold ?
						EN_MT_QUOTE_CONNECTION_DOWN :
						EN_MT_QUOTE_CONNECTION_RECONNECTING);
				}
			}
			else if (bSetupPending)
			{
				bool bSetupSucceeded = false;
				try
				{
					bSetupSucceeded = SetupQuoteStream();
				}
				catch (...)
				{
					// SelectedAddAll 异常按订阅失败处理，原连接仍由本线程重试。
					bSetupSucceeded = false;
				}
				if (bSetupSucceeded)
				{
					if (m_bEverReady)
					{
						m_ullReconnectCount.fetch_add(1);
					}
					m_bEverReady = true;
					m_uiConsecutiveFailures.store(0);
					SetState(EN_MT_QUOTE_CONNECTION_READY);
				}
				else
				{
					m_ullFailureCount.fetch_add(1);
				}
			}
			std::unique_lock<std::mutex> clStateLock(m_clStateMutex);
			m_clStateCondition.wait_for(clStateLock,
				std::chrono::milliseconds(
					m_stConnection.uiReconnectIntervalMs),
				[this]()
				{
					return m_bStopping.load() || !m_bActive.load() ||
						m_bSetupPending;
				});
		}
	}

	bool CMt5QuoteConnection::Connect()
	{
		if (m_bStopping.load() || !m_bActive.load() || m_pApi == nullptr)
		{
			return false;
		}
		const std::wstring wstrAddress = Utf8ToWide(m_stConnection.strAddress);
		const std::wstring wstrPassword = Utf8ToWide(m_stConnection.strPassword);
		if (wstrAddress.empty() || wstrPassword.empty())
		{
			return false;
		}
		std::lock_guard<std::mutex> clApiLock(m_clApiMutex);
		m_pApi->Disconnect();
		const MTAPIRES iRet = m_pApi->Connect(wstrAddress.c_str(),
			m_stConnection.ullLogin, wstrPassword.c_str(), nullptr,
			IMTManagerAPI::PUMP_MODE_SYMBOLS,
			m_stConnection.uiConnectTimeoutMs);
		return iRet == MT_RET_OK;
	}

	bool CMt5QuoteConnection::SetupQuoteStream()
	{
		{
			std::lock_guard<std::mutex> clApiLock(m_clApiMutex);
			if (m_pApi == nullptr || m_bStopping.load() || !m_bActive.load())
			{
				return false;
			}
			const MTAPIRES iRet = m_pApi->SelectedAddAll();
			if (iRet != MT_RET_OK && iRet != MT_RET_OK_NONE)
			{
				return false;
			}
		}
		std::lock_guard<std::mutex> clStateLock(m_clStateMutex);
		m_bSetupPending = false;
		return true;
	}

	bool CMt5QuoteConnection::BeginCallback()
	{
		std::lock_guard<std::mutex> clStateLock(m_clStateMutex);
		if (m_bStopping.load() || !m_bActive.load())
		{
			return false;
		}
		++m_uiActiveCallbacks;
		return true;
	}

	void CMt5QuoteConnection::EndCallback()
	{
		std::lock_guard<std::mutex> clStateLock(m_clStateMutex);
		if (m_uiActiveCallbacks > 0)
		{
			--m_uiActiveCallbacks;
		}
		m_clStateCondition.notify_all();
	}

	void CMt5QuoteConnection::SetState(
		EN_MT_QUOTE_CONNECTION_STATE p_enState)
	{
		const EN_MT_QUOTE_CONNECTION_STATE enPrevious =
			m_enState.exchange(p_enState);
		if (enPrevious != p_enState && m_pSink != nullptr)
		{
			m_pSink->OnConnectionState(m_stConnection.uiPriority, p_enState);
		}
	}

	class CMtQuoteLogicalSource;

	// 每条物理连接拥有固定 Sink，确保回调可携带自身优先级且不依赖容器地址稳定性。
	class CMtQuoteConnectionSink final : public IMtQuoteConnectionSink
	{
	public:
		CMtQuoteConnectionSink(CMtQuoteLogicalSource* p_pOwner,
			unsigned int p_uiPriority);
		virtual void OnConnectionTick(unsigned int p_uiPriority,
			const ST_QUOTE_BINARY_TICK& p_refTick) override;
		virtual void OnConnectionState(unsigned int p_uiPriority,
			EN_MT_QUOTE_CONNECTION_STATE p_enState) override;

	private:
		CMtQuoteLogicalSource* m_pOwner; // 非拥有逻辑源；物理连接先于 Owner 析构停止。
		unsigned int m_uiPriority;       // 固定连接优先级，用于拒绝错误回调身份。
	};

	// 逻辑源负责热备选择和唯一发布闸门；不修改 Tick 字段，不拥有业务缓存。
	class CMtQuoteLogicalSource final : public IMtQuoteSource
	{
	public:
		CMtQuoteLogicalSource();
		virtual ~CMtQuoteLogicalSource();
		virtual bool Start(const ST_MT_QUOTE_SOURCE_CONFIG& p_refConfig,
			const ST_MT_QUOTE_SERVICE_CONFIG& p_refServiceConfig,
			IMtQuoteSourceSink* p_pSink,
			std::string& p_refError) override;
		virtual bool Activate(std::string& p_refError) override;
		virtual void Deactivate() override;
		virtual void Stop() override;
		virtual void GetStatus(std::vector<ST_MT_QUOTE_CONNECTION_STATUS>& p_refStatus,
			std::uint64_t& p_refSwitchCount,
			unsigned int& p_refActivePriority) const override;
		void OnPhysicalTick(unsigned int p_uiPriority,
			const ST_QUOTE_BINARY_TICK& p_refTick);
		void OnPhysicalState(unsigned int p_uiPriority,
			EN_MT_QUOTE_CONNECTION_STATE p_enState);

	private:
		struct ST_CONNECTION_RUNTIME
		{
			ST_MT_QUOTE_CONNECTION_CONFIG stConfig; // 不可变连接配置副本。
			std::unique_ptr<CMtQuoteConnectionSink> ptrSink; // 固定回调桥。
			std::unique_ptr<IMtQuotePhysicalConnection> ptrConnection; // 独占 SDK 会话。
			EN_MT_QUOTE_CONNECTION_STATE enState; // 最近回调状态，只在逻辑锁下访问。
			std::int64_t llReadySinceMs;             // 连续 READY 起始时间。
			bool bEverActiveReady;                   // 当前连接作为 Active 是否曾 READY。

			ST_CONNECTION_RUNTIME()
				: stConfig(), ptrSink(), ptrConnection(),
				enState(EN_MT_QUOTE_CONNECTION_STOPPED),
				llReadySinceMs(0), bEverActiveReady(false)
			{
			}
		};

		void Monitor();
		ST_CONNECTION_RUNTIME* FindRuntimeLocked(unsigned int p_uiPriority);
		const ST_CONNECTION_RUNTIME* FindRuntimeLocked(unsigned int p_uiPriority) const;
		bool SwitchToLocked(unsigned int p_uiPriority, const char* p_szReason,
			IMtQuoteSourceSink*& p_refSink, std::string& p_refReason);

	private:
		ST_MT_QUOTE_SOURCE_CONFIG m_stConfig; // 逻辑源和主备策略副本。
		IMtQuoteSourceSink* m_pSink;           // 非拥有 Manager 回调。
		mutable std::mutex m_clMutex;          // 保护主备状态、Active 和生命周期。
		std::condition_variable m_clCondition; // 唤醒主备监控和停机。
		std::vector<std::unique_ptr<ST_CONNECTION_RUNTIME>> m_aRuntime; // 优先级排序物理连接。
		std::thread m_clMonitor;               // 唯一主备决策线程。
		bool m_bStopping;                      // 监控线程停止标记，只在逻辑锁下访问。
		bool m_bActive;                        // 当前逻辑源是否持有 Owner。
		unsigned int m_uiActivePriority;       // 唯一允许发布的优先级。
		std::int64_t m_llLastSwitchMs;         // 最近切换时刻，用于冷却。
		std::uint64_t m_ullSwitchCount;        // 主备切换累计数。
	};

	CMtQuoteConnectionSink::CMtQuoteConnectionSink(
		CMtQuoteLogicalSource* p_pOwner, unsigned int p_uiPriority)
		: m_pOwner(p_pOwner)
		, m_uiPriority(p_uiPriority)
	{
	}

	void CMtQuoteConnectionSink::OnConnectionTick(unsigned int p_uiPriority,
		const ST_QUOTE_BINARY_TICK& p_refTick)
	{
		if (m_pOwner != nullptr && p_uiPriority == m_uiPriority)
		{
			m_pOwner->OnPhysicalTick(p_uiPriority, p_refTick);
		}
	}

	void CMtQuoteConnectionSink::OnConnectionState(unsigned int p_uiPriority,
		EN_MT_QUOTE_CONNECTION_STATE p_enState)
	{
		if (m_pOwner != nullptr && p_uiPriority == m_uiPriority)
		{
			m_pOwner->OnPhysicalState(p_uiPriority, p_enState);
		}
	}

	CMtQuoteLogicalSource::CMtQuoteLogicalSource()
		: m_stConfig()
		, m_pSink(nullptr)
		, m_clMutex()
		, m_clCondition()
		, m_aRuntime()
		, m_clMonitor()
		, m_bStopping(false)
		, m_bActive(false)
		, m_uiActivePriority(0)
		, m_llLastSwitchMs(0)
		, m_ullSwitchCount(0)
	{
	}

	CMtQuoteLogicalSource::~CMtQuoteLogicalSource()
	{
		Stop();
	}

	bool CMtQuoteLogicalSource::Start(
		const ST_MT_QUOTE_SOURCE_CONFIG& p_refConfig,
		const ST_MT_QUOTE_SERVICE_CONFIG& p_refServiceConfig,
		IMtQuoteSourceSink* p_pSink, std::string& p_refError)
	{
		Stop();
		p_refError.clear();
		if (p_pSink == nullptr || p_refConfig.aConnection.empty())
		{
			p_refError = "QUOTE_LOGICAL_SOURCE_START_INVALID: sink or connection list is invalid";
			return false;
		}
		m_stConfig = p_refConfig;
		m_pSink = p_pSink;
		for (const ST_MT_QUOTE_CONNECTION_CONFIG& refConnection :
			p_refConfig.aConnection)
		{
			std::unique_ptr<ST_CONNECTION_RUNTIME> ptrRuntime(
				new ST_CONNECTION_RUNTIME());
			ptrRuntime->stConfig = refConnection;
			ptrRuntime->ptrSink.reset(new CMtQuoteConnectionSink(
				this, refConnection.uiPriority));
			if (p_refConfig.usVersion == 4)
			{
				ptrRuntime->ptrConnection.reset(new CMt4QuoteConnection());
			}
			else if (p_refConfig.usVersion == 5)
			{
				ptrRuntime->ptrConnection.reset(new CMt5QuoteConnection());
			}
			else
			{
				p_refError = "QUOTE_SOURCE_VERSION_UNSUPPORTED";
				Stop();
				return false;
			}
			if (!ptrRuntime->ptrConnection->Start(p_refConfig,
				refConnection, p_refServiceConfig,
				ptrRuntime->ptrSink.get(), p_refError))
			{
				if (refConnection.bRequired)
				{
					p_refError = "QUOTE_PHYSICAL_SOURCE_START_FAILED: id=" +
						refConnection.strId + ", detail=" + p_refError;
					Stop();
					return false;
				}
				continue;
			}
			m_aRuntime.push_back(std::move(ptrRuntime));
		}
		if (m_aRuntime.empty())
		{
			p_refError = "QUOTE_PHYSICAL_SOURCE_NONE: no connection started";
			Stop();
			return false;
		}
		try
		{
			m_bStopping = false;
			m_clMonitor = std::thread(&CMtQuoteLogicalSource::Monitor, this);
		}
		catch (const std::exception& p_refException)
		{
			p_refError = std::string("QUOTE_FAILOVER_MONITOR_START_FAILED: detail=") +
				p_refException.what();
			Stop();
			return false;
		}
		return true;
	}

	bool CMtQuoteLogicalSource::Activate(std::string& p_refError)
	{
		p_refError.clear();
		std::vector<IMtQuotePhysicalConnection*> aConnection;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			if (m_aRuntime.empty() || m_bStopping)
			{
				p_refError = "QUOTE_LOGICAL_SOURCE_ACTIVATE_INVALID: source is not started";
				return false;
			}
			m_bActive = true;
			m_uiActivePriority = m_aRuntime.front()->stConfig.uiPriority;
			m_llLastSwitchMs = 0;
			for (const std::unique_ptr<ST_CONNECTION_RUNTIME>& ptrRuntime : m_aRuntime)
			{
				ptrRuntime->bEverActiveReady = false;
				aConnection.push_back(ptrRuntime->ptrConnection.get());
			}
		}
		for (IMtQuotePhysicalConnection* pConnection : aConnection)
		{
			if (pConnection != nullptr)
			{
				pConnection->Activate();
			}
		}
		m_clCondition.notify_all();
		return true;
	}

	void CMtQuoteLogicalSource::Deactivate()
	{
		std::vector<IMtQuotePhysicalConnection*> aConnection;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			m_bActive = false;
			m_uiActivePriority = 0;
			for (const std::unique_ptr<ST_CONNECTION_RUNTIME>& ptrRuntime : m_aRuntime)
			{
				aConnection.push_back(ptrRuntime->ptrConnection.get());
			}
		}
		for (IMtQuotePhysicalConnection* pConnection : aConnection)
		{
			if (pConnection != nullptr)
			{
				pConnection->Deactivate();
			}
		}
		m_clCondition.notify_all();
	}

	void CMtQuoteLogicalSource::Stop()
	{
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			m_bStopping = true;
			m_bActive = false;
			m_uiActivePriority = 0;
		}
		m_clCondition.notify_all();
		for (const std::unique_ptr<ST_CONNECTION_RUNTIME>& ptrRuntime : m_aRuntime)
		{
			if (ptrRuntime && ptrRuntime->ptrConnection)
			{
				ptrRuntime->ptrConnection->Stop();
			}
		}
		if (m_clMonitor.joinable() &&
			m_clMonitor.get_id() != std::this_thread::get_id())
		{
			m_clMonitor.join();
		}
		m_aRuntime.clear();
		m_pSink = nullptr;
	}

	void CMtQuoteLogicalSource::GetStatus(
		std::vector<ST_MT_QUOTE_CONNECTION_STATUS>& p_refStatus,
		std::uint64_t& p_refSwitchCount,
		unsigned int& p_refActivePriority) const
	{
		p_refStatus.clear();
		std::lock_guard<std::mutex> clLock(m_clMutex);
		p_refSwitchCount = m_ullSwitchCount;
		p_refActivePriority = m_uiActivePriority;
		for (const std::unique_ptr<ST_CONNECTION_RUNTIME>& ptrRuntime : m_aRuntime)
		{
			ST_MT_QUOTE_CONNECTION_STATUS stStatus;
			ptrRuntime->ptrConnection->GetStatus(stStatus);
			stStatus.bActive = m_bActive &&
				stStatus.uiPriority == m_uiActivePriority;
			p_refStatus.push_back(stStatus);
		}
	}

	void CMtQuoteLogicalSource::OnPhysicalTick(unsigned int p_uiPriority,
		const ST_QUOTE_BINARY_TICK& p_refTick)
	{
		IMtQuoteSourceSink* pSink = nullptr;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			if (m_bActive && !m_bStopping &&
				p_uiPriority == m_uiActivePriority)
			{
				pSink = m_pSink;
			}
		}
		if (pSink != nullptr)
		{
			pSink->OnQuoteTick(p_refTick);
		}
	}

	void CMtQuoteLogicalSource::OnPhysicalState(unsigned int p_uiPriority,
		EN_MT_QUOTE_CONNECTION_STATE p_enState)
	{
		IMtQuoteSourceSink* pResetSink = nullptr;
		bool bReset = false;
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			ST_CONNECTION_RUNTIME* pRuntime = FindRuntimeLocked(p_uiPriority);
			if (pRuntime == nullptr)
			{
				return;
			}
			const EN_MT_QUOTE_CONNECTION_STATE enPrevious = pRuntime->enState;
			pRuntime->enState = p_enState;
			pRuntime->llReadySinceMs = p_enState == EN_MT_QUOTE_CONNECTION_READY ?
				GetNowMs() : 0;
			if (m_bActive && p_uiPriority == m_uiActivePriority &&
				p_enState == EN_MT_QUOTE_CONNECTION_READY)
			{
				bReset = pRuntime->bEverActiveReady &&
					enPrevious != EN_MT_QUOTE_CONNECTION_READY;
				pRuntime->bEverActiveReady = true;
				pResetSink = m_pSink;
			}
		}
		m_clCondition.notify_all();
		if (bReset && pResetSink != nullptr)
		{
			pResetSink->OnQuoteStreamChanged(m_stConfig.usVersion,
				m_stConfig.iNo, p_uiPriority, "RECONNECT");
		}
	}

	void CMtQuoteLogicalSource::Monitor()
	{
		for (;;)
		{
			IMtQuoteSourceSink* pSwitchSink = nullptr;
			std::string strReason;
			unsigned int uiNewPriority = 0;
			{
				std::unique_lock<std::mutex> clLock(m_clMutex);
				m_clCondition.wait_for(clLock, std::chrono::milliseconds(200));
				if (m_bStopping)
				{
					break;
				}
				if (!m_bActive || !m_stConfig.bSourceFailoverEnable)
				{
					continue;
				}
				const std::int64_t llNowMs = GetNowMs();
				const bool bCooldownReady = m_llLastSwitchMs == 0 ||
					llNowMs - m_llLastSwitchMs >=
						static_cast<std::int64_t>(m_stConfig.uiSwitchCooldownMs);
				ST_CONNECTION_RUNTIME* pActive =
					FindRuntimeLocked(m_uiActivePriority);
				if (bCooldownReady && pActive != nullptr &&
					pActive->enState == EN_MT_QUOTE_CONNECTION_DOWN)
				{
					for (const std::unique_ptr<ST_CONNECTION_RUNTIME>& ptrRuntime : m_aRuntime)
					{
						if (ptrRuntime->stConfig.uiPriority != m_uiActivePriority &&
							ptrRuntime->enState == EN_MT_QUOTE_CONNECTION_READY)
						{
							uiNewPriority = ptrRuntime->stConfig.uiPriority;
							SwitchToLocked(uiNewPriority, "FAILOVER",
								pSwitchSink, strReason);
							break;
						}
					}
				}
				else if (bCooldownReady && pActive != nullptr)
				{
					for (const std::unique_ptr<ST_CONNECTION_RUNTIME>& ptrRuntime : m_aRuntime)
					{
						if (ptrRuntime->stConfig.uiPriority >= m_uiActivePriority ||
							ptrRuntime->enState != EN_MT_QUOTE_CONNECTION_READY ||
							ptrRuntime->llReadySinceMs <= 0 ||
							llNowMs - ptrRuntime->llReadySinceMs <
								static_cast<std::int64_t>(m_stConfig.uiRecoveryStableMs))
						{
							continue;
						}
						uiNewPriority = ptrRuntime->stConfig.uiPriority;
						SwitchToLocked(uiNewPriority, "FAILBACK",
							pSwitchSink, strReason);
						break;
					}
				}
			}
			if (pSwitchSink != nullptr && uiNewPriority != 0)
			{
				try
				{
					pSwitchSink->OnQuoteStreamChanged(m_stConfig.usVersion,
						m_stConfig.iNo, uiNewPriority, strReason.c_str());
				}
				catch (...)
				{
					// 第二阶段切源回调异常不得结束监控线程；下一轮状态对账可继续收敛。
				}
			}
		}
	}

	CMtQuoteLogicalSource::ST_CONNECTION_RUNTIME*
		CMtQuoteLogicalSource::FindRuntimeLocked(unsigned int p_uiPriority)
	{
		for (const std::unique_ptr<ST_CONNECTION_RUNTIME>& ptrRuntime : m_aRuntime)
		{
			if (ptrRuntime->stConfig.uiPriority == p_uiPriority)
			{
				return ptrRuntime.get();
			}
		}
		return nullptr;
	}

	const CMtQuoteLogicalSource::ST_CONNECTION_RUNTIME*
		CMtQuoteLogicalSource::FindRuntimeLocked(unsigned int p_uiPriority) const
	{
		for (const std::unique_ptr<ST_CONNECTION_RUNTIME>& ptrRuntime : m_aRuntime)
		{
			if (ptrRuntime->stConfig.uiPriority == p_uiPriority)
			{
				return ptrRuntime.get();
			}
		}
		return nullptr;
	}

	bool CMtQuoteLogicalSource::SwitchToLocked(unsigned int p_uiPriority,
		const char* p_szReason, IMtQuoteSourceSink*& p_refSink,
		std::string& p_refReason)
	{
		ST_CONNECTION_RUNTIME* pRuntime = FindRuntimeLocked(p_uiPriority);
		if (!m_bActive || pRuntime == nullptr ||
			pRuntime->enState != EN_MT_QUOTE_CONNECTION_READY ||
			p_uiPriority == m_uiActivePriority)
		{
			return false;
		}
		m_uiActivePriority = p_uiPriority;
		m_llLastSwitchMs = GetNowMs();
		++m_ullSwitchCount;
		pRuntime->bEverActiveReady = true;
		p_refSink = m_pSink;
		p_refReason = p_szReason != nullptr ? p_szReason : "SWITCH";
		return true;
	}
}

ST_MT_QUOTE_CONNECTION_STATUS::ST_MT_QUOTE_CONNECTION_STATUS()
	: strConnectionId()
	, uiPriority(0)
	, enState(EN_MT_QUOTE_CONNECTION_STOPPED)
	, bActive(false)
	, ullReconnectCount(0)
	, ullFailureCount(0)
	, llLastTickTimeMs(0)
{
}

IMtQuoteConnectionSink::~IMtQuoteConnectionSink()
{
}

IMtQuoteSourceSink::~IMtQuoteSourceSink()
{
}

IMtQuoteSource::~IMtQuoteSource()
{
}

std::unique_ptr<IMtQuoteSource> CreateMtQuoteSource(
	const ST_MT_QUOTE_SOURCE_CONFIG& p_refConfig,
	std::string& p_refError)
{
	p_refError.clear();
	if (p_refConfig.usVersion != 4 && p_refConfig.usVersion != 5)
	{
		p_refError = "QUOTE_SOURCE_VERSION_UNSUPPORTED: version=" +
			std::to_string(p_refConfig.usVersion);
		return std::unique_ptr<IMtQuoteSource>();
	}
	return std::unique_ptr<IMtQuoteSource>(new CMtQuoteLogicalSource());
}
