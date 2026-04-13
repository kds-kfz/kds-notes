#include "ServerProc.h"

#include "Log.h"
#include "UserMng.h"
#include "nsdk.h"
#include "nsdk_atomic.h"

#include <algorithm>

CServerProc *CServerProc::m_pThis = NULL;

#define FJB_MAX_NUM 2000
CServerProc::CServerProc()
{
}

CServerProc::~CServerProc()
{

}

void CServerProc::Release()
{
	nsdk_del(m_pThis);
}

CServerProc *CServerProc::GetInstance()
{
	if (NULL == m_pThis)
	{
		m_pThis = new CServerProc;
	}
	return m_pThis;
}

// 不要再case下直接写代码，累计堆栈会打爆;
//处理客户端发过来的委托请求;
int  CServerProc::Process(NetRequsetDat * netreqdata,char *receive_buf, char *transfer_buf,
	long &len, char *viewbuf)
{
	//TODO 根据业务分批处理 在这里主要是处理长连接请求：例如TCP/WEB，HTTP请求不在这里处
	//处理完成后把结果放到 transfer_buf

	return 0;
}


int  CServerProc::getreqtype(short req)
{
	return 0;
}

int CServerProc::GetReqPbType(short req)
{
	return 0;
}

//异步回调;//插件间异步请求，异步应答回调
int CServerProc::AsynRoutine(void* p_pReslut)
{
	return MA_OK;
}

/******************************************************************/