#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>

/*
	void* shmat(int shmid,int cmd,struct shmid_ds *buf)
	第一个参数是共享内存的标志
	第二个参数是共享内存的操作
	IPC_STAT:读取
	IPC_SET:设置
	IPC_RMID:移除
*/

int main()
{
	int shmId = shmget(ftok("shm",1),1024,IPC_CREAT|0777);
	if(shmId < 0){
		printf("shmget error...\n");
	}

	printf("shmget ok...\n");
	int ret = shmat(shmId,NULL,SHM_RND);
	if(ret < 0){
		printf("shm error...\n");		
	}
	/*返回值就是代表成功映射的地址值*/
	char *buf = shmat(shmId,NULL,SHM_RND);
	if(buf == NULL){
		printf("shmat error...\n");		
	}
	shmdt(buf);//去掉映射
	//注意读写之前加互斥机制(文件锁或是信号量)
	printf("%s\n",buf);
	//解锁	
	ret = shmctl(shmId,IPC_RMID,0);
	if(ret < 0){
		printf("shmctl error...\n");
	}

	return 0;
}