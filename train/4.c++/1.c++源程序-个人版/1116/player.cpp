#include"player.h"
#include"font.h"
int Player::count3=3000;

void Player:: addAttr(const Attribute &at){
    attr2.hp+=at.hp;
    attr2.mp+=at.mp;
    attr2.spd+=at.spd;
    attr2.spd2+=at.spd2;
    attr2.atk+=at.atk;
    attr2.atm+=at.atm;
    attr2.def+=at.def;
    attr2.res+=at.res;
}
void Player::reduceAttr(const Attribute &at){
    attr2.hp-=at.hp;
    attr2.mp-=at.mp;
    attr2.spd-=at.spd;
    attr2.atk-=at.atk;
    attr2.atm-=at.atm;
    attr2.def-=at.def;
    attr2.res-=at.res;
}
void Player::showAttr1(){
    attr1.show();
}
void Player::showAttr2(){
    attr2.show();
}
void Player::showAttr3(){
    set_cursor(61,9);
    cout<<yellow<<attr2.hp<<" |"<<attr1.hp<<"+"<<attr2.hp-attr1.hp;
    set_cursor(61,10);
    cout<<yellow<<attr2.mp<<" |"<<attr1.mp<<"+"<<attr2.mp-attr1.mp;
    set_cursor(61,11);
    cout<<yellow<<attr2.spd<<" |"<<attr1.spd<<"+"<<attr2.spd-attr1.spd;
    set_cursor(61,12);
    cout<<yellow<<attr2.spd2<<" |"<<attr1.spd2<<"+"<<attr2.spd2-attr1.spd2;
    set_cursor(61,13);
    cout<<yellow<<attr2.atk<<" |"<<attr1.atk<<"+"<<attr2.atk-attr1.atk;
    set_cursor(61,14);
    cout<<yellow<<attr2.atm<<" |"<<attr1.atm<<"+"<<attr2.atm-attr1.atm;
    set_cursor(61,15);
    cout<<yellow<<attr2.def<<" |"<<attr1.def<<"+"<<attr2.def-attr1.def;
    set_cursor(61,16);
    cout<<yellow<<attr2.res<<" |"<<attr1.res<<"+"<<attr2.res-attr1.res;

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

