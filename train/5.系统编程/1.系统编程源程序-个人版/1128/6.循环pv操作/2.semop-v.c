#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
//2.semop-v.c
#if 0
//ipcs 查看进程消息队列
    int semop(int semid, struct sembuf *sops, size_t nsops);

    unsigned short sem_num;  /* semaphore number */
    short          sem_op;   /* semaphore operation */
    short          sem_flg;  /* operation flags */
    
#endif
int main(){
    //打开一个已有的信号量
    int semid;
//    semid=semget(ftok("sem1.txt",1),0,IPC_EXCL);
    semid=semget(ftok("sem1.txt",1),1,IPC_CREAT|0777);
    if(semid==-1){
	printf("semget error\n");
	exit(-1);
    }
    printf("semid ok ...\n");
    //获取信号量集合semid中第一个信号类量的计数器的值,ret接收
    int ret=semctl(semid,0,GETVAL,1);
    if(ret==-1){
	printf("semctl fail\n");
	exit(-1);
    }
    #if 1
    struct sembuf sem;
    sem.sem_num=0;
    sem.sem_op=-3;
    sem.sem_flg=SEM_UNDO;
    int count=0;
    int s=0;
    while(1){
	count++;
	ret=semop(semid,&sem,1);
	if(ret==-1){
	    printf("semop -3 fial\n");
	    exit(-1);
	}
	s+=3;
	sleep(1);
	printf("1s后,第%d次，每次消费-3,共消费%d个\n",count,s);
    }
    #endif
    return 0;
}

