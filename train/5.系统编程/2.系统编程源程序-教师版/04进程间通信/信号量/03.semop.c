#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

/*
	当进程需要申请或释放公共资源的时候,可以调用semop()来
	对信号量进程操作
	semop参数:
	第一个参数是信号量的id
	第二个参数是struct sembuf结构体
	unsigned short sem_num:信号量集合中信号量的索引号(也就是要操作第几个信号量,按数组下标来)
	short sem_op:操作类型的整数
	整数:加到信号量的值上    
	负数:信号量的值减去绝对值,如果小于零,
	进程阻塞,直到信号量的值至少等于其绝对值
	0:导致操作阻塞,直到信号量的值为0才继续
	short sem_flg:一个符号位
	IPC_NOWAIT:非阻塞操作
	IPC_UNDO:进程退出的时候自动撤销该次操作
	也就是说如果进程退出后忘记释放资源,操作系统会帮助我们释放资源
	第三个参数:第二个参数数组的长度(也就是结构体的个数)
*/

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
	printf("semop sec...\n");
	sem.sem_num = 0;
	sem.sem_op = -1;
	sem.sem_flg = SEM_UNDO;
	ret = semop(semId,&sem,1);
	if(ret < 0){
		printf("semop error.\n");
	}

	return 0;
}
