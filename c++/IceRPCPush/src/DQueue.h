// DQueue.hHandle: interface for the CDQueue class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DQUEUE_H__EED1F0DE_8387_45C8_8D75_288B4EC98ABB__INCLUDED_)
#define AFX_DQUEUE_H__EED1F0DE_8387_45C8_8D75_288B4EC98ABB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <list>
#include "AutoCS.h"

// 轻量级线程安全指针队列，队列只保存节点指针，不负责节点生命周期。
template<class T>
class CDataQueue  
{
public:
	CDataQueue()
	{
		m_clMutex.Create();

		m_nQueueSize	= 0;
		m_pfnGreatFunc		= GreatFunc;
	}
	virtual ~CDataQueue()
	{
		m_clMutex.Close();
	}
	// 如果要进行排序，调用方可替换比较函数；当前排序插入仅保留兼容入口。
	bool (WINAPI *	m_pfnGreatFunc)(const T & p_refLeft,const T & p_refRight);	
	
private:
// 保护 list 和缓存队列长度的临界区。
	xsdk::CMutex			 m_clMutex;
// 实际队列容器，节点内存由压入/弹出两端约定释放。
	std::list<T*>		m_list;
// 缓存队列长度，SimpleGetTotals 读取它以减少加锁成本。
	size_t				m_nQueueSize;
public:
// 暴露手工加锁入口，供需要遍历 Reffer 的旧代码保持互斥。
	void		RefLock()
	{
		m_clMutex.Lock();
	}
	void		RefUnlock()
	{
		m_clMutex.Unlock();
	}
// 返回内部容器引用，调用方必须先 RefLock，避免遍历期间被其他线程修改。
	std::list<T*> &	Reffer()
	{
		return m_list;
	}
public:
// 快速读取缓存长度，允许存在短暂并发误差，仅用于限流/观察。
	int			SimpleGetTotals()	{	return m_nQueueSize;	}
	int			GetTotals()
	{
		CAutoCS clAutoLock(&m_clMutex);
		 m_nQueueSize = m_list.size();
		return m_nQueueSize;
	}
// 清空队列指针但不释放节点内容，调用前要确认节点已由调用方处理。
	void		Clear()
	{
		CAutoCS clAutoLock(&m_clMutex);
		 m_list.clear();
	}
// 从队尾压入节点，返回压入后的队列长度用于唤醒/限流判断。
	size_t			PushBack(T*	 p_pNode)
	{
		CAutoCS clAutoLock(&m_clMutex);

		 m_list.push_back(p_pNode);
		 m_nQueueSize = m_list.size();
		return m_nQueueSize;
	}
	size_t			PushFront(T*	 p_pNode)
	{
		CAutoCS clAutoLock(&m_clMutex);

		 m_list.push_front(p_pNode);
		 m_nQueueSize = m_list.size();
		return m_nQueueSize;
	}
	// 这个函数没有完工
	int			SortPushBack(T*	 p_pNode)
	{
		CAutoCS clAutoLock(&m_clMutex);
		
		 m_list.push_back(p_pNode);
		 m_nQueueSize = m_list.size();

		return m_nQueueSize;
	}
// 从队首弹出一个节点，返回后节点所有权转移给调用方。
	T*			PopFront()
	{
		T*	p_pNode = 0;
		
		CAutoCS clAutoLock(&m_clMutex);

		 if ( !m_list.empty() )
		 {
			 p_pNode = m_list.front();
			 m_list.pop_front();
		 }

		return p_pNode;
	}
	T*			PopBack()
	{
		T*	p_pNode = 0;
		
		CAutoCS clAutoLock(&m_clMutex);

		 if ( !m_list.empty() )
		 {
			 p_pNode = m_list.back();
			 m_list.pop_back();
		 }

		return p_pNode;
	}

	static bool WINAPI GreatFunc(const T & p_refLeft,const T & p_refRight)
	{
		return true;
	}

};







// 固定块内存池的历史块结构，目前仅保留旧接口兼容。
struct ST_CHUNK
{
    void Init(size_t p_uBlockSize,unsigned char p_chBlocks);
    void Release();
    void* Allocate(size_t p_uBlockSize);
    void Deallocate(void* p_pBlock,size_t p_uBlockSize);
// 连续块内存首地址，由 Init 分配并由 Release 释放。
    unsigned char *pData;
    unsigned char chFirstAvailableBlock; // 当前第一个可用块索引。
    unsigned char chBlocksAvailable;     // 当前剩余可用块数量。

// 保护块分配链表的临界区。
	xsdk::CMutex clMutex; 

	ST_CHUNK()
	{
		pData = NULL;
		chFirstAvailableBlock = 0;
		chBlocksAvailable = 0;
		clMutex.Create();
	}
	~ST_CHUNK()
	{
		clMutex.Close();
	}
};



#endif // !defined(AFX_DQUEUE_H__EED1F0DE_8387_45C8_8D75_288B4EC98ABB__INCLUDED_)
