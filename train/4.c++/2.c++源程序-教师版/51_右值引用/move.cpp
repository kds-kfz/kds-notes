#include <iostream>
using namespace std;
int sum(int&& a, int&& b)
{
	cout << "&&" << endl;
	return a + b;
}
int main()
{
	int i = 9;	//有一个内存 取个名字叫做i, 存了9
	int j = 8;
	//std::move() 函数，把左值转成右值
	cout << sum(std::move(i), std::move(j)) << endl;

	int& r1 = i;	//左值引用:r1是i的别名
	int&& r = std::move(i);	//右值引用: 直接使用9
	r = 80;
	
	cout << &r << " " << &i << endl;

	return 0;
}
