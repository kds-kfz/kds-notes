#include "nsdk_event.h"

#include <string.h>

#if !defined(OS_IS_WINDOWS)
#include <fcntl.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#endif

namespace
{
#if !defined(OS_IS_WINDOWS)
	static const unsigned int s_uiNamedEventReady = 0x4E534544;

#if defined(PTHREAD_MUTEX_ROBUST)
#define NSDK_EVENT_PTHREAD_MUTEX_ROBUST PTHREAD_MUTEX_ROBUST
#define nsdk_event_pthread_mutex_consistent pthread_mutex_consistent
#elif defined(PTHREAD_MUTEX_ROBUST_NP)
#define NSDK_EVENT_PTHREAD_MUTEX_ROBUST PTHREAD_MUTEX_ROBUST_NP
#define nsdk_event_pthread_mutex_consistent pthread_mutex_consistent_np
#endif

	struct CNamedEventStorage
	{
		volatile unsigned int m_uiReady;
		volatile int m_iRefCount;
		pthread_mutex_t m_stMutex;
		pthread_cond_t m_stCond;
		bool m_bManualReset; // 所有附着者共享的复位模式。
		bool m_bSignaled;    // 所有附着者共享的信号状态。
	};

	std::string NormalizeEventName(const char* p_pszName)
	{
		std::string strName = (NULL == p_pszName) ? "" : p_pszName;
		if (strName.empty())
		{
			return strName;
		}

		if ('/' != strName[0])
		{
			strName.insert(strName.begin(), '/');
		}

		for (size_t i = 1; i < strName.size(); ++i)
		{
			if ('/' == strName[i] || '\\' == strName[i])
			{
				strName[i] = '_';
			}
		}

		return strName;
	}

	std::string BuildEventLockFile(const std::string& p_strName)
	{
		std::string strFile = "/tmp/nsdk_event";
		strFile += p_strName;
		for (size_t i = 0; i < strFile.size(); ++i)
		{
			if ('/' == strFile[i] || '\\' == strFile[i])
			{
				strFile[i] = '_';
			}
		}
		strFile += ".lock";
		return strFile;
	}

	void SleepForRetry()
	{
		struct timespec stTs = {};
		stTs.tv_nsec = 1000000;
		nanosleep(&stTs, NULL);
	}

	bool BuildAbsTimeout(unsigned int p_uiMilliseconds, struct timespec& p_rstAbsTime)
	{
		if (clock_gettime(CLOCK_REALTIME, &p_rstAbsTime) != 0)
		{
			return false;
		}

		p_rstAbsTime.tv_sec += (p_uiMilliseconds / 1000);
		p_rstAbsTime.tv_nsec += static_cast<long>((p_uiMilliseconds % 1000) * 1000000UL);
		if (p_rstAbsTime.tv_nsec >= 1000000000L)
		{
			p_rstAbsTime.tv_sec += (p_rstAbsTime.tv_nsec / 1000000000L);
			p_rstAbsTime.tv_nsec %= 1000000000L;
		}
		return true;
	}

	int InitPthreadMutex(pthread_mutex_t* p_pstMutex, bool p_bProcessShared)
	{
		pthread_mutexattr_t stAttr = {};
		int iRet = pthread_mutexattr_init(&stAttr);
		if (0 != iRet)
		{
			return iRet;
		}

		if (p_bProcessShared)
		{
			iRet = pthread_mutexattr_setpshared(&stAttr, PTHREAD_PROCESS_SHARED);
			if (0 != iRet)
			{
				pthread_mutexattr_destroy(&stAttr);
				return iRet;
			}
		}

#if defined(NSDK_EVENT_PTHREAD_MUTEX_ROBUST)
		iRet = pthread_mutexattr_setrobust(&stAttr, NSDK_EVENT_PTHREAD_MUTEX_ROBUST);
		if (0 != iRet && EINVAL != iRet
#if defined(ENOTSUP)
			&& ENOTSUP != iRet
#endif
			)
		{
			pthread_mutexattr_destroy(&stAttr);
			return iRet;
		}
#endif

		iRet = pthread_mutex_init(p_pstMutex, &stAttr);
		pthread_mutexattr_destroy(&stAttr);
		return iRet;
	}

	int InitPthreadCond(pthread_cond_t* p_pstCond, bool p_bProcessShared)
	{
		pthread_condattr_t stAttr = {};
		int iRet = pthread_condattr_init(&stAttr);
		if (0 != iRet)
		{
			return iRet;
		}

		if (p_bProcessShared)
		{
			iRet = pthread_condattr_setpshared(&stAttr, PTHREAD_PROCESS_SHARED);
			if (0 != iRet)
			{
				pthread_condattr_destroy(&stAttr);
				return iRet;
			}
		}

		iRet = pthread_cond_init(p_pstCond, &stAttr);
		pthread_condattr_destroy(&stAttr);
		return iRet;
	}

	int LockEventMutex(pthread_mutex_t* p_pstMutex)
	{
		int iRet = pthread_mutex_lock(p_pstMutex);
#if defined(NSDK_EVENT_PTHREAD_MUTEX_ROBUST)
		if (EOWNERDEAD == iRet)
		{
			iRet = nsdk_event_pthread_mutex_consistent(p_pstMutex);
			if (0 == iRet)
			{
				return 0;
			}
		}
#endif
		return iRet;
	}

	int RecoverEventMutexResult(int p_iRet, pthread_mutex_t* p_pstMutex)
	{
#if defined(NSDK_EVENT_PTHREAD_MUTEX_ROBUST)
		if (EOWNERDEAD == p_iRet)
		{
			int iRet = nsdk_event_pthread_mutex_consistent(p_pstMutex);
			if (0 == iRet)
			{
				return 0;
			}

			return iRet;
		}
#endif
		return p_iRet;
	}

	int UnlockCoordFile(int p_iLockFd)
	{
		while (flock(p_iLockFd, LOCK_UN) != 0)
		{
			if (EINTR != errno)
			{
				return errno;
			}
		}

		return 0;
	}

	int LockCoordFileShared(int p_iLockFd)
	{
		while (flock(p_iLockFd, LOCK_SH) != 0)
		{
			if (EINTR != errno)
			{
				return errno;
			}
		}

		return 0;
	}

	int TryLockCoordFileExclusive(int p_iLockFd, bool& p_rbLocked)
	{
		p_rbLocked = false;
		while (flock(p_iLockFd, LOCK_EX | LOCK_NB) != 0)
		{
			if (EINTR == errno)
			{
				continue;
			}

			if (
#if defined(EWOULDBLOCK)
				EWOULDBLOCK == errno ||
#endif
#if defined(EAGAIN)
				EAGAIN == errno ||
#endif
				false)
			{
				return 0;
			}

			return errno;
		}

		p_rbLocked = true;
		return 0;
	}

	void ResetNamedEventCreateState(void*& p_rMappedAddr, int& p_riShmFd)
	{
		if (NULL != p_rMappedAddr)
		{
			munmap(p_rMappedAddr, sizeof(CNamedEventStorage));
			p_rMappedAddr = NULL;
		}

		if (p_riShmFd >= 0)
		{
			close(p_riShmFd);
			p_riShmFd = -1;
		}
	}
#endif
}

BGN_NAMESPACE_NSDK

CEvent::CEvent(void)
{
#if defined(OS_IS_WINDOWS)
	m_hEvent = NULL;
#else
	m_bNamed = false;
	m_strName.clear();
	m_strLockFile.clear();
	memset(&m_stMutex, 0, sizeof(m_stMutex));
	memset(&m_stCond, 0, sizeof(m_stCond));
	m_pstMutex = NULL;
	m_pstCond = NULL;
	m_bSignaled = false;
	m_pbSignaled = NULL;
	m_pMappedAddr = NULL;
	m_iShmFd = -1;
	m_iLockFd = -1;
	m_bManualReset = false;
	m_bInit = false;
#endif
	m_iLastError = NSDK_OK;
	m_strLastErrorMsg.clear();
}

CEvent::~CEvent()
{
	Close();
}

int CEvent::Create(bool p_bManualReset, bool p_bInitialState, const char* p_pszName)
{
	if (Close() != NSDK_OK)
	{
		return NSDK_KO;
	}

	m_iLastError = NSDK_OK;
	m_strLastErrorMsg.clear();

#if defined(OS_IS_WINDOWS)
	m_hEvent = ::CreateEventA(NULL, p_bManualReset ? TRUE : FALSE, p_bInitialState ? TRUE : FALSE,
		(NULL != p_pszName && '\0' != p_pszName[0]) ? p_pszName : NULL);
	if (NULL == m_hEvent)
	{
		return SetLastError("CEvent::Create", "CreateEventA");
	}
#else
	m_bManualReset = p_bManualReset;

	if (NULL == p_pszName || '\0' == p_pszName[0])
	{
		int iRet = InitPthreadMutex(&m_stMutex, false);
		if (0 != iRet)
		{
			errno = iRet;
			return SetLastError("CEvent::Create", "pthread_mutex_init");
		}

		iRet = InitPthreadCond(&m_stCond, false);
		if (0 != iRet)
		{
			pthread_mutex_destroy(&m_stMutex);
			memset(&m_stMutex, 0, sizeof(m_stMutex));
			errno = iRet;
			return SetLastError("CEvent::Create", "pthread_cond_init");
		}

		m_bSignaled = p_bInitialState;
		m_pstMutex = &m_stMutex;
		m_pstCond = &m_stCond;
		m_pbSignaled = &m_bSignaled;
		m_bInit = true;
		return NSDK_OK;
	}

	m_strName = NormalizeEventName(p_pszName);
	m_strLockFile = BuildEventLockFile(m_strName);

	m_iLockFd = open(m_strLockFile.c_str(), O_CREAT | O_RDWR, 0644);
	if (m_iLockFd < 0)
	{
		m_strLockFile.clear();
		m_strName.clear();
		return SetLastError("CEvent::Create", "open");
	}

	int iRet = 0;
	while (true)
	{
		bool bOwnExclusive = false;
		iRet = TryLockCoordFileExclusive(m_iLockFd, bOwnExclusive);
		if (0 != iRet)
		{
			close(m_iLockFd);
			m_iLockFd = -1;
			m_strLockFile.clear();
			m_strName.clear();
			errno = iRet;
			return SetLastError("CEvent::Create", "flock");
		}

		if (bOwnExclusive)
		{
			if (shm_unlink(m_strName.c_str()) != 0 && ENOENT != errno)
			{
				const int iErrno = errno;
				UnlockCoordFile(m_iLockFd);
				close(m_iLockFd);
				m_iLockFd = -1;
				m_strLockFile.clear();
				m_strName.clear();
				errno = iErrno;
				return SetLastError("CEvent::Create", "shm_unlink");
			}

			m_iShmFd = shm_open(m_strName.c_str(), O_RDWR | O_CREAT | O_EXCL, 0644);
			if (m_iShmFd < 0)
			{
				const int iErrno = errno;
				UnlockCoordFile(m_iLockFd);
				close(m_iLockFd);
				m_iLockFd = -1;
				m_strLockFile.clear();
				m_strName.clear();
				errno = iErrno;
				return SetLastError("CEvent::Create", "shm_open");
			}

			if (ftruncate(m_iShmFd, sizeof(CNamedEventStorage)) != 0)
			{
				const int iErrno = errno;
				ResetNamedEventCreateState(m_pMappedAddr, m_iShmFd);
				UnlockCoordFile(m_iLockFd);
				close(m_iLockFd);
				m_iLockFd = -1;
				m_strLockFile.clear();
				m_strName.clear();
				errno = iErrno;
				return SetLastError("CEvent::Create", "ftruncate");
			}

			m_pMappedAddr = mmap(NULL, sizeof(CNamedEventStorage), PROT_READ | PROT_WRITE, MAP_SHARED, m_iShmFd, 0);
			if (MAP_FAILED == m_pMappedAddr)
			{
				const int iErrno = errno;
				m_pMappedAddr = NULL;
				ResetNamedEventCreateState(m_pMappedAddr, m_iShmFd);
				UnlockCoordFile(m_iLockFd);
				close(m_iLockFd);
				m_iLockFd = -1;
				m_strLockFile.clear();
				m_strName.clear();
				errno = iErrno;
				return SetLastError("CEvent::Create", "mmap");
			}

			CNamedEventStorage* pstEvent = static_cast<CNamedEventStorage*>(m_pMappedAddr);
			memset(pstEvent, 0, sizeof(CNamedEventStorage));

			iRet = InitPthreadMutex(&pstEvent->m_stMutex, true);
			if (0 != iRet)
			{
				ResetNamedEventCreateState(m_pMappedAddr, m_iShmFd);
				UnlockCoordFile(m_iLockFd);
				close(m_iLockFd);
				m_iLockFd = -1;
				m_strLockFile.clear();
				m_strName.clear();
				errno = iRet;
				return SetLastError("CEvent::Create", "pthread_mutex_init");
			}

			iRet = InitPthreadCond(&pstEvent->m_stCond, true);
			if (0 != iRet)
			{
				pthread_mutex_destroy(&pstEvent->m_stMutex);
				ResetNamedEventCreateState(m_pMappedAddr, m_iShmFd);
				UnlockCoordFile(m_iLockFd);
				close(m_iLockFd);
				m_iLockFd = -1;
				m_strLockFile.clear();
				m_strName.clear();
				errno = iRet;
				return SetLastError("CEvent::Create", "pthread_cond_init");
			}

			pstEvent->m_iRefCount = 0;
			pstEvent->m_bManualReset = p_bManualReset;
			pstEvent->m_bSignaled = p_bInitialState;
			__sync_synchronize();
			pstEvent->m_uiReady = s_uiNamedEventReady;

			iRet = LockCoordFileShared(m_iLockFd);
			if (0 != iRet)
			{
				ResetNamedEventCreateState(m_pMappedAddr, m_iShmFd);
				close(m_iLockFd);
				m_iLockFd = -1;
				m_strLockFile.clear();
				m_strName.clear();
				errno = iRet;
				return SetLastError("CEvent::Create", "flock");
			}

			m_bNamed = true;
			m_bManualReset = pstEvent->m_bManualReset;
			m_pstMutex = &pstEvent->m_stMutex;
			m_pstCond = &pstEvent->m_stCond;
			m_pbSignaled = &pstEvent->m_bSignaled;
			m_bInit = true;
			break;
		}

		iRet = LockCoordFileShared(m_iLockFd);
		if (0 != iRet)
		{
			close(m_iLockFd);
			m_iLockFd = -1;
			m_strLockFile.clear();
			m_strName.clear();
			errno = iRet;
			return SetLastError("CEvent::Create", "flock");
		}

		m_iShmFd = shm_open(m_strName.c_str(), O_RDWR, 0644);
		if (m_iShmFd < 0)
		{
			const int iErrno = errno;
			UnlockCoordFile(m_iLockFd);
			if (ENOENT == iErrno)
			{
				SleepForRetry();
				continue;
			}

			close(m_iLockFd);
			m_iLockFd = -1;
			m_strLockFile.clear();
			m_strName.clear();
			errno = iErrno;
			return SetLastError("CEvent::Create", "shm_open");
		}

		struct stat stStat = {};
		if (0 != fstat(m_iShmFd, &stStat))
		{
			const int iErrno = errno;
			ResetNamedEventCreateState(m_pMappedAddr, m_iShmFd);
			UnlockCoordFile(m_iLockFd);
			close(m_iLockFd);
			m_iLockFd = -1;
			m_strLockFile.clear();
			m_strName.clear();
			errno = iErrno;
			return SetLastError("CEvent::Create", "fstat");
		}

		if (stStat.st_size < static_cast<off_t>(sizeof(CNamedEventStorage)))
		{
			ResetNamedEventCreateState(m_pMappedAddr, m_iShmFd);
			UnlockCoordFile(m_iLockFd);
			SleepForRetry();
			continue;
		}

		m_pMappedAddr = mmap(NULL, sizeof(CNamedEventStorage), PROT_READ | PROT_WRITE, MAP_SHARED, m_iShmFd, 0);
		if (MAP_FAILED == m_pMappedAddr)
		{
			const int iErrno = errno;
			m_pMappedAddr = NULL;
			ResetNamedEventCreateState(m_pMappedAddr, m_iShmFd);
			UnlockCoordFile(m_iLockFd);
			close(m_iLockFd);
			m_iLockFd = -1;
			m_strLockFile.clear();
			m_strName.clear();
			errno = iErrno;
			return SetLastError("CEvent::Create", "mmap");
		}

		CNamedEventStorage* pstEvent = static_cast<CNamedEventStorage*>(m_pMappedAddr);
		if (pstEvent->m_uiReady != s_uiNamedEventReady)
		{
			ResetNamedEventCreateState(m_pMappedAddr, m_iShmFd);
			UnlockCoordFile(m_iLockFd);
			SleepForRetry();
			continue;
		}

		m_bNamed = true;
		m_bManualReset = pstEvent->m_bManualReset;
		m_pstMutex = &pstEvent->m_stMutex;
		m_pstCond = &pstEvent->m_stCond;
		m_pbSignaled = &pstEvent->m_bSignaled;
		m_bInit = true;
		break;
	}
#endif

	return NSDK_OK;
}

int CEvent::Close(void)
{
#if defined(OS_IS_WINDOWS)
	if (NULL != m_hEvent)
	{
		if (!::CloseHandle(m_hEvent))
		{
			return SetLastError("CEvent::Close", "CloseHandle");
		}
		m_hEvent = NULL;
	}
#else
	if (m_bNamed)
	{
		int iErrno = 0;
		const char* pszCallFunc = NULL;

		if (NULL != m_pMappedAddr)
		{
			if (munmap(m_pMappedAddr, sizeof(CNamedEventStorage)) != 0)
			{
				iErrno = errno;
				pszCallFunc = "munmap";
			}
			m_pMappedAddr = NULL;
		}

		if (m_iShmFd >= 0)
		{
			if (close(m_iShmFd) != 0 && 0 == iErrno)
			{
				iErrno = errno;
				pszCallFunc = "close";
			}
			m_iShmFd = -1;
		}

		if (m_iLockFd >= 0)
		{
			int iRet = UnlockCoordFile(m_iLockFd);
			if (0 != iRet && 0 == iErrno)
			{
				iErrno = iRet;
				pszCallFunc = "flock";
			}

			if (close(m_iLockFd) != 0 && 0 == iErrno)
			{
				iErrno = errno;
				pszCallFunc = "close";
			}
			m_iLockFd = -1;
		}

		m_bNamed = false;
		m_bInit = false;
		m_bManualReset = false;
		m_pstMutex = NULL;
		m_pstCond = NULL;
		m_pbSignaled = NULL;
		m_strLockFile.clear();
		m_strName.clear();

		if (0 != iErrno)
		{
			errno = iErrno;
			return SetLastError("CEvent::Close", pszCallFunc);
		}
	}
	else if (m_bInit)
	{
		int iRet = pthread_cond_destroy(&m_stCond);
		if (0 != iRet)
		{
			errno = iRet;
			return SetLastError("CEvent::Close", "pthread_cond_destroy");
		}

		iRet = pthread_mutex_destroy(&m_stMutex);
		if (0 != iRet)
		{
			errno = iRet;
			return SetLastError("CEvent::Close", "pthread_mutex_destroy");
		}

		memset(&m_stMutex, 0, sizeof(m_stMutex));
		memset(&m_stCond, 0, sizeof(m_stCond));
		m_pstMutex = NULL;
		m_pstCond = NULL;
		m_pbSignaled = NULL;
		m_bSignaled = false;
		m_bManualReset = false;
		m_bInit = false;
	}
#endif

	m_iLastError = NSDK_OK;
	m_strLastErrorMsg.clear();
	return NSDK_OK;
}

int CEvent::SetEvent(void)
{
#if defined(OS_IS_WINDOWS)
	if (NULL == m_hEvent)
	{
		::SetLastError(ERROR_INVALID_HANDLE);
		return SetLastError("CEvent::SetEvent", "SetEvent");
	}

	if (!::SetEvent(m_hEvent))
	{
		return SetLastError("CEvent::SetEvent", "SetEvent");
	}
#else
	if (!m_bInit || NULL == m_pstMutex || NULL == m_pstCond || NULL == m_pbSignaled)
	{
		errno = EINVAL;
		return SetLastError("CEvent::SetEvent", "pthread_mutex_lock");
	}

	int iRet = LockEventMutex(m_pstMutex);
	if (0 != iRet)
	{
		errno = iRet;
		return SetLastError("CEvent::SetEvent", "pthread_mutex_lock");
	}

	*m_pbSignaled = true;
	// 手动复位事件唤醒全部等待者，自动复位事件只唤醒一个等待者。
	iRet = m_bManualReset ? pthread_cond_broadcast(m_pstCond) : pthread_cond_signal(m_pstCond);

	int iUnlockRet = pthread_mutex_unlock(m_pstMutex);
	if (0 == iRet && 0 != iUnlockRet)
	{
		iRet = iUnlockRet;
	}

	if (0 != iRet)
	{
		errno = iRet;
		return SetLastError("CEvent::SetEvent", "pthread_cond_signal");
	}
#endif

	m_iLastError = NSDK_OK;
	m_strLastErrorMsg.clear();
	return NSDK_OK;
}

int CEvent::ResetEvent(void)
{
#if defined(OS_IS_WINDOWS)
	if (NULL == m_hEvent)
	{
		::SetLastError(ERROR_INVALID_HANDLE);
		return SetLastError("CEvent::ResetEvent", "ResetEvent");
	}

	if (!::ResetEvent(m_hEvent))
	{
		return SetLastError("CEvent::ResetEvent", "ResetEvent");
	}
#else
	if (!m_bInit || NULL == m_pstMutex || NULL == m_pbSignaled)
	{
		errno = EINVAL;
		return SetLastError("CEvent::ResetEvent", "pthread_mutex_lock");
	}

	int iRet = LockEventMutex(m_pstMutex);
	if (0 != iRet)
	{
		errno = iRet;
		return SetLastError("CEvent::ResetEvent", "pthread_mutex_lock");
	}

	*m_pbSignaled = false;

	iRet = pthread_mutex_unlock(m_pstMutex);
	if (0 != iRet)
	{
		errno = iRet;
		return SetLastError("CEvent::ResetEvent", "pthread_mutex_unlock");
	}
#endif

	m_iLastError = NSDK_OK;
	m_strLastErrorMsg.clear();
	return NSDK_OK;
}

int CEvent::WaitEvent(unsigned int p_uiMilliseconds)
{
#if defined(OS_IS_WINDOWS)
	if (NULL == m_hEvent)
	{
		::SetLastError(ERROR_INVALID_HANDLE);
		return SetLastError("CEvent::WaitEvent", "WaitForSingleObject");
	}

	const DWORD dwRet = ::WaitForSingleObject(m_hEvent, p_uiMilliseconds);
	if (WAIT_OBJECT_0 == dwRet)
	{
		m_iLastError = NSDK_OK;
		m_strLastErrorMsg.clear();
		return WAIT_OBJECT_0;
	}

	if (WAIT_TIMEOUT == dwRet)
	{
		m_iLastError = WAIT_TIMEOUT;
		m_strLastErrorMsg = "CEvent::WaitEvent timed out.";
		return WAIT_TIMEOUT;
	}

	return SetLastError("CEvent::WaitEvent", "WaitForSingleObject");
#else
	if (!m_bInit || NULL == m_pstMutex || NULL == m_pstCond || NULL == m_pbSignaled)
	{
		errno = EINVAL;
		return SetLastError("CEvent::WaitEvent", "pthread_mutex_lock");
	}

	int iRet = LockEventMutex(m_pstMutex);
	if (0 != iRet)
	{
		errno = iRet;
		return SetLastError("CEvent::WaitEvent", "pthread_mutex_lock");
	}

	if (!(*m_pbSignaled))
	{
		if (0 == p_uiMilliseconds)
		{
			pthread_mutex_unlock(m_pstMutex);
			m_iLastError = WAIT_TIMEOUT;
			m_strLastErrorMsg = "CEvent::WaitEvent timed out.";
			return WAIT_TIMEOUT;
		}

		if (INFINITE == p_uiMilliseconds)
		{
			while (!(*m_pbSignaled))
			{
				// 等待期间如果遇到 robust mutex owner 异常退出，需要先恢复一致性。
				iRet = pthread_cond_wait(m_pstCond, m_pstMutex);
				iRet = RecoverEventMutexResult(iRet, m_pstMutex);
				if (0 != iRet)
				{
					pthread_mutex_unlock(m_pstMutex);
					errno = iRet;
					return SetLastError("CEvent::WaitEvent", "pthread_cond_wait");
				}
			}
		}
		else
		{
			struct timespec stAbsTime = {};
			if (!BuildAbsTimeout(p_uiMilliseconds, stAbsTime))
			{
				const int iErrno = errno;
				pthread_mutex_unlock(m_pstMutex);
				errno = iErrno;
				return SetLastError("CEvent::WaitEvent", "clock_gettime");
			}

			while (!(*m_pbSignaled))
			{
				// 定时等待和无限等待共用同一套恢复路径。
				iRet = pthread_cond_timedwait(m_pstCond, m_pstMutex, &stAbsTime);
				iRet = RecoverEventMutexResult(iRet, m_pstMutex);
				if (ETIMEDOUT == iRet)
				{
					pthread_mutex_unlock(m_pstMutex);
					m_iLastError = WAIT_TIMEOUT;
					m_strLastErrorMsg = "CEvent::WaitEvent timed out.";
					return WAIT_TIMEOUT;
				}

				if (0 != iRet)
				{
					pthread_mutex_unlock(m_pstMutex);
					errno = iRet;
					return SetLastError("CEvent::WaitEvent", "pthread_cond_timedwait");
				}
			}
		}
	}

	if (!m_bManualReset)
	{
		// 自动复位事件在成功放行一个等待者后立即清除信号状态。
		*m_pbSignaled = false;
	}

	iRet = pthread_mutex_unlock(m_pstMutex);
	if (0 != iRet)
	{
		errno = iRet;
		return SetLastError("CEvent::WaitEvent", "pthread_mutex_unlock");
	}
#endif

	m_iLastError = NSDK_OK;
	m_strLastErrorMsg.clear();
	return WAIT_OBJECT_0;
}

int CEvent::SetLastError(const char* p_pszHostFunc, const char* p_pszCallFunc)
{
#if defined(OS_IS_WINDOWS)
	m_iLastError = static_cast<int>(::GetLastError());

	LPSTR pszError = NULL;
	DWORD dwLen = ::FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		static_cast<DWORD>(m_iLastError),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPSTR>(&pszError),
		0,
		NULL);
#else
	m_iLastError = errno;
#endif

	std::ostringstream oss;
	oss << (NULL != p_pszHostFunc ? p_pszHostFunc : "CEvent")
		<< " call "
		<< (NULL != p_pszCallFunc ? p_pszCallFunc : "")
		<< " failed, error=" << m_iLastError;

#if defined(OS_IS_WINDOWS)
	if (0 != dwLen && NULL != pszError)
	{
		while (dwLen > 0 &&
			('\r' == pszError[dwLen - 1] || '\n' == pszError[dwLen - 1] || ' ' == pszError[dwLen - 1]))
		{
			pszError[dwLen - 1] = '\0';
			--dwLen;
		}
		oss << ": " << pszError;
	}

	if (NULL != pszError)
	{
		::LocalFree(pszError);
	}
#else
	const char* p_pszErr = strerror(m_iLastError);
	if (NULL != p_pszErr && '\0' != p_pszErr[0])
	{
		oss << ": " << p_pszErr;
	}
#endif

	m_strLastErrorMsg = oss.str();
	return WAIT_FAILED;
}

END_NAMESPACE_NSDK

