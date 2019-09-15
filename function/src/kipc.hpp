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
 * -- 注意：使用消息对量作为标识，需要使用信号量，不然又可能造成写已满，读阻塞，造成程序空转
 * -- 其原因有：读的代码耗时过长，恰好这时，其他进程又不断向队列写，造成队列已满
 * 
 * 本文件就是针对这五种进程间通信方法进行实现
 * 设计理念：处理多进程通信资源共享时存在的系列问题
 **********************************************************************************/
# ifndef LOG_MODULE
# define LOG_MODULE "KIPC "
# endif

/* 
 * ipcs -a  是默认的输出信息 打印出当前系统中所有的进程间通信方式的信息
 * ipcrm -s SemaphoreID //删除信号量
 * ipcrm -m SharedMemoryID //删除共享内存段
 * ipcrm -q MessageID //删除消息队列
 *
 * ipcs -s  打印出使用信号进行进程间通信的信息
 * ipcs -m  打印出使用共享内存进行进程间通信的信息
 * ipcs -q   打印出使用消息队列进行进程间通信的信息
 */

//消息队列标识符
extern int g_msqid;
//信号量的键值
extern int g_semid;
//共享内存的标识符
extern int g_shmid;
//指向共享内存段的指针
extern char *g_shm;
//IPC通讯 (消息队列、信号量和共享内存)
extern key_t g_key;


//创建消息队列的键值 | 信号灯的键值 | 共享内存的键值
//IPC通讯 (消息队列、信号量和共享内存)
bool creatID();

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 管 道 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 有 名 管 道 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 消 息 队 列 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
//消息结构体，用于收发消息队列内容，可以自定义该结构体
struct msgbuff{
    long mtype;         /* type of message */
    char mtext[512];    /* message text */
};

//创建新的消息队列或获取已有的消息队列
bool creatMsg(int msgflg);
//消息队列初始化
bool initMsg();
//接收消息队列内容
bool recvMsg(long mtype, struct msgbuff *pmsg, bool &flag);
//发送消息队列内容
bool sendMsg(long mtype, const char *text);
//删除消息队列
bool delMsg();

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 信 号 量 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#define IPC_MODE (IPC_CREAT | SHM_R | SHM_W)

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

//共享内存大小
#define SHM_MAX_SIZE 1024

//清空共享内存内容
bool clearShm();
//向共享内存写入内容
unsigned int writeShm(const char *content);

//创建共享内存
bool creatShm(int shmflg);
//链接共享内存
bool linkShm();
//共享内存初始化(服务器)
bool initShms();
//共享内存初始化(客户端)
bool initShmc();
//接收消息队列内容 + 读取共享内存内容
bool recvshm(long mtype, bool &flag);
//发送消息队列内容 + 写入共享内存内容
bool sendshm(long mtype, const char *text, const char *content);
//断开共享内存
bool disconshm();
//删除共享内存
bool delshm();
//删除共享内存 + 删除消息队列 + 删除信号量
bool delShm();

#endif
