#include <iostream>
using namespace std;

int main()
{
	int a = 10;
	//c++11 -std=c++11 定义一个与已知对象类型相同的对象
	decltype (a) b = 23;
//	-------- int b = 23;

	short s = 23;
	decltype (s) c = 43;

	cout << sizeof(b) << " " << sizeof(c) << endl;
	
	int* p1 = &a;
	int* const p2 = &a;
	//	 顶层const 指针不能变	
	int const* p3 = &a;
	//	 底层const 指针指向的对象不能变
	
	int const * const p4 = &a;
//		  |		　|
//		底层	顶层
	

	decltype (p1) px = &b;
	px = &a;
	*px = 78;
	decltype (p2) py = &b;
//	py = &a;
	*py = 78;
	decltype (p3) pz = &b;
	pz = &a;
//	*pz = 78;

	int& r = a;
	decltype (r) r2;
	
		
	

	return 0;
}
