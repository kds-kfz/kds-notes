#include <stdio.h>
#include <signal.h>

void fun(int sig)
{
	printf("recv sig:%d\n",sig);
	return;
}

int main()
{
	signal(2,fun);
	sleep(3);
	
	raise(2);//给当前进程发送信号
	
	for(;;){
		sleep(1);
	}

	return 0;
}
