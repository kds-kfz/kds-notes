#pragma once

#include "Macro.h"
#include "structb.h"
#include <string>
#include <vector>
#include <iostream>
#include <sstream>

#include <windows.h>

#define ZZSTKTYPE 122
#define GZSTKTPYE 103

#define TONGPEIFUSTR "@YY@MM@DD"
#define HISTORY_VALID_DATE (19900101)//历史K线有效日期

#define BUF_SIZE_MAX	(256)
#define GDEL_ARRAY(p) { if (p) delete [](p); p = NULL; }
#define GDEL(p) { if (p) delete (p); p = NULL; }

//2023/6/28 兼容现有转码机代码
const short BT = 19;	// 比特币
const short BTM = 20;	// 比特币资金
const short LDJ = 9;	// 伦敦金, （总市场，不再使用）

extern	char  g_strMarkCode[MAX_MARKETNUM][12];
extern	char  g_strMarkName[MAX_MARKETNUM][20];
DWORD	myfilelength ( FILE * fp );
char *strupper(char *str);
long GetLsUnit(short setcode);
char *  GetSetStr(int setcode);
short GetSetByStr(const char* szSetCode,const char** szCode = NULL);
BOOL	IsQHMarket(short setcode);
float cal_averagetmp(struct DiskStkInfoB *scode,struct CurrStockDataBEx * hqp);
__time32_t	long2time(const long_short & lTime,int isdate);
const char * GetGUID(char szGatewayName[255]);
extern inline int double2int(double dVal);
extern inline DWORD double2dword(double dVal);
extern inline __int64 double2int64(double dVal);
extern unsigned short tranftou(float price,char precise);
extern unsigned long tranftou2(float price,char precise);
extern float	tranutof(unsigned short price,char precise);
float tranutof2(unsigned long price,char precise);
double tranutodf(unsigned long price,char precise);
unsigned long trandoublet2long(double price,char precise);
__int64 trandoublet2int64(double price,char precise);
//void   ConvertDoubleHQ(const CurrStockDataB * srchq,CurrStockDataB * hq,int xsflag);



const char * ASC2UTF8(const char* srcbuf,char * outbuf,int outlen);
const char * UTF82ASC(const char *n,char * outbuf);
int WINAPI	SDK_TRACE(const char *format , ... );


short  GetLB    (struct DiskStkInfoB *scode);
short  GetFzNum(short setcode);
BOOL   testzs(const char * code,short setcode);
BOOL   testzs(struct DiskStkInfoB *pStkInfo);
BOOL   testzsmk(const char * code,short setcode);

extern int System2Jrj(short setcode,int m,short * fz);
extern int  GetRealTotalMinute(short *fz);
// 取得到现在为止, 总共经过的分钟数
extern int GetJRJMinute( short * fz,short setcode,int bYesterdayFlag );
// fznum 为 1 表示 5 分钟, 为 3 表示 15 分钟线等等
// 下面的过程用来取得到现在为止, 已经经过了多少个 5 (或15, 等) 分钟线
extern int GetMinuteXh( int lineperiod,short * fz,short mulnum,short setcode,int bYesterdayFlag);
int GetRealMinuteXh( int lineperiod,short * fz ,short mulnum);
extern short GetFZnoFromMinute( short setcode,int m ,int lineperiod,short * fz,short mulnum);
// 基本上是上面过程的逆过程, 根据序号得到当前应该的分钟数
extern int  GetMinuteFromFZ(int xh,int fznum,short * fz);
extern void InitStkTime(short setcode,short * fz);
int  GetMinuteFromFZ(int xh,int fznum,short * fz);
int GetRealTotalMinute( short *fz );
int TimeAdd(int t0,int addminute);
int TimeSub(int t0,int addminute);
BOOL	isInTimeSpan(int t,int s,int e);
int TimeSpan(int t2,int t1);
void gettime ( struct bc_time * timep );
short getabstime ( void );

BOOL GetFileVersion(LPCTSTR strFile, CString& strVersion);
BOOL CenterAndActivateWindow(HWND hWnd);

void AllTrim( char *s);
char * MakeVol(double ftmp);  //特大浮点形的处理
char * MakeJE(double ftmp);
size_t  GetStringArray(std::vector<std::string> & toids,const char * sendto,BYTE cal);
size_t  GetStringArrayOR(std::vector<std::string> & toids,const char * sendto,const char * strmask);
size_t	GetStrArrayOR(std::vector<std::string>& toids,  const char* pSendTo, BYTE cal);
void	GetNetCardStr(std::vector<std::string> & aNetCard);
// BKDRHash
unsigned int GenHash(const char* str, unsigned int len,unsigned int seed);
unsigned __int64 GenHash64(const char* str, unsigned int len);
BOOL DeleteDirectory(char* psDirName)   ;

//是否都小于0.000001
bool TradeSuspension(double fNow, double fBuy, double fSell);
int	 GetIndexSetDomain(const char* pIndex);
int accept_timeout(int fd, struct sockaddr_in *addr, int *addrlen, int time);

__int64 GetCurTimetoJcl();

int Comparefloat(double a, double b);
const char* GetRootPath();

//获取yyyymmddhhmmss的数据
#define CalDate(lt)		((lt)/1000000) 
#define CalYear(lt)		((lt)/10000000000) 
#define CalMonth(lt)	(((lt)/100000000)%100) 
#define CalDay(lt)		(((lt)/1000000)%100)
#define CalMonthDay(lt)	(((lt)/1000000)%10000)
#define CalHour(lt)		(((lt)/10000)%100)
#define CalMinute(lt)	(((lt)/100)%100)
#define CalHourMinute(lt)	(((lt)%1000000)/10000)
#define CalAbsMin(lt)		(CalHour(lt)*60 + CalMinute(lt)) 
#define CalSecond(lt)		((lt)%100)
#define CalNms(lt)		(1000*((lt)%100))

namespace Market_A
{
	// 每个市场不一样
	BOOL	isCanOpenDTBig();
	BOOL	isCanOpenDT();
	BOOL	isCanPHDT();
	BOOL	isToOpenTime(short nInitTime);
	CTime	GetOpenDate(int iniMinutes, BOOL bOpen_CurDay = TRUE);
	BOOL	isInWeekend();
	BOOL	isInWeekend(int date);
}


long_short transJcltime(__int64 jcltime,BOOL bDate);
CString GetModuleDir();
bool RegularPath(std::string &sPath);
bool CreatePath(char* path);

#define MIN_DOUBLE  1e-6
#define IS_DOUBLE_ZERO(d) (fabs(d) <= MIN_DOUBLE)
/* 相等0  d1 > d2 大于0   d1 < d2 小于0 */
inline int8_t equalDouble(const double& d1, const double& d2) {
	if (d1 > d2 + MIN_DOUBLE) return 1;
	else if (d2 > d1 + MIN_DOUBLE) return -1;
	else return 0;
}

UINT GetCurDate(bool bDate = true);
//std::string doubleToString(double val, int precise = 2);
//std::string intToString(int val);
std::string intToString(int64_t val);

bool isInWeekend(int date);

int md2d(int leap, int month, int day);
int y2d(int year);
long addNumDay(int szDate, int nDay);
long subNumDay(int szDate, int nDay);

int DaysBetween2Date(std::string date1, std::string date2);
bool StringToDate(std::string date, int& year, int& month, int& day);
bool IsLeap(int year);
int DayInYear(int year, int month, int day);
double round3(double number, int bits);
int stringSplit(const std::string str, const std::string sep, std::vector<std::string> &vec);
int zh_sort_func(const void * a, const void * b);

// 文件目录操作
bool copyDir(const std::string& src, const std::string& dst);
bool removeDir(const std::string& strDir);
bool removeDir2(const std::string& strDir);
bool copyFile(const std::string& src, const std::string& dst);
bool checkFolderExist(const std::string& strPath);
bool checkFileExist(const std::string& strPath);
int getAllFiles(std::string strDir, std::vector<std::string>& filesVec);
int getSubDirsAndFiles(std::string strDir, std::vector<std::string>& filesVec);
std::string getFileName(const std::string& path, bool needSufix);
bool mkpath(const std::string& strPath);
FILE* CreateAppendFile(LPCSTR szFile);
DWORD filelength(FILE* fp);
__int64 GetFileLength(LPCSTR szFile);

bool replace(std::string& str, const std::string& substr, const std::string& restr);
void splitStr(std::string str, const const char split, std::vector<std::string>& rst);


bool TranStrCode(LPCSTR szinfo, tagCode& stk);
float GetTPPrice(short setcode, const char* Code, const char* Name, char xs_flag, float Close, BOOL bUp);
short GetmarketNum(const char* cSetcode);
__int64 ConvertTimet(LPCTSTR lpTime, bool tran = false);
void SplitStr(const char* szValue, char a, std::vector<std::string>& vecSub);
bool IsEquals(double a, double b);
bool IsEquals(double a, double b, int xs_flag);
bool IsDoubleZero(double x);
void TrimStr(CString& str, const char* pstr, int len);
void TrimStr(char* pStr, int len);
void TrimStr(char* pDst, char* pSrc, int len);
void TrimStr(std::string& str);
bool is_limit_up(double fclose, double close);
void		InitStkTime(short setcode, char* code, short* fz);
bool IsHbEtf(short setcode, char* code);
bool testzs_other(short setcode, const char* code);	// 由我们软件约定的指数类型：分类板块指数（SH）
short  GetXSFlag(short setcode, char* Code, char xs_flag, int chStockFlag);
short testzs(short setcode, char* code, int chStockFlag);
StockCodeType GetStockType(short setcode, char* code, int chStockFlag = -1);
short getvolunit(short setcode, char* code, int chStockFlag);	//取成交量倍数
__int64 getNowJcltime(void);
double Round(double dval, short iPlaces = 2);
int	System2DT(short setcode, int m, short* fz);
int   GetMinuteFromFZ(int xh, int fznum, short* fz, short setcode);
long GetFriday(long date);
short need_justcjl(short setcode, char* code, int chStockFlag);
void FormatPath(char *Path,int MaxLen);
long GetNextDate(long date, int days = 1);
int GetNextYear(int p_iCurYear, int p_iOffset = 1);
int FindProcess(const char *p_szProcessName);
long FindProcessid(const char *p_szProcessName);
int KillProcess(long p_lProcessID);
long IsProcessIdExists(long p_iProcessId);
long StartProcess(const char *p_szProcessWorkPath, const char *p_szProcessFullPath);
long InitProcessSem(HANDLE &p_hProcessSem, const char *p_szSemName, long p_lInitCount, long lMaxCount);
long WaitForProcessSem(const char *p_szSemName);
extern unsigned long tranf2long(float price, char precise);

bool IsDateValid(unsigned int uiDate);
__int64 GetCurDate(int p_iType);
int GetIntervalDate(const int& p_iBeforeDate, const int& p_iAfterDate);
int GetNextDate2(const int& p_iDate, const int& p_idays);//日期计算不准确
std::wstring string2wstring(std::string str);
std::string wstring2string(std::wstring wstr);
std::wstring char_to_wchar(const char* ch);
std::wstring TrimStr(const std::wstring &p_wsInput, const std::wstring p_wsSymbol = L" ");
void strTolower(std::string &p_strInput);
std::vector<std::string> GetAdapterInfo();//获取适配器网络信息
double CalcRelevance(const std::vector<double>& p_vecKline1, const std::vector<double>& p_vecKline2);
void DFS(const int& p_node, const std::vector<std::vector<bool>>& p_vecAdjacencyMatrix, std::vector<bool>& p_vecVisited, std::vector<int>& vecTmp);
int GetGraphs(std::vector<std::vector<bool>>& p_vecAdjacencyMatrix, std::vector<std::vector<int>>& p_vecGraphsIndex);

bool MarkCodeSplit(std::string str, short *setcode, std::string &code);
bool MarkCodeMerge(short p_nSetcode, char *p_szCode, std::string &p_strCode);
int Binary_Search(AnalyDataB * AnalyDatap, int iCount, __int64 iDate, int iNear);