#ifndef _NSDK_ATOMIC_H_
#define _NSDK_ATOMIC_H_

#include "nsdk_define.h"
#include <cmath>

#ifdef __cplusplus
extern "C" {
#endif
#include <stdio.h>

//公共部分
#define nsdk_max(a,b)            (((a) > (b)) ? (a) : (b))
#define nsdk_min(a,b)            (((a) < (b)) ? (a) : (b))
#define nsdk_arry_size(arr)		(sizeof(arr) / sizeof((arr)[0]))
#define nsdk_del(a)				{if (a) {delete a;a=NULL;}}
#define nsdk_del_arry(a)		{if (a) {delete []a;a=NULL;}}
#define nsdk_free(a)			{if (a) {free(a);a=NULL;}}
#define nsdk_malloc(ptr,size,type)	{ ptr = new type[size/sizeof(type)];\
									if(ptr) memset(ptr,0,size);\

//跨平台部分
#if defined(OS_IS_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <Windows.h>
#define NSDK_PATH_DELIMETER "\\"
#define nsdk_atomic_t                         LONG
#define nsdk_AtomicAdd(mem, val)              InterlockedExchangeAdd(mem, val)
#define nsdk_AtomicSet(mem, val)              InterlockedExchange(mem, val)
#define nsdk_AtomicInc(mem)                   InterlockedIncrement(mem)
#define nsdk_AtomicDec(mem)                   InterlockedDecrement(mem)
#define nsdk_AtomicCas(mem, with, cmp)        InterlockedCompareExchange(mem, with, cmp)
#define nsdk_AtomicRead(mem)                  (*mem)

#define nsdk_Sleep(p_uiMilliseconds)          ::Sleep(p_uiMilliseconds)

#define nsdk_atomic_t64                       LONG64
#define nsdk_AtomicAdd64(mem, val)            InterlockedExchangeAdd64(mem, val)
#define nsdk_AtomicSet64(mem, val)            InterlockedExchange64(mem, val)
#define nsdk_AtomicInc64(mem)                 InterlockedIncrement64(mem)
#define nsdk_AtomicDec64(mem)                 InterlockedDecrement64(mem)
#define nsdk_AtomicCas64(mem, with, cmp)      InterlockedCompareExchange64(mem, with, cmp)
#define nsdk_AtomicRead64(mem)                (*mem)

#define nsdk_snprintf(buf, size, fmt, ...)     _snprintf(buf, size, fmt, ##__VA_ARGS__)
#define nsdk_isnan(num)                        _isnan(num)
#define nsdk_finite(num)                       _finite(num)
#elif defined(OS_IS_LINUX)
#include <unistd.h>
#include <math.h>
#define NSDK_PATH_DELIMETER "/"
#define nsdk_Sleep(p_uiMilliseconds)          ::usleep((p_uiMilliseconds)*1000)

typedef volatile int nsdk_atomic_t;
#define nsdk_AtomicAdd(mem, val)              (__sync_fetch_and_add(mem, (val)))
#define nsdk_AtomicSet(mem, val)              (*mem = (val))
#define nsdk_AtomicInc(mem)                   (__sync_add_and_fetch(mem, 1))
#define nsdk_AtomicDec(mem)                   (__sync_sub_and_fetch(mem, 1))
#define nsdk_AtomicCas(mem, with, cmp)        (__sync_val_compare_and_swap (mem, cmp, with))
#define nsdk_AtomicRead(mem)                  (*mem)

typedef volatile long long                    nsdk_atomic_t64;
#define nsdk_AtomicAdd64(mem, val)            (__sync_fetch_and_add(mem, (val)))
#define nsdk_AtomicSet64(mem, val)            (*mem = (val))
#define nsdk_AtomicInc64(mem)                 (__sync_add_and_fetch(mem, 1))
#define nsdk_AtomicDec64(mem)                 (__sync_sub_and_fetch(mem, 1))
#define nsdk_AtomicCas64(mem, with, cmp)      (__sync_val_compare_and_swap (mem, cmp, with))
#define nsdk_AtomicRead64(mem)                (*mem)

#define nsdk_snprintf(buf, size, fmt, ...)     snprintf(buf, size, fmt, ##__VA_ARGS__)
#define nsdk_isnan(num)                        std::isnan(num)
#define nsdk_finite(num)                       !std::isinf(num)
#endif

#ifdef __cplusplus
}
#endif

BGN_NAMESPACE_NSDK

class CAtomicMutex
{
public:
	CAtomicMutex(void)
	{
		Create();
	};

	virtual ~CAtomicMutex(void)
	{
	};

	int Create(void)
	{
		m_lIsLocked = 0;
		return 0;
	}

	int Close(void)
	{
		return 0;
	}

	int Lock(void)
	{
		while (nsdk_AtomicCas(&m_lIsLocked, 1, 0) != 0)
		{
		}
		return 0;
	}

	int Unlock(void)
	{
		while (nsdk_AtomicCas(&m_lIsLocked, 0, 1) != 1)
		{
		}
		return 0;
	}

	int GetLastError(void) { return 0; }              // 为兼容nsdk::CMutex而保留，无实际用途
	const char* GetLastErrorMsg(void) { return ""; } // 为兼容nsdk::CMutex而保留，无实际用途

private:
	nsdk_atomic_t m_lIsLocked;
};

class CAtomicRWLock
{
public:
	CAtomicRWLock(void) { m_lWriting = 0; m_lReading = 0; }
	~CAtomicRWLock() {}

	int Create(void) { m_lWriting = 0; m_lReading = 0; return 0; }
	int Close(void) { return 0; }

	int ReadLock(void)
	{
		while (0 != nsdk_AtomicCas(&m_lWriting, 1, 0))
		{
		}
		nsdk_AtomicAdd(&m_lReading, 1);
		while (1 != nsdk_AtomicCas(&m_lWriting, 0, 1))
		{
		}

		return 0;
	}

	int ReadUnlock(void)
	{
		nsdk_AtomicAdd(&m_lReading, -1);
		return 0;
	}

	int WriteLock(void)
	{
		while (0 != nsdk_AtomicCas(&m_lWriting, 1, 0))
		{
		}

		while (m_lReading > 0)
		{
		}

		return 0;
	}

	int WriteUnlock(void)
	{
		while (1 != nsdk_AtomicCas(&m_lWriting, 0, 1))
		{
		}

		return 0;
	}

	int GetReadLockCount(void) { return m_lReading; }
	bool IsLockedForWrite(void) { return m_lWriting > 0; }

	int GetLastError(void) { return 0; }              // 为兼容nsdk::CRWLock而保留，无实际用途
	const char* GetLastErrorMsg(void) { return ""; } // 为兼容nsdk::CRWLock而保留，无实际用途

private:
	nsdk_atomic_t m_lWriting;      // 正在写的数量
	nsdk_atomic_t m_lReading;      // 正在读的数量
};

END_NAMESPACE_NSDK

#endif  // _NSDK_ATOMIC_H_
