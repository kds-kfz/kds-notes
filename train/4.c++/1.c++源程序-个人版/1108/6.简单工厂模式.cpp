#include<iostream>
#include<vector>
//6.简单工厂模式.cpp
using namespace std;

//设计模式
//简单工厂模式
enum STATE{
//  种子    芽	    苗	  开花	  结果	 成熟
    seed,sprout,plantlet,bloom,fruitage,grown
};
class Plant{
    protected:
    int price;
    int time;
    int level;
    public:
    Plant(): price(0),time(0),level(0){}
    virtual void show()=0;
    virtual STATE state()=0;
    virtual ~Plant(){
	cout<<"~Plant"<<endl;
    }
};
enum PLANT{
    apple=1,pear
};
class Apple :public Plant{

    public:
    Apple() : Plant(){}
    void show(){
	cout<<"这是一个苹果"<<state()<<endl;
    }
    STATE state(){
	return STATE(time/100);
    }
};
class Pear : public Plant{
    public:
    Pear() : Plant(){}
    void show(){
	cout<<"这是一个梨子"<<state()<<endl;
    }
    STATE state(){
	return STATE(time/15);
    }
};
//简单工厂模式
Plant *shop(PLANT p){
    switch(p){
	case apple:
	    return new Apple;
	case pear:
	    return new Pear;
	default:return NULL;
    }
}
class Player{
    int id;
    string name;
    int gold;
    int level;
    int exp;
    public:
    Player():id(1001),name("玩家"),
	    gold(200),level(1),exp(0){}
    const string &Getname()const{return name;}
    int Getgold()const{return gold;}
    int Getlevel()const{return level;}
    int Getexp()const{return exp;}
};
class Game{
    struct Block{
	Plant *p;
	int state;//土地状态,1可种植,0不可种植
	Block() : p(NULL),state(0){}
    };
    Player pl;
//    vector<Plant *>land;
    Block land[16];
    //map<Plant *,int>bag;
    vector<Plant *>bag;//可以装任何类型道具的背包
    public:
    Game(){
	for(int i=0;i<4;i++)
	    land[i].state=1;
    }
    ~Game(){
	for(int i=0;i<16;i++){
	    cout<<"delete "<<i<<endl;
	    if(land[i].p!=NULL)
		delete land[i].p;
	}
	for(auto m:bag){
	    cout<<"delete";
	    m->show();
	    delete m;
	}
    }
    void showPlayer(){
	cout<<"\033[1;1H\033[32m"<<flush;
	cout<<pl.Getname()
	    <<" 金币:"<<pl.Getgold()
	    <<" 等级:"<<pl.Getlevel()
	    <<" 经验:"<<pl.Getexp()
	    <<"\033[0m"<<endl;
    }
    int run(){
	string s;
	int n;
	Plant *p;
	while(1){
	    system("clear");
	    //显示玩家信息
	    showPlayer();
	    //显示土地信息
	    showland();
	    cout<<"1.商店"<<endl;
	    cout<<"2.查看背包"<<endl;
	    cout<<"3.加工长"<<endl;
	    cout<<"4.种植"<<endl;
	    cout<<"0.退出"<<endl;

	    cin>>n;
	    switch(n){
		case 1:showshop();
		       cin>>n;
		       //lan.push_back(shop(n));
		       p=shop(PLANT(n));
		       if(p)
			   bag.push_back(p);
		       else 
			   cout<<"输入错误"<<endl;
		       break;
		case 2:showbag();
		       getchar();
		       getchar();
		       cout<<"输入回车继续"<<endl;
		       break;
		case 0:cout<<"game over"<<endl;
		       return 0;
		default:break;
	    }
	    //sleep(1);
	}
    }
    void showshop(){
	cout<<"1.苹果种子"<<endl;
	cout<<"2.梨子种子"<<endl;
	cout<<"3.西瓜种子"<<endl;
	cout<<"4.白菜种子"<<endl;
	cout<<"5.牧草种子"<<endl;
    }
    void showbag(){
	cout<<"---------------"<<endl;
	for(auto m:bag)
	    m->show();
	cout<<"---------------"<<endl;

    }
    Plant *shop(PLANT p){
    switch(p){
	case apple:return new Apple;
	case pear:return new Pear;
	default:return NULL;
    }
    }
    void showland(){
	for(int i=0;i<16;i++){
	    cout<<land[i].state<<" ";
	if((i+1)%4==0)
	    cout<<endl;
	}
    }
};
int main(){
#if 0
    vector<Plant *>land;
    Plant *p=show(pear);
    lan.push_back(p);
    p=show(apple);
    lan.push_back(p1);
    
    /*
    Aplle *p1=new Apple;
    lan.push_back(p1);
    Pear *p2=new Pear;
    land.push_back(p2);
    */
    for(auto m:land)
	m->show();
    for(auto m:land)
	delete m;
#endif
    Game g;
    g.run();
    return 0;
}
