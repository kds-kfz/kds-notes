#include<stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
//8.msgsnd.c
#if 0
    int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
    msqid：消息队列id
    -------------------------------------------------------------------
    msgp：
    指向msgbuf结构体的指针，该结构体包含了1个正长整型的字段，和消息数据
    struct msgbuf {
	long mtype;       /* message type, must be > 0 */
	char mtext[1];    /* message data */
    };
    -------------------------------------------------------------------
    msgsz：实际数据字节数
    -------------------------------------------------------------------
    msgflg：
	0：阻塞
	IPC_NOWAIT：非阻塞
    -------------------------------------------------------------------
    返回值：成功返回0,错误返回-1
    -------------------------------------------------------------------
    注意：
	msgsnd返回成功时，消息队列相关的msqid_ds结构会随之更新，
	表明调用的进程id(msg_lspid),调用的时间(msg_stime)
	以及队列中新增的消息(msg_qnum)
#endif
struct msgbuf{
    long mtype;       
    char mtext[20];  
};
int main(){
    struct msgbuf msg={1,"lisi"};
//    int msqid=msgget(ftok("msg3.txt",1),IPC_CREAT|0777);
    int msqid=msgget(ftok("msg3.txt",1),IPC_EXCL);
    if(msqid==-1){
	printf("msgget fail\n");
	exit(-1);
    }
    printf("msgget msqid = %d\n",msqid);
    int ret=msgsnd(msqid,&msg,sizeof(msg)-sizeof(long),0);
    if(ret==-1){
	printf("msgsnd fail\n");
    }
    printf("wait msgsnd\n");
    return 0;
}
