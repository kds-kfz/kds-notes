#include<iostream>
#include<cstdio>//==<stdio.h>
//1.cpp
using namespace std;

int main(){
    int a=123;
    int b=0123;
    int c=0x123;//0x
    int d=0b1111;//二进制定义，二进制打印，无法打印二进制格式

    printf("%d,%o,%x,%d\n",a,b,c,d);

    cout<<a<<" "<<oct<<a<<" "<<hex<<a<<" "<<dec<<a<<endl;

    unsigned int n=1<<a31;
    //算数右移：当符号位为1时，高位补1，符号位为0时，高位补0
    //逻辑右移：高位补0
    //打印二进制
    while(n){
	cout<<bool(n&d);
	n>>=1;
    }
#if 0
    14 = 01110
    14%2	    0
    14/2    7%2	    1	^
    7/2	    3%2	    1	^
    3/2	    1%2	    1	^
    1/2	    0%2	    0	^
#endif
    return 0;
}

