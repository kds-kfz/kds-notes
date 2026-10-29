#include "Log.h"
#include <cstdarg>
#include <chrono>
#include <ctime>
#include <thread>
#include "publicfunc.h"

CSocketServerLog *CSocketServerLog::m_pThis = nullptr;
CSocketServerTcpLog *CSocketServerTcpLog::m_pThis = nullptr;
CSocketServerWebLog *CSocketServerWebLog::m_pThis = nullptr;
CSocketServerHttpLog* CSocketServerHttpLog::m_pThis = nullptr;

void CSocketServerBaseLog::Init()
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
}

CSocketServerBaseLog::CSocketServerBaseLog()
{
	Init();
}

CSocketServerBaseLog::~CSocketServerBaseLog()
{
	BaseRelease();
}

void CSocketServerBaseLog::Close()
{
	if (m_pFileLog)
	{
		fclose(m_pFileLog);
		m_pFileLog = nullptr;
	}
}

int CSocketServerBaseLog::InitLog(int p_iLogDate)
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

	m_uiLastDate = 0 == p_iLogDate ? SocketServerCurrentDate() : p_iLogDate;

	char szLogFile[SOCKET_SERVER_MAX_PATH] = { 0 };
	snprintf(szLogFile, SOCKET_SERVER_MAX_PATH - 1U, "%s%s%s%08d.log", m_strLogFold.c_str(), PATH_DELIMETER, m_strLogName.c_str(), m_uiLastDate);
	m_strLogPath = szLogFile;
	m_pFileLog = fopen(szLogFile, "a+");

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
		m_pLogFileThread = new CSocketServerLogFileThread();
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

void CSocketServerBaseLog::BaseRelease()
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

	Close();
}

void CSocketServerBaseLog::Resume()
{
	if (m_pLogFileThread)
	{
		m_pLogFileThread->Resume();
	}
}

int CSocketServerBaseLog::InitLog(const char* p_szFold, enHQLogType p_eType, const char* p_szLogFileName)
{
	// 日志初始化
	if (nullptr == p_szFold)
	{
		return -1;
	}

	m_strLogFold = p_szFold;
	m_strLogName = p_szLogFileName;
	int iRet = 0;
	if (MA_OK != SocketServerFolderExists(p_szFold))
	{
		if ((iRet = SocketServerCreateFolder(p_szFold)) != MA_OK)
		{
			return iRet;
		}
	}

	m_enLogType = p_eType;

	iRet = InitLog();

	return iRet;
}

void CSocketServerBaseLog::SetLogLevel(char *p_strLogLevel)
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

int CSocketServerBaseLog::ThreadLog()
{
	if (!m_bInitStatus)
		return MA_KO;

	std::unique_lock<std::mutex> clLock(m_clLogMutex);
	if (m_vecLog.empty())
	{
		clLock.unlock();
		// 空队列时主动让出一个刷新周期，避免日志线程空转占满 CPU。
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		return MA_OK;
	}
	std::vector<std::string> vecCacheLog;
	vecCacheLog.swap(m_vecLog);
	m_vecLog.clear();
	clLock.unlock();

	//
	for (int i = 0; i < vecCacheLog.size(); i++)
	{
		fprintf(m_pFileLog, "%s", vecCacheLog[i].c_str());
		fprintf(m_pFileLog, "\n");
	}

	fflush(m_pFileLog);
	return MA_OK;
}

void CSocketServerBaseLog::AddLog(enHQLogType p_eType, const char* p_szFormat, ...)
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
	for (int iCount = 0; iCount < HQLOG_BUF_MAX; ++iCount)
	{
		// 每次重试重新初始化参数列表；Linux 的 va_list 不能重复消费。
		va_list valist;
		va_start(valist, p_szFormat);
		iRet = vsnprintf(&vecBuf[0], vecBuf.size(), p_szFormat, valist);
		va_end(valist);
		if (iRet > -1 && iRet < vecBuf.size()) //非负数，且小于 iRet
		{
			break;
		}
		if (iCount + 1 < HQLOG_BUF_MAX)
		{
			vecBuf.resize(vecBuf.size() * 2);
		}
	}
	if (iRet < 0)
	{
		return;
	}
	if (static_cast<size_t>(iRet) >= vecBuf.size())
	{
		// 超长日志只保留已格式化进缓冲区的部分，不能按返回的目标长度越界读取。
		iRet = static_cast<int>(vecBuf.size() - 1);
	}

	// 时间
	const std::chrono::system_clock::time_point tpNow = std::chrono::system_clock::now();
	const std::time_t ttNow = std::chrono::system_clock::to_time_t(tpNow);
	const std::tm stCurrTime = SocketServerLocalTime(ttNow);
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

	// 记录
	std::lock_guard<std::mutex> clLock(m_clLogMutex);
	m_vecLog.push_back(strLog);
}

CSocketServerLog *CSocketServerLog::GetInstance()
{
	if (nullptr == m_pThis)
	{
		m_pThis = new CSocketServerLog;
	}
	return m_pThis;
}

void CSocketServerLog::Release()
{
	if (nullptr == m_pThis)
		return;

	delete m_pThis;
	m_pThis = nullptr;
}

CSocketServerTcpLog *CSocketServerTcpLog::GetInstance()
{
	if (nullptr == m_pThis)
	{
		m_pThis = new CSocketServerTcpLog;
	}
	return m_pThis;
}

void CSocketServerTcpLog::Release()
{
	if (nullptr == m_pThis)
		return;

	delete m_pThis;
	m_pThis = nullptr;
}

CSocketServerWebLog *CSocketServerWebLog::GetInstance()
{
	if (nullptr == m_pThis)
	{
		m_pThis = new CSocketServerWebLog;
	}
	return m_pThis;
}

void CSocketServerWebLog::Release()
{
	if (nullptr == m_pThis)
		return;

	delete m_pThis;
	m_pThis = nullptr;
}

CSocketServerHttpLog *CSocketServerHttpLog::GetInstance()
{
	if (nullptr == m_pThis)
	{
		m_pThis = new CSocketServerHttpLog;
	}
	return m_pThis;
}

void CSocketServerHttpLog::Release()
{
	if (nullptr == m_pThis)
		return;

	delete m_pThis;
	m_pThis = nullptr;
}



