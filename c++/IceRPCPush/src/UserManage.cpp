// UserManage.cpp: implementation of the CUserManage class.
//
//////////////////////////////////////////////////////////////////////

#include "publicfunc.h"
#include "UserManage.h"
#include "AutoCS.h"
#include "Log.h"


extern	CUserManage	g_UserManage;

// 为新连接申请接收缓存和订阅表，保持 ST_USER_DATA 可被结构体复制。
void ST_USER_DATA::Init()
{
	pBuf = g_UserManage.AllocRcvBuf();//new char[MAX_CACHE_BUFLEN+10];
	//memset(pBuf,0,MAX_CACHE_BUFLEN+10);
	pMapKey = new std::map<DWORD,int>;
}
// 务必最后释放的时候release，不要依靠析构，因为中间临时变量太多，容易出问题；
// 其他时候，临时变量对拷就不用重新申请空间;
// 释放连接动态资源并清零状态，槽位随后可被新连接复用。
void ST_USER_DATA::ResetIt()
{
	if ( pBuf )
	{
		// 统一外部释放 delete [] pBuf;
		g_UserManage.FreeRcvBuf(pBuf);

		pBuf = NULL;
	}
	iHandleRef	= 0;
	dwReqGnid	= 0;
	dwAnsGnid	= 0;
	hHandle=(HCLIENT)-1;
	hServer=0;
	iRecvLen=0;
	tmLastActive=0;
	lRef=0;

	if ( pMapKey )
	{
		pMapKey->clear();
		delete pMapKey;
		pMapKey = NULL;
	}
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUserManage::CUserManage():m_clMemMng(MAX_REQ_BUFLEN+1024*1024)
{
	m_clMutex.Create();
}

CUserManage::~CUserManage()
{
	m_clMutex.Close();
}

// 初始化连接槽位数组，调用前应确保没有活跃连接。
void CUserManage::Init(int p_iMaxUser)
{
	CAutoCS clAutoLock(&m_clMutex);
	ST_USER_DATA ud;
	m_aUser.resize(p_iMaxUser,ud);
	m_iUserCount=0;
	m_iMaxUserCount=p_iMaxUser;
	m_mapFind.clear();
}


// 把新连接复制到空闲槽位，并建立句柄到槽位索引的查找表。
bool CUserManage::Add(ST_USER_DATA* p_pData)
{
    CAutoCS clAutoLock(&m_clMutex);
	if(m_iUserCount>=m_iMaxUserCount)
	{
		return false;
	}
    int i;
	for(i=0;i<m_iMaxUserCount;i++)
	{
		if(m_aUser[i].lRef==0)
		{
			p_pData->lRef=1;
			m_iUserCount++;
 			memcpy(&m_aUser[i],p_pData,sizeof(ST_USER_DATA));
			ST_HDATA_HCLIENT stKey = {p_pData->hServer,p_pData->hHandle};
			m_mapFind[ stKey ] = i;
			m_mapSubUser[stKey] = i;
			break;
		}
	}
	return true;
}

// 批量删除连接时先在锁内摘除，再在锁外关闭 socket 避免回调死锁。
int  CUserManage::DelAll()
{
	int	iNotDelete = 0;
	std::vector<HS> aHs;
	std::vector<HCLIENT> aHc;
	//避免死锁
	{
		CAutoCS clAutoLock(&m_clMutex);
		int i;
		for(i=0;i<m_aUser.size();i++)
		{
			if(m_aUser[i].lRef>0)
			{
				m_aUser[i].lRef--;
                if(m_aUser[i].lRef==0)
				{
					aHs.push_back(m_aUser[i].hServer);
					aHc.push_back(m_aUser[i].hHandle);
					m_iUserCount--;
					m_aUser[i].ResetIt();
				}
				else
				{
					++iNotDelete;
					CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "error not delete :%d\r\n",iNotDelete);
				}
			}
		}
	}
	int i;
	for(i=0;i<aHs.size();i++)
	{
		//s_cleardata(vhs[i],vhc[i]);
		s_close(aHs[i],aHc[i]);
	}
	m_mapFind.clear();
	m_mapSubUser.clear();
	return iNotDelete;
}

// 强制回收槽位资源，不等待引用归零，仅用于异常退出。
void CUserManage::ForceDelRes()
{
	//避免死锁
	{
		CAutoCS clAutoLock(&m_clMutex);
		int i;
		for(i=0;i<m_aUser.size();i++)
		{
			if(m_aUser[i].lRef>0)
			{
				m_aUser[i].lRef--;

				{
					m_iUserCount--;
					m_aUser[i].ResetIt();

				}
			}
		}
	}
}

// 查询连接并增加引用计数，调用方处理结束必须 ReleaseIt。
ST_USER_DATA * CUserManage::Query(HS p_hServer,HCLIENT p_hHandle)
{
	CAutoCS clAutoLock(&m_clMutex);
	ST_USER_DATA *p_pData=NULL;
 	int i;
	ST_HDATA_HCLIENT stKey = {p_hServer,p_hHandle};
	if ( m_mapFind.find(stKey) != m_mapFind.end() )
	{
		i = m_mapFind[stKey];
		if(m_aUser[i].lRef>0 && s_compare(m_aUser[i].hHandle,p_hHandle)==0)
		{
  			m_aUser[i].lRef++;
			p_pData=&m_aUser[i];
		}
	}
 	return p_pData;
}

// 释放 Query 引用，最后一个引用负责关闭连接和删除查找表。
void CUserManage::ReleaseIt(ST_USER_DATA * p_pData)
{
	if ( !p_pData )
		return;
	HS hServer=NULL;
	HCLIENT hClient=NULL;
	bool bNeedRelease=false;
	//避免死锁
	{
		CAutoCS clAutoLock(&m_clMutex);
 		p_pData->lRef--;
		if(p_pData->lRef==0)
		{
			hServer=p_pData->hServer;
			hClient=p_pData->hHandle;
			ST_HDATA_HCLIENT stKey = {p_pData->hServer,p_pData->hHandle};
			bNeedRelease=true;
			m_iUserCount--;
			p_pData->ResetIt();

			// 20091001
			// 重大问题，这个地方应该也要释放，否则下次可能找到，具体导致是del跟release顺序反了
			m_mapFind.erase(stKey);
			m_mapSubUser.erase(stKey);
		}
	}
	if(bNeedRelease)
	{
		//s_cleardata(hServer,hc);
		s_close(hServer,hClient);
	}
}
// 做特殊处理，不能多次del，因为del是不需要query的，容易直接把计数器多次消耗;
// 理论上del只要发出过一次，就可以了，其他地方release可以自动检测
// 网络断开通知的删除入口，只消耗一次连接引用避免重复关闭。
void CUserManage::Del(HS p_hServer,HCLIENT p_hHandle)
{
	bool bNeedRelease=false;
	ST_HDATA_HCLIENT stKey = {p_hServer,p_hHandle};
	//避免死锁
	{
		CAutoCS clAutoLock(&m_clMutex);
		int i = 0;
		if ( m_mapFind.find(stKey) != m_mapFind.end() )
		{
			i = m_mapFind[ stKey ];
			if(m_aUser[i].lRef>0 && s_compare(m_aUser[i].hHandle,p_hHandle)==0)
			{
				m_aUser[i].lRef--;
				if(m_aUser[i].lRef==0)
				{
					bNeedRelease=true;
					m_iUserCount--;
					m_aUser[i].ResetIt();
					
					m_mapFind.erase(stKey);
					m_mapSubUser.erase(stKey);
				}
			}
		}
	}
	if(bNeedRelease)
	{
		//s_cleardata(hServer,hc);
		s_close(p_hServer,p_hHandle);
	}
}

int CUserManage::GetCount()
{
	CAutoCS clAutoLock(&m_clMutex);
 	return m_iUserCount;
}

int CUserManage::GetFindIn()
{
	CAutoCS clAutoLock(&m_clMutex);
	return SafeSizeToLength<int>(m_mapFind.size());
}


// 从统一池申请接收缓存，减少高并发连接时的堆碎片。
char * CUserManage::AllocRcvBuf()
{
	CAutoCS clAutoLock(&m_clMutex);
	char * p_pBuf = (char*)m_clMemMng.Malloc();
	return p_pBuf;
}

void CUserManage::FreeRcvBuf(char * p_pBuf)
{
	CAutoCS clAutoLock(&m_clMutex);
	m_clMemMng.Free(p_pBuf);
}



// 拷贝当前订阅用户表，推送线程拿快照后可在锁外遍历。
void CUserManage::GetAllSubUser(std::map<ST_HDATA_HCLIENT,int> & p_refUsers)
{
	CAutoCS clAutoLock(&m_clMutex);

	p_refUsers = m_mapSubUser;
}

// 更新单连接订阅集合，调用方已完成协议解析。
void CUserManage::UpdateSubKey(ST_USER_DATA* p_pData,const std::map<DWORD,int> & p_refSubId)
{
	CAutoCS clAutoLock(&m_clMutex);
	if ( p_pData )
	{
		*p_pData->pMapKey = p_refSubId;
	}
}

void CUserManage::GetSubKey(ST_USER_DATA* p_pData,std::map<DWORD,int> & p_refSubId)
{
	CAutoCS clAutoLock(&m_clMutex);
	if ( p_pData )
	{
		p_refSubId = *p_pData->pMapKey;
	}
}

