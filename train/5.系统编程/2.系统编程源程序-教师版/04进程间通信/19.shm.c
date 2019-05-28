#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
	int shmId = shmget(ftok("shm",1),1024,IPC_CREAT|0777);
	if(shmId < 0){
		fprintf(stdout,"shmget error...\n");
		exit(-1);
	}
	printf("shmget ok...\n");

	return 0;
}
