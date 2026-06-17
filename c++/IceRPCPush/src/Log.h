// Log.h: IceRPCPush 日志基础类和业务日志类。
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GATHERLOG_H__F9C4FB28_8DBA_4290_8383_02AB51DB467C__INCLUDED_)
#define AFX_GATHERLOG_H__F9C4FB28_8DBA_4290_8383_02AB51DB467C__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "nsdk_define.h"

#include <pthread.h>
#include <cstdio>
#include <string>
#include <vector>

struct ST_XML_CONFIG_DATA;

#ifndef PATH_DELIMETER
#define PATH_DELIMETER "\\"
#endif

#ifndef MA_OK
#define MA_OK 0
#endif

#ifndef MA_KO
#define MA_KO (-1)
#endif

#define HQTIME_BUF_SIZE	(128)
#define HQLOG_BUF_SIZE	(1024)
#define HQLOG_BUF_MAX	(4)

BGN_NAMESPACE_NSDK

// 线程通知类型，保留 SocketServer 日志线程的通知接口形态。
enum NotifyType
{
	NT_FINISH = 1,	// 线程任务完成通知。
	NT_COLLECT		// 线程采集数据通知。
};

// 线程数据通知接口，当前日志线程不主动派发通知，仅保留基类契约。
class CDataNotify
{
public:
	// 线程事件回调入口，派生类按需处理线程 ID、通知类型和附带数据。
	virtual void OnDataNotify(unsigned long long p_ullThreadId, NotifyType p_enType, void* p_pData, int p_iDataLen) = 0;
};

// SocketServer 日志基类依赖的轻量线程封装，本工程直接复制实现，避免包含外部 src 源码。
class CThread
{
public:
	CThread();
	virtual ~CThread();

	// 线程运行状态，和 SocketServer 原实现保持同一语义。
	enum ThreadState
	{
		Stoped,		// 已停止或尚未启动。
		Running,	// 正在运行。
		Paused		// 已暂停。
	};

	// 返回当前线程状态。
	ThreadState State() const;
	// 线程入口循环，由 pthread 回调触发。
	void Run();
	// 创建线程，p_uId 用于上层标识业务线程。
	int Start(unsigned int p_uId);
	// 停止线程并等待线程退出。
	int Stop();
	// 暂停线程循环。
	int Pause();
	// 恢复线程循环。
	int Resume();
	// 返回线程是否已经结束。
	bool GetFinish() const;
	// 返回业务线程 ID。
	unsigned long long GetThreadId() const;

protected:
	// 派生类实际工作函数。
	virtual int Work() = 0;
	// 注册通知接收者。
	virtual bool AddNotify(CDataNotify* p_pNotify) = 0;
	// 删除通知接收者。
	virtual bool DelNotify(CDataNotify* p_pNotify) = 0;
	// 发送线程通知。
	virtual void SendNotify(unsigned long long p_ullThreadId, NotifyType p_enNotifyType, void* p_pData, int p_iDataLen) = 0;

	// 线程完成标识。
	bool m_bFinishFlag;
	// 业务线程编号。
	unsigned int m_uThreadId;

private:
	// 将平台线程 ID 转成无符号长整型，保留 SocketServer 线程类辅助能力。
	unsigned long long ThreadId2uLong(unsigned long p_ulThreadId);

private:
	// pthread 线程对象，Start 创建，Stop 释放。
	pthread_t* m_pThread;
	// 暂停/恢复条件变量对应的互斥量。
	pthread_mutex_t m_mutex;
	// 暂停/恢复条件变量。
	pthread_cond_t m_condition;
	// 暂停标识，为 true 时线程等待条件变量。
	bool m_bPauseFlag;
	// 停止标识，为 true 时线程退出循环。
	bool m_bStopFlag;
	// 当前线程状态。
	ThreadState m_enState;
};

END_NAMESPACE_NSDK

// 日志类型，错误/警告/运行/调试。
enum enHQLogType
{
	eError = 0,
	eWarn,
	eInfo,
	eDebug
};

class CLogFileThread;

// SocketServer 文件日志基类的本地副本，负责日志目录、日志文件和异步刷盘。
class CBaseLog
{
public:
	CBaseLog();
	~CBaseLog();

	// 日志线程工作函数，批量写入缓存中的日志文本。
	int ThreadLog();
	// 初始化日志目录、日志级别和文件名前缀。
	int InitLog(const char* p_szFold, enHQLogType p_enType = eInfo, const char* p_szLogFileName = "");
	// 按指定日期重新打开日志文件，日期为 0 时使用当天日期。
	int InitLog(int p_iLogDate = 0);
	// 追加一条格式化日志，实际写文件由后台线程完成。
	void AddLog(enHQLogType p_enType, const char* p_szFormat, ...);
	// 返回当前日志文件完整路径。
	std::string GetLogPath() const;
	// 按字符串设置日志级别，当前兼容 debug 和 info。
	void SetLogLevel(const char* p_szLogLevel);
	// 恢复日志线程写入。
	void Resume();

private:
	// 关闭当前日志文件句柄。
	void Close();
	// 初始化成员默认值。
	void Init();
	// 释放日志线程、互斥量和文件句柄。
	void BaseRelease();

private:
	// 日志缓存互斥量，保护 m_vecLog。
	pthread_mutex_t m_mutexLog;
	// 待写入文件的日志缓存。
	std::vector<std::string> m_vecLog;
	// 后台写文件线程。
	CLogFileThread* m_pLogFileThread;
	// 当前允许写入的日志级别。
	volatile enHQLogType m_enLogType;
	// 当前日志文件句柄。
	FILE* m_pFileLog;
	// 当前日志文件日期，格式 YYYYMMDD。
	unsigned int m_uLastDate;
	// 日志目录。
	std::string m_strLogFold;
	// 日志文件名前缀。
	std::string m_strLogName;
	// 当前日志文件完整路径。
	std::string m_strLogPath;
	// 日志是否完成初始化，未初始化时禁止写入。
	volatile bool m_bInitStatus;
};

// 日志刷盘线程，调用 CBaseLog::ThreadLog 完成异步写文件。
class CLogFileThread : public nsdk::CThread
{
public:
	CLogFileThread();
	virtual ~CLogFileThread();

	// 执行一次日志刷盘。
	virtual int Work();
	// 日志线程不需要外部通知接收者。
	virtual bool AddNotify(nsdk::CDataNotify* p_pNotify);
	// 日志线程不需要外部通知接收者。
	virtual bool DelNotify(nsdk::CDataNotify* p_pNotify);
	// 日志线程不派发通知，保留接口以满足基类契约。
	virtual void SendNotify(unsigned long long p_ullThreadId, nsdk::NotifyType p_enNotifyType, void* p_pData, int p_iDataLen);
	// 绑定所属日志基类对象，线程工作时回调它写文件。
	void SetThis(CBaseLog* p_pParent);

private:
	// 所属日志基类对象，生命周期由 CBaseLog 控制。
	CBaseLog* m_pParent;
};

// IceRPCPush 日志级别，和 SocketServer 日志级别保持同一语义。
enum EN_LOG_LEVEL
{
	EN_LOG_LEVEL_ERROR = 0,
	EN_LOG_LEVEL_WARN,
	EN_LOG_LEVEL_INFO,
	EN_LOG_LEVEL_DEBUG,
	EN_LOG_LEVEL_OFF
};

// IceRPCPush 专用日志类，继承 SocketServer 日志基类并保留业务侧三段式写日志接口。
class CIceRPCPushLog : public CBaseLog
{
public:
	// 返回进程级日志对象，生命周期随 DLL 进程保持。
	static CIceRPCPushLog& Instance();
public:
	// 初始化日志目录和文件名前缀，p_szLogName 为空时使用默认 JSONRPC。
	bool Open(const char* p_szLogName);
	// 从 XML 或旧 INI 配置读取日志级别，默认 info；支持 debug/info/warn/error/off。
	void ApplyConfig(const char* p_szCfgFile, const ST_XML_CONFIG_DATA* p_pConfig);
	// 手工设置日志级别，Open 前后均可调用。
	void SetLevel(EN_LOG_LEVEL p_enLevel);
	// 按字符串设置日志级别，非法值保持当前级别不变。
	void SetLevelName(const char* p_szLevel);
	// 返回当前日志级别，便于调试配置是否生效。
	EN_LOG_LEVEL GetLevel() const;
	// 追加一条 INFO 级格式化日志，保留旧 IceRPCPush 三段式调用语义。
	bool WriteLog(const char* p_szKhh, const char* p_szAction, const char* p_szFormat, ...);
	// 写 DEBUG 级别日志，替代旧调试输出，是否写入由配置级别控制。
	bool WriteDebug(const char* p_szKhh, const char* p_szAction, const char* p_szFormat, ...);
	// 关闭日志写入，通常仅在 DLL 退出或测试清理时调用。
	void CloseLog();
private:
	CIceRPCPushLog();
	virtual ~CIceRPCPushLog();

	// 统一格式化并写入 SocketServer 日志，内部会修正旧 %m_hSocket 占位符。
	bool WriteByLevel(EN_LOG_LEVEL p_enLevel, const char* p_szKhh, const char* p_szAction, const char* p_szFormat, va_list p_stArgs);
	// 将字符串配置解析为日志级别，非法值回退 p_enDefault。
	EN_LOG_LEVEL ParseLevel(const char* p_szLevel, EN_LOG_LEVEL p_enDefault) const;
	// 判断当前配置是否允许写入指定级别。
	bool CanWrite(EN_LOG_LEVEL p_enLevel) const;
	// 重新应用目录、文件名和日志级别配置。
	bool ReopenBaseLog();

	// 日志是否已经初始化，避免未 Open 时反复写失败。
	bool m_bOpened;
	// 当前日志级别，默认 info；debug 级别才记录调试日志。
	EN_LOG_LEVEL m_enLevel;
	// 旧接口传入的日志名前缀，重开日志时传给 SocketServer 日志基类。
	std::string m_strName;
};

#endif // !defined(AFX_GATHERLOG_H__F9C4FB28_8DBA_4290_8383_02AB51DB467C__INCLUDED_)
