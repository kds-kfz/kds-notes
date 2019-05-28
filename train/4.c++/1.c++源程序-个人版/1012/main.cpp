//#include<iostream>
#include<stdio.h>
//main.cpp

int a=10;

namespace A{
    int a=100;
}
namespace A{
    int c=33;
    void fun1(){
	printf("c=%d\n",c);
    }
}

//fun存在1.cpp中，在main.cpp调用，需将其声明
void fun();

int main(){
    int a=120;
    fun();//1.cpp中的fun

    printf("a=%d\n",a);//就近原则打印

    printf("a=%d\n",::a);//::a 是全局变量
    //::域操作符
//    using A::fun;
    using namespace A;
    fun1();//A作用里的fun

    printf("a=%d\n",a);//A作用域里的a
    return 0;
}
