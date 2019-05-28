#include <stdio.h>
#include "1.cpp"
int g_num = 0;
namespace A
{
	int g_num2 = 200;
	void fun() {
		printf("A::fun ---\n");
	}
}
int main()
{
	int g_num = 100;
					//本作用域下的g_num
	printf("g_num = %d\n", g_num);
	//	::g_num是全局变量
	printf("::g_num = %d\n", ::g_num);
	printf("A::g_num = %d\n", A::g_num);
	fun();
	A::fun();
	return 0;
}
