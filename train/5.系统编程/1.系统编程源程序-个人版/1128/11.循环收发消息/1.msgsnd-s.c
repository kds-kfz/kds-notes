#include<stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
//1.msgsnd-s.c
#if 0
    int msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg);
    struct msgbuf {
	long mtype;       /* message type, must be > 0 */
	char mtext[1];    /* message data */
    };
#endif
struct Stu{
    long mtype; 
    int id;
    char name[20];  
};
int main(){
    struct Stu msg={1,1000,"lisi"};
    int msqid=msgget(ftok("msg4.txt",1),IPC_CREAT|0777);
//    int msqid=msgget(ftok("msg4.txt",1),IPC_EXCL);
    if(msqid==-1){
	printf("msgget fail\n");
	exit(-1);
    }
    printf("msgget msqid = %d\n",msqid);
    while(1){
	int ret=msgsnd(msqid,&msg,sizeof(msg)-sizeof(long),1);
	msg.id++;
	if(ret==-1){
	    printf("msgsnd fail\n");
	}
	printf("2s after snd : id = %d,name = %s\n",msg.id,msg.name);
	sleep(2);
    }
    printf("wait msgsnd\n");
    return 0;
}
