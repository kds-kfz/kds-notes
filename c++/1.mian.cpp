#include<stdio.h>

int a = 10;

namespace A{
    int c = 30;
    void fun1(){
        printf("c = %d\n",c);
    }
}

void fun();

int main(){
    int a = 100;
    fun();
    //就近原则
    printf("a = %d\n",a);
    printf("c = %d\n",A::c);
    A::fun1();
    using namespace A;
    fun1();
    //打印全局变量
    printf("a = %d\n", ::a);
}
