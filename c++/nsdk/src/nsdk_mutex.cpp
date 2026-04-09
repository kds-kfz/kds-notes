#include "nsdk_mutex.h"

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
	static const unsigned int s_uiNamedMutexReady = 0x4E53444B;

#if defined(PTHREAD_MUTEX_ROBUST)
#define NSDK_PTHREAD_MUTEX_ROBUST PTHREAD_MUTEX_ROBUST
#define nsdk_pthread_mutex_consistent pthread_mutex_consistent
#elif defined(PTHREAD_MUTEX_ROBUST_NP)
#define NSDK_PTHREAD_MUTEX_ROBUST PTHREAD_MUTEX_ROBUST_NP
#define nsdk_pthread_mutex_consistent pthread_mutex_consistent_np
#endif

	struct CNamedMutexStorage
	{
		volatile unsigned int m_uiReady;
		volatile int m_iRefCount;
		pthread_mutex_t m_stMutex; // 命名锁真正共享的互斥量实体。
	};

	std::string NormalizeMutexName(const char* p_pszName)
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

	std::string BuildMutexLockFile(const std::string& p_strName)
	{
		std::string strFile = "/tmp/nsdk_mutex";
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

#if defined(NSDK_PTHREAD_MUTEX_ROBUST)
		iRet = pthread_mutexattr_setrobust(&stAttr, NSDK_PTHREAD_MUTEX_ROBUST);
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

	int LockPthreadMutex(pthread_mutex_t* p_pstMutex, unsigned int p_uiMilliseconds, bool& p_rbRecovered)
	{
		p_rbRecovered = false;

		int iRet = 0;
		if (INFINITE == p_uiMilliseconds)
		{
			iRet = pthread_mutex_lock(p_pstMutex);
		}
		else
		{
			struct timespec stAbsTime = {};
			if (!BuildAbsTimeout(p_uiMilliseconds, stAbsTime))
			{
				return errno;
			}
			iRet = pthread_mutex_timedlock(p_pstMutex, &stAbsTime);
		}

#if defined(NSDK_PTHREAD_MUTEX_ROBUST)
		if (EOWNERDEAD == iRet)
		{
			iRet = nsdk_pthread_mutex_consistent(p_pstMutex);
			if (0 != iRet)
			{
				return iRet;
			}

			p_rbRecovered = true;
			return 0;
		}
#endif

		return iRet;
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

	void ResetNamedMutexCreateState(void*& p_rMappedAddr, int& p_riShmFd)
	{
		if (NULL != p_rMappedAddr)
		{
			munmap(p_rMappedAddr, sizeof(CNamedMutexStorage));
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

CMutex::CMutex(void)
{
#if defined(OS_IS_WINDOWS)
	m_hMutex = NULL;
#else
	m_bNamed = false;
	m_strName.clear();
	m_strLockFile.clear();
	memset(&m_stMutex, 0, sizeof(m_stMutex));
	m_pstMutex = NULL;
	m_pMappedAddr = NULL;
	m_iShmFd = -1;
	m_iLockFd = -1;
	m_bInit = false;
#endif
	m_iLastError = 0;
	m_strLastErrorMsg.clear();
}

CMutex::~CMutex()
{
	Close();
}

int CMutex::Create(const char* p_pszName)
{
	if (Close() != 0)
	{
		return -1;
	}

	m_iLastError = 0;
	m_strLastErrorMsg.clear();

#if defined(OS_IS_WINDOWS)
	m_hMutex = ::CreateMutexA(NULL, FALSE,
		(NULL != p_pszName && '\0' != p_pszName[0]) ? p_pszName : NULL);
	if (NULL == m_hMutex)
	{
		return SetLastError("CMutex::Create", "CreateMutexA");
	}
#else
	if (NULL == p_pszName || '\0' == p_pszName[0])
	{
		int iRet = InitPthreadMutex(&m_stMutex, false);
		if (0 != iRet)
		{
			errno = iRet;
			return SetLastError("CMutex::Create", "pthread_mutex_init");
		}

		m_pstMutex = &m_stMutex;
		m_bInit = true;
		return 0;
	}

	m_strName = NormalizeMutexName(p_pszName);
	m_strLockFile = BuildMutexLockFile(m_strName);

	m_iLockFd = open(m_strLockFile.c_str(), O_CREAT | O_RDWR, 0644);
	if (m_iLockFd < 0)
	{
		m_strLockFile.clear();
		m_strName.clear();
		return SetLastError("CMutex::Create", "open");
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
			return SetLastError("CMutex::Create", "flock");
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
				return SetLastError("CMutex::Create", "shm_unlink");
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
				return SetLastError("CMutex::Create", "shm_open");
			}

			if (ftruncate(m_iShmFd, sizeof(CNamedMutexStorage)) != 0)
			{
				const int iErrno = errno;
				ResetNamedMutexCreateState(m_pMappedAddr, m_iShmFd);
				UnlockCoordFile(m_iLockFd);
				close(m_iLockFd);
				m_iLockFd = -1;
				m_strLockFile.clear();
				m_strName.clear();
				errno = iErrno;
				return SetLastError("CMutex::Create", "ftruncate");
			}

			m_pMappedAddr = mmap(NULL, sizeof(CNamedMutexStorage), PROT_READ | PROT_WRITE, MAP_SHARED, m_iShmFd, 0);
			if (MAP_FAILED == m_pMappedAddr)
			{
				const int iErrno = errno;
				m_pMappedAddr = NULL;
				ResetNamedMutexCreateState(m_pMappedAddr, m_iShmFd);
				UnlockCoordFile(m_iLockFd);
				close(m_iLockFd);
				m_iLockFd = -1;
				m_strLockFile.clear();
				m_strName.clear();
				errno = iErrno;
				return SetLastError("CMutex::Create", "mmap");
			}

			CNamedMutexStorage* pstNamedMutex = static_cast<CNamedMutexStorage*>(m_pMappedAddr);
			memset(pstNamedMutex, 0, sizeof(CNamedMutexStorage));

			iRet = InitPthreadMutex(&pstNamedMutex->m_stMutex, true);
			if (0 != iRet)
			{
				ResetNamedMutexCreateState(m_pMappedAddr, m_iShmFd);
				UnlockCoordFile(m_iLockFd);
				close(m_iLockFd);
				m_iLockFd = -1;
				m_strLockFile.clear();
				m_strName.clear();
				errno = iRet;
				return SetLastError("CMutex::Create", "pthread_mutex_init");
			}

			pstNamedMutex->m_iRefCount = 0;
			__sync_synchronize();
			pstNamedMutex->m_uiReady = s_uiNamedMutexReady;

			iRet = LockCoordFileShared(m_iLockFd);
			if (0 != iRet)
			{
				ResetNamedMutexCreateState(m_pMappedAddr, m_iShmFd);
				close(m_iLockFd);
				m_iLockFd = -1;
				m_strLockFile.clear();
				m_strName.clear();
				errno = iRet;
				return SetLastError("CMutex::Create", "flock");
			}

			m_bNamed = true;
			m_pstMutex = &pstNamedMutex->m_stMutex;
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
			return SetLastError("CMutex::Create", "flock");
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
			return SetLastError("CMutex::Create", "shm_open");
		}

		struct stat stStat = {};
		if (0 != fstat(m_iShmFd, &stStat))
		{
			const int iErrno = errno;
			ResetNamedMutexCreateState(m_pMappedAddr, m_iShmFd);
			UnlockCoordFile(m_iLockFd);
			close(m_iLockFd);
			m_iLockFd = -1;
			m_strLockFile.clear();
			m_strName.clear();
			errno = iErrno;
			return SetLastError("CMutex::Create", "fstat");
		}

		if (stStat.st_size < static_cast<off_t>(sizeof(CNamedMutexStorage)))
		{
			ResetNamedMutexCreateState(m_pMappedAddr, m_iShmFd);
			UnlockCoordFile(m_iLockFd);
			SleepForRetry();
			continue;
		}

		m_pMappedAddr = mmap(NULL, sizeof(CNamedMutexStorage), PROT_READ | PROT_WRITE, MAP_SHARED, m_iShmFd, 0);
		if (MAP_FAILED == m_pMappedAddr)
		{
			const int iErrno = errno;
			m_pMappedAddr = NULL;
			ResetNamedMutexCreateState(m_pMappedAddr, m_iShmFd);
			UnlockCoordFile(m_iLockFd);
			close(m_iLockFd);
			m_iLockFd = -1;
			m_strLockFile.clear();
			m_strName.clear();
			errno = iErrno;
			return SetLastError("CMutex::Create", "mmap");
		}

		CNamedMutexStorage* pstNamedMutex = static_cast<CNamedMutexStorage*>(m_pMappedAddr);
		if (pstNamedMutex->m_uiReady != s_uiNamedMutexReady)
		{
			ResetNamedMutexCreateState(m_pMappedAddr, m_iShmFd);
			UnlockCoordFile(m_iLockFd);
			SleepForRetry();
			continue;
		}

		m_bNamed = true;
		m_pstMutex = &pstNamedMutex->m_stMutex;
		m_bInit = true;
		break;
	}
#endif

	return 0;
}

int CMutex::Close(void)
{
#if defined(OS_IS_WINDOWS)
	if (NULL != m_hMutex)
	{
		if (!::CloseHandle(m_hMutex))
		{
			return SetLastError("CMutex::Close", "CloseHandle");
		}
		m_hMutex = NULL;
	}
#else
	if (m_bNamed)
	{
		int iErrno = 0;
		const char* pszCallFunc = NULL;

		if (NULL != m_pMappedAddr)
		{
			if (munmap(m_pMappedAddr, sizeof(CNamedMutexStorage)) != 0)
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
		m_pstMutex = NULL;
		m_strLockFile.clear();
		m_strName.clear();

		if (0 != iErrno)
		{
			errno = iErrno;
			return SetLastError("CMutex::Close", pszCallFunc);
		}
	}
	else if (m_bInit)
	{
		int iRet = pthread_mutex_destroy(&m_stMutex);
		if (0 != iRet)
		{
			errno = iRet;
			return SetLastError("CMutex::Close", "pthread_mutex_destroy");
		}

		memset(&m_stMutex, 0, sizeof(m_stMutex));
		m_pstMutex = NULL;
		m_bInit = false;
	}
#endif

	m_iLastError = 0;
	m_strLastErrorMsg.clear();
	return 0;
}

int CMutex::Lock(unsigned int p_uiMilliseconds)
{
#if defined(OS_IS_WINDOWS)
	if (NULL == m_hMutex)
	{
		::SetLastError(ERROR_INVALID_HANDLE);
		return SetLastError("CMutex::Lock", "WaitForSingleObject");
	}

	const DWORD dwRet = ::WaitForSingleObject(m_hMutex, p_uiMilliseconds);
	if (WAIT_OBJECT_0 == dwRet)
	{
		m_iLastError = 0;
		m_strLastErrorMsg.clear();
		return 0;
	}

	if (WAIT_ABANDONED == dwRet)
	{
		// 锁已经拿到，但受保护数据可能需要调用方补做一致性检查。
		m_iLastError = WAIT_ABANDONED;
		m_strLastErrorMsg = "CMutex::Lock acquired an abandoned mutex; protected state may need recovery.";
		return NSDK_OK;
	}

	if (WAIT_TIMEOUT == dwRet)
	{
		m_iLastError = WAIT_TIMEOUT;
		m_strLastErrorMsg = "CMutex::Lock call WaitForSingleObject timed out.";
		return -1;
	}

	return SetLastError("CMutex::Lock", "WaitForSingleObject");
#else
	if (!m_bInit || NULL == m_pstMutex)
	{
		errno = EINVAL;
		return SetLastError("CMutex::Lock", "pthread_mutex_lock");
	}

	bool bRecovered = false;
	const int iRet = LockPthreadMutex(m_pstMutex, p_uiMilliseconds, bRecovered);
	if (0 != iRet)
	{
		errno = iRet;
		return SetLastError("CMutex::Lock", (INFINITE == p_uiMilliseconds) ? "pthread_mutex_lock" : "pthread_mutex_timedlock");
	}

	if (bRecovered)
	{
		// robust mutex 恢复后仍按成功返回，恢复信息通过错误状态暴露给调用方。
		m_iLastError = EOWNERDEAD;
		m_strLastErrorMsg = "CMutex::Lock acquired a recovered mutex after the previous owner exited unexpectedly; protected state may need verification.";
		return NSDK_OK;
	}

	m_iLastError = 0;
	m_strLastErrorMsg.clear();
	return 0;
#endif
}

int CMutex::Unlock(void)
{
#if defined(OS_IS_WINDOWS)
	if (NULL == m_hMutex)
	{
		::SetLastError(ERROR_INVALID_HANDLE);
		return SetLastError("CMutex::Unlock", "ReleaseMutex");
	}

	if (!::ReleaseMutex(m_hMutex))
	{
		return SetLastError("CMutex::Unlock", "ReleaseMutex");
	}
#else
	if (!m_bInit || NULL == m_pstMutex)
	{
		errno = EINVAL;
		return SetLastError("CMutex::Unlock", "pthread_mutex_unlock");
	}

	int iRet = pthread_mutex_unlock(m_pstMutex);
	if (0 != iRet)
	{
		errno = iRet;
		return SetLastError("CMutex::Unlock", "pthread_mutex_unlock");
	}
#endif

	m_iLastError = 0;
	m_strLastErrorMsg.clear();
	return 0;
}

int CMutex::SetLastError(const char* p_pszHostFunc, const char* p_pszCallFunc)
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
	oss << (NULL != p_pszHostFunc ? p_pszHostFunc : "CMutex")
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
	return -1;
}

END_NAMESPACE_NSDK
