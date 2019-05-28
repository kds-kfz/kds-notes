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
};
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
