#include <stdio.h>

//int g_num = 0;
namespace A
{
	int g_num = 200;
	void fun() {
		printf("A::fun ---\n");
	}
}
int main()
{
//	int g_num = 100;
	//使用A命名空间里的所有变量和函数
	using namespace A;

	//下面的g_num就是A::g_num;
//	using A::g_num;
					//A作用域下的g_num
	printf("g_num = %d\n", g_num);
	//	::g_num是全局变量
//	printf("::g_num = %d\n", ::g_num);
	printf("A::g_num = %d\n", A::g_num);
	
//	using A::fun;
	fun();
	

	return 0;
}
