#include <iostream>
using namespace std;

//异常处理
int Div(int a, int b)
{
	if (b == 0) {
	//	cout << "错误" << endl;
	//	return 0;
		throw "除数为0";
	}
	return a / b;
}
class Base
{
	int n;
public:
	Base(int i) : n(i) {
		if (n < 0)
			//在构造函数里可以抛出异常，阻止对象产生
			throw "Base 为负数";
		cout << "B" << n << endl;
	}
	//~Base() = delete; 析构函数不要写成delete 
	~Base() {
		//原则上，在析构函数里不要抛出异常
		cout << "~B" << endl;
	}
};
//练习: 定义一个三角形类, 如果三边不能构成三角形，阻止这个对象构造

int main()
{
	Base* p = nullptr;
	try
	{
	//	throw 0; //只要抛出异常，当前作用域下的代码不会再执行
	//	Div(2, 1);
	//	throw 1.1;
		p = new Base(-1);
		cout << "end try" << endl;
	} catch (const char* s) {
		cout << s << endl;
		delete p;
		return 0;
	} catch (int a) {
		cout << a << endl;
	} catch (...) { //能捕获任意类型的异常, 必须放在最后
		cout << "..." << endl;
	}/* catch (double d) {
		cout << d << endl;
	} */
	
	cout << "end main" << endl;
	delete p;
	return 0;
}
