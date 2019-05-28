#include"player.h"

void addAttr(Attribute &at1,const Attribute &at2){
    at1.hp+=at2.hp;
    at1.mp+=at2.mp;
    at1.spd+=at2.spd;
    at1.atk+=at2.atk;
    at1.atm+=at2.atm;
    at1.def+=at2.def;
    at1.res+=at2.res;
}
void reduceAttr(Attribute &at1,const Attribute &at2){
    at1.hp-=at2.hp;
    at1.mp-=at2.mp;
    at1.spd-=at2.spd;
    at1.atk-=at2.atk;
    at1.atm-=at2.atm;
    at1.def-=at2.def;
    at1.res-=at2.res;
}
void Player::useEqu(const HAT &h){
    cout<<"配备了:"<<h.getn1()<<" 效果:"<<h.getn2()<<endl;
    addAttr(attr2,h.getat());
}
void Player::nonuseEqu(const HAT &h){
    cout<<"移除了:"<<h.getn1()<<" 效果减少:"<<h.getn2()<<endl;
    reduceAttr(attr2,h.getat());
}

void Player::addArm(const Armor &ar){}
//void Player::addEqu(const Equip &eq){}
/*
   void Player::useDrug(const Equip &eq){
   cout<<"配备了:"<<eq.getn1()<<" 效果:"<<eq.getn2()<<endl;
   addAttr(attr2,eq.getat());
   }
 */
void Player::showAttr(){
    cout<<"id号\t"<<"名字\t"<<"经验\t"<<"等级\t"<<"金币\t"<<"钻石\t"<<"QB\t"<<endl;
    cout<<id<<"\t"<<name<<"\t"<<exp<<"\t"<<level<<"\t"
	<<wallet.golD<<"\t"<<wallet.dimonD<<"\t"<<wallet.QB<<endl;
    cout<<"原始状态:"<<endl;
    attr1.show();
    cout<<"装备状态:"<<endl;
    attr2.show();
}
