#include <iostream>
using namespace std;

int fun()
{
	return 1;
}
int main()
{
	int a = 9;
	//c++11 auto 通过值推断类型
	auto b = 10;
	auto c = 1.1;	//double
	auto d = 1.2f;	//float
	
	auto e = "hello";//const char*
//	cout << e.size() << endl;
	auto m = fun();

	cout << sizeof(b) << " " << sizeof(c) << " " 
		<< sizeof(d) << endl;
	int& r = a;
	auto& x = r;	//不能直接推断引用类型
	x = 23;			//如果想定义引用，加&符号
	
	auto p = &a;	// p === int*
	*p = 12; 
	p = &b;
	int* const q1 = &a;	//不能直接处理顶层const
	int const* q2 = &a;	//能处理底层const

	auto const p1 = q1; //如果想定义顶层const，需要添加const关键字
//	p1 = &b;
	*p1 = 34;
//	auto p2 = q2;
//	p2 = &b;
//	*p2 = 34;

	cout << a << endl;

	
	return 0;
}
