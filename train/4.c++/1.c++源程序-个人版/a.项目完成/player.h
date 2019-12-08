#ifndef _PLAYER_H
#define _PLAYER_H
#include<map>
#include"shop.h"
using namespace std;
class Player{
    int id;		//玩家id
    string name;	//玩家名字
    string key;		//玩家密码
//    int gold;		
    int exp;		//玩家经验
    int level;		//玩家等级
    int weigh;		//玩家负重 
    static int count3;
//    int diamond;    
    Attribute attr1;//玩家状态1
    Attribute attr2;//玩家状态1
    Value wallet;	//玩家钱包
    public:
//    int parts[10]={Hat,Shirt,Pants,Belt,Shoes,Hand,Dress};
//    int rares[10]={Common,Senior,Smriti,Relic,Artifact,Fame,Epic,Limit};
    //保存装备部位+稀有
    char parts[10]={};//7个
    char rares[10]={};//8个
    
    map<string,pair<Actor *,int> >bag;//道具name，道具地址，数量
    
    Player() : Player("马叉虫","--"){}
    Player(string n,string k) : id(count3++),name(n),key(k),
    exp(0),level(1),weigh(500),wallet(99999,88888,0,500,0){}
    Player(int i,string n,string k) : id(i),name(n),key(k),
    exp(0),level(1),weigh(500),wallet(99999,88888,0,500,0){}
    //读数据库后构造
    Player(int i,string n,string k,int e,int l,int w,
	   int h1,int m1,float ps1,float ps2,int a1,int a2,int d1,int r1,
	   int h2,int m2,float ps3,float ps4,int a3,int a4,int d2,int r2,
	   int g,int d,int qb,int t,char *a,char *b):
	id(i),name(n),key(k),exp(e),level(l),weigh(w),
	attr1(h1,m1,ps1,ps2,a1,a2,d1,r1),attr2(h2,m2,ps3,ps4,a3,a4,d2,r2),
	wallet(g,d,0,t,qb)
    {
	for(int i;a[i]!='\0';i++)
	    parts[i]=a[i];
	for(int i;b[i]!='\0';i++)
	    rares[i]=b[i];
    }
    
    Player(const Player &p)=delete;
    //显示基本状态
    void showAttr1();
    //显示加成状态
    void showAttr2();
    void showAttr3();
    //显示玩家信息
    void shownews();

    int getid()const{return id;}
    string getna()const{return name;}
    string getke()const{return key;}
    int getle()const{return level;}
    int getex()const{return exp;}
    int getwe()const{return weigh;}
    int getgo()const{return wallet.golD;}
    int getdi()const{return wallet.dimonD;}
    int getqb()const{return wallet.QB;}
    int getto()const{return wallet.tokeN;}
    void set_wallet1(int n){wallet.golD=n;}
    void set_wallet2(int n){wallet.dimonD=n;}
    void set_wallet3(int n){wallet.pointS=n;}
    void set_wallet5(int n){wallet.tokeN=n;}
    void set_wallet4(int n){wallet.QB=n;}
    Attribute getat1()const{return attr1;}//玩家状态1
    Attribute getat2()const{return attr2;}//玩家状态1
    //使用装备
    void addAttr(const Attribute &at);
    //移除装备
    void reduceAttr(const Attribute &at);
    /*
    ~Player(){
	for(auto m:bag)
	    delete m.second.first;
    }
    */
};

//其它函数声明
extern void reduceAttr(Attribute &at1,const Attribute &at2);

#endif
