#include<iostream>
//8.cpp
using namespace std;

//多继承
//函数结尾加以下关键字
//关键字：
//const：不能改变成员
//override：说明该函数是重写的
//final：后面继承中不能再对该函数重写

//基类A
class A{
    protected:
    int a;
    public:
    //无参构造
    A(){cout<<"A"<<endl;}
    //虚函数
    virtual void show(){
    cout<<"A::show"<<endl;
    }
};
//派生类B，继承A
//class B final : public A
//final 表示一个类不能再被继承
class B : public A{
    protected:
    int b;
    public:
    //无参构造
    B(){cout<<"B"<<endl;}

    //final 在函数后面表示一个函数不能被重写
    //override 说明这个函数是重写的，
    //在派生类中，如果对不是基类中虚函数重写，编译override会报错
    void show() override{
    cout<<"B::show"<<endl;
    }
};
//派生类C，继承B
class C : public B{
    protected:
    int c;
    public:
    //无参构造
    C(){cout<<"C"<<endl;}

    //不重写，则继承的是B::show
    
    //重写show
    void show(){
    cout<<"C::show"<<endl;
    }
    
};
//注意:
//1.const final override
//==const override final
//const 必须写在final或override前
//2.派生也可以作为基类
int main(){
    A *p1=new A;
    p1->show();

    p1=new C;
    p1->show();

    p1=new B;
    p1->show();

    B *p2=new B;
    p2->show();
    return 0;
}
