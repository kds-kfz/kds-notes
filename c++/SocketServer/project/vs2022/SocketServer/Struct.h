#ifndef _STRUCT_SERVER_H_
#define _STRUCT_SERVER_H_

#include <string.h>

// 请求缓存
struct ReqCacheData
{
	unsigned long long ullConnID;
	unsigned long ulLength;
	unsigned long ulPos;
	char *pBuf;
public:
	ReqCacheData() :ullConnID(0), ulLength(0), ulPos(0), pBuf(nullptr) {}
	~ReqCacheData() {
		if (nullptr != pBuf)
		{
			delete []pBuf;
			pBuf = nullptr;
		}
	}
	ReqCacheData& operator=(const ReqCacheData &p_stData) {
		if (this != &p_stData) {
			ullConnID = p_stData.ullConnID;
			ulLength = p_stData.ulLength;
			ulPos = p_stData.ulPos;
			memcpy(pBuf, p_stData.pBuf, p_stData.ulLength);
		}
		return *this;
	}
};

//用户信息
struct ClientData
{
	unsigned long long ullConnID;
	char szIp[32];
	unsigned short unPort;
public:
	ClientData(){
		memset(this, 0, sizeof(ClientData));
	}
	ClientData& operator=(const ClientData &p_stData) {
		if (this != &p_stData) {
			ullConnID = p_stData.ullConnID;
			unPort = p_stData.unPort;
			memcpy(szIp, p_stData.szIp, 32);
		}
		return *this;
	}
};

//通知任务
struct NotifyTask
{
	char *pBuf;
	unsigned int uiLen;
	unsigned long long ullConnID;
	char szErrMsg[1024];
	unsigned long long ullTaskID;
	TcpSockNotifyType enNotifyType;
public:
	NotifyTask(): pBuf(nullptr), uiLen(0), ullConnID(0), ullTaskID(0){
		memset(szErrMsg, 0, 1024);
	}
	~NotifyTask() {
		if (nullptr != pBuf)
		{
			//TODO 并发太高由应用层使用完成后释放,后续管理每个连接固定缓存大小
			//delete[]pBuf;
			pBuf = nullptr;
		}
	}
};

#endif