#include "publicfunc.h"
//#include "IceRPCPush.hHandle"
#include "vld.h"
#include "ClientThreadPool.h"


#ifdef _DEBUG
// 调试计数用于观察异步回调对象是否成对创建和释放。
DWORD		CJsonBinRPCCallBack::s_dwRpcPack = 0;
DWORD		CJsonBinRPCCallBack::s_dwException = 0;
#endif



