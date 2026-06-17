// Log.cpp: IceRPCPush 日志基础类和业务日志类实现。
//////////////////////////////////////////////////////////////////////

#include "publicfunc.h"
#include "Log.h"
#include "XmlConfig.h"

#include "nsdk.h"
#include "nsdk_atomic.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <sstream>
#include <vector>

namespace nsdk
{

// pthread 线程入口，转发到 CThread::Run。
static void* IceRPCPushThreadHandle(void* p_pParam)
{
	CThread* p_pThis = static_cast<CThread*>(p_pParam);
	if (p_pThis != NULL)
	{
		p_pThis->Run();
	}
	return reinterpret_cast<void*>(1);
}

CThread::CThread()
	: m_bFinishFlag(false)
	, m_uThreadId(0)
	, m_pThread(NULL)
	, m_mutex()
	, m_condition()
	, m_bPauseFlag(true)
	, m_bStopFlag(false)
	, m_enState(Stoped)
{
}

CThread::~CThread()
{
	Stop();
}

CThread::ThreadState CThread::State() const
{
	return m_enState;
}

int CThread::Start(unsigned int p_uId)
{
	if (m_pThread != NULL)
	{
		return MA_KO;
	}

	if (pthread_mutex_init(&m_mutex, NULL) != 0)
	{
		return MA_KO;
	}
	if (pthread_cond_init(&m_condition, NULL) != 0)
	{
		pthread_mutex_destroy(&m_mutex);
		return MA_KO;
	}

	m_pThread = new pthread_t;
	if (m_pThread == NULL)
	{
		pthread_cond_destroy(&m_condition);
		pthread_mutex_destroy(&m_mutex);
		return MA_KO;
	}

	if (pthread_create(m_pThread, NULL, IceRPCPushThreadHandle, this) != 0)
	{
		delete m_pThread;
		m_pThread = NULL;
		pthread_cond_destroy(&m_condition);
		pthread_mutex_destroy(&m_mutex);
		return MA_KO;
	}

	m_bPauseFlag = true;
	m_bStopFlag = false;
	m_enState = Paused;
	m_bFinishFlag = false;
	m_uThreadId = p_uId;
	return MA_OK;
}

int CThread::Stop()
{
	if (m_pThread == NULL)
	{
		return MA_KO;
	}

	m_bPauseFlag = false;
	m_bStopFlag = true;
	pthread_cond_signal(&m_condition);
	pthread_join(*m_pThread, NULL);
	delete m_pThread;
	m_pThread = NULL;
	m_enState = Stoped;

	pthread_mutex_destroy(&m_mutex);
	pthread_cond_destroy(&m_condition);
	return MA_OK;
}

int CThread::Pause()
{
	if (m_pThread == NULL)
	{
		return MA_KO;
	}

	m_bPauseFlag = true;
	m_enState = Paused;
	return MA_OK;
}

int CThread::Resume()
{
	if (m_pThread == NULL)
	{
		return MA_KO;
	}

	m_bPauseFlag = false;
	pthread_cond_signal(&m_condition);
	m_enState = Running;
	return MA_OK;
}

bool CThread::GetFinish() const
{
	return m_bFinishFlag;
}

unsigned long long CThread::GetThreadId() const
{
	return m_uThreadId;
}

void CThread::Run()
{
	while (!m_bStopFlag)
	{
		if (m_bPauseFlag)
		{
			pthread_mutex_lock(&m_mutex);
			while (m_bPauseFlag && !m_bStopFlag)
			{
				pthread_cond_wait(&m_condition, &m_mutex);
			}
			pthread_mutex_unlock(&m_mutex);
		}

		if (m_bStopFlag)
		{
			break;
		}

		Work();
	}

	m_bFinishFlag = true;
	m_bPauseFlag = false;
	m_bStopFlag = false;
}

unsigned long long CThread::ThreadId2uLong(unsigned long p_ulThreadId)
{
	std::ostringstream clStream;
	clStream << p_ulThreadId;
	return std::stoull(clStream.str());
}

} // namespace nsdk

CBaseLog::CBaseLog()
{
	Init();
}

CBaseLog::~CBaseLog()
{
	BaseRelease();
}

void CBaseLog::Init()
{
	m_vecLog.clear();
	m_pLogFileThread = NULL;
	m_enLogType = eInfo;
	m_pFileLog = NULL;
	m_uLastDate = 0;
	m_strLogFold = "";
	m_strLogName = "";
	m_strLogPath = "";
	m_bInitStatus = false;
	pthread_mutex_init(&m_mutexLog, NULL);
}

void CBaseLog::Close()
{
	if (m_pFileLog != NULL)
	{
		fclose(m_pFileLog);
		m_pFileLog = NULL;
	}
}

int CBaseLog::InitLog(int p_iLogDate)
{
	int iRet = MA_OK;

	// 重新打开文件期间暂停提交日志，避免后台线程写入已关闭句柄。
	m_bInitStatus = false;
	if (m_pLogFileThread != NULL)
	{
		iRet = m_pLogFileThread->Pause();
		if (iRet != MA_OK)
		{
			return MA_KO;
		}
	}

	Close();
	m_uLastDate = (p_iLogDate == 0) ? nsdk::GetCurDate() : static_cast<unsigned int>(p_iLogDate);

	char szLogFile[NSDK_MAX_PATH] = {0};
	snprintf(szLogFile, sizeof(szLogFile), "%s%s%s%08d.log", m_strLogFold.c_str(), PATH_DELIMETER, m_strLogName.c_str(), m_uLastDate);
	m_strLogPath = szLogFile;
	m_pFileLog = fopen(szLogFile, "a+");
	if (m_pFileLog == NULL)
	{
		return -2;
	}

	if (m_pLogFileThread != NULL)
	{
		iRet = m_pLogFileThread->Resume();
		if (iRet != MA_OK)
		{
			return -3;
		}
	}
	else
	{
		m_pLogFileThread = new CLogFileThread();
		if (m_pLogFileThread == NULL)
		{
			return -4;
		}
		m_pLogFileThread->SetThis(this);
		iRet = m_pLogFileThread->Start(0);
		if (iRet != MA_OK)
		{
			delete m_pLogFileThread;
			m_pLogFileThread = NULL;
			return -5;
		}
	}

	m_bInitStatus = true;
	return MA_OK;
}

void CBaseLog::BaseRelease()
{
	m_bInitStatus = false;
	if (m_pLogFileThread != NULL)
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
	if (m_pLogFileThread != NULL)
	{
		m_pLogFileThread->Resume();
	}
}

int CBaseLog::InitLog(const char* p_szFold, enHQLogType p_enType, const char* p_szLogFileName)
{
	if (p_szFold == NULL)
	{
		return MA_KO;
	}

	m_strLogFold = p_szFold;
	m_strLogName = (p_szLogFileName == NULL) ? "" : p_szLogFileName;
	if (nsdk::FolderExists(p_szFold) != MA_OK)
	{
		int iRet = nsdk::CreateFolder(p_szFold);
		if (iRet != MA_OK)
		{
			return iRet;
		}
	}

	m_enLogType = p_enType;
	return InitLog();
}

std::string CBaseLog::GetLogPath() const
{
	return m_strLogPath;
}

void CBaseLog::SetLogLevel(const char* p_szLogLevel)
{
	if (p_szLogLevel == NULL)
	{
		return;
	}

	if (strcmp(p_szLogLevel, "debug") == 0)
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
	{
		return MA_KO;
	}

	pthread_mutex_lock(&m_mutexLog);
	if (m_vecLog.empty())
	{
		pthread_mutex_unlock(&m_mutexLog);
		nsdk_Sleep(1000);
		return MA_OK;
	}

	std::vector<std::string> vecCacheLog;
	vecCacheLog.swap(m_vecLog);
	pthread_mutex_unlock(&m_mutexLog);

	for (size_t uIndex = 0; uIndex < vecCacheLog.size(); ++uIndex)
	{
		fprintf(m_pFileLog, "%s", vecCacheLog[uIndex].c_str());
		fprintf(m_pFileLog, "\n");
	}
	fflush(m_pFileLog);
	return MA_OK;
}

void CBaseLog::AddLog(enHQLogType p_enType, const char* p_szFormat, ...)
{
	if (!m_bInitStatus || p_szFormat == NULL)
	{
		return;
	}

	// warn/error 始终写入；info/debug 按配置级别过滤。
	if (p_enType > eWarn && p_enType > m_enLogType)
	{
		return;
	}

	std::vector<char> vecBuf(HQLOG_BUF_SIZE, 0);
	int iRet = -1;

	va_list stArgs;
	va_start(stArgs, p_szFormat);
	for (int iCount = 0; iCount < HQLOG_BUF_MAX; ++iCount)
	{
		va_list stArgsCopy;
		va_copy(stArgsCopy, stArgs);
		iRet = vsnprintf(&vecBuf[0], vecBuf.size(), p_szFormat, stArgsCopy);
		va_end(stArgsCopy);

		if (iRet > -1 && iRet < static_cast<int>(vecBuf.size()))
		{
			break;
		}
		if (iRet > -1)
		{
			vecBuf.resize(static_cast<size_t>(iRet) + 1, 0);
		}
		else
		{
			vecBuf.resize(vecBuf.size() * 2, 0);
		}
	}
	va_end(stArgs);

	if (iRet < 0)
	{
		return;
	}
	if (iRet >= static_cast<int>(vecBuf.size()))
	{
		iRet = static_cast<int>(vecBuf.size()) - 1;
		vecBuf[iRet] = '\0';
	}

	const std::chrono::system_clock::time_point stNowPoint = std::chrono::system_clock::now();
	const std::time_t ttNow = std::chrono::system_clock::to_time_t(stNowPoint);
	const std::tm stCurrTime = nsdk::SafeLocalTime(ttNow);
	const int iMilliseconds = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(stNowPoint.time_since_epoch()).count() % 1000);

	unsigned int uCurDate = static_cast<unsigned int>((stCurrTime.tm_year + 1900) * 10000 + (stCurrTime.tm_mon + 1) * 100 + stCurrTime.tm_mday);
	if (uCurDate > m_uLastDate)
	{
		InitLog(static_cast<int>(uCurDate));
	}

	char szTime[HQTIME_BUF_SIZE] = {0};
	snprintf(szTime, sizeof(szTime), "[%04d%02d%02d %02d:%02d %02d:%03d] ",
		stCurrTime.tm_year + 1900, stCurrTime.tm_mon + 1, stCurrTime.tm_mday, stCurrTime.tm_hour,
		stCurrTime.tm_min, stCurrTime.tm_sec, iMilliseconds);

	std::string strLog = szTime;
	if (iRet > 0)
	{
		strLog.append(vecBuf.data(), static_cast<size_t>(iRet));
	}

	pthread_mutex_lock(&m_mutexLog);
	m_vecLog.push_back(strLog);
	pthread_mutex_unlock(&m_mutexLog);
}

CLogFileThread::CLogFileThread()
{
	m_pParent = NULL;
}

CLogFileThread::~CLogFileThread()
{
	m_pParent = NULL;
}

int CLogFileThread::Work()
{
	return (m_pParent != NULL) ? m_pParent->ThreadLog() : MA_KO;
}

bool CLogFileThread::AddNotify(nsdk::CDataNotify* p_pNotify)
{
	return false;
}

bool CLogFileThread::DelNotify(nsdk::CDataNotify* p_pNotify)
{
	return false;
}

void CLogFileThread::SendNotify(unsigned long long p_ullThreadId, nsdk::NotifyType p_enNotifyType, void* p_pData, int p_iDataLen)
{
}

void CLogFileThread::SetThis(CBaseLog* p_pParent)
{
	m_pParent = p_pParent;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// 将 IceRPCPush 配置级别转换成 SocketServer 日志基类识别的级别。
static enHQLogType ConvertLogLevel(EN_LOG_LEVEL p_enLevel)
{
	switch (p_enLevel)
	{
	case EN_LOG_LEVEL_ERROR:
		return eError;
	case EN_LOG_LEVEL_WARN:
		return eWarn;
	case EN_LOG_LEVEL_DEBUG:
		return eDebug;
	case EN_LOG_LEVEL_INFO:
	case EN_LOG_LEVEL_OFF:
	default:
		return eInfo;
	}
}

// 兼容旧代码里误用的 %m_hSocket 占位符，统一替换成标准 %s。
static std::string NormalizeLogFormat(const char* p_szFormat)
{
	if (p_szFormat == NULL)
	{
		return "";
	}

	std::string strFormat = p_szFormat;
	std::string::size_type uPos = 0;
	while ((uPos = strFormat.find("%m_hSocket", uPos)) != std::string::npos)
	{
		strFormat.replace(uPos, strlen("%m_hSocket"), "%s");
		uPos += 2;
	}
	return strFormat;
}

// 生成带级别和来源的日志前缀，方便替代旧调试输出后定位来源。
static const char* GetLevelName(EN_LOG_LEVEL p_enLevel)
{
	switch (p_enLevel)
	{
	case EN_LOG_LEVEL_ERROR:
		return "ERROR";
	case EN_LOG_LEVEL_WARN:
		return "WARN";
	case EN_LOG_LEVEL_DEBUG:
		return "DEBUG";
	case EN_LOG_LEVEL_INFO:
		return "INFO";
	case EN_LOG_LEVEL_OFF:
	default:
		return "OFF";
	}
}

CIceRPCPushLog::CIceRPCPushLog()
{
	m_bOpened = false;
	m_enLevel = EN_LOG_LEVEL_INFO;
}

CIceRPCPushLog::~CIceRPCPushLog()
{
	CloseLog();
}

// 返回日志单例，兼容 IceRPCPush 进程级日志生命周期。
CIceRPCPushLog& CIceRPCPushLog::Instance()
{
	static CIceRPCPushLog clLog;
	return clLog;
}

// 使用本地复制的 SocketServer CBaseLog 初始化 Day_Logs 目录和日志文件前缀。
bool CIceRPCPushLog::Open(const char* p_szLogName)
{
	if (p_szLogName == NULL || p_szLogName[0] == '\0')
	{
		p_szLogName = "JSONRPC";
	}

	m_strName = p_szLogName;
	m_bOpened = ReopenBaseLog();
	if (m_bOpened)
	{
		WriteLog("启动LOG", "", "**************************** Level=%s", GetLevelName(m_enLevel));
	}
	return m_bOpened;
}

// 从配置中读取日志级别，XML 和旧 INI 都使用 ICEPUSH/LogLevel。
void CIceRPCPushLog::ApplyConfig(const char* p_szCfgFile, const ST_XML_CONFIG_DATA* p_pConfig)
{
	std::string strLevel = GetConfigString(p_pConfig, p_szCfgFile, "ICEPUSH", "LogLevel", "");
	if (!strLevel.empty())
	{
		SetLevelName(strLevel.c_str());
		return;
	}

	// DebugLog 是旧式布尔开关，仅用于打开 DEBUG 日志。
	std::string strDebug = GetConfigString(p_pConfig, p_szCfgFile, "ICEPUSH", "DebugLog", "");
	if (_stricmp(strDebug.c_str(), "1") == 0 || _stricmp(strDebug.c_str(), "true") == 0 || _stricmp(strDebug.c_str(), "debug") == 0)
	{
		SetLevel(EN_LOG_LEVEL_DEBUG);
	}
}

void CIceRPCPushLog::SetLevel(EN_LOG_LEVEL p_enLevel)
{
	m_enLevel = p_enLevel;
	if (m_bOpened)
	{
		m_bOpened = ReopenBaseLog();
	}
}

void CIceRPCPushLog::SetLevelName(const char* p_szLevel)
{
	SetLevel(ParseLevel(p_szLevel, m_enLevel));
}

EN_LOG_LEVEL CIceRPCPushLog::GetLevel() const
{
	return m_enLevel;
}

// 兼容旧 WriteLog 三段式格式，底层调用 CBaseLog::AddLog。
bool CIceRPCPushLog::WriteLog(const char* p_szKhh, const char* p_szAction, const char* p_szFormat, ...)
{
	va_list stArgs;
	va_start(stArgs, p_szFormat);
	bool bRet = WriteByLevel(EN_LOG_LEVEL_INFO, p_szKhh, p_szAction, p_szFormat, stArgs);
	va_end(stArgs);
	return bRet;
}

// 旧调试输出统一落到 DEBUG 日志，是否写入由 LogLevel 控制。
bool CIceRPCPushLog::WriteDebug(const char* p_szKhh, const char* p_szAction, const char* p_szFormat, ...)
{
	va_list stArgs;
	va_start(stArgs, p_szFormat);
	bool bRet = WriteByLevel(EN_LOG_LEVEL_DEBUG, p_szKhh, p_szAction, p_szFormat, stArgs);
	va_end(stArgs);
	return bRet;
}

bool CIceRPCPushLog::WriteByLevel(EN_LOG_LEVEL p_enLevel, const char* p_szKhh, const char* p_szAction, const char* p_szFormat, va_list p_stArgs)
{
	if (!m_bOpened || p_szFormat == NULL || !CanWrite(p_enLevel))
	{
		return false;
	}

	std::string strFormat = NormalizeLogFormat(p_szFormat);
	char szBuf[4096] = {0};
	vsnprintf(szBuf, sizeof(szBuf), strFormat.c_str(), p_stArgs);
	szBuf[sizeof(szBuf) - 1] = '\0';

	if (p_szKhh == NULL)
	{
		p_szKhh = "";
	}
	if (p_szAction == NULL)
	{
		p_szAction = "";
	}

	AddLog(ConvertLogLevel(p_enLevel), "[%s] %s:%s_%s", GetLevelName(p_enLevel), p_szKhh, p_szAction, szBuf);
	return true;
}

EN_LOG_LEVEL CIceRPCPushLog::ParseLevel(const char* p_szLevel, EN_LOG_LEVEL p_enDefault) const
{
	if (p_szLevel == NULL || p_szLevel[0] == '\0')
	{
		return p_enDefault;
	}

	std::string strLevel = p_szLevel;
	std::transform(strLevel.begin(), strLevel.end(), strLevel.begin(), [](unsigned char p_uValue) { return static_cast<char>(std::tolower(p_uValue)); });
	if (strLevel == "debug" || strLevel == "4" || strLevel == "true")
	{
		return EN_LOG_LEVEL_DEBUG;
	}
	if (strLevel == "info" || strLevel == "3")
	{
		return EN_LOG_LEVEL_INFO;
	}
	if (strLevel == "warn" || strLevel == "warning" || strLevel == "2")
	{
		return EN_LOG_LEVEL_WARN;
	}
	if (strLevel == "error" || strLevel == "1")
	{
		return EN_LOG_LEVEL_ERROR;
	}
	if (strLevel == "off" || strLevel == "none" || strLevel == "0" || strLevel == "false")
	{
		return EN_LOG_LEVEL_OFF;
	}
	return p_enDefault;
}

bool CIceRPCPushLog::CanWrite(EN_LOG_LEVEL p_enLevel) const
{
	if (m_enLevel == EN_LOG_LEVEL_OFF)
	{
		return false;
	}
	if (p_enLevel <= EN_LOG_LEVEL_WARN)
	{
		return true;
	}
	return p_enLevel <= m_enLevel;
}

bool CIceRPCPushLog::ReopenBaseLog()
{
	std::string strPath(::GetRootPath());
	strPath.append("\\Day_Logs");
	int iRet = InitLog(strPath.c_str(), ConvertLogLevel(m_enLevel), m_strName.c_str());
	if (iRet == MA_OK)
	{
		Resume();
		return true;
	}
	return false;
}

// 关闭日志写入，CBaseLog 析构会完成线程和文件句柄释放。
void CIceRPCPushLog::CloseLog()
{
	m_bOpened = false;
}
