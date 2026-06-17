// AutoCS.hHandle: interface for the CAutoCS class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_AUTOCS_H__865D1508_075E_4258_991F_6A87E43DA7C7__INCLUDED_)
#define AFX_AUTOCS_H__865D1508_075E_4258_991F_6A87E43DA7C7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


// 临界区 RAII 包装，保证异常或提前返回时也能释放锁。
class CAutoCriticalRegion  
{
public:
	CAutoCriticalRegion(CRITICAL_SECTION * p_pCriticalSection);
	virtual ~CAutoCriticalRegion();

// 只保存外部临界区地址，不拥有 CRITICAL_SECTION 生命周期。
	CRITICAL_SECTION *m_pCriticalSection;

};

// 作用域退出时调用释放函数，统一处理 Query 后的引用释放。
class	CAutoReleaseFunc
{
public:
	CAutoReleaseFunc(void * p_pData,LPTHREAD_START_ROUTINE p_pfnRelease)
	{
		m_pData			= p_pData;
		m_pfnRelease	= p_pfnRelease;
	}
	~CAutoReleaseFunc()
	{
		m_pfnRelease(m_pData);
	}
public:
// 需要回传给释放函数的对象指针。
	void	*				m_pData;
// Windows 线程函数签名的释放回调，兼容旧代码写法。
	LPTHREAD_START_ROUTINE	m_pfnRelease;
};

#endif // !defined(AFX_AUTOCS_H__865D1508_075E_4258_991F_6A87E43DA7C7__INCLUDED_)
