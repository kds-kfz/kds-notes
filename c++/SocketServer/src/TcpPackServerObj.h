#ifndef _TCP_SERVER_SERVER_H_
#define _TCP_SERVER_SERVER_H_

#include <stdio.h>
#include <sys/stat.h>  
#include <iostream>
#include <algorithm>

#include "SocketInterface.h"
#include "HPSocket.h"

using namespace std;

class  CTcpPackServerObj : public ITcpPackServer
{
	virtual EnHandleResult OnClose(ITcpServer* pSender, CONNID dwConnID, EnSocketOperation enOperation, int iErrorCode)
	{
		// 客户端关闭连接时的处理逻辑
		// 例如：记录日志，重新连接等
		printf("client closed, conn_id=%llu\n", (unsigned long long)dwConnID);

		// 如果需要继续处理，返回 HR_OK
		return HR_OK;
	}
};

#endif
