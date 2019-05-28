#include <iostream>
#include <functional>//c++关于函数的头文件
using namespace std;
using namespace std::placeholders;//这个命名空间下定义了占位符
int sum(int a, int b, int c)
{
	cout << "a = " << a << endl;
	cout << "b = " << b << endl;
	cout << "c = " << c << endl;
	return a + b + c;
}
//auto p = bind(callback, args);
int main()
{
	auto p1 = bind(sum, -1, _1, _2);
	auto p2 = bind(sum, _1, 100, _2);
	auto p3 = bind(sum, 0, 0, 0);
//	cout << sum(2, 4) << endl;
	cout << p1(2, 3) << endl;
	cout << p2(1, 2) << endl;
	cout << p3() << endl;
	cout << sum(2, 4, 9) << endl;
	return 0;
}
