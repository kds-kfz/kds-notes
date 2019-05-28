#include<iostream>
//2.cpp
//类内的static静态变量

using namespace std;
//int num=1000;//全局变量

class Person{
    int id;
    string name;
    int age;
    public:
    //静态成员存在数据区,生存期很长
    //静态成员变量，整个类只有1个变量
    //静态成员变量,对象都可以访问
    static int count;
    Person(string n="--",int a=0):id(count++),name(n),age(a){}
    friend ostream &operator<<(ostream &out,const Person p);
    //静态函数没有this指针,不能访问类的普通成员变量
    //能访问静态成员变量
    /*
    static int get(){//错误
	return id;
    }
    */
    static int get(){//正确
	return count;
    }
    //同一个函数不能有静态和非静态两种版本
    /*
    int get(){//错误
	return id;
    }
    */
    int getid(){
	return id;
    }
};

//对静态成员初始化，
//必须在类外对静态成员进行全局的初始化
int Person::count=1000;//初始化1次
//Person::count=2000;//错误，不能在全局给变量赋值 
/*
栈区
p1	p2
id	id
name	name
age	age
数据区
count
*/

ostream &operator<<(ostream &out,const Person p){
    out<<p.id<<" "<<p.name<<" "<<p.age<<endl;    
}

int main(){
    Person::count=2000;//赋值

    Person p1("lisi",23);//1001
    Person p2("lisi",24);//1002
    cout<<p1;
    cout<<p2;
    cout<<"-----------------"<<endl;
    cout<<sizeof(p1)<<endl;
    cout<<sizeof(p2)<<endl;
    cout<<"-----------------"<<endl;
    cout<<Person::count<<endl;
    cout<<"-----------------"<<endl;
    cout<<p1.get()<<endl;
    cout<<p2.getid()<<endl;
    cout<<"-----------------"<<endl;
    cout<<p1.count<<endl;
    cout<<p2.count<<endl;
    return 0;
}

