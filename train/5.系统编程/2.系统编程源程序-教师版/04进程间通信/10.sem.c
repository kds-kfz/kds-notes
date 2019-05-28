#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
	int semId;
	semId = semget(ftok("sem",1),1,IPC_CREAT|0777);
	if(semId == -1){
		fprintf(stdout,"semget error\n");
		exit(-1);
	}
	
	
	printf("semget ok...\n");

	return 0;
}
