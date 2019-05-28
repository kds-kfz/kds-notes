#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
	struct sembuf sem;
	int semId = semget(4,1,IPC_CREAT|0777);
	if(semId == -1){
		fprintf(stdout,"semget error...\n");
		exit(-1);
	}
	//int ret = semctl(semId,0,SETVAL,1);
	//if(ret < 0){
	//	fprintf(stdout,"semctl error...\n");
	//	exit(-1);
	//}
	int count = 0;
	sem.sem_num = 0; 
	sem.sem_op = 1;
	sem.sem_flg = SEM_UNDO;
	while(1){
		sleep(1);
		int ret = semop(semId,&sem,1);
		if(ret < 0){
			fprintf(stdout,"semop error...\n");
			exit(-1);
		}
		printf("生产了一个...:count=%d\n",++count);
	}
	return 0;
}
