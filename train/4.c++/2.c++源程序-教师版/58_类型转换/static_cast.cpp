#include <iostream>

using namespace std;


// _cast
//static_cast const_cast dynamic_cast reinterpret_cast
//数据			const		继承的类	底层
//不能转const
//不能转非继承关系的类指针

//当基类指针转成派生指针时
//不能转虚继承类的指针

struct Base
{
	int n;
	virtual void show() { cout << "show B" << endl;}
};
struct Derived : virtual public Base
{
	int m;
	virtual void show() { cout << "show D" << endl;}
};

struct A
{
	int w;
};
int main()
{
	//static_cast	只转换数据类型, 不转换const属性
	int a = 10;
	const int* p = &a;
	//double d = (double)a; 可以转换任意基本数据类型
	//double d = double(a);	可以转换任意指针
	int* p2 = (int*)p;//	可以转换const属性
	*p2 = 32;
	cout << a << endl;
	double d = static_cast<double>(a);
	cout << d << endl;
	//static_cast不能转换const属性
//	int* p3 = static_cast<int*>(p);	错误
//	*p3 = 109;
	cout << a << endl;
	Derived* pd = new Derived;
	pd->n = 200;
	pd->m = 900;
	
	Base* pb = new Base;	
	pb->n = 10;
	
	//pd = (Derived*)pb; 派生指针指向基类对象，能强制类型转换，但是会有危险
//	pd = static_cast<Derived*>(pb);
	pb = static_cast<Base*>(pd);
//	A* pa = new A;
//	pa->w = 333;
	pb->show();

	//pd = (Derived*)pa;
//	pd = static_cast<Derived*>(pa);
	//能转换有继承关系的指针，但是不考虑派生和基类的关系, 如果没有继承关系的不能转换
	
	cout << pd->n << endl;
	cout << pd->m << endl;
	

//	Derived* de = (Derived)b;
	

	return 0;
}
