#include<iostream>
#include<map>
//shop.cpp
using namespace std;
/*-----------------------------------------*/
enum PARTS{//部位
    Hat=1,Shirt,Pants,Belt,Shoes,Weapon,Body,Lowbody,Upbody
};
/*-----------------------------------------*/
enum TYPE{//装备类型
    Clothes=1,Dress,Orbament
};
/*-----------------------------------------*/
/*
enum Goods{
   HAT,SHIRT,PANTS,BELT,SHOES,WEAPON,
   CLOTHES,DRESS,ORBAMENTA
};
*/
struct Attribute{
    int hp;//生命
    int mp;
    float spd;
    int atk;//普攻
    int atm;
    int def;//物抗
    int res;
    Attribute() : Attribute(0,0,0,0,0,0,0){}
    Attribute(int h,int m,float s,int a1,int a2,int d,int r) : 
    hp(h),mp(m),spd(s),atk(a1),atm(a2),def(d),res(r){}
    void show(){
	cout<<"生命\t"<<"魔法\t"<<"攻速\t"
	    <<"普攻\t"<<"魔攻\t"<<"物抗\t"<<"魔抗"<<endl;
	cout<<hp<<"\t"<<mp<<"\t"<<spd<<"\t"
	    <<atk<<"\t"<<atm<<"\t"<<def<<"\t"<<res<<endl;
    }
};
class Actor{//物品
    protected:
    int id;
    PARTS part;
    string name;
    string nature;
    public:
    static int count;
    Actor() : Actor(0,Hat,"物品","--"){}
    Actor(int i,PARTS p,string n1,string n2) : 
	id(count),part(p),name(n1),nature(n2){}
    Actor(const Actor &a)=delete;
    virtual void show()=0;
    string getn1(){return name;};
    string getn2(){return nature;};
    int getid()const{return id;}
};
int Actor::count=1000;
class Prop : public Actor{//道具,消耗品
    public:
    Prop() : Prop(0,Body,"消耗品","--"){}
    Prop(int i,PARTS p,string n1,string n2) : Actor(i,p,n1,n2){}
    Prop(const Prop &p)=delete;
    virtual void show()=0;//显示效果
};
class Equipment : public Actor{//装备
    public:
    Equipment() : Equipment(0,Body,"装备","--"){}
    Equipment(int i,PARTS p,string n1,string n2) : Actor(i,p,n1,n2){}
    Equipment(const Prop &p)=delete;
    virtual void show()=0;//显示效果
};
class HAT : public Equipment{
    Attribute eattr;//装备效果
    public:
    HAT() : HAT("头盔","--",0,0,0){}
    HAT(string n1,string n2,int h,int a,int d) : 
	Equipment(count++,Hat,n1,n2){
	    eattr.hp=h;eattr.atk=a;eattr.def=d;}
//    HAT(string n1,string n2,int h,int m,float s,int a1,int a2,int d,int r) : 
//	Equipment(100,Hat,n1,n2),eattr(h,m,s,a1,a2,d,r){}
    void show(){//显示效果
	cout<<"消耗品:"<<name<<endl;
	cout<<"效果:"<<nature<<endl;
	cout<<"生命值：+"<<eattr.hp<<endl;
	cout<<"物理攻击：+"<<eattr.atk<<endl;
	cout<<"物理防御：+"<<eattr.def<<endl;
    }
    /*
    void showAll(){
	cout<<eattr.hp<<" "
	    <<eattr.mp<<" "
	    <<eattr.spd<<" "
	    <<eattr.atk<<" "
	    <<eattr.atm<<" "
	    <<eattr.def<<" "
	    <<eattr.res<<" "
	    <<endl;
    }
    */
    HAT(const HAT &h)=delete; 
    Attribute getHAT()const{return eattr;}
    string getn1()const{return name;}
    string getn2()const{return nature;}
    int getid()const{return id;}

};
class CLOTHES : public Prop{
    Attribute pattr;//装备效果
    public:
    CLOTHES() : CLOTHES("头盔","--",0,0,0){}
    CLOTHES(string n1,string n2,int m,int a,int r) : 
	Prop(count++,Upbody,n1,n2){
	    pattr.mp=m;pattr.atm=a;pattr.res=r;}
//    CLOTHES(string n1,string n2,int h,int m,float s,int a1,int a2,int d,int r) : 
//	Equipment(200,Upbody,n1,n2),pattr(h,m,s,a1,a2,d,r){}
    void show(){//显示效果
	cout<<"消耗品:"<<name<<endl;
	cout<<"效果:"<<nature<<endl;
	cout<<"魔法值：+"<<pattr.mp<<endl;
	cout<<"魔法攻击：+"<<pattr.atm<<endl;
	cout<<"魔法防御：+"<<pattr.res<<endl;
    }
    CLOTHES(const CLOTHES &h)=delete; 
    Attribute getCLO()const{return pattr;}
    string getn1()const{return name;}
    string getn2()const{return nature;}
    int getid()const{return id;}
};
//抽象工厂
class Factory{
    Attribute pattr;//道具效果
    public:
    virtual Prop *producePro
	(TYPE t,string n1,string n2,int h,int m,float s,int a1,int a2,int d,int r)=0;
    virtual Equipment *produceEqu
	(PARTS p,string n1,string n2,int h,int m,float s,int a1,int a2,int d,int r)=0;
    virtual ~Factory(){}
};
class Factory_All : public Factory{
    public:
    virtual Prop *producePro
	(TYPE t,string n1,string n2,int h,int m,float s,int a1,int a2,int d,int r){
	switch(t){
	    case Clothes:
		return new CLOTHES{n1,n2,m,a2,r};
	    case Dress:break;
	    case Orbament:break;
	    default:return NULL;
	}
    }
    virtual Equipment *produceEqu
	(PARTS p,string n1,string n2,int h,int m,float s,int a1,int a2,int d,int r){
	switch(p){
	    case Hat:return new HAT{n1,n2,h,a1,d};
	    case Shirt:break;
	    case Pants:break;
	    case Belt:break;
	    case Shoes:break;
	    case Weapon:break;
	    case Body:break;
	    default:return NULL;
	}
    }
    ~Factory_All(){}
};
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
class Player{
    int id;
    string name;
    int gold;
    int exp;
    int level;
    int diamond;
    Attribute attr1;
    Attribute attr2;
    public:
    map<int,Actor *>bag;
    Player() : Player(1001,"马叉虫",2000,0,1,0){}
    Player(int i,string n,int g,int e,int l,int d) : 
	id(i),name(n),gold(g),exp(e),level(l),diamond(d){}
    Player(const Player &p)=delete;
    //使用装备
    void useEqu(const HAT &h){
	cout<<"配备了:"<<h.getn1()<<" 效果:"<<h.getn2()<<endl;
	addAttr(attr2,h.getHAT());
    }
    //移除装备
    void nonuseEqu(const HAT &h){
	cout<<"移除了:"<<h.getn1()<<" 效果减少:"<<h.getn2()<<endl;
	reduceAttr(attr2,h.getHAT());
    }
    //添加到背包
    void addEqu(const Equipment &eq){}
    //使用消耗品
    void usePro(const CLOTHES &c){
	cout<<"配备了:"<<c.getn1()<<" 效果:"<<c.getn2()<<endl;
	addAttr(attr2,c.getCLO());
    }
    //显示状态
    void showAttr(){
	cout<<"id号\t"<<"名字\t"<<"金币\t"<<"经验\t"<<"等级\t"<<"钻石\t"<<endl;
	cout<<id<<"\t"<<name<<"\t"<<gold<<"\t"
	    <<exp<<"\t"<<level<<"\t"<<diamond<<endl;
	cout<<"原始状态:"<<endl;
	attr1.show();
	cout<<"装备状态:"<<endl;
	attr2.show();
    }
};
class Game{
    public:
    int run(){
	
	return 0;
    }
    void buy_goods(){
	cout<<"请输入道具id:"<<endl;
	int id=0;
	cin>>id;    
    }
    void show_goods(){
	map<Actor *,pair<int,pair<int,int> > >goods;//商品，等级，价格
	Factory_All ap;
	Equipment *p1=ap.produceEqu(Hat,"白银头盔1","增加物抗",1,2,3,4,5,6,7);
	Equipment *p2=ap.produceEqu(Hat,"白银头盔2","增加物抗",1,2,3,4,5,6,7);
	Prop *p3=ap.producePro(Clothes,"白银铠甲3","增加体力",1,2,3,4,5,6,7);
	Prop *p4=ap.producePro(Clothes,"白银铠甲4","增加体力",1,2,3,4,5,6,7);
	goods.insert({p1,{3,{99,20}}});
	goods.insert({p2,{5,{99,20}}});
	goods.insert({p3,{7,{99,20}}});
	goods.insert({p4,{9,{99,20}}});
	cout<<"\t\t---道具商城---"<<endl;
	cout<<"道具id\t"<<"道具名称\t"<<"等级\t"<<"金币\t"<<"钻石"<<endl;
	for(auto m:goods){
	    cout<<m.first->getid()<<"\t"<<m.first->getn1()<<"\t"
	    <<m.second.first<<"\t"<<m.second.second.first<<"\t"
	    <<m.second.second.second<<endl;
	}
    }
};
int main(){
#if 0
//    HAT hat;
    HAT hat("黄金头盔","物理属性全加成",50,100,150);
    hat.show();
    CLOTHES clothes("黄金铠甲","魔法属性全加成",60,110,160);
    clothes.show();
    cout<<"------------------------------------------------------"<<endl;
    Player pl;
    pl.useEqu(hat);
    pl.usePro(clothes);
    pl.showAttr();
    cout<<"------------------------------------------------------"<<endl;
    pl.nonuseEqu(hat);
    pl.showAttr();
    cout<<"------------------------------------------------------"<<endl;
    Factory_All ap;
    Equipment *p1=ap.produceEqu(Hat,"白银头盔","增加物抗",1,2,3,4,5,6,7);
    Prop *p2=ap.producePro(Clothes,"白银铠甲","增加体力",1,2,3,4,5,6,7);
    Prop *p3=ap.producePro(Clothes,"白银铠甲","增加体力",1,2,3,4,5,6,7);
    pl.bag.insert({1,p1});
    pl.bag.insert({2,p2});
    pl.bag.insert({3,p3});
    for(auto m:pl.bag){
	cout<<m.first<<" "; 
	m.second->show();
    }
    for(auto m:pl.bag){
	cout<<"delete "<<m.second->getn1()<<endl;
	delete m.second;
    }
#endif
    Game g;
    g.show_goods();
    return 0;
}
