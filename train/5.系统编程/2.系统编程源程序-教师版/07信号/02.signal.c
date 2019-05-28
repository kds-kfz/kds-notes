#include <stdio.h>
#include <signal.h>

void fun(int sig)
{
	printf("recv sig:%d\n",sig);
	return ;
}

int main()
{
	//signal(SIGINT,fun);
	//signal(9,fun);
	//signal(9,SIG_DFL);
	signal(2,fun);
	for(;;){
		sleep(1);
	}

	return 0;
}
