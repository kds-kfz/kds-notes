#include <stdio.h>
#include <sys/ipc.h>
#include <sys/types.h>
/*
	第二个参数:消息缓冲区指针
		struct msgbuf这样一个结构体
		struct msgbuf{
			long mtype;//类型
			char mtext[1];//内容
		}
		每一个消息都必须遵循这种格式
	第三个参数:消息的大小
	第四个参数:消息的类型,发送端结构体里指定的类型
	第五个参数msgflg:
		0:阻塞 	消息队列是有上限的,如果满了之后,会一直阻塞在这等消耗
		IPC_NOWAIT:非阻塞	

*/
struct msgbuf{
	long mtype;
	char mtext[32];
};

int main()
{
	struct msgbuf msg;
	int msgId = msgget(ftok("msg",1),IPC_CREAT|0777);
	if(msgId < 0){
		printf("msgget error...\n");
	}
	printf("msg ok\n");
	//如果消息类型不匹配会阻塞到这
	int ret = msgrcv(msgId,&msg,sizeof(msg)-sizeof(long),1,0);
	if(ret < 0){
		printf("msgrcv error...\n");
	}
	printf("recv msgtext:%s\n",,msg.mtext);
	return 0;
}
