#include "Log.h"
#include <cstdio>
#include <cstdarg>
#include <chrono>
#include <ctime>
#include <thread>
#include "nsdk_atomic.h"
#include "nsdk.h"
#include "LogDlg.h"

CLog *CLog::m_pThis = nullptr;

void CBaseLog::Init()
{
	m_vecLog.clear();
	m_pLogFileThread = nullptr;
	m_enLogType = eInfo;
	m_pFileLog = nullptr;
	m_uiLastDate = 0;
	m_strLogFold = "";
	m_strLogName = "";
	m_strLogPath = "";
	m_bInitStatus = false;
	pthread_mutex_init(&m_mutexLog, nullptr);
}

CBaseLog::CBaseLog()
{
	Init();
}

CBaseLog::~CBaseLog()
{
	BaseRelease();
}

void CBaseLog::Close()
{
	if (m_pFileLog)
	{
		fclose(m_pFileLog);
		m_pFileLog = nullptr;
	}
}

int CBaseLog::InitLog(int p_iLogDate)
{
	int iRet = MA_OK;

	//禁止提交日志打印
	m_bInitStatus = false;

	//挂起
	if (nullptr != m_pLogFileThread)
	{
		iRet = m_pLogFileThread->Pause();
		if (MA_OK != iRet)
		{
			return -1;
		}
	}

	//关闭日志
	Close();

	m_uiLastDate = 0 == p_iLogDate ? nsdk::GetCurDate() : p_iLogDate;

	char szLogFile[NSDK_MAX_PATH] = { 0 };
	snprintf(szLogFile, NSDK_MAX_PATH - 1, "%s%s%s%08d.log", m_strLogFold.c_str(), NSDK_PATH_DELIMETER, m_strLogName.c_str(), m_uiLastDate);
	m_strLogPath = szLogFile;
	// wyl 2026-05-06：日志写入和按钮打开统一使用m_strLogPath这一份完整路径。
	m_pFileLog = fopen(m_strLogPath.c_str(), "a+");

	if (nullptr == m_pFileLog)
	{
		return -2;
	}

	if (nullptr != m_pLogFileThread)
	{
		iRet = m_pLogFileThread->Resume();
		if (MA_OK != iRet)
		{
			return -3;
		}
	}
	else
	{
		m_pLogFileThread = new CLogFileThread();
		if (nullptr == m_pLogFileThread)
		{
			return -4;
		}
		m_pLogFileThread->SetThis(this);
		iRet = m_pLogFileThread->Start(0);
		if (MA_OK != iRet)
		{
			delete m_pLogFileThread;
			m_pLogFileThread = nullptr;
			return -5;
		}
	}

	m_bInitStatus = true;

	return MA_OK;
}

void CBaseLog::BaseRelease()
{
	if (!m_bInitStatus)
		return;

	m_bInitStatus = false;
	if (m_pLogFileThread)
	{
		m_pLogFileThread->Stop();
		delete m_pLogFileThread;
		m_pLogFileThread = nullptr;
	}

	pthread_mutex_destroy(&m_mutexLog);
	Close();
}

void CBaseLog::Resume()
{
	if (m_pLogFileThread)
	{
		m_pLogFileThread->Resume();
	}
}

int CBaseLog::InitLog(const char* p_szFold, enHQLogType p_eType, const char* p_szLogFileName)
{
	// 日志初始化
	if (nullptr == p_szFold)
	{
		return -1;
	}

	m_strLogFold = p_szFold;
	m_strLogName = p_szLogFileName;
	int iRet = 0;
	if (MA_OK != FolderExists(p_szFold))
	{
		if ((iRet = CreateFolder(p_szFold)) != MA_OK)
		{
			return iRet;
		}
	}

	m_enLogType = p_eType;

	iRet = InitLog();

	return iRet;
}

void CBaseLog::SetLogLevel(char *p_strLogLevel)
{
	if (nullptr == p_strLogLevel)
		return;

	if (strcmp(p_strLogLevel, "debug") == 0)
	{
		m_enLogType = eDebug;
	}
	else
	{
		m_enLogType = eInfo;
	}
}

int CBaseLog::ThreadLog()
{
	if (!m_bInitStatus)
		return MA_KO;

	pthread_mutex_lock(&m_mutexLog);
	if (m_vecLog.empty())
	{
		pthread_mutex_unlock(&m_mutexLog);
		nsdk_Sleep(1000);
		return MA_OK;
	}
	std::vector<std::string> vecCacheLog;
	vecCacheLog.swap(m_vecLog);
	m_vecLog.clear();
	pthread_mutex_unlock(&m_mutexLog);

	//
	for (int i = 0; i < vecCacheLog.size(); i++)
	{
		fprintf(m_pFileLog, "%s", vecCacheLog[i].c_str());
		fprintf(m_pFileLog, "\n");
	}

	fflush(m_pFileLog);
	//m_vecLog.clear();
	//m_mutexLog.Unlock();

	return MA_OK;
}

void CBaseLog::AddLog(enHQLogType p_eType, const char* p_szFormat, ...)
{
	if (!m_bInitStatus)
		return;

	if (nullptr == p_szFormat)
	{
		return;
	}

	//日志类型拦截, 针对enDebug、enInfo 判断不考虑enWarn、enError
	//enInfo包含所有, enDebug包含enDebug、enWarn、enError
	//动态品种:enDebug，静态品种:enInfo, 轻微错误: enWarn, 致命错误:enError
	if (p_eType > eWarn && p_eType > m_enLogType)
		return;

	// 格式化
	vector<char> vecBuf(HQLOG_BUF_SIZE);
	int iRet = 0;
	int iCount = 0;

#if defined( OS_IS_WINDOWS )
	va_list valist;
	va_start(valist, p_szFormat);
#else
	va_list valist;
	va_start(valist, p_szFormat);
#endif
	for (; iCount < HQLOG_BUF_MAX; iCount++)
	{
		iRet = vsnprintf(&vecBuf[0], vecBuf.size(), p_szFormat, valist);
		if (iRet > -1 && iRet < vecBuf.size()) //非负数，且小于 iRet
		{
			break;
		}
		vecBuf.resize(vecBuf.size() * 2);
	}

#if defined( OS_IS_WINDOWS )
	va_end(valist);
#else
	va_end(valist);
#endif

	if (iRet == vecBuf.size())//超大内容 16k
	{
		vecBuf[iRet - 1] = '\0';
	}

	// 时间
	const std::chrono::system_clock::time_point tpNow = std::chrono::system_clock::now();
	const std::time_t ttNow = std::chrono::system_clock::to_time_t(tpNow);
	const std::tm stCurrTime = SafeLocalTime(ttNow);
	const int iMilliseconds = (int)(std::chrono::duration_cast<std::chrono::milliseconds>(
		tpNow.time_since_epoch()).count() % 1000);

	//校验是否隔日;
	unsigned int uiCurDate = (stCurrTime.tm_year + 1900) * 10000 + (stCurrTime.tm_mon + 1) * 100 + stCurrTime.tm_mday;
	if (uiCurDate > m_uiLastDate)
	{
		InitLog(uiCurDate);
	}

	char szTime[HQTIME_BUF_SIZE] = { 0 };
	snprintf(szTime, HQTIME_BUF_SIZE - 1, "[%04d%02d%02d %02d:%02d %02d:%03d] ",
		stCurrTime.tm_year + 1900, stCurrTime.tm_mon + 1, stCurrTime.tm_mday, stCurrTime.tm_hour,
		stCurrTime.tm_min, stCurrTime.tm_sec, iMilliseconds);

	// 内容
	string strLog;
	strLog = szTime;

	string strContent;
	if (iRet > 0)
	{
		// wyl 2026-03-30：仅拷贝本次真正格式化出的长度，避免把缓冲区尾部无效内容写进日志。
		strContent.assign(vecBuf.data(), iRet);
	}

	strLog += strContent;

	if (NULL != g_pLogDlg)
	{
		g_pLogDlg->OutInfo(szTime, strContent);
	}

	// 记录
	pthread_mutex_lock(&m_mutexLog);
	m_vecLog.push_back(strLog);
	pthread_mutex_unlock(&m_mutexLog);
}

CLog *CLog::GetInstance()
{
	if (nullptr == m_pThis)
	{
		m_pThis = new CLog;
	}
	return m_pThis;
}

void CLog::Release()
{
	if (nullptr == m_pThis)
		return;

	delete m_pThis;
	m_pThis = nullptr;
}
