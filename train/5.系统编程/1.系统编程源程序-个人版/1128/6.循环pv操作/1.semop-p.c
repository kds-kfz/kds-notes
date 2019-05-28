#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
//1.semop-p.c
#if 0
//ipcs 查看进程消息队列
    int semop(int semid, struct sembuf *sops, size_t nsops);

    unsigned short sem_num;  /* semaphore number */
    short          sem_op;   /* semaphore operation */
    short          sem_flg;  /* operation flags */
    
#endif
int main(){
    //创建信号量
    int semid;
    semid=semget(ftok("sem1.txt",1),1,IPC_CREAT|0777);
    if(semid==-1){
	printf("semget error\n");
	exit(-1);
    }
    printf("semid ok ...\n");
    //信号量初始化
    int ret=semctl(semid,0,SETVAL,1);
    if(ret==-1){
	printf("semctl fail\n");
	exit(-1);
    }
    #if 1
    struct sembuf sem;
    sem.sem_num=0;
    sem.sem_op=1;
    sem.sem_flg=SEM_UNDO;
    int count1=0,count2=1;
    int s1=1,s2=1;
    while(1){
	count1++;
	ret=semop(semid,&sem,1);
	if(ret==-1){
	    printf("semop -1 fial\n");
	    exit(-1);
	}
	s1=count1+1;
	s2=s1-3*count2;
	if(s2<0)s2=s1;
	if(s2%3==0&&s2>0){
	    count2++;
	    s2=s1;
	    s2-=3*count2;
	}
	sleep(1);
	printf("1s后,第%d次,每次生产+1,总生产%d个,信号量剩余%d\n",count1,s1,s2);
    }
    #endif
    return 0;
}

