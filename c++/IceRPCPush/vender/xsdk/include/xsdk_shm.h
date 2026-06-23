#if !defined(__xsdk_shm_h__)
#define __xsdk_shm_h__

#if defined(OS_IS_WINDOWS)
  #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <windows.h>
#else
  #include <sys/mman.h>
  #include <sys/stat.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <errno.h>
#endif  // defined(OS_IS_WINDOWS)

#include <string>
#include <sstream>

#include "xsdk_define.h"

BGN_NAMESPACE_XSDK

class CShm
{
public:
  enum EN_SHM_FLAG
  {
    SHM_CREAT = 0x0001,         // Specifies SHM_CREAT create a new shared memory segment or open if exists
    SHM_EXCL = 0x0002,          // used with SHM_CREAT to ensure failure if the shared memory segment already exists

    SHM_READ = 0x0100,
    SHM_WRITE = 0x0200,
    SHM_READWRITE = SHM_READ|SHM_WRITE
  };

  CShm(void);
  CShm(const char *p_pszShmName, size_t p_nShmSize, int p_iShmFlag = SHM_CREAT|SHM_READWRITE, bool p_bAutoClear = true);
  ~CShm();

  // If 'p_iShmFlag' specifies both SHM_CREAT and SHM_EXCL and a shared memory segment already exists for 'p_pszShmName',
  // then function fails with return code: XSDK_EXISTS
  int Create(const char *p_pszShmName, size_t p_nShmSize, int p_iShmFlag = SHM_CREAT|SHM_READWRITE, bool p_bAutoClear = true);
  int Close(void);

  operator void *() const {return m_pvdShmPtr;}
  operator char *() const {return (char *)m_pvdShmPtr;}
  operator const char *() const {return (const char *)m_pvdShmPtr;}

  int GetLastError(void) {return m_iLastError;}
  const char * GetLastErrorMsg(void) {return m_strLastErrorMsg.c_str();}  

protected:
  int SetLastError(const char *p_pszHostFunc, const char *p_pszCallFunc);

protected:
#if defined(OS_IS_WINDOWS)
  HANDLE m_hShm;
#else
  int m_hShm;
#endif
  std::string m_strShmName;
  void *m_pvdShmPtr;
  size_t m_nShmSize;

  bool m_bAutoClear;
  int m_iLastError;
  std::string m_strLastErrorMsg;
};

END_NAMESPACE_XSDK

#endif  // __xsdk_shm_h__
