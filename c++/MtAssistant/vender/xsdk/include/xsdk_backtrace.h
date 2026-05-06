#if !defined(__xsdk_backtrace_h__)
#define __xsdk_backtrace_h__

#if defined(OS_IS_WINDOWS)
  #include <windows.h>
  #include "xsdk_atomic.h"
#endif
#include "xsdk_define.h"

BGN_NAMESPACE_XSDK

class CBackTrace
{
public:
  CBackTrace(void);
  ~CBackTrace();

  static const char * CallStack(
    char *p_pszCallStack,
    int p_iCallStackSize,
    int p_iCallStackLvl,
    const char *p_pszTopSymName = NULL);

private:
  static bool Initialize(void);
  static bool Uninitialize(void);

#if defined(OS_IS_WINDOWS)
private:
  static xsdk_atomic_t m_utReference;
#endif
};

END_NAMESPACE_XSDK

#endif  // __xsdk_backtrace_h__