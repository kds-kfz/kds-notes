#include <iostream>
using namespace std;
void fun(int* arr, int n)
{
	/*	数组传参后退化成指针，不能使用范围for循环
	for (int a : arr) {
		
	}
	*/
}

int main()
{
	int arr[4] = {4, 56, 23, 321};
//	int i = 0;
	for (int i=0; i<4; i++) {
	//	arr[i] = 10;
		cout << arr[i] << " ";	
	//	i++;
	}
	cout << endl;
	
	//范围for语句，只能从存储结构(数组，string ...)的开始到末尾
	//int n;	//在外面定义n是c++14的标准
	for (int& n : arr) //c++11,  int n = arr [...]拷贝
	{				   //		 int& n = arr[...]引用
		n = 10;
		cout << n << " ";
	}
	cout << endl;
	for (int i=0; i<4; i++) {
		cout << arr[i] << " ";	
	}
	cout << endl;
	string s = "hello";
	//auto 自动推导出成员类型
	for (auto& c : s) {
		cout << c << " ";
	}
	cout << endl;
	return 0;
}



