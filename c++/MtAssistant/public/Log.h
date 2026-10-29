#ifndef _HQLOG_H_
#define _HQLOG_H_

#include "Thread.h"

#include <mutex>
#include <string>
#include <vector>

#define MA_OK 0
#define MA_KO (-1)

#define HQTIME_BUF_SIZE (128)
#define HQLOG_BUF_SIZE (1024)
#define HQLOG_BUF_MAX (4)

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
	const std::string& GetLogPath() const { return m_strLogPath; }// wyl 2026-05-06：对外只暴露当前日志完整路径，按钮打开也复用该变量。
	void SetLogLevel(char* p_strLogLevel);
	void Resume();
private:
	std::mutex m_clLogMutex; // 保护待写日志队列；持锁期间不执行文件 I/O 或休眠。
	std::vector<std::string> m_vecLog;
	CLogFileThread* m_pLogFileThread;
	volatile enHQLogType m_enLogType;
	FILE* m_pFileLog;
	unsigned int m_uiLastDate;
	std::string m_strLogFold;
	std::string m_strLogName;
	std::string m_strLogPath;
	volatile bool m_bInitStatus;

	void Close();
	void Init();
	void BaseRelease();
};

class CLogFileThread : public nsdk::CThread
{
public:
	CLogFileThread(void) : m_pParent(nullptr) {}
	virtual ~CLogFileThread(void) { m_pParent = nullptr; }

	virtual int Work(void)
	{
		return (m_pParent != nullptr) ? m_pParent->ThreadLog() : MA_KO;
	}

	virtual bool AddNotify(nsdk::CDataNotify* p_pNotify) { return false; }
	virtual bool DelNotify(nsdk::CDataNotify* p_pNotify) { return false; }
	virtual void SendNotify(unsigned long long p_lluThreadId, nsdk::NotifyType p_enNotifyType, void* p_pData, int p_iDataLen) {}

	void SetThis(CBaseLog* p_pParent)
	{
		m_pParent = p_pParent;
	}
private:
	CBaseLog* m_pParent;
};

class CLog : public CBaseLog
{
private:
	static CLog* m_pThis;
public:
	CLog() {}
	~CLog() {}
	static void Release();
	static CLog* GetInstance();
};

#define MT_INFO(log_fmt, ... ) \
do{ \
	CLog::GetInstance()->AddLog(eInfo, "[INFO] " log_fmt, ##__VA_ARGS__); \
} while (0)

#define MT_WARN(log_fmt, ... ) \
do{ \
	CLog::GetInstance()->AddLog(eWarn, "[WARN] " log_fmt, ##__VA_ARGS__); \
} while (0)

#endif // _HQLOG_H_
