#include "MtApiUrls.h"
#include "nsdk_atomic.h"

const ST_MT_URL_MSG g_arrMtUrlMsg[] = {
	{"/symbols", "获取所有品种"}, 
	{"/place_order","下单"},
	{"/update_order", "修改订单"}, 
	{"/accounts/orders","订单列表/详情"},
	{"/close_position","平仓"}, 
	{"/login","登录"},
	{"/position","持仓"}
};

const int g_nMtUrlMsgCount = nsdk_arry_size(g_arrMtUrlMsg);
