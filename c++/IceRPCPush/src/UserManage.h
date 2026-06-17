// UserManage.h: CUserManage 类接口。
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_USERMANAGE_H__208233F2_F47F_41BA_BBFD_7CC7604B9FA2__INCLUDED_)
#define AFX_USERMANAGE_H__208233F2_F47F_41BA_BBFD_7CC7604B9FA2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "userdata.h"
#include "sexport.h"
#include "AutoCS.h"
#include <map>
#include "compat/SimplePool.h"

// SocketServer 服务句柄与客户端句柄组成唯一连接键。
struct ST_HDATA_HCLIENT
{
	HS		hServer;		// 记录了提供端口服务的总句柄
    HCLIENT hHandle;		// 记录了这个用户的链接句柄
	friend bool operator<(const ST_HDATA_HCLIENT &p_refLeft,const ST_HDATA_HCLIENT &p_refRight)//定义struct比较方法
	{
		if ( p_refLeft.hServer != p_refRight.hServer)
			return p_refLeft.hServer < p_refRight.hServer;
		else
			return p_refLeft.hHandle < p_refRight.hHandle;
	}
};

// 订阅状态枚举，保留旧推送协议的整数值约定。
enum EN_SUB_STATE
{
	SUB_DEL	= 0,
	SUB_ADD	= 1,
	SUB_CLEAR=2,
	SUB_ALL	= 3,	// 订阅全部
};

// TCP 推送连接管理器，负责连接槽位、引用计数和接收缓存池。
class CUserManage  
{
public:
	CUserManage();
	virtual ~CUserManage();
    
	// 初始化最大用户数并预分配槽位，避免连接高峰时频繁扩容。
	void Init(int p_iMaxUser);

// 添加新连接，pdata 的动态成员会随结构体复制到内部槽位。
	bool Add(ST_USER_DATA* p_pData);
// 标记连接断开并按引用计数决定是否立即释放资源。
	void Del(HS p_hServer,HCLIENT p_hHandle);
// 删除全部连接，返回仍被业务线程引用而未能立即释放的数量。
	int  DelAll();
// 强制释放所有槽位资源，只在进程退出或异常清理时使用。
	void ForceDelRes();
	// Query 会增加引用计数，业务处理完必须调用 ReleaseIt。
    ST_USER_DATA * Query(HS p_hServer,HCLIENT p_hHandle);
// 释放 Query 获取的引用，计数归零时关闭连接并回收缓存。
	void ReleaseIt(ST_USER_DATA * p_pData);
	
// 暴露锁入口兼容旧调用，新增逻辑优先使用 CAutoCriticalRegion。
	void Lock(){	EnterCriticalSection(&m_csLock);}
	void Unlock(){	LeaveCriticalSection(&m_csLock);}

	//统计
	//取总的在线人数
	int GetCount();
	int GetFindIn();
	// 接收缓冲区来自统一池，便于控制单连接最大内存占用。
	char *	AllocRcvBuf();
	void	FreeRcvBuf(char * p_pBuf);
	// 得到所有订阅用户
// 快照当前订阅用户集合，推送线程据此遍历目标连接。
	void	GetAllSubUser(std::map<ST_HDATA_HCLIENT,int> & p_refUsers);
// 更新单连接订阅功能号，网络回调解析订阅包后调用。
	void	UpdateSubKey(ST_USER_DATA* p_pData,const std::map<DWORD,int> & p_refSubId);
	// 读取单连接订阅集合，推送前用于判断 lReqNo 是否需要发送。
	void	GetSubKey(ST_USER_DATA* p_pData,std::map<DWORD,int> & p_refSubId);
public:
// 预分配连接槽位数组，ST_USER_DATA 内含动态缓存指针。
	std::vector<ST_USER_DATA > m_aUser;
// 在线用户数和允许的最大连接数。
	int m_iUserCount,m_iMaxUserCount;
// 保护槽位数组、查找表和内存池的全局锁。
  	CRITICAL_SECTION				m_csLock;

	// 连接查找表和订阅用户表，value 是 m_aUser 中的槽位索引。
	std::map<ST_HDATA_HCLIENT,int>		m_mapFind,m_mapSubUser;	// 记录句柄对应的I位置，空间提前安排好
// 接收缓存池，每个块按 MAX_REQ_BUFLEN 级别预留空间。
	CSimplePool					m_clMemMng; 

};


// CAutoReleaseFunc 使用的释放适配函数，统一把 ST_USER_DATA 引用归还给管理器。
DWORD WINAPI UserManageFunc(LPVOID p_pParameter	);

#endif // !defined(AFX_USERMANAGE_H__208233F2_F47F_41BA_BBFD_7CC7604B9FA2__INCLUDED_)
