#include<stdio.h>
#include<stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
//1.shmget.c
#if 0
    int shmget(key_t key, size_t size, int shmflg);
    key：
    size：
    shmflg：
    返回值：成功返回共享存储id，错误返回-1
#endif
int main(){
    int shmid=shmget(ftok("shm1.txt",1),1024,IPC_CREAT|0777);
    if(shmid==-1){
	printf("shmget fail\n");
	exit(-1);
    }
    printf("shmget ok\n");
    printf("shmid = %d\n",shmid);
    return 0;
}
