#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct STU{
	char name[20];
	int id;
	char age;
};

struct msgbuf {
   long mtype;       
   char mtext[20];// struct STU stu;  
};


int main()
{
	struct msgbuf msg;

	int msgId = msgget(ftok("msg",1),IPC_CREAT|0777);
	if(msgId < 0){
		fprintf(stdout,"msgget error...\n");
		exit(-1);
	}

	msg.mtype = 1;//消息类型
	strcpy(msg.mtext,"lisi");
	
	int ret = msgsnd(msgId,&msg,sizeof(msg)-sizeof(long),0);
	if(ret < 0){
		fprintf(stdout,"msgsnd error...\n");
		exit(-1);
	}

	return 0;
}

