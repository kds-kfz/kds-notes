#include <iostream>
using namespace std;
class A
{
protected:
	int a;
public:
	A() { cout << "A" << endl; }
	A(int i) : a(i) { cout << "A " << a << endl; }
	virtual void show() {
		cout << "show A " << a << endl;
	}
};

class B1 : virtual public A
{
protected:
	int b1;
public:
	B1() { cout << "B1" << endl; }
	B1(int i) : A(10), b1(i) { 
		cout << "B1 " << b1 << " " 
		<< a<< endl; 
	}
	void show() override {
		cout << "show B1 " << a << " " << b1 << endl;
	}
};
class B2 : virtual public A
{
protected:
	int b2;
public:
	B2() { cout << "B2" << endl; }
	B2(int i) : A(20), b2(i) { cout << "B2 " << a 
		<< " " << b2 << endl; }
	void show() override {
		cout << "show B2 " << a << " " << b2 << endl;
	}
};
/*
A		A
|		|
B1		B2
\		/
	D
*/
/*
   菱形继承结构，需要用虚继承避免产生多个共同基类
	   A
	 /	  \
	 B1	  B2	  
	  \	 /
	    D
*/
/*
B1 b1						B2 b2
------					---------
p		---> [B1::show]		p   --> [B2::show]
a							a
b1							b2
*/

class D : public B1, public B2
{
	int d;
public:
	// : B1(), B2() 
	//　必须先构造共同基类
	D() : A(), B1(), B2() { cout << "D" << endl;}
	D(int i1, int i2, int i3, int i4) : 
		A(i1), B1(i2), B2(i3), d(i4)
	{
		cout << "D " << a << " " << b1 << " "
			<< b2 << " " << d << endl;
	}
	void show() {
	//	cout << B1::a << " " << B2::a << " " << 
		cout << "show ";
		cout << a << " " << 	
		b1 << " " << 
			b2 << " " << d 
			<< endl;
	}
};
int main()
{
	D d(1, 2, 3, 4);
	d.show();
	cout << sizeof(A) << endl;
	cout << sizeof(B1) << endl;
	cout << sizeof(B2) << endl;
	cout << sizeof(D) << endl;
	A* p = &d;
	p->show();

	return 0;
}
