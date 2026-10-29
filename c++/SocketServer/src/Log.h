#ifndef _HQLOG_H_
#define _HQLOG_H_

#include "Thread.h"

#include <mutex>
#include <string>
#include <vector>
using namespace std;

using namespace nsdk;

#if defined(_WIN32)
#define PATH_DELIMETER "\\"
#else
#define PATH_DELIMETER "/"
#endif
#define MA_OK 0
#define MA_KO (-1)

#define HQTIME_BUF_SIZE	(128)
#define HQLOG_BUF_SIZE	(1024)
#define HQLOG_BUF_MAX	(4)		//1024 * 2^4 = 16K

// 日志类型，错误/警告/运行/调试
enum enHQLogType
{
	eError = 0,
	eWarn,
	eInfo,
	eDebug
};

class CSocketServerLogFileThread;

class CSocketServerBaseLog
{
public:
	CSocketServerBaseLog();
	~CSocketServerBaseLog();
	int ThreadLog();

	int InitLog(const char* p_szFold, enHQLogType p_eType = eInfo, const char* p_szLogFileName = "");
	int InitLog(int p_iLogDate = 0);
	void AddLog(enHQLogType p_eType, const char* p_szFormat, ...);
	string GetLogPath() { return m_strLogPath; }
	void SetLogLevel(char *p_strLogLevel);
	void Resume();
private:
	std::mutex m_clLogMutex; // 仅保护进程内日志缓存；Linux 标准库仍通过 pthread 实现。
	std::vector<string> m_vecLog;

	CSocketServerLogFileThread *m_pLogFileThread;
	volatile enHQLogType m_enLogType;//日志级别
	FILE *m_pFileLog;
	unsigned int m_uiLastDate;//日期
	string m_strLogFold;
	string m_strLogName;
	string m_strLogPath;
	volatile bool m_bInitStatus;//初始化状态

private:
	void Close();
	void Init();
	void BaseRelease();
};

class CSocketServerLogFileThread : public CThread
{
public:
	CSocketServerLogFileThread(void) : m_pParent(nullptr) {};
	virtual ~CSocketServerLogFileThread(void) { m_pParent = nullptr; }

	virtual int Work(void)
	{
		return (m_pParent != nullptr) ? m_pParent->ThreadLog() : MA_KO;
	}

	virtual bool AddNotify(CDataNotify* p_pNotify) { return false;  };
	virtual bool DelNotify(CDataNotify* p_pNotify) { return false; };
	virtual void SendNotify(unsigned long long p_lluThreadId, NotifyType p_enNotifyType, void *p_pData, int p_iDataLen) {};//通知异常数据位置

	void SetThis(CSocketServerBaseLog * p_pParent)
	{
		m_pParent = p_pParent;
	}
private:
	CSocketServerBaseLog *m_pParent;

};

// SocketServer 的日志实现属于库内私有类型。类名必须携带库前缀，避免静态链接到宿主
// EXE 时与宿主工程常见的 CLog/CHttpLog/CWebLog/CTcpLog 产生全局 C++ 符号冲突。
class CSocketServerLog : public CSocketServerBaseLog
{
private:
	static CSocketServerLog *m_pThis; // 通用日志单例，仅在 SocketServer 内部使用
public:
	CSocketServerLog() {}
	~CSocketServerLog() {}
	static void Release();
	static CSocketServerLog *GetInstance();
};

// wyl 2026-03-30：TCP 服务独立日志单例，避免与 Web 服务共享日志路径和生命周期。
class CSocketServerTcpLog : public CSocketServerBaseLog
{
private:
	static CSocketServerTcpLog *m_pThis; // TCP 日志单例实例
public:
	CSocketServerTcpLog() {}
	~CSocketServerTcpLog() {}
	static void Release();
	static CSocketServerTcpLog *GetInstance();
};

// wyl 2026-03-30：Web 服务独立日志单例，避免与 TCP 服务共享日志路径和生命周期。
class CSocketServerWebLog : public CSocketServerBaseLog
{
private:
	static CSocketServerWebLog *m_pThis; // Web 日志单例实例
public:
	CSocketServerWebLog() {}
	~CSocketServerWebLog() {}
	static void Release();
	static CSocketServerWebLog *GetInstance();
};

// wyl 2026-04-07：http 服务独立日志单例，避免与 TCP 服务共享日志路径和生命周期。
class CSocketServerHttpLog : public CSocketServerBaseLog
{
private:
	static CSocketServerHttpLog* m_pThis; // Http 日志单例实例
public:
	CSocketServerHttpLog() {}
	~CSocketServerHttpLog() {}
	static void Release();
	static CSocketServerHttpLog* GetInstance();
};
/*
//日志宏
#define HQLOG( log_type, log_fmt, ... ) \
do{ \
	CLog::GetInstance()->AddLog(log_type, "%s:%d [%s] " log_fmt, __FILE__, __LINE__, \
	eInfo == log_type ? "INFO" : eDebug == log_type ? "DEBUG" : eWarn == log_type ? \
	"WARN" : eError == log_type ? "ERROR" : "INFO", ##__VA_ARGS__); \
} while (0)

#define INFO(log_fmt, ... ) \
do{ \
	CLog::GetInstance()->AddLog(eInfo, "[INFO] " log_fmt, ##__VA_ARGS__); \
} while (0)

#define DEBUG(log_fmt, ... ) \
do{ \
	CLog::GetInstance()->AddLog(eDebug, "[DEBUG] %s:%d " log_fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
} while (0)

#define WARN(log_fmt, ... ) \
do{ \
	CLog::GetInstance()->AddLog(eWarn, "[WARN] " log_fmt, ##__VA_ARGS__); \
} while (0)

#define ERROR(log_fmt, ... ) \
do{ \
	CLog::GetInstance()->AddLog(eError, "[ERROR] " log_fmt, ##__VA_ARGS__); \
} while (0)

#define TCP_HQLOG( log_type, log_fmt, ... ) \
do{ \
	CTcpLog::GetInstance()->AddLog(log_type, "%s:%d [%s] " log_fmt, __FILE__, __LINE__, \
	eInfo == log_type ? "INFO" : eDebug == log_type ? "DEBUG" : eWarn == log_type ? \
	"WARN" : eError == log_type ? "ERROR" : "INFO", ##__VA_ARGS__); \
} while (0)
*/

/* TCP 日志宏 */
#define TCP_INFO(log_fmt, ... ) \
do{ \
	CSocketServerTcpLog::GetInstance()->AddLog(eInfo, "[INFO] [TCP] " log_fmt, ##__VA_ARGS__); \
} while (0)

#define TCP_DEBUG(log_fmt, ... ) \
do{ \
	CSocketServerTcpLog::GetInstance()->AddLog(eDebug, "[DEBUG] [TCP] %s:%d " log_fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
} while (0)

#define TCP_WARN(log_fmt, ... ) \
do{ \
	CSocketServerTcpLog::GetInstance()->AddLog(eWarn, "[WARN] [TCP] " log_fmt, ##__VA_ARGS__); \
} while (0)

#define TCP_ERROR(log_fmt, ... ) \
do{ \
	CSocketServerTcpLog::GetInstance()->AddLog(eError, "[ERROR] [TCP] " log_fmt, ##__VA_ARGS__); \
} while (0)

/* WEB 日志宏 */
#define WEB_HQLOG( log_type, log_fmt, ... ) \
do{ \
	CSocketServerWebLog::GetInstance()->AddLog(log_type, "%s:%d [%s] " log_fmt, __FILE__, __LINE__, \
	eInfo == log_type ? "INFO" : eDebug == log_type ? "DEBUG" : eWarn == log_type ? \
	"WARN" : eError == log_type ? "ERROR" : "INFO", ##__VA_ARGS__); \
} while (0)

#define WEB_INFO(log_fmt, ... ) \
do{ \
	CSocketServerWebLog::GetInstance()->AddLog(eInfo, "[INFO] [WEB] " log_fmt, ##__VA_ARGS__); \
} while (0)

#define WEB_DEBUG(log_fmt, ... ) \
do{ \
	CSocketServerWebLog::GetInstance()->AddLog(eDebug, "[DEBUG] [WEB] %s:%d " log_fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
} while (0)

#define WEB_WARN(log_fmt, ... ) \
do{ \
	CSocketServerWebLog::GetInstance()->AddLog(eWarn, "[WARN] [WEB] " log_fmt, ##__VA_ARGS__); \
} while (0)

#define WEB_ERROR(log_fmt, ... ) \
do{ \
	CSocketServerWebLog::GetInstance()->AddLog(eError, "[ERROR] [WEB] " log_fmt, ##__VA_ARGS__); \
} while (0)

/* HTTP 日志宏 */
#define HTTP_HQLOG( log_type, log_fmt, ... ) \
do{ \
	CSocketServerHttpLog::GetInstance()->AddLog(log_type, "%s:%d [%s] " log_fmt, __FILE__, __LINE__, \
	eInfo == log_type ? "INFO" : eDebug == log_type ? "DEBUG" : eWarn == log_type ? \
	"WARN" : eError == log_type ? "ERROR" : "INFO", ##__VA_ARGS__); \
} while (0)

#define HTTP_INFO(log_fmt, ... ) \
do{ \
	CSocketServerHttpLog::GetInstance()->AddLog(eInfo, "[INFO] [HTTP] " log_fmt, ##__VA_ARGS__); \
} while (0)

#define HTTP_DEBUG(log_fmt, ... ) \
do{ \
	CSocketServerHttpLog::GetInstance()->AddLog(eDebug, "[DEBUG] [HTTP] %s:%d " log_fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
} while (0)

#define HTTP_WARN(log_fmt, ... ) \
do{ \
	CSocketServerHttpLog::GetInstance()->AddLog(eWarn, "[WARN] [HTTP] " log_fmt, ##__VA_ARGS__); \
} while (0)

#define HTTP_ERROR(log_fmt, ... ) \
do{ \
	CSocketServerHttpLog::GetInstance()->AddLog(eError, "[ERROR] [HTTP] " log_fmt, ##__VA_ARGS__); \
} while (0)

#endif // _HQLOG_H_


