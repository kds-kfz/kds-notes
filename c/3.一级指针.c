#include<stdio.h>

int main(){
    //向内存申请4个字节的空间，占有内存，初始化这地址的值为4，将值赋给常量a
    //值与地址都是变量，地址随值而改变，常量是a
    int a = 10;
    //定义1个指向整型的指针变量，是个野指针，指向不定
    //64bit机，指针地址占8个字节；32bit机，则占4字节
    int *p;
    printf("p address: %p\n", p);
    //取常量a的地址赋给指针变量p，之前随机变量的地址被覆盖
    //p本身的地址不被覆盖
    p = &a;
    printf("a address: %p\n", &a);
    printf("p address: %p\n", p);

    //先取值后运算
    (*p)++;
    printf("*p = %d, p = %p\n",*p, p);
    //先取值后运算
    ++*p;
    printf("*p = %d, p = %p\n",*p, p);
    //先地址运算，再取值
    *p++;
    printf("*p = %d, p = %p\n",*p, p);
    //这里不是耳机指针
    //&p++;

    //地址可以 +，-，++，-- 运算，不可以 *，/，% 运算

    return 0;
}
