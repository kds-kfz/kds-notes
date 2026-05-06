#ifndef   __JZTSTRUCEB_H__
#define   __JZTSTRUCEB_H__

//说明：客户端应答数据结构 + 服务器公共数据结构

#include <Windows.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <ymath.h>

#define	  VAR_CODE_LEN		32
#define	  VAR_NAME_LEN		32

#define	  VAR_CODE_LENB		32
#define	  VAR_NAME_LENB		64	// 32

#define   SH_CODE_LEN      6
#define   NAME_LEN         8


const	long	FROMYEAR= 1990;
const	long	HMSMMOD	= 1000000000;
const	int		FROMYY = 10000;//年
const	int		FROMMM = 100;//月

const	int		MAX_MARKETNUM = 9;//11;//9;
const	short	CODELIST_VERSION = 1;// 正整数 目前支持的代码链最大版本

#pragma pack(push, 1)

// ************************************************************
// 从此处开始为客户端与服务器对应结构体
// 起始结构体：StockCodeType
// 终止结构体：AnalyDataB
// 注意：有些结构体是有依赖某些特定结构体，故一并放在一起。
// 后续有客户端关联的结构体在此基础上添加，保持统一。

// **客户端对应结构体**

struct bc_date
{
	unsigned short da_year; // Year - 1980
	char  da_day;  // Day of the month
	char  da_mon;  // Month (1 = Jan)
};

struct bc_time
{
	char  ti_min;  // Minutes
	char  ti_hour; // Hours
	char  ti_hund; // Hundredths of seconds
	char  ti_sec;  // Seconds
};

// 结构体名一致
union StockCodeType
{
	struct flag_t
	{
		uint64_t reserved : 1;				//保留位

		uint64_t SZAG : 1;					//A股
		uint64_t SZZS : 1;					//指数
		uint64_t SZZX : 1;					//中小企业板
		uint64_t SZCY : 1;					//创业板
		uint64_t SZBG : 1;					//B股
		uint64_t SZJJ_LOF : 1;				//开放式基金(LOF基金)
		uint64_t SZJJ_LOFFJ : 1;			//分级基金
		uint64_t SZJJ_ETF : 1;				//ETF基金
		uint64_t SZJJ_FBS : 1;				//传统封闭式证券投资基金
		uint64_t SZBOND_GZQ : 1;			//国债
		uint64_t SZBOND_TXGZQ : 1;			//贴现国债
		uint64_t SZBOND_DFZQ : 1;			//地方债
		uint64_t SZBOND_ZFZCZQ : 1;			//政府支持债券
		uint64_t SZBOND_GSZQ : 1;			//公司债
		uint64_t SZBOND_ZQGSDQGSZQ : 1;		//证券公司短期公司债券
		uint64_t SZBOND_ZQGSCJZQ : 1;		//证券公司次级债券
		uint64_t SZBOND_FGKFXGSZQ : 1;		//非公开发行公司债券
		uint64_t SZBOND_CXPZGSZQ : 1;		//创新品种公司债券
		uint64_t SZBOND_QYZQ : 1;			//企业债
		uint64_t SZBOND_BDCTZXTQYZQ : 1;	//不动产投资信托基金
		uint64_t SZBOND_ZCZCZQ : 1;			//企业资产支持证券债券
		uint64_t SZBOND_KZHZQ : 1;			//可转换债券
		uint64_t SZBOND_CXCYKZHZQ : 1;		//创新创业可转换债券
		uint64_t SZBOND_KJHGSZQ : 1;		//可交换公司债券
		uint64_t SZBOND_FGKFXKJHGSZQ : 1;	//非公开发行可交换公司债券
		uint64_t SZBOND_KZHGSZQ : 1;		//认股权和债券分离交易的可转换公司债券
		uint64_t SZBOND_JRZQ : 1;			//政策性金融债券
		uint64_t SZBOND_NOCOV : 1;		//非可转债
		uint64_t SZQZ_SUB : 1;				//A股认购权证
		uint64_t SZQZ_PUT : 1;				//A股认沽权证
		uint64_t SZQZ_BG : 1;				//B股权证
		uint64_t SZQZ_CYB : 1;				//创业板权证
		uint64_t SZQZ_AGPG : 1;				//A股配股权证
		uint64_t SZHG : 1;					//债券回购
		uint64_t SZPUB_AGZF : 1;			//A股增发
		uint64_t SZPUB_CYB : 1;				//创业板增发
		uint64_t SZYXG : 1;					//优先股
		uint64_t SZWLTP : 1;				//网络投票
		uint64_t SZMMJHGS : 1;				//投资者服务密码激活/挂失处理
		uint64_t SZHK : 1;            // 深港通

		uint64_t SHZS : 1;					//上证指数系列、中证指数系列
		uint64_t SHBOND_GZ : 1;				//国债
		uint64_t SHBOND_ZCXYHJRZQ : 1;		//政策性银行金融债
		uint64_t SHBOND_TXGZ : 1;			//记账式贴现国债
		uint64_t SHZYQ_GZZYSHG : 1;			//国债质押式回购质押券出入库
		uint64_t SHHG_BOND : 1;				//债券回售
		uint64_t SHZYQ_QYZQ : 1;			//企业债券质押券出入库 
		uint64_t SHZYQ_GSZQ : 1;			//公司债券质押券出入库
		uint64_t SHZYQ_KFLJY_KZHGSZQ : 1;	//分离交易的可转换公司债券质押券出入库
		uint64_t SHZYQ_ZQJYXKFJJ : 1;		//债券交易型开放式指数基金质押券出入库
		uint64_t SHZYQ_KZHGSZQ : 1;			//可转换公司债券质押券出入库
		uint64_t SHZYQ_DFZQ : 1;			//地方政府债券质押券出入库
		uint64_t SHZYQ_TXGZ : 1;			//记账式贴现国债质押券出入库
		uint64_t SHZYQ_ZCXYHJRZQ : 1;		//政策性银行金融债质押券出入库
		uint64_t SHBOND_KZHGSZQ_GKFX : 1;	//上市公司公开发行可转换公司债券     
		uint64_t SHBOND_KZHGSZQ_FGKFX : 1;	//上市公司非公开发行可转换公司债券     
		uint64_t SHBOND_KZHGSZQ_FLJY : 1;	//分离交易的可转换公司债券
		uint64_t SHBOND_KZHGSZQ : 1;		//可转换公司债券     
		uint64_t SHBOND_GSZQ : 1;			//公司债券                       
		uint64_t SHBOND_QYZQ : 1;			//企业债券 
		uint64_t SHBOND_ZCZQ : 1;			//资产支持证券 
		uint64_t SHBOND_ZXQYSMZQ : 1;		//中小企业私募债券、非公开发行公司债券
		uint64_t SHBOND_XDZCZQ : 1;			//信贷资产支持证券
		uint64_t SHBOND_DFZQ : 1;			//地方政府债券     
		uint64_t SHBOND_KJHGSZQ_FGKFX : 1;	//非公开发行可交换公司债券
		uint64_t SHBOND_KJHGSZQ : 1;		//可交换公司债券
		uint64_t SHZYQ_KJHGSZQ : 1;			//可交换公司债券质押券出入库     
		uint64_t SHZYQ_GKFXGSZQ : 1;		//公开发行公司债券质押券出入库
		uint64_t SHBOND_GKFXGSZQ : 1;		//公开发行公司债券交易 
		uint64_t SHBOND_FGKFXGSZQ : 1;		//非公开发行公司债券
		uint64_t SHBOND_ZQGSDQZQ : 1;		//证券公司短期债券
		uint64_t SHBOND_BGCZSMZQ : 1;		//并购重组私募债券
		uint64_t SHBOND_NOCOV : 1;		//非可转债
		uint64_t SHZG_FGKFX_KJHGSZQ : 1;	//非公开发行可交换公司债券换股
		uint64_t SHZG_KZHGSZQ : 1;			//可转换公司债券转股               
		uint64_t SHZG_KJHGSZQ : 1;			//可交换公司债券换股
		uint64_t SHZG_CXCYFGKFXKZHGSZQ : 1;	//创新创业公司非公开发行可转换公司债券转股
		uint64_t SHHG_GZ_XWTG : 1;			//国债回购(席位托管方式)
		uint64_t SHHG_QYZQ : 1;				//企业债回购(席位托管方式)
		uint64_t SHHG_GZMD : 1;				//国债买断式回购 
		uint64_t SHHG_ZQZYSHG_ZHTG : 1;		//债券质押式回购(账户托管方式)
		uint64_t SHHG_ZQZYSBJHG : 1;		//质押式报价回购      
		uint64_t SHHG_ZQZYSXYHG : 1;		//债券质押式协议回购     
		uint64_t SHHG_ZQZYSSFHG : 1;		//债券质押式三方回购
		uint64_t SHQH_GZ : 1;				//国债期货(暂停交易)
		uint64_t SHYXG_GKFX : 1;			//公开发行优先股 
		uint64_t SHYXG_FGKFX : 1;			//非公开发行优先股
		uint64_t SHJJ : 1;					//基金
		uint64_t SHJJ_FBQY : 1;				//契约型封闭式基金
		uint64_t SHJJ_LOF : 1;				//上市开放式基金
		uint64_t SHJJ_LOFFJ : 1;			//上证LOF财务分级基金交易
		uint64_t SHJJ_FBCXX : 1;			//创新型封闭式证券投资基金
		uint64_t SHJJ_ETF : 1;				//交易型开放式指数证券投资基金(标的指数为沪市指数、跨市场指数或跨境指数)
		uint64_t SHJJ_ZQETF : 1;			//债券交易型开放式指数基金
		uint64_t SHJJ_HBETF : 1;			//交易型货币市场基金
		uint64_t SHJJ_SPETF : 1;			//商品交易型开放式证券投资基金
		uint64_t SHPUB_KFSJJSGSH : 1;		//开放式基金申赎
		uint64_t SHPUB_KFSJJRG : 1;			//开放式基金认购
		uint64_t SHPUB_KFSJJTG : 1;			//开放式基金跨市场转托管
		uint64_t SHPUB_KFSJJFH : 1;			//开放式基金分红
		uint64_t SHPUB_KFSJJZH : 1;			//开放式基金基金转换
		uint64_t SHQZ : 1;					//权证(含股改权证、公司权证)
		uint64_t SHQZXQ : 1;				//权证行权
		uint64_t SHAG : 1;					//A股
		uint64_t SHKCB : 1;					//A股(科创板)
		uint64_t SHCTPZ : 1;				//存托凭证
		uint64_t SHPUB_PG : 1;				//配股
		uint64_t SHPUB_ZPG : 1;				//转配股
		uint64_t SHPUB_ZGGPG : 1;			//职工股配股
		uint64_t SHPUB_PS : 1;				//配售
		uint64_t SHPUB_CGPZQ : 1;			//持股配债
		uint64_t SHPUB_JJKM : 1;			//基金扩募
		uint64_t SHYYSG : 1;				//要约收购、现金选择权(其中706600-706999用于科创板公司要约收购、现金选择权)
		uint64_t SHPUB_SZPSPG : 1;			//按市值配售配股
		uint64_t SHPUB_SGZF : 1;			//网上按市值申购或增发    
		uint64_t SHPUB_CGZF : 1;			//持股增发
		uint64_t SHPUB_SGKZFK : 1;			//申购款或增发款
		uint64_t SHPUB_JJSG : 1;			//基金申购
		uint64_t SHPUB_SGZFPH : 1;			//网上按市值申购或增发配号
		uint64_t SHPUB_SZPS : 1;			//按市值配售
		uint64_t SHWLTP_AG : 1;				//网络投票
		uint64_t SHPUB_SZPSSG : 1;			//按市值配售申购
		uint64_t SHPUB_KZHGSZQSG : 1;		//可转换公司债券申购
		uint64_t SHPUB_KZHGSZQSGK : 1;		//可转换公司债券申购款
		uint64_t SHPUB_KZHGSZQPH : 1;		//可转换公司债券配号
		uint64_t SHPUB_JJSGK : 1;			//基金申购款
		uint64_t SHPUB_JJSGPH : 1;			//基金申购配号
		uint64_t SHPUB_SZPSPH : 1;			//按市值配售配号
		uint64_t SHPUB_GZCX : 1;			//国债承销发行
		uint64_t SHPUB_GZ : 1;				//国债分销
		uint64_t SHPUB_ZCXYHJRZQ : 1;		//政策性银行金融债券分销
		uint64_t SHPUB_DFZQ : 1;			//地方政府债券网上分销
		uint64_t SHPUB_GZLVZB : 1;			//利率招标国债预发行交易
		uint64_t SHPUB_GZJGZB : 1;			//价格招标国债预发行交易
		uint64_t SHPUB_GSZQ : 1;			//公司债券及企业债分销
		uint64_t SHPUB_GKFXGSZQ : 1;		//面向合格投资者公开发行公司债券网上分销
		uint64_t SHPUB_KJHZQSG : 1;			//可交换公司债券网上发行申购(759000-759099用于可交换公司债券网上发行申购)
		uint64_t SHPUB_KJHZQPH : 1;			//可交换公司债券网上发行配号(758000-758099用于可交换公司债券网上发行配号)
		uint64_t SHPUB_GKFXYXGSG : 1;		//公开发行优先股申购     
		uint64_t SHPUB_GKFXYXGPGPS : 1;		//公开发行优先股配股、配售
		uint64_t SHPUB_GKFXYXGSGK : 1;		//公开发行优先股申购款
		uint64_t SHPUB_GKFXYXGSGPH : 1;		//公开发行优先股申购配号
		uint64_t SHPUB_KCBPG : 1;			//科创板股票配股
		uint64_t SHPUB_KCBSG : 1;			//科创板股票网上申购
		uint64_t SHPUB_KCBSGPH : 1;			//科创板股票网上申购配号
		uint64_t SHPUB_KCBCTPZSG : 1;		//科创板存托凭证网上申购
		uint64_t SHPUB_KCBCTPZSGPH : 1;		//科创板存托凭证网上申购配号
		uint64_t SHZJQDKZ : 1;				//799970 资金前端控制自设额度应急调整代码
		uint64_t SHRZRQ_YQHZ : 1;			//799981 余券划转
		uint64_t SHRZRQ_HQHZ : 1;			//799982 还券划转
		uint64_t SHRZRQ_DBWHZ : 1;			//799983 担保物划转
		uint64_t SHRZRQ_QYHZ : 1;			//799984 券源划转
		uint64_t SHWLTP_MMFW : 1;			//799988 A股网络投票密码服务
		uint64_t SHTZZSFRZ : 1;				//799991 通过交易报盘方式为投资者办理中国结算网络服务身份认证
		uint64_t SHRZRQ_SBJS : 1;			//799993 证券金融公司转融通申报结束提醒
		uint64_t SHHGZDCX : 1;				//799996 回购指定撤销
		uint64_t SHHGZD : 1;				//799997 回购指定
		uint64_t SHZDJYCX : 1;				//799998 撤销指定
		uint64_t SHZDJY : 1;				//799999 指定交易
		uint64_t SHBZQ : 1;					//标准券(888880代码为新标准券，用于债券回购转换成标准券)
		uint64_t SHBG : 1;					//B股
		uint64_t SHWLTP_BG : 1;				//网络投票(B股)                   
		uint64_t SHWLTP_BGMMFW : 1;			//B股网络投票密码服务(现仅用939988)
		uint64_t SHBGQZ : 1;				//B股配股权证
		uint64_t SHBLOCK : 1;				//板块指数
		uint64_t SHAMV : 1;					//AMV指数
		uint64_t SHHK : 1;          // 沪港通
		uint64_t SHQQ : 1;          // 上海期权

									//海峡交易所
		uint64_t HXYSZB : 1;				//艺术主板
		uint64_t HXYSXB : 1;				//艺术新版
		uint64_t HXYPSC : 1;				//邮票收藏
		uint64_t HXQBSC : 1;				//钱币收藏
		uint64_t HXPSJK : 1;				//配售转让
		uint64_t HXZS : 1;					//海峡艺术品指数
		uint64_t HXCCQB : 1;				//茶产权版

											//广州茶叶
		uint64_t TEAZS : 1;					//茶叶指数
		uint64_t TEAGOODS : 1;				//茶叶商品

		uint64_t SZQQ : 1;				///>深圳期权

		uint64_t SHREITS : 1;			///>上海reits
		uint64_t SZREITS : 1;			///>深圳reits

		uint64_t GZLWTSAG : 1;			//股转两网公司及退市公司 A 股，400*
		uint64_t GZLWTSBG : 1;			//股转两网公司及退市公司 B 股，420*
		uint64_t GZGPSS : 1;			//股转挂牌/上市公司股票，43*、83*、87*
		uint64_t GZTSKZH : 1;			//股转退市公司可转换公司债券，404*
		uint64_t GZKZHZQ : 1;			//股转可转换公司债券，81*
		uint64_t GZYXG : 1;				//股转优先股，820*
		uint64_t GZYYSG : 1;			//股转要约收购，840*
		uint64_t GZYYGG : 1;			//股转要约回购，841*
		uint64_t GZGQJLQQ : 1;			//股转股权激励期权，850*
		uint64_t GZFXYW : 1;			//股转发行业务，889*
		uint64_t GZBJZS : 1;			//股转北交 指数，899*
	}u;
	uint64_t type[(sizeof(flag_t) - 1) / sizeof(uint64_t) + 1];

	bool operator & (const StockCodeType& other) const
	{
		for (int i = 0; i < _countof(type); i++)
		{
			if (type[i] & other.type[i]) return true;
		}
		return false;
	}

	bool operator == (const StockCodeType& other) const
	{
		for (int i = 0; i < _countof(type); i++)
		{
			if (type[i] != other.type[i]) return false;
		}
		return true;
	}
	///>上海A股
	bool IsShAg()
	{
		return u.SHAG;
	}
	void SetShAg()
	{
		u.SHAG = 1;
	}
	///>上海B股
	bool IsShBg()
	{
		return u.SHBG;
	}
	void SetShBg()
	{
		u.SHBG = 1;
	}
	///>深圳A股
	bool IsSzAg()
	{
		return u.SZAG;
	}
	void SetSzAg()
	{
		u.SZAG = 1;
	}
	bool IsShKcb()
	{
		return u.SHKCB;
	}
	void SetShKcb()
	{
		u.SHKCB = 1;
	}
	///>深圳B股
	bool IsSzBg()
	{
		return u.SZBG;
	}
	void SetSzBg()
	{
		u.SZBG = 1;
	}
	///>上海债券
	bool IsShBoud()
	{
		return u.SHBOND_GZ ||
			u.SHBOND_ZCXYHJRZQ ||
			u.SHBOND_TXGZ ||
			u.SHBOND_KZHGSZQ_GKFX ||
			u.SHBOND_KZHGSZQ_FGKFX ||
			u.SHBOND_KZHGSZQ_FLJY ||
			u.SHBOND_KZHGSZQ ||
			u.SHBOND_GSZQ ||
			u.SHBOND_QYZQ ||
			u.SHBOND_ZCZQ ||
			u.SHBOND_ZXQYSMZQ ||
			u.SHBOND_XDZCZQ ||
			u.SHBOND_DFZQ ||
			u.SHBOND_KJHGSZQ_FGKFX ||
			u.SHBOND_KJHGSZQ ||
			u.SHBOND_GKFXGSZQ ||
			u.SHBOND_FGKFXGSZQ ||
			u.SHBOND_ZQGSDQZQ ||
			u.SHBOND_BGCZSMZQ;
	}
	void SetShBoud()
	{
		u.SHBOND_GZ = 1;
		u.SHBOND_ZCXYHJRZQ = 1;
		u.SHBOND_TXGZ = 1;
		u.SHBOND_KZHGSZQ_GKFX = 1;
		u.SHBOND_KZHGSZQ_FGKFX = 1;
		u.SHBOND_KZHGSZQ_FLJY = 1;
		u.SHBOND_KZHGSZQ = 1;
		u.SHBOND_GSZQ = 1;
		u.SHBOND_QYZQ = 1;
		u.SHBOND_ZCZQ = 1;
		u.SHBOND_ZXQYSMZQ = 1;
		u.SHBOND_XDZCZQ = 1;
		u.SHBOND_DFZQ = 1;
		u.SHBOND_KJHGSZQ_FGKFX = 1;
		u.SHBOND_KJHGSZQ = 1;
		u.SHBOND_GKFXGSZQ = 1;
		u.SHBOND_FGKFXGSZQ = 1;
		u.SHBOND_ZQGSDQZQ = 1;
		u.SHBOND_BGCZSMZQ = 1;
	}
	///>深圳债券
	bool IsSzBoud()
	{
		return u.SZBOND_GZQ ||
			u.SZBOND_TXGZQ ||
			u.SZBOND_DFZQ ||
			u.SZBOND_ZFZCZQ ||
			u.SZBOND_GSZQ ||
			u.SZBOND_ZQGSDQGSZQ ||
			u.SZBOND_ZQGSCJZQ ||
			u.SZBOND_FGKFXGSZQ ||
			u.SZBOND_CXPZGSZQ ||
			u.SZBOND_QYZQ ||
			u.SZBOND_BDCTZXTQYZQ ||
			u.SZBOND_ZCZCZQ ||
			u.SZBOND_KZHZQ ||
			u.SZBOND_CXCYKZHZQ ||
			u.SZBOND_KJHGSZQ ||
			u.SZBOND_FGKFXKJHGSZQ ||
			u.SZBOND_KZHGSZQ ||
			u.SZBOND_JRZQ;
	}
	void SetSzBoud()
	{
		u.SZBOND_GZQ = 1;
		u.SZBOND_TXGZQ = 1;
		u.SZBOND_DFZQ = 1;
		u.SZBOND_ZFZCZQ = 1;
		u.SZBOND_GSZQ = 1;
		u.SZBOND_ZQGSDQGSZQ = 1;
		u.SZBOND_ZQGSCJZQ = 1;
		u.SZBOND_FGKFXGSZQ = 1;
		u.SZBOND_CXPZGSZQ = 1;
		u.SZBOND_QYZQ = 1;
		u.SZBOND_BDCTZXTQYZQ = 1;
		u.SZBOND_ZCZCZQ = 1;
		u.SZBOND_KZHZQ = 1;
		u.SZBOND_CXCYKZHZQ = 1;
		u.SZBOND_KJHGSZQ = 1;
		u.SZBOND_FGKFXKJHGSZQ = 1;
		u.SZBOND_KZHGSZQ = 1;
		u.SZBOND_JRZQ = 1;
	}
	///>沪深A股
	bool IsHSAg()
	{
		return u.SHAG || u.SZAG;
	}
	void SetHSAg()
	{
		u.SHAG = u.SZAG = 1;
	}
	///>沪深B股
	bool IsHSBg()
	{
		return u.SHBG || u.SZBG;
	}
	void SetHSBg()
	{
		u.SHBG = u.SZBG = 1;
	}
	///>沪深债券
	bool IsHsBoud()
	{
		return IsSzBoud() || IsShBoud();
	}
	void SetHsBoud()
	{
		SetSzBoud();
		SetShBoud();
	}
	bool IsETF()
	{
		return u.SHJJ_ETF ||
			u.SZJJ_ETF ||
			u.SHJJ_ZQETF ||
			u.SHJJ_HBETF ||
			u.SHJJ_SPETF;
	}
	///>上海基金
	bool IsShJj()
	{
		return u.SHJJ_FBQY ||
			u.SHJJ_LOF ||
			u.SHJJ_LOFFJ ||
			u.SHJJ_ETF ||
			u.SHJJ_HBETF ||
			u.SHJJ_ZQETF;
	}
	void SetShJj()
	{
		u.SHJJ_FBQY = 1;
		u.SHJJ_LOF = 1;
		u.SHJJ_LOFFJ = 1;
		u.SHJJ_ETF = 1;
		u.SHJJ_HBETF = 1;
		u.SHJJ_ZQETF = 1;
	}
	///>深圳基金
	bool IsSzJj()
	{
		return u.SZJJ_LOF ||
			u.SZJJ_LOFFJ ||
			u.SZJJ_ETF ||
			u.SZJJ_FBS;
	}
	void SetSzJj()
	{
		u.SZJJ_LOF = 1;
		u.SZJJ_LOFFJ = 1;
		u.SZJJ_ETF = 1;
		u.SZJJ_FBS = 1;
	}
	///>沪深基金
	bool IsHsJj()
	{
		return IsShJj() || IsSzJj();
	}
	void SetHsJj()
	{
		SetShJj();
		SetSzJj();
	}
	///>所有指数
	bool IsZs()
	{
		return u.SHZS ||
			u.SZZS ||
			u.SHBLOCK ||
			u.SHAMV ||
			u.GZBJZS;
	}
	void SetZs()
	{
		u.SHZS = 1;
		u.SZZS = 1;
		u.SHBLOCK = 1;
		u.SHAMV = 1;
	}
	///>中小
	bool IsZx()
	{
		return u.SZZX;
	}
	void SetZx()
	{
		u.SZZX = 1;
	}
	///>创业
	bool IsCy()
	{
		return u.SZCY;
	}
	void SetCy()
	{
		u.SZCY = 1;
	}
	//创业板和科创板
	bool IsKCBCY()
	{
		return u.SZCY || u.SHKCB;
	}
	///>上海可转债
	bool IsShKZZ()
	{
		return u.SHBOND_KZHGSZQ_GKFX ||
			u.SHBOND_KZHGSZQ_FGKFX ||
			u.SHBOND_KZHGSZQ_FLJY ||
			u.SHBOND_KZHGSZQ;
	}
	void SetShKZZ()
	{
		u.SHBOND_KZHGSZQ_GKFX = 1;
		u.SHBOND_KZHGSZQ_FGKFX = 1;
		u.SHBOND_KZHGSZQ_FLJY = 1;
		u.SHBOND_KZHGSZQ = 1;
	}
	///>深圳可转债
	bool IsSzKZZ()
	{
		return u.SZBOND_KZHZQ ||
			u.SZBOND_CXCYKZHZQ ||
			u.SZBOND_KJHGSZQ ||
			u.SZBOND_FGKFXKJHGSZQ ||
			u.SZBOND_KZHGSZQ;
	}
	void SetSzKZZ()
	{
		u.SZBOND_KZHZQ = 1;
		u.SZBOND_CXCYKZHZQ = 1;
		u.SZBOND_KJHGSZQ = 1;
		u.SZBOND_FGKFXKJHGSZQ = 1;
		u.SZBOND_KZHGSZQ = 1;
	}
	///>可转债
	bool IsKZZ()
	{
		return IsShKZZ() || IsSzKZZ();
	}
	void SetKZZ()
	{
		SetShKZZ();
		SetSzKZZ();
	}
	///>上海回购
	bool IsShHg()
	{
		return u.SHHG_BOND ||
			u.SHHG_GZ_XWTG ||
			u.SHHG_GZMD ||
			u.SHHG_QYZQ ||
			u.SHHG_ZQZYSHG_ZHTG ||
			u.SHHG_ZQZYSBJHG ||
			u.SHHG_ZQZYSXYHG ||
			u.SHHG_ZQZYSSFHG;
	}
	void SetShHg()
	{
		u.SHHG_BOND = 1;
		u.SHHG_GZ_XWTG = 1;
		u.SHHG_GZMD = 1;
		u.SHHG_QYZQ = 1;
		u.SHHG_ZQZYSHG_ZHTG = 1;
		u.SHHG_ZQZYSBJHG = 1;
		u.SHHG_ZQZYSXYHG = 1;
		u.SHHG_ZQZYSSFHG = 1;
	}
	///>深圳回购
	bool IsSzHg()
	{
		return u.SZHG;
	}
	void SetSzHg()
	{
		u.SZHG = 1;
	}
	///>回购
	bool IsHg()
	{
		return IsShHg() || IsSzHg();
	}
	void SetHg()
	{
		SetShHg();
		SetSzHg();
	}
	bool IsSzBondNoCov()
	{
		return u.SZBOND_NOCOV;
	}
	bool IsShBondNoCov()
	{
		return u.SHBOND_NOCOV;
	}
	///>上海权证
	bool IsShQz()
	{
		return u.SHQZ;
	}
	void SetShQz()
	{
		u.SHQZ = 1;
	}
	///>深圳权证
	bool IsSzQz()
	{
		return u.SZQZ_SUB ||
			u.SZQZ_PUT ||
			u.SZQZ_BG ||
			u.SZQZ_CYB ||
			u.SZQZ_AGPG;
	}
	void SetSzQz()
	{
		u.SZQZ_SUB = 1;
		u.SZQZ_PUT = 1;
		u.SZQZ_BG = 1;
		u.SZQZ_CYB = 1;
		u.SZQZ_AGPG = 1;
	}
	bool IsSHEtf()
	{
		return u.SHJJ_ETF ||
			u.SHJJ_HBETF ||
			u.SHJJ_SPETF ||
			u.SHJJ_ZQETF;
	}
	bool IsSZEtf()
	{
		return u.SZJJ_ETF;
	}
	void SetSHEtf()
	{
		u.SHJJ_ETF = u.SHJJ_HBETF = u.SHJJ_SPETF = u.SHJJ_ZQETF = 1;
	}
	void SetSZEtf()
	{
		u.SZJJ_ETF = 1;
	}
	void SetEtf()
	{
		SetSZEtf();
		SetSHEtf();
	}
	bool IsGGT()
	{
		return IsSGT() || IsHGT();
	}
	bool IsSGT() 
	{
		return u.SZHK;
	}
	bool IsHGT() 
	{
		return u.SHHK;
	}

	StockCodeType & operator |= (const StockCodeType& other)
	{
		for (int i = 0; i < _countof(type); i++)
		{
			type[i] |= other.type[i];
		}
		return *this;
	}

	StockCodeType & operator &= (const StockCodeType& other)
	{
		for (int i = 0; i < _countof(type); i++)
		{
			type[i] &= other.type[i];
		}
		return *this;
	}
};


// **客户端对应结构体**
// 结构体名一致
typedef struct
{
	short      Minute;
	float      Now;
	unsigned long NowAmount;	//单笔成交金额(百元)
	long	   NowVol;
	float      BuyP[3];
	long       BuyV[3];
	float      SellP[3];
	long       SellV[3];
	char       InOutFlag;
	char       MaxFlag;//最高价标志 0：最高价在前面已出现或者Now即为最高价1:最高价存放于Max中
	char       MinFlag;
	float      Max;
	float      Min;
	DWORD      CJBSDIF;
}TICK_BSP;


// **客户端对应结构体** 
// 精简行情结构
// 结构体名一致
struct simhq_Info
{
	float          Open;                  // 今开盘价,指数
	float          Max;                   // 最高价,指数
	float          Min;                   // 最低价,指数
	float          Now;                   // 现价,最近指数
	DWORD          Volume;                // 总手
	float          Amount;                // 总成交金额  
};


// **客户端对应结构体** 
// 14协议号
// 对应结构体: BaseInfo
struct remote_BaseInfo	//Protocol Change
{
	char		setcode;			//市场类型
	char        Code[SH_CODE_LEN];  // 证券代码
	float       ActiveCapital;      //流通股本
	short       J_addr;				//所属省份
	short       J_hy;				//所属行业
	long        J_gxrq;				//更新日期
	long		J_start;			//上市日期
	float       J_zgb;				//总股本
	float       J_gjg;				//国家股
	float       J_fqrfrg;			//发起人法人股
	float       J_frg;				//法人股
	float       J_bg;				//B股
	float       J_hg;				//H股
	float       J_zgg;				//职工股
	float       J_zzc;				//总资产(千元)
	float       J_ldzc;				//流动资产
	float       J_gdzc;				//固定资产
	float       J_wxzc;				//无形资产
	float       J_cqtz;				//长期投资
	float       J_ldfz;				//流动负债
	float       J_cqfz;				//长期负债
	float       J_zbgjj;			//资本公积金
	float       J_jzc;				//股东权益(就是净资产)
	float       J_zysy;				//主营收入
	float       J_zyly;				//主营利益
	float       J_qtly;				//其它利益
	float       J_yyly;				//营业利益
	float       J_tzsy;				//投资收益
	float       J_btsy;				//补贴收入
	float       J_yywsz;			//营业外收支
	float       J_snsytz;			//上年损益调整
	float       J_lyze;				//利益总额
	float       J_shly;				//税后利益
	float       J_jly;				//净利益
	float       J_wfply;			//未分配利益
	float       J_tzmgjz;			//调整每股净资产 物理意义:  净资产/调整后的总股本
};

struct HeartInfo
{
	int iDate;
	int iTime;
	char chMsg[5];
	HeartInfo()
	{
		memset(this, 0, sizeof(HeartInfo));
	}
};


// **客户端对应结构体** 
//21 内部窥视信息
// 结构体名一致
struct monitorinfo
{
	int  ClientNum;		//当前在线人数
	int  MaxConnectNum;
	long PackageNum;
	char HasStatus;
	char HasLog;
	char bHQ;
	char bWT;
	char starttime[25];

	char HostVer[30];
	char ProtocolVer;
	long HisMaxClientNum;//当前总在线人数
	long UsedClientNum;	//从开启主站到现在有多少人登陆过
	char bAutoBase;
	char HomePath[255];
	char NetCardStr[20];
	long InfDate;
	long InfHms;
	char HostType;		//服务器类型 独立，子，主
	char ProcType;		//0:批作业方式 1:多线程方式
	char ZlibType;		//压缩方式 0:自动优化 1:完全不压缩 2:最大限度压缩
	char bDayToMem;
	char bWeekToMem;
	char bMonToMem;
	char bMinToMem;
	char bKExplain;			//是不是成功地启动了选股等服务
	char bAlarm;
	char bHqStat;
	char bCanOldFy;		//是不是允许下列拨入
	char bCanDos;
	char bCanPC;
	char bCanWeb;
	char bCanPda;
	char bCanWide;
	char bCanPlay;
	char bCanJdh;

	char bTapi;
	char bUdp;
	char bChat;
	short RecSocket;
	short UdpSocket;

	short ProcessNum;	//进程数
	short ThreadNum;	//线程数
	short unused1;

	char  TicFlag;		//TIC标志
	char  MtcFlag;		//MTC标志
	char  unused2[30];
};

//
struct hqRecordInitinfo
{
	// 保留字段
	char szReserver[512];
};

enum enStaticDataType//数据类型
{
	enAll = -1,
	enBasecw,
	enCqcx,
	enSHBlock,
	enIndex,
	enDelist
};

struct StaticDataBase
{
	enStaticDataType enType;		//类型
	unsigned __int64 ullHash;		//哈希
};

struct StaticDataInfo//推送给客户端
{
	short num;		//个数
	char data[0];//cNum个StaticDataBase
};

// **客户端对应结构体** 
// 1104：代码链请求
// 对应结构体：StkInfoOld
struct remote_TmpStkInfo3
{
	char           Code[SH_CODE_LEN];     // 证券代码
	short          Unit;                  // 交易单位
	char           Name[NAME_LEN];        // 证券名称
	long           VolBase;               // 量比的基量
	char           precise;               //停牌标志
	float		   Close;				  //昨收
										  //	char           flflag;                // 股票分类
	short		   BaseFreshCount;		  //基本资料的更新次新
	short		   GbbqFreshCount;		  //股本变迁的更新次新
};


// **客户端对应结构体** 
// 1106：代码链请求
// 对应结构体：StkInfoEx
struct remote_TmpStkInfo5
{
	char			Code[VAR_CODE_LEN];		// 证券代码
	char			Name[VAR_NAME_LEN];		// 证券名称
	long			Unit;				// 交易单位
	long			VolBase;			// 量比的基量
	short			precise;		// 停牌
	short			main;			// 是否主力合约
	double			Close;				// 昨收
	double			Settle;				// 昨结
	double			Tick;				// 最小变动价
	int64_t			nkey;				// 内部编码
	short			fz[8];				// 开收盘时间段

	int16_t		   BaseFreshCount;		// 基本资料的更新次新
	int16_t		   GbbqFreshCount;		// 股本变迁的更新次新
};

/*****
1 除权除息
数据1  分红
数据2  配股价
数据3  送转股
数据4  配股
2 送配股上市
数据1  前流通盘
数据2  前总股本
数据3  后流通盘
数据4  后总股本
3 非流通股上市
数据1  前流通盘
数据2  前总股本
数据3  后流通盘
数据4  后总股本
4 未知股本变动 公众号协议分析与还原
数据1   0
数据2   0
数据3   0
数据4   0
5 股本变化
数据1  前流通盘
数据2  前总股本
数据3  后流通盘
数据4  后总股本
6 增发新股
数据1  0
数据2  增发价
数据3  增发数量
数据4  0
7 股份回购
数据1  前流通盘
数据2  前总股本
数据3  后流通盘
数据4  后总股本
8 增发新股上市
数据1  前流通盘
数据2  前总股本
数据3  后流通盘
数据4  后总股本
9 转配股上市
数据1  前流通盘
数据2  前总股本
数据3  后流通盘
数据4  后总股本
10 可转债上市 protosec
数据1  前流通盘
数据2  前总股本
数据3  后流通盘
数据4  后总股本
11 扩缩股
数据1  0
数据2  0
数据3  比例
数据4  0
12 非流通股缩股
数据1  0
数据2  0
数据3  比例
数据4  0
13 送认购权证
数据1  行权价
数据2  0
数据3  份数
数据4  0
14 送认沽权证
数据1  行权价
数据2  0
数据3  份数
数据4  0
*****/
//除权除息
struct CWDATA
{
	char	Setcode;	//新版数据才有此字段
	char 	Code[7];
	long    Date;		//日期：年月日
	char    Type;		//数据类别，1-14
	float   B01;		//数据1
	float   B02;		//数据2
	float   B03;		//数据3
	float   B04;		//数据4
};

// 复权因子
// price(复权后) = price(除权价) * dFa + dFb
struct  ST_TQ_FACTOR
{
	long Date;  // 日期：年月日
	double dFa; // 乘积系数
	double dFb; // 加减系数
};

// **客户端对应结构体**
// 结构体名一致
struct  PriceVolTable
{
	DWORD Price;
	double Volume;
	double Kcl;
	double Pcl;
};


// **客户端对应结构体**
// 结构体名一致
typedef struct
{
	int		Num;
	char	SetCode[20];
	char	Code[20][7];
	float	Now[20];
	float	Other[20];
}ZHRESULT, *LPZHRESULT;



// ************************************************************
// **客户端对应结构体**
union long_short
{
	long    Date;                     // 日期YYYYMMDD(日线)

	struct
	{
		WORD Minute : 16;//11;//16;         // 零点以来的分钟数
		WORD Mon_Day : 11;        //月日 2048
		WORD Year : 5;//10;// +1900 年,到2024年 //5;   //年, 离2004的年数
	}Daye1;
};

// ************************************************************
// **客户端对应结构体** 
// 结构体名一致
struct tagCode
{
	char setcode;
	char code[VAR_CODE_LEN];

	bool operator < (const tagCode& other) const
	{
		if (setcode == other.setcode)
			return strncmp(code, other.code, VAR_CODE_LEN) < 0;
		return setcode < other.setcode;
	}

	bool operator == (const tagCode& other) const
	{
		return setcode == other.setcode && !strncmp(code, other.code, VAR_CODE_LEN);
	}

	bool operator != (const tagCode& other) const
	{
		return !(*this == other);
	}

	tagCode()
	{
		memset(this, 0, sizeof(tagCode));
	}
};

struct tagCodeWithNkey : public tagCode
{
	int64_t nkey;

	tagCodeWithNkey()
	{
		memset(this, 0, sizeof(tagCodeWithNkey));
	}
};

// *********************************************************
// **客户端对应结构体**
// 结构体名一致
struct	CalcZafData
{
	short setcode; //0:SZ 1:SH
	unsigned __int64 nKey;

	double zaf_5min;		// 5分钟涨幅
	double zaf_3days;		// 3日涨幅(交易日)
	double zaf_5days;		// 5日涨幅(交易日)
	double zaf_20days;		// 20日涨幅(交易日)
	double zang_days;		// 连涨天数 
	double die_days;		// 连跌天数

	double high_20days;		// 20日最高价
	double low_20days;		// 20日最低价
	double high_his;		// 历史最高价
	double low_his;			// 历史最低价

	double zaf_90days;		// 90日(三月)涨幅(自然日)
	double zaf_180days;		// 180日(半年)涨幅(自然日)
	double zaf_365days;		// 365日(一年)涨幅(自然日)

	double zb_state[5];		// 指标对应输出结果，目前需要两个
	CalcZafData()
	{
		setcode = nKey = 0;
		zaf_5min = zaf_3days = zaf_5days = zaf_20days = high_20days = low_20days = high_his = low_his = zaf_90days = zaf_180days = zaf_365days = _Nan._Double;
		zang_days = die_days = _Nan._Double;
		zb_state[0] = zb_state[1] = zb_state[2] = zb_state[3] = zb_state[4] = _Nan._Double;
	}
};


// *********************************************************
// **客户端对应结构体** 

// 大数据采用google快速压缩
// 量比是衡量相对成交量的指标。它是指股市开市后平均每分钟的成交量与过去5个交易日平均每分钟成交量之比。
// 其计算公式为：量比=（现成交总手数 / 现累计开市时间(分) ）/ 过去5日平均每分钟成交量.
// 量比反映出的主力行为从计算公式中可以看出，量比的数值越大，表明了该股当日流入的资金越多，市场活跃度越高
union	UniKey
{
	unsigned __int64	nKey;			// 唯一hash值（0-0的块是无效块)
	struct
	{
		DWORD	LowKey;		// 低位
		DWORD	HighKey;	// 高位
	}dwHLKey;
};

//014 代码链
struct DiskStkInfoB         // 用于在.INF文件中定位的临时数据结构
{
	unsigned __int64	nKey;									// 唯一ID号(唯一硬编码)

																//
	short					SetCode;									// 市场代码
	char					Code[VAR_CODE_LEN + 1];   					// 证券代码
	char					Name[VAR_NAME_LENB + 1];						// 证券名称
	StockCodeType	StkType;											// 证券类型 
	long					startDate;									// 上市日期 	
	__int64				jclTime;										// 更新时间
	short					nFZ[8];										// 交易时间

																		//
	char					XsFlag;										// 小数点个数
	long					StopFlag;									// 停牌标志
	int						volUnit;									// 成交量倍数
	int						handUnit;									// 每股手数

	int						ZzgzMarketCode;							//中证国证市场号
	int						ZzgzFcFlag;								//上海中证、深圳国证、股转北交分层标志

	double				VolBase;									// 量比的基量(前5天的量比)   运用资金，可以正负； 比特币可以是小数
	double				YClose;										// 昨日收盘价
	double				YSettle;									// 昨日结算价
	double				AdvStop;									// 涨停价
	double				DecStop;									// 跌停价	

																	// 14版本新增交易信息
	double			  TradeUnit;								// 数量单位

	double				LmtPriceQtyUp;						// 限价申报数量上限
	double				LmtPriceQtyDown;					// 限价申报数量下限
	double				MktPriceQtyUp;						// 市价申报数量上限
	double				MktPriceQtyDown;					// 市价申报数量下限

															// 注册制新增
	char				  noProfit;									// 是否尚未盈利  上海: ‘U’表示上市时尚未盈利 深圳: 'Y/N'
	char					VotRight;									// 是否表决权	  上海: ‘W’表示具有表决权差异安排的发行人的股票或存托凭证 深圳: 'Y/N'
	char					isReg;										// 是否注册制	  上海: ‘Y’表示注册制 深圳: 'Y/N'
	char          isVIE;										// 是否控制架构	  上海: 无, 深圳:'Y/N' 
																/* 涨跌幅限制类型:
																上海: ‘N’表示有涨跌幅限制类型,‘R’表示无涨跌幅限制类型,‘S’表示回购涨跌幅控制类型,‘F’表示基于参考价格的涨跌幅控制,‘P’表示IPO上市首日的涨跌幅控制类型,‘U’表示无任何价格涨跌幅控制类型
																深圳: 17-P 8-R(恢复上市首日) 10+18-R(退市整理期首日)
																ExpirationDays>0 - S
																*/
	char					AdvDecType;
	char					proStatus;								/*上海: 第29位产品状态标志4位：D’表示国内正常交易产品，’S’表示股票风险警示产品，’P’表示退市整理产品（主板），T’表示退市转让产品，’U’表示优先股产品。
																	深圳：证券状态SecurityStatus：4-ST、5-*ST -->上海的S 10-退市整理期-->上海的P */

	char					dayTrading;								// 是否支持当日回转交易, Y 支持, N 不支持

																	//
	DiskStkInfoB()
	{
		memset(this, 0, sizeof(DiskStkInfoB));
	}
	unsigned __int64	getKey()
	{
		return nKey;
	}
	// 统一硬编码  nKey = GenKey(Code)
	static unsigned __int64 GenKey(const char* str)
	{
		UniKey	uk;
		unsigned int len = strlen(str);
		uk.dwHLKey.LowKey = GenHash(str, len, 31);
		uk.dwHLKey.HighKey = GenHash(str, len, 131);
		return uk.nKey;
	}
	static unsigned __int64 GenKey(const char* str, unsigned int len)
	{
		UniKey	uk;
		uk.dwHLKey.LowKey = GenHash(str, len, 31);
		uk.dwHLKey.HighKey = GenHash(str, len, 131);
		return uk.nKey;
	}
	// sprintf(szkey,"%s%s",GetSetStr(setcode),head[setcode][j].Code);
	// sprintf_s(szkey,"%d%s",pNode->SetCode,pNode->Code);
	// 改成下面一种生成Key，数字避免了setcodestr的大小写问题
	static unsigned __int64	GenKey(int setcode, const char* code)
	{
		char szkey[100] = { 0 };
		_snprintf_s(szkey, 99, "%d%s", setcode, code);
		return GenKey(szkey);
	}
	template<size_t N>
	static unsigned __int64	GenKey(int setcode, char code[N])
	{
		char szkey[100] = { 0 }, fmt[32] = { 0 };
		snprintf(fmt, 31, "%%d%%-.%ds", N);
		_snprintf_s(szkey, 99, fmt, setcode, code);
		return GenKey(szkey);
	}
	// BKDRHash : 固定标准hash算法
	static unsigned int GenHash(const char* str, unsigned int len, unsigned int seed)
	{
		//unsigned int seed = 131; /* 31 131 1313 13131 131313 etc.. */
		unsigned int hash = 0;
		unsigned int i = 0;

		for (i = 0; i < len; str++, i++)
		{
			hash = (hash * seed) + (*str);
		}

		return hash;
	}
	// 统一时间格式	20171025 103000 000
	// const	long	HMSMMOD	= 100 00 00 000;
	// 值介于 -2^63 ( -9,223,372,036,854,775,808) 到2^63-1(+9,223,372,036,854,775,807 )之间的整数
	static	long	GetDate(__int64				jclTime)
	{
		return jclTime / HMSMMOD;
	}
	static	long	GetYear(__int64				jclTime)
	{
		return (jclTime / HMSMMOD) / 10000;
	}
	static	long	GetMonth(__int64				jclTime)
	{
		return ((jclTime / HMSMMOD) % 10000) / 100;
	}
	static long		GetMonthDay(__int64				jclTime)
	{
		return  ((jclTime / HMSMMOD) % 10000);
	}
	static	long	GetDay(__int64				jclTime)
	{
		return (jclTime / HMSMMOD) % 100;
	}
	static	long	GetHour(__int64				jclTime)
	{
		return (jclTime%HMSMMOD) / 10000000;
	}
	static	long	GetMinute(__int64				jclTime)
	{
		return ((jclTime%HMSMMOD) / 100000) % 100;
	}
	static	long	GetSecond(__int64				jclTime)
	{
		return (jclTime % 100000) / 1000;
	}
	static long		GetHourMinute(__int64				jclTime)
	{
		return (jclTime%HMSMMOD) / 100000;
	}
	static	long	GetMSecond(__int64				jclTime)
	{
		return (jclTime % 1000);
	}
	static	long	GetTime(__int64				jclTime)
	{
		return jclTime%HMSMMOD;
	}
	static	__int64	GetJclTime(long date, long time, short sCutMode = 0)
	{
		// cutMode = 0 无截断     yyyyMMdd hhmmsszzz
		// cutMode = 1 截断至分钟 yyyyMMdd hhmm00000
		// cutMode = 2 截断至日期 yyyyMMdd 000000000
		switch (sCutMode)
		{
		case 0:
			return (__int64)date * HMSMMOD + time;
			break;
		case 1:
			return (__int64)date * HMSMMOD + time / (100 * 1000) * (100 * 1000);
			break;
		case 2:
			return (__int64)date * HMSMMOD;
			break;
		default:
			return (__int64)date * HMSMMOD + time;
			break;
		}
	}
	static long		GetHms(__int64				jclTime)
	{
		return (jclTime%HMSMMOD) / 1000;
	}
	static	long	GetMinutes(__int64				jclTime)
	{
		long	hhmm = (jclTime%HMSMMOD) / (100 * 1000);//100000;	// 1000000000
		return	(hhmm / 100) * 60 + (hhmm % 100);
	}
	static	long	GetSeconds(__int64				jclTime)
	{
		long	hhmmss = (jclTime%HMSMMOD) / (100 * 10);//100000;	// 1000000000
		return	(hhmmss / 10000) * 60 * 60 + (hhmmss / 100 % 100) * 60 + (hhmmss % 100);
	}
	static	__int64	SetMinites(__int64	jclTime, int  nMinutes)
	{
		long	hhmm = (nMinutes / 60) * 10000 + (nMinutes % 60) * 100;
		jclTime = (jclTime / HMSMMOD)*HMSMMOD + hhmm * 1000;
		return jclTime;
	}
	static __int64 IncMinute(__int64 jclTime)	//分钟加1
	{
		__int64 day = jclTime / HMSMMOD;
		int minute = ((jclTime%HMSMMOD) / 100000) % 100;
		int hour = (jclTime%HMSMMOD) / 10000000;
		minute++;
		if (minute >= 60)
		{
			minute = 0;
			hour++;
		}
		jclTime = day*HMSMMOD + ((__int64)(hour * 10000 + minute * 100)) * 1000;
		return jclTime;
	}
	static	__int64	time2i64(__time64_t		t)
	{
		tm tmtm;
		__int64	i64T = 0;
		_localtime64_s(&tmtm, &t);

		i64T = ((tmtm.tm_year + 1900) * 10000 + (tmtm.tm_mon + 1) * 100 + tmtm.tm_mday);// year -2004???	
		i64T = i64T * HMSMMOD + (tmtm.tm_hour * 10000 + tmtm.tm_min * 100 + tmtm.tm_sec) * 1000;
		return i64T;
	}
	static time_t jcl2time(__int64 jclTime)
	{
		struct tm stm;
		stm.tm_year = DiskStkInfoB::GetYear(jclTime) - 1900;
		stm.tm_mon = DiskStkInfoB::GetMonth(jclTime) - 1;
		stm.tm_mday = DiskStkInfoB::GetDay(jclTime);
		stm.tm_hour = DiskStkInfoB::GetHour(jclTime);
		stm.tm_min = DiskStkInfoB::GetMinute(jclTime);
		stm.tm_sec = DiskStkInfoB::GetSecond(jclTime);
		return mktime(&stm);
	}
	static	__int64	time2i64(__time32_t		t)
	{
		tm tmtm;
		__int64	i64T = 0;
		_localtime32_s(&tmtm, &t);

		SYSTEMTIME st;
		GetSystemTime(&st);	// 用本机的毫秒数（time_t不支持毫秒)
		i64T = ((tmtm.tm_year + 1900) * 10000 + (tmtm.tm_mon + 1) * 100 + tmtm.tm_mday);// year -2004???	
		i64T = i64T * HMSMMOD + (tmtm.tm_hour * 10000 + tmtm.tm_min * 100 + tmtm.tm_sec) * 1000;  //+st.wMilliseconds;  cjf--加本机毫秒数不对
		return i64T;
	}
};

// 009版本新增协议号对应结构体，协议号：1112
struct ST_ADDCODE
{
	short				setcode;
	char				chCode[VAR_CODE_LENB + 1];   // 证券代码
	char				chName[VAR_NAME_LENB + 1];      // 证券名称
	double				dYClose;                 // 昨日收盘价

	ST_ADDCODE()
	{
		memset(this, 0, sizeof(ST_ADDCODE));
	}
};

// **客户端对应结构体**
// 对应结构体名：AnalyData_server
typedef struct AnalyDataB  // 在.IFZ文件中读入的历史数据记录
{
	DWORD			dwItemNum;               // 采样点数 : 0 表示这个周期内，没有行情变动
	__int64			jclTime;				 // 时间精确到毫秒
											 // 存盘数据是long，保证精度；展示可以用float(如有特殊精度要求，用double)
	double			fOpen;	// 单位开盘价
	double			fHigh;	// 单位最高价
	double			fLow;		// 单位最低价
	double			fClose;	// 单位收盘价
	double			fAmount;                          // 单位均价(分钟线/期货)			
													  // 单位成交金额(日线/指数)
	union { double	dVolume; double fVolume; };        // 单位成交量,单位成交金额(指数,100元)
	double			CCL;							// 持仓量（订货量)  VolInStock
	double			JSJ;		// 昨日结算价
	double			YClose;
	// 指数,涨跌家数
	WORD			up;                  // 上涨家数
	WORD			down;                // 下跌家数

	AnalyDataB()
	{
		memset(this, 0, sizeof(AnalyDataB));
	}
	long GetDate()  //返回yyyymmdd
	{
		return DiskStkInfoB::GetDate(jclTime);
	}
	int GetYear()
	{
		return DiskStkInfoB::GetYear(jclTime);
	}
	int GetMonth()
	{
		return DiskStkInfoB::GetMonth(jclTime);
	}
	int GetDay()
	{
		return DiskStkInfoB::GetDay(jclTime);
	}
	int GetMonthDay()
	{
		return DiskStkInfoB::GetMonthDay(jclTime);
	}
	int GetHour()
	{
		return DiskStkInfoB::GetHour(jclTime);
	}
	int GetMinute()
	{
		return DiskStkInfoB::GetMinute(jclTime);
	}
	int GetHourMinute()
	{
		return DiskStkInfoB::GetMinute(jclTime);
	}
	int GetMinutes() //返回从0点开始的分钟数
	{
		return DiskStkInfoB::GetMinutes(jclTime);
	}
	int GetSecond()
	{
		return DiskStkInfoB::GetSecond(jclTime);
	}
	long GetTime()
	{
		return DiskStkInfoB::GetTime(jclTime);
	}
	
} ANALYDATAB;
//******************************************************************

// &&&&&&&&&&&&&&&&&&&&&&&
// 008迭代新增结构体   
// 2023 支持的扩展字段推送, 精简版, 约定：字段下标依次返回定义的顺序
struct KzField
{
	uint64_t nkey;			//品种内部编码
	char Code[VAR_CODE_LEN];//代码
	double NowPrice;		//现价
	double HighPrice;		//最高
	double LowPrice;		//最低
	double OpenPrice;		//今开
	double PreClosePrice;	//昨收
	double Volume;			//总手
	double ZangSu;			//涨速
	double BuyPrice1;		//买价一,昨持仓
	double SellPrice1;		//卖价一,昨结算
	double I_ZJJLR;			//资金净流入
	double MONEY_BIGIN;		//资金大单流入
	double MONEY_BIGOUT;	//资金大单流出
	double MONEY_PUREBIG;	//资金大单净额
	double MONEY_MIDIN;		//资金中单流入
	double MONEY_MIDOUT;	//资金中单流出
	double MONEY_PUREMID;	//资金中单净额
	double MONEY_SMALLIN;	//资金小单流出
	double MONEY_SMALLOUT;	//资金小单流出
	double MONEY_PURESMALL;	//资金小单净额
	KzField()
	{
		memset(this, 0, sizeof(KzField));
	}
};

// LV2 协议字段
struct BSData // 买卖队列数据
{
	double vol; // 买卖手数
	int state; // 状态  0:普通 1:新增 2:完全成交 3:更新(部分成交) 4:撤单
};

#define MMP_COUNT 10

struct BSQueue
{
	int32_t	total;
	int32_t count;
	BSData	data[50];
};

//linux版精简行情
struct CurrStockDataBExSimple
{
	unsigned long long ullNkey;						// 品种内部编码
	int		Source;						// 行情来源
	int		Status;						// 品种交易状态

	unsigned long long ullQuoteTime;					// 行情时间
	int		InOutFlag;					// 内外盘标志
	int		TickCount;					// 成交笔数 
	double		PreClosePrice;				// 前收盘价
	double		OpenPrice;					// 今开盘价
	double		HighPrice;					// 最高价
	double		LowPrice;					// 最低价
	double		NowPrice;					// 最新价
	double		zangsu;						// 涨速
	double		AveragePrice;				// 平均成交价格
	double		PriceDiff;					// 笔升跌(价位差)
	double		LimitUpPrice;				// 涨停价
	double		LimitDownPrice;				// 跌停价
	double		TaxPrice;					// 渤海现价含税价
	double		AverageTaxPrice;			// 渤海平均含税价
	double		PreSettlePrice;				// 期货昨结算价
	double		SettlePrice;				// 期货结算价 //期权结算价
	double		PERatio;					// 市盈率
	double		Volume;						// 总成交量(手)
	double		NowVol;						// 最近一笔成交量(手)
	double		Amount;						// 总成交金额(元)
	double		NowAmount;					// 最近一笔成交金额(元)
	double		Inside;						// 委买总成交量(手),内盘
	double		Outside;					// 委卖总成交量(手),外盘
	double		PreVolInStock;				// 期货昨持仓量
	double		VolInStock;					// 持仓量
	double		VolInStockDiff;				// 持仓差 // 20240621 量比
	double		TotalBuyVolume;				// 委买申报总量(手)
	double		TotalSellVolume;			// 委卖申报总量(手)
	double		BuyAveragePrice;			// 委买平均申报价格
	double		SellAveragePrice;			// 委卖平均申报价格
	int		BuyPriceCount;				// 委买申报价位总数
	int		BuyTickCount;				// 委买申报总笔数
	int		SellPriceCount;				// 委卖申报价位总数
	int		SellTickCount;				// 委卖申报总笔数
	double		BuyPrice[MMP_COUNT];		// 最优十档委买申报价
	double		BuyVolume[MMP_COUNT];		// 最优十档委买申报量
	double		SellPrice[MMP_COUNT];		// 最优十档委卖申报价
	double		SellVolume[MMP_COUNT];		// 最优十档委卖申报量
	double		PreNetValue;				// 基金T-1日净值
	double		NetValue;					// 基金实时参考净值(包括ETF的IOPV)
	int		ETFBuyNumber;				// ETF申购笔数
	int		ETFSellNumber;				// ETF赎回笔数
	union
	{
		double		ETFBuyVolume;				// ETF申购数量
		long long		LzStock;				// 板块领涨股
	};
	union
	{
		double		ETFSellVolume;				// ETF赎回数量
		long long		LdStock;				//板块领跌成分股
	};
	double		ETFBuyAmount;				// ETF申购金额
	double		ETFSellAmount;				// ETF赎回金额
	double		PreYield;					// 指数昨不含加权指数
	double		Yield;						// 不含加权的指数
	double		Lead;						// 领先指标(指数)
	int		IndexUpCount;				// 指数成分股上涨数
	int		IndexLevelCount;			// 指数成分股持平数
	int		IndexDownCount;				// 指数成分股下跌数    
	int		WarnCount;					// (活跃度)报警次数
	CurrStockDataBExSimple()
	{
		memset(this, 0, sizeof(CurrStockDataBExSimple));
	}
};

struct CurrStockDataBEx
{
	uint64_t	nkey;						// 品种内部编码
	int32_t		Source;						// 行情来源
	int32_t		Status;						// 品种交易状态

	int64_t		QuoteTime;					// 行情时间
	int32_t		InOutFlag;					// 内外盘标志
	int32_t		TickCount;					// 成交笔数 
	double		PreClosePrice;				// 前收盘价
	double		OpenPrice;					// 今开盘价
	double		HighPrice;					// 最高价
	double		LowPrice;					// 最低价
	double		NowPrice;					// 最新价
	double		zangsu;						// 涨速
	double		AveragePrice;				// 平均成交价格
	double		PriceDiff;					// 笔升跌(价位差)
	double		LimitUpPrice;				// 涨停价
	double		LimitDownPrice;				// 跌停价
	double		TaxPrice;					// 渤海现价含税价
	double		AverageTaxPrice;			// 渤海平均含税价
	double		PreSettlePrice;				// 期货昨结算价
	double		SettlePrice;				// 期货结算价 //期权结算价
	double		PERatio;					// 市盈率
	double		Volume;						// 总成交量(手)
	double		NowVol;						// 最近一笔成交量(手)
	double		Amount;						// 总成交金额(元)
	double		NowAmount;					// 最近一笔成交金额(元)
	double		Inside;						// 委买总成交量(手),内盘
	double		Outside;					// 委卖总成交量(手),外盘
	double		PreVolInStock;				// 期货昨持仓量
	double		VolInStock;					// 持仓量
	double		VolInStockDiff;				// 持仓差 // 20240621 量比
	double		TotalBuyVolume;				// 委买申报总量(手)
	double		TotalSellVolume;			// 委卖申报总量(手)
	double		BuyAveragePrice;			// 委买平均申报价格
	double		SellAveragePrice;			// 委卖平均申报价格
	int32_t		BuyPriceCount;				// 委买申报价位总数
	int32_t		BuyTickCount;				// 委买申报总笔数
	int32_t		SellPriceCount;				// 委卖申报价位总数
	int32_t		SellTickCount;				// 委卖申报总笔数
	double		BuyPrice[MMP_COUNT];		// 最优十档委买申报价
	double		BuyVolume[MMP_COUNT];		// 最优十档委买申报量
	double		SellPrice[MMP_COUNT];		// 最优十档委卖申报价
	double		SellVolume[MMP_COUNT];		// 最优十档委卖申报量
	BSQueue		BuyQueue;					// 最优买价50档买卖队列
	BSQueue		SellQueue;					// 最优卖价50档买卖队列
	double		PreNetValue;				// 基金T-1日净值
	double		NetValue;					// 基金实时参考净值(包括ETF的IOPV)
	int32_t		ETFBuyNumber;				// ETF申购笔数
	int32_t		ETFSellNumber;				// ETF赎回笔数
	union
	{
		double		ETFBuyVolume;				// ETF申购数量
		int64_t		LzStock;				// 板块领涨股
	};
	union
	{
		double		ETFSellVolume;				// ETF赎回数量
		int64_t		LdStock;				//板块领跌成分股
	};
	double		ETFBuyAmount;				// ETF申购金额
	double		ETFSellAmount;				// ETF赎回金额
	double		PreYield;					// 指数昨不含加权指数
	double		Yield;						// 不含加权的指数
	double		Lead;						// 领先指标(指数)
	int32_t		IndexUpCount;				// 指数成分股上涨数
	int32_t		IndexLevelCount;			// 指数成分股持平数
	int32_t		IndexDownCount;				// 指数成分股下跌数    
	int32_t		WarnCount;					// (活跃度)报警次数
	CurrStockDataBEx()
	{
		memset(this, 0, sizeof(CurrStockDataBEx));
	}
};

//pb精简行情数据结构 20240709 wyl
struct CurrTidyStockData // 品种行情（精简字段）
{
	short nSetCode;				// 市场
	char szCode[VAR_CODE_LEN];	// 品种
	double dHigh;				// 最高价
	double dLow;				// 最低价
	double dNow;				// 最新价
	double dIncSpeed;			// 涨速
	double dVolume;				// 总成交量(手)
	double dAmount;				// 总成交金额(元)
	double dEQRatio;			// 量比
	CurrTidyStockData &CurrTidyStockData::operator =(const CurrTidyStockData &other)
	{
		memcpy(this, &other, sizeof(CurrTidyStockData));
		return *this;
	}
	CurrTidyStockData()
	{
		memset(this, 0, sizeof(CurrTidyStockData));
	}
};

union CurrStockDataBExType
{
	struct flag_t
	{
		uint64_t HQ : 1;				// 行情
		uint64_t Source : 1;			// 行情来源
		uint64_t Status : 1;			// 品种交易状态
		uint64_t QuoteTime : 1;			// 行情时间
		uint64_t InOutFlag : 1;			// 内外盘标志
		uint64_t TickCount : 1;			// 成交笔数 
		uint64_t PreClosePrice : 1;		// 前收盘价
		uint64_t OpenPrice : 1;			// 今开盘价
		uint64_t HighPrice : 1;			// 最高价
		uint64_t LowPrice : 1;			// 最低价
		uint64_t NowPrice : 1;			// 最新价
		uint64_t zangsu : 1;			// 涨速
		uint64_t AveragePrice : 1;		// 平均成交价格
		uint64_t PriceDiff : 1;			// 笔升跌(价位差)
		uint64_t LimitUpPrice : 1;		// 涨停价			
		uint64_t LimitDownPrice : 1;	// 跌停价
		uint64_t TaxPrice : 1;			// 渤海现价含税价	
		uint64_t AverageTaxPrice : 1;	// 渤海平均含税价	
		uint64_t PreSettlePrice : 1;	// 期货昨结算价		
		uint64_t SettlePrice : 1;		// 期货结算价		
		uint64_t PERatio : 1;			// 市盈率
		uint64_t Volume : 1;			// 总成交量(手)
		uint64_t NowVol : 1;			// 最近一笔成交量(手)
		uint64_t Amount : 1;			// 总成交金额(元)
		uint64_t NowAmount : 1;			// 最近一笔成交金额(元)
		uint64_t Inside : 1;			// 委买总成交量(手),内盘
		uint64_t Outside : 1;			// 委卖总成交量(手),外盘
		uint64_t PreVolInStock : 1;		// 期货昨持仓量
		uint64_t VolInStock : 1;		// 持仓量
		uint64_t VolInStockDiff : 1;	// 持仓差 
		uint64_t TotalBuyVolume : 1;	// 委买申报总量(手)
		uint64_t TotalSellVolume : 1;	// 委卖申报总量(手)
		uint64_t BuyAveragePrice : 1;	// 委买平均申报价格
		uint64_t SellAveragePrice : 1;	// 委卖平均申报价格
		uint64_t BuyPriceCount : 1;		// 委买申报价位总数
		uint64_t BuyTickCount : 1;		// 委买申报总笔数
		uint64_t SellPriceCount : 1;	// 委卖申报价位总数
		uint64_t SellTickCount : 1;		// 委卖申报总笔数
		uint64_t BuyPrice : 1;			// 最优十档委买申报价
		uint64_t BuyVolume : 1;			// 最优十档委买申报量
		uint64_t SellPrice : 1;			// 最优十档委卖申报价
		uint64_t SellVolume : 1;		// 最优十档委卖申报量
		uint64_t BuyQueue : 1;			// 最优买价50档买卖队列
		uint64_t SellQueue : 1;			// 最优卖价50档买卖队列
		uint64_t PreNetValue : 1;		// 基金T-1日净值
		uint64_t NetValue : 1;			// 基金实时参考净值(包括ETF的IOPV)
		uint64_t ETFBuyNumber : 1;		// ETF申购笔数
		uint64_t ETFSellNumber : 1;		// ETF赎回笔数
		uint64_t ETFBuyVolume : 1;		// ETF申购数量
		uint64_t ETFSellVolume : 1;		// ETF赎回数量
		uint64_t ETFBuyAmount : 1;		// ETF申购金额
		uint64_t ETFSellAmount : 1;		// ETF赎回金额
		uint64_t PreYield : 1;			// 指数昨不含加权指数
		uint64_t Yield : 1;				// 不含加权的指数
		uint64_t Lead : 1;				// 领先指标(指数)
		uint64_t IndexUpCount : 1;		// 指数成分股上涨数
		uint64_t IndexLevelCount : 1;	// 指数成分股持平数
		uint64_t IndexDownCount : 1;	// 指数成分股下跌数   
		uint64_t WarnCount : 1;			// (活跃度)报警次数
	}u;
	uint64_t value;
};

struct CqcxInfo // 014
{
	unsigned __int64 nkey;	//品种编码
	short num;				//除权除息数据块个数
	CWDATA data[0];			//数据块
};

//2025 专用 pc端 016 版
struct CurrStockDataExSimple
{
	uint64_t nkey;					// 品种内部编码
	int64_t	QuoteTime;				// 行情时间
	double	dNowPrice;				// 最新价
	double	dHighPrice;				// 最高价
	double	dLowPrice;				// 最低价
	double	dAmount;				// 总成交金额(元)
	double	dVolume;				// 总成交量
	double	dZangSu;				// 涨速
	double	dOpenPrice;				// 今开盘价
	double	dPreClosePrice;			// 前收盘价
	double	dBuyPrice1;				// 买一价
	double	dSellPrice1;			// 卖一价
	CurrStockDataExSimple()
	{
		memset(this, 0, sizeof(CurrStockDataExSimple));
	}
};

//2026 专用 pc端 016 版
struct CurrStockDataExTidy
{
	uint64_t nkey;					// 品种内部编码
	int64_t	QuoteTime;				// 行情时间
	double	dNowPrice;				// 最新价
	double	dOpenPrice;				// 今开盘价
	double	dHighPrice;				// 最高价
	double	dLowPrice;				// 最低价
	double	dPreClosePrice;			// 前后盘价
	double	dZangSu;				// 涨速
	double	dAveragePrice;			// 均价
	double	dVolume;				// 总成交量(手)
	double	dAmount;				// 总成交金额(元)
	double	dNowVol;				// 最近一笔成交量(手)
	double	dBuyPrice1;				// 买一价
	double	dSellPrice1;			// 卖一价
	double	dBuyVolume1;			// 买一量
	double	dSellVolume1;			// 卖一量
	double	dInside;				// 委买总成交量(手),内盘
	double	dOutside;				// 委卖总成交量(手),外盘

	CurrStockDataExTidy()
	{
		memset(this, 0, sizeof(CurrStockDataExTidy));
	}
};

//涨停板列表
struct ST_LIMIT_UP_STK
{
	tagCode stCode;
	double dIntensity;//封单强度 涨停金额 / 流通市值 买一价*买一量
	double dMaxBuyVol;//最大买一量 实时
	ST_LIMIT_UP_STK()
	{
		dIntensity = 0.0f;
		dMaxBuyVol = 0.0f;
	}
};

//涨停板指数 涨停股票:收盘价等于涨停价
struct ST_LIMIT_UP_INDEX
{
	int iDate;
	double dLimitUp; //所有T-1日 涨停股票的开盘的涨幅累加 / 股票个数
	ST_LIMIT_UP_INDEX()
	{
		memset(this, 0, sizeof(ST_LIMIT_UP_INDEX));
	}
};

//触版指数 最高价等于涨停价 收盘价小于涨停价的股票为昨日触板的股票
struct ST_TOUCH_BOARD_INDEX
{
	int iDate;
	double dLimitUp; //所有T-1日 昨日触板股票开盘的涨跌幅累加 / 股票个数
	ST_TOUCH_BOARD_INDEX()
	{
		memset(this, 0, sizeof(ST_TOUCH_BOARD_INDEX));
	}
};

//市场情绪得分
struct ST_MARKET_SCORE_INDEX
{
	int iDate;
	double dScore; //T-1日 市场情绪得分
	ST_MARKET_SCORE_INDEX()
	{
		memset(this, 0, sizeof(ST_MARKET_SCORE_INDEX));
	}
};

//存在2个列表  昨日触板 昨日涨停板

#pragma pack(pop) 

#endif