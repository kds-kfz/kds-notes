#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
//2.semctl.c
#if 0
//ipcs 查看进程消息队列
    内核为每个信号量集合维护着一个semid_ds个结构
    struct semid_ds {
	struct ipc_perm sem_perm;
	time_t          sem_otime;
	time_t          sem_ctime;
	unsigned long   sem_nsems;
    };
    ---------------------------------------------------------------------
    每个信号量由一个无名结构体表示，它至少包含类以下成员：
    struct{
	unsigned short semval;
	pid_t sempid;
	unsigned short semncnt;
	unsigned short semzcnt;
    };
    ---------------------------------------------------------------------
    int semctl(int semid, int semnum, int cmd, ... /* union semun arg*/);
    第4个参数是可选的，是否使用取决于请求的命令，
    如果使用该参数，则其类型是semun，它是一个共用体
    union semun{
	int val;
	struct semid_ds *buf;
	unsigned short *array;
	struct seminfo  *__buf;
    };
    ---------------------------------------------------------------------
    int semctl(int semid, int semnum, int cmd, ... /* union semun arg*/);
    semid：信号量id
    ---------------------------------------------------------------------
    semnum：信号量集合中具体信号量，以数组下标访问
    ---------------------------------------------------------------------
    cmd：操作类型
	IPC_STAT：对此集合取semid_ds结构，并存储在由arg.buf指向的结构中
	IPC_SET：按arg.buf指向的结构中的值，设置与此集合相关联的结构体中的
		 sem_perm.uid,sem_perm.gid,sem_perm.mode字段
	IPC_RMID：从系统中删除该信号量集合
	GETVAL：返回信号量集合中的semnum信号量的semval值(计数器值)
	SETVAL：设置信号量集合中的semnum信号量的semval值(计数器值)，由arg.val指定
	GETPID：返回信号量集合中的semnum信号量的sempid值(最后一次操作信号量的进程pid)
	GETNCNT：返回信号量集合中的semnum信号量的semncnt值()
	GETZCNT：返回信号量集合中的semnum信号量的semzcnt值()
	GETALL：取该集合中所有的信号量的值。存储在arg.array指向的数组中
	SETALL：将该集合中所有的信号量值设置成arg.array指向的数组中的值
    ---------------------------------------------------------------------
    返回值：根据操作类型cmd确定

#endif
int main(){
    //创建信号量
    int semid;
    semid=semget(3,1,IPC_CREAT|0777);
    if(semid==-1){
	printf("semget error\n");
	exit(-1);
    }
    printf("semid ok ...\n");
    //信号量初始化
    //对新创建的信号量初始化
    int ret=semctl(semid,0,SETVAL,1);
    if(ret==-1){
	printf("semctl fail\n");
	exit(-1);
    }
    return 0;
}

