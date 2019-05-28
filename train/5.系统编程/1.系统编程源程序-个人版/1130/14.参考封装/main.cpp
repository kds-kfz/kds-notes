#include "Thread.h"
#include <stdio.h>
#include <unistd.h>


void fun1(int x)
{
    printf("I in fun1 x = %d\n", x);
}

int main()
{
    std::function<void(void)> func = std::bind(fun1,1);
    Thread t1(func);//构造函数,创建线程,相关参数初始化
    t1.start();//启动线程

    t1.join();//线程回收
}
