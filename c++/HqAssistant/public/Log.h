#ifndef _HQLOG_H_
#define _HQLOG_H_

#include "Thread.h"

#include <string>
#include <vector>
//#include <mutex>
using namespace std;

USE_NAMESPACE_NSDK

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

class CLogFileThread;

class CBaseLog
{
public:
	CBaseLog();
	~CBaseLog();
	int ThreadLog();

	int InitLog(const char* p_szFold, enHQLogType p_eType = eInfo, const char* p_szLogFileName = "");
	int InitLog(int p_iLogDate = 0);
	void AddLog(enHQLogType p_eType, const char* p_szFormat, ...);
	string GetLogPath() { return m_strLogPath; }
	void SetLogLevel(char *p_strLogLevel);
	void Resume();
private:
	pthread_mutex_t m_mutexLog;
	std::vector<string> m_vecLog;

	CLogFileThread *m_pLogFileThread;
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

class CLogFileThread : public CThread
{
public:
	CLogFileThread(void) : m_pParent(nullptr) {};
	virtual ~CLogFileThread(void) { m_pParent = nullptr; }

	virtual int Work(void)
	{
		return (m_pParent != nullptr) ? m_pParent->ThreadLog() : MA_KO;
	}

	virtual bool AddNotify(CDataNotify* p_pNotify) { return false;  };
	virtual bool DelNotify(CDataNotify* p_pNotify) { return false; };
	virtual void SendNotify(unsigned long long p_lluThreadId, NotifyType p_enNotifyType, void *p_pData, int p_iDataLen) {};//通知异常数据位置

	void SetThis(CBaseLog * p_pParent)
	{
		m_pParent = p_pParent;
	}
private:
	CBaseLog *m_pParent;

};

class CLog : public CBaseLog
{
private:
	static CLog *m_pThis; // 通用日志单例，保留兼容旧接口
public:
	CLog() {}
	~CLog() {}
	static void Release();
	static CLog *GetInstance();
};
// wyl 2026-03-30：TCP 服务独立日志单例，避免与 Web 服务共享日志路径和生命周期。
class CTcpLog : public CBaseLog
{
private:
	static CTcpLog *m_pThis; // TCP 日志单例实例
public:
	CTcpLog() {}
	~CTcpLog() {}
	static void Release();
	static CTcpLog *GetInstance();
};

// wyl 2026-03-30：Web 服务独立日志单例，避免与 TCP 服务共享日志路径和生命周期。
class CWebLog : public CBaseLog
{
private:
	static CWebLog *m_pThis; // Web 日志单例实例
public:
	CWebLog() {}
	~CWebLog() {}
	static void Release();
	static CWebLog *GetInstance();
};

// wyl 2026-04-07：http 服务独立日志单例，避免与 TCP 服务共享日志路径和生命周期。
class CHttpLog : public CBaseLog
{
private:
	static CHttpLog* m_pThis; // Http 日志单例实例
public:
	CHttpLog() {}
	~CHttpLog() {}
	static void Release();
	static CHttpLog* GetInstance();
};

/* MT 服务 日志宏 */
#define MTLOG( log_type, log_fmt, ... ) \
do{ \
	CLog::GetInstance()->AddLog(log_type, "%s:%d [%s] " log_fmt, __FILE__, __LINE__, \
	eInfo == log_type ? "INFO" : eDebug == log_type ? "DEBUG" : eWarn == log_type ? \
	"WARN" : eError == log_type ? "ERROR" : "INFO", ##__VA_ARGS__); \
} while (0)

#define MT_INFO(log_fmt, ... ) \
do{ \
	CLog::GetInstance()->AddLog(eInfo, "[INFO] " log_fmt, ##__VA_ARGS__); \
} while (0)

#define MT_DEBUG(log_fmt, ... ) \
do{ \
	CLog::GetInstance()->AddLog(eDebug, "[DEBUG] %s:%d " log_fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
} while (0)

#define MT_WARN(log_fmt, ... ) \
do{ \
	CLog::GetInstance()->AddLog(eWarn, "[WARN] " log_fmt, ##__VA_ARGS__); \
} while (0)

#define MT_ERROR(log_fmt, ... ) \
do{ \
	CLog::GetInstance()->AddLog(eError, "[ERROR] " log_fmt, ##__VA_ARGS__); \
} while (0)
/* TCP 日志宏 */
#define TCP_HQLOG( log_type, log_fmt, ... ) \
do{ \
	CTcpLog::GetInstance()->AddLog(log_type, "%s:%d [%s] " log_fmt, __FILE__, __LINE__, \
	eInfo == log_type ? "INFO" : eDebug == log_type ? "DEBUG" : eWarn == log_type ? \
	"WARN" : eError == log_type ? "ERROR" : "INFO", ##__VA_ARGS__); \
} while (0)

#define TCP_INFO(log_fmt, ... ) \
do{ \
	CTcpLog::GetInstance()->AddLog(eInfo, "[INFO] [TCP] " log_fmt, ##__VA_ARGS__); \
} while (0)

#define TCP_DEBUG(log_fmt, ... ) \
do{ \
	CTcpLog::GetInstance()->AddLog(eDebug, "[DEBUG] [TCP] %s:%d " log_fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
} while (0)

#define TCP_WARN(log_fmt, ... ) \
do{ \
	CTcpLog::GetInstance()->AddLog(eWarn, "[WARN] [TCP] " log_fmt, ##__VA_ARGS__); \
} while (0)

#define TCP_ERROR(log_fmt, ... ) \
do{ \
	CTcpLog::GetInstance()->AddLog(eError, "[ERROR] [TCP] " log_fmt, ##__VA_ARGS__); \
} while (0)

/* WEB 日志宏 */
#define WEB_HQLOG( log_type, log_fmt, ... ) \
do{ \
	CWebLog::GetInstance()->AddLog(log_type, "%s:%d [%s] " log_fmt, __FILE__, __LINE__, \
	eInfo == log_type ? "INFO" : eDebug == log_type ? "DEBUG" : eWarn == log_type ? \
	"WARN" : eError == log_type ? "ERROR" : "INFO", ##__VA_ARGS__); \
} while (0)

#define WEB_INFO(log_fmt, ... ) \
do{ \
	CWebLog::GetInstance()->AddLog(eInfo, "[INFO] [WEB] " log_fmt, ##__VA_ARGS__); \
} while (0)

#define WEB_DEBUG(log_fmt, ... ) \
do{ \
	CWebLog::GetInstance()->AddLog(eDebug, "[DEBUG] [WEB] %s:%d " log_fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
} while (0)

#define WEB_WARN(log_fmt, ... ) \
do{ \
	CWebLog::GetInstance()->AddLog(eWarn, "[WARN] [WEB] " log_fmt, ##__VA_ARGS__); \
} while (0)

#define WEB_ERROR(log_fmt, ... ) \
do{ \
	CWebLog::GetInstance()->AddLog(eError, "[ERROR] [WEB] " log_fmt, ##__VA_ARGS__); \
} while (0)

/* HTTP 日志宏 */
#define HTTP_HQLOG( log_type, log_fmt, ... ) \
do{ \
	CHttpLog::GetInstance()->AddLog(log_type, "%s:%d [%s] " log_fmt, __FILE__, __LINE__, \
	eInfo == log_type ? "INFO" : eDebug == log_type ? "DEBUG" : eWarn == log_type ? \
	"WARN" : eError == log_type ? "ERROR" : "INFO", ##__VA_ARGS__); \
} while (0)

#define HTTP_INFO(log_fmt, ... ) \
do{ \
	CHttpLog::GetInstance()->AddLog(eInfo, "[INFO] [HTTP] " log_fmt, ##__VA_ARGS__); \
} while (0)

#define HTTP_DEBUG(log_fmt, ... ) \
do{ \
	CHttpLog::GetInstance()->AddLog(eDebug, "[DEBUG] [HTTP] %s:%d " log_fmt, __FILE__, __LINE__, ##__VA_ARGS__); \
} while (0)

#define HTTP_WARN(log_fmt, ... ) \
do{ \
	CHttpLog::GetInstance()->AddLog(eWarn, "[WARN] [HTTP] " log_fmt, ##__VA_ARGS__); \
} while (0)

#define HTTP_ERROR(log_fmt, ... ) \
do{ \
	CHttpLog::GetInstance()->AddLog(eError, "[ERROR] [HTTP] " log_fmt, ##__VA_ARGS__); \
} while (0)

#endif // _HQLOG_H_
