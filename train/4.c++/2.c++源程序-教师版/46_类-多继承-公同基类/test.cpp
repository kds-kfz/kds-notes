#include <iostream>
using namespace std;
class A
{
public:
	int a;
	//pvta  --> [A::f]
	virtual void f() { cout << "f A" << endl; }
};
class B1 : virtual public A
{
public:
	//pvtb1 --> [B1::f|B::show] 虚函数表的指针
	//pcls						虚基类的指针
//	void f() { cout << "f B1 " << endl; }
//	virtual void show() { cout << "show B1" << endl; }
//	int b1;
};
class B2 : virtual public A
{
public:
	//pvtb2 --> [A::f|B2::fun]	虚函数表的指针
	//pcls						虚机类的指针
//	virtual void fun() { cout << "fun B2" << endl; }
//	int b2;
};
class D : public B1, public B2
{
	//a
	//pvtb1 4 --> [B1::f|B1::show]   B1虚函数表的指针
	//--------------------------------
	//pvtb2 4 --> [A::f|B2::fun]	B2虚函数表的指针
	//pcls						虚机类的指针
};

int main()
{
	cout << sizeof(A) << endl;
	cout << sizeof(B1) << endl;
	cout << sizeof(B2) << endl;
	cout << sizeof(D) << endl;
	D d;
	B1* p = &d;
	B2* p2 = &d;
	return 0;
}
