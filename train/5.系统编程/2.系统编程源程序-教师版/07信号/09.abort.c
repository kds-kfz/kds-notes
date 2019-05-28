#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>

void fun(int sig)
{
	printf("recv sig:%d\n",sig);
	return;
}

int main()
{
	signal(6,fun);
	//sleep(3);
	//abort();//程序异常退出
	int i = -1;
	assert(i==0);//断言

	for(;;){
		assert(-1);
		sleep(1);
	}
	return 0;
}
