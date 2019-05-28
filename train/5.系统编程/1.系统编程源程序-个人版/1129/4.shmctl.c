#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
//4.shmctl.c
#if 0
    int shmctl(int shmid, int cmd, struct shmid_ds *buf);
    shmid：共享存储id
    cmd：共享内存的操作
	1.IPC_STAT:读取
	2.IPC_SET:设置
	3.IPC_RMID:移除
    buf：
    返回值：成功返回0，错误返回-1
#endif
int main(){
    int shmid=shmget(ftok("shm",1),1024,IPC_EXCL);
    if(shmid==-1){
	printf("shmget fail\n");
	exit(-1);
    }
    printf("shmget ok\n");
    printf("shmid = %d\n",shmid);
    //移除
    int ret=shmctl(shmid,IPC_RMID,NULL);
    if(ret==-1){
	printf("shmctl error\n");
	exit(-1);
    }
    printf("remove shmid ok\n");
    return 0;
}
