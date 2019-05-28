#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
/*信号量的销毁*/
int main()
{
	struct sembuf sem;

	int semId = semget(ftok("sem",1),1,IPC_CREAT|0640);
	if(semId < 0){
		printf("sem error.\n");
	}
	int ret = semctl(semId,0,SETVAL,1);
	if(ret < 0){
		printf("semctl error.\n");
	}

	printf("semop fir...\n");
	sem.sem_num = 0;
	sem.sem_op = -1;
	sem.sem_flg = SEM_UNDO;
	ret = semop(semId,&sem,1);
	if(ret < 0){
		printf("semop error.\n");
	}
	//移除信号量
	ret = semctl(semId,0,IPC_RMID,0);
	if(ret < 0){
		printf("IPC_RMID error...\n");
	}

	return 0;
}
