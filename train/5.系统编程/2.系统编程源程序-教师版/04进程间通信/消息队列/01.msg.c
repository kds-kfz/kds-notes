/*
	[]->[]->[]->[]->[]->[]
	创建消息队列,实际上是在内核里创建了一个链表
	每个节点存放一个消息(这样消息和消息之间就是隔离的)
	[对比管道,消息和消息之间没有隔离]
	A->B(发送)--->节点增加
	B->A(提取)--->节点减少
*/
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/types.h>
/*
	key值:
		1.指定key值
		2.IPC_PRIVATE系统指定
	msgflg:
	1.IPC_CREAT创建消息队列,没有创建
	2.IPC_EXECL如果有,则返回错误
	3.mode_flags:权限	
*/
int main()
{
	int msgId = msgget(ftok("msg",1),IPC_CREAT|0777);
	if(msgId < 0){
		printf("msgget error...\n");
	}
	printf("msg ok\n");

	return 0;
}
