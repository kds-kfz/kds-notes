#if !defined(KDSC_NET_QUEUE_MNG_H)
#define KDSC_NET_QUEUE_MNG_H

#include "UserData.h"
#include "nsdk_event.h"
#include "nsdk_thread.h"
#include "nsdk_queue.h"
#include "xsdk_thread.h"

class CNetQueueThread;

struct NetQueueThreadInfo 
{
	int iThreadNo;//线程号,旧逻辑 用于异步应答唤醒业务处理;
	CNetQueueThread *pcNetQueue;
	xsdk::CQueue<NetRequsetDat *> aSynAns;//应答节点;
	xsdk::CQueue<NetRequsetDat *> aDeleteNode;//已处理完成,删除节点;
	NetQueueThreadInfo():pcNetQueue(nullptr){}
};

//请求队列管理;
class CNetQueueMng
{
public:
	static CNetQueueMng* GetInstance();
	static void  Release();
	static nsdk_atomic_t64 m_ullReqId;
public:
	CNetQueueMng(void);
	~CNetQueueMng(void);

	bool Init();
	void Uninit();
	int	ProcessIt(int NetSourceType, const char * ip, unsigned short port, USERDATA * puserdata,const char * packdata,int realdatalen);
	unsigned long long DelUserReq(HS p_refServerHandle, HCLIENT p_refClinetHandle, const char *p_szIp, unsigned short p_usPort);
	int	ProcessAsynAns(NetRequsetDat *pNode);
	int	HandleThread(short p_iThreadNo);
	void DoBusiness(NetRequsetDat * r);
public:
	//非携程处理;
	void BusinessNotCoro(NetRequsetDat *p_stNode);
	//多线程异步业务处理;
	int	HandleAnswer(int p_iThreadNo);
	int	DeleteCoroNode(int p_iThreadNo);
	void DeleteNetNode(NetRequsetDat* p_pNode);
protected:
	int	ProcessAsynError(NetRequsetDat *p_pNode);
	void FreeMemPool(AnsMemType p_enMemType, char *p_pBuf);
	char *MallocMemPool(short p_nGNID, AnsMemType &p_refMemType,long &p_lMaxAlloclen);

	static CNetQueueMng	*m_pThis;
	std::vector<NetQueueThreadInfo>	m_vecHandleThread;
	xsdk::CQueue<NetRequsetDat *> m_aRequest;

	BOOL m_Stop;
	xsdk::CEvent m_clEventMiddle;// 请求通知时间，更及时的处理
	nsdk_atomic_t64 m_llUnComplete;	// 累计未返回包;
};

//请求队列线程,业务回调;
class CNetQueueThread : public xsdk::CBaseThread
{
public:
	CNetQueueThread(void) : m_pParent(nullptr) {};
	virtual ~CNetQueueThread(void) { m_pParent = nullptr; }

	virtual int Work(void)
	{
		return (m_pParent != nullptr) ? m_pParent->HandleThread(m_iThreadNo) : -1;
	}

	void SetThis(CNetQueueMng *p_pParent, int p_iThreadNo)
	{
		m_pParent = p_pParent;
		m_iThreadNo = p_iThreadNo;
	}

private:
	CNetQueueMng *m_pParent;
	int m_iThreadNo;
};

#endif