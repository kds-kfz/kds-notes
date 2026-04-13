#if !defined(KDSC_AUTO_CS_H)
#define KDSC_AUTO_CS_H

#include "xsdk_mutex.h"

typedef int (*USER_MNG_FUNC)(void *p_refFunc);

//锁释放类;
class CAutoCS  
{
public:
	CAutoCS(xsdk::CMutex *p)
	{
		m_p = p;
		m_p->Lock();
	}

	virtual ~CAutoCS()
	{
		m_p->Unlock();
	}

	xsdk::CMutex *m_p;
};

//用户管理释放类;
class AutoReleaseFunc
{
public:
	AutoReleaseFunc(void *p, USER_MNG_FUNC pfunc)
	{
		lpPoint			= p;
		lpStartAddress	= pfunc;
	}
	~AutoReleaseFunc()
	{
		lpStartAddress(lpPoint);
	}
public:
	void *lpPoint;
	USER_MNG_FUNC	lpStartAddress;
};


#endif