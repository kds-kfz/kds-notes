/*
练习: 写一个函数，求最大值
		既可以求两个无符号数的最大值，
		也可以求三个数的最大值
		Max(2, 3, 1);
		Max(9, 8);
*/

#include <iostream>
using namespace std;

using U32 = unsigned int;
	//	int
int Max(int a, int b, int c=0x80000000)
{
	a = a > b ? a : b;
	return a > c ? a : c;
}

int main()
{
	cout << Max(-2, -1, -4) << endl;
	cout << Max(-1, -2) << endl;

	return 0;
}
