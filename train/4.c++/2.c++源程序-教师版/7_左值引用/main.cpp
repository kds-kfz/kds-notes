#include <iostream>
using namespace std;

void Swap1(int* n1, int* n2)
{
	int t = *n1;
	*n1 = *n2;
	*n2 = t;
}
//int& n1 = a, int& n2 = b
//非引用传递的是值，引用传递的是对象
int& Swap2(int& n1, int& n2) 
{
	int t = n1;
	n1 = n2;
	n2 = t;
//	return t;//错误，不能以引用的方式返回一个局部变量
	return n1;
}
int main()
{
	int a = 10;
	int b = 20;
//	Swap1(&a, &b);
	int ra = Swap2(a, b);
	Swap2(a, b) = 100;
	cout << "a = " << a << " b = " << b << endl;
	cout << "ra = " << ra << endl;

	int* p = &a;
	cout << p << endl;
	cout << *p << endl;


	int& r = a;//r是a的引用，相当于a的别名
	int& r2 = b;//引用必须初始化，绑定唯一一个对象
	cout << r << endl;
	r = 90;
	r = b;
	cout << "a = " << a << ", r = " << r << endl;
	



	return 0;
}
