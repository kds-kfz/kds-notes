#include <iostream>
using namespace std;

int a[4];

int main()
{
	int a1[4];
	int a2[4] = {1, 2, 3, 4};//列表初始化
	int a3[4] = {};
	int a4[] = {1, 2, 3};
	int a5[] = {};//空的数组
	int a6[0];
	
	cout << &a4 << endl;
	cout << &a5 << endl;
	cout << &a6 << endl;
	cout << sizeof(a5) << endl;
	cout << sizeof(a6) << endl;

	//c++11 
	int b2[4]{1, 2, 3, 4};//列表初始化
	int b3[4]{};
	for (int i=0; i<4; i++)
		cout << b2[i] << " ";
	cout << endl;


	return 0;
}
