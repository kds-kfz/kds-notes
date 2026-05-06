#pragma once
#include <vector>
#include <map>
#include "LogDlg.h"
#include "KdscInterface.h"
#include "CrashDump.h"

using namespace std;

extern int	  g_nZsMinutes;
extern  short    anFZRatio[10];
extern short	sz[8];
extern short	sh[8];
extern short	hk[8];
extern short	sf[8];
extern short	sc[8];
extern short	zc[8];
extern short	dc[8];
extern short	bh[8];
extern short	sj[8];
extern short	ld[8];
extern short    tj[8];
extern short	dpt[8],wh1[8];
extern short	nyse[8];
extern short	nasdaq[8];
extern short	american[8];
extern short	g_ResetTimeA;
extern short	gz[8];
extern short	_bh[8];
extern short	ldj[8];
extern short	btm[8];
extern short	bt[8];

extern map<std::string, std::vector<short>> g_CodeTime;
//extern	char  g_strMarkCode[MAX_MARKETNUM][12];
//extern	char  g_strMarkName[MAX_MARKETNUM][20];
const char* GetStkInfo(const char* symbol, short& setcode);

extern int		lpnSysDomainType[17];
extern short	lpsSysClassType [17];//未使用
extern short	lpsSysClassType_new [17];

extern HANDLE g_hProcessEvent;
extern HANDLE g_hProcessSem;
extern CLogDlg* g_pLogDlg;
extern volatile bool g_bThreadQuit;
extern volatile char g_chServerStatus;
extern CrashDump *g_CrashDump;

//数据加工 待写入的数据
extern __int64 g_lluHqTime[MAX_MARKETNUM];
extern long g_lluHqCount[MAX_MARKETNUM];
