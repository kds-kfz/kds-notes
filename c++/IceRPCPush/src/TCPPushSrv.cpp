#include "publicfunc.h"
#include "DQueue.h"
#include "IceRPCPush.h"
#include "sexport.h"
#include "UserManage.h"
#include "Log.h"

CUserManage	g_UserManage;

// 或者的关系
void TokenizeOR(std::vector<std::string>& p_refTokens, const std::string& p_strText, const std::string& p_strDelimiters);

// CAutoReleaseFunc 的回调适配器，确保网络数据处理后归还 ST_USER_DATA 引用。
DWORD WINAPI UserManageFunc(LPVOID p_pParam	)
{
	if ( p_pParam )
	{
		ST_USER_DATA * pUserData = (ST_USER_DATA*)p_pParam;
		pUserData->iHandleRef --;
		g_UserManage.ReleaseIt(pUserData);
	}
	return 0;
}

// SocketServer 兼容层的事件入口，负责连接生命周期和订阅包解析。
void WINAPI ServerCallBack(HS p_hHandle,HCLIENT p_hClient,EN_S_NOTIFY_TYPE p_enType,const void *p_pData,int p_iDataLen,const char *p_szIp,unsigned short p_uPort,void * p_pParam)
{
	switch(p_enType)
	{
	case EN_S_TYPE_CLOSE://通知连接断开
		{
			g_UserManage.Del(p_hHandle,p_hClient);
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "用户断开TCP连接[%p,%p] %m_hSocket:%d\n",p_hHandle,p_hClient,p_szIp,p_uPort);
		}
		break;
	case 	EN_S_TYPE_CONNECTED://通知连接成功
		{
			ST_USER_DATA ud;
			ud.hServer=p_hHandle;
			ud.hHandle=p_hClient;
			ud.tmLastActive=time(0);
			strcpy(ud.strIp,p_szIp);
			ud.iPort = p_uPort;
			// 把申请缓冲区，放到Manager内部 
			ud.Init(  );		// 申请缓冲区
			if(g_UserManage.Add(&ud))	// memcpy方式，连同指针一起复制
			{
				ST_USER_DATA * pUserData=g_UserManage.Query(p_hHandle,p_hClient);
				CAutoReleaseFunc	autoFunc(pUserData,UserManageFunc);
				if ( !pUserData )
				{
					
					g_UserManage.Del(p_hHandle,p_hClient);
					CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "用户连接添加失败[%p,%p] %m_hSocket:%d\n",p_hHandle,p_hClient,p_szIp,p_uPort);
				}
				else
				{
				}
				CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "用户TCP连接成功[%p,%p] %m_hSocket:%d\n",p_hHandle,p_hClient,p_szIp,p_uPort);
			}
			else
			{
				// 太多没有加入
				ud.ResetIt();
				CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "异常处理,连接用户过多[%p,%p] %m_hSocket:%d\n",p_hHandle,p_hClient,p_szIp,p_uPort);
			}
		}
		break;
	case 	EN_S_TYPE_ERROR://通知网络出错
		{
            g_UserManage.Del(p_hHandle,p_hClient);
			CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "异常处理,网络出错用户断开[%p,%p]:%m_hSocket\n",p_hHandle,p_hClient,p_pData);
		}
		break;
	case 	EN_S_TYPE_DATA://通知读取数据
		{
			ST_USER_DATA * pUserData=g_UserManage.Query(p_hHandle,p_hClient);
			CAutoReleaseFunc	autoFunc(pUserData,UserManageFunc);
			if(!pUserData)
			{
				s_close(p_hHandle,p_hClient);
				CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "查无此户，断开连接[%p,%p] %m_hSocket:%d\n",p_hHandle,p_hClient,p_szIp,p_uPort);
				break;
			}
			// 防止打穿缓冲区，超出单连接最大包缓存时直接丢弃并断开。
			if(p_iDataLen+pUserData->iRecvLen<= MAX_REQ_BUFLEN )
			{
				//保存最新数据
				memcpy(pUserData->pBuf+pUserData->iRecvLen,p_pData,p_iDataLen);
				pUserData->iRecvLen+=p_iDataLen;
				// 判断是否需要处理了，一个数据可能包含多个请求，此处循环拆包。
				while(true  )
				{
					if(pUserData->iRecvLen < sizeof(ST_REQ_HEADER))
						break;
					ST_REQ_HEADER *pi=NULL;
					pi=(ST_REQ_HEADER*)pUserData->pBuf;
					if(  pi->dwPacketLen <0 || pi->stInfo.chCompressed > 2 ||
						pi->dwPacketLen >= MAX_REQ_BUFLEN - sizeof(ST_REQ_HEADER) )
					{
						CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "异常处理 : TCP packtype Error,m_dwPacketLen:%d,p_szIp:%m_hSocket",pi->dwPacketLen,p_szIp);
						g_UserManage.Del(p_hHandle,p_hClient);
						return;		
					}
					//判断内容是否已经收完,没有则继续收
					if(pUserData->iRecvLen  <  pi->dwPacketLen+sizeof(ST_REQ_HEADER))
					{
						break;//不够一个包,退出循环继续收
					}
					// 不支持压缩
					short			ReqNo=0;	// 这个当做short拷贝的，不能用long
					memcpy(&ReqNo,(pi+1),sizeof(short));
					if ( ReqNo == PACKET_PUSH_HQ_SUB )
					{
						ST_SUB_UNSUB_STRING * ans = (ST_SUB_UNSUB_STRING *)(pi+1);
						if ( ans->lLen > 1 )
						{
							int		i;
							DWORD	dwV=0;
							std::map<DWORD,int>	m_mapSubId;
							const	char * pstr = NULL;
							std::vector<std::string>	tosecs;
							CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "B端订阅：%m_hSocket,%m_hSocket,%d\r\n",ans->pBuf,p_szIp,p_uPort);
							TokenizeOR(tosecs,ans->pBuf,"|");
							// 增加订阅的类型
							for ( i=0;i<tosecs.size();++i )
							{
								pstr = SundayQuickSearch(tosecs[i].c_str(),"~");
								if ( pstr )
								{
									dwV = atol(pstr+1);
									m_mapSubId[ dwV ]	= 0;		// 取消订阅
								}
								else 
								{
									dwV = atol(tosecs[i].c_str());
									m_mapSubId[ dwV ]	= 1;		// 订阅
								}
							}
							g_UserManage.UpdateSubKey(pUserData,m_mapSubId);
						}
						else
						{
							CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "无B端订阅：%d,%m_hSocket,%d\r\n",ans->lLen,p_szIp,p_uPort);
						}

					}
					else
					{
						CIceRPCPushLog::Instance().WriteDebug("DEBUG", "Trace", "未知协议：%d,%m_hSocket,%d\r\n",ReqNo,p_szIp,p_uPort);
					}
					//////////////////////////////////////////////////////////////////////////
					int left = pUserData->iRecvLen - (pi->dwPacketLen+sizeof(ST_REQ_HEADER));
					if ( left > 0 )
					{
						memmove(pUserData->pBuf,pUserData->pBuf+(pi->dwPacketLen+sizeof(ST_REQ_HEADER)),left);
					}
					pUserData->iRecvLen=left;
				}
			}

		}
		break;
	default:
		break;
	}

}


