#include<iostream>
//1.cpp
using namespace std;

//委托构造函数
class Stu{
    int id;
    string name;
    int age;
    public:
    //构造
    /*
    Stu(){
	cout<<"无参"<<endl;
	id=0;
	age=0;
    }*/
    //委托构造函数
    Stu():Stu(0,"",0){
	cout<<"无参,委托构造函数初始化"<<endl;
    }
#if 0
    Stu(int i,const string n,int a){
	cout<<"直接赋值"<<endl;
	cout<<"id :"<<id<<endl;//随机值
	id=i;
	name=n;
	age=a;
    }
#endif
    //列表初始化,数组不能使用
    //初始化顺序是一定的顺序
    //只能用在拷贝构造函数或构造函数
    Stu(int i,const string n,int a) : id(i),name(n),age(a){
	cout<<"直接赋值,列表初始化"<<endl;
    }
    //拷贝构造
    /*
    Stu(const Stu &c){//既可以引用左值也可以引用右值
	cout<<"拷贝构造"<<endl;
	id=c.id;
	name=c.name;
	age=c.age;
    }*/
    Stu(const Stu &c) : id(c.id),name(c.name),age(c.age){
	cout<<"拷贝构造,列表初始化"<<endl;
    }
    //显示
    void show(){
	cout<<"id :"<<id<<endl;
	cout<<"name :"<<name<<endl;
	cout<<"age :"<<age<<endl;
    }
};

int main(){
    Stu d1();
    Stu d2(1001,"lisi",24);
    Stu d3(d2);
//    Stu d4(1004);//错误

    return 0;
}
