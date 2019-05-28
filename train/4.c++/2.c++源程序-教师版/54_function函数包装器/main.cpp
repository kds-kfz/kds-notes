#include <iostream>
#include <functional>
using namespace std;
using namespace std::placeholders;
int sum(int a, int b)
{
	return a + b;
}

class Value
{
public:
	int operator()() {
		cout << "no" << endl;
	}
	int operator()(int a) {
		return -a;
	}
};
int main()
{
	int n = 10;
	//可调用对象: 函数, 函数指针，lambda表达式, bind的返回值, 重载了()运算符的对象

	//函数包装器，可以指向任意类型的可调用对象
	function<int(int, int)> ps = sum;
	
	//void (*p)()	错误，类型不匹配
	function<void()> p = [n]() {
		cout << n << endl;
	};

	p();
	
	function<int(int)> pb =bind(sum, _1, 0);
	cout << pb(3) << endl;
	
	Value v;				//定义一个没名字的对象	
	function<int(int)> pv = Value();//v是个可调用对象
	
	cout << pv(3) << endl;
//	cout << v(8) << endl;
	function<int()> pv2 = v;
	pv2();

	return 0;
}
/*
int sum(int a, int b)
{
}
auto p = [](int a, int b) {return }


使用:函数，函数指针，lambda表达式，类
写+ - * / 四则运算
map<char, function<int(int,int)> > m;
m['+'] = sum;	//增加
m.insert(pair<char, function<int(int, int) >('-', p)>);

//调用
m['+'](2, 3)
*/
