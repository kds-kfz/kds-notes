#include <iostream>
using namespace std;
				 // 数据区，未初始化段,默认初始值是0
int num;		 //	全局变量，默认初始值是0
static int s_num;//	静态变量，默认初始值是0

string s;

int main()
{
	int num;//	局部变量，栈区或堆区，默认初始值是随机的
	static int s_num;

	string s;//s都是空字符串, 自定义类型，有自己的初始化方式


	int i = 0;//变量的初始化	拷贝初始化
	int j(2); //直接初始化
	
	string s1 = "hello";	//拷贝初始化
		//	s1 = string("hello");
	string s2("world");		//直接初始化
	string s3(s2);			//拷贝初始化
	
	int k = {3};	//列表初始化
	//c++11　标准 编译加 -std=c++11 或-std=c++1y
	int m{4};		//列表初始化

	double d = 1.23;
	int a = d;		//直接和拷贝初始化，可以执行，会损失精度
	
	int b = {d};	//列表初始化,错误，因为d的值赋给b有丢失信息的风险
	

	cout << a << " " << b << endl;

	return 0;
}
