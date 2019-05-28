#include <iostream>
#include <cstdio>
using namespace std;

int sum(int a, int b)
{
	return a + b;
}
int sub(int a, int b)
{
	return a - b;
}
typedef int I32;

//定义函数指针类型
typedef int (*pFun)(int, int);
using pFunc = int (*)(int, int);

int main()
{
	//函数名字是函数的地址
	cout << (void*)sum << endl;

	//函数名字加(), 是调用函数
	cout << sum(2, 3) << endl;

//	struct Student s1;
	
	int (*p1)(int, int) = sum;
	pFun pf = sum;
	pFunc pf2 = sub;

	/*
	   等同上面的p1
	int (*p2)(int, int) = &sum;
	int (*p3)(int, int) = *sum;
	*/
	cout << pf2(4, 5) << endl;
//	cout << p2(4, 5) << endl;
//	cout << p3(4, 5) << endl;
	

//	printf("%p\n", sum);

	return 0;
}
