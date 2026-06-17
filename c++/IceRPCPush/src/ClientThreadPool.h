/*	多线程处理Client请求队列的框架，每个Node节点里面的 HANDLE 就是CJsonBinRPCImp* 。
客户端的角色主要是调用Slice函数，如果要推送，单独用StockPush；客户端没有Slice被动调用，不用 JsonBinSrvPopfront弹出，JsonBinSrvComplete删除节点

*/
#pragma once

#include <Ice/Ice.h>
#include "DQueue.h"
#include "CJsonBinRPCImp.h"


// struct  TASYNReqQueue
// {
// 	short											m_chMode;		// EN_JSON_INPUT_RPC  EN_JSON_INPUT_PUT
// 	short											submode;	// 0==async  1==Pre
// 
// 	HANDLE											m_hHandle;
// 	long long											m_lSynId;
// 	long long											m_lFuncId;
// 	long long											setcode;
// 	::JSONBINRPC::AByte							m_stJsonReq;
// 	LPTHREAD_START_ROUTINE							func;
// 	void*											m_pParam;
// 	ST_JSON_M_RESULT_TOP	*							me;
// 	//tag_ICEHRESULT*								hice;
// 
// 
// 	// char											m_szErrInfo[64];
// 	TASYNReqQueue()
// 	{
// 		m_chMode = 0;
// 		submode = 0;
// 		//memset(m_szErrInfo,0,sizeof(m_szErrInfo));
// 		m_hHandle = NULL;
// 		func = NULL;
// 		me = NULL;
// 		m_lSynId = setcode = m_lFuncId = 0;
// 	}
// };

// Ice 客户端异步回调对象，负责把远程返回转换为 DLL 对外结果结构。
class CJsonBinRPCCallBack : public IceUtil::Shared
{
public:
#ifdef _DEBUG
	static		DWORD		s_dwRpcPack,s_dwException;
	int			m_iStatus;
	CJsonBinRPCCallBack()
	{
		m_iStatus = 0;
		++s_dwRpcPack;
	}
	~CJsonBinRPCCallBack()
	{
		--s_dwRpcPack;
// 		//旧调试输出("~JBRPC [%d]:%u  %u \r\n",m_iStatus,s_dwRpcPack,s_dwException);
// 		if ( m_iStatus )
// 			旧调试输出(".");
// 		else
// 			旧调试输出("*");
	}
#endif
	// 正常返回路径：解压输出参数并把结果交给调用方回调。
	void ice_response(::Ice::Long p_lRetVal, ::Ice::Long p_lParam, const ::JSONBINRPC::AByte& p_stLParam, ::Ice::Long p_wParam, const ::JSONBINRPC::AByte& p_stWParam, const ::std::string& p_szErrInfo)
	{
#ifdef _DEBUG
		m_iStatus	= 1;
#endif
		ST_JSON_MULTI_RESULT_EX * pResult = new ST_JSON_MULTI_RESULT_EX;
		pResult->lSynId		= m_lSynId;
		pResult->lFuncId		= m_lFuncId;
		pResult->aReqJson	= m_stJsonReq;
		//pResult->stJsonReq		= pResult->aReqJson.c_str();
		pResult->stJsonReq.lLen		= static_cast<int>(m_stJsonReq.size());
		pResult->stJsonReq.pBuffer	= m_stJsonReq.empty() ? NULL : (unsigned char*)&*pResult->aReqJson.begin();		// 一直存在的空间

		// pResult->pResult= nullptr;
		pResult->pParam	= m_pParam;
		pResult->lRetVal	= p_lRetVal;
		pResult->lParam	= p_lParam;
		pResult->wParam	= p_wParam;
		strncpy(pResult->szErrInfo,p_szErrInfo.c_str(),sizeof(pResult->szErrInfo)-1);

		size_t uLength = 0;
		if ( p_stLParam.size()> 0 )
		{
			//snappy::GetUncompressedLength((const char*)p_stLParam.data(), p_stLParam.size(), &uLength);
			//pResult->aLParam.resize(uLength);
			//char* pUncompressBuffer = new char[uLength];
			//bool bOk = snappy::RawUncompress((const char*)p_stLParam.data(), p_stLParam.size(),(char*)&*pResult->aLParam.begin());
			
			uLength = UnCompressAByte(pResult->aLParam,(const char*)p_stLParam.data(), SafeSizeToLength<long>(p_stLParam.size()));

			//pResult->aLParam.resize(uLength);
			//memcpy(&*pResult->aLParam.begin(),pUncompressBuffer,uLength);
			//delete []pUncompressBuffer;

			pResult->stLParam.lLen		= static_cast<int>(uLength);
			pResult->stLParam.pBuffer	= pResult->aLParam.empty() ? NULL : (unsigned char*)pResult->aLParam.data();

			//pResult->stLParam.lLen		= p_stLParam.size();
			//pResult->stLParam.pBuffer	= (char*)p_stLParam.data();//new char[pResult->stLParam.lLen];
			//memcpy(pResult->stLParam.pBuffer,&*p_stLParam.begin(),pResult->stLParam.lLen);
		}
		if ( p_stWParam.size()> 0 )
		{
			uLength = UnCompressAByte(pResult->aWParam,(const char*)p_stWParam.data(), SafeSizeToLength<long>(p_stWParam.size()));

			pResult->stWParam.lLen		= static_cast<int>(uLength);
			pResult->stWParam.pBuffer	= pResult->aWParam.empty() ? NULL : (unsigned char*)pResult->aWParam.data();//new char[pResult->stWParam.lLen];
			//memcpy(pResult->stWParam.pBuffer,&*p_stWParam.begin(),pResult->stWParam.lLen);
		}
		if ( m_dwCallTimes == 0 )
		{
			InterlockedIncrement(&m_dwCallTimes);
			m_pfnCallbackClient(pResult);
		}
		else
		{
			InterlockedIncrement(&m_dwCallTimes);
		}
		InterlockedDecrement64(&g_lCrowded);

	}
	void sent(bool p_bRet)
	{

	}
	// 异常返回路径：构造 lRetVal=-1 的结果，保证上层回调仍被触发。
	void exception(const Ice::Exception& p_refException)
	{
#ifdef _DEBUG
		m_iStatus	= 0;
		++s_dwException;
#endif
		ST_JSON_MULTI_RESULT_EX * pResult = new ST_JSON_MULTI_RESULT_EX;
		pResult->lRetVal = -1;
		pResult->lSynId		= m_lSynId;
		pResult->lFuncId		= m_lFuncId;
		pResult->aReqJson	= m_stJsonReq;
		//pResult->stJsonReq		= pResult->aReqJson.c_str();
		pResult->stJsonReq.lLen		= static_cast<int>(m_stJsonReq.size());
		pResult->stJsonReq.pBuffer	= m_stJsonReq.empty() ? NULL : (unsigned char*)&*pResult->aReqJson.begin();		// 一直存在的空间

		// pResult->pResult= nullptr;
		pResult->pParam	= m_pParam;
		strncpy(pResult->szErrInfo,p_refException.ice_id(),sizeof(pResult->szErrInfo)-1);
		if ( InterlockedIncrement(&m_dwCallTimes) == 1 )
		{
			m_pfnCallbackClient(pResult);
		}
			
		InterlockedDecrement64(&g_lCrowded);
	}
	// 防止 Ice 的 sent/response/exception 多路径导致上层回调重复执行。
	DWORD						m_dwCallTimes;
	long long						m_lSynId;
	long long						m_lFuncId;
	void *						m_pParam;
	// 保存压缩后的请求数据，回调结果引用其内部缓冲区。
	::JSONBINRPC::AByte			m_stJsonReq;
	LPTHREAD_START_ROUTINE		m_pfnCallbackClient;
	//::Ice::AsyncResultPtr		m_pResult;
};
typedef IceUtil::Handle<CJsonBinRPCCallBack> JsonBinRPCCallBackPtr;
typedef IceUtil::Handle<CJsonBinRPCCallBack> JsonBinPUTCallBackPtr;