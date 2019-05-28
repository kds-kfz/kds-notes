#include <iostream>
#include <algorithm>
using namespace std;
//练习: 求两个变量的最大值, 函数名字Max	
//		int, double, string

int Max(int a, int b)
{
	return a > b ? a : b;
}
double Max(double a, double b)
{
	return a > b ? a : b;
}
string Max(string a, string b)
{
	return a > b ? a : b;
}

//	max	是标准库的模板,会识别"hello"成const char* 类型 , 比较的结果是首字母地址的大小

int main()
{
	cout << Max(2, 3) << endl;
	cout << Max(2.2, 3.1) << endl;
	cout << Max("hello", "friend") << endl;
//	cout << max("hello", "friend") << endl;

	return 0;
}
