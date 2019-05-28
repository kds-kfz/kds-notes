#include<iostream>
#include<map>
//main.cpp
/*******************************
  项目二：游戏装备
  时间：2017/11/9
  地点：虚拟大学A103
  进度：
  问题：
  解决：
 *******************************/
using namespace std;
/*----------------------------------------------------*/
enum Part{//装备部位
    //帽子，衬衣，裤子，腰带，鞋子，武器
    Hat=1,Shirt,Pants,Belt,Shoes,Weapon};

enum Type{//装备类型
    Clothes=1,//普通的装备
    Dress,//时装
    Orbament};//首饰
/*----------------------------------------------------*/
//玩家状态
struct Attribute{
    int hp;	//玩家生命值
    int mp;	//魔法值
    string spd;	//攻击速度
    int atk;	//普通攻击
    int atm;	//魔法攻击
    int def;	//物理防御
    int res;	//魔法防御
    public:
    //无参构造
    Attribute() : Attribute(500,300,"0.65/s",75,40,20,20){}
    //有参
    Attribute(int h,int m,string s,int a1,int a2,int d,int r) : 
	hp(h),mp(m),spd(s),atk(a1),atm(a2),def(d),res(r){}
    void show(){
	cout<<"生命\t"<<"魔法\t"<<"攻速\t"
	    <<"普攻\t"<<"魔攻\t"<<"物抗\t"<<"魔抗"<<endl;
	cout<<hp<<"\t"<<mp<<"\t"<<spd<<"\t"
	    <<atk<<"\t"<<atm<<"\t"<<def<<"\t"<<res<<endl;
    }
};
/*----------------------------------------------------*/
class Actor{//物品
    public:
	//显示物品属性效果，需要重写
	virtual void show()=0;
};
class Equipment : public Actor{//装备
//    int id;
    int count;
    string name;
    string nature;
    public:
    PANTS() : PANTS(0,"--","--"){}
    PANTS(int c,string n1,string n2) : count(c),name(n1),nature(n2){}
    //    int getid(){return id;}
    int getcount(){return count;}
    string getname(){return name;}
    string getnature(){return nature;}
    //~PANTS(){cout<<"~PANTS"<<endl;}
	//    Attribute attr;//状态
};
/*----------------------------------------------------*/
/*
class PANTS{
    //    int id;
    int count;
    string name;
    string nature;
    public:
    PANTS() : PANTS(0,"--","--"){}
    PANTS(int c,string n1,string n2) : count(c),name(n1),nature(n2){}
    //    int getid(){return id;}
    int getcount(){return count;}
    string getname(){return name;}
    string getnature(){return nature;}
    //~PANTS(){cout<<"~PANTS"<<endl;}
};
*/
/*----------------------------------------------------*/
//玩家实例
class Player{
    private:	    //私有成员变量
	int id;	    //玩家id
	string name;    //玩家名字
	int gold;	    //玩家金币
	int exp;	    //玩家经验
	int level;	    //玩家等级
	int diamond;    //玩家钻石
	Attribute attr1;//玩家自身状态1,随着等级提高
	Attribute attr2;//玩家自身状态2,所有装备加成状态 
    public:
	/*----------------------------------------------------*/
	class Prop : public Actor{//道具
	    public:
		//玩家更新道具函数，存在背包里
		virtual void changeState(Player &pl){}
	};
	class Blood : public Prop{//血量
	    public:
		//玩家吃药剂，增加血量
		void changeState(Player &pl){}
	};
	Player() : id(1001),name("马叉虫"),gold(1000),exp(0),
	level(1),diamond(0){}
	/*----------------------------------------------------*/
	//玩家换装备函数，包括着装，时装，首饰
	void changeEqu(const Equipment &eq_id){}
	//玩家移除装备函数
	void removeEqu(const Equipment &eq_id){}
	//玩家增加装备函数
	void addEqu(const Equipment &eq_id){}
	//换装备后，更新状态函数
	void useProp(const Prop &pr){}
	//显示自己状态
	void showattr(){
	    //显示原有状态+有使用装备后的状态
	}
	void show(){
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
    Player pl;
    map<int,PANTS>bag;
    public:
    void showbag(){
	cout<<"id号\t"<<"数量\t"<<"名称\t\t"<<"属性\t"<<endl;
	for(auto m:bag)
	    cout<<m.first<<"\t"<<m.second.getcount()<<"\t"
		<<m.second.getname()<<"\t"<<m.second.getnature()<<endl;
    }
    bool seekeq(int id){
	map<int,PANTS>::iterator it=bag.find(id);
	if(it!=bag.end())
	    return true;
	return false;
    }

    void putquip(){
	cout<<"配备装备界面"<<endl;
	cout<<"请输入装备id号:";
	int id=0;
	cin>>id;
	if(seekeq(id))
	    cout<<"配备成功"<<endl;
	else
	    cout<<"配备失败"<<endl;
    }
    void showequip(){
	cout<<"装备界面"<<endl;
	showbag();
	cout<<"1.配备装备"<<endl;
	cout<<"2.卸下装备"<<endl;
	cout<<"3.退出"<<endl;
	cout<<"请输入您的选择:"<<endl;
	char choose=0;
	while(1){
	    cin>>choose;
	    switch(choose){
		case '1':	
		case '2':
		case '3':return;
		default :continue;
	    }
	}
	break;

    }
    void run(){
	system("clear");
	bag.insert({101,{1,"黄金头盔","增加30物抗"}});
	bag.insert({102,{1,"上古兵刃","增加50物攻"}});
	cout<<"正在游戏..."<<endl;
	cout<<"1.玩家信息"<<endl;
	cout<<"2.查看背包"<<endl;
	cout<<"3.商  店"<<endl;
	cout<<"4.退  出"<<endl;
	cout<<"请输入您的选择:"<<endl;
	char  choose=0;
	while(1){
	    cin>>choose;
	    switch(choose){
		case '1':pl.show();break;
		case '2':
		case '3':break;
		case '4':return;
		default:break;
	    }
	}
    }
};
int main(){
    Game g;
    g.run();
    return 0;
}
