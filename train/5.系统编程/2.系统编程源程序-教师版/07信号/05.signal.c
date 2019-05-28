#include <stdio.h>
#include <signal.h>

void fun(int sig)
{
	printf("recv sig:%d\n",sig);
}

int main()
{
	signal(10,fun);
	for(;;){
		printf("in maining...\n");
		sleep(2);
	}

	return 0;
}
