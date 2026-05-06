

#ifndef   STRUC_BEST_H
#define   STRUC_BEST_H

#include "macro.h"
#include "struct.h"
#include <stdint.h>
#include "KdscStruct.h"


#pragma pack(push, 1)

#define MMP_COUNT 10

typedef enum MARKETBLOCK_ST
{
	SHAG_S = 1,			//上海 A 股
	SHKCB_S,			//上海科创板
	SHETF_S,			//上海 ETF
	SZAG_S,				//深圳 A 股
	SZZXQY_S,			//深圳中小企业
	SZCYB_S,			//深圳创业板
	SZETF_S				//深圳 ETF
}MARKETBLOCK_S;

typedef struct AnalyRadarData_ST
{
	unsigned __int64 nkey;		// nkey
	__int64			jclTime;	// 时间
	char			code[VAR_CODE_LENB + 1];// 股票代码
	double			fHigh;		// 单位最高价
	double			fLow;		// 单位最低价
	double			fClose;		// 单位收盘价
	MARKETBLOCK_S	block;		// 板块

	AnalyRadarData_ST()
	{
		memset(this, 0, sizeof(AnalyRadarData_ST));
	}
}AnalyRadarData_S;

// 大数据采用google快速压缩
// 量比是衡量相对成交量的指标。它是指股市开市后平均每分钟的成交量与过去5个交易日平均每分钟成交量之比。
// 其计算公式为：量比=（现成交总手数 / 现累计开市时间(分) ）/ 过去5日平均每分钟成交量.
// 量比反映出的主力行为从计算公式中可以看出，量比的数值越大，表明了该股当日流入的资金越多，市场活跃度越高


//2023 支持的扩展字段推送, 精简版, 约定：字段下标依次返回定义的顺序

struct LastInfoB
{
  char          Code[VAR_CODE_LENB+1];   // 证券代码
  double	    VolBase;               // 量比的基量
  double        YClose;                 // 昨日收盘价
  double        YBalancePrice;          // 昨日结算价(期货)
  double		VolInStock;            // 昨日持仓量(期货)
}PACKED;

struct LasAverB
{
	char		Code[VAR_CODE_LENB+1];//code
	double		LasAverage;//上一分钟均价
	short		IniMinutes;//初始化开盘分钟数
	short       IniTimes;//初始化时间
	short       NowTimes;//当前时间
};

#define PKNUM	10

//struct CurrStockDataB     // 在.IHQ文件中读入的数据结构 Modified 2003.9.29
//{
//	unsigned __int64	nKey;			    // 唯一ID号(唯一硬编码)
//	int					SetCode;
//	__int64				jclTime;// 时间精确到毫秒
//	//////////////////////////////////////////////////////////////////////////
//	DWORD				dwItemNum;               // 采样点数
//	// float兼顾空间大小，精度有限，转换根据具体情况定，不足精度用double
//	double				fClose;		// 昨日余额
//	double				fOpen;		// 今开盘余额
//	double				fHigh;		// 最高余额
//	double				fLow;		// 最低余额
//	double				fNow;		// 当前余额
//	double				fLead;		// 领先指标(指数)
//	double				fAverage;	// 均价
//	double				fSettlePrice;// 结算价(期货期指现货特有)
//	double				fOpenInterest;	// 开仓量 持仓量(期货期指特有)
//	double				fVolume;			// 总手		387	成交总量",  TotalVolumeTrade
//	double				fNowVol;			// 现手(总手差)
//	double				fAmount;			//  成交总金额 TotalValueTrade ;成交均价=成交金额/(成交量*wTradeUnit)
//	double				fNowAmt;			// 为了简化，客户端不要考虑Unit单位，直接推送差额
//	double				fIn;				// 内盘  
//	double				fOut;				// 外盘 
//	double              fUnknown;           // 不明盘
//	double				fTickDiff;			// 笔升跌(价位差)
//
//	double				fYield;           // 市盈率1,不含加权的指数
//	double				fVolInStock;      // 持仓量
//	union
//	{
//		double			f_Buy;					// 最高叫买价
//		double			fBuy;
//		double			f_Bid;					// 叫买 /指数领先指标  如果是债券,则存放利息
//		double			fOpenVolume;				// 开仓量
//		double			f_AdvStop;				// 涨停价
//	};
//	union
//	{
//		double			f_Sell;					// 最低叫卖价,领先指标(指数)
//		double			fSell;
//		//float			Lead;					// 领先指标(指数)
//		double			f_Lead;					// 价格统一替换
//		double			f_Ask;					// 叫卖	//排序的时候,存放排序值
//		double			f_Offer;					// 叫卖	//排序的时候,存放排序值
//		double			f_DecStop;				// 跌停价
//	};
//	union {
//		double				fBuyp[PKNUM];
//		// 基金成交金额:Buyp[0]
//		// 债券成交金额:Buyp[1]
//		// 国债成交金额:Buyp[2]
//		struct{
//			double			fJJAmtSum;
//			double			fZQAmtSum;
//			double			fGZAmtSum;
//		};
//	};
//	union {
//		double				Buyv[PKNUM];
//		// 上涨家数:Buyv[0]
//		//BG收入笔Buyv[9]
//		struct{
//			long			nUpStocks;
//		};
//	};
//	union {
//		double				fSellp[PKNUM];
//		// 权证成交金额:fSellp[0]
//		// 期货昨持仓量:fSellp[1]
//		// 指数其它成交金额:fSellp[1]
//		// 期货昨结算价:fSellp[2]   ==>>  fPreSettlePrice
//		// 指数昨不含加权指数:fSellp[2]
//		struct{
//			double			fQZAmtSum;
//			union{
//				double		fQHCCLSum;			// fOpenInterest 之和
//				double		fOtherAmtSum;
//			};
//			union{
//				double		fZSPreAVIndex;
//			};
//		};
//	};
//	union {
//		double				Sellv[PKNUM];
//		// 下跌家数:Sellv[0]
//		struct{
//			long			nDownStocks;
//		};
//	};
//
//	char				InOutFlag;			// 内外盘标志,
//	char				sdunit;				// 笔升跌单位(用于计算领先指标) 跌停 跌停等 3 -3 2 -2 ....
//	union
//	{
//		short			RestVol;			// 零股
//		WORD			YQSY;				// 国债预期收益
//	};
//	__int64				Reserved;
//
//	CurrStockDataB()
//	{
//		memset(this,0,sizeof(CurrStockDataB));
//	}
//	unsigned __int64	getKey()
//	{
//		return nKey;
//	}
//};

//struct CurrStockDataExB
//{
//	char		SetCode;
//	char        Code[VAR_CODE_LENB];
//	struct CurrStockDataB hq;		   // 原始行情
//	char        dkflag;				   // 
//	double      tbp;                   // 多空平衡点	
//	double      stoplost;              // 止损点
//	double      leave;                 // 了结点
//	double		zangsu;			   // 涨速
//	unsigned short	nWarnCount;			   // (活跃度)报警次数
//};
//struct CurrStockDataExStandB
//{
//	char		SetCode;
//	char        Code[VAR_CODE_LENB+1];
//	CurrStockDataB hq;		   // 原始行情
//	char        dkflag;				   // 
//	double      tbp;                   // 多空平衡点
//	double      stoplost;              // 止损点
//	double      leave;                 // 了结点
//	double		zangsu;			   // 涨速
//	unsigned short	nWarnCount;			   // (活跃度)报警次数
//};
// float  : 1bit（符号位） 8bits（指数位） 23bits（尾数位）
// double : 1bit（符号位） 11bits（指数位） 52bits（尾数位）
// float和double的精度是由尾数的位数来决定的
// float：2^23 = 8388608，一共七位，这意味着最多能有7位有效数字，但绝对能保证的为6位，也即float的精度为6~7位有效数字；
// double：2^52 = 4503599627370496，一共16位，同理，double的精度为15~16位。


/*
// 数据源输入，格式也要改成double推送，暂时兼顾上游不动，用float
struct	DiskInfIhqB : public DiskStkInfoB,CurrStockDataBEx	// 数据源PEEKCTP
{
	DiskInfIhqB()
	{
		memset(this,0,sizeof(DiskInfIhqB));
	}
};
*/

// 数据源输入，格式也要改成double推送，暂时兼顾上游不动，用float
struct	DiskInfIhqB : public DiskStkInfoB// ,CurrStockDataB	// 数据源PEEKCTP
{
	//CurrStockDataB 去掉前面重复字段
	//unsigned __int64	nKey;			    // 唯一ID号(唯一硬编码)
	//int					SetCode;		// 方便同时从disk和currenttosk派生
	//__int64				jclTime;// 时间精确到毫秒
	//////////////////////////////////////////////////////////////////////////
	DWORD				dwItemNum;               // 采样点数
	// float兼顾空间大小，精度有限，转换根据具体情况定，不足精度用double
	double				fClose;		// 昨日余额
	double				fOpen;		// 今开盘余额
	double				fHigh;		// 最高余额
	double				fLow;		// 最低余额
	double				fNow;		// 当前余额
	double				fLead;		// 领先指标(指数)
	double				fAverage;	// 均价
	double				fSettlePrice;// 结算价(期货期指现货特有)
	double				fOpenInterest;	// 开仓量 持仓量(期货期指特有)
	double				fVolume;			// 总手		387	成交总量",  TotalVolumeTrade
	double				fNowVol;			// 现手(总手差)
	double				fAmount;			//  成交总金额 TotalValueTrade ;成交均价=成交金额/(成交量*wTradeUnit)
	double				fNowAmt;			// 为了简化，客户端不要考虑Unit单位，直接推送差额
	double				fIn;				// 内盘  
	double				fOut;				// 外盘 
	double              fUnknown;           // 不明盘
	double				fTickDiff;			// 笔升跌(价位差)

	double				fYield;           // 市盈率1,不含加权的指数
	double				fVolInStock;      // 持仓量
	union
	{
		double			f_Buy;					// 最高叫买价
		double			fBuy;
		double			f_Bid;					// 叫买 /指数领先指标  如果是债券,则存放利息
		double			fOpenVolume;				// 开仓量
		double			f_AdvStop;				// 涨停价
	};
	union
	{
		double			f_Sell;					// 最低叫卖价,领先指标(指数)
		double			fSell;
		//float			Lead;					// 领先指标(指数)
		double			f_Lead;					// 价格统一替换
		double			f_Ask;					// 叫卖	//排序的时候,存放排序值
		double			f_Offer;					// 叫卖	//排序的时候,存放排序值
		double			f_DecStop;				// 跌停价
	};
	union {
		double				fBuyp[PKNUM];
		// 基金成交金额:Buyp[0]
		// 债券成交金额:Buyp[1]
		// 国债成交金额:Buyp[2]
		struct {
			double			fJJAmtSum;
			double			fZQAmtSum;
			double			fGZAmtSum;
		};
	};
	union {
		double				Buyv[PKNUM];
		// 上涨家数:Buyv[0]
		//BG收入笔Buyv[9]
		struct {
			long			nUpStocks;
		};
	};
	union {
		double				fSellp[PKNUM];
		// 权证成交金额:fSellp[0]
		// 期货昨持仓量:fSellp[1]
		// 指数其它成交金额:fSellp[1]
		// 期货昨结算价:fSellp[2]   ==>>  fPreSettlePrice
		// 指数昨不含加权指数:fSellp[2]
		struct {
			double			fQZAmtSum;
			union {
				double		fQHCCLSum;			// fOpenInterest 之和
				double		fOtherAmtSum;
			};
			union {
				double		fZSPreAVIndex;
			};
		};
	};
	union {
		double				Sellv[PKNUM];
		// 下跌家数:Sellv[0]
		struct {
			long			nDownStocks;
		};
	};

	char				InOutFlag;			// 内外盘标志,
	char				sdunit;				// 笔升跌单位(用于计算领先指标) 跌停 跌停等 3 -3 2 -2 ....
	union
	{
		short			RestVol;			// 零股
		WORD			YQSY;				// 国债预期收益
	};
	__int64				Reserved;

	DiskInfIhqB()
	{
		memset(this, 0, sizeof(DiskInfIhqB));
	}
};


union long_shortB
{    
	long    Date;                     // 日期YYYYMMDD(日线)
//       struct
//       {                                 // 分钟线
// 	      short Day;                    // 日
// 	      short Minute;                 // 零点以来的分钟数
//       }       Daye;
	struct
	{
		WORD Minute:16;         // 零点以来的分钟数
		WORD Mon_Day:11;        //月日
		WORD Year:5;			//年, 离2004的年数
	}Daye1;

}PACKED;


// java  System.currentTimeMillis()
// System.currentTimeMillis()产生一个当前的毫秒，这个毫秒其实就是自1970年1月1日0时起的毫秒数
// 获得的是自1970-1-01 00:00:00.000 到当前时刻的时间距离
struct JCL_TimeTool 
{
	// 到目前为止的毫秒数
	static		__int64		GetMilliseconds()
	{
		__int64	lastMilliseconds=0;
	
		time_t clocksec;
		struct tm tm;
		SYSTEMTIME wtm;
		GetLocalTime(&wtm);
		tm.tm_year     = wtm.wYear - 1900;
		tm.tm_mon     = wtm.wMonth - 1;
		tm.tm_mday     = wtm.wDay;
		tm.tm_hour     = wtm.wHour;
		tm.tm_min     = wtm.wMinute;
		tm.tm_sec     = wtm.wSecond;
		tm. tm_isdst    = -1;
		clocksec = mktime(&tm);

		lastMilliseconds = clocksec*1000 + wtm.wMilliseconds;

		return lastMilliseconds;
	}
};
// 3 short 3*16  
// union		JCL_DateTime
// {
// 	unsigned	__int64		uTime;		// 方便进行值得大小比较
// 
// 	struct		
// 	{
// 		WORD	Milliseconds:10;	// 0 --1024
// 		WORD	Second:6;		// 0 -- 64
// 
// 		WORD	Minute:6;		// 0 -- 64
// 		WORD	Hour:5;			// 
// 		WORD	Day:5;			// 0 -- 32
// 
// 		WORD	Month:4;		// 0 -- 16
// 		WORD	Year:12;		// 0 -- 4096
// 
// 
// 	};
// };

// 3 short 3*16  
// struct		JCL_DateTime
// {
// 	WORD	Milliseconds:10;	// 0 --1024
// 	WORD	Second:6;		// 0 -- 64
// 
// 	WORD	Minute:6;		// 0 -- 64
// 	WORD	Hour:5;			// 0 -- 32
// 	WORD	Day:5;			// 0 -- 32
// 
// 	WORD	Month:4;		// 0 -- 16
// 	WORD	Year:12;		// 0 -- 4096
// 	JCL_DateTime()
// 	{
// 		memset(this,0,sizeof(JCL_DateTime));
// 	}
// 	friend bool operator<(const JCL_DateTime &a,const JCL_DateTime &b)//定义struct比较方法
// 	{
// 		unsigned __int64 ia = a.Year;
// 		unsigned __int64 ib = b.Year;
// 		// 18446744073709551616
// 		ia = ((ia*10000 + ((int)a.Month)*100 + a.Day)*1000000 + ((int)a.Hour)*10000 + ((int)a.Minute)*100 + a.Second)*1000+a.Milliseconds;
// 		ib = ((ib*10000 + ((int)b.Month)*100 + b.Day)*1000000 + ((int)b.Hour)*10000 + ((int)b.Minute)*100 + b.Second)*1000+b.Milliseconds;
// 		
// 		return ia < ib;
// 	}
// };
// 
// struct  JCL_DateTimeM
// {
// 	/// 方案二
// 	DWORD	Reserv:4;
// 	DWORD	Milliseconds:10;			// 1024
// 	DWORD	lTime:18;					// 0 -- 262144  HHMMSS  235959
// 
// 	DWORD	lDate;                     // 日期YYYYMMDD(日线)
// 	JCL_DateTimeM()
// 	{
// 		memset(this,0,sizeof(JCL_DateTimeM));
// 	}
// };

// gettimeofday


// 单独增加盘口数据结构,直接对行情源进行录制
// 122 字节: 最多1500分钟，每分钟60笔(SF有120笔)，一天90000笔，10980000字节，一个品种每天10.5M
// 每个品种一个文件，分开存放;一个月200M，一年2G；
typedef struct TickDataB      // 在.TIC文件中读入的数据结构
{ 
	__int64			jclTime;			// 时间精确到毫秒
	double			fNow;               // 现价*1000
	double			fNowVol;            // 现手
	double			fNowAmount;			// BG复用为当前上存或下拨值
	////////////////////盘中完全取代盘口////////////////////////////////
	//	union {DWORD	fVolume; DWORD	Volume;};   // 总手		387	成交总量",  TotalVolumeTrade
	//	union {double   fAmount; double Amount;};		// 总成交金额 8504	成交总金额 TotalValueTrade ;成交均价=成交金额/(成交量*wTradeUnit)
	//	union {DWORD   fIn;DWORD Inside;};        // 内盘  国外不一样,没有买卖盘.但有up/down.
	//	union {DWORD   fOut;DWORD Outside;};      // 外盘 
	//////////////////////////////////////////////////////////////////
	double			VolInStock_dif;        //  // 持仓量增减(大盘存zsNowVol)
	char			InOutFlag;             // BG收入支出标志,0，收入，1支出
	//	union { long	l_Buyp[5];	float	fBuyp[5];	};
	//	DWORD		   BuyV[5];
	//	union { long   l_Sellp[5];   float	fSellp[5];	};
	//	DWORD		   SellV[5];	
	TickDataB()
	{
		memset(this,0,sizeof(TickDataB));
	}
} TICKDATAB;

typedef struct AuctionDataB      // 在.TIC文件中读入的数据结构
{ 
	__int64			jclTime;			// 时间精确到毫秒
	double			fNow;                   // 现价*1000
	double			fNowVol;                // 现手
	AuctionDataB()
	{
		memset(this,0,sizeof(AuctionDataB));
	}
} AUCTIONDATAB;

typedef struct AuctionDataBEx : public AuctionDataB
{
	double	fUnsuitVol;	// 未匹配量
	int		nUnsuitBS;	// 未匹配方向 0:卖 1:买 2:未匹配量为0，没有方向
} AUCTIONDATABEX;


/*
typedef struct TickDataOfMtc      // 在.TIC文件中读入的数据结构
{ short          Minute;                // 零点以来的分钟数
  long			 Now;                   // 现价*1000
  long			 NowVol;                // 现手
  long           VolInStock_dif;        // 持仓量增减
  short          InOutFlag;             // 买卖标志
  long           Lead;                  //指数领先指标
  DWORD			 Buyv;          //叫买量　上涨家数　　　　叫买量
  DWORD			 Sellv;         //叫卖量　下跌家数　　　　叫卖量　
  float          NowAmount;
} TICKDATAOFMTC;
*/

typedef struct  MinuteDataB                      // 在.IFZ文件中读入的数据结构1
{ 
	DWORD			dwItemNum;               // 采样点数	： 0 表示这个周期内，行情没有变动
	__int64			jclTime;				 // 时间精确到毫秒
	//	float Now;                            // 现价,现指数
	// 1分钟的K线可以直接这里得到，X分钟的就可以灵活转变
	double			fOpen;	// 今开盘余额
	double			fHigh;	// 最高余额
	double			fLow;	// 最低余额
	double			fNow;	// 当前余额

	double			fAverage;	// 均价,动态结算价(期货),不含加权的指数
	double			fSettlePrice;// 结算价(期货期指现货特有)
	double			fNowVol;                         // 现手,现成交金额(100元)，一分钟内的成交量之和
	// -- 改为当前分钟内的
	double			fAmount;						// BG复用为总成交额，包含上村或下拨和收入或支出
	double			Buyv;                           // 买量,上涨家数
	double			Sellv;                          // 卖量,下跌家数
	double			fOpenInterest;					// 持仓量（订货量)  VolInStock
	double			Lead;                       // 领先指标(指数)

}PACKED MINDATAB;


struct MinuteDataDLB //转码机到主站的推送
{
	unsigned __int64	nKey;			    // 唯一ID号(唯一硬编码)
	MinuteDataB mindata;
};

// 不能直接用index索引,那个必须换日，而且要重新读取文件
//struct CurrStockDataDLB		// 独立行情
//{
//	union	{
//		WORD		m_wMarket;									// 股票市场类型
//		WORD		SetCode;
//	};
//	union	{
//		char		m_szLabel[VAR_CODE_LENB+1];					// 股票代码,以'\0'结尾
//		char		Code[VAR_CODE_LENB+1];
//	};
//	unsigned __int64	nKey;			    // 唯一ID号(唯一硬编码)
//	CurrStockDataB	hq;
//};


struct MinuteDataArrayB 
{
	unsigned __int64	nKey;
	MinuteDataB				m[1500];	// 最多1444分钟
	AnalyDataB				m5[300];//最多300个5min
	short						icurMin;//0点以来的分钟数
	unsigned __int64	getKey()
	{
		return nKey;
	}
	MinuteDataArrayB()
	{
		memset(this,0,sizeof(MinuteDataArrayB));
	}
};

struct AnalyDataDLB
{
	union	{
		WORD		m_wMarket;									// 股票市场类型
		WORD		SetCode;
	};
	union	{
		char		m_szLabel[VAR_CODE_LENB+1];					// 股票代码,以'\0'结尾
		char		Code[VAR_CODE_LENB+1];
	};
	unsigned __int64	nKey;			    // 唯一ID号(唯一硬编码)
	AnalyDataB analydata;
};


struct  PriceVolTableB
{ 
	double  Price;
	double	Volume;
	double	Kcl;
	double	Pcl;
};

struct MARKETSIMPLEINF
{
	short		setcode;
	__int64		jclTime;
	short		nFZ[8];		// 开盘时段
	WORD		nStockNum;	// 品种个数
	__int64		infhms;		//代码链更新时间
	unsigned __int64 codehash; //代码链Hash值
	int			yesflag;	//是否开盘标志
	MARKETSIMPLEINF()
	{
		memset(this,0,sizeof(MARKETSIMPLEINF));
	}
};

//主站基本信息
struct HISALLSIMPLEINF
{
	short		setcode;
	long		nCwReadDate;
	HISALLSIMPLEINF()
	{
		memset(this,0,sizeof(HISALLSIMPLEINF));
	}
};

struct PositionManage
{
	double bs_zb[8];
	double state[8];
	int pos;
	double position;
	double score;
	void SetScore()
	{
		int newstate[8];
		for (int i=0;i<8;++i)
		{
			if (state[i] == 6) newstate[i] = 0;
			else if (state[i] > 6) newstate[i] = state[i] - 1;
			else newstate[i] = state[i];
		}
		int sum = newstate[0] + newstate[1] * 2 + newstate[2] * 3 + newstate[3] * 3 + newstate[4] * 2 + newstate[5];
		score = sum * 100.f / 84;
	}
	void SetPosition()
	{
		if ((pos & 0XE0) == 0xE0)
		{
			position = 300;
		}
		else if ((pos & 0x70) == 0x70)
		{
			position = 200;
		}
		else if ((pos & 0x38) == 0x38)
		{
			position = 100;
		}
		else if ((pos & 0x1C) == 0x1C)
		{
			position = 60;
		}
		else if ((pos & 0x0E) == 0x0E)
		{
			position = 30;
		}
		else if ((pos & 0x07) == 0x07)
		{
			position = 15;
		}
		else
		{
			position = 0;
		}
	}
	PositionManage()
	{
		memset(this,0,sizeof(PositionManage));
	}
};

struct PositionStruct
{
	UINT64 nkey;
	int periodIdx;
	int bs_zb;
	int state;
	bool position;
};

struct DayCalcData
{
	int state;
	double k;
	double k1;
	double k2;
	double k3;
	int days;
	double zaf;
};

struct StrategyData
{
	double earn;
	double grow;
	double safe;
	double mainforce;
	double dividends;
	double evaluation;
	double reserve[4];
	StrategyData()
	{
		earn = _Nan._Double;
	}
};

struct FinancePushStruct
{
	UINT64 nkey;
	int state;
	double k;
	double k1;
	double k2;
	double k3;
	int days;
	double zaf;
};

struct DTPushStruct
{
	UINT64 nkey;
	double dt5;
	double dt1;
};

struct DTData
{
	double dt5;
	double dt1;
};

struct EarnData
{
	UINT64 nkey;
	double value;
};

struct rps_push : public tagCode
{
	double score[14];
};

struct zhpf_data
{
	UINT64 nkey;
	double score;
};

struct PoolPushStruct
{
	UINT64 nkey;
	bool bIn;
	int value;
};

//struct CalcZafData
//{
//	short setcode; //0:SZ 1:SH
//	unsigned __int64 nKey;
//
//	double zaf_5min;
//	double zaf_3days;
//	double zaf_5days;
//	double zaf_20days;
//	double zang_days;
//	double die_days;
//
//	double high_20days;	
//	double low_20days;	
//	double high_his;
//	double low_his;
//
//	double zaf_90days;
//	double zaf_180days;
//	double zaf_365days;
//
//	double zb_state[5];
//	CalcZafData()
//	{
//		zaf_5min = zaf_3days = zaf_5days = zaf_20days = high_20days = low_20days = high_his = low_his = zaf_90days = zaf_180days = zaf_365days = _Nan._Double;
//		zang_days = die_days = _Nan._Double;
//		zb_state[0] = zb_state[1] = zb_state[2] = zb_state[3] = zb_state[4] = _Nan._Double;
//	}
//};

typedef struct __tqzf_push__
{
	UINT64 nkey;
	double zaf_3;
	double zaf_5;
	double zaf_10;
	double reserv[3];
}TQZAF;

struct push_zbstate // 推送计算指标的结果
{
	uint64_t nkey;	//品种nkey
	int type;		//类型 短线还是中线长线之类的
	double zb;		//指标输出1
};


struct strategy_zb_data : public tagCode
{
	double value;
	double price;
	double zaf;
};

struct LoadDisData
{
public:
	short m_setcode;
	__int64 m_infhms;
	std::vector<DiskStkInfoB*> m_data;
public:
	LoadDisData() :m_setcode(0), m_infhms(0)
	{
		m_data.clear();
	}
	~LoadDisData()
	{
		m_setcode = 0;
		m_infhms = 0;
		std::vector<DiskStkInfoB*>emp;
		m_data.swap(emp);
		m_data.clear();
	}
};



#pragma pack(pop) 


#endif
