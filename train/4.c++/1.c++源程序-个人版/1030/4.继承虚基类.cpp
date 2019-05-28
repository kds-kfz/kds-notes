#include<iostream>
//4.cpp
using namespace std;
//派生继承虚基类，在多派生类共同继承基类时，
//只继承基类1次，确保多继承中只保留1份基类成员
class A{
    protected:
    int a;
    public:
    A(){cout<<"A"<<endl;}
    A(int i) : a(i){
	cout<<"A "<<a<<endl;
    }
    //增加1个虚函数表指针，是匿名指针名
    virtual void show(){
	cout<<"show A "<<a<<endl;
    }
};
class B1 : virtual public A{
    protected:
    int b1;
    public:
    B1(){cout<<"B1"<<endl;}
    B1(int i1) : A(10),b1(i1){
	cout<<"B1 "<<a<<" "<<b1<<endl;
    }
    void show(){
	cout<<"show B1 "<<a<<" "<<b1<<endl;
    }
};
//继承虚基类，多一个虚基类指针，记录是否已经发生基类继承
//如果继承有继承基类，后面若遇到再继承基类，就不会再继承
//虚基类指针：构造偏移量，构造函数的位置
class B2 : virtual public A{
    protected:
    int b2;
    public:
    B2(){cout<<"B2"<<endl;}
    B2(int i2) : A(20),b2(i2){
	cout<<"B2 "<<a<<" "<<b2<<endl;
    }
    void show(){
	cout<<"show B2 "<<a<<" "<<b2<<endl;
    }
};
class C : public B1,public B2{
    protected:
    int c;
    public:
    //必须先构造共同基类
    C() : A(),B1(),B2(),c(0){cout<<"C"<<endl;}
    C(int i1,int i2,int i3,int i4) : A(i1),B1(i2),B2(i3),c(i4){
	cout<<a<<" "<<b1<<" "<<b2<<" "<<c<<endl;
    }
    void show(){
	cout<<"show C "<<a<<" "<<b1<<" "<<b2<<" "<<c<<endl;
    }
};
#if 0
A      A
|      |
B1     B2
  \   /
    C
//u型
    A
  /   \
B1     B2
  \   /
    C
菱形继承结构，需要用虚继承避免产生多个共同基类
#endif
int main(){
    A a(1);
    B1 b1(10);
    B2 b2(100);
    C c(1,2,3,4);
    A *p1=&a;
    p1->show();
    p1=&b1;
    p1->show();
    p1=&b2;
    p1->show();
    p1=&c;
    p1->show();
    cout<<"A_size = "<<sizeof(A)<<endl;
    cout<<"B1_size = "<<sizeof(B1)<<endl;
    cout<<"B2_size = "<<sizeof(B2)<<endl;
    cout<<"C_size = "<<sizeof(C)<<endl;
    return 0;
}
