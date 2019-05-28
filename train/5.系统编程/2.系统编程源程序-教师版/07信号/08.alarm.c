#include <stdio.h>
#include <signal.h>

void fun(int sig)
{
	printf("recv sig:%d\n",sig);
	return;
}

int main()
{
	signal(14,fun);
	printf("before alarm\n");
	alarm(5);
	
	for(;;){
		sleep(1);
	}
	return 0;
}
