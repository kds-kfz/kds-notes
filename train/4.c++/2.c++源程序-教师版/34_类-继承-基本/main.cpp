#include <iostream>
using namespace std;
class Base
{
private:	//本类内部可以访问
protected:	//本类和派生的类可以直接访问
	int a;
	int b;
public:
	void show() {
		cout << "Base " <<  a << " " << b << endl;
	}
};

class Derived : public Base
{
public:
	//如果定义了重名的变量，派生类里同时存在两个a
	//基类的a被隐藏
	int a;
	int c;
	void show() {
		cout << "Derived ";
		cout << Base::a << " " << 
			" " << Derived::a << " " << 
			b << " " << c << endl;
	}
};
/*********************
  内存分布	:
Base b;			Derived d;
-------			---------
int a			int a Base::a	<-	有同名的成员变量，										基类的a会被隐藏
int b			int b
				int a Derived::a <- 使用a就是派生的a
				int c
--------------------------------
 */
int main()
{
	Base b1;
//	b1.a = 2;
//	b1.b = 4;
	b1.show();
	
	Derived d1;	//d1和b1没关系，d1.a b1.a 都是独立的
//	d1.Base::a = 2;
//	d1.b = 9;
//	d1.c = 90;
	d1.Base::show();
	d1.Derived::show();
	cout << sizeof(Base) << " " << sizeof(Derived) << endl;
	return 0;
}
