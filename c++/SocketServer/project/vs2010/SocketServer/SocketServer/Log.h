#ifndef _HQLOG_H_
#define _HQLOG_H_

#include "Thread.h"

#include <string>
#include <vector>

using namespace std;

#ifdef OS_IS_WINDOWS
#define PATH_DELIMETER "\\"
#elif defined(__GNUC__)
#define PATH_DELIMETER "/"
#else
#error compiler not supported!
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

class CLogFileThread;

class CBaseLog
{
public:
	CBaseLog();
	~CBaseLog();
	int ThreadLog();

	int InitLog(const char* p_szFold, enHQLogType p_eType = eInfo, const char* p_szLogFileName = "");
	int InitLog();
	void AddLog(enHQLogType p_eType, const char* p_szFormat, ...);
	string GetLogPath() { return m_strLogPath; }
	void SetLogLevel(char *p_strLogLevel);
	void Resume();
private:
	pthread_mutex_t m_mutexLog;
	std::vector<string> m_vecLog;
	std::vector<string> m_vecCacheLog;

	CLogFileThread *m_pLogFileThread;
	volatile enHQLogType m_enLogType;//日志级别
	FILE *m_pFileLog;
	int m_iLastDate;
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
	CLogFileThread(void) : m_pParent(NULL) {};
	virtual ~CLogFileThread(void) { m_pParent = NULL; }

	virtual int Work(void)
	{
		return (m_pParent != NULL) ? m_pParent->ThreadLog() : MA_KO;
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
	static CLog *m_pThis;
public:
	CLog() {}
	~CLog() {}
	static void Release();
	static CLog *GetInstance();
};

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

#define NSDK_DEBUG(log_fmt, ... ) \
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

#endif // _HQLOG_H_
