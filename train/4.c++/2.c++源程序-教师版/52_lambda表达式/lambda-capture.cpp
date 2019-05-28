#include <iostream>
using namespace std;
int c;

int sum(int a, int b)
{
	int k = 1;
	cout << "c = " << c << endl;
	return a + b;
}

int main()
{
	int d = 9;
	int e = 2;
	int f;
	//[]捕获列表，可以将本作用域上面的变量捕获进来
	//[d]以拷贝的形式捕获,默认是只读的，除非在函数后面加mutable
	//[&d] 以引用的形式捕获，可变
	//[=] 全部以拷贝的形式捕获
	//[&] 全部以引用的形式捕获
	//[&, d] 除了d是拷贝的，其他的全部引用
	//[=, &d] 除了d是引用的，其他都是拷贝
//	int (*p)(int, int) 不匹配带捕获列表的lambda表达式
	auto p = [=, &d](int a, int b) mutable { 
		d = 23;
		e = 89;
		cout << "c = " << c << endl;
		cout << "d = " << d << endl;
		cout << "e = " << e << endl;
//		cout << "f = " << f << endl;
		return a + b; 
	};

	cout << p(2, 3) << endl;
	cout << "d = " << d << endl;
	cout << "e = " << e << endl;

	return 0;
}
