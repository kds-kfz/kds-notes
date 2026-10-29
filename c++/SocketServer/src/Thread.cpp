#include "Thread.h"
#include <sstream>
#include <system_error>
#include <utility>

using namespace nsdk;

using namespace std;

CThread::CThread():
	m_bFinishFlag(false),
	m_ulThreadId(0),
	m_clThread(),
	m_clStateMutex(),
	m_clStateCondition(),
	m_bJoinInProgress(false),
	m_bPauseFlag(true),
	m_bStopFlag(false),
	m_enState(Stoped)
{
}

CThread::~CThread()
{
	Stop();
}

CThread::ThreadState CThread::State() const
{
	std::lock_guard<std::mutex> clLock(m_clStateMutex);
	return m_enState;
}

bool CThread::GetFinish() const
{
	std::lock_guard<std::mutex> clLock(m_clStateMutex);
	return m_bFinishFlag;
}

unsigned long long CThread::GetThreadId() const
{
	std::lock_guard<std::mutex> clLock(m_clStateMutex);
	return m_ulThreadId;
}

int CThread::Start(unsigned int p_uiId)
{
	std::lock_guard<std::mutex> clLock(m_clStateMutex);
	if (m_clThread.joinable() || m_bJoinInProgress)
	{
		return -1;
	}

	m_bPauseFlag = true;
	m_bStopFlag = false;
	m_enState = Paused;
	m_bFinishFlag = false;
	m_ulThreadId = p_uiId;
	try
	{
		m_clThread = std::thread(&CThread::Run, this);
	}
	catch (const std::system_error&)
	{
		m_enState = Stoped;
		m_bFinishFlag = true;
		return -1;
	}
	return 0;
}

int CThread::Stop()
{
	std::thread clThread;
	{
		std::lock_guard<std::mutex> clLock(m_clStateMutex);
		if (!m_clThread.joinable() ||
			m_clThread.get_id() == std::this_thread::get_id())
		{
			return -1;
		}
		m_bPauseFlag = false;
		m_bStopFlag = true;
		m_bJoinInProgress = true;
		m_clStateCondition.notify_all();
		clThread = std::move(m_clThread);
	}

	clThread.join();
	{
		std::lock_guard<std::mutex> clLock(m_clStateMutex);
		m_bStopFlag = false;
		m_bJoinInProgress = false;
		m_enState = Stoped;
	}
	return 0;
}

int CThread::Pause()
{
	std::lock_guard<std::mutex> clLock(m_clStateMutex);
	if (!m_clThread.joinable() || m_bJoinInProgress)
	{
		return -1;
	}
	m_bPauseFlag = true;
	m_enState = Paused;
	return 0;
}

int CThread::Resume()
{
	std::lock_guard<std::mutex> clLock(m_clStateMutex);
	if (!m_clThread.joinable() || m_bJoinInProgress)
	{
		return -1;
	}
	m_bPauseFlag = false;
	m_enState = Running;
	m_clStateCondition.notify_one();
	return 0;
}

void CThread::Run()
{
	for (;;)
	{
		{
			std::unique_lock<std::mutex> clLock(m_clStateMutex);
			m_clStateCondition.wait(clLock, [this]()
			{
				return !m_bPauseFlag || m_bStopFlag;
			});
			if (m_bStopFlag)
			{
				break;
			}
		}
		Work();
	}

	std::lock_guard<std::mutex> clLock(m_clStateMutex);
	m_bFinishFlag = true;
	m_bPauseFlag = false;
	m_enState = Stoped;
}

unsigned long long CThread::ThreadId2uLong(unsigned long p_ulThreadId)
{
	std::ostringstream oss;
	oss << p_ulThreadId;
	std::string stid = oss.str();
	return std::stoull(stid);
}

