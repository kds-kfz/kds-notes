#ifndef  _MACRO_H
#define  _MACRO_H

//#define	  VAR_CODE_LEN		32
//#define	  VAR_NAME_LEN		32
#define	  VAR_NOTICE_LEN	128

#define	  VAR_CODE_LENB		32
#define	  VAR_NAME_LENB		64	// 32

#define   SH_CODE_LEN      6
#define   NAME_LEN         8

// protobuf 功能号偏移,超出了short范围，RPC适用，int64
const unsigned __int64	PROTOBUF_OFFSET = 100000000;	// 加上这个偏移

//TJZT1.1.0 新增 
//PB协议段 11501~12000
#define PB_PROTOCOL_INDATA_MIN		11501		// 指标插件协议段起点
#define PB_PROTOCOL_INDATA_MAX		12000		// 指标插件协议段终点

#define TDEL(a) {if (a) {delete a;a=NULL;}}
#define TFREE(a) {if (a) {free(a);a=NULL;}}
#define TMALLOC(ptr,size,type)   { ptr = new type[size/sizeof(type)];\
									if(ptr) memset(ptr,0,size);\
								}

#define VIP_MODE	0
#define DJJS_MODE	1
#define PHFX_MODE	2
#define TCPIP_MODE	3
#define OTHER_MODE	4

#define		USER_SIZE		400	//概念板块和组合板块中股票的最大数目
#define MAXGPNUM		12000	//5000
#define MAXBIGBSPNUM	20
#define MEANLESS_DATA   7654321.00

#define ALIVE_CHECKTIME	200		//存活包检查秒数 30->60->120->200

#define COMPPREC		10e-6   // 精度宏

#define MAX max
#define MIN min

#ifdef  LINUX
#define O_BINARY    0
#define _SH_DENYNO  0x40
#define SH_DENYNO   _SH_DENYNO
#define _PACKED		__attribute__((packed))
#define PACKED		_PACKED
#else
#define	_PACKED
#define PACKED
#endif

#define PER_MIN5     0
#define PER_MIN15    1
#define PER_MIN30    2
#define PER_HOUR     3 
#define PER_DAY      4
#define PER_WEEK     5
#define PER_MONTH    6 
/////////////////////
#define PER_MIN1     7 
#define PER_MINN	 8
#define PER_DAYN     9

#define PER_SEASON	 10		//季线,需要下载日线数据
#define PER_YEAR	 11		//年线,需要下载日线数据

#define PER_EXDAY	20		//扩展日线的周期,通达信券商集成版暂不支持
#define PER_ZST		21		//直接给走势图用



#define TICK_DAT    100
#define	MINUTE_DAT  101		//分时数据
#define	BSP_DAT		102		//买卖盘
#define GBINFO_DAT	103		//股本信息
#define	REPORT_DAT	104
#define	STKINFO_DAT	105
#define OTHER_DAT	130		//其它

#define FZNUM           360

#define NOXS          0        //没有小数
#define ZS            1        //指数
#define XS2           2        //2位小数
#define XS3           3        //3位小数
#define XS4			  4

#define SUCCESS			0
#define FAIL			-1

#define	HOST_SUB_KEY        _T("Software\\Microsoft\\WHost")
#define TWORSAKEY_VALUE		  _T("Asymmetric")

#define QHLB          0     //期货类别
#define GPLB          1     //股标类别
#define ZSLB          2     //指数
#define HKLB          3

#define DRAWALL       0     //全部重画
#define UPDATE        1     //画变化部分(价位变化)
#define TIMEUPDATE    2     //(时间变化)
#define UP            3     //上Scroll
#define DOWN          4     //下Scroll
#define ACTIVEUPDATE  5
#define SCROLL_DRAWALL 6    //自动翻页Draw
#define DRAWTICKALL   7
#define DRAWREDRAW    8

const short NOSORT  =0;     //不排名
const short DECREASE=1;     //按降序排名
const short INCREASE=2;     //按升序排名

//系统支持的基本面类型
const short TEXTXX	=117;		//大文本文件
const short SHXX	=118;		//上海信息
const short WXXX	=119;		
const short SZWXXX	=120;		//深圳新闻信息
const short NFXX	=121;		
const short SMEXX	=122;	
const short SICXX	=123;
const short YCQSXX	=124;		//远程券商信息
const short MULXX	=125;		//通用信息类型
const short SWXX	=126;		//申银万国资讯

#define ALL_SZ		0
#define SZ_NO_ZS	1
#define SZ_ZS		2
#define SZ_GP		3
#define ALL_SH		4
#define SH_NO_ZS	5
#define SH_ZS		6
#define SH_GP		7
#define ALL_HS		8
#define ALL_QH		9
#define ALL_ALL		10
#define ALL_WD		11
#define ALL_CY		12


#define ALL_HK		13
#define ALL_SF		14
#define ALL_SC		15
#define ALL_DC		16
#define ALL_ZC		17
#define ALL_BH		18
#define ALL_SJ		19
#define ALL_GJ		20
#define	ALL_TJ		21
#define ALL_FX		22
#define ALL_HX		23
#define ALL_USR		24
#define ALL_X3B  	25
#define ALL_LDJ0	26
#define ALL_LDJ1	27
#define ALL_LDJ2	28
#define ALL_TEA		29
#define ALL_DPT     30
#define ALL_NYSE	31
#define ALL_NASDAQ	32
#define	ALL_AMER	33
#define	ALL_USA		34

#define DI_FUND_HOMEPAGE				4000	            //分级基金-B基金
#define DI_FUND_SUBJECT					4001				//主题基金（挂钩指数、母基金、A基金、B基金）
#define DI_FUND_B								4002				//B类基金
#define DI_FUND_SUBJECT_HOT			4003				//主题基金
#define	 DI_FUND_HOMEPAGE_A			4004				// 分级基金-A基金
#define DI_FUND_HOMEPAGE_M			4005				// 分级基金-母基
#define DI_FUND_HOMEPAGE_FJ_T2		4006				// 分级基金-分级T+2
#define DI_FUND_HOMEPAGE_FJ_T1		4007				// 分级基金-分级T+1
#define DI_FUND_HOMEPAGE_FJ_T0		4008				// 分级基金-分级T+0
#define DI_FUND_MONEY_T0				4009				// 货币基金T+0
#define DI_FUND_MONEY_T1				4010				// 货币基金T+1
#define DI_FUND_HOLDSTOCK				5000				//	B基金对应的持仓股
#define DI_FUND_INDEXSTOCK				5001				//	指数对应持仓股
//const short SZ		=0;
//const short SH		=1;
//const short QH		=2;
//const short SZ_SH	= 3;//7;//3;

//  512 文件可以同时处于打开 级别的用于 lowio (即打开并访问通过 fopen、fgetc，fputc，依此类推 I/O 功能系列)。
// 通过_setmaxstdio增加到最大： 2,048个文件句柄
//const	int		MAX_MARKETNUM	= 25;//11;//9;
//{"sz",  "sh",  "hk",  "sf",  "sc",  "dc",  "zc"};
//{"深圳","上海","香港","股指","上商","连商","郑商",""}

const int MARKETBLOCK = 2;	//0深圳 1上海

const short GZ = 2; // 股转
const short BJ = 3; // 北交所

// 如果直接改成市场连续，客户端到服务端很多地方没法兼容
const short SZ=0;
const short SH=1;
const short QH=2;	// 2~6合并请求
const short HK=9;	// 香港------------------------------------新接外盘市场
const short SF=3;	// 上海股指期货
const short SC=4;	// 上海商品
const short DC=5;	// 大连商品
const short ZC=6;	// 郑州商品
const short BH=7;	// 渤海
const short SJ=8;	// 上海金------------------------------------新接外盘市场
const short GJ=9;	// 国际黄金(包含伦敦金)---------------新接外盘市场
const short TJ=10;	// TJ天津金)-------------新接外盘市场
const short FX = 11;//外汇市场-----------------------------新接外盘市场
const short HX=12;	//	厦门海峡艺术
const short X3B=13; //新三版市场(替换HQXG横琴稀贵市场)--------新接外盘市场
const short USR=14; //美股市场
//;0是伦敦xx品种，1是布伦特原油，2是伦敦金其他品种
const short LDJ0=15;	// 伦敦金0
const short LDJ1=16;	// 伦敦金1
const short LDJ2=17;	// 伦敦金2
const short BG=18;	// 宝钢资金
const short TEA=19;		// 广州茶叶
const short DPT=20;	// 甬交所行情 类似于海峡

// 纽约交易所 纳斯达克交易所 美国交易所
const short	NYSE		= 21;	// 纽约交易所
const short NASDAQ		= 22;	// 纳斯达克交易所
const short American	=23;// 美国交易所

//enHK = 2, // 香港
//20100726 edit
//enSF = 3, // 上海股指期货
//enSC = 4, // 上海商品
//enDC = 5, // 大连商品
//enZC = 6, // 郑州商品

const short SZ_SH	=30;

//用于远程读行情 
#define VAL_SHAG       0    //上证Ａ股
#define VAL_SHBG       1    //上证Ｂ股
#define VAL_SZAG       2    //深证Ａ股
#define VAL_SZBG       3    //深证Ｂ股
#define VAL_SHZQ       4    //上证债券
#define VAL_SZZQ       5    //深证债券
#define VAL_SZSHAG     6    //深沪Ａ股
#define VAL_SZSHBG     7    //深沪Ｂ股
#define VAL_SZSHZQ     8    //深沪债券
#define VAL_SZSHJJ     9    //深沪基金
#define VAL_ALL       10    //所有品种
#define VAL_ZS        11    //所有指数
#define VAL_CY		  12	//中小板块
#define VAL_CYB		  13	//创业板
#define VAR_CZZ		  14	//可转债(未使用)
#define VAR_SHHG	  15	//上海回购(未使用)
#define VAR_SZHG	  16	//深圳回购(未使用)
#define VAL_ALL_SPQH  17	//	商品期货(上海商品,郑州商品,大连商品)(未使用)	

#define VAL_ETFGZINDEX	14 // ETF跟踪的指数(沪深市场) 新
#define VAL_ETFSTOCK	15 // 股票型ETF(沪深市场) 新
#define VAL_ETFINDEX	16 // 宽基指数ETF(沪深市场) 新
#define VAL_ETFQDII		17 // 外盘ETF(沪深市场) 新
#define VAL_ETFHYZT		18 // 行业主题ETF(沪深) 新
#define VAL_ETFHSG		19 // 沪港深ETF 新
#define VAL_ETFCURRENCY	20 // 货币ETF 新
#define VAL_ETFGOODS	21 // 商品ETF 新
#define VAL_ETFBOND		22 // 债券ETF 新
#define VAL_ETFT0		23 // T0ETF 新
 
// DI_BH  = 19,
// DI_QH_DLSP  = 20,
// DI_QH_ZZSP  = 21,
// DI_QH_SHSP  = 22,
// DI_QH_GZQH  = 23,
#define	VAL_BH			19 // (未使用)
#define VAL_DLSP		20 // (未使用)
#define VAL_ZZSP		21 // (未使用)
#define VAL_SHSP		22 // (未使用)
#define VAL_GZQH		23 // (未使用)
#define VAL_SJ			24
#define VAL_GJ			25   //国际黄金
#define VAL_TJ			26
#define VAL_BTB			27
#define VAL_KZZ			28
#define VAL_FX			29		// 新增外汇
#define VAL_ALL_BLOCKINDEX		30		// 
#define VAL_AREA_BLOCKINDEX		31		// 881XXX 地域板块所有板块指数的
#define VAL_INDUSTRY_BLOCKINDEX	32		// 880XXX 行业板块所有板块指数的
#define VAL_CONCEPT_BLOCKINDEX	33		// 882XXX 

#define DI_RM_BLOCK_ALL			34		//热门板块-所有
#define DI_RM_BLOCK_AREA		35		//热门板块-地域
#define DI_RM_BLOCK_INDUSTRY	36		//热门板块-行业
#define DI_RM_BLOCK_CONCEPT		37		//热门板块-概念

#define VAL_HX				38		//海峡艺术品交易所-所有品种
#define DI_HX_YSZB			39		//海峡艺术品交易所-艺术主板
#define DI_HX_YSXB			40		//海峡艺术品交易所-艺术新版
#define DI_HX_YBSC			41		//海峡艺术品交易所-邮票收藏
#define DI_HX_QBSC			42		//海峡艺术品交易所-钱币收藏
#define DI_HX_PSJK			43		//海峡艺术品交易所-配售缴款
#define DI_HX_CCQB			44		//海峡艺术品交易所-茶产权版

#define VAL_QLSP				50	//	齐鲁商品市场
#define VAL_X3B			51		//	新三版市场
#define	VAL_ALL_GJ		52	//	
#define DI_PH_FUND		53	//	鹏华基金分组
#define VAL_TEA			54	//	广州茶叶商品期货-所有品种
#define VAL_TEA_ZS		55	//	广州茶叶商品期货-指数
#define VAL_TEA_GOODS	56	//	广州茶叶商品期货-商品
#define VAL_DPT         57  //  甬交所行情kkki
#define VAL_HK          58    //香港  
#define VAL_USR         59
#define VAL_TFB         60   
#define VAL_NYSE		61
#define VAL_NASDAQ		62
#define	VAL_AMER		63
#define VAL_CHINABK		64
#define VAL_ALLUSA		70
#define VAL_KCB			74
#define VAL_ETF			75
#define VAL_CYB_REG		76
#define VAL_CYB_UNREG	77
#define VAL_HS300		78
#define VAL_SH50		79
#define VAL_REITS		83
#define VAL_HSZS		85
#define VAL_INDEX_CFG   86
#define VAL_GGT_SZ		87 //深港通
#define VAL_GGT_SH		88 //沪港通
#define VAL_BJ			92	//北交
#define VAL_GZ_AG		93	//股转A
#define VAL_GZ_BG		94	//股转B
#define VAL_GZ_SSGS		95	//股转上市公司

#define VAR_FJJJ_T2	  4006	//货币基金T+2
#define VAR_FJJJ_T1	  4007	//货币基金T+1
#define VAR_FJJJ_T0	  4008	//货币基金T+0

#define VAR_HBJJ_T0	  4009	//货币基金T+0
#define VAR_HBJJ_T1	  4010	//货币基金T+1

// 扩展的排序
#define VAL_AREA_BLOCK		100			// +i : 地域板块对应指数，对应成分股的排名
#define VAL_INDUSTRY_BLOCK	200	
#define VAL_CONCEPT_BLOCK	300	
#define VAL_COMBO_BLOCK		1300	


enum DomainIndex
{
	DI_SHAG = 0,
	DI_SHBG = 1,
	DI_SZAG = 2,
	DI_SZBG = 3,
	// 4: 上海债券
	// 5 : 深圳债券
	DI_SHBOUD=4,
	DI_SZBOUD=5,
	DI_AG = 6,
	DI_BG = 7,
	DI_BOND = 8,
	DI_FUND = 9,
	DI_ALLGP= 10, //所有商品
	DI_ALLINDEX=11,//所有指数
	DI_ZHONGXIAO = 12,	//中小企业
	
	DI_CUSTOM = 13,	// 自选股
	DI_CONDITION,	// 条件股
	DI_SANBAN,		// 三板
	DI_COMBO,		// 组合板块
	
	DI_OPTION = 17,	// 17 权证
	DI_GEM = 18,		// 18 创业板
	DI_BH  = 19,
	DI_QH_DLSP  = 20,
	DI_QH_ZZSP  = 21,
	DI_QH_SHSP  = 22,
	DI_QH_GZQH  = 23,
	DI_DC  = 20,
	DI_ZC  = 21,
	DI_SC  = 22,
	DI_SF  = 23,
	
	DI_AREA	= 24,//DMI_TOTAL,	// 地域
	DI_INDUSTRY,			// 行业
	DI_CONCEPT,				// 概念
	DI_USER_DEFINED,		// 自定义
	DI_FIRST_3RD = 50,		// 第三方
	DI_LAST_3RD = 99,
	
	DI_FIRST_CUSTOM_BLOCK = 0,
	DI_FIRST_AREA_BLOCK = 100,
	DI_FIRST_INDUSTRY_BLOCK = 200,
	DI_FIRST_CONCEPT_BLOCK = 1000,
	DI_FIRST_COMBO_BLOCK = 1300,//400,
	
	DI_SECOND_INDUSTRY_BLOCK = 1600,
	DI_SYSTEMBLOCK	= 1888,	// 系统板块

    DI_FIRST_DPT_BLOCK = 20000,
};
//股票栏目代码
const short ZQDM = 0;			//代码
const short ZQJC = 1;			//证券名
const short ZRSP = 2;			//昨收
const short JRKP = 3;			//今开
const short ZGCJ = 4;			//最高
const short ZDCJ = 5;			//最低
const short ZJCJ = 6;			//现价
const short ZGJM = 7;			//叫买价
const short ZDJM = 8;			//叫卖价
const short CJL  = 9;			//总手
const short CJJE = 10;			//总金额
const short XS   = 11;			//现手
const short QRSD = 12;			//日升跌
const short QBSD = 13;			//笔升跌
const short ZAF  = 14;			//涨幅
const short ZEF  = 15;			//振幅
const short JUNJ = 16;			//均价
const short SYL  = 17;			//市盈率,持仓
const short WTB  = 18;			//委比
const short LPNP   = 19;		//内盘	LP
const short WP   = 20;			//外盘
const short LWB  = 21;			//内外比
const short WLC  = 22;			//委量差

const short BJL1 = 23;			//买手一
const short SJL1 = 24;			//卖手一
const short BJ1  = 25;			//买价一,昨持仓,
const short SJ1  = 26;			//卖价一,昨结算

const short BJ2  = 27;			//买价二
const short BJL2 = 28;			//买手二
const short SJ2  = 29;			//卖价二,持仓增减
const short SJL2 = 30;			//卖手二

const short BJ3  = 31;			//买价三
const short BJL3 = 32;			//买手三
const short SJ3  = 33;			//卖价三
const short SJL3 = 34;			//卖手三

const short LIANGB=35;			//量比

const short J_HSL =36;			//换手率
const short J_LTGB=37;			//流通股本
const short J_LTSZ=38;			//流通市值
const short J_ZSZ =39;			//总市值

const short DKPH  =  40;		//多空平衡
const short DTHL  =  41;		//多头获利
const short DTZS  =  42;		//多头止损
const short KTHB  =  43;		//空头回补
const short KTZS  =  44;		//空头止损
const short QRD   =  45;		//强弱度

const short ZANGSU  =46;		//涨速

const short HYD		=47;		//活路度
const short MBZL	=48;		//每笔均量
const short MBHSL	=49;		//每笔换手

const short J_GXRQ	=50;		//更新日期
const short J_START =51;		//上市日期
const short J_ZGB	=52;		//总股本
const short J_GJG	=53;		//国家股
const short J_FQRFRG=54;		//发起人法人股
const short J_FRG	=55;		//法人股
const short J_BG	=56;		//B股
const short J_HG	=57;		//H股
const short J_ZGG	=58;		//职工股
const short J_ZZC	=59;		//总资产(千元)
const short J_LDZC	=60;		//流动资产
const short J_GDZC	=61;		//固定资产
const short J_WXZC	=62;		//无形资产
const short J_CQTZ	=63;		//长期投资
const short J_LDFZ	=64;		//流动负债
const short J_CQFZ	=65;		//长期负债
const short J_ZBGJJ	=66;		//资本公积金
const short J_JZC	=67;		//股东权益(就是净资产)
const short J_ZYSY	=68;		//主营收入
const short J_ZYLY	=69;		//主营利益
const short J_QTLY	=70;		//其它利益
const short J_YYLY	=71;		//营业利益
const short J_TZSY	=72;		//投资收益
const short J_BTSY	=73;		//补贴收入
const short J_YYWSZ	=74;		//营业外收支
const short J_SNSYTZ=75;		//上年损益调整
const short J_LYZE	=76;		//利益总额
const short J_SHLY	=77;		//税后利益
const short J_JLY	=78;		//净利益
const short J_WFPLY	=79;		//未分配利益
const short J_TZMGJZ=80;		//调整每股净资产

const short J_JYL	=81;		//净益率
const short J_MGWFP	=82;		//每股未分配
const short J_MGSY	=83;		//每股收益
const short J_MGGJJ	=84;		//每股公积金
const short J_MGJZC	=85;		//每股净资产
const short J_GDQYB	=86;		//股东权益比

const short ZBCOL =87; 		//指标排序栏目

const short	SPELL_CODE=88;		//外汇简称
const short QH_JSJ = 89;		//期货结算价
const short QH_YJSJ= 90;		//期货前结算价

//大福三方特有
const short SPREAD = 91;		//买卖差价
const short BSUNIT = 92;		//买卖单位
const short CURRENCYNAME = 93;	//货币单位
const short AVERPRICE = 94;		//平均价
const short YIELDVAL = 95;		//收益率
const short HIS_HIGH = 96;		//年最高
const short HIS_LOW = 97;		//年最低
const short IEP = 98;			//参考价
const short IEV = 99;			//参考量
const short MRKCAP = 100;		//市值
const short PE  =101;			//市盈率
const short PREMINUM = 102;		//溢价%
const short GEARING = 103;		//贡杆比率%
const short EXECPRICE = 104;		//行使价
const short CONVRATIO = 105;		//换购比率
const short EXPIREDATE = 106;	//到期日
const short NOTAXPRCIE = 107;	//不含税价 = 交易价
const short DEPOSITMONEY = 108;	//不含税价 = 订货保证金
const short DAYDEPOSITMONEY = 109;	//不含税价 = 增仓保证金
const short AVGTAXPRICE = 110;//不含税价 = 平均含税价
const short ZXGTIME = 117;		//自选股添加日期
const short SSBK = 118;		//所属板块
const short ZLJZ = 119;		//主力净值
const short ZLZB = 120;		//主力占比
const short SMJZ = 121;		//私募净值
const short SMZB = 122;		//私募占比

const short HTMLXX	=129;		//Html信息
const short URGENTXX = 130;		//紧急通告(营业部通知)
const short NEWSGATE1 = 131;	//新闻搜索器
const short NEWSGATE2 = 132;	//新闻搜索器

// 20120402
const short I_ZJJLR		= 133;		// 资金净流入
const short I_ZJJLRBL	= 134;		// 资金净流入占盘比
const short I_ZLJLR		= 135;		// 主力净流入
const short I_ZLJLRBL	= 136;		// 主力净流入占盘比
const short I_ZLZJLR	= 137;		// 主力资金流入
const short I_ZLZJLRBL	= 138;		// 主力资金流入占盘比
const short I_ZLZJLC	= 139;		// 主力资金流出
const short I_ZLZJLCBL	= 140;		// 主力资金流出占盘比
const short I_INOUTZJBL	= 141;		// 内外盘资金比
const short I_ZLCJBL	= 142;		//主力成交占比   （主力买入+主力卖出）/（主力买入+主力卖出+散户买入+散户卖出）

const short BH_DHL = 143;		//订货量
const short BH_RZC = 144;		//日增仓
const short MONEY_ZLJLC = 145;   //主力净流出
const short MONEY_ZLJLR = 146;   //主力净流入
const short MONEY_ZLJLC3 = 147;   //3日主力净流出
const short MONEY_ZLJLR3 = 148;   //3日主力净流入
const short MONEY_ZLJLC5 = 149;   //5日主力净流出
const short MONEY_ZLJLR5 = 150;  //5日主力净流入


const short CJBL			= 152;			//成交比例
const short ZS_LZ			= 153;			//领涨成分股
const short ZS_LD			= 154;			//领跌成分股
const short ZS_CFG			= 155;			//成分股个数
const short ZS_ZDP			= 156;			//涨跌平
const short ZS_SZJS			= 157;			//上涨家数
const short ZS_XDJS			= 158;			//下跌家数
const short ZS_PJS			= 159;			//平家数
const short	FUND_BEGIN		= 160;			//基金排序开始列
const short FUND_BCODE		= 160;			//基金B代码
const short FUND_BNAME		= 161;			//基金B名称
const short FUND_BPRICE		= 162;			//基金B现价
const short FUND_BZAF		= 163;			//基金B涨幅
const short FUND_BJINGZHI	= 164;			//基金B净值
const short FUND_BYJL		= 165;			//基金B溢价率
const short FUND_FEBHL		= 166;			//基金B份额变化率
const short FUND_SGTLKJ		= 167;			//基金申购套利空间
const short FUND_SHTLKJ		= 168;			//基金赎回套利空间
const short FUND_ABPRICE	= 169;			//基金AB合并价
const short FUND_MCODE		= 170;			//母级基金代码
const short FUND_MNAME		= 171;			//母级基金名称
const short FUND_MJINGZHI	= 172;			//母级基金净值
const short FUND_ZSCODE		= 173;			//基金挂钩指数代码
const short FUND_ZSNAME		= 174;			//基金挂钩指数名称
const short FUND_ZSZAF		= 175;			//基金挂钩指数涨幅
const short FUND_ACODE		= 176;			//基金A代码
const short FUND_ANAME		= 177;			//基金A名称
const short FUND_APRICE		= 178;			//基金A现价
const short FUND_AZAF		= 179;			//基金A涨幅
const short FUND_AJINGZHI	= 180;			//基金A净值
const short FUND_AYJL		= 181;			//基金A溢价率

const short FUND_BFE		= 182;			//基金B份额
const short FUND_BJZBHL		= 183;			//基金B净值增长率%
const short FUND_FOLDUP		= 184;			//上折阈值
const short FUND_FOLDDOWN	= 185;			//下折阈值
const short FUND_BJZFOLDUP	= 186;			//基金B净值上折差距%
const short FUND_BJZFOLDDOWN= 187;			//基金B净值下折差距%
const short FUND_BNOTICE	= 188;			//基金B公告
const short FUND_SUBJECT	= 189;			//基金主题
const short FUND_SGPRICE	= 190;			//申购价格
const short FUND_SHPRICE	= 191;			//赎回价格
const short FUND_ABYJL		= 192;			//整体溢价率%
const short FUND_MJZBHL		= 193;			//母级基金净值增长率%
const short FUND_MNOTICE	= 194;			//母级基金公告
const short FUND_FEZB		= 195;			//AB份额比率（A:B）
const short FUND_AJZBHL		= 196;			//基金A净值增长率%
const short FUND_AFOLDPRICE	= 197;			//基金A折价%
const short FUND_APROFIT	= 198;			//基金A到期收益%
const short FUND_AFE		= 199;			//基金A份额
const short FUND_AFEBHL		= 200;			//基金A份额变化率%
const short FUND_ARESTDAY	= 201;			//基金A剩余年限
const short FUND_AENDDATE	= 202;			//基金A定期折算
const short FUND_AYDPROFIT	= 203;			//基金A约定收益率%
const short FUND_ANOTICE	= 204;			//基金A公告
//资金协议块,必须保证这里是连续的
const short MONEY_ZLJME_1	= 205;			//1日主力净买额
const short MONEY_ZLJME_2	= 206;			//2日主力净买额
const short MONEY_ZLJME_3	= 207;			//3日主力净买额
const short MONEY_ZLJME_4	= 208;			//4日主力净买额
const short MONEY_ZLJME_5	= 209;			//5日主力净买额
const short MONEY_ZLJZB_1	= 210;			//1日主力净占比
const short MONEY_ZLJZB_2	= 211;			//2日主力净占比
const short MONEY_ZLJZB_3	= 212;			//3日主力净占比
const short MONEY_ZLJZB_4	= 213;			//4日主力净占比
const short MONEY_ZLJZB_5	= 214;			//5日主力净占比
const short MONEY_SHJME_1	= 215;			//1日散户净买额
const short MONEY_SHJME_2	= 216;			//2日散户净买额
const short MONEY_SHJME_3	= 217;			//3日散户净买额
const short MONEY_SHJME_4	= 218;			//4日散户净买额
const short MONEY_SHJME_5	= 219;			//5日散户净买额
const short MONEY_SHJZB_1	= 220;			//1日散户净占比
const short MONEY_SHJZB_2	= 221;			//2日散户净占比
const short MONEY_SHJZB_3	= 222;			//3日散户净占比
const short MONEY_SHJZB_4	= 223;			//4日散户净占比
const short MONEY_SHJZB_5	= 224;			//5日散户净占比
const short FUND_JINGZHI	= 225;			//净值
const short FUND_YJL		= 226;			//溢价
const short FUND_UPDATEDATE = 227;			//净值更新日期
const short FUND_POSITION   = 228;			//基金仓位系数
const short FUND_B_REAL_JZ	= 229;			//B基金公布净值
const short FUND_PRICELEVEL	= 230;			//B基金价格杠杆
const short FUND_LEVEL		= 231;			//B基金净值杠杆
const short FUND_REAL_LEVEL	= 232;			//B基金实际杠杆
const short FUND_FINANCE	= 233;			//B基金融资成本
const short FUND_ABYJL_T1	= 234;			//T-1溢价率
const short FUND_ABYJL_T2	= 235;			//T-2溢价率
const short FUND_AFE_INC	= 236;			//A基金份额新增(万份)
const short FUND_RATE_RULE	= 237;			//利率规则
const short FUND_RATE		= 238;			//本期利率
const short FUND_RATE_NEXT	= 239;			//下期利率
const short FUND_M_REL_JZ	= 240;			//母基公布净值
const short FUND_CREATE		= 241;			//基金成立日期
const short FUND_A_CJJE		= 242;			//A基金成交额(亿)
const short FUND_B_CJJE		= 243;			//B基金成交额(亿)
const short FUND_A_HSL		= 244;			//A基金换手率
const short FUND_B_HSL		= 245;			//B基金换手率
const short FUND_BFE_INC	= 246;			//B基金份额新增(万份)
const short FUND_MPRICE		= 247;			//母基价格
const short FUND_MZAF		= 248;			//母基涨幅
const short FUND_MYJL		= 249;			//母基溢价率
const short FUND_MABYJL		= 250;			//母子溢价率
const short FUND_ENDDATE	= 251;			//基金到期日期
const short FUND_M_TYPE     = 252;   		//基金类型
const short FUND_END		= 252;			//基金排序终止列
//客户端已用const short TIPDIS			= 253;			//预警提示列
const short ZGCODE			= 254;			//正股代码
const short ZGNAME			= 255;			//正股名称
const short ZGPRICE_0		= 256;			//正股价
const short ZGZAF			= 257;			//正股涨幅
const short ZGASSETS		= 258;			//正股净值产
const short ZGRATE			= 259;			//正股市净率
const short ZGQSR			= 260;			//转股起始日
const short ZGPRICE_1		= 261;			//转股价
const short ZGEVALUE		= 262;			//转股价值
const short YJRATE			= 263;			//溢价率
const short HSCFB			= 264;			//回售触发比
const short HSCFPRICE		= 265;			//回售触发价
const short HSPRICE			= 266;			//回售价
const short HSQSR			= 267;			//回售起始日
const short QSCFB			= 268;			//强赎触发比
const short QSCFPRICE		= 269;			//强赎触发价
const short QSPRICE			= 270;			//强赎价
const short QSQSR			= 271;			//强赎起始日
const short BONDAMT			= 272;			//债券规模
const short RESTAMT			= 273;			//剩余规模
const short ZZRATE			= 274;			//转债占比
const short ZZSTARTDATE		= 275;			//转债发行日期
const short ZZENDDATE		= 276;			//转债到期日期
const short RESTDAY			= 277;			//剩余年限
const short INTEREST		= 278;			//利息
const short SQSY			= 279;			//税前收益
const short SHSY			= 280;			//税后收益
const short ZTJG			= 281;			//行情涨停价
const short DTJG			= 282;			//行情跌停价
const short GG_F10			= 283;			//个股资讯
const short GPCTIME			= 284;			//股票池入选时间
const short GPCPRICE		= 285;			//股票池入选价格
const short GPCZAF			= 286;			//股票池入选涨幅
const short GPC5DAYZAF		= 287;			//股票池入选后5日最大涨幅
const short HQ_204007_NOW	= 288;			//国债逆回购利率204007行情现价
const short HQ_204007_ZAF	= 289;			//国债逆回购利率204007行情涨幅
const short HQ_000012_NOW	= 290;			//国债利率指数000012最新指数
const short HQ_000012_ZAF	= 291;			//国债利率指数000012行情涨幅
const short FUND_TLKJ		= 292;			//套利空间 整溢≥0，套利空间=整溢-申购费率；整溢＜0，套利空间=0-整溢-赎回费率；整溢为无效数，套利空间为无效数。

const short ZAF_5MIN		= 293;			//5分钟涨幅
const short ZAF_3DAYS		= 294;			//3日涨幅
const short ZAF_5DAYS		= 295;			//5日涨幅
const short ZAF_20DAYS		= 296;			//20日涨幅
const short ZANG_DAYS		= 297;			//连涨天数
const short HIGH_20DAYS		= 298;			//20日最高价
const short LOW_20DAYS		= 299;			//20日最低价
const short HIGH_HIS		= 300;			//历史最高价
const short LOW_HIS			= 301;			//历史最低价
const short ZAF_90DAYS		= 302;			//3个月涨幅
const short ZAF_180DAYS		= 303;			//半年涨幅
const short ZAF_365DAYS		= 304;			//一年涨幅


const short MONEY_BIGIN		= 305;			//资金大单流入
const short MONEY_BIGOUT	= 306;			//资金大单流出
const short MONEY_PUREBIG	= 307;			//资金大单净额
const short MONEY_MIDIN		= 308;			//资金中单流入
const short MONEY_MIDOUT	= 309;			//资金中单流出
const short MONEY_PUREMID	= 310;			//资金中单净额
const short MONEY_SMALLIN	= 311;			//资金小单流入
const short MONEY_SMALLOUT	= 312;			//资金小单流出
const short MONEY_PURESMALL	= 313;			//资金小单净额
const short MONEY_PUREOUT	= 314;			//资金净流出

const short DIE_DAYS		= 315;			//连跌天数
const short ZB_STATE0		= 316;			//指标0输出
const short ZB_STATE1		= 317;			//指标1输出
const short ZB_STATE2		= 318;			//指标2输出
const short ZB_STATE3		= 319;			//指标3输出
const short ZB_STATE4		= 320;			//指标4输出

const short BS_1MIN			= 321;			//BS计算1分钟
const short BS_5MIN			= 322;
const short BS_15MIN		= 323;
const short BS_30MIN		= 324;
const short BS_60MIN		= 325;
const short BS_DAY			= 326;
const short BS_WEEK			= 327;
const short BS_MONTH		= 328;

const short STATE_1MIN		= 329;
const short STATE_5MIN		= 330;
const short STATE_15MIN		= 331;
const short STATE_30MIN		= 332;
const short STATE_60MIN		= 333;
const short STATE_DAY		= 334;
const short STATE_WEEK		= 335;
const short STATE_MONTH		= 336;

const short POSITION_MANAGE	= 337;

const short POSITIONEX_CODE	= 338;
const short POSITIONEX_NAME = 339;
const short POSITIONEX_ZAF	= 340;
const short POSITIONEX_NOW	= 341;
const short POSITIONEX_ZD	= 342;
const short POSITIONEX_POS	= 343;
const short PEX_STATE_1MIN	= 344;
const short PEX_STATE_5MIN	= 345;
const short PEX_STATE_15MIN	= 346;
const short PEX_STATE_30MIN	= 347;
const short PEX_STATE_60MIN	= 348;
const short PEX_STATE_DAY	= 349;
const short PEX_STATE_WEEK	= 350;
const short PEX_STATE_MONTH	= 351;

const short PEX_PON_INDUSTRY= 352;
const short PEX_PON_AREA	= 353;
const short PEX_PON_CONCEPT = 354;
const short PEX_PON_CONCODE	= 355;

const short FINANCE_STATE	= 356;
const short DAYCALC_K		= 357;
const short DAYCALC_K1		= 358;
const short DAYCALC_K2		= 359;
const short DAYCALC_K3		= 360;
const short DAYCALC_TS		= 361;
const short DAYCALC_ZF		= 362;
const short MMPERIOD_DT5	= 363;
const short MMPERIOD_DT1	= 364;

const short STRATEGY_EARN	= 365;
const short POOL_IN			= 366;
const short POSITION_SCORE	= 367;

const short STRATEGY_GROW	= 368;
const short STRATEGY_SAFE	= 369;
const short STRATEGY_MAIN	= 370;
const short STRATEGY_EVA	= 371;
const short STRATEGY_DIV	= 372;

const short RPS_WEEK1		= 373;
const short RPS_WEEK2		= 374;
const short RPS_WEEK4		= 375;
const short RPS_WEEK6		= 376;
const short RPS_WEEK13		= 377;
const short RPS_WEEK26		= 378;
const short RPS_WEEK52		= 379;
const short RPSMA_WEEK1		= 380;
const short RPSMA_WEEK2		= 381;
const short RPSMA_WEEK4		= 382;
const short RPSMA_WEEK6		= 383;
const short RPSMA_WEEK13	= 384;
const short RPSMA_WEEK26	= 385;
const short RPSMA_WEEK52	= 386;
const short RPS_SHORT		= 387;
const short RPS_MID			= 388;
const short RPS_LONG		= 389;

const short HYPJ_ZHPF		= 390;
const short BLOCK_ZTNUM		= 391;
const short BLOCK_DTNUM		= 392;
const short ZAF_10DAYS		= 393;

const short STK_NKEY = 396;
const short J_SJL = 397;

const short MAXENDCOL		= 397;
// new append
const short OPEN_P = 400;			// 开盘% ：（开盘 - 昨收） / 昨收 * 100
const short HIGH_P = 401;			// 最高% ：（最高 - 昨收） / 昨收 * 100
const short LOW_P = 402;			// 最低% ：（最低 - 昨收） / 昨收 * 100
const short ZF_New = 403;			// 实体涨幅：（现价 - 今开） / 今开 * 100
const short UNDERTOW = 404;			// 回头波:（现价 - 最高） / 最高 * 100
const short ATTACKWAVE = 405;		// 攻击波 : （现价 - 最低） / 最低 * 100
const short ZXG_PRICE = 406;		// *自选价 : 当时记录自选日字段时候的现价
const short ZXG_PROFIT = 407;		// *自选收益：（现价 - 自选价） / 自选价 * 100
const short DD_TCOUNT = 408;		// 成交笔数
const short SUPER_IN = 409;			// 超大单买入
const short BIG_IN = 410;			// 大单买入
const short BUY_PROFIT = 411;		// 买入比例
const short SUPER_HSL = 412;		// 超大单换手率
const short BIG_HSL = 413;			// 大单换手率
const short EACH_AMOUNT = 414;		// 每笔金额
const short EACH_GU = 415;			// 每笔股数
// 



//帮助宏(值一定要和帮助的HID相对应)
#define HID_QIANYAN		40000

#define UM_BCSTRECEIVE		WM_USER+1110
#define UM_CLOSECLIENT		WM_USER+1111
#define IPXSOCKET_MESSAGE   WM_USER+1112
#define UM_SETLISTINF       WM_USER+1113
#define UM_UDPRECEIVE		WM_USER+1114
#define UM_TAPIADDSUB		WM_USER+1115

#define UM_CLOSESERVER		WM_USER+1116
#define UM_STARTSERVER		WM_USER+1117
#define UM_STOPSERVER		WM_USER+1118

#define UM_INFO				WM_USER+1200
#define UM_CARRYDOWN		WM_USER+1201
#define UM_PROGRESS			WM_USER+1202

#define UDPSOCKET_MESSAGE	WM_USER+1204
#define SOCKETBUF_MESSAGE	WM_USER+1205

#define UM_NEWDATEINIT		WM_USER+1300

//用于综合排名
#define ZH_ZANGF	0
#define ZH_ZENGF	1
#define ZH_FZZANG	2
#define ZH_LIANB	3
#define ZH_WT		4
#define ZH_JE		5
#define ZH_NOW		6

/////////////////////////////////////////////////////////////////////
#define ERR_NOTYPE		11
#define ERR_NOINMODE	12
#define ERR_NOUSER		13
#define ERR_ERRPASSWORD	14
#define ERR_INVALID		15
#define ERR_EXCEEDUSER	16
#define ERR_ERRNETCARD	17
#define ERR_ERRTIME		18
#define ERR_ERRSERIAL	19
#define ERR_ERRVER		20
#define ERR_OTHER		21

/////////////////////////////////////////////////////////////////////

//服务类型
#define WT_SERTYPE		1
#define HQ_SERTYPE		2
#define CHAT_SERTYPE	3
#define TAPI_SERTYPE	4
#define UDP_SERTYPE		5

//拨入方式
#define OLDFY_INMODE	0
#define DOS_INMODE		1
#define PC_INMODE		2
#define WEB_INMODE		3
#define PDA_INMODE		4
#define WIDE_INMODE		5
#define PLAY_INMODE		6	//播放器
#define JDH_INMODE		7	//机顶盒



#define WT_Q			0
#define HQ_Q			1

/////////////////

// 证券类型	
#define CODE_SZAG       0		// 深圳A股
#define CODE_SZQZ       1		// 深圳权证
#define CODE_SZGZ       2		// 国债
#define CODE_SZZQ       3		// 债券(多地方债)
#define CODE_SZKZHZQ    4		// 可转换债券
#define CODE_SZGZHG     5		// 国债回购
#define CODE_SZJJ       6		// 基金
#define CODE_SZBG       7		// B股
#define CODE_SZCY       8		// 中小板（以前的创业板说法)
#define CODE_SZOTHER    9		// 其他类型

#define CODE_SHAG      10
#define CODE_SHQZ      11
#define CODE_SHGZ      12
#define CODE_SHZQ      13
#define CODE_SHKZHZQ   14
#define CODE_SHGZHG    15
#define CODE_SHJJ      16
#define CODE_SHBG      17
#define CODE_SHOTHER   18

#define CODE_KFJJ	   19	//开放式基金
#define CODE_SB		   20	//三板

#define CODE_SZSPEC    22	// 特别转让，即将停牌
#define CODE_SHSPEC    23   
#define CODE_SZ300CY   24	// 300 开头的创业板

#define CODE_KCB		25

#define CODE_GZLWTSAG	26	//两网公司及退市公司 A 股，400*
#define CODE_GZLWTSBG	27	//两网公司及退市公司 B 股，420*
#define CODE_GZGPSS		28	//挂牌/上市公司股票，43*、83*、87*
#define CODE_GZTSKZH	29	//退市公司可转换公司债券，404*
#define CODE_GZKZHZQ	30	//可转换公司债券，81*
#define CODE_GZYXG		31	//优先股，820*
#define CODE_GZYYSG		32	//要约收购，840*
#define CODE_GZYYGG		33	//要约回购，841*
#define CODE_GZGQJLQQ	34	//股权激励期权，850*
#define CODE_GZFXYW		35	//发行业务，889*
#define CODE_GZBJZS		36	//股转北交 指数，899*

#define ERR_CHECKGD_FAIL			2001
#define ERR_WTHOST_TIMEOUT			2002
#define ERR_GETZJZH_FAIL			2003

#define MAX_NETCARD			1000

#endif
