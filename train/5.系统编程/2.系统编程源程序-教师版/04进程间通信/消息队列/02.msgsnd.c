#include <stdio.h>
#include <sys/ipc.h>
#include <sys/types.h>
/*
	msgsnd(id,&msg,sizeof(msg)-sizeof(long)/消息本身的大小/,0)
	第二个参数:消息缓冲区指针
		struct msgbuf这样一个结构体
		struct msgbuf{
			long mtype;//类型
			char mtext[1];//内容[消息本身]
		}
		每一个消息都必须遵循这种格式
	第三个参数:消息的大小(仅限于消息本身长度)
	第四个参数msgflg:
		0:阻塞 	消息队列是有上限的,如果满了之后,会一直阻塞在这等消耗
		IPC_NOWAIT:非阻塞	

*/

struct msgbuf{
	long mtype;
	char mtext[32];/*可以改成struct stu s这样就可以发送学生信息了*/
};

int main()
{
	struct msgbuf msg = {1,"hello"};

	int msgId = msgget(ftok("msg",1),IPC_CREAT|0777);
	if(msgId < 0){
		printf("msgget error...\n");
	}
	printf("msg ok\n");
	int ret = msgsnd(msgId,&msg,sizeof(msg)-sizeof(long),0);
	if(ret < 0){
		printf("msgsnd error...\n");
	}
	
	return 0;
}
