#include<iostream>
//9.单例模式.cpp
using namespace std;
#if 0
    //懒汉模式
class Base{
    static Base *instance;
    int num;
    Base() : num(0){}
    public:
    Base(const Base &b)=delete;
    Base &operator=(const Base &b)=delete;
    static Base *createInstance(){
	//线程不安全
	if(instance==NULL){
	    //加锁
	    instance=new Base;
	    //解锁
	}
	return instance;
    }
};
Base *Base::instance=NULL;
int main(){
    Base *b1=Base::createInstance();//第一次建指针，申请一片空间
    Base *b2=Base::createInstance();//第二次建指针，返回第一次申请的空间
    if(b1==b2)
	cout<<"b1==b2"<<endl;
    else
	cout<<"b1!=b2"<<endl;
    return 0;
}
#endif
#if 1
    //饿汉模式
class Base{
    static Base *instance;
    int num;
    Base() : num(0){}
    public:
    Base(const Base &b)=delete;
    Base &operator=(const Base &b)=delete;
    static Base *createInstance(){
	return instance;
    }
    //注意，单例模式纯虚函数的使用，配置必须重写该函数
    //virtual void show()=0;
};

class A : public Base{
    void show();
};
//同时不能为虚函数申请空间，只能定义指针变量，然后改指针变量指向派生类空间
//例如：Base *Base::instance=NULL; instance = new Base();之后才有几倍指针访问派生类
//初始化申请空间 
Base *Base::instance=new Base;
int main(){
    Base *b1=Base::createInstance();//第一次建指针，直接返回初始化空间
    Base *b2=Base::createInstance();//第二次建指针，直接返回初始化空间 
    if(b1==b2)
	cout<<"b1==b2"<<endl;
    else
	cout<<"b1!=b2"<<endl;
    return 0;
}
#endif
