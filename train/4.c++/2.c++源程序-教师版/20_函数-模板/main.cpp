#include <iostream>
using namespace std;
/*
int Max(int a, int b)
{
	cout << "int " << endl;
	return a > b ? a : b;
}
*/
//函数模板
template <typename T> 
T Max(T a, T b)
{
	cout << "T" << endl;
	return a > b ? a : b;
}
int main()
{
	cout << Max(2, 4) << endl;
	cout << Max(1.2, 2.3) << endl;
	const char* s1 = "an"; //   &s1[0]
	const char* s2 = "apple";
	//模板推断"..."是const char* 类型 比较的是首字母地址
	cout << Max("an", "apple") << endl;
	string a("hello");
	cout << Max(string("hello"), string("apple")) << endl;
	//<string>　表示模板的类型T == string
	cout << Max<string>("12", "apple") << endl;
	return 0;
}

