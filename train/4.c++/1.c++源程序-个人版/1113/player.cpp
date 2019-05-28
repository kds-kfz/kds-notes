#include"player.h"
#include"font.h"
int Player::count3=3000;

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
void Player::showAttr1(){
    attr1.show();
}
void Player::showAttr2(){
    attr2.show();
}
void Player::shownews(){
    cout<<cursor_right2<<"id号："<<id<<"\n"<<endl;
    cout<<cursor_right3<<"帐号："<<name<<"\n"<<endl;
    cout<<cursor_right4<<"密码："<<key<<"\n"<<endl;
    cout<<cursor_right5<<"经验："<<exp<<"\n"<<endl;
    cout<<cursor_right6<<"等级："<<level<<"\n"<<endl;
    cout<<cursor_right2<<"负重："<<weigh<<"kg\n"<<endl;
    cout<<cursor_right3<<"金币："<<wallet.golD<<"\n"<<endl;
    cout<<cursor_right4<<"代币："<<wallet.dimonD<<"\n"<<endl;
    cout<<cursor_right5<<"钻石："<<wallet.tokeN<<"\n"<<endl;
    cout<<cursor_right6<<"QB："<<wallet.QB<<"\n"<<endl;
}

