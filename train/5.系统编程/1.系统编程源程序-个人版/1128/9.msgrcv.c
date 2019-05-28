#include<stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
//9.msgrcv.c
#if 0
    ssize_t msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp,int msgflg);
    msqid：消息队列id
    ---------------------------------------------------------------------------
    msgp：需要接收的结构体指针，存放接收内容
    (包含一个长整型数，即存储的是返回的消息类型，
     其后跟随的是存储实际消息数据的缓存区)
    如果返回的消息长度大于缓存区定义的长度，而且msgflg设置类MSG_NOERROR位，
    则消息会被截断，被阶段的部分丢弃，若没设置该标志且消息态长，则出错返回E2BIG
    消息仍保留在队列中
    ---------------------------------------------------------------------------
    msgsz：实际数据字节数
    ---------------------------------------------------------------------------
    msgtyp：可以取以下值
	0：返回队列的第一个消息
	>0：返回队列中消息类型为msgtyp的第一个消息
	<0：返回队列中消息类型值小于等于msgtyp绝对值的消息，
	如果这种消息有若干个，则取类型值最小的消息
    ---------------------------------------------------------------------------
    msgflg：
	0：阻塞，直到有了指定类型的消息可用，或是从系统中删除了队列
	IPC_NOWAIT：非阻塞，若没有指定类型的消息可用，msgrcv返回-1
    ---------------------------------------------------------------------------
    返回值：成功返回消息数据部分的长度，错误返回-1
    ---------------------------------------------------------------------------
    注意：
	msgrcv成功执行时，消息队列相关的msqid_ds结构会随之更新，
	表明调用的进程id(msg_lspid),调用的时间(msg_stime)，
	并指示队列中的消息数量减少了1个(msg_qnum)
#endif
struct msgbuf{
    long mtype;       
    char mtext[20];  
};
int main(){
    struct msgbuf msg;
    struct msqid_ds msg1;
    int ret;
    int msqid=msgget(ftok("msg2.txt",1),IPC_EXCL);
    if(msqid==-1){
	printf("msgget fail\n");
	exit(-1);
    }
    printf("msgget msqid = %d\n",msqid);
    ret=msgrcv(0,&msg,sizeof(msg)-sizeof(long),1,0);
    if(ret==-1){
	printf("msgrcv fail\n");
    }
    printf("rcv mtext[20] = %s\n",msg.mtext);
    return 0;
}
