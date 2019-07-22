#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
//3.semop.c
#if 0
//ipcs 查看进程消息队列
    int semop(int semid, struct sembuf *sops, size_t nsops);
    当进程需要申请或释放公共资源的时候，可以调用semop()来对信号量操作
    -----------------------------------------------------------------------------
    semid：参数是信号量id
    -----------------------------------------------------------------------------
    struct sembuf结构体：
    struct sembuf{
	unsigned short sem_num;  /* semaphore number */
	short          sem_op;   /* semaphore operation */
	short          sem_flg;  /* operation flags */
    };
    sem_num：信号量集合中信号量的索引号(也就是要操作第几个信号，按数组下标来)
    sem_op：操作类型的整数
	1.正整数：加到信号量的值上
	2.负正数：信号量的值减去绝对值，如果小于0
	3.进程阻塞，直到信号的值至少等于其绝对值
	0.：倒置操作阻塞，直到信号量的值为0才继续
    sem_flg：一个符号位
	1.IPC_NOWAIT：非阻塞操作
	2.SEM_UNDO：进程退出的时候自动撤销本次操作
	也就是说如果进程退出后忘记释放资源，操作系统会帮助我们释放资源
    -----------------------------------------------------------------------------
    nsops：第二个参数数组的长度(也就是结构体的个数)
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
    sem.sem_op=-1;
    sem.sem_flg=SEM_UNDO;
    printf("第1次消费\n");
    ret=semop(semid,&sem,1);
    if(ret==-1){
	printf("semop -1 fial\n");
    }
    sleep(3);
    printf("第2次消费\n");
    ret=semop(semid,&sem,1);
    if(ret==-1){
	printf("semop -1 fial\n");
    }
    #endif
    return 0;
}
