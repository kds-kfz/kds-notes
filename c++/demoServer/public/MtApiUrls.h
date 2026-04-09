#ifndef __MT_API_URL_H__
#define __MT_API_URL_H__

#include<string>

//MT中间件服务接口说明
struct ST_MT_URL_MSG
{
	std::string strUrl;	//服务url
	std::string strDesc;//url描述
};

extern const ST_MT_URL_MSG g_arrMtUrlMsg[];
extern const int g_nMtUrlMsgCount;

#endif