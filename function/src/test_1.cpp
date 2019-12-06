#include<iostream>
#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <string.h>
#include <sys/shm.h>
#include <cstring>
#include <errno.h>
#include"thread_pool.hpp"
#include<vector>

#define IPC_MODE (IPC_CREAT | SHM_R | SHM_W)
using namespace std;

typedef struct T{
    string name;
    string sex;
    string job;
    string age;
}TI;

typedef struct U{
    string node;
}NI;

typedef struct J{
    TI data;
    NI info;
}TASK;

int task_n = 0;

void *client_task(void *arg){
    TASK *p = (TASK *)arg;
    TI ti = p->data;
    NI ni = p->info;
    

    usleep(500);
    printf("正在处理任务: %s, %s, %s, %s, %s\n",
            ni.node.c_str(),
            ti.name.c_str(),
            ti.sex.c_str(),
            ti.job.c_str(),
            ti.age.c_str()
            );
    task_n-=1;
    printf("剩余数: %d\n", task_n);
}

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
    
    /*
    pid_t pid;
    for(int i = 0; i<10; i++){
        usleep(1);
        if((pid = fork()) == 0)
            break;
    }
    */
    while(1){
    string name;
    string sex;
    string job;
    string age;
    vector<TI> Info;
    TI ti[5] = {
        {"张三", "男", "护士", "25"},
        {"李四", "女", "学生", "26"},
        {"王五", "男", "教师", "27"},
        {"麻六", "女", "职员", "28"},
        {"小七", "男", "教授", "29"}
    };

    for(int i = 0; i < 5; i++){
        Info.push_back(ti[i]);
    }
    /*
    printf("模拟，已收到任务...\n");
    for(int i = 0; i < 5; i++){
        printf("任务[%d]: %s, %s, %s, %s\n",
                i,
                Info.at(i).name.c_str(),
                Info.at(i).sex.c_str(),
                Info.at(i).job.c_str(),
                Info.at(i).age.c_str()
                );
    }
    */
    pid_t pid;
    pid = fork();
    if(pid < 0){
        cout<<"分裂失败! "<<endl;
        exit(-1);
    
    }else if(pid == 0){//子进程
        printf("child pid1 = %d, ppid = %d\n",getpid(),getppid());
        int task_num = Info.size();
        printf("已收到[%d]个任务\n", task_num);
        
        TASK *pi = new TASK[task_num];
        //memset(pi, 0, sizeof(TASK) * task_num);

        for(int i = 0; i < task_num; i++){
            /*
            memcpy((void*)pi[i].info.node.c_str(), "1001", 4);
            memcpy((void*)pi[i].data.name.c_str(), Info.at(i).name.c_str(), Info.at(i).name.length());
            memcpy((void*)pi[i].data.sex.c_str(), Info.at(i).name.c_str(), Info.at(i).sex.length());
            memcpy((void*)pi[i].data.job.c_str(), Info.at(i).name.c_str(), Info.at(i).job.length());
            memcpy((void*)pi[i].data.age.c_str(), Info.at(i).name.c_str(), Info.at(i).age.length());
            */
            pi[i].info.node = "1001";
            pi[i].data.name =  Info.at(i).name;
            pi[i].data.sex  = Info.at(i).sex;
            pi[i].data.job = Info.at(i).job;
            pi[i].data.age = Info.at(i).age;

        }

        printf("已准备好以下任务...\n");
        /*
        for(int i = 0; i < task_num; i++){
            printf("%s, %s, %s, %s, %s\n",
                    pi[i].info.node.c_str(),
                    pi[i].data.name.c_str(),
                    pi[i].data.sex.c_str(),
                    pi[i].data.job.c_str(),
                    pi[i].data.age.c_str()
                    );
        }
        */
        int ret;
        printf("正在创建线程池...\n");
        if((ret=pool_init(5,10))!=0){
            perror("pool init fail\n");
            exit(-1);
        }
        task_n = task_num;
        //功能已完成，优化：动态创建线程
        for(int i = 0; i < task_num; i++){
            printf("正在创建任务[%d]\n", i);
            ret=pool_add_task(client_task, &pi[i]);
            if(ret!=0){
                printf("add task[%d] fail\n",i);
                exit(-1);
            }
        }
        sleep(1);
        if(task_n == 0){
            delete []pi;
            printf("~~~任务已完成...\n");
        printf("销毁线程池.......\n");
        ret = pool_destroy(pool);
        printf("ret = %d\n", ret);
        }
    }else{
        //sleep(1);
    }
    sleep(6);
    }   
    /*
        if(task_n == 0){
        delete []pi;
        ret = pool_destroy(pool);
        printf("销毁线程池.......\n");
        printf("ret = %d\n", ret);
        printf("child pid=%d, 正在退出\n",getpid());
        exit(-1);
        }
        */
    return 0;
}

