#include<iostream>
//1.cpp
using namespace std;

//构造函数
class Person{
    int id;
    string name;
    int age;
    public:
    //构造函数，与类名完全相同，没有返回值，
    //用来初始化对象，在定义对象的时候被调用
    Person(int i,string n,int a){
	id=i;
	name=n;
	age=a;
    }
    //不写任何构造函数情况下，会产生默认的无参构造函数
    Person(){//无参构造函数
	id=0;
	age=0;
    }
    void show(){
	cout<<id<<" "<<name<<" "<<age<<endl;
    }
    void init(int i,string n,int a){
	id=i;
	name=n;
	age=a;
    }
};

int main(){
//    Person d1={1001,"lisi",12};//c++11,列表初始化
    Person d1(1001,"lisi",13);//调用构造函数
    d1.show();
//    Person d2();//错误,声明一个d2()函数，但是没有d2()函数

    Person d2;//调用默认的无参构造函数，Person()
    //如果存在无参构造函数，调用默认的无参构造函数，Person()
    //如果没有无参构造函数，则定义了一个对象，没有初始化
    d2.show();
    cout<<endl;

    Person *p1=new Person;
//等价于  Person *p1=new Person();
    cout<<"*p1 :\n";
    p1->show();
    cout<<endl;

    Person p2=Person();//擦汗骚男横一个匿名对象
    cout<<"p2 :\n";
    p2.show();
//    p2.init(1002,"mao",25);//调用普通函数
    return 0;
}
