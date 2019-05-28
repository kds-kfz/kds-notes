#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>

int main()
{
	int shmId = shmget(ftok("shm",1),1024,IPC_CREAT|0777);
	if(shmId < 0){
		fprintf(stdout,"shmget error...\n");
		exit(-1);
	}
	printf("shmget ok...\n");
	char *buf = shmat(shmId,NULL,SHM_RND);
	if(buf == NULL){
		fprintf(stdout,"shmat error...\n");
		exit(-1);
	}
	char *buf1 = "hello";
	char *buf2 = "world" ;
	sprintf(buf,"%s%s",buf1,buf2);

	return 0;
}
