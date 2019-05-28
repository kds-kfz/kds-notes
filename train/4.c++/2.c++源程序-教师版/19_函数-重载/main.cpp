#include <iostream>
using namespace std;
//函数的重载，可以根据参数的类型或个数不同，同名函数可以区分出不同
//函数的默认参数有可能与函数重载冲突
int sum(int a, int b, int c/*=0*/)
{
	cout << "int 3 " << endl;
	return a + b + c;
}
int sum(int a, int b)
{
	cout << "int 2" << endl;
	return a + b;
}

double sum(double a, double b)
{
	cout << "double " << endl;
	return a + b;
}

float sum(float a, float b)
{
	cout << "float " << endl;
	return a + b;
}

int main()
{
	cout << sum(2, 3) << endl;
	cout << sum(2, 3, 4) << endl;
	cout << sum(2.0, 4.2) << endl;
	cout << sum(2.1f, 1.2f) << endl;
	//定义模糊  (int, double)
	//cout << sum(2, 4.2) << endl;
	return 0;
}
	
