#if !defined(MT_DATA_H)
#define MT_DATA_H

#include <string>

#define VAR_CODE_LEN	(32)
#define VAR_NAME_LEN	(64)

//基础数据定义
struct TagCode
{
	short setcode;
	char code[VAR_CODE_LEN];
	TagCode& operator =(const TagCode& other)
	{
		memcpy(this, &other, sizeof(TagCode));
		return *this;
	}
	TagCode(short psetcode, const char* pcode) :setcode(psetcode)
	{
		memcpy(code, pcode, VAR_CODE_LEN - 1);
	}
	TagCode()
	{
		memset(this, 0, sizeof(TagCode));
	}
};

union StockDataSubType
{
	struct flag_t
	{
		unsigned long long HQ : 1;	//行情
		unsigned long long ZQDM : 1;	//代码
		unsigned long long ZJCJ : 1;	//现价
		unsigned long long ZGCJ : 1;	//最高
		unsigned long long ZDCJ : 1;	//最低
		unsigned long long JRKP : 1;	//今开
		unsigned long long ZRSP : 1;	//昨收
		unsigned long long CJL : 1;		//总手
	}u;
	unsigned long long value;
	StockDataSubType& operator |= (const StockDataSubType& other)
	{
		this->u.HQ |= other.u.HQ;
		this->u.ZQDM |= other.u.ZQDM;
		this->u.ZJCJ |= other.u.ZJCJ;
		this->u.ZGCJ |= other.u.ZGCJ;
		this->u.ZDCJ |= other.u.ZDCJ;
		this->u.JRKP |= other.u.JRKP;
		this->u.ZRSP |= other.u.ZRSP;
		this->u.CJL |= other.u.CJL;
		return *this;
	}
	StockDataSubType& operator &= (const StockDataSubType& other)
	{
		this->u.HQ &= other.u.HQ;
		this->u.ZQDM &= other.u.ZQDM;
		this->u.ZJCJ &= other.u.ZJCJ;
		this->u.ZGCJ &= other.u.ZGCJ;
		this->u.ZDCJ &= other.u.ZDCJ;
		this->u.JRKP &= other.u.JRKP;
		this->u.ZRSP &= other.u.ZRSP;
		this->u.CJL &= other.u.CJL;
		return *this;
	}
};

#endif