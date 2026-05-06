#if !defined(__xsdk_numeric_h__)
#define __xsdk_numeric_h__

#include "xsdk_define.h"
#include <string.h>

BGN_NAMESPACE_XSDK

class CNumeric
{
public:
  // 功能描述：以CNumeric类对象初始化类对象
  CNumeric(const CNumeric &p_refclNumeric)
  { InitFromNumeric(p_refclNumeric); }
  // 功能描述：以长整数初始化类对象
  // 入参说明：
  //     [in]  p_llNumeric       长整数，所表达的数值为整数部分
  //                            如果p_llNumeric所表达的数值含小数部分，则需要调用InitFromLonglong
  CNumeric(long long p_llNumeric = 0LL)
  { InitFromLonglong(p_llNumeric, 0); }
  // 功能描述：以双精度浮点数初始化类对象
  // 入参说明：
  //     [in]  p_dNumeric        双精度浮点数
  //     [in]  p_siPrecision    类对象有效精度{<= 18}
  //                            1、有效精度小于零，则取C语言snprintf的"%lf"的输出精度，通常为6位小数精度
  //                            2、按有效精度（不大于18）进行四舍五入存储数值
  CNumeric(double p_dNumeric, short p_siPrecision = -1)
  { InitFromDouble(p_dNumeric, p_siPrecision); }
  // 功能描述：以字符串定点数（即：小数字符串）初始化类对象
  // 入参说明：
  //     [in]  p_pszNumeric      字符串定点数
  //     [in]  p_siPrecision    类对象有效精度{<= 18}
  //                            1、有效精度小于零，则按字符串的实际精度（不大于18）进行四舍五入存储数值
  //                            2、按有效精度（不大于18）进行四舍五入存储数值
  CNumeric(const char *p_pszNumeric, short p_siPrecision = -1)
  { InitFromCharPtr(p_pszNumeric, p_siPrecision); }

  ~CNumeric() {}

  // 功能描述：与相应参数的构造函数功能描述相同
  CNumeric & InitFromNumeric(const CNumeric &p_refclNumeric);
  // 功能描述：与相应参数的构造函数功能描述相同
  CNumeric & InitFromDouble(double p_dNumeric, short p_siPrecision = -1);
  // 功能描述：与相应参数的构造函数功能描述相同
  CNumeric & InitFromCharPtr(const char *p_pszNumeric, short p_siPrecision = -1);
  // 功能描述：以含小数位整数初始化类对象，与相应参数的构造函数功能描述不同
  // 入参说明：
  //     [in]  p_llNumeric       长整数，所表达的数值已按指定精度放大，如：长整数1000，精度3，表达数值1.000
  //     [in]  p_siPrecision    类对象有效精度{<= 18}
  //                            1、有效精度小于零，则类对象的有效精度为0，长整数p_llNumeric，表达为个位数值
  //                            2、按有效精度（不大于18）进行四舍五入存储数值
  CNumeric & InitFromLonglong(long long p_llNumeric, short p_siPrecision);
  // 功能描述：将类对象存储的数值按对象有效精度放大以长整数返回
  //           如果类对象表达的数值大于有符号长整数的最大值或最小值（即：9223372036854775807或-9223372036854775808）
  //           则以有符号长整数的最大值或最小值返回
  long long CvtToLonglong(void) const;

  // 功能描述：将p_refclNumeric类对象表达的数值及溢出标志赋予被赋值对象，并于被赋值对象原有的精度进行四舍五入
  CNumeric & operator =(const CNumeric &p_refclNumeric);
  // 功能描述：将p_pszNumeric字符串表达的数值赋予被赋值对象，并于被赋值对象原有的精度进行四舍五入
  CNumeric & operator =(const char *p_pszNumeric);
  // 功能描述：将p_llNumeric数值赋予被赋值对象
  CNumeric & operator =(long long p_llNumeric);

  // 功能描述：重载'-'和'+'一元运算符，不改变类对象的存储数值
  CNumeric operator -(void) { return (CNumeric(0LL) - *this); }
  CNumeric operator +(void) { return *this; }
  // 功能描述：重载前后缀'++'和'--' 运算符
  CNumeric & operator ++ (void) { *this = *this + CNumeric(1LL); return *this; }
  CNumeric   operator ++ (int)  { CNumeric clNumeric(*this); *this = *this + CNumeric(1LL); return clNumeric; }
  CNumeric & operator -- (void) { *this = *this - CNumeric(1LL); return *this; }
  CNumeric   operator -- (int)  { CNumeric clNumeric(*this); *this = *this - CNumeric(1LL); return clNumeric; }
  // 功能描述：重载'+='、'-='、'*='和'/='运算符
  CNumeric & operator += (const CNumeric &p_refclNumeric) { *this = *this + p_refclNumeric; return *this; }
  CNumeric & operator -= (const CNumeric &p_refclNumeric) { *this = *this - p_refclNumeric; return *this; }
  CNumeric & operator *= (const CNumeric &p_refclNumeric) { *this = *this * p_refclNumeric; return *this; }
  CNumeric & operator /= (const CNumeric &p_refclNumeric) { *this = *this / p_refclNumeric; return *this; }

  CNumeric & operator += (long long p_llNumeric) { *this = *this + CNumeric(p_llNumeric); return *this; }
  CNumeric & operator -= (long long p_llNumeric) { *this = *this - CNumeric(p_llNumeric); return *this; }
  CNumeric & operator *= (long long p_llNumeric) { *this = *this * CNumeric(p_llNumeric); return *this; }
  CNumeric & operator /= (long long p_llNumeric) { *this = *this / CNumeric(p_llNumeric); return *this; }

  // 功能描述：加、减、乘、除等四则运算方法
  friend CNumeric operator + (const CNumeric &p_refclNumeric1, const CNumeric &p_refclNumeric2);
  friend CNumeric operator - (const CNumeric &p_refclNumeric1, const CNumeric &p_refclNumeric2);
  friend CNumeric operator * (const CNumeric &p_refclNumeric1, const CNumeric &p_refclNumeric2);
  friend CNumeric operator / (const CNumeric &p_refclNumeric1, const CNumeric &p_refclNumeric2);

  friend CNumeric operator + (const CNumeric &p_refclNumeric1, long long p_llNumeric2)
  {return p_refclNumeric1 + CNumeric(p_llNumeric2); }
  friend CNumeric operator - (const CNumeric &p_refclNumeric1, long long p_llNumeric2)
  {return p_refclNumeric1 - CNumeric(p_llNumeric2); }
  friend CNumeric operator * (const CNumeric &p_refclNumeric1, long long p_llNumeric2)
  {return p_refclNumeric1 * CNumeric(p_llNumeric2); }
  friend CNumeric operator / (const CNumeric &p_refclNumeric1, long long p_llNumeric2)
  {return p_refclNumeric1 / CNumeric(p_llNumeric2); }

  friend CNumeric operator +(long long p_llNumeric1, const CNumeric &p_refclNumeric2)
  {return CNumeric(p_llNumeric1) + p_refclNumeric2; }
  friend CNumeric operator -(long long p_llNumeric1, const CNumeric &p_refclNumeric2)
  {return CNumeric(p_llNumeric1) - p_refclNumeric2; }
  friend CNumeric operator *(long long p_llNumeric1, const CNumeric &p_refclNumeric2)
  {return CNumeric(p_llNumeric1) * p_refclNumeric2; }
  friend CNumeric operator /(long long p_llNumeric1, const CNumeric &p_refclNumeric2)
  {return CNumeric(p_llNumeric1) / p_refclNumeric2; }

  // 功能描述：将类对象存储的数值整数部分返回，小数部分截除（不进行四舍五入）
  operator long long() const;
  // 功能描述：将类对象存储的数值按字符指针返回
  operator char *() const {return (char *)m_szNumeric;}
  // 功能描述：将类对象存储的数值按常量字符指针返回
  operator const char *() const {return (const char *)m_szNumeric;}

  // 功能描述：按类对象的有效精度进行四舍五入运算，并将结果作为类对象的存储数值
  CNumeric & Round(void) {return Round(m_siPrecision);}
  // 功能描述：对p_refclNumeric类对象按指定精度进行四舍五入（不改变p_refclNumeric类对象的存储数值）
  friend CNumeric Round(const CNumeric &p_refclNumeric, short p_siPrecision);

  // 功能描述：取绝对值
  CNumeric & Abs(void);
  // 功能描述：对p_refclNumeric类对象取绝对值（不改变p_refclNumeric类对象的存储数值）
  friend CNumeric Abs(const CNumeric &p_refclNumeric);

  const char *GetNumeric(void) const {return (const char *)m_szNumeric;}
  short GetPrecision(void) const {return m_siPrecision;}
  bool IsOverFlow(void) const {return m_bOverFlow;}

  // 功能描述：比较p_pszNumeric1和p_pszNumeric2字符串所表达数值的大小
  //           该方法被下面比较运算符的重载方法引用
  // 返回说明：
  //           1、若：p_pszNumeric1 > p_pszNumeric2，则返回：1
  //           2、若：p_pszNumeric1 = p_pszNumeric2，则返回：0
  //           3、若：p_pszNumeric1 = p_pszNumeric2，则返回：-1
  int CmpNumeric(const char *p_pszNumeric1, const char *p_pszNumeric2) const;

protected:
  void ResetThisClass(void)
  {
    m_szNumeric[1] = 0x00;m_szTempNumeric[0] = 0x00;//memset((void *)m_szNumeric, 0x00, sizeof(m_szNumeric));//
    m_szNumeric[0] = '0';
    m_siPrecision = 0;
    m_bOverFlow = false;
  }

  // 以下方法都为正值运算，由p_chSign指示正负
  int AddMethod(char *p_pszResult, int p_iSize, const char *p_pszNumeric1, const char *p_pszNumeric2, char p_chSign);
  int SubMethod(char *p_pszResult, int p_iSize, const char *p_pszNumeric1, const char *p_pszNumeric2, char p_chSign);
  int MulMethod(char *p_pszResult, int p_iSize, const char *p_pszNumeric1, const char *p_pszNumeric2, char p_chSign);
  int DivMethod(char *p_pszResult, int p_iSize, const char *p_pszNumeric1, const char *p_pszNumeric2, char p_chSign);

  // 功能描述：按指定精度进行四舍五入运算，并将结果作为类对象的存储数值
  //     [in]  p_siPrecision    指定精度
  //                            大于零则小数位按指定精度进行四舍五入
  //                            小于零则整数位按指定精度进行四舍五入
  CNumeric & Round(short p_siPrecision);
  
  void  fn_filter_zero(char *p_pszNumeric);

protected:
  char m_szNumeric[1024 + 1];
  char m_szTempNumeric[1024 + 1];
  short m_siPrecision;

  bool m_bOverFlow;
};

class CRate : public CNumeric
{
public:
  CRate(const CNumeric &p_refclNumeric) : CNumeric(p_refclNumeric.GetNumeric(), 8) {;}
  CRate(double p_dNumeric) : CNumeric(p_dNumeric, 8) {;}
  CRate(long long p_llNumeric = 0LL) : CNumeric(p_llNumeric) {m_siPrecision = 8; Round();}
  CRate(const char *p_pszNumeric) : CNumeric(p_pszNumeric, 8) {;}
  ~CRate() {;}

  CRate & InitFromLonglong(long long p_llNumeric) { CNumeric::InitFromLonglong(p_llNumeric, 8); return *this; }

protected:
  CNumeric & InitFromNumeric(const CNumeric &p_refclNumeric);
  CNumeric & InitFromDouble(double p_dNumeric, short p_siPrecision = -1);
  CNumeric & InitFromCharPtr(const char *p_pszNumeric, short p_siPrecision = -1);
  CNumeric & InitFromLonglong(long long p_llNumeric, short p_siPrecision);
  
 
};

class CMoney : public CNumeric
{
public:
  CMoney(const CNumeric &p_refclNumeric) : CNumeric(p_refclNumeric.GetNumeric(), 3) {;}
  CMoney(double p_dNumeric) : CNumeric(p_dNumeric, 3) {;}
  CMoney(long long p_llNumeric = 0LL) : CNumeric(p_llNumeric) {m_siPrecision = 3; Round();}
  CMoney(const char *p_pszNumeric) : CNumeric(p_pszNumeric, 3) {;}
  ~CMoney() {;}

  CMoney & InitFromLonglong(long long p_llNumeric) { CNumeric::InitFromLonglong(p_llNumeric, 3); return *this; }

protected:
  CNumeric & InitFromNumeric(const CNumeric &p_refclNumeric);
  CNumeric & InitFromDouble(double p_dNumeric, short p_siPrecision = -1);
  CNumeric & InitFromCharPtr(const char *p_pszNumeric, short p_siPrecision = -1);
  CNumeric & InitFromLonglong(long long p_llNumeric, short p_siPrecision);
};


//typedef CMoney CPrice;
// 3位小数
class CPrice : public CNumeric
{
public:
  CPrice(const CNumeric &p_refclNumeric) : CNumeric(p_refclNumeric.GetNumeric(), 3) {;}
  CPrice(double p_dNumeric) : CNumeric(p_dNumeric, 3) {;}
  CPrice(long long p_llNumeric = 0LL) : CNumeric(p_llNumeric) {m_siPrecision = 3; Round();}
  CPrice(const char *p_pszNumeric) : CNumeric(p_pszNumeric, 3) {;}
  ~CPrice() {;}

  CPrice & InitFromLonglong(long long p_llNumeric) { CNumeric::InitFromLonglong(p_llNumeric, 3); return *this; }

protected:
  CNumeric & InitFromNumeric(const CNumeric &p_refclNumeric);
  CNumeric & InitFromDouble(double p_dNumeric, short p_siPrecision = -1);
  CNumeric & InitFromCharPtr(const char *p_pszNumeric, short p_siPrecision = -1);
  CNumeric & InitFromLonglong(long long p_llNumeric, short p_siPrecision);
};

// 4位小数
class CPrice4 : public CNumeric
{
public:
  CPrice4(const CNumeric &p_refclNumeric) : CNumeric(p_refclNumeric.GetNumeric(), 4) {;}
  CPrice4(double p_dNumeric) : CNumeric(p_dNumeric, 4) {;}
  CPrice4(long long p_llNumeric = 0LL) : CNumeric(p_llNumeric) {m_siPrecision = 4; Round();}
  CPrice4(const char *p_pszNumeric) : CNumeric(p_pszNumeric, 4) {;}
  ~CPrice4() {;}

  CPrice4 & InitFromLonglong(long long p_llNumeric) { CNumeric::InitFromLonglong(p_llNumeric, 4); return *this; }

protected:
  CNumeric & InitFromNumeric(const CNumeric &p_refclNumeric);
  CNumeric & InitFromDouble(double p_dNumeric, short p_siPrecision = -1);
  CNumeric & InitFromCharPtr(const char *p_pszNumeric, short p_siPrecision = -1);
  CNumeric & InitFromLonglong(long long p_llNumeric, short p_siPrecision);
};



bool operator == (const CNumeric &p_refclNumeric1, const CNumeric &p_refclNumeric2);
bool operator != (const CNumeric &p_refclNumeric1, const CNumeric &p_refclNumeric2);
bool operator  < (const CNumeric &p_refclNumeric1, const CNumeric &p_refclNumeric2);
bool operator <= (const CNumeric &p_refclNumeric1, const CNumeric &p_refclNumeric2);
bool operator  > (const CNumeric &p_refclNumeric1, const CNumeric &p_refclNumeric2);
bool operator >= (const CNumeric &p_refclNumeric1, const CNumeric &p_refclNumeric2);

bool operator == (const CNumeric &p_refclNumeric1, const char *p_pszNumeric2);
bool operator != (const CNumeric &p_refclNumeric1, const char *p_pszNumeric2);
bool operator  < (const CNumeric &p_refclNumeric1, const char *p_pszNumeric2);
bool operator <= (const CNumeric &p_refclNumeric1, const char *p_pszNumeric2);
bool operator  > (const CNumeric &p_refclNumeric1, const char *p_pszNumeric2);
bool operator >= (const CNumeric &p_refclNumeric1, const char *p_pszNumeric2);

bool operator == (const char *p_pszNumeric1, const CNumeric &p_refclNumeric2);
bool operator != (const char *p_pszNumeric1, const CNumeric &p_refclNumeric2);
bool operator  < (const char *p_pszNumeric1, const CNumeric &p_refclNumeric2);
bool operator <= (const char *p_pszNumeric1, const CNumeric &p_refclNumeric2);
bool operator  > (const char *p_pszNumeric1, const CNumeric &p_refclNumeric2);
bool operator >= (const char *p_pszNumeric1, const CNumeric &p_refclNumeric2);

bool operator == (const CNumeric &p_refclNumeric1, long long p_llNumeric2);
bool operator != (const CNumeric &p_refclNumeric1, long long p_llNumeric2);
bool operator  < (const CNumeric &p_refclNumeric1, long long p_llNumeric2);
bool operator <= (const CNumeric &p_refclNumeric1, long long p_llNumeric2);
bool operator  > (const CNumeric &p_refclNumeric1, long long p_llNumeric2);
bool operator >= (const CNumeric &p_refclNumeric1, long long p_llNumeric2);

bool operator == (long long p_llNumeric1, const CNumeric &p_refclNumeric2);
bool operator != (long long p_llNumeric1, const CNumeric &p_refclNumeric2);
bool operator  < (long long p_llNumeric1, const CNumeric &p_refclNumeric2);
bool operator <= (long long p_llNumeric1, const CNumeric &p_refclNumeric2);
bool operator  > (long long p_llNumeric1, const CNumeric &p_refclNumeric2);
bool operator >= (long long p_llNumeric1, const CNumeric &p_refclNumeric2);


inline bool operator == (const CNumeric &p_refclNumeric1, const CNumeric &p_refclNumeric2)
{return p_refclNumeric1.CmpNumeric((const char *)p_refclNumeric1, (const char *)p_refclNumeric2) == 0 ? true : false;}
inline bool operator != (const CNumeric &p_refclNumeric1, const CNumeric &p_refclNumeric2)
{return p_refclNumeric1.CmpNumeric((const char *)p_refclNumeric1, (const char *)p_refclNumeric2) != 0 ? true : false;}
inline bool operator  < (const CNumeric &p_refclNumeric1, const CNumeric &p_refclNumeric2)
{return p_refclNumeric1.CmpNumeric((const char *)p_refclNumeric1, (const char *)p_refclNumeric2) < 0 ? true : false;}
inline bool operator <= (const CNumeric &p_refclNumeric1, const CNumeric &p_refclNumeric2)
{return p_refclNumeric1.CmpNumeric((const char *)p_refclNumeric1, (const char *)p_refclNumeric2) <= 0 ? true : false;}
inline bool operator  > (const CNumeric &p_refclNumeric1, const CNumeric &p_refclNumeric2)
{return p_refclNumeric1.CmpNumeric((const char *)p_refclNumeric1, (const char *)p_refclNumeric2) > 0 ? true : false;}
inline bool operator >= (const CNumeric &p_refclNumeric1, const CNumeric &p_refclNumeric2)
{return p_refclNumeric1.CmpNumeric((const char *)p_refclNumeric1, (const char *)p_refclNumeric2) >= 0 ? true : false;}

inline bool operator == (const CNumeric &p_refclNumeric1, const char *p_pszNumeric2)
{return p_refclNumeric1.CmpNumeric((const char *)p_refclNumeric1, p_pszNumeric2) == 0 ? true : false;}
inline bool operator != (const CNumeric &p_refclNumeric1, const char *p_pszNumeric2)
{return p_refclNumeric1.CmpNumeric((const char *)p_refclNumeric1, p_pszNumeric2) != 0 ? true : false;}
inline bool operator < (const CNumeric &p_refclNumeric1, const char *p_pszNumeric2)
{return p_refclNumeric1.CmpNumeric((const char *)p_refclNumeric1, p_pszNumeric2) < 0 ? true : false;}
inline bool operator <= (const CNumeric &p_refclNumeric1, const char *p_pszNumeric2)
{return p_refclNumeric1.CmpNumeric((const char *)p_refclNumeric1, p_pszNumeric2) <= 0 ? true : false;}
inline bool operator  > (const CNumeric &p_refclNumeric1, const char *p_pszNumeric2)
{return p_refclNumeric1.CmpNumeric((const char *)p_refclNumeric1, p_pszNumeric2) > 0 ? true : false;}
inline bool operator >= (const CNumeric &p_refclNumeric1, const char *p_pszNumeric2)
{return p_refclNumeric1.CmpNumeric((const char *)p_refclNumeric1, p_pszNumeric2) >= 0 ? true : false;}

inline bool operator == (const char *p_pszNumeric1, const CNumeric &p_refclNumeric2)
{return p_refclNumeric2.CmpNumeric(p_pszNumeric1, (const char *)p_refclNumeric2) == 0 ? true : false;}
inline bool operator != (const char *p_pszNumeric1, const CNumeric &p_refclNumeric2)
{return p_refclNumeric2.CmpNumeric(p_pszNumeric1, (const char *)p_refclNumeric2) != 0 ? true : false;}
inline bool operator < (const char *p_pszNumeric1, const CNumeric &p_refclNumeric2)
{return p_refclNumeric2.CmpNumeric(p_pszNumeric1, (const char *)p_refclNumeric2) < 0 ? true : false;}
inline bool operator <= (const char *p_pszNumeric1, const CNumeric &p_refclNumeric2)
{return p_refclNumeric2.CmpNumeric(p_pszNumeric1, (const char *)p_refclNumeric2) <= 0 ? true : false;}
inline bool operator > (const char *p_pszNumeric1, const CNumeric &p_refclNumeric2)
{return p_refclNumeric2.CmpNumeric(p_pszNumeric1, (const char *)p_refclNumeric2) > 0 ? true : false;}
inline bool operator >= (const char *p_pszNumeric1, const CNumeric &p_refclNumeric2)
{return p_refclNumeric2.CmpNumeric(p_pszNumeric1, (const char *)p_refclNumeric2) >= 0 ? true : false;}

inline bool operator == (const CNumeric &p_refclNumeric1, long long p_llNumeric2)
{return p_refclNumeric1.CmpNumeric((const char *)p_refclNumeric1, (const char *)CNumeric(p_llNumeric2)) == 0 ? true : false;}
inline bool operator != (const CNumeric &p_refclNumeric1, long long p_llNumeric2)
{return p_refclNumeric1.CmpNumeric((const char *)p_refclNumeric1, (const char *)CNumeric(p_llNumeric2)) != 0 ? true : false;}
inline bool operator < (const CNumeric &p_refclNumeric1, long long p_llNumeric2)
{return p_refclNumeric1.CmpNumeric((const char *)p_refclNumeric1, (const char *)CNumeric(p_llNumeric2)) < 0 ? true : false;}
inline bool operator <= (const CNumeric &p_refclNumeric1, long long p_llNumeric2)
{return p_refclNumeric1.CmpNumeric((const char *)p_refclNumeric1, (const char *)CNumeric(p_llNumeric2)) <= 0 ? true : false;}
inline bool operator > (const CNumeric &p_refclNumeric1, long long p_llNumeric2)
{return p_refclNumeric1.CmpNumeric((const char *)p_refclNumeric1, (const char *)CNumeric(p_llNumeric2)) > 0 ? true : false;}
inline bool operator >= (const CNumeric &p_refclNumeric1, long long p_llNumeric2)
{return p_refclNumeric1.CmpNumeric((const char *)p_refclNumeric1, (const char *)CNumeric(p_llNumeric2)) >= 0 ? true : false;}

inline bool operator == (long long p_llNumeric1, const CNumeric &p_refclNumeric2)
{return p_refclNumeric2.CmpNumeric((const char *)CNumeric(p_llNumeric1), (const char *)p_refclNumeric2) == 0 ? true : false;}
inline bool operator != (long long p_llNumeric1, const CNumeric &p_refclNumeric2)
{return p_refclNumeric2.CmpNumeric((const char *)CNumeric(p_llNumeric1), (const char *)p_refclNumeric2) != 0 ? true : false;}
inline bool operator < (long long p_llNumeric1, const CNumeric &p_refclNumeric2)
{return p_refclNumeric2.CmpNumeric((const char *)CNumeric(p_llNumeric1), (const char *)p_refclNumeric2) < 0 ? true : false;}
inline bool operator <= (long long p_llNumeric1, const CNumeric &p_refclNumeric2)
{return p_refclNumeric2.CmpNumeric((const char *)CNumeric(p_llNumeric1), (const char *)p_refclNumeric2) <= 0 ? true : false;}
inline bool operator > (long long p_llNumeric1, const CNumeric &p_refclNumeric2)
{return p_refclNumeric2.CmpNumeric((const char *)CNumeric(p_llNumeric1), (const char *)p_refclNumeric2) > 0 ? true : false;}
inline bool operator >= (long long p_llNumeric1, const CNumeric &p_refclNumeric2)
{return p_refclNumeric2.CmpNumeric((const char *)CNumeric(p_llNumeric1), (const char *)p_refclNumeric2) >= 0 ? true : false;}

CNumeric Abs(const CNumeric &p_refclNumeric);
CNumeric Round(const CNumeric &p_refclNumeric, short p_siPrecision);

END_NAMESPACE_XSDK

#endif  // __xsdk_numeric_h__
