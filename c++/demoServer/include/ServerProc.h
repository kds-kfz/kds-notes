#if !defined(KDSC_SERVER_PROC_H)
#define KDSC_SERVER_PROC_H

#include "nsdk_mutex.h"
#include "UserData.h"

//业务处理类;
class CServerProc
{
public:
	CServerProc();
	~CServerProc();

	static void Release();
	static CServerProc *GetInstance();

	int Process(struct NetRequsetDat * netreqdata,char *receive_buf,char *transfer_buf,long &len,char *viewbuf);

private:
	static CServerProc *m_pThis;

private:
	int   getreqtype(short req);
	int   GetReqPbType(short req);//获取pb协议所属业务类型

	//TODO 异步回调 目前暂时没有场景
	int AsynRoutine(void* p_pReslut);

	//TODO 正常的业务流程

private:
	//TODO 代码链
public:
	//TODO 其他具体实现的类 如：排序，等
};

#endif