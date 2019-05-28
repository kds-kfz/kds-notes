#include <iostream>
using namespace std;
template <typename T>
int fun(T& a) //只能引用左值
{
	cout << "& " << a << endl;
}
template <typename T>
int fun(T&& a)//模板里，&&既可以引用左值，也可以引用右值 
{
	cout << "&& " << a << endl;
}
template <typename T>
int test(T&& i) //在编译器中会把有名字的右值识别成左值
{
	fun(i);			//一定会把i当作左值处理
	fun(std::move(i));//一定是右值
	fun(std::forward<T>(i));//把i本身的类型进行完美转发
}
int main()
{
	//调用了右值引用
//	fun(1);
	int a = 10;
	//调用了左值引用
//	fun(a);
	test(a);
	test(9);

	return 0;
}
