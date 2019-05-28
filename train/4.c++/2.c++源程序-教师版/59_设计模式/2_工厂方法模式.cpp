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
//工厂方法模式:
//每一个工厂只生产单一产品，
//需要有具体的工厂对象生产具体的产品
class Factory 
{
    public:
	virtual Plant* createPlant() = 0;
};
class AppleFactory : public Factory
{
    public:
	virtual Plant* createPlant() {
	    return new Apple;
	}
};
class PearFactory : public Factory
{
    public:
	virtual Plant* createPlant() {
	    return new Pear;
	}
};
class OrangeFactory : public Factory
{
    public:
	virtual Plant* createPlant() {
	    return new Orange;
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

int main()
{
    Factory* pf = nullptr;
    Plant* p = shop(APPLE);
    p->show();


    //	FactoryA fa;
    //	FactoryP pa;
    //	Apple* p = fa.createPlant();
    //	p = pa.cratePlant();

    return 0;
}
