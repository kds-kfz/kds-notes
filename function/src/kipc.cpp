#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>

#include"kipc.hpp"
#include"klog.hpp"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 消 息 队 列 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

struct msgbuf msg;
int g_msqid = 0;
int g_semid = 0;
int g_shmid = 0;
key_t key = 0;
char *g_shm  =  NULL;

//创建新的消息队列或获取已有的消息队列
bool creatMsg(int msgflg){
    if ((g_msqid = msgget(key, msgflg)) == -1){
        ERROR_TLOG("msgget Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }
    return true;
}

//接收消息队列内容
bool Recvmsg(int msqid, long mtype, const char *content){
    memset(&msg, 0, sizeof(msgbuf));
    if(msgrcv(msqid, &msg, 1, mtype, 0) == -1){
        ERROR_TLOG("msgrcv Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }

    //比对消息队列内容是否与 content 一致
    if(!strcmp(msg.mtext, content)){
        sem_p();
        INFO_TLOG("已接收到共享内存的消息内容:[%s]\n", g_shm);
        sem_v();
    }
    return true;
}

//发送消息队列内容
bool Sendmsg(int msqid, long mtype, const char *text, const char *content){
    memset(&msg, 0, sizeof(msgbuf));
    sem_p();
    memcpy(g_shm, content, strlen(content));
    sem_v();
    msg.mtype = mtype;
    memcpy(msg.mtext, text, strlen(text));
    if(msgsnd(msqid, &msg, sizeof(msg.mtext), 0) == -1){
        ERROR_TLOG("msgsnd Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }
    return true;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 信 号 量 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

//创建IPC通讯 (消息队列、信号量和共享内存) 的ID值
//创建信号灯的键值
bool creatID(key_t key){
    if((key = ftok(".", 'a')) < 0){
        ERROR_TLOG("ftok Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }
}

//创建信号量集
bool creatSem(int shmflg){
    //创建一个新的信号量或获取一个已经存在的信号量的键值
    if((g_semid = semget(key, 1, shmflg)) == -1){
        ERROR_TLOG("semget Error errno = [%d], errmsg = [%s]\n",
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
        ERROR_TLOG("semctl SETVSL Error errno = [%d], errmsg = [%s]\n",
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
        ERROR_TLOG("semop v Error errno = [%d], errmsg = [%s]\n",
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
        ERROR_TLOG("semop p Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }
    return true;
}

//信号量删除
bool delsem(){
    union semun sem_b;
    if(semctl(g_semid, 0, IPC_RMID, sem_b) == -1){
        ERROR_TLOG("semctl IPC_RMID Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }
    return true;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 共 享 内 存 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

//创建共享内存
bool creatShm(key_t key, int shmflg){
    //创建共享内存的键值
    if(!creatID(key)){
        return false;
    }

    //建立共享内存的长度,1 至 4096，则实际申请到的共享内存大小为 4K (一页)
    //4097 到 8192，则实际申请到的共享内存大小为 8K (两页)
    if((g_shmid = shmget(key, 1024, shmflg)) == -1){
        ERROR_TLOG("shmget Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }
    return true;
}

//链接共享内存
bool linkShm(){
    if((g_shm = (char *)shmat(g_shmid, NULL, 0)) == NULL){
        ERROR_TLOG("shmat Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }
    return true;
}

//共享内存初始化(服务器)
bool initShms(){
    //创建共享内存 + 链接共享内存 + 创建新的消息队列或获取已有的消息队列 + 获取 key 值 + 创建信号量集 + 信号量初始化
    
    return !creatShm(key, IPC_CREAT|0666) ? false : !linkShm() ? false : 
        !creatMsg(IPC_CREAT|0777) ? false : !creatID(key) ? false :
        !creatSem(IPC_MODE) ? false : !initSem(1) ? false : true;
}

//共享内存初始化(客户端)
bool initShmc(){
    //创建共享内存 + 链接共享内存 + 创建新的消息队列或获取已有的消息队列 + 创建信号量集

    return !creatShm(key, 0) ? false : !linkShm() ? false :
        !creatMsg(0) ? false :!creatSem(0) ? false : true;
}

//删除共享内存
bool delShm(){
    //断开共享内存
    if(shmdt(g_shm) == -1){
        ERROR_TLOG("shmdt Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }

    //删除共享内存
    if(shmctl(g_shmid, IPC_RMID, 0) == -1){
        ERROR_TLOG("shmctl IPC_RMID Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }

    //删除消息队列
    if(msgctl(g_msqid, IPC_RMID, 0) == -1){
        ERROR_TLOG("msgctl IPC_RMID  Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }

    //删除信号量
    if(!delsem()){
        return false;
    }

    return true;
}
