#include <iostream>
using namespace std;
//练习: 写两个递归函数， 1~n的和，　n的阶乘, 使用函数指针调用函数
//	pro(4);	4*3*2*1	
//  sum(4); 4+3+2+1 
int pro(int n)
{
	if (n < 0)
		return -1;
	else if (n <= 1)
		return 1;
	else 
		return n * pro(n-1);
}
int sum(int n)
{
	if (n <= 0)
		return 0;
	else 
		return n + sum(n-1);
}
using pFun = int (*)(int);

int main()
{
	pFun p = sum;
	cout << p(4) << endl;
	pFun q = pro;
	cout << q(5) << endl;
	
	return 0;
}
