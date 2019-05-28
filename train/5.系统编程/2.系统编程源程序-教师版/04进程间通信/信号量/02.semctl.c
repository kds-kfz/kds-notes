#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

/*
	semctl函数分析:
	第一个参数是信号量操作的id
	第二个参数是信号量集合中第几个信号量(相当于数组中第几个元素)
*/

int main()
{
	int semId = semget(ftok("sem",1),1,IPC_CREAT|0640);
	if(semId < 0){
		printf("sem error.\n");
	}
	int ret = semctl(semId,0,SETVAL,1);
	if(ret < 0){
		printf("semctl error.\n");
	}

	return 0;
}
