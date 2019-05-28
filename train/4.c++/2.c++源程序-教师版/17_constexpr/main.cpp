#include <iostream>
using namespace std;

//c++11　constexpr 函数内部不能有分支　if switch不可以,
//可以递归，要求传入的参数是const的
constexpr int pro(int n)
{
	/*
	if (n < 0)
		return -1;
	else if (n <= 1)
		return 1;
	else 
		return n * pro(n-1);
		*/
	return n <= 1 ? 1 : n*pro(n-1);
}

int main()
{
	constexpr int a = 10;
	//常量表达式, 在编译的时候，知道结果
	//节省执行的时间, 还可以检验某个变量或表达式是不是始终是常量
	constexpr int b = 1 + 2;
	int n = 5;
	constexpr int c = pro(5);

	cout << c << endl;
	return 0;
}
