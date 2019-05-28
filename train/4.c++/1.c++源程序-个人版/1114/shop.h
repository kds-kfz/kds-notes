#ifndef _SHOP_H_
#define _SHOP_H_
#include<iostream>
#include<map>
using namespace std;
/*---------------------------------------------------------------枚举装备部位*/
enum PARTS{	//装备部位
    Other,
    Hat,	//帽子
    Shirt,	//衬衣
    Pants,	//裤子
    Belt,	//腰带
    Shoes,	//鞋子
    Weapon,	//武器
    Drug	//药剂
};
/*---------------------------------------------------------------枚举装备类型*/
enum TYPE{	//装备类型
    Hand,	//手部
    Body,	//身体
    Dress,	//时装
    Orbament	//饰品
};
/*-------------------------------------------------------------------枚举财富*/
enum WEALTH{	//商品财富	
    Gold=1,	//金币	游戏中获得
    Dimond,	//钻石	1元=10钻,活动中可充值
    Points,	//点券	1元=10点
    Token	//代币	游戏中获得
};
/*-------------------------------------------------------------------商品价值*/
struct Value{	//商品价值，玩家钱包	
    int golD;	//金币	游戏中获得
    int dimonD;	//钻石	1元=10钻,活动中可充值
    int pointS;	//点券	1元=10点
    int tokeN;	//代币	游戏中获得
    int QB;	//Q币	1元=1QB
    Value() : Value(0,0,0,0,0){}
    Value(int g,int d,int p,int t,int q) : 
	golD(g),dimonD(d),pointS(p),tokeN(t),QB(q){}
    Value(const Value &v)=delete;
};
//void my_Wealth(int a,int b,int c,int d,int e);
/*---------------------------------------------------------枚举部位装备稀有度*/
enum RARE{	//装备稀有度---时装---饰品
    otheR,
    Common,	//普通
    Senior,	//高级
    Smriti,	//传承
    Relic,	//圣物
    Artifact,	//神器
    Fame,	//传说
    Epic,	//史诗
    Limit	//限定
};
/*---------------------------------------------------------------结构体-状态*/
class Attribute{
    public:
    int hp;	//生命值
    int mp;	//魔法值
    float spd;	//功速
    float spd2;	//移动速度
    int atk;	//物理伤害
    int atm;	//魔法伤害
    int def;	//物理防御
    int res;	//魔法防御
    int ten;	//持久度
    RARE rare;	//稀有度
    WEALTH weal;//价值

    Attribute() : Attribute(2000,800,0.4,0.6,300,150,220,90,0,Common,Gold){}
    Attribute(int h,int m,float s1,float s2,int a1,int a2,int d,int r) : 
	hp(h),mp(m),spd(s1),spd2(s2),atk(a1),atm(a2),def(d),res(r){}
    Attribute(int h,int m,float s1,float s2,int a1,
	    int a2,int d,int r1,int t2,RARE r2,WEALTH w) : 
	hp(h),mp(m),spd(s1),spd2(s2),atk(a1),
	atm(a2),def(d),res(r1),ten(t2),rare(r2),weal(w){}
    void show();
};
/*---------------------------------------------------------------抽象类-物品*/
class Actor{		    //物品：是防具部位的基类
    protected:
	int id;		    //id号
	int level;	    //等级
	int weight;	    //重量
	PARTS part;	    //部位
	string name;	    //名称
	string nature;	    //属性
	static int count;   //id号获取
    public:
	Actor() : Actor(0,0,Hat,"物品","--"){}
	Actor(int l,int w,PARTS p,string n1,string n2) : 
	    id(count++),level(l),weight(w),part(p),name(n1),nature(n2){}
	Actor(const Actor &a)=delete;
	virtual void show()=0;
	virtual void show2()=0;
	virtual void show3()=0;
	virtual string getn1()const=0;
	virtual string getn2()const=0;
	virtual int getid()const=0;
	virtual int getle()const=0;
	virtual int getva()const=0;
	virtual int getdi()const=0;
	virtual int getwe()const=0;
	virtual PARTS getpa()const=0;
	virtual RARE getra()const=0;
};
/*
class Actor2{		    //物品2：是装备类型的基类
    protected:
	int id;		    //id号
	int level;	    //等级
	TYPE type;	    //类型
	string name;	    //名称
	string nature;	    //属性
	static int count2;   //id号获取
    public:
	Actor2() : Actor2(0,Weapon,"物品2","--"){}
	Actor2(int l,TYPE t,string n1,string n2) : 
	    id(count2++),level(l),type(t),name(n1),nature(n2){}
	Actor2(const Actor2 &a)=delete;
	virtual void show()=0;
	virtual string getn1()const=0;
	virtual string getn2()const=0;
	virtual int getid()const=0;
	virtual int getle()const=0;
	virtual int getva()const=0;
};
*/
//int Actor::count=1000;//初始化道具id
/*---------------------------------------------------------------派生类-防具*/
class Armor : public Actor{//防具：部位
    public:
	Armor() : Armor(0,0,Hat,"装备","--"){}
	Armor(int l,int w,PARTS p,string n1,string n2) : Actor(l,w,p,n1,n2){}
	Armor(const Armor &p)=delete;
	virtual void show()=0;//显示效果
};
/*---------------------------------------------------------------派生类-装备*/
/*
class Equip : public Actor2{//装备：类型
    public:
	Equip() : Equip(0,Weapon,"武器","--"){}
	Equip(int i,TYPE t,string n1,string n2) : Actor2(i,t,n1,n2){}
	Equip(const Equip &p)=delete;
	virtual void show()=0;//显示效果
};
*/
/*********************************************************************/
class HAT : public Armor{
    Attribute attr;//装备效果
    Value value;
    public:
    HAT() : HAT(0,0,"头盔","--",0,0,0,0,0,0,Common,Gold){}
    HAT(int l,int w2,string n1,string n2,int a,int d,int r1,int t,
	    int g,int d2,RARE r2,WEALTH w) : 
	Armor(l,w2,Hat,n1,n2){
	    attr.atk=a;
	    attr.def=d;
	    attr.res=r1,
	    attr.ten=t;
	    value.golD=g;
	    value.dimonD=d2;
	    attr.rare=r2;
	    attr.weal=w;
	}
    void show();//显示效果
    void show2();//显示效果
    void show3();//显示效果
    HAT(const HAT &h)=delete; 
    Attribute getat()const{return attr;}
    int getva()const{return value.golD;}
    string getn1()const{return name;}
    string getn2()const{return nature;}
    int getid()const{return id;}
    int getle()const{return level;}
    int getdi()const{return value.dimonD;}
    int getwe()const{return weight;}
    PARTS getpa()const{return part;}
    RARE getra()const{return attr.rare;}

};
/**********************************************************************/
class SHIRT : public Armor{
    Attribute attr;//装备效果
    Value value;
    public:
    SHIRT() : SHIRT(0,0,"铁布衫","--",0,0,0,0,0,0,Common,Gold){}
    SHIRT(int l,int w2,string n1,string n2,int h,float s,int d,int t,
	    int g,int d2,RARE r,WEALTH w) : 
	Armor(l,w2,Shirt,n1,n2){
	    attr.hp=h;
	    attr.spd=s;
	    attr.def=d,
	    attr.ten=t;
	    value.golD=g;
	    value.dimonD=d2;
	    attr.rare=r;
	    attr.weal=w;}
    void show();//显示效果
    void show2();//显示效果
    void show3();//显示效果
    SHIRT(const SHIRT &h)=delete; 
    Attribute getat()const{return attr;}
    int getva()const{return value.golD;}
    string getn1()const{return name;}
    string getn2()const{return nature;}
    int getid()const{return id;}
    int getle()const{return level;}
    int getdi()const{return value.dimonD;}
    int getwe()const{return weight;}
    PARTS getpa()const{return part;}
    RARE getra()const{return attr.rare;}
};
/*********************************************************************/
class PANTS : public Armor{
    Attribute attr;//装备效果
    Value value;
    public:
    PANTS() : PANTS(0,0,"长裤","--",0,0,0,0,0,0,Common,Gold){}
    PANTS(int l,int w2,string n1,string n2,int h,int d,int r,int t,
	    int g,int d2,RARE r2,WEALTH w) : 
	Armor(l,w2,Pants,n1,n2){
	    attr.hp=h;
	    attr.def=d;
	    attr.res=r,
	    attr.ten=t;
	    value.golD=g;
	    value.dimonD=d2;
	    attr.rare=r2;
	    attr.weal=w;}
    void show();//显示效果
    void show2();//显示效果
    void show3();//显示效果
    PANTS(const PANTS &h)=delete; 
    Attribute getat()const{return attr;}
    int getva()const{return value.golD;}
    string getn1()const{return name;}
    string getn2()const{return nature;}
    int getid()const{return id;}
    int getle()const{return level;}
    int getdi()const{return value.dimonD;}
    int getwe()const{return weight;}
    PARTS getpa()const{return part;}
    RARE getra()const{return attr.rare;}
};
/**********************************************************************/
class BELT : public Armor{
    Attribute attr;//装备效果
    Value value;
    public:
    BELT() : BELT(0,0,"腰带","--",0,0,0,0,0,0,Common,Gold){}
    BELT(int l,int w2,string n1,string n2,int h,int d,int r,int t,
	    int g,int d2,RARE r2,WEALTH w) : 
	Armor(l,w2,Belt,n1,n2){
	    attr.hp=h;
	    attr.def=d;
	    attr.res=r,
	    attr.ten=t;
	    value.golD=g;
	    value.dimonD=d2;
	    attr.rare=r2;
	    attr.weal=w;}
    void show();//显示效果
    void show2();//显示效果
    void show3();//显示效果
    BELT(const BELT &h)=delete; 
    Attribute getat()const{return attr;}
    int getva()const{return value.golD;}
    string getn1()const{return name;}
    string getn2()const{return nature;}
    int getid()const{return id;}
    int getle()const{return level;}
    int getdi()const{return value.dimonD;}
    int getwe()const{return weight;}
    PARTS getpa()const{return part;}
    RARE getra()const{return attr.rare;}
};
/**********************************************************************/
class SHOES : public Armor{
    Attribute attr;//装备效果
    Value value;
    public:
    SHOES() : SHOES(0,0,"忍者鞋","--",0,0,0,0,0,0,Common,Gold){}
    SHOES(int l,int w2,string n1,string n2,float s,int d,int r,int t,
	    int g,int d2,RARE r2,WEALTH w): 
	Armor(l,w2,Shoes,n1,n2){
	    attr.spd2=s;
	    attr.def=d;
	    attr.res=r;
	    attr.ten=t;
	    value.golD=g;
	    value.dimonD=d2;
	    attr.rare=r2;
	    attr.weal=w;}
    void show();//显示效果
    void show2();//显示效果
    void show3();//显示效果
    SHOES(const SHOES &h)=delete; 
    Attribute getat()const{return attr;}
    int getva()const{return value.golD;}
    string getn1()const{return name;}
    string getn2()const{return nature;}
    int getid()const{return id;}
    int getle()const{return level;}
    int getdi()const{return value.dimonD;}
    int getwe()const{return weight;}
    PARTS getpa()const{return part;}
    RARE getra()const{return attr.rare;}
};
/**********************************************************************/
#if 0
class WEAPON : public Equip{
    Attribute eattr;//装备效果
    Value value;
    public:
    WEAPON() : WEAPON(0,"武器","--",0,0,0,0,0,Common,Gold){}
    WEAPON(int l,string n1,string n2,float s,int a1,int a2,int t,int g,RARE r2,WEALTH w) : 
	Equip(l,Weapon,n1,n2){
	    eattr.spd=s;
	    eattr.atk=a1;
	    eattr.atm=a2;
	    eattr.ten=t;
	    value.golD=g;
	    eattr.rare=r2;
	    eattr.weal=w;}
    void show();//显示效果
    WEAPON(const WEAPON &h)=delete; 
    Attribute getat()const{return eattr;}
    int getva()const{return value.golD;}
    string getn1()const{return name;}
    string getn2()const{return nature;}
    int getid()const{return id;}
    int getle()const{return level;}
};
/*---------------------------------------------------------------*/
class DRUG : public Equip{
    Attribute eattr;//装备效果
    Value value;
    public:
    DRUG() : DRUG(0,"药剂","--",0,0,Common,Gold){}
    DRUG(int l,string n1,string n2,int h,int g,RARE r2,WEALTH w) : 
	Equip(l,Drug,n1,n2){
	    eattr.hp=h;
	    value.golD=g;
	    eattr.rare=r2;
	    eattr.weal=w;}
    void show();//显示效果
    DRUG(const DRUG &h)=delete; 
    Attribute getat()const{return eattr;}
    int getva()const{return value.golD;}
    string getn1()const{return name;}
    string getn2()const{return nature;}
    int getid()const{return id;}
    int getle()const{return level;}
};
#endif
class WEAPON : public Armor{
    Attribute attr;//装备效果
    Value value;
    public:
    WEAPON() : WEAPON(0,0,"武器","--",0,0,0,0,0,0,Common,Gold){}
    WEAPON(int l,int w2,string n1,string n2,float s,int a1,int a2,int t,
	    int g,int d2,RARE r2,WEALTH w) : 
	Armor(l,w2,Weapon,n1,n2){
	    attr.spd=s;
	    attr.atk=a1;
	    attr.atm=a2;
	    attr.ten=t;
	    value.golD=g;
	    value.dimonD=d2;
	    attr.rare=r2;
	    attr.weal=w;}
    void show();//显示效果
    void show2();//显示效果
    void show3();//显示效果
    WEAPON(const WEAPON &h)=delete; 
    Attribute getat()const{return attr;}
    int getva()const{return value.golD;}
    string getn1()const{return name;}
    string getn2()const{return nature;}
    int getid()const{return id;}
    int getle()const{return level;}
    int getdi()const{return value.dimonD;}
    int getwe()const{return weight;}
    PARTS getpa()const{return part;}
    RARE getra()const{return attr.rare;}
};
/*---------------------------------------------------------------*/
class DRUG : public Armor{
    Attribute attr;//装备效果
    Value value;
    public:
    DRUG() : DRUG(0,0,"药剂","--",0,0,0,Common,Gold){}
    DRUG(int l,int w2,string n1,string n2,int h,int g,int d2,RARE r2,WEALTH w) : 
	Armor(l,w2,Drug,n1,n2){
	    attr.hp=h;
	    value.golD=g;
	    value.dimonD=d2;
	    attr.rare=r2;
	    attr.ten=1000;
	    attr.mp=200;
	    attr.weal=w;}
    void show();//显示效果
    void show2();//显示效果
    void show3();//显示效果
    DRUG(const DRUG &h)=delete; 
    Attribute getat()const{return attr;}
    int getva()const{return value.golD;}
    string getn1()const{return name;}
    string getn2()const{return nature;}
    int getid()const{return id;}
    int getle()const{return level;}
    int getdi()const{return value.dimonD;}
    int getwe()const{return weight;}
    PARTS getpa()const{return part;}
    RARE getra()const{return attr.rare;}
};
//抽象工厂
class Factory{
    public:
    //生产部位防具
    virtual Armor *produceArm(int l,int w2,PARTS p,string n1,string n2,
	int h,int m,float s1,float s2,int a1,int a2,int d,
	    int r,int t2,int g,int d2,RARE r2,WEALTH w)=0;
/*
    //生产类型装备
    virtual Equip *produceEqu(int l,TYPE t,string n1,string n2,
	int h,int m,float s1,float s2,int a1,int a2,int d,
	    int r,int t2,int g,RARE r2,WEALTH w)=0;
*/
    //生产部位防具---时装---首饰---武器---药剂
    virtual Armor *produceArm(int l,int w2,TYPE t,PARTS p,string n1,string n2,
	int h,int m,float s1,float s2,int a1,int a2,int d,
	    int r,int t2,int g,int d2,RARE r2,WEALTH w)=0;

    virtual ~Factory(){}
};
class Factory_All : public Factory{
    public:
    Armor *produceArm(int l,int w2,PARTS p,string n1,string n2,
	    int h,int m,float s1,float s2,int a1,int a2,int d,
		    int r,int t2,int g,int d2,RARE r2,WEALTH w);
    /*
    Equip *produceEqu(int l,TYPE t,string n1,string n2,
	int h,int m,float s1,float s2,int a1,int a2,int d,
	    int r,int t2,int g,RARE r2,WEALTH w);
    */
    Armor *produceArm(int l,int w2,TYPE t,PARTS p,string n1,string n2,
	int h,int m,float s1,float s2,int a1,int a2,int d,
	    int r,int t2,int g,int d2,RARE r2,WEALTH w);
    
    virtual ~Factory_All(){}
};
#endif
