#include "stdafx.h"

#include "publicGlobalvar.h"
#include "..\Include\Macro.h"
#include "..\Include\StructMF.h"

map<std::string, std::vector<short>> g_CodeTime;


char  g_strMarkCode[MAX_MARKETNUM][12] = {
	"SZ","SH","GZ", "BJ","SQ","DL",
	"ZZ","ZJ","NY"
};
char  g_strMarkName[MAX_MARKETNUM][20] = {
	"深圳","上海","股转","北京","上期","大商",
	"郑商","中金","能源"
};

// 旧版市场定义，如果插件相关出现问题，需要进行兼容升级
//char  g_strMarkCode[MAX_MARKETNUM][12]   = {
//	"SZ","SH","HK","SF","SC",
//	"DC","ZC","BH","SJ","GJ",
//	"USR","FX","HX","X3B","QL",
//	"COMEX","EUREX","LDJ2","BG","TEA",
//	"DPT","NYSE","NASDAQ","AMERICAN"
//};
//char  g_strMarkName[MAX_MARKETNUM][20]   = {
//	//"深圳","上海","香港","股指","上商","连商","郑商","渤海","上金","伦敦","天金"
//	"深圳","上海","香港","股指","上商",
//	"连商","郑商","渤海","上金","国际黄金",
//	"美股","外汇","海峡","新三版","齐鲁行情",
//	"国际期货","国际期货","伦敦金2","宝钢","茶叶",
//	"甬交所","纽约","纳斯达克","美国"
//};

int	  g_nZsMinutes = 5;
short				anFZRatio[10]={5,15,30,60,1,1,1,1,10,10}; // <- anFzRatio[7]={1,3,6,12,1,1,1}
short sz[8]={570,690,780,900,900,900,900,900};
short sh[8]={570,690,780,900,900,900,900,900};
short hk[8]={570,720,780,960,960,960,960,960}; //香港
short sf[8]={555,690,780,915,915,915,915,915}; //中国金融期货交易所(期货)
short sc[8]={540,615,630,690,810,900,900,900}; //上海期货交易所(期货)
short zc[8]={540,615,630,690,810,900,900,900}; //郑州商品交易所(期货)
short dc[8]={540,615,630,690,810,900,900,900}; //大连商品交易所(期货)
short bh[8]={19*60,3*60,9*60,11*60+30,13*60+30,16*60,16*60,16*60}; //渤海商品交易所(现货)
short sj[8]={21*60,2*60+30,9*60,11*60+30,13*60+30,15*60+30,15*60+30,15*60+30};
short ld[8]={6*60,14*60,14*60,22*60,22*60,6*60,6*60,6*60};
//short bh[8]={540,780,810,960,1140,180,180,180};// 渤海商品 早盘9：00～11：30；午盘13：30～16：00；夜盘19：00～3：00，把夜盘分割成两个交易节
short tj[8]={6*60,13*60+20,13*60+20,20*60+40,20*60+40,4*60,4*60,4*60};
short dpt[8]={570,690,780,900,900,900,900,900};
short wh1[8]={6*60,14*60,14*60,22*60,22*60,6*60,6*60,6*60};
short nyse[8] = {1350,1440,0,300,300,300,300,300};
short nasdaq[8] = {1350,1440,0,300,300,300,300,300};
short american[8] = {1350,1440,0,300,300,300,300,300};
short gz[8] = { 570,720,780,970,970,970,970,970 };//国证

short _bh[8] = { 19 * 60,3 * 60,9 * 60,11 * 60 + 30,13 * 60 + 30,16 * 60,16 * 60,16 * 60 }; //渤海商品交易所(现货)
short ldj[8] = { 6 * 60,13 * 60 + 20,13 * 60 + 20,20 * 60 + 40,20 * 60 + 40,6 * 60,6 * 60,6 * 60 };
short btm[8] = { 0,720,720,1440,1440,1440,1440,1440 };
short bt[8] = { 0,720,720,1440,1440,1440,1440,1440 };

/*struct fltype               // 股票种类过滤条件位结构
{  
WORD     astk:1;    //A股
WORD     bstk:1;    //B股
WORD     jj:1;      //基金
WORD     gz:1;      //国债
WORD     qiz:1;     //企业债券
WORD     qz:1;      //认,配股权证
WORD     qtstk:1;   //其它
WORD	 cystk:1;	//中小企业
WORD	 sb:1;		//三板
WORD	 addcode:1;	//配售比如003
WORD	 cyb:1;		//创业板
WORD     unused:5;  //
};

union   flunion 
{
	struct	fltype ftype;
	WORD	fshort;
};*/
union   flunion g_zaiquan = {0,0,0,1,1};		// 债券
union   flunion g_astock  = {1,0,0,0,0,0,0,1,0,1,1};	// A股的，要算上创业板
union	flunion g_cyb	  = {0,0,0,0,0,0,0,0,0,0,1};
//int   lpnSysDomainType[]={SH,SH,SZ,SZ,SH,SZ,SZ_SH,SZ_SH,SZ_SH,SZ_SH,SZ_SH,SZ_SH,SZ};			//上海深圳类别,新增创业板
int   lpnSysDomainType[17]  ={SH,SH,SZ,SZ,SH,SZ,SZ_SH,SZ_SH,SZ_SH,SZ_SH,SZ_SH,SZ_SH,SZ,SZ,BJ};  //上海深圳类别,新增中小企业
//short lpsSysClassType []={0x205,0x02,0x285,0x02,0x178,0x178,0x285,0x02,0x178,0x04,0x00,0x99,0x80};   //股票类别:A/B/C
//short lpsSysClassType []={0x01,0x02,g_astock.fshort,0x02,g_zaiquan.fshort,0x078,0x81,0x02,0x078,0x04,0x00,0x99,0x80};   //股票类别:A/B/C
								    //(0x05改为0x85),深圳A股,深沪A股包括中小企业)
short lpsSysClassType [17]={0x01,0x02,g_astock.fshort,0x02,g_zaiquan.fshort,0x078,0x81,0x02,0x078,0x04,0x00,0x99,0x80,g_cyb.fshort};   //股票类别:A/B/C
//short lpsSysClassType_new []={0x01,0x02,0x81,0x02,0x078,0x078,0x81,0x02,0x078,0x04,0x00,0x99,0x80};   //股票类别:A/B/C
//short lpsSysClassType_new []={0x01,0x02,0x81,0x02,g_zaiquan.fshort,0x078,0x81,0x02,0x078,0x04,0x00,0x99,0x80};   //股票类别:A/B/C
//short lpsSysClassType_new []={0x01,0x02,g_astock.fshort,0x02,g_zaiquan.fshort,0x078,0x81,0x02,0x078,0x04,0x00,0x99,0x80};   //股票类别:A/B/C
short lpsSysClassType_new[17]={0x01,0x02,g_astock.fshort,0x02,g_zaiquan.fshort,g_zaiquan.fshort,
						//  上证Ａ股|上证Ｂ股|深证Ａ股|深证Ｂ股|上证债券|深证债券|
							g_astock.fshort,0x02,g_zaiquan.fshort,0x04,
						//  0x81深沪Ａ股|深沪Ｂ股|深沪债券|中小企业|北京a股
						0x00,0x99,0x80,g_cyb.fshort,0x00};   //股票类别:A/B/C

short g_ResetTimeA = 554; //9:14

const char* GetStkInfo(const char* symbol, short& setcode)
{
	for (int t = 0; t < MAX_MARKETNUM; ++t)
	{
		if (_strnicmp(symbol, g_strMarkCode[t], strlen(g_strMarkCode[t])) == 0)
		{
			setcode = t;
			return	(symbol + strlen(g_strMarkCode[t]));
		}
	}
	return symbol;
}

HANDLE g_hProcessSem = NULL;// 本进程的信号//TODO 要删掉

CLogDlg* g_pLogDlg = NULL;

volatile bool g_bThreadQuit = false;//所有线程退出标志，最高优先级，线程退出均为自洽方式
volatile char g_chServerStatus = enStart;//TODO 其他插件多依赖的，参考转码/主站定义数组形式，默认0是本插件
CrashDump *g_CrashDump = NULL;

__int64 g_lluHqTime[MAX_MARKETNUM];
long g_lluHqCount[MAX_MARKETNUM];