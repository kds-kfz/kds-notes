#if !defined(__xsdk_atomic_h__)
#define __xsdk_atomic_h__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(OS_IS_WINDOWS)
  #include <Windows.h>
  #define xsdk_atomic_t                         LONG
  #define xsdk_AtomicAdd(mem, val)              InterlockedExchangeAdd(mem, val)
  #define xsdk_AtomicSet(mem, val)              InterlockedExchange(mem, val)
  #define xsdk_AtomicInc(mem)                   InterlockedIncrement(mem)
  #define xsdk_AtomicDec(mem)                   InterlockedDecrement(mem)
  #define xsdk_AtomicCas(mem, with, cmp)        InterlockedCompareExchange(mem, with, cmp)
  #define xsdk_AtomicRead(mem)                  (*mem)

  #define xsdk_Sleep(p_uiMilliseconds)          ::Sleep(p_uiMilliseconds)

  #define xsdk_atomic_t64                       LONG64
  #define xsdk_AtomicAdd64(mem, val)            InterlockedExchangeAdd64(mem, val)
  #define xsdk_AtomicSet64(mem, val)            InterlockedExchange64(mem, val)
  #define xsdk_AtomicInc64(mem)                 InterlockedIncrement64(mem)
  #define xsdk_AtomicDec64(mem)                 InterlockedDecrement64(mem)
  #define xsdk_AtomicCas64(mem, with, cmp)      InterlockedCompareExchange64(mem, with, cmp)
  #define xsdk_AtomicRead64(mem)                (*mem)
#elif defined(OS_IS_LINUX)
  #include "unistd.h"
  #define xsdk_Sleep(p_uiMilliseconds)          ::usleep((p_uiMilliseconds)*1000)

  typedef volatile int xsdk_atomic_t;
  #define xsdk_AtomicAdd(mem, val)              (__sync_fetch_and_add(mem, (val)))
  #define xsdk_AtomicSet(mem, val)              (*mem = (val))
  #define xsdk_AtomicInc(mem)                   (__sync_add_and_fetch(mem, 1))
  #define xsdk_AtomicDec(mem)                   (__sync_sub_and_fetch(mem, 1))
  #define xsdk_AtomicCas(mem, with, cmp)        (__sync_val_compare_and_swap (mem, cmp, with))
  #define xsdk_AtomicRead(mem)                  (*mem)

  typedef volatile long long                    xsdk_atomic_t64;
  #define xsdk_AtomicAdd64(mem, val)            (__sync_fetch_and_add(mem, (val)))
  #define xsdk_AtomicSet64(mem, val)            (*mem = (val))
  #define xsdk_AtomicInc64(mem)                 (__sync_add_and_fetch(mem, 1))
  #define xsdk_AtomicDec64(mem)                 (__sync_sub_and_fetch(mem, 1))
  #define xsdk_AtomicCas64(mem, with, cmp)      (__sync_val_compare_and_swap (mem, cmp, with))
  #define xsdk_AtomicRead64(mem)                (*mem)
#endif

#ifdef __cplusplus
}
#endif

#include "xsdk_define.h"

BGN_NAMESPACE_XSDK

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
    while (xsdk_AtomicCas(&m_lIsLocked, 1, 0) != 0)
    {
      NULL;
    }
    return 0;
  }

  int Unlock(void)
  {
    while (xsdk_AtomicCas(&m_lIsLocked, 0, 1) != 1)
    {
        NULL;
    }
    return 0;
  }

  int GetLastError(void) {return 0;}              // 为兼容xsdk::CMutex而保留，无实际用途
  const char * GetLastErrorMsg(void) {return "";} // 为兼容xsdk::CMutex而保留，无实际用途

private:
    xsdk_atomic_t m_lIsLocked;
};

class CAtomicRWLock
{
public:
  CAtomicRWLock(void) {m_lWriting = 0; m_lReading = 0;}
  ~CAtomicRWLock() {}

  int Create(void) {m_lWriting = 0; m_lReading = 0; return 0;}
  int Close(void) {return 0;}

  int ReadLock(void)
  {
    while (0 != xsdk_AtomicCas(&m_lWriting, 1, 0))
    {
    }
    xsdk_AtomicAdd(&m_lReading, 1);
    while (1 != xsdk_AtomicCas(&m_lWriting, 0, 1))
    {
    }

    return 0;
  }

  int ReadUnlock(void)
  {
    xsdk_AtomicAdd(&m_lReading, -1);
    return 0;
  }

  int WriteLock(void)
  {
    while (0 != xsdk_AtomicCas(&m_lWriting, 1, 0))
    {
    }

    while (m_lReading > 0)
    {
    }

    return 0;
  }

  int WriteUnlock(void)
  {
    while (1 != xsdk_AtomicCas(&m_lWriting, 0, 1))
    {
    }

    return 0;
  }

  int GetReadLockCount(void) {return m_lReading;}
  bool IsLockedForWrite(void) {return m_lWriting > 0; }

  int GetLastError(void) {return 0;}              // 为兼容xsdk::CRWLock而保留，无实际用途
  const char * GetLastErrorMsg(void) {return "";} // 为兼容xsdk::CRWLock而保留，无实际用途

private:
  xsdk_atomic_t m_lWriting;      // 正在写的数量
  xsdk_atomic_t m_lReading;      // 正在读的数量
};

END_NAMESPACE_XSDK

#endif  // __xsdk_atomic_h__