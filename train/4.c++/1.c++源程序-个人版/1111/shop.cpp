#include"shop.h"
//shop.cpp
/*
   enum Goods{
   HAT,SHIRT,PANTS,BELT,SHOES,WEAPON,
   CLOTHES,DRESS,ORBAMENTA
   };
 */
int Actor::count=1000;//初始化部位防具id
int Actor2::count2=2000;//初始化类型装备id

Equip * Factory_All::produceEqu(int l,TYPE t,string n1,string n2,
	int h,int m,float s1,float s2,int a1,int a2,int d,
	    int r,int t2,int g,RARE r2,WEALTH w){
	switch(t){
	case Weapon:
	    return new WEAPON{l,n1,n2,s1,a1,a2,t2,g,r2,w};
	case Drug:
	    return new DRUG{l,n1,n2,h,g,r2,w};
	default:return NULL;
    }
}

Armor * Factory_All::produceArm(int l,PARTS p,string n1,string n2,
    int h,int m,float s1,float s2,int a1,int a2,int d,
    int r,int t2,int g,RARE r2,WEALTH w){
	switch(p){
	case Hat:
	    return new HAT{l,n1,n2,a1,d,r,t2,g,r2,w};
	case Shirt:
	    return new SHIRT{l,n1,n2,h,s1,d,t2,g,r2,w};
	case Pants:
	    return new PANTS{l,n1,n2,h,d,r,t2,g,r2,w};
	case Belt:
	    return new BELT{l,n1,n2,h,d,r,t2,g,r2,w};
	case Shoes:
	    return new SHOES{l,n1,n2,s2,d,r,t2,g,r2,w};
	default:return NULL;
    }
}
Armor * Factory_All::produceArm(int l,TYPE t,PARTS p,RARE r1,string n1,string n2,
	int h,int m,float s1,float s2,int a1,int a2,int d,
	    int r,int t2,int g,RARE r2,WEALTH w){
	switch(t){
	case Dress:
	    switch(p){
		case Hat:
		    return new HAT{l,n1,n2,a1,d,r,t2,g,r2,w};
		case Shirt:
		    return new SHIRT{l,n1,n2,h,s1,d,t2,g,r2,w};
		case Pants:
		    return new PANTS{l,n1,n2,h,d,r,t2,g,r2,w};
		case Belt:
		    return new BELT{l,n1,n2,h,d,r,t2,g,r2,w};
		case Shoes:
		    return new SHOES{l,n1,n2,s2,d,r,t2,g,r2,w};
		default:return NULL;
	    }
	case Orbament:
	    switch(p){
		case Hat:
		    return new HAT{l,n1,n2,a1,d,r,t2,g,r2,w};
		case Shirt:
		    return new SHIRT{l,n1,n2,h,s1,d,t2,g,r2,w};
		case Pants:
		    return new PANTS{l,n1,n2,h,d,r,t2,g,r2,w};
		case Belt:
		    return new BELT{l,n1,n2,h,d,r,t2,g,r2,w};
		case Shoes:
		    return new SHOES{l,n1,n2,s2,d,r,t2,g,r2,w};
		default:return NULL;
	    }
    }
}
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
void Attribute::show(){
    cout<<"生命\t"<<"魔法\t"<<"攻速\t"
	<<"普攻\t"<<"魔攻\t"<<"物抗\t"<<"魔抗"<<endl;
    cout<<hp<<"\t"<<mp<<"\t"<<spd<<"\t"
	<<atk<<"\t"<<atm<<"\t"<<def<<"\t"<<res<<endl;
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
void Player::addEqu(const Equip &eq){}
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
void Game::show_goods(){
    /*
    map<Actor *,pair<int,pair<int,int> > >goods;//商品，等级，价格
    map<Actor2 *,pair<int,pair<int,int> > >good2;//商品，等级，价格
    */
    map<Actor *,pair<int,int> >good3;//商品,价格
    map<Actor *,int>good4;//商品,数量
    map<Actor2 *,int>good5;//商品,数量
    Factory_All ap;
    Armor *p1=ap.produceArm(1,Hat,"圣战头盔","增加攻击物抗和魔抗，普通",
	    0,0,0,0,60,0,100,40,200,400,Common,Gold);
    Armor *p2=ap.produceArm(3,Shirt,"荣光夹克","增加生命攻速和物抗，高级",
	    100,0,0.3,0,0,0,60,0,180,450,Common,Gold);
    Armor *p3=ap.produceArm(5,Pants,"海神长裤","增加生命物抗和魔抗，稀有",
	    200,0,0,0,0,0,70,50,230,500,Common,Gold);
    Armor *p4=ap.produceArm(7,Belt,"龙麟腰带","增加生命物抗和魔抗，传承",
	    300,0,0,0,0,0,100,100,240,550,Common,Gold);
    Armor *p5=ap.produceArm(9,Shoes,"忍者足具","增加生命物抗和魔抗，稀有",
	    0,0,0,60,0,0,50,40,190,600,Common,Gold);

    Equip *q1=ap.produceEqu(2,Weapon,"绝世好剑","增加攻速物攻魔伤，传说",
	    0,0,0.7,0,300,100,0,0,300,650,Common,Gold);
    Equip *q2=ap.produceEqu(4,Drug,"死者苏醒","大量增加生命值，史诗",
	    500,0,0,0,0,0,0,0,0,700,Common,Gold);
    good4.insert({p1,1});
    good4.insert({p2,1});
    good4.insert({p3,1});
    good4.insert({p4,1});
    good4.insert({p5,1});

    good5.insert({q1,1});
    good5.insert({q2,1});
//    good3.insert({p1,{450,9}});
//    good3.insert({p2,{450,9}});
    /*
    goods.insert({p2,{3,{99,20}}});
    goods.insert({p3,{5,{99,20}}});
    goods.insert({p4,{7,{99,20}}});
    goods.insert({p5,{9,{99,20}}});
    goods.insert({p6,{9,{99,20}}});

    good2.insert({q1,{2,{99,20}}});
    good2.insert({q2,{4,{99,20}}});
    //    good2.insert({q3,{6,{99,20}}});
    //    good2.insert({q4,{8,{99,20}}});
    cout<<"\t\t---道具商城---"<<endl;
    cout<<"道具id\t"<<"道具名称\t"<<"等级\t"<<"金币\t"<<"钻石"<<endl;
    for(auto m:goods){
	cout<<m.first->getid()<<"\t"<<m.first->getn1()<<"\t"
	    <<m.second.first<<"\t"<<m.second.second.first<<"\t"
	    <<m.second.second.second<<endl;
    }
    */
    /*
    for(auto m:good2){
	cout<<m.first->getid()<<"\t"<<m.first->getn1()<<"\t"
	    <<m.second.first<<"\t"<<m.second.second.first<<"\t"
	    <<m.second.second.second<<endl;
    }
    */
    /*
    cout<<"\t\t---道具商城---"<<endl;
    cout<<"道具id\t"<<"道具名称\t"<<"等级\t"<<"金币\t"<<"钻石"<<endl;
    for(auto m:good3){
	cout<<m.first->getid()<<"\t"<<m.first->getn1()<<"\t"
	    <<m.first->getle()<<"\t"<<m.second.first<<"\t"
	    <<m.second.second<<endl;
    }
    */
    cout<<"\t\t---道具商城---"<<endl;
    cout<<"道具id\t"<<"道具名称\t"<<"等级\t"<<"金币\t"<<"数量"<<endl;
    for(auto m:good4){
	cout<<m.first->getid()<<"\t"<<m.first->getn1()<<"\t"
	    <<m.first->getle()<<"\t"<<m.first->getva()<<"\t"
	    <<m.second<<endl;
    }
    cout<<"----------------------------------------------------------"<<endl;
    for(auto m:good5){
	cout<<m.first->getid()<<"\t"<<m.first->getn1()<<"\t"
	    <<m.first->getle()<<"\t"<<m.first->getva()<<"\t"
	    <<m.second<<endl;
    }
}
void HAT::show(){//显示效果
    cout<<"装备名称:"<<name<<endl;
    cout<<"装备属性:"<<nature<<endl;
    cout<<"物理伤害：+"<<attr.atk<<endl;
    cout<<"物理防御：+"<<attr.def<<endl;
    cout<<"魔法防御：+"<<attr.res<<endl;
    cout<<"耐久度："<<attr.ten<<endl;
}
void SHIRT::show(){//显示效果
    cout<<"装备名称:"<<name<<endl;
    cout<<"装备属性:"<<nature<<endl;
    cout<<"生命值：+"<<attr.hp<<endl;
    cout<<"攻击速度：+"<<attr.spd<<endl;
    cout<<"物理防御：+"<<attr.def<<endl;
    cout<<"耐久度："<<attr.ten<<endl;
}
void PANTS::show(){//显示效果
    cout<<"装备名称:"<<name<<endl;
    cout<<"装备属性:"<<nature<<endl;
    cout<<"生命值：+"<<attr.hp<<endl;
    cout<<"物理防御：+"<<attr.def<<endl;
    cout<<"魔法防御：+"<<attr.res<<endl;
    cout<<"耐久度："<<attr.ten<<endl;
}
void BELT::show(){//显示效果
    cout<<"装备名称:"<<name<<endl;
    cout<<"装备属性:"<<nature<<endl;
    cout<<"生命值：+"<<attr.hp<<endl;
    cout<<"物理防御：+"<<attr.def<<endl;
    cout<<"魔法防御：+"<<attr.res<<endl;
    cout<<"耐久度："<<attr.ten<<endl;
}
void SHOES::show(){//显示效果
    cout<<"装备名称:"<<name<<endl;
    cout<<"装备属性:"<<nature<<endl;
    cout<<"移动速度：+"<<attr.spd2<<endl;
    cout<<"物理防御：+"<<attr.def<<endl;
    cout<<"魔法防御：+"<<attr.res<<endl;
    cout<<"耐久度："<<attr.ten<<endl;
}
void WEAPON::show(){//显示效果
    cout<<"装备名称:"<<name<<endl;
    cout<<"装备属性:"<<nature<<endl;
    cout<<"攻击速度：+"<<eattr.spd<<endl;
    cout<<"物理伤害：+"<<eattr.atk<<endl;
    cout<<"魔法伤害：+"<<eattr.atm<<endl;
    cout<<"耐久度："<<eattr.ten<<endl;
}
void DRUG::show(){//显示效果
    cout<<"消耗品:"<<name<<endl;
    cout<<"效果:"<<nature<<endl;
    cout<<"生命值：+"<<eattr.hp<<endl;
}
