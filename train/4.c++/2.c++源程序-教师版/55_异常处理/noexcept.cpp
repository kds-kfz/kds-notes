#include <iostream>
using namespace std;
//c++11 noexcept 不抛出异常，阻止我们处理异常，终端来处理这个异常
int Div(int a, int b) noexcept(true) //说明符/描述符
{					  //noexcept(true)
	if (b == 0)
		throw 0;
	return a / b;
}
int main()
{
	try {
		//运算符 判断函数是否有 noexcept描述符,有是true
		cout << noexcept(Div(2,1)) << endl;
		Div(2, 0);
	} catch (int n) {
		cout << n << endl;
		return 0;
	}
	return 0;
}
