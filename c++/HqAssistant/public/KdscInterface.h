#ifndef __KDSC_INTERFACE_H__
#define __KDSC_INTERFACE_H__

//说明: 各插件公共接口/协议定义

#include "KdscStruct.h"

/************************************************************
Protocol.h：未使用，只负责编译通过
struct.h：未使用，只负责编译通过
structB.h：服务器内部使用的结构体
JztProtocol.h：主站与客户端接口数据结构
JztStructB.h：客户端应答数据结构 + 服务器公共数据结构
************************************************************/

//协议号
#define	DC_PROTOCOL_GetCodeList				10003			// 查询代码链

#define DC_PROTOCOL_GetStaticDataInfo		10045		// 查询所有静态数据信息
#define DC_PROTOCOL_GetTradeDate			10046		// 查询某天是否交易日
#define DC_PROTOCOL_GetNotTradeDate			10047		// 查询一段时间非交易日
#define DC_PROTOCOL_GetStaticData			10048		// 查询静态数据块
#define DC_PROTOCOL_GetServerStatus			10049		// 查询各插件状态
#define DC_PROTOCOL_GetWeekEndTest			10050		// 查询周末测试
#define DC_PROTOCOL_GetWeekEndTest			10050		// 查询周末测试
#define DC_PROTOCOL_GetServerInfo			10051		// 查询各服务信息
#define DC_PROTOCOL_GetETFClassify 			10052		// 获取ETF分类及各分类下品种列表
#define DC_PROTOCOL_GetOptimizationList		10053		// 查询优选Etf列表（指标插件）
#define DC_PROTOCOL_GetFollowData 			10054		// 获取指数（ETF）跟踪数据
#define DC_PROTOCOL_GetIndexCaption			10055		// 获取指数解读列表
#define DC_PROTOCOL_GetCodeMapping			10056		// 获取新旧代码映射表

//命名说明：IS_插件名_REQ
#define IS_ERROR_REQ 0
#define ISHQ_REQ   1
#define ISWT_REQ   2
#define ISFILE_REQ 3
#define ISMORE_REQ 4
#define ISNEW_REQ  5
#define ISCALC_REQ 6
#define ISSTRATEGY_REQ 7
#define ISHQSTA_REQ 8
#define ISLV2_REQ 9
#define ISRPS_REQ 10
#define IS_POSITION_REQ 11
#define IS_DXJL_REQ 12
#define IS_THIRD_REQ 13
#define IS_INDATA_REQ 14
#define IS_HQSERVICE_REQ 15 //TODO 主站暂时归类 后续在细分功能 代码链等
#define IS_HQRECODE_REQ 16
#define IS_SORT_REQ 17
#define IS_INDEX_REQ 18
#define IS_MONEY_REQ 19
#define IS_GRID_REQ 20
#define IS_DATACENTER_REQ 21
#define IS_QTYIND_REQ 22
#define IS_QTY_REQ 23

//插件功能号范围
#define HQSERVICE_REQ_START		10001
#define HQSERVICE_REQ_END		10500
#define HQRECODE_REQ_START		10501
#define HQRECODE_REQ_END		11000
#define SORT_REQ_START			11001
#define SORT_REQ_END			11500
#define INDEX_REQ_START			11501
#define INDEX_REQ_END			12000
#define MONEY_REQ_START			12001
#define MONEY_REQ_END			12500
#define GRID_REQ_START			12501
#define GRID_REQ_END			13000
#define DATACENTER_REQ_START	13501
#define DATACENTER_REQ_END		14000
#define QTY_REQ_START			14001
#define QTY_REQ_END				14500

#pragma pack(push,1)

struct statichash_req //json协议
{
	char type;//数据类型
};

struct statichash_ans//静态数据hash+路径
{
	short num;
	char data[0];//num个stStaticData
};

struct staticdata_req //json协议
{
	char type;//数据类型
};

struct staticdata_ans//静态数据块
{
	char type;//数据类型
	long lbefore;//压缩前大小
	long lafter;//压缩后大小
	char data[0];//压缩的数据块,解压后是文本内容
};

struct codemapping_req //新旧代码映射请求
{
	short setcode;//市场,-1:全市场
};

struct codemapping_ans//新旧代码映射应答
{
	short num;
	char data[0];//num个ST_CODE_MAPPING
};

//接口1：判断某天是否交易日 json协议
struct tradedate_req
{
	short req;	//协议号
	short setcode;//市场
	int date;	//开始日期yymmdd
	int offset;//偏移量 0：判断date是否交易日，以date为中心偏移offset天
};

struct tradedate_ans
{
	short setcode;//市场
	int date;//日期yymmdd
	char isTradeData;//1：是交易日，0：非交易日
};

//接口2：获取一段时间非交易日期 json协议
struct nottradedate_req
{
	short req;//协议号
	short setcode;//市场
	int startDate;//开始日期
	int endDate;//结束日期
};

struct nottradedate_ans
{
	short setcode;//市场
	short num;//非交易日天数
	int data[0];//num个非交易日期
};

struct serverstatus_req//json协议
{
	char type;//服务类型
};

struct serverstatus_ans
{
	enServerType type;//服务类型
	char status;//服务状态
};

struct weekendtest_req
{
	short req;	//协议号
};

struct weekendtest_ans
{
	char isWeekendTest;//1：是，0：否
};

struct serverinfo_ans
{
	short type;//服务类型 1-转码归档
	char data[0];
};

#pragma pack(pop)

#endif // __KDSC_INTERFACE_H__
