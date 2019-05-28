#ifndef _PLAYER_H
#define _PLAYER_H
#include<map>
#include"shop.h"
using namespace std;

class Player{
    int id;		//玩家id
    string name;	//玩家名字
//    int gold;		
    int exp;		//玩家经验
    int level;		//玩家等级
//    int diamond;    
    Attribute attr1;	//玩家状态1
    Attribute attr2;	//玩家状态2
    Value wallet;	//玩家钱包
    public:
    map<int,Actor *>bag;
    
    Player() : Player(1100,"马叉虫",0,1){}
    Player(int i,string n,int e,int l) : 
	id(i),name(n),exp(e),level(l),wallet(2000,0,0,500,0){}
    
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
    //显示状态
    void showAttr();
};


#endif
