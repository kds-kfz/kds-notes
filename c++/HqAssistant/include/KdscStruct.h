#ifndef __KDSC_STRUCT_H__
#define __KDSC_STRUCT_H__

//#include <stdlib.h>
#include "JztStructB.h"
#include <string>

//说明: 各插件公共数据接口定义

//公共市场源用定义
#define MARKETKEY_NUM 3 //市场源个数
#define MARKETKEY_SZSH "JsonBinRPCSZSH"
#define MARKETKEY_GZBJ "JsonBinRPCGZBJ"
#define MARKETKEY_CTP "JsonBinRPCCTP"

//宏
#define SERVER_NAME_NUM 9
#define SERVER_STATUS_NUM 4
#define STATIC_DATA_NUM 5

#define	VALUEADD_MONEY		"MoneyAddedValue"		// 资金增值服务名
#define VALUEADD_STKSORT	"SortAddedValue"		// 排序增值服务名
#define VALUEADD_FUND		"FundAddedValue"		// 基金增值服务名
#define VALUEADD_CORBA		"Corbastockio"			// 行情基础接口
#define VALUEADD_COMBOSEL	"StrategyProtcolResponse"// 云选股增值服务名
#define VALUEADD_RPS		"StrategyRPCResponse"	// RPS
#define VALUEADD_POSITION	"PositionManage"
#define VALUEADD_DXJL		"DxjlResponse"			// 短线精灵
#define VALUEADD_THIRD		"ThirdDataServer"
#define VALUEADD_GRIDTCP	"GridBackTestServer"	// 网格回测服务
#define VALUEADD_GOTCP		"GoTcpServer"
#define VALUEADD_INDATA		"InDataServer"			// 指标
#define VALUEADD_DATACENTER	"DataCenterServer"		// 数据中心
#define VALUEADD_HQRECODE	"WorkSZSHRPC"			// 转码
#define VALUEADD_HQSERVICE	"HisAllRPC"				// 主站
#define VALUEADD_DERIVEDATA	"DeriveDataServer"		// 衍生
#define VALUEADD_QTYINDDATA	"QtyIndDataServer"		// 量化

#pragma pack(push,1) 

//数据结构
enum enServerStatus // 服务状态
{
	enStart = 0,//启动：插件处于初始化阶段
	enStatic,	//静态：可提供静态文本数据
	enWork,		//工作：可提供所有数据、服务接口
	enBusy		//繁忙：未处理请求队列已满
};

enum enServerType//服务类型
{
	enHQRecode,
	enHService,
	enDataCenter,
	enInData,
	enGridBackTest,
	enSort,
	enMoney,
	enDerive,
	enQtyInd,
};

//静态数据信息
struct stStaticData
{
	enStaticDataType enType;		//数量类型
	unsigned __int64 ullHash;		//哈希
	bool bChange;		//是否变更
	char szDataPath[_MAX_PATH];	//数据路径
	stStaticData()
	{
		memset(this, 0, sizeof(stStaticData));
	}
};

//码表映射信息
struct ST_CODE_MAPPING
{
	short sSetCode;//市场
	unsigned long long ullNkey;//新品种标识
	char szOldCode[VAR_CODE_LEN + 1];//旧代码
	char szNewCode[VAR_CODE_LEN +1];//新代码
	ST_CODE_MAPPING()
	{
		memset(this, 0, sizeof(ST_CODE_MAPPING));
	}
};

// 量比是衡量相对成交量的指标。它是指股市开市后平均每分钟的成交量与过去5个交易日平均每分钟成交量之比。
// 其计算公式为：量比=（现成交总手数 / 现累计开市时间(分) ）/ 过去5日平均每分钟成交量.
// 量比反映出的主力行为从计算公式中可以看出，量比的数值越大，表明了该股当日流入的资金越多，市场活跃度越高

// 财务数据
struct DiskCWData
{
	unsigned __int64	nKey;		// 唯一ID号(唯一硬编码)
	short				setcode;	// 市场代码
	char				Code[VAR_CODE_LEN + 1];   // 证券代码
	double		 ActiveCapital;  // 流通股本
	double		 GrossCapital;	 // 总股本 (全为0, J_zgb 才是 总股本)
	short        J_addr;         //所属省份
	short        J_hy;           //所属行业
	short		 J_shzt;		 //上市状态
	char		 J_prov[32];	 //所属省份—新版
	char		 J_hangye[128];	 //所属行业—新版
	long         J_gxrq;         //更新日期
	long		 J_last;		 //退市日期
	double		 J_srkpj;		 //首日开盘价		-pt  2017.11.3  14:30
	double		 J_tsspj;		 //退市收盘价
	long		 J_start;		 //开市时间
	double		 J_yycb;		 //营业总成本		-新增

								 //从J_zgb到J_HalfYearFlag共计30个对象，其中他们的顺序不要变化，会在AUTOBASE2EX_EREQ协议中影响传递结果，造成错位
	double       J_zgb;          //总股本
	double       J_gjg;          //国家股
	double       J_fqrfrg;       //发起人法人股
	double       J_frg;          //法人股
	double       J_bg;           //B股
	double       J_hg;           //H股
	double       J_zgg;          //职工股
	double       J_zzc;          //总资产(千元)
	double       J_ldzc;         //流动资产
	double       J_gdzc;         //固定资产
	double       J_wxzc;         //无形资产
	double       J_cqtz;         //长期投资
	double       J_ldfz;         //流动负债
	double       J_cqfz;         //长期负债
	double       J_zbgjj;        //资本公积金
	double       J_jzc;          //股东权益(就是净资产)
	double       J_zysy;         //主营总收入
								 //通过财汇数据库查询，主营利益数据实际为营业成本数据
	double       J_zyly;         //主营利益
	double       J_qtly;         //其它利益
	double       J_yyly;         //营业利益
	double       J_tzsy;         //投资收益
	double       J_btsy;         //补贴收入
	double       J_yywsz;        //营业外收支
	double       J_snsytz;       //上年损益调整
	double       J_lyze;         //利益总额
	double       J_shly;         //税后利益
	double       J_jly;          //净利益
	double       J_wfply;        //未分配利益
	double       J_tzmgjz;       //调整每股净资产 物理意义:  净资产/调整后的总股本
	double		 J_HalfYearFlag; //全部更改为以月为单位
	DiskCWData()
	{
		memset(this, 0, sizeof(DiskCWData));
	}
};

struct stSettleData
{
	std::string marketkey;//市场源标识
	int date;//归档日期
};

struct ST_ETF_CLASSSIFY_DATA
{
    uint64_t ulKeys;
};

// DTM 2024/1/27 : 网格插件同步ETF分类数据新增传输结构 (2维数组)
// N1 (int) + {ST_ETF_CLASSSIFY_INDEX + ulKeys* N2 } * N1
struct ST_ETF_CLASSSIFY_INDEX     // ETF分类数据索引结构-二维数组（网络插件间同步ETF分类数据使用的结构）
{
    __int32  iDataSize;					// 标的（nKey）长度
    __int32  iClassifyType;				// 分类类型，对应 jztprotocl.pb 中定义的 BlockType
    ST_ETF_CLASSSIFY_DATA stDatas[0];	// 动态数组

    ST_ETF_CLASSSIFY_INDEX()
    {
        memset(this, 0, sizeof(ST_ETF_CLASSSIFY_INDEX));
    }
};

// ETF跟踪数据 数据交换使用结构
struct FollowData
{
    tagCodeWithNkey n_index;
    tagCodeWithNkey n_etf;

    FollowData()
    {
        memset(this, 0, sizeof(FollowData));
    }
};

// 指数解说数据 数据交换使用结构
struct ST_INDEX_CAPTION : tagCodeWithNkey
{
	int iCaptionType;			//解说类型
	ST_INDEX_CAPTION() {
		memset(this, 0, sizeof(ST_INDEX_CAPTION));
	}
};

//停牌信息
struct SuspenInfo
{
	unsigned long long lluNkey;	//标识
	short nSetCode;		//市场
	char szCode[32];	//品种
	//char szName[64];	//名称
	int	iSuspendDate;	//停牌日期
	int iResumpDate;	//复牌日期(交易日) 0 标识复牌日期未定
	SuspenInfo()
	{
		memset(this, 0, sizeof(SuspenInfo));
	}
};


#pragma pack(pop)

//变量
static char *g_strServerStatus[] = { "启动", "静态", "工作", "繁忙" };//各个插件状态, 与enServerStatus对应
static char *g_strStaticDataName[] = { "基础财务", "除权除息", "特色板块","指数成分股", "退市代码链" };//静态数据名称，与enStaticDataType对应
static char *g_strServerName[] = { "转码", "主站", "数据中心", "指标", "网格", "排序", "资金", "衍生", "量化" };//服务名称，与enServerType对应

#endif // __KDSC_STRUCT_H__