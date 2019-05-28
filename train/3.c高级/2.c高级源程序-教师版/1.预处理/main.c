#include <stdio.h>
#include <string.h>
#include "fun.h"

//static 全局变量　限定此变量只能在此文件使用
//static 函数　此函数只作用于此文件
//extern 全局变量（不同文件同一个变量 不能初始化两次 ）
//声明此变量是在其他地方定义

int a = 10000;
int main()
{
   fun1();
   printf ("a = %d\n",a);
    char a[20];
    char* s = fun2(a,"abcd");
    printf ("%d\n",strlen(a));

    fun3();
    return 0;
}
