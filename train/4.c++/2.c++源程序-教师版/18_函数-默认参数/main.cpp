#include <iostream>
using namespace std;
//前置返回类型
//函数的默认参数, 如果不传参，形参的值是默认参数的值, 默认参数要从后面对齐
int sum(int a, int b=0)
{
	return a + b;
}
//c++11 尾置返回类型
auto sub(int a, int b) -> int
{
	return a - b;
}

int main()
{
	cout << sum(2) << endl;
	cout << sum(3, 4) << endl;

	cout << sub(4, 3) << endl;
	return 0;
}
