#if !defined(KDSC_USER_MANAGE_H)
#define KDSC_USER_MANAGE_H

#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "SocketServer.h"
#include "UserData.h"
#include "AutoCS.h"

const int MAX_CACHE_BUFLEN = 1024 * 1024;

struct HDATA_HCLIENT
{
	HS		pServerHandle;		// 记录了提供端口服务的总句柄;
	HCLIENT pClinetHandle;		// 记录了这个用户的链接句柄;
	friend bool operator<(const HDATA_HCLIENT& a, const HDATA_HCLIENT& b)//定义struct比较方法;
	{
		if (a.pServerHandle != b.pServerHandle)
			return a.pServerHandle < b.pServerHandle;
		else
			return a.pClinetHandle < b.pClinetHandle;
	}
};

struct HDATA_SECOND
{
	int		i;
	unsigned char	ucReqPackType;// 是否采用protobuffer格式推送;
	int		nNetSourceType;		// 数据网络来源，tcp或者http，websock;
	bool	bJson;				// 若是发送pb是否发送json或二进制,不支持同一个用户发两种数据类型;
	HDATA_SECOND()
	{
		memset(this, 0, sizeof(HDATA_SECOND));
	}
};

enum
{
	SUB_DEL = 0,
	SUB_ADD = 1,
	SUB_CLEAR = 2
};

int UserManageFunc(void* lpParameter);

//用户管理;
class CUserMng
{
private:
	static CUserMng* m_pThis;
public:
	CUserMng();
	virtual ~CUserMng();
	static CUserMng* GetInstance();

	//最大用户个数;
	void Init(int p_iMaxUserCount);
	bool Add(USERDATA* p_pUserData);
	void Del(HS p_refServerHandle, HCLIENT p_refClinetHandle);
	int  DelAll();
	void ForceDelRes();
	// query会增加引用讲数,用完应该Release;
	USERDATA* Query(HS p_refServerHandle, HCLIENT p_refClinetHandle);
	// 释放用户;
	void ReleaseIt(USERDATA* p_pstUserData);
	// 统计, 取总的在线人数;
	int GetCount();
	// 释放空闲内存;
	int	FreeMemory();
	// 用户缓存区申请;
	char* AllocRcvBuf();
	// 释放缓冲区;
	void FreeRcvBuf(char* p_pBuf);
	//设置股票订阅数据;
	int UpSetClientStockDataSubData(HS p_refServerHandle, HCLIENT p_refClinetHandle, const std::map<unsigned long long, StockDataSubType>& p_refmapAllKey, std::map<unsigned long long, TagCode>& p_mapTagCode,
		int p_nSubWay, unsigned char p_ucReqPackType, std::string& p_strMsg, bool p_bJson = false);
	// 得到所有订阅用户;
	void GetAllSubUser(std::map<HDATA_HCLIENT, HDATA_SECOND>& p_refmapAllSubUser);
	// 得到某个用户订阅请求;
	int GetUserStockDataSub(HS p_refServerHandle, HCLIENT p_refClinetHandle, std::map<unsigned long long, StockDataSubType>& aKey, std::map<unsigned long long, TagCode>& p_mapTagCode);
	// 得到所有用户;
	void GetAllUser(std::map<HDATA_HCLIENT, HDATA_SECOND>& p_refmapAllUser);
	// 设置认证;
	void SetAuth(USERDATA*& p_refUserDta, bool bAuth);

public:
	std::vector<USERDATA> m_vecUser;
	int m_iUserCount, m_iMaxUserCount;
	nsdk::CMutex m_mutex;

	std::map<HDATA_HCLIENT, HDATA_SECOND> m_mapAllClient, m_aSubUser;	// 记录句柄对应的I位置，空间提前安排好
};

#endif