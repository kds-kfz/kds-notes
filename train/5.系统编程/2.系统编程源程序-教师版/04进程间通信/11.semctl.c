#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
	//创建信号量
	int semId = semget(3,1,IPC_CREAT|0777);
	if(semId < 0){
		fprintf(stdout,"semget error...\n");
		exit(-1);
	}
	printf("semget ok...\n");
	//信号量初始化(控制)
	int ret = semctl(semId,0,SETVAL,1);
	if(ret < 0){
		fprintf(stdout,"semctl error...\n");
		exit(-1);
	}
	printf("semctl ok...\n");

	return 0;
}
