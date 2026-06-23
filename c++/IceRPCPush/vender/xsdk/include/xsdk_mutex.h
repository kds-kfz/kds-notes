#if !defined(__xsdk_mutex_h__)
#define __xsdk_mutex_h__

#if defined(OS_IS_WINDOWS)
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <windows.h>
#else
  #include <pthread.h>
  #include <semaphore.h>
  #include <sys/time.h>
  #include <errno.h>
  #include <fcntl.h>
#endif

#include <string>
#include <sstream>

#include "xsdk_define.h"

BGN_NAMESPACE_XSDK

class CMutex
{
public:
  CMutex(void);
  ~CMutex();

  int Create(const char *p_pszName = NULL);
  int Close(void);

  int Lock(unsigned int p_uiMilliseconds = INFINITE);
  int Unlock(void);

  int GetLastError(void) {return m_iLastError;}
  const char * GetLastErrorMsg(void) {return m_strLastErrorMsg.c_str();}  

protected:
  int SetLastError(const char *p_pszHostFunc, const char *p_pszCallFunc);

protected:
#if defined(OS_IS_WINDOWS)
  HANDLE m_hMutex;
#else
  bool m_bNamed;
  std::string m_strName;

  sem_t m_stSem;
  sem_t *m_pstSem;

  pthread_mutex_t m_stMutex;
  bool m_bInit;
#endif

  int m_iLastError;
  std::string m_strLastErrorMsg;
};

END_NAMESPACE_XSDK

#endif  // __xsdk_mutex_h__
