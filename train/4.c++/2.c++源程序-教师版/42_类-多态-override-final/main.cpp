#include <iostream>
using namespace std;

class Base
{
protected:		--> [Base::show]
	int b;
public:			
	Base() { cout << "Base" << endl;}
	virtual void show() { 
		cout << "Base::show" << endl;
	}
};
//final 表示一个类不能再被继承
class D /*final*/ : public Base
{
protected://	---> [D::show]
	int d;
public:
	D() { cout << "D" << endl;}
	//final 在函数后面表示一个函数不能被重写
	void show() final override {//这个函数是重写的, 如果不是重写，编译override会报错
		cout << "D::show" << endl;
	}
};

class E : public D
{		
		//	---> [D::show]
	int e;
public:
	E() { cout << "E" << endl;}
/*	
	void show() {
		cout << "E::show" << endl;
	}
*/	
};


int main()
{
//	E e;
	Base* p = new E;
	p->show();
	p = new D;
//	p->show(21);
	D* p2 = new D;
	p2->show();
	return 0;
}
