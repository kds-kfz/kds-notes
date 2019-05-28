#include <iostream>
using namespace std;

int Max(int a, int b)
{
	cout << "int " << endl;
	return a > b ? a : b;
}

//特例--普通函数
string Max(string a, string b)
{
	cout << "size " << endl;
	return a.size() > b.size() ? a : b;
}
//函数模板
template <typename T> 
T Max(T a, T b)
{
	cout << "T" << endl;
	return a > b ? a : b;
}

int main()
{
	//模板推断"..."是const char* 类型 比较的是首字母地址
	cout << Max("an", "apple") << endl;
	cout << Max(string("hello"), string("apple")) << endl;
	//<string>　表示模板的类型T == string
	cout << Max<string>("12", "apple") << endl;
	//特例函数和模板的使用方式,　加<>会调用模板
	//不加<>会调用函数
	cout << Max(string("an"), string("apple")) << endl;
	
	
	return 0;
}

