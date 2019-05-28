#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
//3.shmat-r.c
#if 0
    void *shmat(int shmid, const void *shmaddr, int shmflg);
    shmid：
    shmaddr：
    shmflg：
    返回值：成功返回指向共享内存段的指针，错误返回-1
#endif
int main(){
    int shmid=shmget(ftok("shm1.txt",1),1024,IPC_CREAT|0777);
    if(shmid==-1){
	printf("shmget fail\n");
	exit(-1);
    }
    printf("shmget ok\n");
    printf("shmid = %d\n",shmid);
    char *buf=shmat(shmid,NULL,SHM_RND);
    if(buf==NULL){
	printf("shmat read error\n");
	exit(-1);
    }
    printf("buf = %s\n",buf);
    return 0;
}
