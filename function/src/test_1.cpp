#include<iostream>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <string.h>
#include <sys/shm.h>
#include <string.h>

#define IPC_MODE (IPC_CREAT | SHM_R | SHM_W)
using namespace std;
    static int g_semid;                     //信号量的键值
    static key_t g_key;                     //IPC通讯 (消息队列、信号量和共享内存)

union semun{
    int val;                    /*value for SETVAL*/
    struct semid_ds *buf;       /*buffer for IPC_STAT & IPC_SET*/
    unsigned short int *array;  /*array for GETALL & SETALL*/
    struct seminfo *__buf;      /*buffer for IPC_INFO*/
};
//创建信号量集
bool creatSem(int shmflg){
    //创建一个新的信号量或获取一个已经存在的信号量的键值
    if((g_semid = semget(g_key, 1, shmflg)) == -1){
        printf("semget Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }
    return true;
}

//信号量初始化
bool initSem(int val){
    union semun sem_union;
    sem_union.val = val;
    
    //控制信号量的函数
    //SETVAL 设置信号量集中的一个单独的信号量的值
    if(semctl(g_semid, 0, SETVAL, sem_union) == -1){
        printf("semctl SETVSL Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }
    return true;
}

// v 操作
// 释放资源并将信号量值+1
// 如果有进程正在挂起等待，则唤醒它们
bool sem_v(){
    struct sembuf sem_b;
    sem_b.sem_num = 0;       //信号量编码，这里单个信号的编码为0
    sem_b.sem_op  = 1;       //信号量操作，取值为1，表示 v 操作
    sem_b.sem_flg = SEM_UNDO;

    if(semop(g_semid, &sem_b, 1) == -1){
        printf("semop v Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }
    return true;
}

// p 操作
// 若信号量值为1，获取资源并将信号量值-1
// 若信号量值为0，进程挂起等待
bool sem_p(){
    struct sembuf sem_b;
    sem_b.sem_num = 0;       //信号量编码，这里单个信号的编码为0
    sem_b.sem_op  = -1;      //信号量操作，取值为-1，表示 p 操作
    sem_b.sem_flg = SEM_UNDO;

    if(semop(g_semid, &sem_b, 1) == -1){
        printf("semop p Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }
    return true;
}
int main(){
static int num =10;
    bool ok = creatSem(IPC_MODE) ? initSem(1) : false;
    printf("初始化信号量: %d\n", ok);
    
    pid_t pid;
    for(int i = 0; i<10; i++){
        usleep(1);
        if((pid = fork()) == 0)
            break;
    }
    if(pid < 0){
        cout<<"分裂失败! "<<endl;
        exit(-1);
    
    }else if(pid == 0){//子进程
        printf("child pid1=%d,ppid=%d, num = %d \n",getpid(),getppid(), num);
        do{
            sem_p();
            num--;
            sem_v();
        }while(false);
        printf("pid_t: %d, 正在退出...\n", getpid());
        exit(-1);
    }else{
        usleep(2);
        printf("parent pid=%d\n",getpid());
    }
    
    return 0;
}

