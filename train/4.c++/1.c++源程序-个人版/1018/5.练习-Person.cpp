#include<iostream>
#include<cmath>
//5.cpp
using namespace std;

//类访问权限
//class：关键字定义的类，默认权限全部是私有的
//struct：默认权限全部是共有的
class Person{
//    protected://受保护的，本类内和它的派生类可以访问
	      //外部不可访问
//    private://只有在类内可以直接访问
    int id;
    string name,sex;
    public://类内和类外都能访问
    void init(Person *p,int id,string name,string sex){
	p->id=id;
	p->name=name;
	p->sex=sex;
    }
    void init(int id,string name,string sex){
	this->id=id;
	this->name=name;
	this->sex=sex;
    }
    void show(){
	cout<<"id :"<<id<<endl;
	cout<<"name :"<<name<<endl;
	cout<<"sex :"<<sex<<endl;
    }
    void set_name(string name){
	this->name=name;
    }
}; 
int main(){
#if 1
    Person d1;
    Person d2;

    d1.init(&d2,1002,"lisi","男");
    d2.show();
    cout<<endl;

    d1.init(1001,"lisi","男");
    d1.show();
    cout<<endl;

    d1.set_name("pipi");
    d1.show();
#endif
}
