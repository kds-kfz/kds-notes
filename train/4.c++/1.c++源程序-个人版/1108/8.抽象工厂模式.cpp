#include<iostream>
//抽象工厂模式.cpp
using namespace std;

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
class Juice{
    string brand;//商标
    int ml;
    public:
    virtual void show()=0;
    virtual ~Juice(){}
};
class AppleJuice : public Juice{
    public:
	void show(){cout<<"苹果汁"<<endl;}
};
class PearJuice : public Juice{
    public:
	void show(){cout<<"冰糖雪梨"<<endl;}
};
class OrangeJuice : public Juice{
    public:
	void show(){cout<<"橙汁"<<endl;}
};
//抽象工厂方法模式
//每个工厂可以生产多种产品
//需要有具体的工厂对象生产具体的产品

class Factory{
    public:
	virtual Plant *createPlant()=0;
	virtual Juice *createJuice()=0;

};
class AppleFactory : public Factory{
    public:
	virtual Plant *createPlant(){
	    return new Apple;
	}
	virtual Juice *createJuice(){
	    return new AppleJuice;
	}
};
class PearFactory : public Factory{
    public:
	virtual Plant *createPlant(){
	    return new Pear;
	}
	virtual Juice *createJuice(){
	    return new PearJuice;
	}
};
class OrangeFactory : public Factory{
    public:
	virtual Plant *createPlant(){
	    return new Orange;
	}
	virtual Juice *createJuice(){
	    return new OrangeJuice;
	}
};
AppleFactory fa;
PearFactory fp;
OrangeFactory fo;
int main(){
    Juice *aj=fa.createJuice();
    aj->show();
    delete aj;
    return 0;
}
