#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
	//创建一个消息队列
	int msgId = msgget(ftok("msg",1),IPC_CREAT|0777);
	if(msgId < 0){
		fprintf(stdout,"msgId error...\n");
		exit(-1);
	}
	printf("creat a msg...\n");

	return 0;
}


