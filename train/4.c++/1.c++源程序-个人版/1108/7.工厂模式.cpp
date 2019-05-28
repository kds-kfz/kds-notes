#include<iostream>
//7.工厂模式.cpp
using namespace std;

//工厂模式
//每一个工长只生产单一产品
//需要具体的工厂对象生产具体的产品
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
    apple=1,pear,orange
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
class Orange : public Plant{
    public:
    Orange() : Plant(){}
    void show(){
	cout<<"这是一个橘子"<<state()<<endl;
    }
    STATE state(){
	return STATE(time/15);
    }
};
class Factory{
    public:
	virtual Plant *createPlant()=0;
};
class AppleFactory : public Factory{
    public:
	virtual Plant *createPlant(){
	    return new Apple;
	}
};
class PearFactory : public Factory{
    public:
	virtual Plant *createPlant(){
	    return new Pear;
	}
};
class OrangeFactory : public Factory{
    public:
	virtual Plant *createPlant(){
	    return new Orange;
	}
};
AppleFactory fa;
PearFactory fp;
OrangeFactory fo;

Plant *shop(PLANT plant){
    Factory *p=NULL;
    switch(plant){
	case apple:p=&fa;break;
	case pear:p=&fp;break;
	case orange:p=&fo;break;
	default :break;
    }
    if(p)
	return p->createPlant();
    else
	return NULL;
}

int main(){
    Plant *p=shop(apple);
    p->show();
    return 0;
}
