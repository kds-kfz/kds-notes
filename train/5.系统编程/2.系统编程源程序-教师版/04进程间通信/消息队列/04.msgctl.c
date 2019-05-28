#include <stdio.h>
#include <sys/ipc.h>
#include <sys/types.h>

/*
	第一个参数:消息队列标识符
	第二个参数:cmd
	IPC_STAT:读取消息队列的数据结构msgqid_ds,并将其存储在buf指定的地址中
	IPC_SET:设置消息队列的数据结构msgqid_ds中的ipc_perm,msg_qbytes,msg_ctime元素的值,这个值取自buf
	IPC_RMID:从系统内核中移走消息队列
*/

struct msgbuf{
	long mtype;
	char mtext[20];
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
	printf("recv msgtext:%s\n",msg.mtext);

	ret = msgctl(msgId,IPC_RMID);
	if(ret < 0){
		printf("msgctl...\n");
	}
	return 0;

}
