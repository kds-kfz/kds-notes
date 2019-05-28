#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
/*
	第一个参数是key,IPC_PRIVATE由系统指定键值
	第二个参数是共享内存大小
	msgflg:
		1.IPC_CREAT
        2.IPC_EXECL
        3.权限
*/

int main()
{
	int shmId = shmget(ftok("shm",1),1024,IPC_CREAT|0777);
	if(shmId < 0){
		printf("shmget error...\n");
	}

	printf("shmget ok...\n");

	return 0;
}
