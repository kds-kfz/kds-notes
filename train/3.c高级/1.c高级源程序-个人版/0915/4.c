#include<stdio.h>
//4.c
//位运算

int main(){
//&与运算
/*
    int a=88888;
    int count=0;
    while(a){
	a&=a-1;
	count++;
    }
    printf("count=%d\n",count);
    //88888二进制1的个数即count的值

    int a=9,b=5;
//9
//0000 0000 0000 0000 0000 0000 0000 1001
//-5原码
//1000 0000 0000 0000 0000 0000 0000 0101
//-5反码
//1111 1111 1111 1111 1111 1111 1111 1010
//-5补码
//1111 1111 1111 1111 1111 1111 1111 1011
//即1001 & 0111 =1011 = 9

    printf("a=%#x,b=%#x\n",a,b);
    int c=a&b;
    printf("c=%#x\n",a&b);
*/
/*
//|或运算
    int a=-9;
    int b=5;
    a |= b;
    printf("a=%d\n",a);
*/
    int a=15;
    int b=20;

    a=a^b;
    b=a^b;
    a=a^b;

    printf("a=%d,b=%d\n",a,b);
    return 0;
}
