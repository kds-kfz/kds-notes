#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

int main()
{
	struct sembuf sem;
	int semId = semget(ftok("sem",1),1,IPC_CREAT|0640);
	if(semId < 0){
		printf("semget error...\n");
	}
	int ret = semctl(semId,0,SETVAL,1);
	if(ret < 0){
		printf("semctl error...\n");
	}
	sem.sem_num = 0;
	sem.sem_op = 1;
	sem.sem_flg = SEM_UNDO;
	ret = semop(semId,&sem,0);
	if(ret < 0){
		printf("semop error...\n");
	}

	return 0;
}