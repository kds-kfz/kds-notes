#include <iostream>
using namespace std;

//const_cast 只处理const属性的指针或引用，不会转换数据类型

int main()
{
	int a = 19;
	int b = 90;
	int& r = a;
	const int& cr = a;

	int* p1 = &a;
	const int* p2 = &a;
	int *const p3 = &a;
	
//	const int* q1 = const_cast<const int*>(p1);
//	int* const q1 = const_cast<int* const>(p1);
//	q1 = &b;
//	*q1 = 65;
	//int* q2 = const_cast<int* >(p2);
//	int* const q2 = const_cast<int* const>(p2);
//	*q2 = 43;
//	cout << "*q2 " << *q2 << endl;
//	cout << "a " << a << endl;
//	q2 = &b;
//	*q2 = 54;
//	cout << b << endl;
//	int* q3 = const_cast<int*>(p3);
//	const int* q3 = const_cast<const int*>(p3);
//	*q3 = 43;
//	q3 = &b;
	
	int& r2 = const_cast<int&>(cr);
	const int& r3 = const_cast<const int&>(r);

	r3 = 44;
	cout << a;

	return 0;
}
