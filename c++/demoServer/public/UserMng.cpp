#include "UserMng.h"
#include "TcpSockMng.h"
#include "Log.h"
#include <cstdio>

struct DiskStkInfoB
{
};

class CCloudDataIOMng
{
public:
	static CCloudDataIOMng* GetInstance()
	{
		return nullptr;
	}
};

CUserMng* CUserMng::m_pThis = NULL;

void USERDATA::Init()
{
	pBuf = CUserMng::GetInstance()->AllocRcvBuf();//new char[MAX_CACHE_BUFLEN+10];
}

// 务必最后释放的时候release，不要依靠析构，因为中间临时变量太多，容易出问题；
// 其他时候，临时变量对拷就不用重新申请空间;
void USERDATA::ResetIt()
{
	if (pBuf)
	{
		// 统一外部释放 delete [] buf;
		CUserMng::GetInstance()->FreeRcvBuf(pBuf);
		pBuf = NULL;
	}

	pServerHandle = 0;
	pClinetHandle = 0;
	iClientSockId = 0;
	// 缓存信息;
	pBuf = NULL;
	iRecvLen = 0;
	// 连接信息;
	bAuth = false;
	llLastAcitve = 0;
	iRef = 0;
	nUserType = 0;
	nNetSourceType = 0;
	bAskDel = false;
	enMsgType = MSG_DEFAULT;
	// 请求信息;
	ulThreadId = 0;
	ulReqGNID = 0;
	llNRequest = 0;
	lpKeyMap = NULL;
	lpTagCodeMap = NULL;

	if (lpKeyMap)
	{
		lpKeyMap->clear();
		nsdk_del(lpKeyMap);
	}

	if (lpTagCodeMap)
	{
		lpTagCodeMap->clear();
		nsdk_del(lpTagCodeMap);
	}
}

//*********** 用户管理 ***********;

int UserManageFunc(void* lpParameter)
{
	if (lpParameter)
	{
		USERDATA* puserdata = (USERDATA*)lpParameter;
		CUserMng::GetInstance()->ReleaseIt(puserdata);
	}
	return 0;
}

CUserMng* CUserMng::GetInstance()
{
	if (!m_pThis)
	{
		m_pThis = new CUserMng;
	}
	return m_pThis;
}

CUserMng::CUserMng()//:m_MemMng(MAX_CACHE_BUFLEN+10)
{
	m_mutex.Create();
}

CUserMng::~CUserMng()
{
	m_mutex.Close();
}

void CUserMng::Init(int p_iMaxUserCount)
{
	CAutoCS ac(&m_mutex);
	USERDATA stUserData;
	m_vecUser.resize(p_iMaxUserCount, stUserData);
	m_iUserCount = 0;
	m_iMaxUserCount = p_iMaxUserCount;
}


bool CUserMng::Add(USERDATA* p_pUserData)
{
	CAutoCS ac(&m_mutex);
	if (m_iUserCount >= m_iMaxUserCount)
	{
		return false;
	}
	int i;
	for (i = 0; i < m_iMaxUserCount; i++)
	{
		if (m_vecUser[i].iRef == 0)
		{
			p_pUserData->iRef = 1;
			m_iUserCount++;
			memcpy(&m_vecUser[i], p_pUserData, sizeof(USERDATA));
			HDATA_HCLIENT stCSkkey = { p_pUserData->pServerHandle, p_pUserData->pClinetHandle };
			m_mapAllClient[stCSkkey].i = i;
			m_mapAllClient[stCSkkey].nNetSourceType = p_pUserData->nNetSourceType;
			break;
		}
	}
	return true;
}

int  CUserMng::DelAll()
{
	int	notdelete = 0;
	std::vector<HS> vhs;
	std::vector<HCLIENT> vhc;
	//避免死锁;
	{
		CAutoCS ac(&m_mutex);
		int i;
		for (i = 0; i < m_vecUser.size(); i++)
		{
			if (m_vecUser[i].iRef > 0)
			{
				m_vecUser[i].iRef--;
				if (m_vecUser[i].iRef == 0)
				{
					// tcp 要关闭;
					if (m_vecUser[i].nNetSourceType == TCP_SOURCE_TYPE)
					{
						vhs.push_back(m_vecUser[i].pServerHandle);
						vhc.push_back(m_vecUser[i].pClinetHandle);
					}
					m_iUserCount--;
					m_vecUser[i].ResetIt();
				}
				else
				{
					++notdelete;
					//MT_WARN("error not delete :%d",notdelete);
				}
			}
		}
	}
	int i;
	for (i = 0; i < vhs.size(); i++)
	{
		CTcpSockMng::GetInstance()->TcpSockClose(vhs[i], vhc[i], "service close client", strlen("service close client"));
	}
	m_mapAllClient.clear();
	return notdelete;
}

void CUserMng::ForceDelRes()
{
	//避免死锁;
	CAutoCS ac(&m_mutex);
	for (int i = 0; i < m_vecUser.size(); i++)
	{
		if (m_vecUser[i].iRef > 0)
		{
			m_vecUser[i].iRef--;
			m_iUserCount--;
			m_vecUser[i].ResetIt();
		}
	}
}

USERDATA* CUserMng::Query(HS p_refServerHandle, HCLIENT p_refClinetHandle)
{
	CAutoCS ac(&m_mutex);
	USERDATA* p = NULL;
	int i;
	HDATA_HCLIENT stCSkkey = { p_refServerHandle, p_refClinetHandle };
	if (m_mapAllClient.find(stCSkkey) != m_mapAllClient.end())
	{
		i = m_mapAllClient[stCSkkey].i;
		if (m_vecUser[i].iRef > 0 &&
			CTcpSockMng::GetInstance()->TcpSockCompare(m_vecUser[i].pClinetHandle, p_refClinetHandle) == 0)
		{
			m_vecUser[i].iRef++;
			p = &m_vecUser[i];
		}
	}
	return p;
}

void CUserMng::SetAuth(USERDATA*& p_refUserDta, bool bAuth)
{
	CAutoCS ac(&m_mutex);
	p_refUserDta->bAuth = bAuth;
}

void CUserMng::ReleaseIt(USERDATA* p_pstUserData)
{
	HS pServerHandle;
	HCLIENT hc;
	if (!p_pstUserData)	// 导致崩溃;
		return;
	bool needrelease = false;
	//避免死锁;
	{
		CAutoCS ac(&m_mutex);
		p_pstUserData->iRef--;
		if (p_pstUserData->iRef == 0)
		{
			pServerHandle = p_pstUserData->pServerHandle;
			hc = p_pstUserData->pClinetHandle;
			HDATA_HCLIENT stCSkkey = { p_pstUserData->pServerHandle,p_pstUserData->pClinetHandle };
			if (p_pstUserData->nNetSourceType == TCP_SOURCE_TYPE ||
				p_pstUserData->nNetSourceType == WEBSOCK_SOURCE_TYPE)
			{
				needrelease = true;
			}
			m_iUserCount--;
			p_pstUserData->ResetIt();

			// 20091001
			// 重大问题，这个地方应该也要释放，否则下次可能找到，具体导致是del跟release顺序反了????
			m_mapAllClient.erase(stCSkkey);
			m_aSubUser.erase(stCSkkey);
		}
	}
	if (needrelease)
	{
		if (p_pstUserData->nNetSourceType == TCP_SOURCE_TYPE)
		{
			const char* p_szErrData = "user ref = 0";
			CTcpSockMng::GetInstance()->TcpSockClose(pServerHandle, hc, p_szErrData, strlen(p_szErrData));
		}
		else if (p_pstUserData->nNetSourceType == WEBSOCK_SOURCE_TYPE)
		{
		}
	}
}
// 做特殊处理，不能多次del，因为del是不需要query的，容易直接把计数器多次消耗;
// 理论上del只要发出过一次，就可以了，其他地方release可以自动检测;
void CUserMng::Del(HS p_refServerHandle, HCLIENT p_refClinetHandle)
{
	bool needrelease = false;
	HDATA_HCLIENT stCSkkey = { p_refServerHandle, p_refClinetHandle };
	//避免死锁;
	{
		CAutoCS ac(&m_mutex);
		int i = 0;
		if (m_mapAllClient.find(stCSkkey) != m_mapAllClient.end())
		{
			i = m_mapAllClient[stCSkkey].i;
			if (m_vecUser[i].iRef > 0 &&
				CTcpSockMng::GetInstance()->TcpSockCompare(m_vecUser[i].pClinetHandle, p_refClinetHandle) == 0)
			{
				if (m_vecUser[i].bAskDel)
				{	// 如果已经发出过删除请求，不能再次发送;
					return;
				}
				m_vecUser[i].bAskDel = true;
				m_vecUser[i].iRef--;
				if (m_vecUser[i].iRef == 0)
				{
					if (m_vecUser[i].nNetSourceType == TCP_SOURCE_TYPE)
					{
						needrelease = true;
					}
					m_iUserCount--;
					m_vecUser[i].ResetIt();

					m_mapAllClient.erase(stCSkkey);
					m_aSubUser.erase(stCSkkey);
				}
			}
		}
	}
	if (needrelease)
	{
		CTcpSockMng::GetInstance()->TcpSockClose(p_refServerHandle, p_refClinetHandle, "service close client", strlen("service close client"));
	}
}

int CUserMng::FreeMemory()
{
	CAutoCS ac(&m_mutex);
	// 定期回收空闲的，峰值后，会导致虚拟内存下不来;
	return 1;//m_MemMng.release_memory();

}

int CUserMng::GetCount()
{
	CAutoCS ac(&m_mutex);

	return m_iUserCount;
}

char* CUserMng::AllocRcvBuf()
{
	//CAutoCS ac(&m_cs);
	char* p = (char*)new char[MAX_CACHE_BUFLEN + 1024];//m_MemMng.ordered_malloc();
	return p;
}

void CUserMng::FreeRcvBuf(char* p_pBuf)
{
	nsdk_del_arry(p_pBuf);
}

int  CUserMng::UpSetClientStockDataSubData(HS p_refServerHandle, HCLIENT p_refClinetHandle, const std::map<unsigned long long, StockDataSubType>& p_refmapAllKey,
	std::map<unsigned long long, TagCode>& p_mapTagCode, int p_nSubWay, unsigned char p_ucReqPackType, std::string& p_strMsg, bool p_bJson)
{
	CAutoCS ac(&m_mutex);
	int i, num = 0;
	HDATA_HCLIENT stCSkkey = { p_refServerHandle, p_refClinetHandle };

	if (m_mapAllClient.find(stCSkkey) != m_mapAllClient.end())
	{
		i = m_mapAllClient[stCSkkey].i;

		// 如果曾经打开，就一直用protobuf
		if (PROTOBUF_PACK_TYPE == p_ucReqPackType)
		{
			m_mapAllClient[stCSkkey].ucReqPackType = p_ucReqPackType;
			m_aSubUser[stCSkkey].ucReqPackType = p_ucReqPackType;
			m_aSubUser[stCSkkey].bJson = p_bJson;
		}
		m_mapAllClient[stCSkkey].nNetSourceType = m_vecUser[i].nNetSourceType;
		if (m_vecUser[i].iRef > 0 &&
			CTcpSockMng::GetInstance()->TcpSockCompare(m_vecUser[i].pClinetHandle, p_refClinetHandle) == 0)
		{
			std::map<unsigned long long, StockDataSubType>::iterator obj_it;
			std::map<unsigned long long, StockDataSubType>* pKeyMap = NULL;
			std::map<unsigned long long, TagCode>* pTagCodeMap = NULL;
			DiskStkInfoB stInstk;
			CCloudDataIOMng* pNetIO = CCloudDataIOMng::GetInstance();

			if (m_vecUser[i].lpKeyMap == NULL)
			{
				m_vecUser[i].lpKeyMap = new std::map<unsigned long long, StockDataSubType>;
			}
			if (m_vecUser[i].lpTagCodeMap == NULL)
			{
				m_vecUser[i].lpTagCodeMap = new std::map<unsigned long long, TagCode>;
			}
			pKeyMap = m_vecUser[i].lpKeyMap;
			pTagCodeMap = m_vecUser[i].lpTagCodeMap;

			if (p_nSubWay == SUB_CLEAR)
			{
				pKeyMap->clear();
				pTagCodeMap->clear();
				p_strMsg = "clear all sub ok";
			}
			else
			{
				// 按照入参的Key更新
				for (auto itk = p_refmapAllKey.begin(); itk != p_refmapAllKey.end(); itk++)
				{
					if (p_nSubWay == SUB_ADD)
					{
						//struct DiskStkInfoB* stCode = pNetIO->GetDiskStkInfoB(itk->first, stInstk);
						//if (stCode != NULL)
						//{
						//	if ((*pKeyMap).find(itk->first) == (*pKeyMap).end())
						//	{
						//		(*pKeyMap)[itk->first] = itk->second;
						//		if (p_mapTagCode.find(itk->first) != p_mapTagCode.end())
						//		{
						//			(*pTagCodeMap)[itk->first] = p_mapTagCode[itk->first];
						//		}
						//	}
						//	else
						//	{
						//		//已经订阅过的品种, 整合到原有的品种订阅数据
						//		StockDataSubType& szSubType = (*pKeyMap)[itk->first];
						//		szSubType |= itk->second;
						//	}
						//}
						//else
						//{
						//	//无效的品种
						//	char buf[32] = { 0 };
						//	snprintf(buf, sizeof(buf), "%llu,", itk->first);
						//	p_strMsg.append(buf);
						//}
					}
					else if (p_nSubWay == SUB_DEL)
					{
						if ((*pKeyMap).find(itk->first) != (*pKeyMap).end())
						{
							StockDataSubType& szSubType = (*pKeyMap)[itk->first];
							szSubType &= itk->second;
							//如果品种各项数据都取消了订阅, 则删除该品种订阅
							if (szSubType.value == 0)
							{
								(*pKeyMap).erase(itk->first);
								(*pTagCodeMap).erase(itk->first);
							}
						}
						else
						{
							//删除没有的品种
							char buf[32] = { 0 };
							snprintf(buf, sizeof(buf), "%llu,", itk->first);
							p_strMsg.append(buf);
						}
					}
				}

				if (p_nSubWay == SUB_ADD)
				{
					if (p_strMsg.empty())
					{
						p_strMsg = "sub ok";
					}
					else
					{
						MT_WARN("[主站服务] 订阅失败的品种,%s", p_strMsg.c_str());
						p_strMsg.append("sub error");
					}
				}
				else if (p_nSubWay == SUB_DEL)
				{
					if (p_strMsg.empty())
					{
						p_strMsg = "unsub ok";
					}
					else
					{
						MT_WARN("[主站服务] 取消订阅失败的品种,%s", p_strMsg.c_str());
						p_strMsg.append("unsub error");
					}
				}
			}

			// 订阅队列不空，认为是订阅客户，否则删除
			num = (*pKeyMap).size();
			//有功能号数据订阅、或者品种数据订阅
			if ((*pKeyMap).size() > 0)
			{
				m_aSubUser[stCSkkey].i = i;
				m_aSubUser[stCSkkey].nNetSourceType = m_vecUser[i].nNetSourceType;
				m_aSubUser[stCSkkey].bJson = p_bJson;
			}
			else
			{
				m_aSubUser.erase(stCSkkey);
			}
		}
	}
	return num;
}

int  CUserMng::GetUserStockDataSub(HS p_refServerHandle, HCLIENT p_refClinetHandle, std::map<unsigned long long, StockDataSubType>& p_mapNKey, std::map<unsigned long long, TagCode>& p_mapTagCode)
{
	p_mapNKey.clear();
	CAutoCS ac(&m_mutex);
	int i, num = 0;
	HDATA_HCLIENT stCSkkey = { p_refServerHandle, p_refClinetHandle };
	if (m_mapAllClient.find(stCSkkey) != m_mapAllClient.end())
	{
		i = m_mapAllClient[stCSkkey].i;
		if (m_vecUser[i].iRef > 0 &&
			CTcpSockMng::GetInstance()->TcpSockCompare(m_vecUser[i].pClinetHandle, p_refClinetHandle) == 0)
		{
			if (m_vecUser[i].lpKeyMap)
			{
				p_mapNKey = *(m_vecUser[i].lpKeyMap);	// 低效率复制拷贝
				p_mapTagCode = *(m_vecUser[i].lpTagCodeMap);
			}
		}
	}
	return num;
}

void CUserMng::GetAllUser(std::map<HDATA_HCLIENT, HDATA_SECOND>& p_refmapAllUser)
{
	CAutoCS ac(&m_mutex);
	p_refmapAllUser = m_mapAllClient;
}

void CUserMng::GetAllSubUser(std::map<HDATA_HCLIENT, HDATA_SECOND>& p_refmapAllSubUser)
{
	CAutoCS ac(&m_mutex);

	p_refmapAllSubUser = m_aSubUser;
}
