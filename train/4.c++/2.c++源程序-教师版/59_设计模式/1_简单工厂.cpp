#include <iostream>
#include <vector>
using namespace std;

//简单工厂模式
enum STATE
{
    SEED,		//种子
    SPROUT,		//芽
    PLANTLET,	//小苗
    BLOOM,		//开花
    FRUITAGE,	//结果
    GROWN,		//成熟
};
enum PLANT
{
    APPLE = 1,
    PEAR,
};
class Plant
{
    protected:
	int price;
	int time;
	int level;
    public:
	Plant() : price(0), time(0), level(0) {
	}
	virtual ~Plant() { cout << "~Plant" << endl; }
	virtual void show() = 0;
	virtual STATE state() = 0;
};
class Apple : public Plant
{
    public:
	Apple() : Plant() { }
	void show() { 
	    cout << "苹果 : " << state() << endl;
	}
	STATE state() {	
	    return STATE(time / 10);
	}
};
class Pear : public Plant
{
    public:
	Pear() : Plant() { }
	void show() { 
	    cout << "梨 : " << state() << endl;
	}
	STATE state() {	
	    return STATE(time / 15);
	}
};

//简单工厂模式
Plant* shop(PLANT p) {
    /*
       cout << "1, 苹果种子" << endl;
       cout << "2, 梨种子" << endl;
       cout << "3, 白菜种子" << endl;
       cout << "4, 西瓜种子" << endl;
       int n;
       cin >> n;
     */
    switch(p) {
	case APPLE:
	    return new Apple;
	case PEAR:
	    return new Pear;
	default:
	    return NULL;
    }
}
class Player
{
    int id;
    string name;
    int gold;
    int level;
    int exp;
    public:
    Player() : 
	id(1001), name("玩家"), 
	gold(200), level(1), exp(0) 
    {
    }
    const string& getName() const { return name; }
    int getGold() const { return gold; }
    int getLevel() const { return level; }
    int getExp() const { return exp; }
};

class Game
{
    struct Block
    {
	Plant* p;
	int state; //土地状态 0不可种植，1是可种植
	Block() : p(NULL), state(0) {
	}
    };
    Player pl;
    //	vector<Plant*> land;
    Block land[16];
    //	map<Plant*, int> bag;
    vector<Plant*> bag;	//可以装任何类型道具的背包
    public:
    Game() {
	for(int i=0; i<4; i++)
	    land[i].state = 1;
    }
    ~Game() {
	for (int i=0; i<16; i++) {
	    cout << "delete " << i << endl;
	    if (land[i].p != nullptr)
		delete land[i].p;
	}
	for (auto m : bag) {
	    cout << "delete ";
	    m->show();
	    delete m;
	}
    }
    void showPlayer() {
	//把光标定位到 y,x 的位置
	// cout << "\033[" << y << ";" << x << "H" << flush;
	//		   "\033[前景色号;背景色号m"
	//				31~37		41~47		
	cout << "\033[1;1H\033[31m  " << flush;
	cout << pl.getName() 
	    << " 金币:" << pl.getGold() 
	    << " 等级:" << pl.getLevel() 
	    << " 经验:" << pl.getExp() 
	    << "\033[0m" << endl;
    }
    int run() {
	string s;
	int n;
	Plant* p;
	while(1) {
	    system("clear");
	    //显示玩家信息
	    showPlayer();
	    //显示土地信息
	    showLand();
	    cout << "1, 商店 " << endl;
	    cout << "2, 加工厂 " << endl;
	    cout << "3, 查看背包 " << endl;
	    cout << "4, 种植" << endl;
	    cout << "0, 退出" << endl;
	    cin >> n;
	    switch(n) {
		case 1: 
		    showShop();
		    cin >> n;
		    p = shop(PLANT(n));
		    if (p)
			bag.push_back(p);
		    else 
			cout << "输入错误" << endl;
		    break;
		case 3:
		    showBag();
		    getchar();
		    getchar();
		    cout << "按enter继续" << endl;
		    break;
		case 0:
		    cout << "Byebye!" << endl;
		    return 0;
		default:
		    break;
	    }
	    //sleep(1);
	}
    }
    void showShop() {
	cout << "1, 苹果种子" << endl;
	cout << "2, 梨种子" << endl;
	cout << "3, 白菜种子" << endl;
	cout << "4, 西瓜种子" << endl;
    }
    void showBag() {
	cout << "------------" << endl;
	for (auto m : bag)
	    m->show();
	cout << "------------" << endl;
    }
    Plant* shop(PLANT p) {
	switch(p) {
	    case APPLE:
		return new Apple;
	    case PEAR:
		return new Pear;
	    default:
		return nullptr;
	}
    }
    void showLand() {
	for (int i=0; i<16; i++) {
	    cout << land[i].state << " ";
	    if ((i+1) % 4 == 0)
		cout << endl;
	}
    }
    //	vector<Prop*> bag;	//可以装任何类型道具的背包
};

int main()
{
    Game g;	
    g.run();
    /*
       Plant* p = shop(PEAR);
       land.push_back(p);
       p = shop(APPLE);
       land.push_back(p);
     */
    /*	
	Apple* p1 = new Apple;
	land.push_back(p1);
	Pear* p2 = new Pear;
	land.push_back(p2);

	for (auto m : land) {
	m->show();
	}

	for (auto m : land) {
	delete m;
	}
     */	
    return 0;
}

