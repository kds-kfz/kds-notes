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
    map<int,Actor *>bag;
    
    Player() : Player("马叉虫","--"){}
    Player(string n,string k) : id(count3++),name(n),key(k),
    exp(0),level(1),weigh(500),wallet(2000,10,0,500,0){}
    
    Player(const Player &p)=delete;
    //使用装备
    void useEqu(const HAT &h);
    //移除装备
    void nonuseEqu(const HAT &h);
    //添加部位防具到背包
    void addArm(const Armor &ar);
    //添加类型装备到背包
//    void addEqu(const Equip &eq);
    //使用药剂消耗品
//    void useDrug(const Equip &eq);
    //显示基本状态
    void showAttr1();
    //显示加成状态
    void showAttr2();
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
};


#endif
