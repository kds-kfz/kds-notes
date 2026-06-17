// AutoCS.cpp: implementation of the CAutoCS class.
//
//////////////////////////////////////////////////////////////////////

#include "publicfunc.h"
#include "AutoCS.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
// 构造时立即进入临界区，调用方只需控制对象作用域。
CAutoCriticalRegion::CAutoCriticalRegion(CRITICAL_SECTION * p_pCriticalSection)
{
   m_pCriticalSection=p_pCriticalSection;
   EnterCriticalSection(p_pCriticalSection);
}

// 析构时离开临界区，保证异常路径也释放锁。
CAutoCriticalRegion::~CAutoCriticalRegion()
{
   LeaveCriticalSection(m_pCriticalSection);
}
