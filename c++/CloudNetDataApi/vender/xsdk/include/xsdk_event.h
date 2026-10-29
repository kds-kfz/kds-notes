#if !defined(__xsdk_event_h__)
#define __xsdk_event_h__

#if defined(OS_IS_WINDOWS)
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <windows.h>
#else
  #include <semaphore.h>
  #include <sys/time.h>
  #include <errno.h>
  #include <fcntl.h>
#endif

#include <string>
#include <sstream>

#include "xsdk_define.h"

BGN_NAMESPACE_XSDK

class CEvent
{
public:
  CEvent(void);
  ~CEvent();

  int Create(bool p_bManualReset = false, bool p_bInitialState = false, const char *p_pszName = NULL);
  int Close(void);

  int SetEvent(void);
  int ResetEvent(void);
  int WaitEvent(unsigned int p_uiMilliseconds = INFINITE);

  int GetLastError(void) {return m_iLastError;}
  const char * GetLastErrorMsg(void) {return m_strLastErrorMsg.c_str();}  

protected:
  int SetLastError(const char *p_pszHostFunc, const char *p_pszCallFunc);

protected:
#if defined(OS_IS_WINDOWS)
  HANDLE m_hEvent;
#else
  bool m_bNamed;
  std::string m_strName;
  
  sem_t m_stSem;
  sem_t *m_pstSem;

  bool m_bManualReset;
#endif

  int m_iLastError;
  std::string m_strLastErrorMsg;
};

END_NAMESPACE_XSDK

#endif  // __xsdk_event_h__
