#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>

#include"kipc.hpp"
#include"klog.hpp"
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 管 道 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
//两个文件描述符，一写一读，半双工通信
//数据从父进程流向子进程；关闭父进程读端，关闭子进程写端
//数据从子进程流向父进程：关闭子进程读端，关闭父进程写端
int fd[2] = {0, 0};
char pipebuff[PIPE_MAX_SIZE];

//管道初始化
bool initPipe(){
    //创建无名管道
    if(pipe(fd) < 0){
        ERROR_TLOG("pipe Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }
    return true;
}

//数据从父进程流向子进程
ssize_t writechild(const char *content){
    //关闭读端
    close(fd[0]);
    if(content == NULL){
        ERROR_TLOG("无名管道通信，向子进程写入数据时，内容非法!\n");
        return -1;
    }
    ssize_t len = write(fd[1], content, PIPE_MAX_SIZE);
    INFO_TLOG("向子进程写入的数据为:[%s]\n", content);
    return len;
}

//从子进程读数据
ssize_t readchild(char *pbuf){
    //关闭读端
    close(fd[1]);
    bzero(pbuf, PIPE_MAX_SIZE);
    ssize_t len = read(fd[0], pbuf, PIPE_MAX_SIZE);
    INFO_TLOG("读取子进程中的数据为:[%s]\n", pbuf);
    return len;
}

//数据从子进程流向父进程
ssize_t writefather(const char *content){
    //关闭读端
    close(fd[1]);
    if(content == NULL){
        ERROR_TLOG("无名管道通信，向父进程写入数据时，内容非法!\n");
        return -1;
    }
    ssize_t len = write(fd[0], content, PIPE_MAX_SIZE);
    INFO_TLOG("向父进程写入的数据为:[%s]\n", content);
    return len;
}

//从父进程读数据
ssize_t readfather(char *pbuf){
    //关闭读端
    close(fd[0]);
    bzero(pbuf, PIPE_MAX_SIZE);
    ssize_t len = read(fd[1], pbuf, PIPE_MAX_SIZE);
    INFO_TLOG("读取父进程中的数据为:[%s]\n", pbuf);
    return len;
}


/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 有 名 管 道 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

char fifobuff[PIPE_MAX_SIZE];

//有名管道文件是一种特殊设备文件，存在于系统文件中
//有名管道初始化
bool initfifo(){
    //创建 fifo 有名管道
    if(mkfifo(FIFO_FILE_NAME, 0640) < 0 && errno != EEXIST){
        ERROR_TLOG("创建 fifo 有名管道失败, Error [%d], errmsg [%s]\n",
                errno, strerror(errno));
        return false;
    }
    return true;
}

//向有名管道文件写入数据
ssize_t writefifo(const char *content){
    if(content == NULL){
        ERROR_TLOG("有名管道通信，写入内容非法，写入失败!\n");
        return -1;
    }
    
    int fd;
    if((fd = open(FIFO_FILE_NAME, O_WRONLY)) == -1){
        ERROR_TLOG("有名管道通信，文件[%s]打开失败!\n", FIFO_FILE_NAME);
        return -2;
    }
    
    int len = write(fd, content, strlen(content));
    if(len == -1){
        ERROR_TLOG("向有名管道写入数据失败!\n");
        return -3;
    }else{
        INFO_TLOG("向有名管道写入数据为:[%s]\n", content);
    }
    close(fd);
    return len;
}

//读取有名管道数据
ssize_t readfifo(char *pbuf){
    bzero(pbuf, PIPE_MAX_SIZE);
    int fd;
    if((fd = open(FIFO_FILE_NAME, O_RDONLY)) == -1){
        ERROR_TLOG("有名管道通信，文件[%s]打开失败!\n", FIFO_FILE_NAME);
        return -1;
    }
    
    int len = read(fd, pbuf, PIPE_MAX_SIZE);
    pbuf[len] = '\0';
    INFO_TLOG("读取名管道的数据为:[%s]\n", pbuf);
    close(fd);
    return len;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 消 息 队 列 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

//服务端消息结构体
struct msgbuff msgs;
//客户端消息结构体
struct msgbuff msgc;

//消息队列标识符
int g_msqid = 0;
//信号量的键值
int g_semid = 0;
//共享内存的标识符
int g_shmid = 0;
//指向共享内存段的指针
char *g_shm  =  NULL;
//IPC通讯 (消息队列、信号量和共享内存)
key_t g_key = 0;

//创建IPC通讯 (消息队列、信号量和共享内存) 的ID值
//创建信号灯的键值
bool creatID(){
    if((g_key = ftok(".", 'a')) < 0){
        ERROR_TLOG("ftok Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }
    return true;
}

//创建新的消息队列或获取已有的消息队列
bool creatMsg(int msgflg){
    if ((g_msqid = msgget(g_key, msgflg)) == -1){
        ERROR_TLOG("msgget Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }
    return true;
}

//TODO 消息队列收发待优化，考虑队列写满，读取数据读不到的情况
//消息队列初始化
bool initMsg(){
    //创建消息队列的键值
    if(!creatID()){
        ERROR_TLOG("g_key = %d", g_key);
        return false;
    }
    //创建新的消息队列或获取已有的消息队列
    //IPC_CREAT 如果消息队列对象不存在，则创建之，否则则进行打开操作
    return creatMsg(IPC_CREAT|0777);
}

//接收消息队列内容
bool recvMsg(long mtype, struct msgbuff *pmsg, bool &flag){
    memset(pmsg, 0, sizeof(msgbuff));
    //接收这一类型的第一个消息
    if(msgrcv(g_msqid, pmsg, sizeof(pmsg->mtext), mtype, 0) == -1){
        ERROR_TLOG("msgrcv Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }
    if(pmsg && !strcmp(pmsg->mtext, "#9002")){
        INFO_TLOG("收到退出指令:[%s], 正在退出...\n", pmsg->mtext);
        flag = false;
    }else{
        INFO_TLOG("读取消息队列内容:[%s]\n", pmsg->mtext);
    }
    return true;
}

//发送消息队列内容
bool sendMsg(long mtype, const char *text){
    memset(&msgc, 0, sizeof(msgbuff));
    msgc.mtype = mtype;
    memcpy(msgc.mtext, text, strlen(text));
    if(msgsnd(g_msqid, &msgc, sizeof(msgc.mtext), 0) == -1){
        ERROR_TLOG("msgsnd Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }
    return true;
}

//接收消息队列内容 + 读取共享内存内容
bool recvshm(long mtype, bool &flag){
    //以下是判断消息内容
    bool mflag = false;
    if(recvMsg(mtype, &msgs, mflag) && !strcmp(msgs.mtext, "#9001")){
        INFO_TLOG("已接收到共享内存的消息内容:[%s]\n", g_shm);
    }else if(!strcmp(msgs.mtext, "#9002")){
        INFO_TLOG("收到退出指令, 正在退出...\n");
        flag = false;
    }else{
        return false;
    }
    return true;
}

//写入共享内存内容 + 发送消息队列内容
bool sendshm(long mtype, const char *text, const char *content){
    if(!strcmp(content, "")){
        clearShm();
    }else{
        writeShm(content);
    }

    return sendMsg(mtype, text);
}

//删除消息队列
bool delMsg(){
    if(msgctl(g_msqid, IPC_RMID, 0) == -1){
        ERROR_TLOG("msgctl IPC_RMID  Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }
    return true;
}

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 信 号 量 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

//创建信号量集
bool creatSem(int shmflg){
    //创建一个新的信号量或获取一个已经存在的信号量的键值
    if((g_semid = semget(g_key, 1, shmflg)) == -1){
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
bool creatShm(int shmflg){
    //创建共享内存的键值
    if(!creatID()){
        ERROR_TLOG("g_key = %d", g_key);
        return false;
    }

    //建立共享内存的长度,1 至 4096，则实际申请到的共享内存大小为 4K (一页)
    //4097 到 8192，则实际申请到的共享内存大小为 8K (两页)
    if((g_shmid = shmget(g_key, SHM_MAX_SIZE, shmflg)) == -1){
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
    //创建 key 值 + 创建共享内存 + 链接共享内存 + 创建新的消息队列或获取已有的消息队列 + 创建信号量集 + 信号量初始化
    
    return !creatShm(IPC_CREAT|0666) ? false : !linkShm() ? false : 
        !creatMsg(IPC_CREAT|0777) ? false :!creatSem(IPC_MODE) ? false : !initSem(1) ? false : true;
}

//共享内存初始化(客户端)
bool initShmc(){
    //创建共享内存 + 链接共享内存 + 创建新的消息队列或获取已有的消息队列 + 创建信号量集

    return !creatShm(0) ? false : !linkShm() ? false :
        !creatMsg(0) ? false :!creatSem(0) ? false : true;
}

//清空共享内存内容
bool clearShm(){
    sem_p();
    memset(g_shm, 0, SHM_MAX_SIZE);
    sem_v();
    return true;
}

//向共享内存写入内容
unsigned int writeShm(const char *content){
    if(strlen(content) > SHM_MAX_SIZE){
        ERROR_TLOG("writeShm content gt %d\n", SHM_MAX_SIZE);
        return false;
    }
    sem_p();
    memcpy(g_shm, content, strlen(content));
    sem_v();
    return strlen(content);
}

//断开共享内存
bool disconshm(){
    if(shmdt(g_shm) == -1){
        ERROR_TLOG("shmdt Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }
    return true;
}

//删除共享内存
bool delshm(){
    //断开共享内存 + 删除共享内存
    if(disconshm() && (shmctl(g_shmid, IPC_RMID, 0) == -1)){
        ERROR_TLOG("shmctl IPC_RMID Error errno = [%d], errmsg = [%s]\n",
                errno, strerror(errno));
        return false;
    }
    return true;
}

//删除共享内存 + 删除消息队列 + 删除信号量
bool delShm(){
    return !delshm() ? false : !delMsg() ? false : !delsem() ? false : true;
}

