#include<stdio.h>
//6.c
int main(){
//^异或运算，相同为0,不同为1
//任意数跟本身异或是0
//任意数跟整数0异或值本身
//异或没有顺序,5^6^7==7^5^6
/*
//a与b值交换
    int a=15;
    int b=20;
//方式1：
    a=a^b;
    b=a^b;
    a=a^b;
    printf("a=%d,b=%d\n",a,b);
//方式2：
    a=a+b;
    b=a-b;
    a=a-b;
    printf("a=%d,b=%d\n",a,b);
//方式3：
    int c=a;
    a=b;
    b=c;
    printf("a=%d,b=%d\n",a,b);
*/
//异或可做简单加密
    int password=0;
    printf("请输入数字:869\n");
    scanf("%d",&password);
    password^=520;
    printf("想你%d天\n",password);

    return 0;
}
