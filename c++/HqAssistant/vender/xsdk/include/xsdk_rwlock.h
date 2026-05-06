#if !defined(__xsdk_rwlock_h__)
#define __xsdk_rwlock_h__

#if defined(OS_IS_WINDOWS)
  #include <windows.h>
#endif

#include <string>
#include <sstream>

#include "xsdk_define.h"
#include "xsdk_mutex.h"
#include "xsdk_event.h"
#include "xsdk_shm.h"

BGN_NAMESPACE_XSDK

class CRWLock
{
public:
  CRWLock(void);
  ~CRWLock();

  int Create(const char *p_pszName = NULL);
  int Close(void);

  int ReadLock(unsigned int p_uiMilliseconds = INFINITE);
  int ReadUnlock(void);

  int WriteLock(unsigned int p_uiMilliseconds = INFINITE);
  int WriteUnlock(void);

  int GetReadLockCount(void) const;
  bool IsLockedForWrite(void) const;

  int GetLastError(void) {return m_iLastError;}
  const char * GetLastErrorMsg(void) {return m_strLastErrorMsg.c_str();}  

protected:
  std::string m_strName;

  xsdk::CMutex m_clMutex;
  xsdk::CEvent m_clEventReaders;
  xsdk::CEvent m_clEventWriters;
  xsdk::CShm m_clShm;
  struct ST_RWLOCK_DATA
  {
    int iInitialized;
    int iWaitReaders;
    int iWaitWriters;
  }m_stRWLockData, *m_pstRWLockData;

  bool m_bInit;

  int m_iLastError;
  std::string m_strLastErrorMsg;
};

END_NAMESPACE_XSDK

#endif  // __xsdk_rwlock_h__
