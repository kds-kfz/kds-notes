#ifndef __KIPC_H__
#define __KIPC_H__

/************************************* 标 签 *************************************
 * 作者：kfz
 * 日期：2019-09-12
 * 描述：进程通信方式简要说明，有以下五种
 * (1) 管道
 * -- 它是半双工的（即数据只能在一个方向上流动），具有固定的读端和写端
 * -- 它只能用于具有亲缘关系的进程之间的通信 (也是父子进程或者兄弟进程之间)
 * -- 它可以看成是一种特殊的文件，对于它的读写也可以使用普通的read、write 等函数。
 * -- 但是它不是普通的文件，并不属于其他任何文件系统，并且只存在于内存中
 * (2) FIFO 有名管道
 * -- FIFO 可以在无关的进程之间交换数据，与无名管道不同
 * -- FIFO 有路径名与之相关联，它以一种特殊设备文件形式存在于文件系统中
 * (3) 消息队列
 * -- 消息队列是面向记录的，其中的消息具有特定的格式以及特定的优先级
 * -- 消息队列独立于发送与接收进程。进程终止时，消息队列及其内容并不会被删除
 * -- 消息队列可以实现消息的随机查询,消息不一定要以先进先出的次序读取,也可以按消息的类型读取
 * (4) 信号量
 * -- 信号量用于进程间同步，若要在进程间传递数据需要结合共享内存
 * -- 信号量基于操作系统的 PV 操作，程序对信号量的操作都是原子操作
 * -- 每次对信号量的 PV 操作不仅限于对信号量值加 1 或减 1，而且可以加减任意正整数
 * -- 支持信号量组
 * (5) 共享内存
 * -- 共享内存是最快的一种 IPC，因为进程是直接对内存进行存取
 * -- 因为多个进程可以同时操作，所以需要进行同步
 * -- 信号量 + 共享内存通常结合在一起使用，信号量用来同步对共享内存的访问
 * 
 * 本文件就是针对这五种进程间通信方法进行实现
 * 设计理念：处理多进程通信资源共享时存在的系列问题
 **********************************************************************************/
extern int g_msqid;
extern int g_semid;
extern int g_shmid;
extern char *g_shm;
extern key_t key;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 管 道 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 有 名 管 道 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 消 息 队 列 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
//消息结构体，用于收发消息队列内容，可以自定义该结构体
#if 0
struct msgbuf{
    long mtype;         /* type of message */
    char mtext[512];      /* message text */
};
#endif
//消息队列标识符
//int g_msqid = 0;

//创建新的消息队列或获取已有的消息队列
bool creatMsg(int msgflg);
//接收消息队列内容
bool Recvmsg(int msqid, long mtype);
//发送消息队列内容
bool Sendmsg(int msqid, long mtype, const char *text, const char *content);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 信 号 量 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define IPC_MODE (IPC_CREAT | SHM_R | SHM_W)

//IPC通讯 (消息队列、信号量和共享内存)
//key_t key = 0;
//信号量的键值
//int g_semid = 0;

//联合体，用于semctl初始化
union semun{
    int val;                    /*value for SETVAL*/
    struct semid_ds *buf;       /*buffer for IPC_STAT & IPC_SET*/
    unsigned short int *array;  /*array for GETALL & SETALL*/
    struct seminfo *__buf;      /*buffer for IPC_INFO*/
};

//int g_semid;
//创建信号量集
bool creatSem(int shmflg);
//初始化信号量
bool initSem(int val);
//信号量 v 操作
bool sem_v();
//信号量 p 操作
bool sem_p();
//删除信号量集
bool delsem();

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 共 享 内 存 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
//共享内存的标识符
//int g_shmid = 0;
//指向共享内存段的指针
//char *g_shm ;

//创建共享内存
bool creatShm(key_t key, int shmflg);
//链接共享内存
bool linkShm();
//共享内存初始化(服务器)
bool initShms();
//共享内存初始化(客户端)
bool initShmc();
//删除共享内存
bool delShm();

#endif
