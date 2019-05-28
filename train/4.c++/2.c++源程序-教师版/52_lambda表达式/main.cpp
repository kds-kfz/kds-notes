#include <iostream>
#include <algorithm>//c++算法头文件

using namespace std;
//	函数名字(参数列表) 返回值类型
auto sum(int a, int b) -> int
{	
	//函数体
	return a + b;
}
/*
   lambda表达式，是一个可调用对象，相当于一个没有名字的函数
[捕获列表](参数列表)-> 返回类型 {函数体};
	|		|			|			|
[可以空](可以空)返回类型可以省略 {可以空};
	// 1, 距离
	// 2, 简洁
	// 3, 效率
 */
int (*p)(int, int) = [](int a, int b)->int{
	return a - b;
};
bool cmp(int a, int b)
{
	return a > b;
}
int main()
{					
	cout << p(2, 3) << endl;
	//写一个labmda表达式，求两个数中的最大数
	auto p2 = [](int a, int b) { return a>b?a:b; };
	cout << p2(9, 3) << endl;
	int a[4] = {3, 4, 5, 3,};
	sort(a, a+4, [](int a, int b) {return a > b;});
	
	for (auto m : a)
		cout << m << " ";
	cout << endl;
	string arr[5] = {"hello", "hi", "world", "friend", "a"};
	//从长到短排序:
	sort(arr, arr+5, [](const string& a, const string& b) {return a.size() > b.size();});
	for (auto m : arr)
		cout << m << " ";
	cout << endl;
	return 0;
}
