#include<iostream>
//2.cpp
//继承---基本

using namespace std;
class Base{
    private://本类内部可以访问

    protected://本类和派生类可以直接访问
	int a;
	int b;
    public:
	void show(){
	    cout<<"Base"<<endl;
	    cout<<a<<" "<<b<<endl;
	}
};
//Derived是Base的继承，派生
class Derived : public Base{
    public:
    int a;
    int c;
    //如果定义里重名的变量，派生类同时存在了两个a
    //基类的a被隐藏
    void show(){
        Base::a=10;
	Base::b=20;
	cout<<"Derived----------------"<<endl;
	//默认打印派生的成员
	cout<<a<<" "<<b<<endl;
	cout<<Base::a<<" "<<Derived::a<<endl;

    }
};
/********************
内存分布
Base a1		    Derived a2
int a		    int a;//Base::a有同名的成员变量，基类的a会被隐藏
int b		    int b;//Base::b
--------------------------------------------------------
		    int a;//Derived::a使用a就是派生的a
		    int c;
  ********************/
int main(){
    Base a1;
    a1.show();
    //    a1::Derived::show();

    Derived a2;//a2与a1没关系，a2.a,a2.b与a1.a,a1.b都是独立的
    a2.show();
    a2.Base::show();

    cout<<"Base_size ="<<sizeof(Base)<<endl;
    cout<<"Derived_size ="<<sizeof(Derived)<<endl;
    return 0;
}

