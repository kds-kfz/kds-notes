#include <stdio.h>
#include <signal.h>


void fun(int sig)
{
	printf("recv sig:%d\n",sig);
	return;
}

int main()
{	
	char *p = NULL;
	
	signal(11,fun);

	*p = "hello";
	
	for(;;){
		sleep(1);
	}
	return 0;
}
