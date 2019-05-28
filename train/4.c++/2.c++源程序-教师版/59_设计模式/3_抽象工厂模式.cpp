#include <iostream>
using namespace std;
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
    ORANGE
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

class Orange : public Plant
{
    public:
	Orange() : Plant() { }
	void show() { 
	    cout << "橘子 : " << state() << endl;
	}
	STATE state() {	
	    return STATE(time / 15);
	}
};
class Juice 
{
    string brand;//商标
    int ml;
    string bottle;
    public:
    virtual void show() = 0;
    virtual ~Juice() { }
};
class AppleJuice : public Juice
{
    public:
	void show() { cout << "苹果汁" << endl; }
};
class PearJuice : public Juice
{
    public:
	void show() { cout << "冰糖雪梨" << endl; }
};
class OrangeJuice : public Juice
{
    public:
	void show() { cout << "橙汁" << endl; }
};
//抽象工厂方法模式:
//每一个工厂可以生成多种产品，
//需要有具体的工厂对象生产具体的产品
class Factory 
{
    public:
	virtual Plant* createPlant() = 0;
	virtual Juice* createJuice() = 0;
};
class AppleFactory : public Factory
{
    public:
	virtual Plant* createPlant() {
	    return new Apple;
	}
	Juice* createJuice() {
	    return new AppleJuice;
	}
};
class PearFactory : public Factory
{
    public:
	virtual Plant* createPlant() {
	    return new Pear;
	}
	Juice* createJuice() {
	    return new PearJuice;
	}
};
class OrangeFactory : public Factory
{
    public:
	virtual Plant* createPlant() {
	    return new Orange;
	}
	Juice* createJuice() {
	    return new OrangeJuice;
	}
};


/*
//简单工厂模式
Plant* factory(PLANT p) {
switch(p) {
case APPLE:
return new Apple;
case PEAR:
return new Pear;
default:
return nullptr;
}
}
 */

AppleFactory fa;
PearFactory fp;
OrangeFactory fo;
/*
   Plant* shop(PLANT plant)
   {
   Factory* p = nullptr;
   switch(plant) {
   case APPLE :
   p = &fa;
   break;
   case PEAR :
   p = &fp;
   break;
   case ORANGE:
   p = &fo;
   break;
   default:
   break;
   }
   if (p)
   return p->createPlant();
   else
   return nullptr;
   }
 */
int main()
{
    //	Factory* pf = nullptr;

    Juice* aj = fa.createJuice();
    aj->show();
    //	FactoryA fa;
    //	FactoryP pa;
    //	Apple* p = fa.createPlant();
    //	p = pa.cratePlant();
    delete aj;
    return 0;
}
