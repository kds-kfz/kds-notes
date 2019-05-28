#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>

struct msgbuf {
  long mtype;      
  char mtext[20];    
};

int main()
{
	struct msgbuf msg;
	int msgId = msgget(ftok("msg",1),IPC_CREAT|0777);
	if(msgId < 0){
		fprintf(stdout,"msgget error...\n");
	}
	int ret = msgrcv(msgId,&msg,sizeof(msg)-sizeof(long),1,0);
	if(ret < 0){
		fprintf(stdout,"msgrcv error...\n");
	}
	printf("rev msg:%s\n",msg.mtext);

	return 0;
}

