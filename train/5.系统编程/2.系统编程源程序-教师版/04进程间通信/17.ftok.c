#include <sys/types.h>
#include <sys/ipc.h>
#include <stdio.h>

int main()
{
	int key = ftok("sem",2);
	printf("key = %d\n",key);

	return 0;
}
