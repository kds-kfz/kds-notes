#include "Log.h"
#include "publicfunc.h"

#if defined( OS_IS_WINDOWS )
#include <windows.h>
#else
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#endif

CLog *CLog::m_pThis = nullptr;

void CBaseLog::Init()
{
	m_vecLog.clear();
	m_vecCacheLog.clear();
	m_pLogFileThread = NULL;
	m_enLogType = eInfo;
	m_pFileLog = NULL;
	m_iLastDate = 0;
	m_strLogFold = "";
	m_strLogName = "";
	m_strLogPath = "";
	m_bInitStatus = false;
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
		m_pFileLog = NULL;
	}
}

int CBaseLog::InitLog()
{
	int iRet = MA_OK;

	//禁止提交日志打印
	m_bInitStatus = false;

	//挂起
	if (NULL != m_pLogFileThread)
	{
		iRet = m_pLogFileThread->Pause();
		if (MA_OK != iRet)
		{
			return -1;
		}
	}

	//关闭日志
	Close();

	m_iLastDate = GetCurDate();

	//
	char szLogFile[NSDK_MAX_PATH] = { 0 };
	snprintf(szLogFile, NSDK_MAX_PATH - 1, "%s%s%s%08d.log", m_strLogFold.c_str(), PATH_DELIMETER, m_strLogName.c_str(), m_iLastDate);
	m_strLogPath = szLogFile;
	printf("%s\n", szLogFile);
	m_pFileLog = fopen(szLogFile, "a+");

	if (NULL == m_pFileLog)
	{
		return -2;
	}

	if (NULL != m_pLogFileThread)
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
		if (NULL == m_pLogFileThread)
		{
			return -4;
		}
		m_pLogFileThread->SetThis(this);
		iRet = m_pLogFileThread->Start(0);
		if (MA_OK != iRet)
		{
			delete m_pLogFileThread;
			m_pLogFileThread = NULL;
			return -5;
		}
	}

	pthread_mutex_init(&m_mutexLog, nullptr);

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
		m_pLogFileThread = NULL;
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
	if (NULL == p_szFold)
	{
		return -1;
	}

	m_strLogFold = p_szFold;
	m_strLogName = p_szLogFileName;

#if defined( OS_IS_WINDOWS )

#else
	RegularPath(m_strLogFold);
#endif
	int iRet = 0;
	if (MA_OK != FolderExists(m_strLogFold.c_str()))
	{
		if ((iRet = CreateFolder(m_strLogFold.c_str())) != MA_OK)
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
	if (NULL == p_strLogLevel)
		return;

	if (strcmp(p_strLogLevel, (char *)"debug") == 0)
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
#if defined( OS_IS_WINDOWS )
		Sleep(1000);
#else
		sleep(1);
#endif
		return MA_OK;
	}
	m_vecCacheLog = m_vecLog;
	m_vecLog.clear();
	pthread_mutex_unlock(&m_mutexLog);

	//TODO 校验是否隔日

	//
	for (int i = 0; i < m_vecCacheLog.size(); i++)
	{
		fprintf(m_pFileLog, m_vecCacheLog[i].c_str());
		fprintf(m_pFileLog, "\n");
	}

	fflush(m_pFileLog);
	//m_vecLog.clear();
	//m_mutexLog.Unlock();
	m_vecCacheLog.clear();

	return MA_OK;
}

void CBaseLog::AddLog(enHQLogType p_eType, const char* p_szFormat, ...)
{
	if (!m_bInitStatus)
		return;

	if (NULL == p_szFormat)
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

	char szTime[HQTIME_BUF_SIZE] = { 0 };

	GetCurDateTime(szTime, HQTIME_BUF_SIZE);

	// 内容
	string strLog = szTime;

	string strContent;
	strContent.assign(vecBuf.begin(), vecBuf.end());

	strLog += strContent;

	// 记录
	pthread_mutex_lock(&m_mutexLog);
	m_vecLog.push_back(strLog);
	pthread_mutex_unlock(&m_mutexLog);
}

CLog *CLog::GetInstance()
{
	if (NULL == m_pThis)
	{
		m_pThis = new CLog;
	}
	return m_pThis;
}

void CLog::Release()
{
	if (NULL == m_pThis)
		return;

	delete m_pThis;
	m_pThis = NULL;
}

