#include<stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
//7.msgget.c
#if 0
    []->[]->[]->[]->[]
    创建消息队列，实际上是在内核里创建一个链表
    每个节点存放一个消息(这样消息和消息之间就是隔离的)
    [对比管道，消息和消息之间没有隔离]
    A->B(发送)--->节点增加
    B->A(提取)--->节点减少
    -------------------------------------------------------
    int msgget(key_t key, int msgflg);
    key：指定key值
    -------------------------------------------------------
    msgflg：
	1.IPC_CREAT：创建消息队列
	2.IPC_EXCL：如果有，则返回错误
	3.mode_flags：权限
#endif
int main(){
    int msqid=msgget(ftok("msg1.txt",1),IPC_CREAT|0777);
    if(msqid==-1){
	printf("msgget fail\n");
	exit(-1);
    }
    printf("msgget msqid = %d\n",msqid);
    return 0;
}
