#if !defined(__xsdk_thread_h__)
#define __xsdk_thread_h__

#if defined(OS_IS_WINDOWS)
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <windows.h>
#else
  #include <pthread.h>
  #include <errno.h>
#endif

#include "xsdk_define.h"
#include "xsdk_event.h"

BGN_NAMESPACE_XSDK

#if defined(OS_IS_WINDOWS)
  unsigned __stdcall ThreadProxy(void *p_pvdParam);
  typedef unsigned (__stdcall *PFN_THREAD_FUNC)(void *);
#else
  void * ThreadProxy(void *p_pvdParam);
  typedef void * (*PFN_THREAD_FUNC)(void *);
#endif

class CBaseThread
{
public:
  CBaseThread(void);
  virtual ~CBaseThread();

public:
  virtual int CreateThread(void *p_pvdParam = NULL);
  virtual int DeleteThread(void);

  virtual int Initialize(void) {return XSDK_OK;}
  virtual int Uninitialize(void) {return XSDK_OK;}

  virtual int Run(void);
  virtual int Work(void) = 0;

  virtual int OnPause(void) {return XSDK_OK;}
  virtual int OnResume(void) {return XSDK_OK;}
  virtual int Pause(void);
  virtual int Resume(void);

  virtual bool IsRunning(void);
  virtual unsigned int GetThreadID(void) const {return m_uiThreadID;}

protected:
  #if defined(OS_IS_WINDOWS)
    HANDLE m_hThread;
  #else
    pthread_t m_hThread;
  #endif
  unsigned int m_uiThreadID;

  CEvent m_clEventIdle;
  CEvent m_clEventWork;
  CEvent m_clEventKill;
  CEvent m_clEventDead;

  void *m_pvdParam;
};

END_NAMESPACE_XSDK

#endif  // __xsdk_thread_h__
