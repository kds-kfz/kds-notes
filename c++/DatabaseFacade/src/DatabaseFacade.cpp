#ifdef _HAS_STD_BYTE
#undef _HAS_STD_BYTE
#endif
#define _HAS_STD_BYTE 1

#include "DatabaseFacade.h"

#include <pqxx/connection>
#include <pqxx/except>
#include <pqxx/nontransaction>
#include <pqxx/params>
#include <pqxx/result>
#include <pqxx/transaction>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace
{
	struct ST_DATABASE_VALUE
	{
		bool bNull; // 是否为 SQL NULL；为 true 时 strValue 为空且不得解释为空字符串。
		std::string strValue; // 非 NULL 字段的原始文本副本；由结果句柄拥有直到销毁。
	};

	class CDatabaseResult final
	{
	public:
		CDatabaseResult() : m_ullAffectedRows(0U), m_strError(), m_aColumns(), m_aRows() {}

		uint64_t m_ullAffectedRows; // INSERT/UPDATE/DELETE 影响行数，查询通常为 0。
		std::string m_strError; // SQL 或 Provider 错误；成功时为空，不包含密码。
		std::vector<std::string> m_aColumns; // 按结果列序保存的 UTF-8 列名副本。
		std::vector<std::vector<ST_DATABASE_VALUE>> m_aRows; // 不可变行列文本副本。
	};

	struct ST_POOL_CONFIG_COPY
	{
		std::string strHost; // 深拷贝主机名；池生命周期内有效。
		std::string strDatabase; // 深拷贝数据库名；池生命周期内有效。
		std::string strUser; // 深拷贝用户；池生命周期内有效。
		std::string strPassword; // 深拷贝密码；只用于建连且不写日志。
		std::string strApplicationName; // 深拷贝服务诊断名称。
		uint32_t uiPort; // PostgreSQL TCP 端口。
		uint32_t uiMinConnections; // 池保留的最小连接数。
		uint32_t uiMaxConnections; // 池允许的最大连接数。
		uint32_t uiAcquireTimeoutMs; // 默认获取连接超时，单位毫秒。
		uint32_t uiConnectTimeoutMs; // 物理建连超时，单位毫秒。
		uint32_t uiStatementTimeoutMs; // SQL 服务端超时，单位毫秒。
		uint32_t uiHealthCheckIntervalMs; // 后台补建与空闲连接健康检查周期，单位毫秒。
		uint32_t uiIdleShrinkMs; // 超过最小连接数后的空闲回收时间；0 表示禁用。
	};

	struct ST_CONNECTION_SLOT
	{
		std::unique_ptr<pqxx::connection> ptrConnection; // 独占物理连接；仅持有租约的线程可访问。
		bool bBusy; // 是否已经被 SQL 或事务租约占用；受池互斥量保护。
		std::map<std::string, bool> mapPrepared; // 本连接已注册的预编译名称集合；仅租约线程访问。
		std::chrono::steady_clock::time_point tmIdleSince; // 最近一次进入空闲状态的单调时钟时间点。

		ST_CONNECTION_SLOT() : ptrConnection(), bBusy(false), mapPrepared(),
			tmIdleSince(std::chrono::steady_clock::now()) {}
	};

	std::string ReadView(const ST_FACADE_STRING_VIEW& p_refView)
	{
		if (p_refView.pszData == nullptr || p_refView.uiLength == 0U)
		{
			return std::string();
		}
		return std::string(p_refView.pszData, p_refView.uiLength);
	}

	bool IsZeroReserved(const uint64_t* p_paullReserved)
	{
		return p_paullReserved[0] == 0U && p_paullReserved[1] == 0U &&
			p_paullReserved[2] == 0U && p_paullReserved[3] == 0U;
	}

	bool IsValidView(const ST_FACADE_STRING_VIEW& p_refView, bool p_bAllowEmpty)
	{
		return p_refView.uiStructSize >= sizeof(ST_FACADE_STRING_VIEW) &&
			p_refView.uiAbiVersion == FACADE_ABI_VERSION && p_refView.uiFlags == 0U &&
			IsZeroReserved(p_refView.aullReserved) &&
			(p_refView.pszData != nullptr || p_refView.uiLength == 0U) &&
			(p_bAllowEmpty || p_refView.uiLength != 0U);
	}

	std::string EscapeConnInfoValue(const std::string& p_refStrValue)
	{
		std::string strEscaped;
		strEscaped.reserve(p_refStrValue.size() + 8U);
		for (const char chValue : p_refStrValue)
		{
			// libpq 单引号连接串用反斜杠转义反斜杠和单引号，防止配置值改变后续键值边界。
			if (chValue == '\\' || chValue == '\'')
			{
				strEscaped.push_back('\\');
			}
			strEscaped.push_back(chValue);
		}
		return strEscaped;
	}

	void AppendConnInfo(std::ostringstream& p_refStream, const char* p_pszKey,
		const std::string& p_refStrValue)
	{
		if (p_refStream.tellp() > 0)
		{
			p_refStream << ' ';
		}
		p_refStream << p_pszKey << "='" << EscapeConnInfoValue(p_refStrValue) << "'";
	}

	std::string BuildConnInfo(const ST_POOL_CONFIG_COPY& p_refConfig)
	{
		std::ostringstream clStream;
		AppendConnInfo(clStream, "host", p_refConfig.strHost);
		AppendConnInfo(clStream, "port", std::to_string(p_refConfig.uiPort));
		AppendConnInfo(clStream, "dbname", p_refConfig.strDatabase);
		AppendConnInfo(clStream, "user", p_refConfig.strUser);
		AppendConnInfo(clStream, "password", p_refConfig.strPassword);
		// libpq connect_timeout 的单位是整秒；向上取整保证配置的毫秒上限不会被截短为 0。
		AppendConnInfo(clStream, "connect_timeout", std::to_string(
			(std::max)(1U, (p_refConfig.uiConnectTimeoutMs + 999U) / 1000U)));
		AppendConnInfo(clStream, "application_name", p_refConfig.strApplicationName);
		return clStream.str();
	}

	bool ReadParameter(const ST_DATABASE_PARAMETER& p_refParameter,
		pqxx::params& p_refParameters)
	{
		if (p_refParameter.uiStructSize < sizeof(ST_DATABASE_PARAMETER) ||
			p_refParameter.uiAbiVersion != FACADE_ABI_VERSION ||
			p_refParameter.uiFlags != 0U ||
			!IsZeroReserved(p_refParameter.aullReserved) ||
			p_refParameter.uiReservedAlignment != 0U)
		{
			return false;
		}
		if (p_refParameter.bNull != 0)
		{
			p_refParameters.append();
			return true;
		}
		if (!IsValidView(p_refParameter.stValue, true))
		{
			return false;
		}
		p_refParameters.append(ReadView(p_refParameter.stValue));
		return true;
	}

	bool BuildParameters(const ST_DATABASE_PARAMETER* p_pParameters,
		size_t p_uiParameterCount, pqxx::params& p_refParameters)
	{
		if (p_uiParameterCount > 65535U ||
			(p_pParameters == nullptr && p_uiParameterCount != 0U))
		{
			return false;
		}
		p_refParameters.reserve(p_uiParameterCount);
		for (size_t szIndex = 0; szIndex < p_uiParameterCount; ++szIndex)
		{
			if (!ReadParameter(p_pParameters[szIndex], p_refParameters))
			{
				return false;
			}
		}
		return true;
	}

	std::unique_ptr<CDatabaseResult> ConvertResult(const pqxx::result& p_refResult)
	{
		std::unique_ptr<CDatabaseResult> ptrResult(new CDatabaseResult());
		ptrResult->m_ullAffectedRows = static_cast<uint64_t>(p_refResult.affected_rows());
		const size_t szColumns = static_cast<size_t>(p_refResult.columns());
		ptrResult->m_aColumns.reserve(szColumns);
		for (size_t szColumn = 0; szColumn < szColumns; ++szColumn)
		{
			const char* pszName = p_refResult.column_name(
				static_cast<pqxx::row_size_type>(szColumn));
			ptrResult->m_aColumns.emplace_back(pszName == nullptr ? "" : pszName);
		}
		ptrResult->m_aRows.reserve(static_cast<size_t>(p_refResult.size()));
		for (const pqxx::row& refRow : p_refResult)
		{
			std::vector<ST_DATABASE_VALUE> aValues;
			aValues.reserve(szColumns);
			for (const pqxx::field& refField : refRow)
			{
				ST_DATABASE_VALUE stValue = {};
				stValue.bNull = refField.is_null();
				if (!stValue.bNull)
				{
					stValue.strValue.assign(refField.c_str(), refField.size());
				}
				aValues.push_back(std::move(stValue));
			}
			ptrResult->m_aRows.push_back(std::move(aValues));
		}
		return ptrResult;
	}

	std::unique_ptr<CDatabaseResult> MakeErrorResult(const std::string& p_refStrError)
	{
		std::unique_ptr<CDatabaseResult> ptrResult(new CDatabaseResult());
		ptrResult->m_strError = p_refStrError;
		return ptrResult;
	}

	class CPostgreSqlPool;

	class CConnectionLease final
	{
	public:
		CConnectionLease(CPostgreSqlPool* p_pPool, ST_CONNECTION_SLOT* p_pSlot);
		~CConnectionLease();
		CConnectionLease(const CConnectionLease&) = delete;
		CConnectionLease& operator=(const CConnectionLease&) = delete;
		ST_CONNECTION_SLOT* Get() const { return m_pSlot; }
		void MarkDiscard() { m_bDiscard = true; }

	private:
		CPostgreSqlPool* m_pPool; // 借用池地址；池必须晚于租约销毁。
		ST_CONNECTION_SLOT* m_pSlot; // 当前线程独占的连接槽；析构时归还。
		bool m_bDiscard; // 为 true 时归还阶段删除断开的物理连接。
	};

	class CPostgreSqlPool final
	{
	public:
		explicit CPostgreSqlPool(const ST_POOL_CONFIG_COPY& p_refConfig)
			: m_stConfig(p_refConfig), m_clMutex(), m_clCondition(), m_aSlots(),
			m_mapPreparedSql(), m_clMaintenanceThread(), m_bStopping(false),
			m_bInitialized(false), m_uiCreatingConnections(0U), m_ullAcquireCount(0U),
			m_ullWaitCount(0U), m_ullTimeoutCount(0U),
			m_ullConnectFailureCount(0U), m_ullDiscardedCount(0U),
			m_ullReconnectCount(0U), m_ullShrinkCount(0U), m_strLastError()
		{
		}

		~CPostgreSqlPool()
		{
			Stop();
		}

		bool Initialize()
		{
			{
				std::lock_guard<std::mutex> clLock(m_clMutex);
				if (m_bInitialized)
				{
					return true;
				}
				m_bInitialized = true;
				m_bStopping = false;
			}

			// 启动路径立即尝试建立最小连接；首次失败后交给后台周期重试，避免数据库
			// 不可达时按最小连接数重复等待 connect_timeout 而阻塞整个服务启动。
			for (uint32_t uiIndex = 0; uiIndex < m_stConfig.uiMinConnections; ++uiIndex)
			{
				{
					std::lock_guard<std::mutex> clLock(m_clMutex);
					++m_uiCreatingConnections;
				}
				std::unique_ptr<ST_CONNECTION_SLOT> ptrSlot = CreateSlot();
				{
					std::lock_guard<std::mutex> clLock(m_clMutex);
					--m_uiCreatingConnections;
					if (ptrSlot)
					{
						m_aSlots.push_back(std::move(ptrSlot));
					}
				}
				if (!ptrSlot && m_aSlots.empty())
				{
					break;
				}
			}
			m_clMaintenanceThread = std::thread(&CPostgreSqlPool::MaintenanceLoop, this);
			return true;
		}

		std::unique_ptr<CConnectionLease> Acquire(uint32_t p_uiTimeoutMs)
		{
			const uint32_t uiTimeout = p_uiTimeoutMs == 0U ?
				m_stConfig.uiAcquireTimeoutMs : p_uiTimeoutMs;
			std::unique_lock<std::mutex> clLock(m_clMutex);
			const auto fnFind = [this]() { return FindIdleSlotLocked(); };
			ST_CONNECTION_SLOT* pSlot = fnFind();
			if (pSlot == nullptr &&
				m_aSlots.size() + m_uiCreatingConnections < m_stConfig.uiMaxConnections)
			{
				// 在释放互斥量执行慢速建连前先占用一个创建名额，防止并发线程同时越过上限。
				++m_uiCreatingConnections;
				clLock.unlock();
				std::unique_ptr<ST_CONNECTION_SLOT> ptrNew = CreateSlot();
				clLock.lock();
				--m_uiCreatingConnections;
				if (ptrNew)
				{
					pSlot = ptrNew.get();
					m_aSlots.push_back(std::move(ptrNew));
				}
				m_clCondition.notify_all();
			}
			if (pSlot == nullptr)
			{
				++m_ullWaitCount;
				const bool bReady = m_clCondition.wait_for(clLock,
					std::chrono::milliseconds(uiTimeout), [this]()
					{
						return m_bStopping || FindIdleSlotLocked() != nullptr;
					});
				if (!bReady || m_bStopping)
				{
					++m_ullTimeoutCount;
					m_strLastError = "database connection acquire timeout";
					return std::unique_ptr<CConnectionLease>();
				}
				pSlot = FindIdleSlotLocked();
			}
			if (pSlot == nullptr)
			{
				return std::unique_ptr<CConnectionLease>();
			}
			pSlot->bBusy = true;
			++m_ullAcquireCount;
			return std::unique_ptr<CConnectionLease>(new CConnectionLease(this, pSlot));
		}

		void Release(ST_CONNECTION_SLOT* p_pSlot, bool p_bDiscard)
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			if (p_bDiscard || p_pSlot == nullptr || !p_pSlot->ptrConnection ||
				!p_pSlot->ptrConnection->is_open())
			{
				const auto iter = std::find_if(m_aSlots.begin(), m_aSlots.end(),
					[p_pSlot](const std::unique_ptr<ST_CONNECTION_SLOT>& p_refPtr)
					{
						return p_refPtr.get() == p_pSlot;
					});
				if (iter != m_aSlots.end())
				{
					m_aSlots.erase(iter);
					++m_ullDiscardedCount;
				}
			}
			else
			{
				p_pSlot->bBusy = false;
				p_pSlot->tmIdleSince = std::chrono::steady_clock::now();
			}
			m_clCondition.notify_one();
		}

		int Prepare(const std::string& p_refStrName, const std::string& p_refStrSql)
		{
			if (p_refStrName.empty() || p_refStrSql.empty())
			{
				return FACADE_INVALID_ARGUMENT;
			}
			std::lock_guard<std::mutex> clLock(m_clMutex);
			m_mapPreparedSql[p_refStrName] = p_refStrSql;
			return FACADE_SUCCESS;
		}

		bool EnsurePrepared(ST_CONNECTION_SLOT& p_refSlot,
			const std::string& p_refStrName)
		{
			if (p_refSlot.mapPrepared.find(p_refStrName) != p_refSlot.mapPrepared.end())
			{
				return true;
			}
			std::string strSql;
			{
				std::lock_guard<std::mutex> clLock(m_clMutex);
				const auto iter = m_mapPreparedSql.find(p_refStrName);
				if (iter == m_mapPreparedSql.end())
				{
					m_strLastError = "prepared statement is not registered";
					return false;
				}
				strSql = iter->second;
			}
			p_refSlot.ptrConnection->prepare(p_refStrName, strSql);
			p_refSlot.mapPrepared[p_refStrName] = true;
			return true;
		}

		void SetLastError(const std::string& p_refStrError)
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			m_strLastError = p_refStrError;
		}

		std::string GetLastError() const
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			return m_strLastError;
		}

		void GetStat(ST_DATABASE_POOL_STAT& p_refStat) const
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			p_refStat.uiTotalConnections = static_cast<uint32_t>(m_aSlots.size());
			p_refStat.uiBusyConnections = 0U;
			for (const std::unique_ptr<ST_CONNECTION_SLOT>& ptrSlot : m_aSlots)
			{
				p_refStat.uiBusyConnections += ptrSlot->bBusy ? 1U : 0U;
			}
			p_refStat.uiIdleConnections = p_refStat.uiTotalConnections -
				p_refStat.uiBusyConnections;
			p_refStat.uiCreatingConnections = m_uiCreatingConnections;
			p_refStat.ullAcquireCount = m_ullAcquireCount;
			p_refStat.ullWaitCount = m_ullWaitCount;
			p_refStat.ullTimeoutCount = m_ullTimeoutCount;
			p_refStat.ullConnectFailureCount = m_ullConnectFailureCount;
			p_refStat.ullDiscardedCount = m_ullDiscardedCount;
			p_refStat.ullReconnectCount = m_ullReconnectCount;
			p_refStat.ullShrinkCount = m_ullShrinkCount;
		}

		bool IsReady() const
		{
			std::lock_guard<std::mutex> clLock(m_clMutex);
			return !m_bStopping && !m_aSlots.empty();
		}

		void Stop()
		{
			{
				std::lock_guard<std::mutex> clLock(m_clMutex);
				if (!m_bInitialized)
				{
					return;
				}
				m_bStopping = true;
				m_clCondition.notify_all();
			}
			if (m_clMaintenanceThread.joinable())
			{
				m_clMaintenanceThread.join();
			}
			std::lock_guard<std::mutex> clLock(m_clMutex);
			m_aSlots.clear();
			m_bInitialized = false;
		}

	private:
		std::unique_ptr<ST_CONNECTION_SLOT> CreateSlot()
		{
			try
			{
				std::unique_ptr<ST_CONNECTION_SLOT> ptrSlot(new ST_CONNECTION_SLOT());
				ptrSlot->ptrConnection.reset(new pqxx::connection(BuildConnInfo(m_stConfig)));
				if (m_stConfig.uiStatementTimeoutMs != 0U)
				{
					pqxx::work clWork(*ptrSlot->ptrConnection);
					// SET 不允许返回数据行；显式检查 no_rows 保持与旧 exec0 相同的契约且避免弃用接口。
					clWork.exec("SET statement_timeout = " +
						std::to_string(m_stConfig.uiStatementTimeoutMs)).no_rows();
					clWork.commit();
				}
				return ptrSlot;
			}
			catch (const std::exception& refException)
			{
				std::lock_guard<std::mutex> clLock(m_clMutex);
				++m_ullConnectFailureCount;
				m_strLastError = refException.what();
				return std::unique_ptr<ST_CONNECTION_SLOT>();
			}
		}

		ST_CONNECTION_SLOT* FindIdleSlotLocked() const
		{
			for (const std::unique_ptr<ST_CONNECTION_SLOT>& ptrSlot : m_aSlots)
			{
				if (!ptrSlot->bBusy)
				{
					return ptrSlot.get();
				}
			}
			return nullptr;
		}

		void MaintenanceLoop()
		{
			for (;;)
			{
				std::unique_lock<std::mutex> clLock(m_clMutex);
				const bool bStopping = m_clCondition.wait_for(clLock,
					std::chrono::milliseconds(m_stConfig.uiHealthCheckIntervalMs),
					[this]() { return m_bStopping; });
				if (bStopping || m_bStopping)
				{
					return;
				}

				const std::chrono::steady_clock::time_point tmNow =
					std::chrono::steady_clock::now();
				if (m_stConfig.uiIdleShrinkMs != 0U)
				{
					for (auto iter = m_aSlots.begin(); iter != m_aSlots.end() &&
						m_aSlots.size() > m_stConfig.uiMinConnections;)
					{
						const bool bExpired = !(*iter)->bBusy &&
							tmNow - (*iter)->tmIdleSince >=
							std::chrono::milliseconds(m_stConfig.uiIdleShrinkMs);
						if (bExpired)
						{
							iter = m_aSlots.erase(iter);
							++m_ullShrinkCount;
						}
						else
						{
							++iter;
						}
					}
				}

				std::vector<ST_CONNECTION_SLOT*> aHealthSlots;
				for (const std::unique_ptr<ST_CONNECTION_SLOT>& ptrSlot : m_aSlots)
				{
					if (!ptrSlot->bBusy)
					{
						ptrSlot->bBusy = true;
						aHealthSlots.push_back(ptrSlot.get());
					}
				}
				clLock.unlock();

				for (ST_CONNECTION_SLOT* pSlot : aHealthSlots)
				{
					bool bHealthy = false;
					try
					{
						pqxx::nontransaction clCheck(*pSlot->ptrConnection);
						pqxx::params clNoParameters;
						clCheck.exec("SELECT 1", clNoParameters).one_row();
						bHealthy = pSlot->ptrConnection->is_open();
					}
					catch (const std::exception& refException)
					{
						SetLastError(refException.what());
					}

					clLock.lock();
					const auto iter = std::find_if(m_aSlots.begin(), m_aSlots.end(),
						[pSlot](const std::unique_ptr<ST_CONNECTION_SLOT>& p_refPtr)
						{
							return p_refPtr.get() == pSlot;
						});
					if (iter != m_aSlots.end())
					{
						if (bHealthy)
						{
							pSlot->bBusy = false;
							pSlot->tmIdleSince = std::chrono::steady_clock::now();
						}
						else
						{
							m_aSlots.erase(iter);
							++m_ullDiscardedCount;
						}
					}
					m_clCondition.notify_all();
					clLock.unlock();
				}

				// 故障连接清理后补足最小连接。创建过程不持有池锁，避免阻塞业务归还连接。
				for (;;)
				{
					clLock.lock();
					if (m_bStopping ||
						m_aSlots.size() + m_uiCreatingConnections >= m_stConfig.uiMinConnections)
					{
						clLock.unlock();
						break;
					}
					++m_uiCreatingConnections;
					clLock.unlock();
					std::unique_ptr<ST_CONNECTION_SLOT> ptrSlot = CreateSlot();
					clLock.lock();
					--m_uiCreatingConnections;
					const bool bCreateSucceeded = ptrSlot != nullptr;
					if (bCreateSucceeded && !m_bStopping)
					{
						m_aSlots.push_back(std::move(ptrSlot));
						++m_ullReconnectCount;
					}
					m_clCondition.notify_all();
					clLock.unlock();
					if (!bCreateSucceeded)
					{
						break;
					}
				}
			}
		}

		ST_POOL_CONFIG_COPY m_stConfig; // 创建时深拷贝的不可变连接配置。
		mutable std::mutex m_clMutex; // 保护连接槽、预编译定义、统计和最后错误。
		std::condition_variable m_clCondition; // 连接归还、停机时唤醒等待线程。
		std::vector<std::unique_ptr<ST_CONNECTION_SLOT>> m_aSlots; // 全部物理连接槽所有权。
		std::map<std::string, std::string> m_mapPreparedSql; // 名称到 SQL 的池级不可变定义。
		std::thread m_clMaintenanceThread; // 周期健康检查、故障补建和空闲收缩线程。
		bool m_bStopping; // 停机后禁止新租约；受 m_clMutex 保护。
		bool m_bInitialized; // 维护线程已经建立且 Stop 尚未完成；受 m_clMutex 保护。
		uint32_t m_uiCreatingConnections; // 当前释放池锁执行建连的线程数；受 m_clMutex 保护。
		uint64_t m_ullAcquireCount; // 累计成功租约数；受 m_clMutex 保护。
		uint64_t m_ullWaitCount; // 累计发生等待的获取次数；受 m_clMutex 保护。
		uint64_t m_ullTimeoutCount; // 累计等待超时次数；受 m_clMutex 保护。
		uint64_t m_ullConnectFailureCount; // 累计物理建连失败次数；受 m_clMutex 保护。
		uint64_t m_ullDiscardedCount; // 累计丢弃断开连接数；受 m_clMutex 保护。
		uint64_t m_ullReconnectCount; // 维护线程累计补建成功数；受 m_clMutex 保护。
		uint64_t m_ullShrinkCount; // 维护线程累计空闲收缩数；受 m_clMutex 保护。
		std::string m_strLastError; // 最近一次池级错误，不包含密码；受 m_clMutex 保护。
	};

	CConnectionLease::CConnectionLease(CPostgreSqlPool* p_pPool,
		ST_CONNECTION_SLOT* p_pSlot)
		: m_pPool(p_pPool), m_pSlot(p_pSlot), m_bDiscard(false)
	{
	}

	CConnectionLease::~CConnectionLease()
	{
		if (m_pPool != nullptr && m_pSlot != nullptr)
		{
			m_pPool->Release(m_pSlot, m_bDiscard);
		}
	}

	class CDatabaseTransaction final
	{
	public:
		CDatabaseTransaction(CPostgreSqlPool* p_pPool,
			std::unique_ptr<CConnectionLease> p_ptrLease)
			: m_pPool(p_pPool), m_ptrLease(std::move(p_ptrLease)),
			m_ptrWork(new pqxx::work(*m_ptrLease->Get()->ptrConnection)), m_bFinished(false)
		{
		}

		CPostgreSqlPool* m_pPool; // 借用池地址，用于错误记录；池必须晚于事务销毁。
		std::unique_ptr<CConnectionLease> m_ptrLease; // 独占物理连接直到提交、回滚或销毁。
		std::unique_ptr<pqxx::work> m_ptrWork; // pqxx 事务对象，仅允许创建线程串行使用。
		bool m_bFinished; // 是否已经提交或回滚；防止重复终结事务。
	};

	class IDatabaseProviderFactory
	{
	public:
		virtual ~IDatabaseProviderFactory() = default;
		virtual std::unique_ptr<CPostgreSqlPool> CreatePostgreSql(
			const ST_POOL_CONFIG_COPY& p_refConfig) const = 0;
	};

	class CDatabaseProviderFactory final : public IDatabaseProviderFactory
	{
	public:
		std::unique_ptr<CPostgreSqlPool> CreatePostgreSql(
			const ST_POOL_CONFIG_COPY& p_refConfig) const override
		{
			return std::unique_ptr<CPostgreSqlPool>(new CPostgreSqlPool(p_refConfig));
		}
	};

	CPostgreSqlPool* GetPool(DATABASE_POOL_HANDLE p_pPool)
	{
		return static_cast<CPostgreSqlPool*>(p_pPool);
	}

	CDatabaseResult* GetResult(DATABASE_RESULT_HANDLE p_pResult)
	{
		return static_cast<CDatabaseResult*>(p_pResult);
	}

	CDatabaseTransaction* GetTransaction(DATABASE_TRANSACTION_HANDLE p_pTransaction)
	{
		return static_cast<CDatabaseTransaction*>(p_pTransaction);
	}

	int CopyOutput(const std::string& p_refStrValue, ST_FACADE_MUTABLE_BUFFER* p_pOutput)
	{
		if (p_pOutput == nullptr ||
			p_pOutput->uiStructSize < sizeof(ST_FACADE_MUTABLE_BUFFER) ||
			p_pOutput->uiAbiVersion != FACADE_ABI_VERSION ||
			p_pOutput->uiFlags != 0U || !IsZeroReserved(p_pOutput->aullReserved) ||
			(p_pOutput->pData == nullptr && p_pOutput->uiCapacity != 0U))
		{
			return FACADE_INVALID_ARGUMENT;
		}
		p_pOutput->uiLength = p_refStrValue.size();
		if (p_pOutput->uiCapacity < p_refStrValue.size())
		{
			return FACADE_BUFFER_TOO_SMALL;
		}
		if (!p_refStrValue.empty())
		{
			std::memcpy(p_pOutput->pData, p_refStrValue.data(), p_refStrValue.size());
		}
		return FACADE_SUCCESS;
	}

	int ExecuteWork(pqxx::transaction_base& p_refWork,
		const std::string& p_refStrSql, const ST_DATABASE_PARAMETER* p_pParameters,
		size_t p_uiParameterCount, DATABASE_RESULT_HANDLE* p_pResult)
	{
		pqxx::params clParameters;
		if (p_pResult == nullptr || p_refStrSql.empty() ||
			!BuildParameters(p_pParameters, p_uiParameterCount, clParameters))
		{
			return FACADE_INVALID_ARGUMENT;
		}
		*p_pResult = nullptr;
		// 不在公共助手中吞掉 pqxx 异常；拥有连接租约的外层必须区分断链并淘汰物理连接。
		std::unique_ptr<CDatabaseResult> ptrResult = ConvertResult(
			p_refWork.exec(p_refStrSql, clParameters));
		*p_pResult = ptrResult.release();
		return FACADE_SUCCESS;
	}

	void CopyText(char* p_pszDestination, size_t p_uiCapacity, const char* p_pszSource)
	{
		const size_t szLength = (std::min)(std::strlen(p_pszSource), p_uiCapacity - 1U);
		std::memcpy(p_pszDestination, p_pszSource, szLength);
		p_pszDestination[szLength] = '\0';
	}
}

int DatabaseFacade_GetAbiInfo(ST_FACADE_ABI_INFO* p_pInfo)
{
	if (p_pInfo == nullptr || p_pInfo->uiStructSize < sizeof(ST_FACADE_ABI_INFO) ||
		p_pInfo->uiAbiVersion != FACADE_ABI_VERSION)
	{
		return FACADE_ABI_MISMATCH;
	}
	std::memset(p_pInfo, 0, sizeof(*p_pInfo));
	p_pInfo->uiStructSize = sizeof(*p_pInfo);
	p_pInfo->uiAbiVersion = FACADE_ABI_VERSION;
	CopyText(p_pInfo->szFacadeName, sizeof(p_pInfo->szFacadeName), "DatabaseFacade");
	CopyText(p_pInfo->szImplementationVersion,
		sizeof(p_pInfo->szImplementationVersion), "1.0.1");
	CopyText(p_pInfo->szDependencyVersion,
		sizeof(p_pInfo->szDependencyVersion), "pqxx-7.10.7;libpq-18.4;openssl-3.5.6-dynamic");
	CopyText(p_pInfo->szBuildId, sizeof(p_pInfo->szBuildId), __DATE__ " " __TIME__);
	return FACADE_SUCCESS;
}

int DatabaseFacade_CreatePool(const ST_DATABASE_POOL_CONFIG* p_pConfig,
	DATABASE_POOL_HANDLE* p_pPool)
{
	if (p_pPool == nullptr)
	{
		return FACADE_INVALID_ARGUMENT;
	}
	*p_pPool = nullptr;
	if (p_pConfig == nullptr || p_pConfig->uiStructSize < sizeof(*p_pConfig) ||
		p_pConfig->uiAbiVersion != FACADE_ABI_VERSION || p_pConfig->uiFlags != 0U ||
		!IsZeroReserved(p_pConfig->aullReserved) ||
		p_pConfig->enProvider != DATABASE_PROVIDER_POSTGRESQL ||
		p_pConfig->uiPort == 0U || p_pConfig->uiPort > 65535U ||
		!IsValidView(p_pConfig->stHost, false) ||
		!IsValidView(p_pConfig->stDatabase, false) ||
		!IsValidView(p_pConfig->stUser, false) ||
		!IsValidView(p_pConfig->stPassword, true) ||
		!IsValidView(p_pConfig->stApplicationName, false) ||
		p_pConfig->uiMinConnections == 0U ||
		p_pConfig->uiMinConnections > p_pConfig->uiMaxConnections ||
		p_pConfig->uiMaxConnections > 1024U ||
		p_pConfig->uiAcquireTimeoutMs == 0U ||
		p_pConfig->uiAcquireTimeoutMs > 600000U ||
		p_pConfig->uiConnectTimeoutMs < 100U ||
		p_pConfig->uiConnectTimeoutMs > 600000U ||
		p_pConfig->uiStatementTimeoutMs > 3600000U ||
		p_pConfig->uiHealthCheckIntervalMs < 1000U ||
		p_pConfig->uiHealthCheckIntervalMs > 3600000U ||
		p_pConfig->uiIdleShrinkMs > 86400000U)
	{
		return p_pConfig != nullptr && p_pConfig->enProvider != DATABASE_PROVIDER_POSTGRESQL ?
			FACADE_NOT_SUPPORTED : FACADE_INVALID_ARGUMENT;
	}
	ST_POOL_CONFIG_COPY stConfig = {};
	stConfig.strHost = ReadView(p_pConfig->stHost);
	stConfig.strDatabase = ReadView(p_pConfig->stDatabase);
	stConfig.strUser = ReadView(p_pConfig->stUser);
	stConfig.strPassword = ReadView(p_pConfig->stPassword);
	stConfig.strApplicationName = ReadView(p_pConfig->stApplicationName);
	stConfig.uiPort = p_pConfig->uiPort;
	stConfig.uiMinConnections = p_pConfig->uiMinConnections;
	stConfig.uiMaxConnections = p_pConfig->uiMaxConnections;
	stConfig.uiAcquireTimeoutMs = p_pConfig->uiAcquireTimeoutMs;
	stConfig.uiConnectTimeoutMs = p_pConfig->uiConnectTimeoutMs;
	stConfig.uiStatementTimeoutMs = p_pConfig->uiStatementTimeoutMs;
	stConfig.uiHealthCheckIntervalMs = p_pConfig->uiHealthCheckIntervalMs;
	stConfig.uiIdleShrinkMs = p_pConfig->uiIdleShrinkMs;
	CDatabaseProviderFactory clFactory;
	std::unique_ptr<CPostgreSqlPool> ptrPool = clFactory.CreatePostgreSql(stConfig);
	if (!ptrPool->Initialize())
	{
		return FACADE_IO_ERROR;
	}
	*p_pPool = ptrPool.release();
	return FACADE_SUCCESS;
}

void DatabaseFacade_DestroyPool(DATABASE_POOL_HANDLE p_pPool)
{
	CPostgreSqlPool* pPool = GetPool(p_pPool);
	if (pPool != nullptr)
	{
		pPool->Stop();
		delete pPool;
	}
}

int DatabaseFacade_IsPoolReady(DATABASE_POOL_HANDLE p_pPool, int32_t* p_piReady)
{
	if (GetPool(p_pPool) == nullptr || p_piReady == nullptr)
	{
		return FACADE_INVALID_ARGUMENT;
	}
	*p_piReady = GetPool(p_pPool)->IsReady() ? 1 : 0;
	return FACADE_SUCCESS;
}

int DatabaseFacade_GetPoolStat(DATABASE_POOL_HANDLE p_pPool,
	ST_DATABASE_POOL_STAT* p_pStat)
{
	if (GetPool(p_pPool) == nullptr || p_pStat == nullptr ||
		p_pStat->uiStructSize < sizeof(*p_pStat) ||
		p_pStat->uiAbiVersion != FACADE_ABI_VERSION)
	{
		return FACADE_ABI_MISMATCH;
	}
	const uint32_t uiStructSize = p_pStat->uiStructSize;
	std::memset(p_pStat, 0, sizeof(*p_pStat));
	p_pStat->uiStructSize = uiStructSize;
	p_pStat->uiAbiVersion = FACADE_ABI_VERSION;
	GetPool(p_pPool)->GetStat(*p_pStat);
	return FACADE_SUCCESS;
}

int DatabaseFacade_GetLastError(DATABASE_POOL_HANDLE p_pPool,
	ST_FACADE_MUTABLE_BUFFER* p_pError)
{
	return GetPool(p_pPool) == nullptr ? FACADE_INVALID_ARGUMENT :
		CopyOutput(GetPool(p_pPool)->GetLastError(), p_pError);
}

int DatabaseFacade_Execute(DATABASE_POOL_HANDLE p_pPool,
	const ST_FACADE_STRING_VIEW* p_pSql,
	const ST_DATABASE_PARAMETER* p_pParameters, size_t p_uiParameterCount,
	uint32_t p_uiAcquireTimeoutMs, DATABASE_RESULT_HANDLE* p_pResult)
{
	CPostgreSqlPool* pPool = GetPool(p_pPool);
	if (pPool == nullptr || p_pSql == nullptr || !IsValidView(*p_pSql, false) ||
		p_pResult == nullptr)
	{
		return FACADE_INVALID_ARGUMENT;
	}
	*p_pResult = nullptr;
	std::unique_ptr<CConnectionLease> ptrLease = pPool->Acquire(p_uiAcquireTimeoutMs);
	if (!ptrLease)
	{
		return FACADE_TIMEOUT;
	}
	try
	{
		pqxx::work clWork(*ptrLease->Get()->ptrConnection);
		const int iResult = ExecuteWork(clWork, ReadView(*p_pSql), p_pParameters,
			p_uiParameterCount, p_pResult);
		if (iResult == FACADE_SUCCESS)
		{
			clWork.commit();
		}
		return iResult;
	}
	catch (const pqxx::broken_connection& refException)
	{
		ptrLease->MarkDiscard();
		pPool->SetLastError(refException.what());
		*p_pResult = MakeErrorResult(refException.what()).release();
		return FACADE_IO_ERROR;
	}
	catch (const std::exception& refException)
	{
		pPool->SetLastError(refException.what());
		*p_pResult = MakeErrorResult(refException.what()).release();
		return FACADE_IO_ERROR;
	}
}

int DatabaseFacade_ExecuteBatch(DATABASE_POOL_HANDLE p_pPool,
	const ST_DATABASE_BATCH_ITEM* p_pItems, size_t p_uiItemCount,
	uint32_t p_uiAcquireTimeoutMs, DATABASE_RESULT_HANDLE* p_pResult)
{
	if (GetPool(p_pPool) == nullptr || p_pItems == nullptr || p_uiItemCount == 0U ||
		p_uiItemCount > 65535U || p_pResult == nullptr)
	{
		return FACADE_INVALID_ARGUMENT;
	}
	// 输出句柄先归零，保证首条语句失败或调用方传入未初始化变量时，
	// 后续清理不会把随机地址当作由 DatabaseFacade 创建的结果句柄释放。
	*p_pResult = nullptr;
	DATABASE_TRANSACTION_HANDLE pTransaction = nullptr;
	int iResult = DatabaseFacade_Begin(p_pPool, p_uiAcquireTimeoutMs, &pTransaction);
	for (size_t szIndex = 0; iResult == FACADE_SUCCESS && szIndex < p_uiItemCount; ++szIndex)
	{
		const ST_DATABASE_BATCH_ITEM& refItem = p_pItems[szIndex];
		if (refItem.uiStructSize < sizeof(refItem) ||
			refItem.uiAbiVersion != FACADE_ABI_VERSION || refItem.uiFlags != 0U ||
			!IsZeroReserved(refItem.aullReserved))
		{
			iResult = FACADE_INVALID_ARGUMENT;
			break;
		}
		DatabaseFacade_DestroyResult(*p_pResult);
		*p_pResult = nullptr;
		iResult = DatabaseFacade_TransactionExecute(pTransaction, &refItem.stSql,
			refItem.pParameters, refItem.uiParameterCount, p_pResult);
	}
	if (iResult == FACADE_SUCCESS)
	{
		iResult = DatabaseFacade_Commit(pTransaction);
	}
	else
	{
		DatabaseFacade_Rollback(pTransaction);
	}
	DatabaseFacade_DestroyTransaction(pTransaction);
	return iResult;
}

int DatabaseFacade_Prepare(DATABASE_POOL_HANDLE p_pPool,
	const ST_FACADE_STRING_VIEW* p_pName, const ST_FACADE_STRING_VIEW* p_pSql)
{
	if (GetPool(p_pPool) == nullptr || p_pName == nullptr || p_pSql == nullptr ||
		!IsValidView(*p_pName, false) || !IsValidView(*p_pSql, false))
	{
		return FACADE_INVALID_ARGUMENT;
	}
	return GetPool(p_pPool)->Prepare(ReadView(*p_pName), ReadView(*p_pSql));
}

int DatabaseFacade_ExecutePrepared(DATABASE_POOL_HANDLE p_pPool,
	const ST_FACADE_STRING_VIEW* p_pName,
	const ST_DATABASE_PARAMETER* p_pParameters, size_t p_uiParameterCount,
	uint32_t p_uiAcquireTimeoutMs, DATABASE_RESULT_HANDLE* p_pResult)
{
	CPostgreSqlPool* pPool = GetPool(p_pPool);
	if (pPool == nullptr || p_pName == nullptr || !IsValidView(*p_pName, false) ||
		p_pResult == nullptr)
	{
		return FACADE_INVALID_ARGUMENT;
	}
	// 与普通执行接口保持相同输出契约：返回非成功状态时也不遗留旧句柄。
	*p_pResult = nullptr;
	pqxx::params clParameters;
	if (!BuildParameters(p_pParameters, p_uiParameterCount, clParameters))
	{
		return FACADE_INVALID_ARGUMENT;
	}
	std::unique_ptr<CConnectionLease> ptrLease = pPool->Acquire(p_uiAcquireTimeoutMs);
	if (!ptrLease)
	{
		return FACADE_TIMEOUT;
	}
	try
	{
		const std::string strName = ReadView(*p_pName);
		if (!pPool->EnsurePrepared(*ptrLease->Get(), strName))
		{
			return FACADE_INVALID_ARGUMENT;
		}
		pqxx::work clWork(*ptrLease->Get()->ptrConnection);
		std::unique_ptr<CDatabaseResult> ptrResult = ConvertResult(
			clWork.exec(pqxx::prepped{strName}, clParameters));
		clWork.commit();
		*p_pResult = ptrResult.release();
		return FACADE_SUCCESS;
	}
	catch (const pqxx::broken_connection& refException)
	{
		ptrLease->MarkDiscard();
		pPool->SetLastError(refException.what());
		*p_pResult = MakeErrorResult(refException.what()).release();
		return FACADE_IO_ERROR;
	}
	catch (const std::exception& refException)
	{
		pPool->SetLastError(refException.what());
		*p_pResult = MakeErrorResult(refException.what()).release();
		return FACADE_IO_ERROR;
	}
}

int DatabaseFacade_Begin(DATABASE_POOL_HANDLE p_pPool, uint32_t p_uiAcquireTimeoutMs,
	DATABASE_TRANSACTION_HANDLE* p_pTransaction)
{
	CPostgreSqlPool* pPool = GetPool(p_pPool);
	if (pPool == nullptr || p_pTransaction == nullptr)
	{
		return FACADE_INVALID_ARGUMENT;
	}
	*p_pTransaction = nullptr;
	std::unique_ptr<CConnectionLease> ptrLease = pPool->Acquire(p_uiAcquireTimeoutMs);
	if (!ptrLease)
	{
		return FACADE_TIMEOUT;
	}
	try
	{
		*p_pTransaction = new CDatabaseTransaction(pPool, std::move(ptrLease));
		return FACADE_SUCCESS;
	}
	catch (const std::exception& refException)
	{
		pPool->SetLastError(refException.what());
		return FACADE_IO_ERROR;
	}
}

int DatabaseFacade_TransactionExecute(DATABASE_TRANSACTION_HANDLE p_pTransaction,
	const ST_FACADE_STRING_VIEW* p_pSql,
	const ST_DATABASE_PARAMETER* p_pParameters, size_t p_uiParameterCount,
	DATABASE_RESULT_HANDLE* p_pResult)
{
	CDatabaseTransaction* pTransaction = GetTransaction(p_pTransaction);
	if (pTransaction == nullptr || pTransaction->m_bFinished || p_pSql == nullptr ||
		!IsValidView(*p_pSql, false))
	{
		return FACADE_INVALID_ARGUMENT;
	}
	try
	{
		return ExecuteWork(*pTransaction->m_ptrWork, ReadView(*p_pSql), p_pParameters,
			p_uiParameterCount, p_pResult);
	}
	catch (const pqxx::broken_connection& refException)
	{
		pTransaction->m_ptrLease->MarkDiscard();
		pTransaction->m_pPool->SetLastError(refException.what());
		*p_pResult = MakeErrorResult(refException.what()).release();
		return FACADE_IO_ERROR;
	}
	catch (const std::exception& refException)
	{
		pTransaction->m_pPool->SetLastError(refException.what());
		*p_pResult = MakeErrorResult(refException.what()).release();
		return FACADE_IO_ERROR;
	}
}

int DatabaseFacade_Commit(DATABASE_TRANSACTION_HANDLE p_pTransaction)
{
	CDatabaseTransaction* pTransaction = GetTransaction(p_pTransaction);
	if (pTransaction == nullptr || pTransaction->m_bFinished)
	{
		return FACADE_INVALID_ARGUMENT;
	}
	try
	{
		pTransaction->m_ptrWork->commit();
		pTransaction->m_bFinished = true;
		pTransaction->m_ptrWork.reset();
		pTransaction->m_ptrLease.reset();
		return FACADE_SUCCESS;
	}
	catch (const pqxx::broken_connection& refException)
	{
		pTransaction->m_ptrLease->MarkDiscard();
		pTransaction->m_pPool->SetLastError(refException.what());
		return FACADE_IO_ERROR;
	}
	catch (const std::exception& refException)
	{
		pTransaction->m_pPool->SetLastError(refException.what());
		return FACADE_IO_ERROR;
	}
}

int DatabaseFacade_Rollback(DATABASE_TRANSACTION_HANDLE p_pTransaction)
{
	CDatabaseTransaction* pTransaction = GetTransaction(p_pTransaction);
	if (pTransaction == nullptr || pTransaction->m_bFinished)
	{
		return FACADE_INVALID_ARGUMENT;
	}
	try
	{
		pTransaction->m_ptrWork->abort();
		pTransaction->m_bFinished = true;
		pTransaction->m_ptrWork.reset();
		pTransaction->m_ptrLease.reset();
		return FACADE_SUCCESS;
	}
	catch (const pqxx::broken_connection& refException)
	{
		pTransaction->m_ptrLease->MarkDiscard();
		pTransaction->m_pPool->SetLastError(refException.what());
		return FACADE_IO_ERROR;
	}
	catch (const std::exception& refException)
	{
		pTransaction->m_pPool->SetLastError(refException.what());
		return FACADE_IO_ERROR;
	}
}

void DatabaseFacade_DestroyTransaction(DATABASE_TRANSACTION_HANDLE p_pTransaction)
{
	CDatabaseTransaction* pTransaction = GetTransaction(p_pTransaction);
	if (pTransaction != nullptr && !pTransaction->m_bFinished)
	{
		DatabaseFacade_Rollback(pTransaction);
	}
	delete pTransaction;
}

void DatabaseFacade_DestroyResult(DATABASE_RESULT_HANDLE p_pResult)
{
	delete GetResult(p_pResult);
}

int DatabaseFacade_ResultGetAffectedRows(DATABASE_RESULT_HANDLE p_pResult,
	uint64_t* p_pullAffectedRows)
{
	if (GetResult(p_pResult) == nullptr || p_pullAffectedRows == nullptr)
	{
		return FACADE_INVALID_ARGUMENT;
	}
	*p_pullAffectedRows = GetResult(p_pResult)->m_ullAffectedRows;
	return FACADE_SUCCESS;
}

int DatabaseFacade_ResultGetDimensions(DATABASE_RESULT_HANDLE p_pResult,
	size_t* p_puiRows, size_t* p_puiColumns)
{
	CDatabaseResult* pResult = GetResult(p_pResult);
	if (pResult == nullptr || p_puiRows == nullptr || p_puiColumns == nullptr)
	{
		return FACADE_INVALID_ARGUMENT;
	}
	*p_puiRows = pResult->m_aRows.size();
	*p_puiColumns = pResult->m_aColumns.size();
	return FACADE_SUCCESS;
}

int DatabaseFacade_ResultGetColumnName(DATABASE_RESULT_HANDLE p_pResult,
	size_t p_uiColumn, ST_FACADE_MUTABLE_BUFFER* p_pName)
{
	CDatabaseResult* pResult = GetResult(p_pResult);
	if (pResult == nullptr || p_uiColumn >= pResult->m_aColumns.size())
	{
		return FACADE_INVALID_ARGUMENT;
	}
	return CopyOutput(pResult->m_aColumns[p_uiColumn], p_pName);
}

int DatabaseFacade_ResultIsNull(DATABASE_RESULT_HANDLE p_pResult,
	size_t p_uiRow, size_t p_uiColumn, int32_t* p_piNull)
{
	CDatabaseResult* pResult = GetResult(p_pResult);
	if (pResult == nullptr || p_piNull == nullptr || p_uiRow >= pResult->m_aRows.size() ||
		p_uiColumn >= pResult->m_aRows[p_uiRow].size())
	{
		return FACADE_INVALID_ARGUMENT;
	}
	*p_piNull = pResult->m_aRows[p_uiRow][p_uiColumn].bNull ? 1 : 0;
	return FACADE_SUCCESS;
}

int DatabaseFacade_ResultGetField(DATABASE_RESULT_HANDLE p_pResult,
	size_t p_uiRow, size_t p_uiColumn, ST_FACADE_MUTABLE_BUFFER* p_pValue)
{
	CDatabaseResult* pResult = GetResult(p_pResult);
	if (pResult == nullptr || p_uiRow >= pResult->m_aRows.size() ||
		p_uiColumn >= pResult->m_aRows[p_uiRow].size())
	{
		return FACADE_INVALID_ARGUMENT;
	}
	const ST_DATABASE_VALUE& refValue = pResult->m_aRows[p_uiRow][p_uiColumn];
	return refValue.bNull ? FACADE_PROTOCOL_ERROR : CopyOutput(refValue.strValue, p_pValue);
}

int DatabaseFacade_ResultGetError(DATABASE_RESULT_HANDLE p_pResult,
	ST_FACADE_MUTABLE_BUFFER* p_pError)
{
	return GetResult(p_pResult) == nullptr ? FACADE_INVALID_ARGUMENT :
		CopyOutput(GetResult(p_pResult)->m_strError, p_pError);
}
