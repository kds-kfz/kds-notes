#include <iostream>
using namespace std;
class B1 {
protected:
	int b1;
	int d;
public:
	B1(int i1=0, int i2=0) : b1(i1), d(i2) {
		cout << "B1 " << endl;
	}
	void show() {
		cout << "show B1 " << b1 << endl;
	}
};
class B2 {
protected:
	int b2;
	int d;
public:
	B2(int i1=0, int i2=0) : b2(i1), d(i2) {
		cout << "B2 " << endl;
	}
	void show() {
		cout << "B2 " << b2 << endl;
	}
};
class D : public B1, public B2
{
	int d;
public:
	//初始化的顺序是继承的顺序和变量定义的顺序
	D(int i1, int i2, int i3, int i4, int i5) :
		B1(i1, i2), B2(i3, i4), d(i5) { 
		cout << "D" << endl;
	}
	
	void show() {
		cout << b1 << " " << b2 << " " << d << endl;
		cout << B1::d << " " << B2::d << endl;
	}
	
};
/*
B1 b1		B2 b2		D d
-------		-------		-------
b1			b2			b1
d			d			d	--> B1
						====
						b2
						d	--> B2
						====
						d	--> D
*/						
int main()
{
	B1 b1;
	B2 b2;
	D d(1, 2, 3, 4, 5);

	d.B1::show();

	cout << sizeof(d) << endl;
	return 0;
}
