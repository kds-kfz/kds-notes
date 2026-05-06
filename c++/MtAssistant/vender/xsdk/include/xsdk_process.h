#if !defined(__xsdk_process_h__)
#define __xsdk_process_h__

#if defined(OS_IS_WINDOWS)
  #include <windows.h>
#else
  #include <unistd.h>
  #include <sys/types.h>
  #include <stdio.h>
  #include <signal.h>
  #include <sys/stat.h>   
  #include <fcntl.h>
  #include <dirent.h> 
#endif

#include <string>
#include <map>
#include "xsdk_define.h"
#include "xsdk_thread.h"

BGN_NAMESPACE_XSDK

class CProcess
{
public:
  CProcess(void);
  virtual ~CProcess(void) {Kill();}

public:

  //------------------------------------------------------------------------------
  // 功能描述：
  //     创建子进程
  // 参数说明：
  //     p_pszAppName[in]   运行子进程程序的全路径
  //     p_pszCmdLine[in]   运行子进程程序的参数列表，以空格分开
  // 返回说明：
  //     XSDK_OK 函数执行成功
  //     XSDK_KO 函数执行失败
  int Create(const char *p_pszAppName, const char *p_pszCmdLine);

  //------------------------------------------------------------------------------
  // 功能描述：
  //     关闭子进程
  // 返回说明：
  //     XSDK_OK 函数执行成功
  //     XSDK_KO 函数执行失败
  int Close(void);

  //------------------------------------------------------------------------------
  // 功能描述：
  //     杀死子进程
  // 返回说明：
  //     XSDK_OK 函数执行成功
  //     XSDK_KO 函数执行失败
  int Kill(void);

  //------------------------------------------------------------------------------
  // 功能描述：
  //     判别子进程是否运行
  // 返回说明：
  //     true 正在运行
  //     false 未运行
  bool IsRunning(void);

  //------------------------------------------------------------------------------
  // 功能描述：
  //     获取子进程ID
  // 返回说明：
  //     <=0  子进程为启动
  //     其它 子进程ID
  unsigned int GetPID(void) const;

  //------------------------------------------------------------------------------
  // 功能描述：
  //     为当前子进程增加进程守护
  // 返回说明：
  //     XSDK_OK 函数执行成功
  //     XSDK_KO 函数执行失败
  int Guard(void);

  //------------------------------------------------------------------------------
  // 功能描述：
  //     取消当前进程的守护功能
  // 返回说明：
  //     XSDK_OK 函数执行成功
  //     XSDK_KO 函数执行失败
  int Ungurd(void);

  //------------------------------------------------------------------------------
  // 功能描述：
  //     判别当前进程是否被守护
  // 返回说明：
  //     true  已守护
  //     false 未守护
  bool IsGuarded(void) {return m_bIsGuard;}

  //------------------------------------------------------------------------------
  // 功能描述：
  //     获取最后错误代码
  // 返回说明：
  //     XSDK_OK 函数执行成功
  //     XSDK_KO 函数执行失败
  int GetLastErrorCode(void) {return m_iLastErrorCode;}

  //------------------------------------------------------------------------------
  // 功能描述：
  //     获取最后错误信息
  // 返回说明：
  //     返回错误信息
  const char * GetLastErrorMsg(void) {return m_strLastErrorMsg.c_str();}

protected:
#if defined(OS_IS_WINDOWS)
  PROCESS_INFORMATION m_stProcInfo;
#else
  pid_t m_uiPid;
#endif

  bool m_bIsGuard;
  int m_iLastErrorCode;
  std::string m_strAppName;
  std::string m_strCmdLine;
  std::string m_strLastErrorMsg;
public:
  class CGuardThread : public CBaseThread
  {
  public:
    CGuardThread(void){}
    virtual ~CGuardThread(void) {}

    virtual int Work(void);
  };

  struct ST_GUARD_PROCESS
  {
    unsigned int iDumpCount;
    std::string strAppName;
    std::string strCmdLine;
    CProcess   *pclProcess;
  };

  static CGuardThread                            *m_pclGuardThread;     // 进程存活的监视线程
  static std::map<unsigned int, ST_GUARD_PROCESS> m_mapGuardedProcess;  // 被监视的进程列表
};

END_NAMESPACE_XSDK

#endif  // __xsdk_process_h__
