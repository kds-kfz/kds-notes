#include "Thread.h"
#include <stdio.h>
#include <unistd.h>


void fun1(int x)
{
	while(1){
		printf("I in fun1 x = %d\n", x);
		sleep(2);
	}
}

int main()
{
	std::function<void(void)> func1 = std::bind(fun1,1);
	std::function<void(void)> func2 = std::bind(fun1,2);
	
	Thread t1(func1);//构造函数,创建线程,相关参数初始化
	Thread t2(func2);

	t1.start();//启动线程
	t2.start();

	t1.join();//线程回收
	t2.join();
}
