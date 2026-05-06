#include "StdAfx.h"
#include "publicfunc.h"
#include "publicGlobalvar.h"
#include "..\Include\StructMF.h"
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <tlhelp32.h>
#include <sys/stat.h>
#include <codecvt>


#include <windows.h>
#include <tlhelp32.h>
#include <io.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

#pragma comment(lib,"version.lib")
#pragma comment(lib,"Iphlpapi.lib")	// 得到网卡地址需要
#pragma comment(lib,"Rpcrt4.lib")

const int day_tab[2][12] = { { 31,28,31,30,31,30,31,31,30,31,30,31 },
{ 31,29,31,30,31,30,31,31,30,31,30,31 } };

char typestr[7][4] = { "tbl","quo","fst","dep","now","_","__" };
char days_of_mon[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

DWORD myfilelength ( FILE * fp )
{
	DWORD oldpos, filelen;

	oldpos = ftell ( fp );
	if(oldpos<0) return 0;
	fseek ( fp, 0L, SEEK_END );
	filelen = ftell ( fp );
	if(filelen<0) return 0;
	fseek ( fp, oldpos, SEEK_SET );
	return filelen;
}

__time32_t	long2time(const long_short &lTime,int isdate)
{
	tm	thtm={0};
	if ( isdate )
	{
		thtm.tm_year = lTime.Date/10000 - 1900;
		thtm.tm_mon	 = (lTime.Date%10000)/100 - 1;
		thtm.tm_mday = lTime.Date%100;
		return _mktime32(&thtm); 
	}
	else
	{
		thtm.tm_year = lTime.Daye1.Year + 2004-1900;
		thtm.tm_mon	 = lTime.Daye1.Mon_Day/100 - 1;
		thtm.tm_mday = lTime.Daye1.Mon_Day%100;
		thtm.tm_hour = lTime.Daye1.Minute/60;
		thtm.tm_min  = lTime.Daye1.Minute%60;
		return _mktime32(&thtm); 
	}
}

char *strupper(char *str)
{
	size_t i,len = strlen(str);
	for(i = 0;i < len;i++)
		str[i] = toupper(str[i]);
	return str;
}
char *  GetSetStr(int setcode)
{   
	if (setcode < 0 || setcode >= _countof(g_strMarkCode))
		setcode = SZ;
	return g_strMarkCode[setcode];
}

short GetSetByStr(const char* szSetCode,const char** szCode/* = NULL*/)
{
	for (short setcode = 0; setcode < _countof(g_strMarkCode); setcode++)
	{
		if (!_strnicmp(g_strMarkCode[setcode],szSetCode,strlen(g_strMarkCode[setcode])))
		{
			if (szCode) *szCode = szSetCode + strlen(g_strMarkCode[setcode]);
			return setcode;
		}
	}
	*szCode = szSetCode;
	return SZ;
}
BOOL	IsQHMarket(short setcode)
{
	BOOL isQH = ( setcode==BH||setcode==SF||setcode==SC||setcode==DC||setcode==ZC||setcode==SJ
		||setcode==GJ||setcode==USR||setcode==FX||setcode==X3B||setcode==TJ|| setcode==LDJ2);
	return isQH;
}

//计算均价
float cal_averagetmp(struct DiskStkInfoB *scode,struct CurrStockDataBEx * hqp)
{
	if(scode == NULL) return (float)0.001;
	float last_price,average;
	short  lb = GetLB(scode);
	short  setcode = scode->SetCode;
	if ((lb == ZSLB||lb == QHLB)&&hqp->PreYield>0.01)
		last_price = (float)hqp->PreYield;
	else   
		last_price = (float)hqp->PreClosePrice;
	if (lb == ZSLB)
	{ 
		if(hqp->Yield<0.01) 
			average = last_price;
		else average = (float)hqp->Yield;
	}
	else
	{ 
		if(hqp->Volume<0.001 ||hqp->Amount<0.001) 
			average = last_price;
		else 
			average = (hqp->Amount/(hqp->Volume*scode->volUnit));
	}
	if ( setcode == SZ && scode->Code[0] == '1' && scode->Code[1] == '3' ) // 国债回购,新数据为13开头
		average = hqp->NowPrice;
	if ( setcode == SH && scode->Code[0] == '2' ) // 国债回购
		average = hqp->NowPrice;
	return average;
}

// 21010=263200EF864640f3A7B94FD2A0F00B4C<SOH>
const char * GetGUID(char szGatewayName[255])
{
	GUID	guid;
	UuidCreate(&guid);
	RPC_CSTR s = NULL;
	UuidToStringA(&guid,&s);
	strcpy_s(szGatewayName,255,(LPCSTR)s);
	RpcStringFreeA(&s);
	return szGatewayName;
}
short  GetFzNum(short setcode)
{
	if (setcode>=HK) return 1500;//g_DTCfg.QHFZNUM;
	return 250;//g_DTCfg.GPFZNUM;	// 缺省250个
}
long GetLsUnit(short setcode)
{ 
	short fznum,knum;
	fznum=GetFzNum(setcode);
	knum=fznum/5;
	return ((long)fznum*sizeof(struct MinuteDataB)+(long)knum*sizeof(struct AnalyDataB));
}
// magic number的奥妙就在这里，通过迫使FPU将尾数移位; 仅仅是一次浮点数加法
// 以四舍五入方式转换为 32 位整数
extern inline int double2int(double dVal)
{
	static double _dMagic=6755399441055744.0;
	dVal+=_dMagic;
	return *(int*)&dVal;
}
// 以四舍五入方式转换为 32 位整数
extern inline DWORD double2dword(double dVal)
{
	static double _dMagic=6755399441055744.0;
	dVal+=_dMagic;
	return *(DWORD*)&dVal;
}
extern  __int64 double2int64( double dVal )
{
	static __int64 mask[] = {0,0xfff8000000000000};
	static double _dMagic=6755399441055744.0;
	int sign = dVal < 0 ? 1 : 0;
	dVal += _dMagic;
	return (*(__int64*)&dVal) & 0x7ffffffffffff | (mask[sign]);

}
unsigned short tranftou(double price,char precise)
{
	unsigned short itmp=0;
	switch (precise)
	{
		//函数不需要+0.5，回四舍五入
		//case 0:itmp = (DWORD)(price);break;
		//case 1:itmp = (DWORD)(0.5+price*10.0);break;
		//case 2:itmp = (DWORD)(0.5+price*100.0);break;
		//case 3:itmp = (DWORD)(0.5+price*1000.0);break;
		//case 4:itmp = (DWORD)(0.5+price*10000.0);break;
	case 0:itmp = double2int(price);break;
	case 1:itmp = double2int(price*10.0);break;
	case 2:itmp = double2int(price*100.0);break;
	case 3:itmp = double2int(price*1000.0);break;
	case 4:itmp = double2int(price*10000.0);break;
	}
	return itmp;
}
unsigned long tranftou2(float price,char precise)
{
	unsigned long itmp=0;
	switch (precise)
	{
		//case 0:itmp = (DWORD)(price);break;
		//case 1:itmp = (DWORD)(0.5+price*10.0);break;
		//case 2:itmp = (DWORD)(0.5+price*100.0);break;
		//case 3:itmp = (DWORD)(0.5+price*1000.0);break;
		//case 4:itmp = (DWORD)(0.5+price*10000.0);break;
	case 0:itmp = double2int(price);break;
	case 1:itmp = double2int(price*10.0);break;
	case 2:itmp = double2int(price*100.0);break;
	case 3:itmp = double2int(price*1000.0);break;
	case 4:itmp = double2int(price*10000.0);break;
	}
	return itmp;
}
unsigned long trandoublet2long(double price,char precise)
{
	unsigned long itmp=0;
	switch (precise)
	{
		//case 0:itmp = (DWORD)(price);break;
		//case 1:itmp = (DWORD)(0.5+price*10.0);break;
		//case 2:itmp = (DWORD)(0.5+price*100.0);break;
		//case 3:itmp = (DWORD)(0.5+price*1000.0);break;
		//case 4:itmp = (DWORD)(0.5+price*10000.0);break;
	case 0:itmp = double2int(price);break;
	case 1:itmp = double2int(price*10.0);break;
	case 2:itmp = double2int(price*100.0);break;
	case 3:itmp = double2int(price*1000.0);break;
	case 4:itmp = double2int(price*10000.0);break;
	}
	return itmp;
}

__int64 trandoublet2int64( double price,char precise )
{
	return double2int64(price*pow(10.,precise));
}

float tranutof(unsigned short price,char precise)
{
	return price * pow(0.1f, precise);
}
float tranutof2(unsigned long price,char precise)
{
	return price * pow(0.1f, precise);
}
double tranutodf(unsigned long price,char precise)
{
	double ftmp=0;
	switch (precise)
	{
	case 0:ftmp = price;break;
	case 1:ftmp = 0.1*price;break;
	case 2:ftmp = 0.01*price;break;
	case 3:ftmp = 0.001*price;break;
	}
	return ftmp;
}

// void   ConvertDoubleHQ(const CurrStockDataB * srchq,CurrStockDataB * hq,int xsflag)
// {
// 	hq->Close = tranutodf(srchq->l_Close,xsflag);
// 	hq->Open = tranutodf(srchq->l_Open,xsflag);
// 	hq->Max = tranutodf(srchq->l_Max,xsflag);
// 	hq->Min = tranutodf(srchq->l_Min,xsflag);
// 	hq->Now = tranutodf(srchq->l_Now,xsflag);
// 	hq->Buy = tranutodf(srchq->l_Buy,xsflag);
// 	hq->Sell = tranutodf(srchq->l_Sell,xsflag);
// 	hq->TickDiff = tranutodf(srchq->l_TickDiff,xsflag);
// 	hq->Yield = tranutodf(srchq->Yield_VolInStock.l_Yield,xsflag);
// 	for ( int pk=0;pk<5;++pk )
// 	{
// 		hq->Buyp[pk] = tranutodf(srchq->l_Buyp[pk],xsflag);
// 		hq->Sellp[pk] = tranutodf(srchq->l_Sellp[pk],xsflag);
// 	}
// }
const char * UTF82ASC(const char *n,char * outbuf)
{
	int len=MultiByteToWideChar(CP_UTF8,0,n,-1,NULL,NULL);
	unsigned short * wszASC = new unsigned short[len+1];
	memset(wszASC, 0, len * 2 + 2);
	MultiByteToWideChar(CP_UTF8,0,n,-1,(LPWSTR)wszASC,len);

	len=WideCharToMultiByte(CP_ACP,0,(LPCWSTR)wszASC,-1,NULL,0,NULL,NULL);
	char * szASC=new char[len+1];
	memset(szASC,0,len+1);
	WideCharToMultiByte(CP_ACP,0,(LPCWSTR)wszASC,-1,szASC,len,NULL,NULL);
	strcpy(outbuf,szASC);
	delete [] wszASC;
	delete [] szASC;
	return outbuf;
}

const char * ASC2UTF8(const char* srcbuf,char * outbuf,int outlen)
{
	int len=MultiByteToWideChar(CP_ACP, 0, (CHAR*)srcbuf, -1, NULL,0);
	if ( len <= 0 )
		return outbuf;
	
	WCHAR * wszUtf8 = new WCHAR[len+1];
	memset(wszUtf8, 0, len * 2 + 2);
	MultiByteToWideChar(CP_ACP, 0, (CHAR*)srcbuf, -1, wszUtf8, len);		// LPCTSTR

	len = WideCharToMultiByte(CP_UTF8, 0, wszUtf8, -1, NULL, 0, NULL, NULL); 
	char *szUtf8=new char[len + 1];
	memset(szUtf8, 0, len + 1);
	WideCharToMultiByte (CP_UTF8, 0, wszUtf8, -1, szUtf8, len, NULL,NULL);

	strncpy(outbuf,szUtf8,outlen-1);

	delete[] szUtf8;
	delete[] wszUtf8;
	return outbuf;
}

int WINAPI	SDK_TRACE(const char *format , ... )
{
	char	buf[100*1024]={0};

	va_list		l;
	va_start(l,format);
	 vsprintf(buf,format,l);
	_vsnprintf(buf,100*1024-1,format,l);
	va_end(l);

	OutputDebugStringA(buf);
	printf("%s",buf);
	/*if(g_pLogDlg!=NULL)
	{
		g_pLogDlg->OutInfo(buf);
	}*/

	return 1;
}

int TimeAdd(int t0,int addminute)
{
	t0 += addminute;
	return t0 %  (24*60);
}
int TimeSub(int t0,int addminute)
{
	if ( t0 > addminute )
		t0 -= addminute;
	else
	{
		addminute -= t0;
		t0 = 24*60 - addminute;
	}
	return t0;
}
int TimeSpan(int t2,int t1)
{
	if ( t2 >= t1 )
		return t2 - t1;
	else
		return (24*60-t1)+t2;
}
BOOL	isInTimeSpan(int t,int s,int e)
{
	if ( s <= e )	// 改成闭区间
		return (t >=s && t < e);	// 全部统一前闭后开
	else
	{
		if ( t >= s && t <= 24*60 )
			return TRUE;
		else if ( t >=0 && t < e )	// 全部统一前闭后开
			return TRUE;
	}
	return FALSE;
}
BOOL	isTimeValid(int t,short * fz)
{
	int		i = 0;
	for ( i=0;i<4;++i )
	{
		if ( isInTimeSpan(t,fz[2*i],fz[2*i+1]) )
		{
			return TRUE;
		}
	}
	return FALSE;
}
BOOL	isTimeValid(int t,int * fz)
{
	int		i = 0;
	for ( i=0;i<4;++i )
	{
		if ( isInTimeSpan(t,fz[2*i],fz[2*i+1]) )
		{
			return TRUE;
		}
	}
	return FALSE;
}

short GetLB(struct DiskStkInfoB *scode)
{ 
	if (testzs(scode)) 					    return ZSLB;
	//if (scode->Code[0]>='A'&&scode->Code[0]<='N') return QHLB;
	if(scode->SetCode==BH||scode->SetCode==SF||scode->SetCode==SC||scode->SetCode==DC||
	   scode->SetCode==ZC||scode->SetCode==SJ||scode->SetCode==GJ||scode->SetCode==USR)
		return QHLB;
	if (scode->Code[0]=='T')                      return HKLB;
	return(GPLB);
}

void InitStkTime(short setcode,short * fz)
{
	if (setcode==SZ)
	{    
		fz[0]=sz[0];fz[2]=sz[2];fz[4]=sz[4];fz[6]=sz[6];
		fz[1]=sz[1];fz[3]=sz[3];fz[5]=sz[5];fz[7]=sz[7];
	}
	else if(setcode==BH)
	{
		fz[0]=bh[0];fz[2]=bh[2];fz[4]=bh[4];fz[6]=bh[6];
		fz[1]=bh[1];fz[3]=bh[3];fz[5]=bh[5];fz[7]=bh[7];
	}
	else if(setcode==SJ)
	{
		fz[0]=sj[0];fz[2]=sj[2];fz[4]=sj[4];fz[6]=sj[6];
		fz[1]=sj[1];fz[3]=sj[3];fz[5]=sj[5];fz[7]=sj[7];
	}
	else if(setcode==GJ)
	{
		fz[0]=ld[0];fz[2]=ld[2];fz[4]=ld[4];fz[6]=ld[6];
		fz[1]=ld[1];fz[3]=ld[3];fz[5]=ld[5];fz[7]=ld[7];
	}
	else if(setcode==USR)
	{
		fz[0]=tj[0];fz[2]=tj[2];fz[4]=tj[4];fz[6]=tj[6];
		fz[1]=tj[1];fz[3]=tj[3];fz[5]=tj[5];fz[7]=tj[7];
	}
	else if(setcode==DPT)
	{
		fz[0]=dpt[0];fz[2]=dpt[2];fz[4]=dpt[4];fz[6]=dpt[6];
		fz[1]=dpt[1];fz[3]=dpt[3];fz[5]=dpt[5];fz[7]=dpt[7];
	}
	else if(setcode==HK)
	{
		fz[0]=hk[0];fz[1]=hk[1];
		fz[2]=hk[2];fz[3]=hk[3];
		fz[4]=hk[4];fz[5]=hk[5];
		fz[6]=hk[6];fz[7]=hk[7];
	}

	else if(setcode==SF)
	{
		fz[0]=sf[0];fz[1]=sf[1];
		fz[2]=sf[2];fz[3]=sf[3];
		fz[4]=sf[4];fz[5]=sf[5];
		fz[6]=sf[6];fz[7]=sf[7];
	}

	else if(setcode==SC)
	{
		fz[0]=sc[0];fz[1]=sc[1];
		fz[2]=sc[2];fz[3]=sc[3];
		fz[4]=sc[4];fz[5]=sc[5];
		fz[6]=sc[6];fz[7]=sc[7];
	}

	else if(setcode==ZC)
	{
		fz[0]=zc[0];fz[1]=zc[1];
		fz[2]=zc[2];fz[3]=zc[3];
		fz[4]=zc[4];fz[5]=zc[5];
		fz[6]=zc[6];fz[7]=zc[7];
	}

	else if(setcode==DC)
	{
		fz[0]=dc[0];fz[1]=dc[1];
		fz[2]=dc[2];fz[3]=dc[3];
		fz[4]=dc[4];fz[5]=dc[5];
		fz[6]=dc[6];fz[7]=dc[7];
	}
	else if(setcode==FX) 
	{
		fz[0]=wh1[0];
		fz[1]=wh1[1];
		fz[2]=wh1[2];
		fz[3]=wh1[3];
		fz[4]=wh1[4];
		fz[5]=wh1[5];
		fz[6]=wh1[6];
		fz[7]=wh1[7];
	}
	else if (setcode == NYSE)	
	{
		memcpy(fz,nyse,sizeof(nyse));
	}
	else if (setcode == NASDAQ)	
	{
		memcpy(fz,nasdaq,sizeof(nasdaq));
	}
	else if (setcode == American)	
	{
		memcpy(fz,american,sizeof(american));
	}
	else
	{  
		fz[0]=sh[0];fz[2]=sh[2];fz[4]=sh[4];fz[6]=sh[6];
		fz[1]=sh[1];fz[3]=sh[3];fz[5]=sh[5];fz[7]=sh[7];
	}
}


void gettime ( struct bc_time * timep )
{
	SYSTEMTIME st;

	GetLocalTime(&st);
	timep->ti_min  = st.wMinute;
	timep->ti_hour = st.wHour;
	timep->ti_hund = 10*st.wMilliseconds;
	timep->ti_sec  = st.wSecond;
}

short getabstime ( void ) // 得到现在相对于今日零时的绝对分钟数
{
	short i;
	struct bc_time nowtime;
	gettime ( &nowtime );
	i = nowtime.ti_hour * 60 + nowtime.ti_min;
	return i;
}

int System2Jrj(short setcode,int m,short * fz)
{  
	// 	int  itemnum;
	//     if   (m<=fz[0])   itemnum=1;
	//     else if(m<fz[1])  itemnum=m-fz[0]+1;
	//     else if(m<fz[2])  itemnum=fz[1]-fz[0];
	//     else if(m<fz[3])  itemnum=m-fz[2]+1+(fz[1]-fz[0]);
	//     else if(m<fz[4])  itemnum=fz[3]-fz[2]+(fz[1]-fz[0]);
	//     else if(m<fz[5])  itemnum=m-fz[4]+1+(fz[3]-fz[2])+(fz[1]-fz[0]);
	//     else if(m<fz[6])  itemnum=fz[5]-fz[4]+(fz[3]-fz[2])+(fz[1]-fz[0]);
	//     else if(m<fz[7])  itemnum=m-fz[6]+1+(fz[5]-fz[4])+(fz[3]-fz[2])+(fz[1]-fz[0]);
	//     else              itemnum=fz[7]-fz[6]+(fz[5]-fz[4])+(fz[3]-fz[2])+(fz[1]-fz[0]);
	//     return itemnum;

	// 	int  itemnum = 1;
	// 	//if   (isInTimeSpan(m,fz[7],fz[0]))			itemnum=1;
	// 	if(isInTimeSpan(m,fz[0],fz[1]))				itemnum=TimeSpan(m,fz[0])+1;
	// 	else if(isInTimeSpan(m,fz[1],fz[2])	)		itemnum=TimeSpan(fz[1],fz[0]);
	// 	else if(isInTimeSpan(m,fz[2],fz[3])  )		itemnum=TimeSpan(m,fz[2])+1+TimeSpan(fz[1],fz[0]);
	// 	else if(isInTimeSpan(m,fz[3],fz[4])	)		itemnum=TimeSpan(fz[3],fz[2])+TimeSpan(fz[1],fz[0]);
	// 	else if(isInTimeSpan(m,fz[4],fz[5])	)		itemnum=TimeSpan(m,fz[4])+1+TimeSpan(fz[3],fz[2])+TimeSpan(fz[1],fz[0]);
	// 	else if(isInTimeSpan(m,fz[5],fz[6])	)		itemnum=TimeSpan(fz[5],fz[4])+TimeSpan(fz[3],fz[2])+TimeSpan(fz[1],fz[0]);
	// 	else if(isInTimeSpan(m,fz[6],fz[7])  )		itemnum=TimeSpan(m,fz[6])+1+TimeSpan(fz[5],fz[4])+TimeSpan(fz[3],fz[2])+TimeSpan(fz[1],fz[0]);
	// 	else              itemnum=TimeSpan(fz[7],fz[6])+TimeSpan(fz[5],fz[4])+TimeSpan(fz[3],fz[2])+TimeSpan(fz[1],fz[0]);
	// 	return itemnum;

	int  itemnum = 0;//1;
	// 不要增加1，本来外部统计分钟就+1了
	// 集合竞价,落在收盘后10分钟指定时间的（注意16：01分这样推动的问题) :
	// 注意，如果是连续24小时的，不能这样判断
#ifdef _DT_PEEKDATA_		// 只有采集写盘
	if(isInTimeSpan(m,fz[7],fz[0]) && TimeSpan(fz[7],fz[0]) > 5 )
	{
		if(isInTimeSpan(m,TimeAdd(fz[7],5),fz[0]) )
			return itemnum;
	}
#else
	// 行情主站不管，如果在收盘时间段，用全程时间
	if ( setcode == BH )
	{
		if(isInTimeSpan(m,TimeSub(fz[0],10),fz[0])  && isInTimeSpan(m,TimeAdd(fz[7],5),fz[0]) )
		{
			return itemnum;
		}
	}
	else
	{
		if(isInTimeSpan(m,TimeSub(fz[0],30),fz[0])  && isInTimeSpan(m,TimeAdd(fz[7],30),fz[0]) )
		{
			return itemnum;
		}
		else if(isInTimeSpan(m,TimeSub(fz[0],15),fz[0])  && isInTimeSpan(m,TimeAdd(fz[7],15),fz[0]) )
		{
			return itemnum;
		}
		else if(isInTimeSpan(m,TimeSub(fz[0],10),fz[0])  && isInTimeSpan(m,TimeAdd(fz[7],5),fz[0]) )
		{
			return itemnum;
		}
	}

#endif
	if(m==fz[0])	// 不能从0开始，全部从+1开始
		return itemnum;
	if(isInTimeSpan(m,fz[0],fz[1]))				itemnum=TimeSpan(m,fz[0]);	// 外部计算，统一-1了
	else if(isInTimeSpan(m,fz[1],fz[2])	)		itemnum=TimeSpan(fz[1],fz[0]);
	else if(isInTimeSpan(m,fz[2],fz[3])  )		itemnum=TimeSpan(m,fz[2])+TimeSpan(fz[1],fz[0]);
	else if(isInTimeSpan(m,fz[3],fz[4])	)		itemnum=TimeSpan(fz[3],fz[2])+TimeSpan(fz[1],fz[0]);
	else if(isInTimeSpan(m,fz[4],fz[5])	)		itemnum=TimeSpan(m,fz[4])+TimeSpan(fz[3],fz[2])+TimeSpan(fz[1],fz[0]);
	else if(isInTimeSpan(m,fz[5],fz[6])	)		itemnum=TimeSpan(fz[5],fz[4])+TimeSpan(fz[3],fz[2])+TimeSpan(fz[1],fz[0]);
	else if(isInTimeSpan(m,fz[6],fz[7])  )		itemnum=TimeSpan(m,fz[6])+TimeSpan(fz[5],fz[4])+TimeSpan(fz[3],fz[2])+TimeSpan(fz[1],fz[0]);
	else              itemnum=TimeSpan(fz[7],fz[6])+TimeSpan(fz[5],fz[4])+TimeSpan(fz[3],fz[2])+TimeSpan(fz[1],fz[0]);
	return itemnum;
}

int GetRealTotalMinute( short *fz )
{
	return TimeSpan(fz[7],fz[6])+TimeSpan(fz[5],fz[4])+TimeSpan(fz[3],fz[2])+TimeSpan(fz[1],fz[0]);
}

// 取得到现在为止, 总共经过的分钟数
int GetJRJMinute( short * fz ,short setcode,int bYesterdayFlag )
{   
// 	int  m;
//     SYSTEMTIME  nowtime;
//     GetLocalTime( &nowtime );
// 	if(nowtime.wYear*10000+nowtime.wMonth*100+nowtime.wDay != g_lOpenRq) //每次重新得到,防止过午夜
// 		return fz[7]-fz[6]+(fz[5]-fz[4])+(fz[3]-fz[2])+(fz[1]-fz[0]);
//     m = nowtime.wHour*60+nowtime.wMinute;  //分钟
//     return System2Jrj( m,fz );

	int  m;
	SYSTEMTIME  nowtime;
	if ( bYesterdayFlag==1)
		//return fz[7]-fz[6]+(fz[5]-fz[4])+(fz[3]-fz[2])+(fz[1]-fz[0]);
		return TimeSpan(fz[7],fz[6])+TimeSpan(fz[5],fz[4])+TimeSpan(fz[3],fz[2])+TimeSpan(fz[1],fz[0]);
	GetLocalTime( &nowtime );
	m = nowtime.wHour*60+nowtime.wMinute;  //分钟
    return System2Jrj( setcode,m,fz);
}

// fznum 为 1 表示 5 分钟, 为 3 表示 15 分钟线等等
// 下面的过程用来取得到现在为止, 已经经过了多少个 5 (或15, 等) 分钟线
int GetMinuteXh( int lineperiod,short * fz,short mulnum,short setcode,int bYesterdayFlag)
{   int  itemnum;
    itemnum = GetJRJMinute( fz ,setcode,bYesterdayFlag);
	int tmpRatio = anFZRatio[lineperiod];
	if(lineperiod == PER_MINN)	tmpRatio = mulnum;
    return ( itemnum + tmpRatio - 1 )/( tmpRatio );
}

int GetRealMinuteXh( int lineperiod,short * fz,short mulnum)
{   
	int  itemnum = GetRealTotalMinute(fz);
	int tmpRatio = anFZRatio[lineperiod];
	if(lineperiod == PER_MINN)	tmpRatio = mulnum;
    return ( itemnum + tmpRatio - 1 )/( tmpRatio );
}

short GetFZnoFromMinute( short setcode,int m ,int lineperiod,short * fz,short mulnum)
{   int itemnum = System2Jrj(setcode,m,fz);
	int tmpRatio = anFZRatio[lineperiod];
	if(lineperiod == PER_MINN)	tmpRatio = mulnum;
    return ( itemnum + tmpRatio - 1 )/( tmpRatio );
}

// 基本上是上面过程的逆过程, 根据序号得到当前应该的分钟数
int  GetMinuteFromFZ(int xh,int fznum,short * fz)
{  
	// 	 int i;
	//      // modified by Cui. 1998.04.22
	//      i=fz[0]+(xh+1)*fznum;
	//      if (i>=fz[1]) i+=fz[2]-fz[1];
	//      if (i>=fz[3]) i+=fz[4]-fz[3];
	//      if (i>=fz[5]) i+=fz[6]-fz[5];
	//      if (i>=fz[7]) i=fz[7];
	//      return i;

	// 	int i;
	// 	i= (fz[0]+xh*fznum) % (24*60);
	// 	if (isInTimeSpan(i,fz[1],fz[7]))	i+=TimeSpan(fz[2],fz[1]);
	// 	if (isInTimeSpan(i,fz[3],fz[7]))	i+=TimeSpan(fz[4],fz[3]);
	// 	if (isInTimeSpan(i,fz[5],fz[7]))	i+=TimeSpan(fz[6],fz[5]);
	// 	if (isInTimeSpan(i,fz[7],fz[0]))	i=fz[7];


	int i,allmin=(xh+1)*fznum;	// 比如775分钟，一共780
//	int i,allmin=(xh)*fznum;	// 比如775分钟，一共780
	//i= (fz[0]+xh*fznum) % (24*60);
	i= (fz[0]+(xh+1)*fznum);
//	i= (fz[0]+(xh)*fznum);
	if ( allmin - TimeSpan(fz[1],fz[0]) <= 0 )	// 如果在第一个时段里面
//	if ( allmin - TimeSpan(fz[1],fz[0]) < 0 )	// 如果在第一个时段里面 : 因为 isInTimeSpan 是前闭后开区间
	{
		i= (fz[0]+allmin) % (24*60);		// 实际时间
		return i;			
	}
	else 
	{
		allmin -= TimeSpan(fz[1],fz[0]);		// 去掉第一段
		if ( allmin - TimeSpan(fz[3],fz[2]) <= 0 )	// 在第二段里面
	//	if ( allmin - TimeSpan(fz[3],fz[2]) < 0 )	// 在第二段里面  因为 isInTimeSpan 是前闭后开区间
		{
			i = (fz[2]+allmin)%(24*60);
		}
		else
		{
			allmin -= TimeSpan(fz[3],fz[2]);		// 去掉第2段
			if ( allmin - TimeSpan(fz[5],fz[4]) <= 0 )	// 在第二段里面
		//	if ( allmin - TimeSpan(fz[5],fz[4]) < 0 )	// 在第二段里面  因为 isInTimeSpan 是前闭后开区间
			{
				i = (fz[4]+allmin)%(24*60);
			}
			else
				i = fz[7];
		}
	}
	return i;

}

BOOL GetFileVersion(LPCTSTR strFile, CString& strVersion)    
{    
	TCHAR szVersionBuffer[8192] = _T("");    
	DWORD dwVerSize;    
	DWORD dwHandle;    

	dwVerSize = GetFileVersionInfoSize(strFile, &dwHandle);    
	if (dwVerSize == 0)    
		return FALSE;    

	if (GetFileVersionInfo(strFile, 0, dwVerSize, szVersionBuffer))    
	{    
		VS_FIXEDFILEINFO * pInfo;    
		unsigned int nInfoLen;    

		if (VerQueryValue(szVersionBuffer, _T("\\"), (void**)&pInfo, &nInfoLen)) 
		{ 
			strVersion.Format(_T("%d.%d.%d.%d"),    
				HIWORD(pInfo->dwFileVersionMS), LOWORD(pInfo->dwFileVersionMS),    
				HIWORD(pInfo->dwFileVersionLS), LOWORD(pInfo->dwFileVersionLS));    
			return TRUE;    
		}    
	}    

	return FALSE;    
}

BOOL CenterAndActivateWindow(HWND hWnd)
{
	if (hWnd == NULL)
		return FALSE; // 确保句柄有效

	BOOL bFlag = SetForegroundWindow(hWnd);

	if (!bFlag)
		return bFlag;

	// 尝试激活窗口
	// 获取屏幕尺寸和窗口尺寸以计算居中位置
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);
	RECT rect;
	GetWindowRect(hWnd, &rect); // 获取窗口尺寸和位置
	int windowWidth = rect.right - rect.left;
	int windowHeight = rect.bottom - rect.top;
	int x = (screenWidth - windowWidth) / 2;
	int y = (screenHeight - windowHeight) / 2;
	// 设置窗口位置为居中并保持其大小不变
	SetWindowPos(hWnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER); // 不改变大小，只改变位置和Z序（堆叠顺序）
																	 //重新弹出窗口
	ShowWindow(hWnd, SW_RESTORE);

	return bFlag;
}

BOOL testzs(const char * code,short setcode)
{
	if(5 == strlen(code))
		return false;
    long l = atol(code);
	if(setcode==SH)
	{
		if(l > 901000 || l <= 999l) return TRUE;
		if( memcmp(code,"777",3)== 0 || memcmp(code,"778",3)== 0 || memcmp(code,"779",3)== 0 || code[0] < '0' || code[0] >'9')
			return TRUE;
	}
	else if (setcode == SZ)
	{
		//if(code[0]=='3' && code[1]=='9') return TRUE;
		return (code[0]=='3' && code[1]=='9' && code[2] == '9' );
	}
	else if (setcode == BH)
	{
		return _strnicmp(code,"XBSI",4) == 0;
	}
	else if (setcode == TEA)
	{
		return strcmp(code,"1001") == 0;
	}

	if (l>=880000l&&l<885000l)	   return TRUE;
	if(l>=990000l&&l<994000l)		return TRUE;

	return FALSE;
}
BOOL testzsmk(const char * code,short setcode)	// 模拟指数monk
{
    long l = atol(code);
	
	if (l>=880000l&&l<885000l)	   return TRUE;
	if(l>=990000l&&l<994000l)		return TRUE;
	
	return FALSE;
}
BOOL testzs(struct DiskStkInfoB *pStkInfo)
{
	return testzs(pStkInfo->Code,pStkInfo->SetCode);
}

size_t  GetStringArray(std::vector<std::string> & toids,const char * sendto,BYTE cal)
{
	const char* start = nullptr;
	toids.clear();
	for (const char* p = sendto; p && *p; p++)
	{
		if (*p == cal)
		{
			if (start)
				toids.push_back(std::string(start,p));
			start = nullptr;
			continue;
		}
		if (!start) start = p;
	}
	if (start) 
		toids.push_back(start);
	return toids.size();
}

size_t  GetStringArrayOR(std::vector<std::string> & toids,const char * sendto,const char * strmask)
{
	char* s = StrDupA(sendto);
	toids.clear();

	char* content = nullptr;
	for (char* p = strtok_s(s,strmask,&content); p; p = strtok_s(nullptr,strmask,&content))
	{
		toids.push_back(p);
	}

	return toids.size();
}
size_t	GetStrArrayOR(std::vector<std::string>& toids,  const char* pSendTo, BYTE cal)
{
	toids.clear();

	const char* pStrTo = pSendTo;
	const char* pStrFind = strchr(pStrTo, cal);
	while ( pStrFind )
	{
		std::string strText;
		strText.assign(pStrTo, pStrFind - pStrTo);
		pStrTo = pStrFind + 1;
		pStrFind = strchr(pStrTo, cal);

		toids.push_back( strText);
	}

	if ( pStrTo != NULL && strlen(pStrTo) )
		toids.push_back(pStrTo);

	return toids.size();
}


void AllTrim( char *s) //Alltrim 回车等
{
	char stmp[256];
	BOOL head = TRUE;
	int tailblank = 0,count = 0;
	int len = strlen(s);
	for(unsigned int i = 0;i < len;i++)
	{
		if(s[i]!=' '&&s[i]!='\t'&&s[i]!='\n'&&s[i]!='\r')
		{
			head	  = FALSE;
			tailblank = 0;
		}
		else	tailblank++;
		if(!head)
		{
			stmp[count] = s[i];
			count++;
		}
	}
	if(tailblank==strlen(s)) tailblank = 0;
	if(count-tailblank > 0)
	{
		strncpy(s,stmp,count-tailblank);
		s[count-tailblank] = '\0';
	}
	else
		s[0] = '\0';
}

char * MakeVol(double ftmp)  //特大浮点形的处理
{
	static char stri[20];
	stri[0] = 0; 
	if (ftmp < 0 || fabs(ftmp) > 1000000000000000.0)
		return "N/A";
	if (fabs(ftmp) < 10000.0)
		sprintf(stri,"%7.2f",ftmp);
	else if (fabs(ftmp) < 100000000.0)
		sprintf(stri,("%7.2f万"),ftmp/10000.0);
	else sprintf(stri,("%7.2f亿"),ftmp/100000000.0);

	return stri;
}


char * MakeJE(double ftmp)  //现金额
{ 
	if(ftmp < 0) //已经溢出
		return "N/A";
	return MakeVol(ftmp); //用处理大浮点成交量的方法来处理现金额
}

void	GetNetCardStr(std::vector<std::string> & aNetCard)
{
	PIP_ADAPTER_INFO pAdapterInfo; 
	PIP_ADAPTER_INFO pAdapter = NULL; 
	DWORD dwRetVal = 0; 
	ULONG ulOutBufLen; 
	pAdapterInfo=(PIP_ADAPTER_INFO)malloc(sizeof(IP_ADAPTER_INFO)); 
	ulOutBufLen = sizeof(IP_ADAPTER_INFO); 

	// 第一次调用GetAdapterInfo获取ulOutBufLen大小 
	if (GetAdaptersInfo( pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) 
	{ 
		free(pAdapterInfo); 
		pAdapterInfo = (IP_ADAPTER_INFO *) malloc (ulOutBufLen); 
	} 

	if ((dwRetVal = GetAdaptersInfo( pAdapterInfo, &ulOutBufLen)) == NO_ERROR) 
	{ 
		pAdapter = pAdapterInfo; 
		while (pAdapter) 
		{ 
			char tmpCard[20]={0};
			sprintf(tmpCard,"%02x%02x%02x%02x%02x%02x",pAdapter->Address[0], 
				pAdapter->Address[1], 
				pAdapter->Address[2], 
				pAdapter->Address[3], 
				pAdapter->Address[4], 
				pAdapter->Address[5]); 

			aNetCard.push_back(tmpCard);


			pAdapter = pAdapter->Next; 
		}
		free(pAdapterInfo); 
	} 
	else 
	{ 
		//MT_WARN("Call to GetAdaptersInfo failed.");
		//ERROR("Call to GetAdaptersInfo failed.");
	} 
}


// BKDRHash
unsigned int GenHash(const char* str, unsigned int len,unsigned int seed)
{
	//unsigned int seed = 131; /* 31 131 1313 13131 131313 etc.. */
	unsigned int hash = 0;
	unsigned int i	 = 0;

	for(i = 0; i < len; str++, i++)
	{
		hash = (hash * seed) + (*str);
	}

	return hash;
}
unsigned __int64 GenHash64(const char* str, unsigned int len)
{
	UniKey	uk;
	uk.dwHLKey.LowKey = GenHash(str,len,31);
	uk.dwHLKey.HighKey= GenHash(str,len,131);
	return uk.nKey;
}

BOOL DeleteDirectory(char* psDirName)   
{   
    CFileFind tempFind;   
    char sTempFileFind[ _MAX_PATH ] = { 0 };   
    sprintf(sTempFileFind,"%s//*.*",psDirName);   
	USES_CONVERSION;	//声明标示符
	//调用函数，T2A和W2A均支持ATL和MFC中字符转换
    BOOL IsFinded = tempFind.FindFile(A2CT(sTempFileFind));   
    while (IsFinded)   
    {   
        IsFinded = tempFind.FindNextFile();   
        if (!tempFind.IsDots())  
        {   
            CString sFoundFileName;//[ _MAX_PATH ] = { 0 };   
            // strcpy(sFoundFileName,tempFind.GetFileName().GetBuffer(200));   
			sFoundFileName = tempFind.GetFileName();
            if (tempFind.IsDirectory())   
            {   
                CString sTempDir;//[ _MAX_PATH ] = { 0 };   
                sTempDir.Format(_T("%s//%s"),A2CT(psDirName),sFoundFileName);   
				#ifdef _UNICODE
                DeleteDirectory(T2A(sTempDir));   
				#else
				DeleteDirectory((char*)(const char *)sTempDir);   
				#endif
            }   
            else   
            {   
                CString sTempFileName;//[ _MAX_PATH ] = { 0 };  
                sTempFileName.Format(_T("%s//%s"),A2CT(psDirName),sFoundFileName);   
                DeleteFile(sTempFileName);   
            }   
        }   
    }   
    tempFind.Close();   
    if(!RemoveDirectory(A2CT(psDirName)))   
    {   
        return FALSE;   
    }   
    return TRUE;   
}  


bool TradeSuspension(double fNow, double fBuy, double fSell)
{
	if ( fNow < 0.000001 && fBuy < 0.000001 && fSell < 0.000001)
		return true;

	return false;
}

int	 GetIndexSetDomain(const char* pIndex)
{
	int nOffset = 0;
	if ( memcmp(pIndex, "881", 3) == 0 )		//	地域板块指数对应的setdomain,地域setdoamin=31
	{
		nOffset = 99;
	}
	else if ( memcmp(pIndex, "880", 3) == 0 )	//	行业板块指数对应的setdomain,行业setdoamin=32
	{
		nOffset = 199;
	}
	else if ( memcmp(pIndex, "882", 3) == 0 )	//	概念板块指数对应的setdomain,概念setdoamin=33
	{
		nOffset = 299;
	}
	else										//	没有匹配到，返回-1，以后有可能扩展
	{
		return -1;
	}

	const char* pSetDomain = pIndex + 3;
	return (atol(pSetDomain)+nOffset) ;
}
int accept_timeout(int fd, struct sockaddr_in *addr, int *addrlen, int time)
{
	SOCKET ret = INVALID_SOCKET;
	if(time > 0) {
		fd_set rSet;
		FD_ZERO(&rSet);
		FD_SET(fd, &rSet);

		struct timeval timeout;
		timeout.tv_sec = time;
		timeout.tv_usec = 0;

		int selectRet;
		do {
			selectRet = select(fd + 1, &rSet, NULL, NULL, &timeout);
		}while(selectRet < 0 && selectRet == EINTR);
		if(selectRet < 0 ) {
			return INVALID_SOCKET;
		} else if(selectRet == 0) {
			errno = ETIMEDOUT;
			return INVALID_SOCKET;
		}	
	}
	if(addr) {
		ret = accept(fd, (struct sockaddr *)addr, addrlen);
	} else {
		ret = accept(fd, NULL, NULL);
	}
	return ret;
}

int Comparefloat(double a, double b)
{
	if (!_finite(a))
	{
		return !_finite(b) ? 0 : -1;
	}
	else if (!_finite(b))
	{
		return 1;
	}

	a -= b;
	if (fabs(a) < COMPPREC)
		return 0;
	return a < 0 ? -1 : 1;
}

const char* GetRootPath()
{
	static char s_szMoundlePath[MAX_PATH] = {0};
	if(s_szMoundlePath[0] == 0)
	{
		GetModuleFileNameA(AfxGetApp()->m_hInstance,s_szMoundlePath,sizeof(s_szMoundlePath) - 1);
		char* pos = strrchr(s_szMoundlePath,'\\');
		*pos = 0;
	}
	return s_szMoundlePath;
}
 
//转换精确的时间为老的格式
long_short transJcltime( __int64 jcltime,BOOL bDate )
{
	long_short time;
	if (bDate) 
	{
		time.Date = jcltime/1000000;
	}
	else
	{
		time.Daye1.Minute = CalAbsMin(jcltime);
		time.Daye1.Mon_Day = CalMonthDay(jcltime);
		time.Daye1.Year = CalYear(jcltime)-2004;
	}
	return time;
}

//取得当前系统时间，转化为JclTime
__int64 GetCurTimetoJcl()
{
	time_t  t;
	t = time(NULL);
	struct tm *local; 

	local = localtime(&t);
	__int64   nDate = (local->tm_year+1900)*10000 + (local->tm_mon+1)*100 + local->tm_mday;
	__int64	  nTime = (local->tm_hour*10000 + local->tm_min*100 + local->tm_sec)*1000;
	//JclTime   m_jcltime(nDate*1000000000 + nTime);
	return (nDate*1000000000 + nTime);
}

CString GetModuleDir()
{
	CString homepath;
	::GetModuleFileName(NULL, homepath.GetBuffer(_MAX_PATH), MAX_PATH);
	int pos = homepath.ReverseFind(TEXT('\\'));
	if (pos >= 0 )
	{
		pos+=1;
	}
	homepath.ReleaseBuffer(pos);
	return homepath;
}

bool RegularPath(std::string &sPath)
{
	std::replace(sPath.begin(),sPath.end(), TEXT('/'), TEXT('\\'));
	if (sPath.find(TEXT(':')) == sPath.npos)
		sPath.insert(0, GetModuleDir());
	for (size_t pos = sPath.find(TEXT("\\.\\")); pos != sPath.npos; pos = sPath.find(TEXT("\\.\\"), pos))
	{
		sPath.erase(pos, 2);
	}
	for (size_t pos = sPath.find(TEXT("\\..\\")); pos != sPath.npos; pos = sPath.find(TEXT("\\..\\"), pos))
	{
		size_t last = sPath.rfind(TEXT('\\'), pos - 1);
		ASSERT(pos && last != sPath.npos);
		sPath.erase(last,pos-last+3);
	}	
	return true;
}

bool CreatePath(char* path)
{
	std::string sPath(path);
	RegularPath(sPath);
	for (size_t pos = sPath.find(TEXT('\\')); pos != sPath.npos; pos = sPath.find(TEXT('\\'), pos + 1))
	{
		sPath[pos] = 0;
		DWORD attr = ::GetFileAttributes(sPath.c_str());
		if (attr == -1 || !(attr&FILE_ATTRIBUTE_DIRECTORY))
		{
			if (!::CreateDirectory(sPath.c_str(), NULL))
				return false;
		}
		sPath[pos] = TEXT('\\');
	}
	DWORD attr = ::GetFileAttributes(sPath.c_str());
	if (attr == -1 || !(attr&FILE_ATTRIBUTE_DIRECTORY))
	{
		if (!::CreateDirectory(sPath.c_str(), NULL))
			return false;
	}
	return true;
}

UINT GetCurDate(bool bDate)
{
	time_t now_t = time(NULL);
	tm   now_time;
	localtime_s(&now_time, &now_t);
	if (bDate)
		return (now_time.tm_year + 1900) * 10000 + (now_time.tm_mon + 1) * 100 + now_time.tm_mday;
	else
		return now_time.tm_hour * 10000 + now_time.tm_min * 100 + now_time.tm_sec;
}

//std::string doubleToString(double val, int precise)
//{
//	//std::stringstream ss;
//	//ss << std::setprecision(precise) << val;
//	char buf[1024] = { 0 };
//	std::string t;
//	t.append("%.");
//	t.append(std::to_string(precise));
//	t.append("f");
//	sprintf_s(buf, t.c_str(), val);
//	return std::string(buf);
//}
//
//std::string intToString(int val)
//{
//	return std::to_string(val);
//}

std::string intToString(int64_t val)
{
	return std::to_string(val);
}

bool isInWeekend(int date)
{
	CTime t(date / 10000, date % 10000 / 100, date % 100, 0, 0, 0);
	int idx = t.GetDayOfWeek();
	//1:天 7:六
	return idx == 7 || idx == 1;
}

int md2d(int leap, int month, int day)
{
	for (int i = month - 2; i >= 0; i--)
	{
		day += day_tab[leap][i];
	}
	return day;
}

int y2d(int year)
{
	return 365 + IsLeap(year);
}

long subNumDay(int szDate, int nDay)
{
	return addNumDay(szDate, -nDay);
}

long addNumDay(int szDate, int nDay)
{
	char buf[16] = { 0 };
	sprintf(buf, "%d", szDate);
	string startDate = buf;
	int days = nDay;
	int year = atoi(startDate.substr(0, 4).c_str());
	int month = atoi(startDate.substr(4, 2).c_str());
	int day = atoi(startDate.substr(6).c_str());

	days += md2d(IsLeap(year), month, day);
	//0<days<=365+leap
	int y = year;
	while (days > y2d(y))
	{
		days -= y2d(y);
		y++;
	}
	while (days <= 0)
	{
		y--;
		days += y2d(y);
	}
	//d2md
	int m = 1;
	int d1;
	while ((d1 = days - day_tab[IsLeap(y)][m - 1]) > 0)
	{
		days = d1;
		m++;
	}
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%04d%02d%02d", y, m, days);
	return atol(buf);
}

int DaysBetween2Date(std::string date1, std::string date2)
{
	//取出日期中的年月日
	int year1, month1, day1;
	int year2, month2, day2;
	int result = -1;

	if (!StringToDate(date1, year1, month1, day1))
	{
		//SDK_TRACE("date1 输入的日期格式不正确[%s]\n", date1.c_str());
		return result;
	}
	if (!StringToDate(date2, year2, month2, day2))
	{
		//SDK_TRACE("date2 输入的日期格式不正确[%s]\n", date2.c_str());
		return result;
	}

	if (year1 == year2 && month1 == month2)
	{
		//result = day1 > day2 ? day1 - day2 : day2 - day1;
		result = day2 - day1 + 1;
		return result;
	}
	else if (year1 == year2)	    //如果年相同
	{
		int d1, d2;
		d1 = DayInYear(year1, month1, day1);
		d2 = DayInYear(year2, month2, day2);
		//result = d1 > d2 ? d1 - d2 : d2 - d1;
		result = d2 - d1 + 1;
		return result;
	}
	else			    //年月都不相同
	{
		bool ok = false;
		if (year1 > year2)
		{
			ok = true;
		}

		//确保year1年份比year2早
		if (year1 > year2)
		{
			//swap进行两个值的交换
			swap(year1, year2);
			swap(month1, month2);
			swap(day1, day2);
		}

		int d1, d2, d3;
		if (IsLeap(year1))
		{
			d1 = 366 - DayInYear(year1, month1, day1); //取得这个日期在该年还于下多少天

		}
		else
		{
			d1 = 365 - DayInYear(year1, month1, day1);
		}

		d2 = DayInYear(year2, month2, day2); //取得在当年中的第几天

		d3 = 0;

		for (int year = year1 + 1; year < year2; year++)
		{
			if (IsLeap(year))
			{
				d3 += 366;
			}
			else
			{
				d3 += 365;
			}
		}
		//cout<<d1<<" "<<d2<<"  "<<d3<<endl;

		result = d1 + d2 + d3 + 1;
		result = ok ? -result : result;
		return result;
	}
}
//上面的StringToDate函数用于取出日期中的年月日并判断日期是否合法
//从字符中最得年月日 规定日期的格式是yyyy-mm-dd
bool StringToDate(std::string date, int& year, int& month, int& day)
{
	year = atoi((date.substr(0, 4)).c_str());
	month = atoi((date.substr(4, 2)).c_str());
	day = atoi((date.substr(6, 2)).c_str());

	int DAY[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	if (IsLeap(year))
	{
		DAY[1] = 29;
	}

	return year >= 0 && month <= 12 && month > 0 && day <= DAY[month - 1] && day > 0;
}

//IsLeap函数判断一个年份是否为闰年，方法如下:
bool IsLeap(int year)
{
	return (year % 4 == 0 || year % 400 == 0) && (year % 100 != 0);
}

//DayInYear能根据给定的日期，求出它在该年的第几天，代码如下
int DayInYear(int year, int month, int day)
{
	//int _day = 0;
	int DAY[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	if (IsLeap(year))
		DAY[1] = 29;
	for (int i = 0; i < month - 1; ++i)
	{
		day += DAY[i];
	}
	return day;
}

double round3(double number, int bits)
{
	double num = number;
	for (int i = 0; i < bits; ++i)
	{
		num *= 10;
	}
	num += 0.5;
	for (int i = 0; i < bits; ++i)
	{
		num /= 10;
	}

	return num;
}

double Round(double dval, short iPlaces)
{
	double dRetval;
	double dMod = 0.00001;
	if (dval < 0.0) dMod = -dMod;
	dRetval = dval;
	dRetval += (5.0 / pow(10.0, iPlaces + 1.0));
	dRetval *= pow(10.0, iPlaces);
	dRetval = floor(dRetval + dMod);
	dRetval /= pow(10.0, iPlaces);
	return dRetval;
}

int stringSplit(const std::string str, const std::string sep, std::vector<std::string> &vec)
{
	std::string::size_type begin, end;
	vec.clear();
	if (str.length() == 0) {
		return 0;
	}

	begin = 0;
	while (1) {
		//end = str.find_first_of( sep, begin );
		end = str.find(sep, begin);
		if (end == std::string::npos) {
			break;
		}
		vec.push_back(str.substr(begin, end - begin));
		begin = end + sep.length();
	}
	vec.push_back(str.substr(begin));

	return vec.size();
}

struct zh_sort_struc
{
    double fValue;
    int nIndex;
};

//排序函数
int zh_sort_func(const void * a, const void * b)
{
	struct zh_sort_struc *fsorttmpa, *fsorttmpb;
	fsorttmpa = (struct zh_sort_struc*) a;
	fsorttmpb = (struct zh_sort_struc*) b;
	if (fsorttmpa->fValue < fsorttmpb->fValue) return 1;
	if (fsorttmpa->fValue > fsorttmpb->fValue) return -1;
	if (fsorttmpa->nIndex < fsorttmpb->nIndex) return 1;
	if (fsorttmpa->nIndex > fsorttmpb->nIndex) return -1;
	return 0;
}

bool copyDir(const std::string& src, const std::string& dst)
{
	//cout << endl;
	//cout << "copyDir:" << src << "," << dst << endl;
	string strDir = src, dstDir = dst;
	CreateDirectory(strDir.c_str(), NULL);
	CreateDirectory(dstDir.c_str(), NULL);
	if (strDir.at(strDir.length() - 1) != '\\')
		strDir.append("\\");
	if (dstDir.at(dstDir.length() - 1) != '\\')
		dstDir.append("\\");

	WIN32_FIND_DATA wfd;
	HANDLE hFind = FindFirstFile((strDir + "*.*").c_str(), &wfd);
	if (hFind == INVALID_HANDLE_VALUE)
		return false;
	//SDK_TRACE("copy 1");
	//cout << "aaaa" << endl;
	do {
		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			if (_stricmp(wfd.cFileName, ".") != 0 && _stricmp(wfd.cFileName, "..") != 0) {
				copyDir(strDir + wfd.cFileName, dstDir + wfd.cFileName);
			}
			else {
				CreateDirectory((dstDir + wfd.cFileName).c_str(), NULL);
			}
		}
		else {
			//CopyFile((strDir + wfd.cFileName).c_str(), (dstDir + wfd.cFileName).c_str(), FALSE);
			copyFile(strDir + wfd.cFileName, dstDir + wfd.cFileName);
		}

	} while (FindNextFile(hFind, &wfd));
	FindClose(hFind);
}

bool removeDir(const std::string& strDir2)
{
	std::string strOrignDir = strDir2;
	//CLog::Instance().WriteLog("removeDir", "removeDir", "path: %s", strOrignDir.c_str());
	if (strOrignDir.at(strOrignDir.length() - 1) != '\\')
	{
		strOrignDir.append("\\");
	}
	//删除文件夹下所有文件
	WIN32_FIND_DATA wfd;
	HANDLE hFind = FindFirstFile((strOrignDir + "*.*").c_str(), &wfd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	do
	{
		if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (_stricmp(wfd.cFileName, ".") != 0 && _stricmp(wfd.cFileName, "..") != 0)
			{
				removeDir(strOrignDir + wfd.cFileName);
			}
		}
		else
		{
			DeleteFile((strOrignDir + wfd.cFileName).c_str());
		}
	} while (FindNextFile(hFind, &wfd));
	FindClose(hFind);
	//删除该文件夹
	if (!RemoveDirectory(strOrignDir.c_str()))
	{
		DWORD dCode = GetLastError();
		if (dCode == ERROR_DIR_NOT_EMPTY)
		{
			removeDir(strOrignDir);
		}
		return false;
	}
	return true;
}

bool removeDir2(const std::string& strDir)
{
	char buf[1024] = { 0 };
	sprintf(buf, "rd %s /s /q", strDir.c_str());
	system(buf);
	return true;
}

bool copyFile(const std::string& src, const std::string& dst)
{
	if (!checkFileExist(src)) return false;
	if (checkFileExist(dst))
		DeleteFile(dst.c_str());

	// mkdir
	CopyFile(src.c_str(), dst.c_str(), FALSE);
	return false;
}

bool checkFolderExist(const string& strPath)
{
	WIN32_FIND_DATA  wfd;
	bool rValue = false;
	HANDLE hFind = FindFirstFile(strPath.c_str(), &wfd);
	if ((hFind != INVALID_HANDLE_VALUE) && (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
	{
		rValue = true;
	}
	FindClose(hFind);
	return rValue;
}

bool checkFileExist(const string& strPath)
{
	WIN32_FIND_DATA  wfd;
	bool rValue = false;
	HANDLE hFind = FindFirstFile(strPath.c_str(), &wfd);
	if ((hFind != INVALID_HANDLE_VALUE))
	{
		rValue = true;
	}
	FindClose(hFind);
	return rValue;
}

// 获取目录下所有文件
int getAllFiles(std::string strDir, std::vector<std::string>& filesVec)
{
	WIN32_FIND_DATA findData;
	std::string strFindFile = strDir + "\\*";
	HANDLE hFind = FindFirstFile(strFindFile.c_str(), &findData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return -1;
	}
	do
	{
		if (std::string(findData.cFileName).compare(".") == 0 ||
			std::string(findData.cFileName).compare("..") == 0)continue;
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			//如果是目录
			std::string strDir1 = strDir + "\\";
			strDir1.append(std::string(findData.cFileName).c_str());
			getAllFiles(strDir1, filesVec);
		}
		else
		{
			filesVec.push_back(strDir + "\\" + std::string(findData.cFileName));
		}
	} while (FindNextFile(hFind, &findData));
	FindClose(hFind);
	return 0;
}

int getSubDirsAndFiles(std::string strDir, std::vector<std::string>& filesVec)
{

	WIN32_FIND_DATA findData;
	std::string strFindFile = strDir + "\\*";
	HANDLE hFind = FindFirstFile(strFindFile.c_str(), &findData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		return -1;
	}
	do
	{
		if (std::string(findData.cFileName).compare(".") == 0 ||
			std::string(findData.cFileName).compare("..") == 0)continue;
		//if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		//{
		//	//如果是目录
		//	std::string strDir1 = strDir + "\\";
		//	strDir1.append(std::string(findData.cFileName).c_str());
		//	getAllFiles(strDir1, filesVec);
		//}
		//else
		{
			filesVec.push_back(strDir + "\\" + std::string(findData.cFileName));
		}
	} while (FindNextFile(hFind, &findData));
	FindClose(hFind);
	return 0;
}

FILE* CreateAppendFile(LPCSTR szFile)
{
	FILE* fp = _fsopen(szFile, "r+b", _SH_DENYNO);
	if (!fp)
		fp = _fsopen(szFile, "wb", SH_DENYNO);
	if (!fp)
	{
		//AfxMessageBox("无法生成" + CString(szFile), MB_OK | MB_ICONEXCLAMATION);
		return NULL;
	}
	return fp;
}

DWORD filelength(FILE* fp)
{
	DWORD oldpos, filelen;

	oldpos = ftell(fp);
	if (oldpos < 0) return 0;
	fseek(fp, 0L, SEEK_END);
	filelen = ftell(fp);
	if (filelen < 0) return 0;
	fseek(fp, oldpos, SEEK_SET);
	return filelen;
}

__int64 GetFileLength(LPCSTR szFile)
{
	__int64 qwFileSize = 0;
	HANDLE hFile = CreateFileA(szFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwHigh;
		DWORD dwLow = ::GetFileSize(hFile, &dwHigh);
		qwFileSize = dwLow + (((__int64)dwHigh) << 32);
	}
	CloseHandle(hFile);
	return qwFileSize;
}

string getFileName(const string& path, bool needSufix)
{
	string _path = path, name;
	auto pos = _path.find('/');
	while (pos != std::string::npos) {
		_path.replace(pos, 1, "\\");
		pos = _path.find('/', pos + 1);
	}

	pos = _path.find_last_of('\\');
	if (pos != std::string::npos) {
		name = _path.substr(pos + 1);
		if (!needSufix) {
			pos = name.find('.');
			if (pos != std::string::npos) {
				name = name.substr(0, pos);
			}
		}
	}
	return name;
}

bool mkpath(const std::string& strPath)
{
	string _path = strPath;
	auto pos = _path.find('/');
	while (pos != std::string::npos) {
		_path.replace(pos, 1, "\\");
		pos = _path.find('/', pos + 1);
	}

	pos = _path.find('\\');
	while (pos != std::string::npos) {
		auto tt = _path.substr(0, pos);
		if (!checkFolderExist(tt)) {
			CreateDirectory(tt.c_str(), NULL);
		}
		//else {
		//	return true;
		//}
		pos = _path.find('\\', pos + 1);
	}

	return true;
}

bool replace(std::string& str, const std::string& substr, const std::string& restr)
{
	auto pos = str.find(substr);
	while (pos != std::string::npos) {
		str.replace(pos, 1, restr);
		pos = str.find(substr);
	}
	return true;
}


void splitStr(string str, const const char split, vector<string>& rst)
{
	istringstream iss(str);    // 输入流
	string token;            // 接收缓冲区
	while (getline(iss, token, split))    // 以split为分隔符
	{
		rst.push_back(token);
	}
}

//不考虑CreateEvent，因为程序异常退出内核事件一直存在
long InitProcessSem(HANDLE &p_hProcessSem, const char *p_szSemName, long p_lInitCount, long lMaxCount)
{
	HANDLE handle = NULL;
	long lErrRet = 0;

	//尝试打开
	handle = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, p_szSemName);
	if (handle != NULL)
	{
		lErrRet = GetLastError();
		//已经存在的则关闭本次句柄，此时p_hProcessSem不处理
		if (lErrRet == ERROR_ALREADY_EXISTS)
		{
			CloseHandle(handle);
			handle = NULL;
		}
		return lErrRet;
	}
	//打开失败则创建
	handle = CreateSemaphore(NULL, p_lInitCount, lMaxCount, p_szSemName);
	if (handle != NULL)
	{
		lErrRet = GetLastError();
		//已经存在的则关闭本次句柄，此时p_hProcessSem不处理
		if (lErrRet == ERROR_ALREADY_EXISTS)
		{
			CloseHandle(handle);
			handle = NULL;
		}
		else
		{
			p_hProcessSem = handle;
		}
	}
	else
	{
		lErrRet = GetLastError();
		p_hProcessSem = NULL;
	}
	return lErrRet;
}

long WaitForProcessSem(const char *p_szSemName)
{
	long lErrRet = -1;
	HANDLE handle = NULL;
	handle = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, p_szSemName);
	if (handle != NULL)
	{
		lErrRet = 0;
		CloseHandle(handle);
		handle = NULL;
	}
	else
	{
		lErrRet = GetLastError();
	}
	return lErrRet;
}

int KillProcess(long p_lProcessID)
{
	int iRet = -1;
	if (p_lProcessID < 0)
		return iRet;

	HANDLE hProcess = ::OpenProcess(PROCESS_TERMINATE, FALSE, p_lProcessID);
	if(NULL == hProcess)
		return -2;

	iRet = ::TerminateProcess(hProcess, 4) ? 0 : -3;
	CloseHandle(hProcess);
	return iRet;
}

long StartProcess(const char *p_szProcessWorkPath, const char *p_szProcessFullPath)
{
	if (p_szProcessFullPath != NULL && _access(p_szProcessFullPath, 0) == -1 &&
		p_szProcessWorkPath != NULL && _access(p_szProcessWorkPath, 0) == -1)
	{
		return -2;
	}

	long lPid = -1;
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// 指定要启动的exe文件路径
	TCHAR szFullPath[MAX_PATH] = { 0 };

	// 指定要启动的exe工作路径
	TCHAR szWorkPath[MAX_PATH] = { 0 };

	_stprintf_s(szFullPath, MAX_PATH, _T("%s"), p_szProcessFullPath);
	_stprintf_s(szWorkPath, MAX_PATH, _T("%s"), p_szProcessWorkPath);

	// 创建一个异步进程
	if (!CreateProcess(szFullPath, NULL, NULL, NULL, FALSE, 0, NULL, szWorkPath, &si, &pi)) {
		return -1;
	}
	else
	{
		lPid = pi.dwProcessId;
	}

	// 关闭进程和线程句柄
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return lPid;
}

int FindProcess(const char *p_szProcessName)
{
	if (p_szProcessName == NULL || strlen(p_szProcessName) == 0)
	{
		return -99;
	}
	PROCESSENTRY32 stPe;
	stPe.dwSize = sizeof(stPe);
	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == NULL)
	{
		return -1;
	}
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return -2;
	}
	bool bMore = ::Process32First(hProcessSnap, &stPe);
	int iFind = -3;
	while (bMore)
	{
		if (_stricmp(p_szProcessName, stPe.szExeFile) == 0)
		{
			iFind = 0;
			break;
		}
		bMore = ::Process32Next(hProcessSnap, &stPe);
	}
	CloseHandle(hProcessSnap);//关闭快照句柄
	hProcessSnap = NULL;

	return iFind;
}

long FindProcessid(const char *p_szProcessName)
{
	if (p_szProcessName == NULL || strlen(p_szProcessName) == 0)
	{
		return -99;
	}
	PROCESSENTRY32 stPe;
	stPe.dwSize = sizeof(stPe);
	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == NULL)
	{
		return -1;
	}
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return -2;
	}

	int iPid = -3;
	bool bMore = ::Process32First(hProcessSnap, &stPe);
	while (bMore)
	{
		if (_stricmp(p_szProcessName, stPe.szExeFile) == 0)
		{
			iPid = stPe.th32ProcessID;
			break;
		}
		bMore = ::Process32Next(hProcessSnap, &stPe);
	}
	CloseHandle(hProcessSnap);//关闭快照句柄
	hProcessSnap = NULL;

	return iPid;
}

long IsProcessIdExists(long p_iProcessId) 
{
	/*
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);
	if (hProcess != NULL) {
		CloseHandle(hProcess); // 如果句柄非空，则关闭句柄
		return dwProcessId; // 进程存在
	}
	else if (GetLastError() == ERROR_INVALID_PARAMETER) {
		return -1; // 进程不存在，错误为 ERROR_INVALID_PARAMETER
	}
	CloseHandle(hProcess);
	return -1; // 发生未知错误
	*/
	if (p_iProcessId < 0)
	{
		return -1;
	}

	PROCESSENTRY32 stPe;
	stPe.dwSize = sizeof(stPe);
	HANDLE hProcessSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hProcessSnap == NULL)
	{
		return -2;
	}
	if (hProcessSnap == INVALID_HANDLE_VALUE)
	{
		return -3;
	}

	int iPid = -4;
	bool bMore = ::Process32First(hProcessSnap, &stPe);
	while (bMore)
	{
		if (stPe.th32ProcessID == p_iProcessId)
		{
			iPid = stPe.th32ProcessID;
			break;
		}
		bMore = ::Process32Next(hProcessSnap, &stPe);
	}
	CloseHandle(hProcessSnap);//关闭快照句柄
	hProcessSnap = NULL;

	return iPid;
}

bool TranStrCode(LPCSTR szinfo, tagCode& stk)
{
	stk.setcode = -1;
	short st = -1;
	const char* szCode = GetStkInfo(szinfo, st);
	if (st != -1)
	{
		stk.setcode = st;
		strcpy(stk.code, szCode);
		return true;
	}
	else
		return false;
}

// 期货和渤海，白银的不准，要细分
float GetTPPrice(short setcode, const char* Code, const char* Name, char xs_flag, float Close, BOOL bUp)
{
	float fUnit = 100.0;
	//long  l_Close = tranf2long(Close,xs_flag);
	fUnit = pow(10.0, xs_flag);
	int nValue = (int)(Close * fUnit + 0.5);
	StockCodeType Type = GetStockType(setcode, const_cast<char*>(Code));
	int iRet = 0;

	if (Type.u.SHKCB || Type.u.SZCY)
	{
		if (bUp)
			iRet = (int)(nValue * 120 + 0.5);
		else
			iRet = (int)(nValue * 80 + 0.5);
	}
	else if (strncmp(Name, "ST", 2) == 0 || strncmp(Name, "*ST", 3) == 0)		//ST
	{
		if (bUp)
			iRet = (int)(nValue * 105 + 0.5);
		else
			iRet = (int)(nValue * 95 + 0.5);
	}
	else if (strncmp(Name, "N", 1) == 0)	//N
	{
		if (bUp)
			iRet = (int)(nValue * 144 + 0.5);
		else
			iRet = (int)(nValue * 64 + 0.5);
	}
	else if (strncmp(Name, "PT", 2) == 0)	//PT
	{
		if (bUp)
			iRet = (int)(nValue * 105 + 0.5);
		else
			iRet = 0;
	}
	//else if(Type==CODE_SB)	//三板
	//{	
	//	if(bUp)	
	//		fRet = (nValue * 1.05)/fUnit;
	//	else	
	//		fRet = (nValue * 0.95)/fUnit;
	//}
	//else if(Type==CODE_SZQZ || Type==CODE_SZGZ || Type==CODE_SZZQ || Type==CODE_SZKZHZQ || Type==CODE_SZGZHG 
	//	|| Type==CODE_SHQZ || Type==CODE_SHGZ || Type==CODE_SHZQ || Type==CODE_SHKZHZQ || Type==CODE_SHGZHG)//国债等
	else if (Type.IsHsBoud())
	{
		iRet = 0.0;
	}
	else												   //其它
	{
		if (bUp)
			iRet = (int)(nValue * 110 + 0.5);
		else
			iRet = (int)(nValue * 90 + 0.5);
	}

	iRet = (iRet + 50) / 100;

	return iRet / fUnit;
}

short GetmarketNum(const char* cSetcode)
{
	CString strSetCode = cSetcode;
	strSetCode.MakeUpper();
	for (int st = 0; st < MAX_MARKETNUM; st++)
	{
		if (strSetCode.Compare(g_strMarkCode[st]) == 0)
			return st;
	}
	return -1;
}

__int64 ConvertTimet(LPCTSTR lpTime, bool tran)
{
	long nHour, nMin;
	float fSecond = 0;
	sscanf(lpTime, _T("%d:%d:%f"), &nHour, &nMin, &fSecond);

	__int64 date = GetCurDate();
	__int64 minsec = nHour * 10000 + nMin * 100 + fSecond;
	if (minsec == 0 || tran) minsec = GetCurDate(false);
	return date * 1000000 * 1000 + minsec * 1000;
}

void SplitStr(const char* szValue, char a, std::vector<string>& vecSub)
{
	std::string src = szValue;
	vecSub.clear();
	string::size_type pos1, pos2;
	pos2 = src.find(a);
	pos1 = 0;
	while (string::npos != pos2)
	{
		vecSub.push_back(src.substr(pos1, pos2 - pos1));
		pos1 = pos2 + 1;
		pos2 = src.find(a, pos1);
	}
	vecSub.push_back(src.substr(pos1));
}

bool IsEquals(double a, double b)
{
	static uint64_t mask = ~0xfffffffui64;
	uint64_t& ra = *reinterpret_cast<uint64_t*>(&a);
	uint64_t& rb = *reinterpret_cast<uint64_t*>(&b);
	return ((ra ^ rb) & mask) == 0;
}

bool IsEquals(double a, double b, int xs_flag)
{
	return fabs(a - b) < pow(.1, xs_flag);
}

bool IsDoubleZero(double x)
{
	return IsEquals(x, 0);
}

bool testzs_other(short setcode, const char* code)	// 由我们软件约定的指数类型：分类板块指数（SH） 
{
	if (setcode == SH)
	{
		return (code[0] == '8' && code[1] == '8' && code[2] >= '0' && code[2] <= '9') ||
			(code[0] == '9' && code[1] == '9' && code[2] >= '0' && code[2] <= '4') ||
			(code[0] == '7' && code[1] == '7' && code[2] >= '7' && code[2] <= '9') ||
			(code[0] < '0' || code[0] > '9');
	}
	else
		return false;
}

// 按照代码规则判断，如果不是，就直接用外部指定的xs_flag判断
short  GetXSFlag(short setcode, char* Code, char xs_flag, int chStockFlag)
{
	//short nStockType;
	if (testzs(setcode, Code, chStockFlag))
		return ZS;
	else if (IsQHMarket(setcode))
		return xs_flag;
	if (setcode == NYSE || setcode == NASDAQ || setcode == American)
	{
		return XS2;
	}
	if (setcode >= HK)
	{
		if (Code[0] == 'T')
			return XS3;

		if (!strncmp(Code + 1, "313", 3))
			return XS2;
		else
			return NOXS;
	}

	StockCodeType nStockType = GetStockType(setcode, Code, chStockFlag);

	if (nStockType.IsShBondNoCov() || nStockType.IsSzBondNoCov())
		return XS3;

	if (nStockType.IsShBg() || nStockType.IsSzBoud() || nStockType.IsSzKZZ() || nStockType.IsHsJj() || nStockType.IsShKZZ() || nStockType.u.SHJJ_SPETF || nStockType.u.SHJJ_ETF)
		return XS3;

	if (strlen(Code) == 8)	///>8位代码
	{
		if (setcode == SH && Code[0] == '1' && Code[1] == '0' && Code[2] == '0' && Code[3] == '0')
			return XS4;
		if (setcode == SZ && Code[0] == '9' && Code[1] == '0' && Code[2] == '0' && Code[3] == '0')
			return XS4;
	}

	if (nStockType.u.SHHG_ZQZYSHG_ZHTG || nStockType.u.SZHG)
	{
		return XS3;

		// fangz 待客户端同步修改
		// return XS35;
	}

	return XS2;
}

//判断该市场的品种代码是否为指数
short testzs(short setcode, char* code, int chStockFlag)
{
	if (SZ == setcode || SH == setcode)
	{
		if (strlen(code) == 5)
		{
			// 港股通的
			return FALSE;
		}

		if (ZZSTKTYPE == chStockFlag || GZSTKTPYE == chStockFlag)
		{
			return TRUE;
		}
	}
	if (SZ == setcode)
	{
		if (code[0] == '3' && code[1] == '9')
			return TRUE;
		return FALSE;
	}
	if (HX == setcode)
	{
		if (!_strnicmp(code, "HXZS", 4))
			return TRUE;
		else if (!_strnicmp(code, "YSXBZS", 6))
			return TRUE;
		else if (!strncmp(code, "YBKZS", 5))
			return TRUE;
		return FALSE;
	}
	long l = atol(code);
	if (l == 0) return 0;
	if (l < 30l)         return TRUE;
	if (l > 9999999)		return FALSE;
	if (l > 999000l)     return TRUE;
	if (l > 880000l && l < 885000l)	   return TRUE;
	if (l > 990000l && l < 994000l)		return TRUE;
	if (SH == setcode)
	{	// SH000959  银河99   --- 目前网际风是这样
		if (l < 1000)         return TRUE;
	}
	return FALSE;
}

StockCodeType GetStockType(short setcode, char* code, int chStockFlag)
{
	StockCodeType type = { { 0 } };
	if (setcode == SZ)
	{
		type.u.reserved = 1;
		if (5 == strlen(code))
		{
			// 深港通
			type.u.SZHK = 1;
			return type;
		}

		if (GZSTKTPYE == chStockFlag)
		{
			type.u.SZZS = 1;
			return type;
		}

		//
		switch (code[0])
		{
		case '0':
			switch (code[1])
			{
			case '0':
				type.u.SZAG = code[2] < '5';										//000***   A股(主板)
																					//001***   A股(主板)
				type.u.SZZX = code[2] == '2' || code[2] == '3' || code[2] == '4';	//002***   A股(中小板)
																					//003***   A股(中小板)
																					//004***   A股(中小板)
				break;
			case '3':
				type.u.SZQZ_SUB = code[2] == '0'									//030***   A股认购权证
					|| code[2] == '1'												//031***   A股认购权证
					|| code[2] == '2';												//032***   A股认购权证
				type.u.SZQZ_PUT = code[2] == '8' || code[2] == '9';					//038***   A股认沽权证
																					//039***   A股认沽权证
				break;
			case '7':
				type.u.SZPUB_AGZF = 1;												//07****   A股增发
				break;
			case '8':
				type.u.SZQZ_AGPG = 1;												//08****   A股配股权证
				break;
			}
			break;
		case '1':
		{
			// 非可转债:100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,1215,1216,1217,1218,1219,1318,133,134,135,136,137,138,139,145,148,149,19
			if ('0' == code[1] || '1' == code[1])
			{
				type.u.SZBOND_NOCOV = 1;
			}
			else if ('2' == code[1])
			{
				if ('0' == code[2])
				{
					type.u.SZBOND_NOCOV = 1;
				}
				if ('1' == code[2] && ('5' == code[3] || '6' == code[3] || '7' == code[3] || '8' == code[3] || '9' == code[3]))
				{
					type.u.SZBOND_NOCOV = 1;
				}
			}
			else if ('3' == code[1])
			{
				if (('1' == code[2] && '8' == code[3]) || '3' == code[2] || '4' == code[2] || '5' == code[2] || '6' == code[2] || '7' == code[2] || '8' == code[2] || '9' == code[2])
				{
					type.u.SZBOND_NOCOV = 1;
				}
			}
			else if ('4' == code[1])
			{
				if ('5' == code[2] || '8' == code[2] || '9' == code[2])
				{
					type.u.SZBOND_NOCOV = 1;
				}
			}
			else if ('9' == code[1])
			{
				type.u.SZBOND_NOCOV = 1;
			}

			//
			switch (code[1])
			{
			case '0':
				type.u.SZBOND_GZQ = code[2] == '0' || code[2] == '1' || code[2] == '2';				//100***   国债
																					//101***   国债 102*** 国债
				type.u.SZBOND_DFZQ = code[2] == '4'									//104***   地方政府债券
					|| code[2] == '5'												//105***   地方政府债券
					|| code[2] == '6'												//106***   地方政府债券
					|| code[2] == '7'												//107***   地方政府债券
					|| code[2] == '9';												//109***   地方政府债券
				type.u.SZBOND_TXGZQ = code[2] == '8' && (code[3] == '2' || code[3] == '3');				//1082**   贴现国债 1083** 贴现国债
				type.u.SZBOND_JRZQ = code[2] == '8' && (code[3] == '6'				//1086**   政策性金融债券 1088**
					|| code[3] == '8' || code[3] == '9');												//1089**   政策性金融债券
				break;
			case '1':
				switch (code[2])
				{
				case '1':
					type.u.SZBOND_QYZQ = code[3] == '0';							//1110**   企业债券
					type.u.SZBOND_ZFZCZQ = code[3] == '9';							//1119**   政府支持债券
					break;
				case '2':
					type.u.SZBOND_GSZQ = 1;											//112***   公司债券
					type.u.SZBOND_CXPZGSZQ = code[3] < '8';							//1120**   创新品种公司债券
													//1121**   创新品种公司债券
													//1122**   创新品种公司债券
													//1123**   创新品种公司债券
													//1124**   创新品种公司债券
													//1125**   创新品种公司债券
													//1126**   创新品种公司债券
													//1127**   创新品种公司债券
					break;
				case '4':
					type.u.SZBOND_FGKFXGSZQ = 1;									//1140**   非公开发行公司债券
													//1141**   非公开发行公司债券
													//1142**   非公开发行公司债券
													//1143**   非公开发行公司债券
													//1144-1148
					type.u.SZBOND_CXPZGSZQ = code[3] == '2' || code[3] == '3';		//1142**   创新品种公司债券 
													//1143**   创新品种公司债券
					break;
				case '5':
					type.u.SZBOND_KZHGSZQ = 1;										//115***   认股权和债券分离交易的可转换公司债券
					break;
				case '6':
					type.u.SZBOND_ZCZCZQ = 1;										//116***   企业资产支持证券债券
					break;
				case '7':
					type.u.SZBOND_FGKFXKJHGSZQ = code[3] == '0' || code[3] == '1';	//1170**   非公开发行可交换公司债券
													//1171**   非公开发行可交换公司债券
					type.u.SZBOND_ZQGSDQGSZQ = code[3] == '5';						//1175**   证券公司短期公司债券
					break;
				case '8':
					type.u.SZBOND_CXPZGSZQ = code[3] > '2';							//1183**   创新品种公司债券
													//1184**   创新品种公司债券
													//1185**   创新品种公司债券
													//1186**   创新品种公司债券
													//1187**   创新品种公司债券
													//1188**   创新品种公司债券
													//1189**   创新品种公司债券
					type.u.SZBOND_FGKFXGSZQ = code[3] > '3' && code[3] < '9';		//1183**   非公开发行公司债券
													//1184**   非公开发行公司债券
													//1185**   非公开发行公司债券
													//1186**   非公开发行公司债券
													//1187**   非公开发行公司债券
													//1188**   非公开发行公司债券
					type.u.SZBOND_ZQGSCJZQ = code[3] == '9';						//1189**   证券公司次级债券
					break;
				case '9':
					type.u.SZBOND_ZCZCZQ = code[3] == '0'							//1190**   企业资产支持证券债券
						|| code[3] == '1'											//1191**   企业资产支持证券债券
						|| code[3] == '2';											//1192**   企业资产支持证券债券
					type.u.SZBOND_BDCTZXTQYZQ = code[3] == '4';						//1194**   不动产投资信托基金
					break;
				}
				break;
			case '2':
				type.u.SZBOND_KJHGSZQ = code[2] == '0';								//120***   可交换公司债券
				type.u.SZBOND_KZHZQ = code[2] == '3' || code[2] == '7' || code[2] == '8';		//123*** 127*** 128*** 可转债
				type.u.SZBOND_BDCTZXTQYZQ = code[2] == '1';							//121***   不动产信托
				break;
			case '3':
				type.u.SZHG = 1;													//13****   债券回购
				type.u.SZBOND_ZCZCZQ = code[2] == '7' || code[2] == '8' || code[2] == '9';	//139***   企业资产支持证券债券
				break;
			case '4':
				type.u.SZYXG = code[2] == '0';										//140***   优先股
				type.u.SZBOND_GSZQ = code[2] == '9';								//149***   公司债
				break;
			case '5':
				type.u.SZJJ_LOF = 1;																	// 15****   开放式基金
				type.u.SZJJ_LOFFJ = code[2] == '0';										// 150***   分级基金
				type.u.SZJJ_ETF = (code[2] == '8' || code[2] == '9');	// 158***, 159***   ETF基金
				break;
			case '6':
				type.u.SZJJ_LOF = 1;												//16****   开放式基金(LOF基金)
				break;
			case '7':
			case '8':
				type.u.SZREITS = code[2] == '0';
				type.u.SZJJ_FBS = 1;												//18****   传统封闭式证券投资基金
				break;
			case '9':
				type.u.SZBOND_DFZQ = code[2] == '1';	//191*** 地方政府债券
			}
		}
		break;
		case '2':
			type.u.SZBG = code[1] == '0';											//20****   B股
			type.u.SZQZ_BG = code[1] == '8';										//28****   B股权证
			break;
		case '3':
			type.u.SZAG = type.u.SZCY = code[1] == '0';								//30****   A股(创业板)
			type.u.SZWLTP = code[1] == '6';											//36****   网络投票
			type.u.SZMMJHGS = code[1] == '6' && code[2] == '9' && code[3] == '9'
				&& code[4] == '9' && code[5] == '9';								//369999   投资者服务密码激活/挂失处理
			type.u.SZPUB_CYB = code[1] == '7';										//37****   创业板增发
			type.u.SZQZ_CYB = code[1] == '8';										//38****   创业板权证
			type.u.SZZS = code[1] == '9';											//39****   指数
			break;
		case '9':
			type.u.SZQQ = 1;		//9***** 深圳期权
		}
	}
	else if (setcode == SH)
	{
		type.u.reserved = 1;

		if (5 == strlen(code))
		{
			// 沪港通
			type.u.SHHK = 1;
			return type;
		}

		if (ZZSTKTYPE == chStockFlag)
		{
			type.u.SHZS = 1;
			return type;
		}

		if ((code[0] == '8' && code[1] == '8' && code[2] >= '0' && code[2] <= '9') ||
			(code[0] == '9' && code[1] == '9' && code[2] >= '0' && code[2] <= '4') ||
			(code[0] == '7' && code[1] == '7' && code[2] >= '7' && code[2] <= '9') ||
			(code[0] < '0' || code[0] > '9'))
		{
			type.u.SHZS = 1;
		}

		// 上海非可转债
		// 009, 010, 018, 019, 020, 132, 160, 163, 171, 173, 175, 184, 185, 186, 188, 090, 091, 099, 101, 102, 103, 104, 105, 106, 107, 108, 109, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 133, 134, 135, 136, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 152, 153, 154, 155, 156, 157, 158, 159, 204, 207, 750, 751
		if ('0' == code[0])
		{
			if ('0' == code[1] && '9' == code[2])
			{
				type.u.SHBOND_NOCOV = 1;
			}
			else if ('1' == code[1] && ('0' == code[2] || '8' == code[2] || '9' == code[2]))
			{
				type.u.SHBOND_NOCOV = 1;
			}
			else if ('2' == code[1] && '0' == code[2])
			{
				type.u.SHBOND_NOCOV = 1;
			}
			else if ('9' == code[1] && ('0' == code[2] || '1' == code[2] || '9' == code[2]))
			{
				type.u.SHBOND_NOCOV = 1;
			}
		}
		else if ('1' == code[0])
		{
			if ('0' == code[1])
			{
				if ('1' == code[2] || '2' == code[2] || '3' == code[2] || '4' == code[2] || '5' == code[2] || '6' == code[2] || '7' == code[2] || '8' == code[2] || '9' == code[2])
				{
					type.u.SHBOND_NOCOV = 1;
				}
				else if ('0' == code[2] && '0' == code[3])
				{
					type.u.SHQQ = 1;
				}
			}
			else if ('2' == code[1])
			{
				type.u.SHBOND_NOCOV = 1;
			}
			else if ('3' == code[1])
			{
				if ('0' == code[2] || '1' == code[2] || '2' == code[2] || '3' == code[2] || '4' == code[2] || '5' == code[2] || '6' == code[2] || '9' == code[2])
				{
					type.u.SHBOND_NOCOV = 1;
				}
			}
			else if ('4' == code[1])
			{
				type.u.SHBOND_NOCOV = 1;
			}
			else if ('5' == code[1])
			{
				if ('2' == code[2] || '3' == code[2] || '4' == code[2] || '5' == code[2] || '6' == code[2] || '7' == code[2] || '8' == code[2] || '9' == code[2])
				{
					type.u.SHBOND_NOCOV = 1;
				}
			}
			else if ('6' == code[1] && ('0' == code[2] || '3' == code[2]))
			{
				type.u.SHBOND_NOCOV = 1;
			}
			else if ('7' == code[1] && ('1' == code[2] || '3' == code[2] || '5' == code[2]))
			{
				type.u.SHBOND_NOCOV = 1;
			}
			else if ('8' == code[1] && ('4' == code[2] || '5' == code[2] || '6' == code[2] || '8' == code[2]))
			{
				type.u.SHBOND_NOCOV = 1;
			}
		}
		else if ('2' == code[0])
		{
			if ('0' == code[1] && ('4' == code[2] || '7' == code[2]))
			{
				type.u.SHBOND_NOCOV = 1;
			}
		}
		else if ('7' == code[0])
		{
			if ('5' == code[1] && ('0' == code[2] || '1' == code[2]))
			{
				type.u.SHBOND_NOCOV = 1;
			}
		}

		switch (code[0])
		{
		case '0':
			switch (code[1])
			{
			case '0':
				type.u.SHZS = code[2] == '0';										//000***   上证指数系列、中证指数系列
				type.u.SHBOND_GZ = code[2] == '9';									//009***   国债(2000年前发行)
				break;
			case '1':
				type.u.SHBOND_GZ = code[2] == '0' || code[2] == '9';				//010***   国债(2000-2009年发行)
																					//019***   国债(2010年及以后发行)
				type.u.SHBOND_ZCXYHJRZQ = code[2] == '8';							//018***   政策性银行金融债
				break;
			case '2':
				type.u.SHBOND_TXGZ = code[2] == '0';								//020***   记账式贴现国债
				break;
			case '9':
				type.u.SHZYQ_GZZYSHG = code[2] == '0'								//090***   国债质押式回购质押券出入库(对应010***国债)
					|| code[2] == '1'												//091***   国债质押式回购质押券出入库(对应019***国债)
					|| code[2] == '9';												//099***   国债质押式回购质押券出入库(对应009***国债)
				break;
			}
			break;
		case '1':
			switch (code[1])
			{
			case '0':
				switch (code[2])
				{
				case '0':
					type.u.SHBOND_GZ = code[3] != '9';							//100***   100000-100899用于可转换公司债券（对应600***)，已不再增用
					type.u.SHHG_BOND = code[3] == '9';								//1009**   债券回售
					break;
				case '2':
				case '3':
					type.u.SHZYQ_QYZQ = 1;											//102***   企业债券质押券出入库(对应127***)
																					//103***   企业债券质押券出入库(对应124***)
					break;
				case '4':
					type.u.SHZYQ_GSZQ = code[3] < '5';								//1040**   公司债券质押券出入库（对应122***)
																					//1041**   公司债券质押券出入库（对应122***)
																					//1042**   公司债券质押券出入库（对应122***)
																					//1043**   公司债券质押券出入库（对应122***)
																					//1044**   公司债券质押券出入库（对应122***)
					type.u.SHZYQ_QYZQ = code[3] > '4';								//1045**   企业债券质押券出入库（对应122***)
																					//1046**   企业债券质押券出入库（对应122***)
																					//1047**   企业债券质押券出入库（对应122***)
																					//1048**   企业债券质押券出入库（对应122***)
																					//1049**   企业债券质押券出入库（对应122***)
					break;
				case '5':
					type.u.SHZYQ_KFLJY_KZHGSZQ = code[3] < '7';						//1050**   分离交易的可转换公司债券质押券出入库（对应126***)
																					//1051**   分离交易的可转换公司债券质押券出入库（对应126***)
																					//1052**   分离交易的可转换公司债券质押券出入库（对应126***)
																					//1053**   分离交易的可转换公司债券质押券出入库（对应126***)
																					//1054**   分离交易的可转换公司债券质押券出入库（对应126***)
																					//1055**   分离交易的可转换公司债券质押券出入库（对应126***)
																					//1056**   分离交易的可转换公司债券质押券出入库（对应126***)
					type.u.SHZYQ_ZQJYXKFJJ = code[3] == '7';						//1057**   债券交易型开放式指数基金质押券出入库
					type.u.SHZYQ_KZHGSZQ = code[3] == '8';							//1058**   可转换公司债券质押券出入库（对应110***、113***）
					type.u.SHZYQ_QYZQ = code[3] == '9';								//1059**   企业债券质押券出入库（对应120***、129***）
					break;
				case '6':
					type.u.SHZYQ_DFZQ = 1;											//106***   地方政府债券质押券出入库（对应130***)
					break;
				case '7':
					type.u.SHZYQ_TXGZ = 1;											//107***   记账式贴现国债质押券出入库（对应020***)
					break;
				case '8':
					type.u.SHZYQ_ZCXYHJRZQ = 1;										//108***   政策性银行金融债质押券出入库(对应018***)
					break;
				}
				break;
			case '1':
				type.u.SHBOND_KZHGSZQ_GKFX = code[2] == '0' && code[3] < '8';		//1100**   上市公司公开发行可转换公司债券（对应600***）
																					//1101**   上市公司公开发行可转换公司债券（对应600***）
																					//1102**   上市公司公开发行可转换公司债券（对应600***）
																					//1103**   上市公司公开发行可转换公司债券（对应600***）
																					//1104**   上市公司公开发行可转换公司债券（对应600***）
																					//1105**   上市公司公开发行可转换公司债券（对应600***）
																					//1106**   上市公司公开发行可转换公司债券（对应600***）
																					//1107**   上市公司公开发行可转换公司债券（对应600***）
				type.u.SHBOND_KZHGSZQ_FGKFX = code[2] == '0' && code[3] > '7';		//1108**   上市公司非公开发行可转换公司债券
																					//1109**   上市公司非公开发行可转换公司债券
				type.u.SHBOND_KZHGSZQ = code[2] == '0' || code[2] == '1' || code[2] == '2' || code[2] == '3' || code[2] == '8';			//110***   可转债(对应600***)
																					//113***   可转债(对应601*** 603***)
				break;
			case '2':
				type.u.SHBOND_QYZQ = code[2] == '0' ||								//120***   企业债券
					(code[2] == '2' && code[3] > '5') ||							//1225**   企业债券
																					//1226**   企业债券
																					//1227**   企业债券
																					//1228**   企业债券
																					//1229**   企业债券
					code[2] == '4' ||												//124***   企业债券
					code[2] == '7' ||												//127***   企业债券 
					code[2] == '9';													//129***   企业债券
				type.u.SHBOND_ZCZQ = code[2] == '1' ||								//121***   资产支持证券
					(code[2] == '3' && code[3] > '4');								//1235**   资产支持证券
																					//1236**   资产支持证券
																					//1237**   资产支持证券
																					//1238**   资产支持证券
																					//1239**   资产支持证券
				type.u.SHBOND_GSZQ = (code[2] == '2' && code[3] < '5') ||			//1220**   公司债券
																					//1221**   公司债券
																					//1222**   公司债券
																					//1223**   公司债券
																					//1224**   公司债券
					(code[2] == '3' && code[3] < '5');								//1230**   企业债券、公司债券
																					//1231**   企业债券、公司债券
																					//1232**   企业债券、公司债券
																					//1233**   企业债券、公司债券
																					//1234**   企业债券、公司债券
				type.u.SHBOND_ZXQYSMZQ = code[2] == '5';							//125***   中小企业私募债券转让
				type.u.SHBOND_KZHGSZQ_FLJY = code[2] == '6';						//126***   分离交易的可转换公司债券
				type.u.SHBOND_XDZCZQ = code[2] == '8';								//128***   信贷资产支持证券交易
				break;
			case '3':
				type.u.SHBOND_DFZQ = code[2] == '0';								//130***   地方政府债
				type.u.SHBOND_ZCZQ = code[2] == '1';								//131***   资产支持证券挂牌转让
				type.u.SHBOND_KJHGSZQ = code[2] == '2';								//132***   可交换公司债券交易
				type.u.SHZYQ_KJHGSZQ = code[2] == '3';								//133***   可交换公司债券质押券出入库
				type.u.SHZYQ_GKFXGSZQ = code[2] == '4';								//134***   公开发行公司债券质押券出入库
				type.u.SHBOND_ZQGSDQZQ = code[2] == '5' && code[3] < '5';			//1350**   证券公司短期债券
																					//1351**   证券公司短期债券
																					//1352**   证券公司短期债券
																					//1353**   证券公司短期债券
																					//1354**   证券公司短期债券
				type.u.SHBOND_BGCZSMZQ = code[2] == '5' && code[3] > '4';			//1355**   并购重组私募债券
																					//1356**   并购重组私募债券
																					//1357**   并购重组私募债券
																					//1358**   并购重组私募债券
																					//1359**   并购重组私募债券
				type.u.SHBOND_GKFXGSZQ = code[2] == '6';							//136***   公开发行公司债券交易
				type.u.SHBOND_KJHGSZQ_FGKFX = code[2] == '7' && code[3] < '5';		//1370**   非公开发行可交换公司债券
																					//1371**   非公开发行可交换公司债券
																					//1372**   非公开发行可交换公司债券
																					//1373**   非公开发行可交换公司债券
																					//1374**   非公开发行可交换公司债券
				type.u.SHZG_FGKFX_KJHGSZQ = code[2] == '8' && code[3] < '5';		//1380**   非公开发行可交换公司债券换股
																					//1381**   非公开发行可交换公司债券换股
																					//1382**   非公开发行可交换公司债券换股
																					//1383**   非公开发行可交换公司债券换股
																					//1384**   非公开发行可交换公司债券换股
				type.u.SHBOND_QYZQ = code[2] == '9';								//139***   企业债券挂牌转让
				break;
			case '4':
				type.u.SHBOND_DFZQ = code[2] == '0' || code[2] == '7';				//140***   地方政府债券
																					//147***   地方政府债券
				type.u.SHZYQ_DFZQ = code[2] == '1' || code[2] == '8';				//141***   地方政府债券质押券出入库
																					//148***   地方政府债券质押券出入库
				type.u.SHBOND_ZCZQ = code[2] == '2' ||								//142***   资产支持证券
					code[2] == '6' ||												//146***   资产支持证券                                                            
					code[2] == '9';													//149***   资产支持证券
				type.u.SHBOND_GKFXGSZQ = code[2] == '3';							//143***   公开发行公司债券
				type.u.SHZYQ_GKFXGSZQ = code[2] == '4';								//144***   公开发行公司债券质押券出入库
				type.u.SHBOND_FGKFXGSZQ = code[2] == '5';							//145***   非公开发行可转换公司债券
				break;
			case '5':
				type.u.SHBOND_FGKFXGSZQ = code[2] == '0' || code[2] == '1';			//150***   非公开发行公司债券挂牌转让
																					//151***   非公开发行公司债券挂牌转让
				type.u.SHBOND_QYZQ = code[2] == '2';								//152***   企业债券
				type.u.SHZYQ_QYZQ = code[2] == '3';									//153***   企业债券质押券出入库
				type.u.SHZYQ_GSZQ = code[2] == '4';									//154***   公司债券质押券出入库
				type.u.SHBOND_GSZQ = code[2] == '5';								//155***   公司债券
				type.u.SHBOND_ZCZQ = code[2] == '6' || code[2] == '9';				//156***   资产支持证券业务
																					//159***   资产支持证券业务
				type.u.SHBOND_DFZQ = code[2] == '7';								//157***   地方政府债券
				type.u.SHZYQ_DFZQ = code[2] == '8';									//158***   地方政府债券质押券出入库
				break;
			case '6':
				type.u.SHBOND_DFZQ = code[2] == '0';								//160***   地方政府债券
				type.u.SHZYQ_DFZQ = code[2] == '1';									//161***   地方政府债券质押券出入库
				type.u.SHBOND_FGKFXGSZQ = code[2] == '2'							//162***   非公开发行公司债券
					|| code[2] == '6'												//166***   非公开发行公司债券
					|| code[2] == '7';												//167***   非公开发行公司债券
				type.u.SHBOND_GKFXGSZQ = code[2] == '3';							//163***   公开发行公司债券
				type.u.SHZYQ_GKFXGSZQ = code[2] == '4';								//164***   公开发行公司债券质押券出入库
				type.u.SHBOND_ZCZQ = code[2] == '5';								//165***   资产支持证券
				break;
			case '8':
				type.u.SHZG_KZHGSZQ = code[2] == '1';								//181***   可转换公司债券转股(对应600***)，已不再增用
				type.u.SHHG_BOND = code[2] == '2' && code[3] < '2';					//1820**   债券回购
																					//1821**   债券回购
				break;
			case '9':
				type.u.SHZG_KZHGSZQ = code[2] == '0' || code[2] == '1';				//190***   可转换公司债券转股(对应600***)
																					//191***   可转换公司债券转股(对应601***)
				type.u.SHZG_KJHGSZQ = code[2] == '2';								//192***   可交换公司债券转股(对应132***)
				type.u.SHZG_CXCYFGKFXKZHGSZQ = code[2] == '3' && code[3] == '0';	//1930**   创新创业公司非公开发行可转换公司债券转股
				break;
			}
			break;
		case '2':
			if (code[1] != '0') break;
			type.u.SHHG_GZ_XWTG = code[2] == '1';									//201***   国债回购(席位托管方式)
			type.u.SHHG_QYZQ = code[2] == '2';										//202***   企业债券回购
			type.u.SHHG_GZMD = code[2] == '3';										//203***   国债买断式回购
			type.u.SHHG_ZQZYSHG_ZHTG = code[2] == '4';								//204***   债券质押式回购(账户托管方式)
			type.u.SHHG_ZQZYSBJHG = code[2] == '5';									//205***   债券质押式报价回购
			type.u.SHHG_ZQZYSXYHG = code[2] == '6';									//206***   债券质押式协议回购交易
			type.u.SHHG_ZQZYSSFHG = code[2] == '7';									//207***   债券质押式三方回购
			break;
		case '3':
			type.u.SHQH_GZ = code[1] == '1' && code[2] == '0';						//310***   国债期货(暂停交易)
			type.u.SHYXG_GKFX = code[1] == '3' && code[2] == '0';					//330***   公开发行优先股
			type.u.SHYXG_FGKFX = code[1] == '6' && code[2] == '0';					//360***   非公开发行优先股
			break;
		case '5':
			switch (code[1])
			{
			case '0':
				type.u.SHJJ_FBQY = code[2] == '0' || code[2] == '8';				//500***   契约型封闭式基金
				type.u.SHJJ_LOF = code[2] == '1' || code[2] == '2';					//501***   上市开放式基金
				type.u.SHJJ_LOFFJ = code[2] == '2';									//502***   上市开放式基金(上证LOF财务分级基金交易)
				type.u.SHREITS = code[2] == '8';
				type.u.SHJJ_ETF = code[2] == '6';
				break;
			case '1':
				type.u.SHJJ_ETF = code[2] == '0' ||									//510***   交易型开放式指数证券投资基金(标的指数为沪市指数、跨市场指数或跨境指数)
					code[2] == '2' ||												//512***   交易型开放式指数证券投资基金(标的指数为跨市场指数)
					code[2] == '3' ||												//513***   交易型开放式指数证券投资基金(标的指数为跨境指数)
					code[2] == '5' ||
					code[2] == '6' ||
					code[2] == '7';													//515***   交易型开放式指数证券投资基金(标的指数为跨市场指数)
				type.u.SHJJ_HBETF = code[2] == '1' && code[3] != '3';				//511***   交易型货币市场基金
				type.u.SHJJ_ZQETF = code[2] == '1' && code[3] == '3';				//5113**   债券交易型开放式指数基金
				type.u.SHJJ_SPETF = code[2] == '8';									//518***   商品交易型开放式证券投资基金
				type.u.SHPUB_KFSJJSGSH = code[2] == '9';							//519***   开放式基金申赎(其中5198**用于实时申赎货币基金)
				break;
			case '2':
				type.u.SHPUB_KFSJJRG = code[2] == '1';								//521***   开放式基金认购
				type.u.SHPUB_KFSJJTG = code[2] == '2';								//522***   开放式基金跨市场转托管
				type.u.SHPUB_KFSJJFH = code[2] == '3';								//523***   开放式基金分红
				type.u.SHPUB_KFSJJZH = code[2] == '4';								//524***   开放式基金基金转换
				break;
			case '5':
				type.u.SHJJ = 1;													//550***   基金
				break;
			case '6':
			{
				type.u.SHJJ_ETF = code[2] == '0' ||
					code[2] == '1' ||
					code[2] == '2' ||
					code[2] == '3';
			}
			break;
			case '8':
				type.u.SHQZ = code[2] == '0';										//580***   权证(含股改权证、公司权证)
				type.u.SHQZXQ = code[2] == '2';										//582***   权证行权
				type.u.SHJJ_ETF = code[2] == '8';
				break;
			}
			break;
		case '6':
			switch (code[1])
			{
			case '0':
				type.u.SHAG = code[2] == '0' || code[2] == '1' || code[2] == '3' || code[2] == '5';	//600***   A股
																									//601***   A股
																									//603***   A股
																									//605***   A股
				break;
			case '8':
				type.u.SHAG = type.u.SHKCB = (code[2] == '8' || code[2] == '9');					//688***  689***  科创板
				break;
			}
			break;
		case '7':
			switch (code[1])
			{
			case '0':
				type.u.SHPUB_PG = code[2] == '0';									//700***   配股(对应600***)
				type.u.SHPUB_ZPG = code[2] == '1';									//701***   转配股
				type.u.SHPUB_ZGGPG = code[2] == '2';								//702***   职工股配股(对应600***)
				type.u.SHPUB_PS = code[2] == '3';									//703***   配售
				type.u.SHPUB_CGPZQ = code[2] == '4';								//704***   持股配债(对应600***)
				type.u.SHPUB_JJKM = code[2] == '5';									//705***   基金扩募
				type.u.SHYYSG = code[2] == '6';										//706***   要约收购(其中706600-706999用于科创板公司要约收购、现金选择权)
				type.u.SHPUB_SZPSPG = code[2] == '9';								//709***   按市值配售配股
				break;
			case '3':
				type.u.SHPUB_SGZF = code[2] == '0' || code[2] == '2';				//730***   网上按市值申购或增发(对应600***)
																					//732***   网上按市值申购或增发(对应603***)
				type.u.SHPUB_CGZF = code[2] == '1';									//731***   持股增发(对应600***)
				type.u.SHPUB_KZHGSZQSG = code[2] == '3';							//733***   可转换公司债券申购(对应600***)
				type.u.SHPUB_SGKZFK = code[2] == '4';								//734***   申购款或增发款(对应603***)
				type.u.SHPUB_JJSG = code[2] == '5';									//735***   基金申购
				type.u.SHPUB_SGZFPH = code[2] == '6';								//736***   网上按市值申购或增发配号(对应603***)
				type.u.SHPUB_SZPS = code[2] == '7';									//737***   按市值配售
				type.u.SHWLTP_AG = code[2] == '8';									//738***   网上投票
				type.u.SHPUB_SZPSSG = code[2] == '9';								//739***   按市值配售申购
				break;
			case '4':
				type.u.SHPUB_SGKZFK = code[2] == '0';								//740***   申购款或增发款(对应600***)
				type.u.SHPUB_SGZFPH = code[2] == '1';								//741***   网上按市值申购或增发配号(对应600***)
				type.u.SHPUB_PG = code[2] == '2';									//742***   配股(对应603***)
				type.u.SHPUB_KZHGSZQSGK = code[2] == '3';							//743***   可转换公司债券申购款(对应600***)
				type.u.SHPUB_KZHGSZQPH = code[2] == '4';							//744***   可转换公司债券配号(对应600***)
				type.u.SHPUB_JJSGK = code[2] == '5';								//745***   基金申购款
				type.u.SHPUB_JJSGPH = code[2] == '6';								//746***   基金申购配号
				type.u.SHPUB_SZPS = code[2] == '7' || code[2] == '8';				//747***   按市值配售
																					//748***   按市值配售
				type.u.SHPUB_SZPSPH = code[2] == '9';								//749***   按市值配售配号
				break;
			case '5':
				switch (code[2])
				{
				case '0':
					type.u.SHPUB_GZCX = 1;											//750***   国债承销发行
					break;
				case '1':
					switch (code[3])
					{
					case '0':														//7510**   国债分销
					case '1':														//7511**   国债分销
						type.u.SHPUB_GZ = 1;
						break;
					case '2':														//7512**   政策性银行金融债券分销
					case '3':														//7513**   政策性银行金融债券分销
						type.u.SHPUB_ZCXYHJRZQ = 1;
						break;
					case '4':														//7514**   地方政府债券网上分销
					case '5':														//7515**   地方政府债券网上分销
						type.u.SHPUB_DFZQ = 1;
						break;
					case '8':
						type.u.SHPUB_GZLVZB = code[4] == '0';						//75180*   利率招标国债预发行交易
						type.u.SHPUB_JJSG = code[4] == '1';							//75181*   价格招标国债预发行交易
						type.u.SHPUB_GKFXGSZQ = code[4] > '4';						//75185*   面向合格投资者公开发行公司债券网上分销
																					//75186*   面向合格投资者公开发行公司债券网上分销
																					//75187*   面向合格投资者公开发行公司债券网上分销
																					//75188*   面向合格投资者公开发行公司债券网上分销
																					//75189*   面向合格投资者公开发行公司债券网上分销
						break;
					case '9':
						type.u.SHPUB_DFZQ = code[4] < '7';							//75190*   地方政府债券网上分销
																					//75191*   地方政府债券网上分销
																					//75192*   地方政府债券网上分销
																					//75193*   地方政府债券网上分销
																					//75194*   地方政府债券网上分销
																					//75195*   地方政府债券网上分销
																					//75196*   地方政府债券网上分销
						type.u.SHPUB_GSZQ = code[4] > '6';							//75197*   公司债券及企业债分销
																					//75198*   公司债券及企业债分销
																					//75199*   公司债券及企业债分销
						break;
					}
					break;
				case '2':
					type.u.SHWLTP_AG = 1;											//752***   网络投票
					break;
				case '3':
					type.u.SHPUB_CGPZQ = 1;											//753***   持股配债
					break;
				case '4':
					type.u.SHPUB_KZHGSZQSG = 1;										//754***   可转换公司债券申购
					break;
				case '5':
					type.u.SHPUB_KZHGSZQSGK = 1;									//755***   可转换公司债券申购款
					break;
				case '6':
					type.u.SHPUB_KZHGSZQPH = 1;										//756***   可转换公司债券配号
					break;
				case '8':
					type.u.SHPUB_KJHZQPH = code[3] == '0';							//7580**   可交换公司债券网上发行配号
					break;
				case '9':
					type.u.SHPUB_KJHZQSG = code[3] == '0';							//7590**   可交换公司债券网上发行申购
					break;
				}
				break;
			case '6':
				type.u.SHPUB_PG = code[2] == '0';									//760***   配股(对应601***)
				type.u.SHPUB_ZGGPG = code[2] == '2';								//762***   职工股配股(对应601***)
				type.u.SHPUB_CGPZQ = code[2] == '4';								//764***   持股配债(对应601***)
				break;
			case '7':
				type.u.SHPUB_GKFXYXGSG = code[2] == '0';							//770***   公开发行优先股申购
				type.u.SHPUB_GKFXYXGPGPS = code[2] == '1';							//771***   公开发行优先股配股、配售
				type.u.SHPUB_GKFXYXGSGK = code[2] == '2';							//772***   公开发行优先股申购款
				type.u.SHPUB_GKFXYXGSGPH = code[2] == '3';							//773***   公开发行优先股申购配号
				break;
			case '8':
				type.u.SHPUB_SGZF = code[2] == '0';									//780***   网上按市值申购或增发(对应601***)
				type.u.SHPUB_CGZF = code[2] == '1';									//781***   持股增发(对应601***)
				type.u.SHPUB_KZHGSZQSG = code[2] == '3';							//783***   可转换公司债券申购(对应601***)
				type.u.SHPUB_KCBPG = code[2] == '5';								//785***   科创板股票配股
				type.u.SHPUB_KCBSG = code[2] == '7';								//787***   科创板股票网上申购
				type.u.SHWLTP_AG = code[2] == '8';									//788***   网络投票(对应601***)
				type.u.SHPUB_KCBSGPH = code[2] == '9';								//789***   科创板股票网上申购配号
				break;
			case '9':
				type.u.SHPUB_SGKZFK = code[2] == '0';								//790***   申购款或增发款(对应601***)
				type.u.SHPUB_SGZFPH = code[2] == '1';								//791***   网上按市值申购或增发配号(对应601***)
				type.u.SHPUB_KZHGSZQSGK = code[2] == '3';							//793***   可转换公司债券申购款(对应601***)
				type.u.SHPUB_KZHGSZQPH = code[2] == '4';							//794***   可转换公司债券配号(对应601***)
				type.u.SHPUB_KCBCTPZSG = code[2] == '5';							//795***   科创板存托凭证网上申购
				type.u.SHPUB_KCBCTPZSGPH = code[2] == '6';							//796***   科创板存托凭证网上申购配号
				if (code[2] != '9' || code[3] != '9') break;
				type.u.SHZJQDKZ = code[4] == '7' && code[5] == '0';					//799970 资金前端控制自设额度应急调整代码
				type.u.SHRZRQ_YQHZ = code[4] == '8' && code[5] == '1';				//799981 余券划转
				type.u.SHRZRQ_HQHZ = code[4] == '8' && code[5] == '2';				//799982 还券划转
				type.u.SHRZRQ_DBWHZ = code[4] == '8' && code[5] == '3';				//799983 担保物划转
				type.u.SHRZRQ_QYHZ = code[4] == '8' && code[5] == '4';				//799984 券源划转
				type.u.SHWLTP_MMFW = code[4] == '8' && code[5] == '8';				//799988 A股网络投票密码服务
				type.u.SHTZZSFRZ = code[4] == '9' && code[5] == '1';				//799991 通过交易报盘方式为投资者办理中国结算网络服务身份认证
				type.u.SHRZRQ_SBJS = code[4] == '9' && code[5] == '3';				//799993 证券金融公司转融通申报结束提醒
				type.u.SHHGZDCX = code[4] == '9' && code[5] == '6';					//799996 回购指定撤销
				type.u.SHHGZD = code[4] == '9' && code[5] == '7';					//799997 回购指定
				type.u.SHZDJYCX = code[4] == '9' && code[5] == '8';					//799998 撤销指定
				type.u.SHZDJY = code[4] == '9' && code[5] == '9';					//799999 指定交易
				break;
			}
			break;
		case '8':
			type.u.SHBZQ = code[1] == '8' && code[2] == '8';						//888***   标准券(888880代码为新标准券，用于债券回购转换成标准券)
																					//type.u.SHBLOCK = code[1] == '0';										//80****   板块分类指数
			type.u.SHBLOCK = 1;
			type.u.SHZS = code[1] == '9' && code[2] == '9';						    //899***   上海指数
			break;
		case '9':
			type.u.SHBG = code[1] == '0' && code[2] == '0';							//900***   B股
			type.u.SHZS = code[1] == '3';							                //93****   上海指数
			type.u.SHWLTP_BG = code[1] == '3' && code[2] == '8';					//938***   网上投票(B股)
			type.u.SHWLTP_BGMMFW = code[1] == '3' && code[2] == '9';				//939***   B股网络投票密码服务(现仅用939988)
			type.u.SHZS = code[1] == '5';                                           //95****   上海指数
			type.u.SHBGQZ = code[1] == '7' && code[2] == '0';
			type.u.SHZS = code[1] == '9';                                           //99****   上海指数
			break;
		case 'H':
			type.u.SHZS = 1;							                            //H*****   中证指数
			break;
		case 'C':
			type.u.SHZS = 1;							                            //C*****   中证指数
			break;
		}
	}
	else if (setcode == BJ || setcode == GZ)
	{
		type.u.reserved = 1;

		if (code[0] == '4' && code[1] == '0' && code[2] == '0')
		{
			type.u.GZLWTSAG = 1;
		}
		else if (code[0] == '4' && code[1] == '2' && code[2] == '0')
		{
			type.u.GZLWTSBG = 1;
		}
		else if ((code[0] == '4' && code[1] == '3') || (code[0] == '8' && code[1] == '3')
			|| (code[0] == '8' && code[1] == '7'))
		{
			type.u.GZGPSS = 1;
		}
		else if (code[0] == '4' && code[1] == '0' && code[2] == '4')
		{
			type.u.GZTSKZH = 1;
		}
		else if (code[0] == '8' && code[1] == '1')
		{
			type.u.GZKZHZQ = 1;
		}
		else if (code[0] == '8' && code[1] == '2' && code[2] == '0')
		{
			type.u.GZYXG = 1;
		}
		else if (code[0] == '8' && code[1] == '4' && code[2] == '0')
		{
			type.u.GZYYSG = 1;
		}
		else if (code[0] == '8' && code[1] == '4' && code[2] == '1')
		{
			type.u.GZYYGG = 1;
		}
		else if (code[0] == '8' && code[1] == '5' && code[2] == '0')
		{
			type.u.GZGQJLQQ = 1;
		}
		else if (code[0] == '8' && code[1] == '8' && code[2] == '9')
		{
			type.u.GZFXYW = 1;
		}
		else if (code[0] == '8' && code[1] == '9' && code[2] == '9')
		{
			type.u.GZBJZS = 1;
		}
	}
	return type;
}

short getvolunit(short setcode, char* code, int chStockFlag)	//取成交量倍数
{
	short unit;

	unit = 1;
	if (setcode == NYSE || setcode == NASDAQ || setcode == American)
	{
		return unit;
	}

	if (!testzs(setcode, code, chStockFlag))
	{
		StockCodeType type = GetStockType(setcode, code, chStockFlag);
		if (type.u.SZQQ) unit = 1;
		else if (type.IsHsBoud() || type.IsKZZ() || type.IsHg())
			unit = 10;
		else
			unit = 100;
	}
	return unit;
}

__int64 getNowJcltime(void)
{
	CTime tm = CTime::GetCurrentTime();
	__int64 lNow = tm.GetYear() * 10000 + tm.GetMonth() * 100 + tm.GetDay();
	lNow = (lNow * 1000000 + tm.GetHour() * 10000 + tm.GetMinute() * 100 + tm.GetSecond()) * 1000;
	return lNow;
}

bool is_limit_up(double fclose, double close)
{
	return fabs(fclose - close) < 0.01;
}

bool IsHbEtf(short setcode, char* code)
{
	if (6 == strlen(code))
	{
		if (setcode == SZ && code[0] == '1' && code[1] == '5' && code[2] == '9' && code[3] == '0')
		{
			return true;
		}
		else if (setcode == SH && code[0] == '5' && code[1] == '1' && code[2] == '9')
		{
			return true;
		}
		else if (setcode == SH && code[0] == '5' && code[1] == '1' && code[2] == '1')
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

void TrimStr(CString& str, const char* pstr, int len)
{
	str = "";
	str.Append(pstr, len);
	str.Trim();
}

void TrimStr(char* pStr, int len)
{
	for (int i = 0; i < len; ++i)
	{
		if (isspace(pStr[i]))
		{
			pStr[i] = '\0';
			break;
		}
	}
}

void TrimStr(char* pDst, char* pSrc, int len)
{
	for (int i = 0; i < len; ++i)
	{
		if (isspace(pSrc[i]))
		{
			pDst[i] = '\0';
			break;
		}
		else
		{
			pDst[i] = pSrc[i];
		}
	}
}

void TrimStr(std::string& str)
{
	int s = str.find_first_not_of(' ');
	int e = str.find_last_not_of(' ');

	//
	if (s >= 0 && e >= 0 && e >= s)
	{
		str = str.substr(s, e - s + 1);
	}
}

//剔除数字
void RejectDigit(char* sou, char* des)
{
	//海峡特殊处理
	int i = 0;
	int nLen = strlen(sou);
	if (nLen >= 4 && sou[i] == '0' && sou[i + 1] == '3' && sou[i + 2] == '9' && sou[i + 3] == 'P')
	{
		strncpy(des, sou, 4);
	}
	else if (nLen >= 3 && sou[i] == 'Y' && sou[i + 1] == '6' && (sou[i + 2] == '0' || sou[i + 2] == '1'))
	{
		strncpy(des, sou, 3);
	}
	else
	{
		int len = strlen(sou);
		for (int i = 0, j = 0; i < len; i++)
		{
			if (sou[i] >= '0' && sou[i] <= '9')
			{
				continue;
			}
			des[j++] = sou[i];
		}
	}
}

void InitStkTime(short setcode, char* code, short* fz)
{
	char desCode[12] = { 0 };
	RejectDigit(code, desCode);
	strupper(desCode);
	char szBuf[60] = { 0 };
	sprintf_s(szBuf, _countof(szBuf), "%d%s", setcode, desCode);
	const char* pFind = strstr(szBuf, "LX");
	if (pFind != NULL)
		szBuf[pFind - szBuf] = 0;

	if (IsQHMarket(setcode))
	{
		if (strlen(szBuf))
		{
			std::string key = szBuf;
			if (g_CodeTime.find(key) != g_CodeTime.end())
			{
				memcpy(fz, &g_CodeTime[key][0], g_CodeTime[key].size() * sizeof(short));
				return;
			}
		}
	}

	StockCodeType type = GetStockType(setcode, code);

	switch (setcode)
	{
	case SZ:
	{
		fz[0] = sz[0]; fz[1] = sz[1];
		fz[2] = sz[2]; fz[3] = sz[3];
		fz[4] = sz[4]; fz[5] = sz[5];
		fz[6] = sz[6]; fz[7] = sz[7];
		//if (type == CODE_SZGZHG)
		if (type.IsSzHg())
			fz[3] = fz[4] = fz[5] = fz[6] = fz[7] = sz[3] + 30;
		if (type.IsSzBondNoCov())
		{
			// 非可转债品种, 交易时间到15:30
			fz[3] = fz[4] = fz[5] = fz[6] = fz[7] = sz[3] + 30;
		}

		if (type.IsGGT())
		{
			memcpy(fz, hk, 8 * sizeof(short));
		}
	}
	break;
	case SH:
	{
		fz[0] = sh[0]; fz[1] = sh[1];
		fz[2] = sh[2]; fz[3] = sh[3];
		fz[4] = sh[4]; fz[5] = sh[5];
		fz[6] = sh[6]; fz[7] = sh[7];
		//if (type == CODE_SHGZHG)
		if (type.IsShHg())
			fz[3] = fz[4] = fz[5] = fz[6] = fz[7] = sz[3] + 30;

		if (type.IsShBondNoCov())
		{
			// 非可转债品种, 交易时间到15:30
			fz[3] = fz[4] = fz[5] = fz[6] = fz[7] = sz[3] + 30;
		}

		if (type.IsGGT())
		{
			memcpy(fz, hk, 8 * sizeof(short));
		}
	}
	break;
	case HK:
	{
		fz[0] = hk[0]; fz[1] = hk[1];
		fz[2] = hk[2]; fz[3] = hk[3];
		fz[4] = hk[4]; fz[5] = hk[5];
		fz[6] = hk[6]; fz[7] = hk[7];
	}
	break;
	case SF:
	{
		fz[0] = sf[0]; fz[1] = sf[1];
		fz[2] = sf[2]; fz[3] = sf[3];
		fz[4] = sf[4]; fz[5] = sf[5];
		fz[6] = sf[6]; fz[7] = sf[7];
	}
	break;
	case SC:
	{
		fz[0] = sc[0]; fz[1] = sc[1];
		fz[2] = sc[2]; fz[3] = sc[3];
		fz[4] = sc[4]; fz[5] = sc[5];
		fz[6] = sc[6]; fz[7] = sc[7];
	}
	break;
	case ZC:
	{
		fz[0] = zc[0]; fz[1] = zc[1];
		fz[2] = zc[2]; fz[3] = zc[3];
		fz[4] = zc[4]; fz[5] = zc[5];
		fz[6] = zc[6]; fz[7] = zc[7];
	}
	break;
	case DC:
	{
		fz[0] = dc[0]; fz[1] = dc[1];
		fz[2] = dc[2]; fz[3] = dc[3];
		fz[4] = dc[4]; fz[5] = dc[5];
		fz[6] = dc[6]; fz[7] = dc[7];
	}
	break;
	case BH:
	{
		fz[0] = _bh[0]; fz[1] = _bh[1];
		fz[2] = _bh[2]; fz[3] = _bh[3];
		fz[4] = _bh[4]; fz[5] = _bh[5];
		fz[6] = _bh[6]; fz[7] = _bh[7];
	}
	break;
	/*
	case LDJ://TODO 没使用的市场要去掉
	{
		fz[0] = ldj[0]; fz[1] = ldj[1];
		fz[2] = ldj[2]; fz[3] = ldj[3];
		fz[4] = ldj[4]; fz[5] = ldj[5];
		fz[6] = ldj[6]; fz[7] = ldj[7];
	}
	*/
	break;	// ldj
	case BTM:
	{
		memcpy(fz, btm, 8 * sizeof(short));
	}
	break;
	case BT:
	{
		memcpy(fz, bt, 8 * sizeof(short));
	}
	break;
	case NYSE:
	{
		memcpy(fz, nyse, 8 * sizeof(short));
	}
	break;
	case NASDAQ:
	{
		memcpy(fz, nasdaq, 8 * sizeof(short));
	}
	break;
	case American:
	{
		memcpy(fz, american, 8 * sizeof(short));
	}
	break;
	}
}

//返回当前时间的今日开盘的分钟数
int System2DT(short setcode, int m, short* fz)
{
	int  itemnum = 1;
	// 不要增加1，本来外部统计分钟就+1了
	// 集合竞价,落在收盘后10分钟指定时间的（注意16：01分这样推动的问题) :
	// 注意，如果是连续24小时的，不能这样判断
	if (setcode == SZ || setcode == SH)
	{	// 9:15 -- 9:30 之间的行情算在9：30（570）	
		if (isInTimeSpan(m, fz[0] - 30, fz[0]))	//ddd 有时看到第一个数据在8.30左右
			return itemnum;
	}
	else //if(setcode!=LDJ0 && setcode != QL && setcode != HQXG)
	{
		if (isInTimeSpan(m, fz[7], fz[0]) && TimeSpan(fz[7], fz[0]) > 5)
		{
			if (isInTimeSpan(m, TimeAdd(fz[7], 5), fz[0]))
				return itemnum;
		}
	}
	if (m == fz[0])	// 不能从0开始，全部从+1开始
		return itemnum + 1;
	else if (m == fz[1])
		return TimeSpan(m, fz[0]) + 1;
	else if (m == fz[3])
		return (TimeSpan(fz[3], fz[2]) + TimeSpan(fz[1], fz[0])) + 1;
	if (isInTimeSpan(m, fz[0], fz[1]))				itemnum = TimeSpan(m, fz[0]);
	else if (isInTimeSpan(m, fz[1], fz[2]))		itemnum = TimeSpan(fz[1], fz[0]);
	else if (isInTimeSpan(m, fz[2], fz[3]))		itemnum = TimeSpan(m, fz[2]) + TimeSpan(fz[1], fz[0]);
	else if (isInTimeSpan(m, fz[3], fz[4]))		itemnum = TimeSpan(fz[3], fz[2]) + TimeSpan(fz[1], fz[0]);
	else if (isInTimeSpan(m, fz[4], fz[5]))		itemnum = TimeSpan(m, fz[4]) + TimeSpan(fz[3], fz[2]) + TimeSpan(fz[1], fz[0]);
	else if (isInTimeSpan(m, fz[5], fz[6]))		itemnum = TimeSpan(fz[5], fz[4]) + TimeSpan(fz[3], fz[2]) + TimeSpan(fz[1], fz[0]);
	else if (isInTimeSpan(m, fz[6], fz[7]))		itemnum = TimeSpan(m, fz[6]) + TimeSpan(fz[5], fz[4]) + TimeSpan(fz[3], fz[2]) + TimeSpan(fz[1], fz[0]);
	else              itemnum = TimeSpan(fz[7], fz[6]) + TimeSpan(fz[5], fz[4]) + TimeSpan(fz[3], fz[2]) + TimeSpan(fz[1], fz[0]);
	itemnum += 2;	// 外部计算，统一-1了
	return itemnum;
}

// 根据序号得到当前应该的分钟数
//判断时间位于哪个区间，返回相对于今日零时的区间的时间分钟
int  GetMinuteFromFZ(int xh, int fznum, short* fz, short setcode)
{
	if (xh <= TimeSpan(fz[1], fz[0]))
	{
		return fz[0] + xh;
	}

	int newxh = xh - TimeSpan(fz[1], fz[0]);

	if (newxh < TimeSpan(fz[3], fz[2]))
	{
		return fz[2] + newxh;
	}

	return fz[3];


	//int i,allmin=(xh+1)*fznum;	// 比如775分钟，一共780
	////int i,allmin=(xh)*fznum;	// 为什么要+1  
	////i= (fz[0]+xh*fznum) % (24*60);
	//i= (fz[0]+(xh+1)*fznum);
	////i= (fz[0]+(xh)*fznum);
	//if ( allmin - TimeSpan(fz[1],fz[0]) <= 0 )	// 如果在第一个时段里面  要求11：30靠前，不用分段后
	//{
	//	i= (fz[0]+allmin) % (24*60);		// 实际时间
	//	return i;			
	//}
	//else 
	//{	// short sc[8]={540,615,630,690,810,900,900,900};
	//	allmin -= TimeSpan(fz[1],fz[0]);		// 去掉第一段
	//	if ( allmin - TimeSpan(fz[3],fz[2]) <= 0 )	// 在第二段里面
	//	{
	//		i = (fz[2]+allmin) % (24*60);
	//	}
	//	else
	//	{
	//		allmin -= TimeSpan(fz[3],fz[2]);		// 去掉第2段
	//		if ( allmin - TimeSpan(fz[5],fz[4]) <= 0 )	// 在第3段里面
	//			//if ( allmin - TimeSpan(fz[5],fz[4]) < 0 )	// 在第3段里面  因为 isInTimeSpan 是前闭后开区间
	//		{
	//			i = (fz[4]+allmin) % (24*60);
	//		}
	//		else
	//			//i = fz[7];//原始的fz[6]和fz[7]没有使用，开市时间没有用到第4个阶段
	//		{
	//			allmin -= TimeSpan(fz[5],fz[4]);
	//			if (allmin - TimeSpan(fz[7],fz[6]) <= 0)	// 在第4段里面
	//			{
	//				i = (fz[6]+allmin)%(24*60);
	//			}
	//			else
	//			{
	//				//if(setcode == SZ || setcode == SH)
	//				i =  fz[7];
	//				//else
	//				//	i = -1;
	//			}
	//		}
	//	}
	//}
	//return i;
}

long GetFriday(long date) // 得到某日的星期五
{
	short  i, year, mon, day;
	long days;
	year = date / 10000;
	mon = (date % 10000) / 100;
	day = (date % 10000) % 100;
	days = 0;
	for (i = 1980; i < year; i++) {
		days += 365;
		if (i % 400 == 0 || (i % 4 == 0 && i % 100 != 0)) // 闰年
			days++;
	}
	for (i = 1; i < mon; i++) {
		days += days_of_mon[i - 1];
		if (i == 2 && (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)))
			days++;
	}
	days += day - 1; // 从1980年至当日共有多少天
	days -= 3; // 1980年1月1日为星期二，则离星期五的天数还要减去3
	i = days % 7;
	if (i > 2) // 得到当日所在星期的星期五。若为星期五、六、日，则为当日
		day += (7 - i) % 7;
	i = days_of_mon[mon - 1];
	if (mon == 2 && (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)))
		i++;
	if (day > i) {
		day -= i;
		mon++;
	}
	if (mon > 12) {
		mon = 1;
		year++;
	}
	return (long)year * 10000L + (long)mon * 100L + (long)day;
}

short need_justcjl(short setcode, char* code, int chStockFlag)
{
	short flag;
	flag = 1;
	if (!testzs(setcode, code, chStockFlag))
	{
		StockCodeType type = GetStockType(setcode, code);
		if (type.IsSzBoud() || type.IsSzKZZ() || type.IsSzHg() || type.IsShKZZ())
			flag = 1;
		else if (type.IsShBoud() || type.IsShHg())
			flag = 0;
		//switch(GetStockType(setcode,code))
		//{
		//case CODE_SZGZ:
		//case CODE_SZZQ:
		//case CODE_SZKZHZQ:
		//case CODE_SZGZHG:
		//	flag = 1;// 0;  20120720
		//	break;
		//case CODE_SHGZ:
		//case CODE_SHZQ:
		//case CODE_SHKZHZQ:
		//case CODE_SHGZHG:
		//	flag = 0;
		//	break;
		//}
	}
	return flag;
}

void FormatPath(char* Path, int MaxLen)
{
	int i, len;

	len = strlen(Path);
#ifdef LINUX
	for (i = 0; i < len; i++)
		if ('\\' == Path[i])
			Path[i] = '/';
#endif
	if (len <= 0 || len > MaxLen - 1) return;
#ifdef LINUX
	if (Path[len - 1] != '/')
	{
		Path[len] = '/';
		Path[len + 1] = 0;
	}
#else
	if (Path[len - 1] != '\\')
	{
		Path[len] = '\\';
		Path[len + 1] = 0;
	}
#endif
}

namespace Market_A
{
	BOOL	isCanOpenDTBig(short iOpen, short iClose)
	{
		CTime Now = CTime::GetCurrentTime();
		int	h = Now.GetHour(), m = Now.GetMinute();
		if (h * 60 + m <= iOpen || (h * 60 + m >= iClose))
			return TRUE;
		else
			return FALSE;
	}

	BOOL	isCanOpenDT()
	{
		CTime Now = CTime::GetCurrentTime();
		int	h = Now.GetHour(), m = Now.GetMinute();
		// if (  (h*60+m<=3*60+10 || h*60+m>=8*60+50) &&!m_bRunning)
		if (h * 60 + m <= 9 * 60 + 10 || h * 60 + m >= 15 * 60 + 10)
			return TRUE;
		else
			return FALSE;
	}
	BOOL	isCanPHDT()
	{
		CTime Now = CTime::GetCurrentTime();
		int	h = Now.GetHour(), m = Now.GetMinute();
		if (h * 60 + m >= 15 * 60 + 10 && h * 60 + m <= 23 * 60 + 30)
			return TRUE;
		else
			return FALSE;
	}
	BOOL isToOpenTime(short nInitTime)
	{
		CTime Now = CTime::GetCurrentTime();
		int	h = Now.GetHour(), m = Now.GetMinute();
		if (h * 60 + m >= nInitTime)
			return TRUE;
		else
			return FALSE;
	}
	BOOL isInWeekend()
	{
		CTime tNow = CTime::GetCurrentTime();
		if (tNow.GetDayOfWeek() == 7)	  //礼拜6	
		{
			return TRUE;
		}
		else if (tNow.GetDayOfWeek() == 1)//礼拜7 
		{
			return TRUE;
		}
		else
			return FALSE;
	}
	BOOL isInWeekend(int date)
	{
		CTime t(date / 10000, date % 10000 / 100, date % 100, 0, 0, 0);
		int idx = t.GetDayOfWeek();
		return idx == 7 || idx == 1;
	}
}

long GetNextDate(long date, int days)
{
	CTime back(date / 10000, date % 10000 / 100, date % 100, 0, 0, 0);
	back += CTimeSpan(days, 0, 0, 0);
	return back.GetYear() * 10000 + back.GetMonth() * 100 + back.GetDay();
}

int GetNextYear(int p_iCurYear, int p_iOffset)
{
	CTime back(p_iCurYear, 1, 1, 0, 0, 0);
	back += CTimeSpan(p_iOffset, 0, 0, 0);
	return back.GetYear();
}

unsigned long tranf2long(float price, char precise)
{
	unsigned long itmp = 0;
	switch (precise)
	{
		//case 0:itmp = (DWORD)(price);break;
		//case 1:itmp = (DWORD)(0.5+price*10.0);break;
		//case 2:itmp = (DWORD)(0.5+price*100.0);break;
		//case 3:itmp = (DWORD)(0.5+price*1000.0);break;
		//case 4:itmp = (DWORD)(0.5+price*10000.0);break;
	case 0:itmp = double2int(price); break;
	case 1:itmp = double2int(price * 10.0); break;
	case 2:itmp = double2int(price * 100.0); break;
	case 3:itmp = double2int(price * 1000.0); break;
	case 4:itmp = double2int(price * 10000.0); break;
	case 6:itmp = double2int(price * 1000000.0); break;
	}
	return itmp;
}

bool IsDateValid(unsigned int uiDate)
{
	if (uiDate < HISTORY_VALID_DATE)
		return false;
	int year = uiDate / 10000;
	int mon = (uiDate / 100) % 100;
	int day = uiDate % 100;

	if (year > 9999)
		return false;

	if (mon == 0 || mon > 12)
		return false;

	if (day == 0 || day > 31)
		return false;

	return true;
}

__int64 GetCurDate(int p_iType)
{
	/*
	time_t now_t = time(NULL);
	tm   now_time;
	localtime_s(&now_time, &now_t);
	if (bDate)
	return (now_time.tm_year + 1900) * 10000 + (now_time.tm_mon + 1) * 100 + now_time.tm_mday;
	else
	return now_time.tm_hour * 10000 + now_time.tm_min * 100 + now_time.tm_sec;
	*/
	SYSTEMTIME now_time;
	GetLocalTime(&now_time);
	__int64 llCurTime = 0;
	if (1 == p_iType)
		llCurTime = static_cast<__int64>(now_time.wYear) * 10000 + (now_time.wMonth) * 100 + now_time.wDay;
	else if (2 == p_iType)
		llCurTime = (__int64)now_time.wHour * 10000 + now_time.wMinute * 100 + now_time.wSecond + now_time.wMilliseconds;
	else
	{
		llCurTime = static_cast<__int64>(now_time.wYear * 10000 + (now_time.wMonth) * 100 + now_time.wDay) * 1000000000 +
			static_cast<__int64>(now_time.wHour * 10000 + now_time.wMinute * 100 + now_time.wSecond) * 1000 + now_time.wMilliseconds;
	}
	return llCurTime;
}

bool isLeap(int year) {	//判断是否是闰年 
	//false返回0，true返回1 
	return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int GetIntervalDate(const int& p_iBeforeDate, const int& p_iAfterDate) 
{
	if (p_iBeforeDate >= p_iAfterDate)
		return 0;

	static const int s_iMonth[12][2] = { { 31, 31 },{ 28, 29 },{ 31, 31 },{ 30, 30 },{ 31, 31 },
	{ 30, 30 },{ 31, 31 },{ 31, 31 },{ 30, 30 },{ 31, 31 },{ 30, 30 },{ 31, 31 } };

	int iBeforeYear = p_iBeforeDate / 10000;
	int iBeforeMon = p_iBeforeDate / 100 % 100;
	int iBeforeDay = p_iBeforeDate % 100;
	int iAfterYear = p_iAfterDate / 10000;
	int iAfterMon = p_iAfterDate / 100 % 100;
	int iAfterDay = p_iAfterDate % 100;

	int ans = 0;

	//第一个日期没有达到第二个日期时进行循环
	while (iBeforeYear < iAfterYear || iBeforeMon < iAfterMon || iBeforeDay < iAfterDay) {
		++iBeforeDay;	//天数加 1 

		if (iBeforeDay == s_iMonth[iBeforeMon - 1][isLeap(iBeforeYear)] + 1) {	//满当月天数，注意判断闰年 
			++iBeforeMon;	//增加月份 
			iBeforeDay = 1; //日期变为下个月的1号
		}
		if (iBeforeMon == 13) {
			++iBeforeYear;
			iBeforeMon = 1;
		}
		ans++;		//累计 
	}
	return ans;
}

int GetNextDate2(const int& p_iDate, const int& p_idays)
{
	if (0 == p_idays) return p_iDate;

	static const int s_iYear[2] = { 365, 366 };

	static const int s_iMonth[12][2] = { { 31, 31 },{ 28, 29 },{ 31, 31 },{ 30, 30 },{ 31, 31 },
	{ 30, 30 },{ 31, 31 },{ 31, 31 },{ 30, 30 },{ 31, 31 },{ 30, 30 },{ 31, 31 } };

	bool bSign = p_idays > 0 ? true : false;
	int iSign = p_idays > 0 ? 1 : -1;
	int iNumDay = bSign ? p_idays : -p_idays;

	int iYear = p_iDate / 10000;
	int iMon = p_iDate / 100 % 100;
	int iDay = p_iDate % 100;

	for (int i = p_idays; i != 0; i -= iSign)
	{
		iDay += iSign;
		if (iDay > s_iMonth[iMon - 1][isLeap(iYear)])
		{
			iDay = 1;
			if (++iMon > 12)
			{
				iMon = 1;
				++iYear;
			}
		}
		else if (iDay <= 0)
		{
			if (--iMon <= 0)
			{
				--iYear;
				iMon = 12;
			}
			iDay = 31;
		}
	}
	return iYear * 10000 + iMon * 100 + iDay;
}

//将string转换成wstring  
std::wstring string2wstring(std::string str)
{
	/*
	std::wstring result;
	//获取缓冲区大小，并申请空间，缓冲区大小按字符计算  
	int len = MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), NULL, 0);
	TCHAR* buffer = new TCHAR[len + 1];
	//多字节编码转换成宽字节编码  
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), str.size(), (LPWSTR)buffer, len);
	buffer[len] = '\0'; //添加字符串结尾  
	//删除缓冲区并返回值  
	result.append(buffer);
	delete[] buffer;
	return result;
	*/
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	return converter.from_bytes(str);
}

//将wstring转换成string  
std::string wstring2string(std::wstring wstr)
{
	std::string result;
	//获取缓冲区大小，并申请空间，缓冲区大小事按字节计算的  
	int len = WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), NULL, 0, NULL, NULL);
	char* buffer = new char[len + 1];
	//宽字节编码转换成多字节编码  
	WideCharToMultiByte(CP_ACP, 0, wstr.c_str(), wstr.size(), buffer, len, NULL, NULL);
	buffer[len] = '\0';
	//删除缓冲区并返回值  
	result.append(buffer);
	delete[] buffer;
	return result;
}

std::wstring char_to_wchar(const char* ch)
{
	wchar_t* wchar;
	int len = MultiByteToWideChar(CP_ACP, 0, ch, strlen(ch), NULL, 0);
	wchar = new wchar_t[len + 1];
	MultiByteToWideChar(CP_ACP, 0, ch, strlen(ch), wchar, len);
	wchar[len] = '\0';
	std::wstring w_str = wchar;
	delete[]wchar;
	return w_str;
}

std::wstring TrimStr(const std::wstring &p_wsInput, const std::wstring p_wsSymbol)
{
	std::wstring wsTrimmed;
	if (p_wsSymbol.empty() || p_wsSymbol.size() > 1)
	{
		return wsTrimmed;
	}

	wchar_t wchar = p_wsSymbol[0];
	// 移除头尾符号
	wsTrimmed.assign(p_wsInput.begin(), p_wsInput.end());
	wsTrimmed.erase(0, wsTrimmed.find_first_not_of(wchar));
	wsTrimmed.erase(wsTrimmed.find_last_not_of(wchar) + 1);
	// 移除中间符号
	wsTrimmed.erase(std::remove(wsTrimmed.begin(), wsTrimmed.end(), wchar), wsTrimmed.end());
	return wsTrimmed;
}

void strTolower(std::string &p_strInput)
{
	std::transform(p_strInput.begin(), p_strInput.end(), p_strInput.begin(), tolower);
}

std::vector<std::string> GetAdapterInfo() {
	vector<std::string> vecIpList;	// ip列表
	IP_ADAPTER_INFO *pAdpFree = NULL;
	IP_ADAPTER_INFO *pIpAdpInfo = (IP_ADAPTER_INFO*)malloc(sizeof(IP_ADAPTER_INFO));
	unsigned long ulBufLen = sizeof(IP_ADAPTER_INFO);
	int iRet = 0;
	//第一次调用获取需要开辟的内存空间大小
	if ((iRet = GetAdaptersInfo(pIpAdpInfo, &ulBufLen)) == ERROR_BUFFER_OVERFLOW) {
		free(pIpAdpInfo);
		//分配实际所需要的内存空间
		pIpAdpInfo = (IP_ADAPTER_INFO*)malloc(ulBufLen);
		if (NULL == pIpAdpInfo)
		{
			return vecIpList;
		}
	}

	char szLocalIp[512] = { 0 };
	if ((iRet = GetAdaptersInfo(pIpAdpInfo, &ulBufLen)) == NO_ERROR)
	{
		pAdpFree = pIpAdpInfo;

		for (int i = 0; pIpAdpInfo; i++)
		{
			std::string strAddrInfo;
			_snprintf(szLocalIp, sizeof(szLocalIp), "netcard=%d,", i);
			strAddrInfo += szLocalIp;
			IP_ADDR_STRING *pIps = &pIpAdpInfo->IpAddressList;
			while (pIps)
			{
				_snprintf(szLocalIp, sizeof(szLocalIp), "des=%s,ip=%s,mask=%s,gate=%s",
					pIpAdpInfo->Description, pIps->IpAddress.String,
					pIps->IpMask.String, pIpAdpInfo->GatewayList.IpAddress.String);
				strAddrInfo += szLocalIp;
				pIps = pIps->Next;
			}
			vecIpList.push_back(strAddrInfo);
			pIpAdpInfo = pIpAdpInfo->Next;
		}
	}
	if (pAdpFree)
	{
		free(pAdpFree);
	}

	return vecIpList;
}

bool MarkCodeSplit(std::string str, short *setcode, std::string &code)
{
	if (str.length() < 3)
	{
		return false;
	}
	std::string szTemp = str.substr(0, 2);
	code = str.substr(2);
	if (szTemp == string("SH"))
	{
		*setcode = SH;
	}
	else if (szTemp == string("SZ"))
	{
		*setcode = SZ;
	}
	else if (szTemp == string("GZ"))
	{
		*setcode = GZ;
	}
	else if (szTemp == string("BJ"))
	{
		*setcode = BJ;
	}
	else if (szTemp == string("HK"))
	{
		*setcode = HK;
	}
	else
	{
		return false;
	}
	return true;
}

bool MarkCodeMerge(short p_nSetcode, char *p_szCode, std::string &p_strCode)
{
	if (NULL == p_szCode || strlen(p_szCode) < 3 ||
		p_nSetcode < SZ || p_nSetcode > BJ)
	{
		return false;
	}
	p_strCode.clear();

	if (SZ == p_nSetcode)
	{
		p_strCode.append("SZ");
	}
	else if (SH == p_nSetcode)
	{
		p_strCode.append("SH");
	}
	else if (GZ == p_nSetcode)
	{
		p_strCode.append("GZ");
	}
	else if (BJ == p_nSetcode)
	{
		p_strCode.append("BJ");
	}
	else
	{
		return false;
	}
	p_strCode.append(p_szCode);
	return true;
}

// iNear: 0 完全匹配, 没找到返回-1
// 1: 模糊查找, 没找到时, 返回小于目标的序号
// 2: 模糊查找, 没找到时, 返回大于目标的序号
int Binary_Search(AnalyDataB * AnalyDatap, int iCount, __int64 iDate, int iNear)
{
	if (NULL == AnalyDatap || iCount < 0 || iDate <= 0)
	{
		return -1;
	}

	//
	int l = 0;
	int r = iCount - 1;

	while (l <= r)
	{
		int mid = (l + r) / 2;
		__int64 iDateNow = AnalyDatap[mid].jclTime;

		if (iDateNow == iDate)
		{
			return mid;
		}

		if (iDateNow < iDate)
		{
			l = mid + 1;
		}
		else
		{
			r = mid - 1;
		}
	}

	if (0 == iNear)
	{
		return -1;
	}
	else if (1 == iNear)
	{
		return r;
	}
	else if (2 == iNear)
	{
		return l;
	}

	return -1;
}


double CalcRelevance(const std::vector<double>& p_vecKline1, const std::vector<double>& p_vecKline2)
{
	int iMinSize = min(p_vecKline1.size(), p_vecKline2.size());
	double dSum1 = 0.f, dSum2 = 0.f;//各自累加值，用于计算平均
	for (int i = iMinSize - 1; i >= 0; --i)
	{
		dSum1 += p_vecKline1[i];
		dSum2 += p_vecKline2[i];
	}
	//各自均值
	double dAvg1 = dSum1 / (double)iMinSize;
	double dAvg2 = dSum2 / (double)iMinSize;

	double dCov = 0.0;//协方差
	double dStd1 = 0.f, dStd2 = 0.f;//各自方差，开根号是标准差
	for (int i = iMinSize - 1; i >= 0; --i)
	{
		dCov += (p_vecKline1[i] - dAvg1) * (p_vecKline2[i] - dAvg2);
		dStd1 += pow((p_vecKline1[i] - dAvg1), 2);
		dStd2 += pow((p_vecKline2[i] - dAvg2), 2);
	}
	if (dStd1 * dStd2 < 0.00001) 
	{
		return 0;
	}
	return dCov / sqrt(dStd1 * dStd2);
}

// 深度优先搜索函数，用于找出连通分量
void DFS(const int& p_node, const std::vector<std::vector<bool>>& p_vecAdjacencyMatrix, std::vector<bool>& p_vecVisited, std::vector<int>& vecTmp)
{
	p_vecVisited[p_node] = true;
	vecTmp.push_back(p_node);
	for (int i = 0; i < p_vecAdjacencyMatrix.size(); ++i)
	{
		if (p_vecAdjacencyMatrix[p_node][i] == true && !p_vecVisited[i])
		{
			DFS(i, p_vecAdjacencyMatrix, p_vecVisited, vecTmp);
		}
	}
}
/*
	p_vecAdjacencyMatrix 为邻接矩阵
	{ 0, 1, 0, 0 },
	{ 1, 0, 1, 0 },
	{ 0, 1, 0, 0 },
	{ 0, 0, 0, 0 }
*/
// 找出图的数量
int GetGraphs(std::vector<std::vector<bool>>& p_vecAdjacencyMatrix, std::vector<std::vector<int>>& p_vecGraphsIndex)
{
	int iNodesNum = p_vecAdjacencyMatrix.size();
	std::vector<bool> vecVisited(iNodesNum, false);//搜索过的节点
	int iCount = 0;
	for (int i = 0; i < iNodesNum; ++i)
	{
		if (!vecVisited[i])
		{
			std::vector<int> vecTmp;
			DFS(i, p_vecAdjacencyMatrix, vecVisited, vecTmp);
			iCount++;
			p_vecGraphsIndex.push_back(vecTmp);
		}
	}
	return iCount;
}

