#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>

/*
	void* shmat(int shmid,const void*shmaddr,int shmflag)
	第一个参数是共享内存的标志
	第二个参数是:
		映射该共享内存的进程内存地址(自己指定的时候,要确保使用的地址未被使用)
		如果为NULL,linux将自动选择合适的地址
	第三个参数:
		SHM_RND:当我们给定自己定义的地址的时候,如果指定系统帮我们去找，会从我们定义的
		地址开始找到可以使用的地址
		SHM_RDONLY:内存默认是读写的,如果加了这个标志就是只读属性
*/

[        |500                  ]

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
	sprintf(buf,"%s","hello world\n");
	
	return 0;
}
