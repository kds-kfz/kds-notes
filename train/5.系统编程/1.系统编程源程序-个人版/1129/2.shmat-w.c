#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
//2.shmat-w.c
#if 0
    void *shmat(int shmid, const void *shmaddr, int shmflg);
    shmid：共享存储id
    shmaddr：有以下3种情况
	1.是NULL，则此段连接到由内核选择的第一个可用地址上
	2.非空，且没有指定SHM_RND，则此段连接到shmaddr所指定的地址上
	3.非空，且指定了SHM_RND，则此段指向
	(shmaddr-(shmaddr mod SHMLBA))所表示的地址上
    shmflg：
	1.SHM_RDONLY，则以只读形式连接此段，否则以读写方式连接此段
	2.SHM_RND：当我们给定自己定义的地址的时候，
	如果指定系统帮我们去找，会从我们定义的地址开始找到可以使用的地址
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
    int *ret=shmat(shmid,NULL,SHM_RND);
    if(*ret==-1){
	printf("shmat fail\n");
	exit(-1);
    }
    printf("*ret = %d\n",*ret);
    char *buf=shmat(shmid,NULL,SHM_RND);
    if(buf==NULL){
	printf("shmat error\n");
	exit(-1);
    }
//    strcpy(buf,"hello world1");
//    sprintf(buf,"%s","hello world");
    char buf1[]="hello world2";
    memcpy(buf,buf1,sizeof(buf1));
    return 0;
}
