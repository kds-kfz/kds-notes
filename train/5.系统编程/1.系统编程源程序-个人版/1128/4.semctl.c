#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
//4.semctl.c
#if 0
    IPC_RMID：从系统中删除该信号量集合。这种删除是立即发生的。
    删除时仍在使用该信号量集合的其它进程，在他们下次试图对此信号量集合进行操作时
    将错误返回EIDRM。
    此命令只能呢个由两种进程执行：
    一种是其有效用户的id等于sem_perm.cuid或sem_perm,uid的进程
    另一种是具有超级用户特权的进程
#endif
int main(){
#if 0
    //获取已有的集合信号量id
    int semid;
    semid=semget(0,1,IPC_EXCL);
    if(semid==-1){
	printf("semget error\n");
	exit(-1);
    }
    printf("semid ok [%ld]...\n", semid);
#endif
    //移除集合信号量
    int ret=semctl(semid,0,IPC_RMID,0);
    if(ret==-1){
	printf("remove semid fial\n");
    }
    else
	printf("remove semid ok\n");
    return 0;
}

