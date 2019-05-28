#include <iostream>

using namespace std;


// _cast
//static_cast const_cast dynamic_cast reinterpret_cast
//数据			const		继承的类	底层
//不能转const				安全的转换继承的类指针
//不能转非继承关系的类指针	必须有虚函数

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
	Derived* pd = new Derived;
	pd->n = 200;
	pd->m = 900;
	
	Base* pb = new Base;
	pb->n = 10;
	pd = dynamic_cast<Derived*>(pb);
//	pb = dynamic_cast<Base*>(pd);	
	//把基类指针转成了派生类型的指针，会得到一个空指针
	//把基类对象转成派生对象的引用，会抛出bad_cast异常
	if (pb == NULL)
		cout << "转换失败" << endl;
//	pb->show();
//	A* pa = new A;
//	pa->w = 333;
//	pb->show();

//	pd = (Derived*)pa;
//	pd = dynamic_cast<Derived*>(pa);
	
	Base b;
	b.n = 28;
	Derived d;
	d.n = 19;
	d.m = 89;

//	Base& rb = dynamic_cast<Base&>(d);
//	rb.show();
//	Derived& rd = dynamic_cast<Derived&>(b);
//	rd.show();
	cout << pd->n << endl;
	cout << pd->m << endl;
	

//	Derived* de = (Derived)b;
	

	return 0;
}
