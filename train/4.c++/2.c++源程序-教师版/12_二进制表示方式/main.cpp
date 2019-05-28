#include <iostream>
#include <cstdio>	// == <stdio.h>
using namespace std;

int main()
{
	int a = 123;
	int b = 0123;
	int c = 0x123;	//0X
	int d = 0b1111;  //0B　二进制数字定义, 如果想打印二进制,没办法直接打印
	
	printf("%d, %o, %x, %d\n", a, b, c, d);
	cout << a << " " << oct << b << " " 
		<< hex << c	<< " " << dec << d << endl;
	
	unsigned int n = 1 << 31;
	//算数右移:当符号位为1的时候，高位会补一,　符号位为0的时候，高位补0
	//逻辑右移:高位补0
	
	//从右向左输出每一位
	while(n) {
		cout << bool(n & d);
		n >>= 1;
	}
	cout << endl;
/*
	14 % 2			0
	14 /= 2  7%2	1
	7  /= 2	 3%2	1	^
	3  /= 2  1%2	1	^
	1  /= 2	 0%2	0	|
*/						
	return 0;
}
