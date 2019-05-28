#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
#include <sys/mman.h>
//1.mmap-w.c
#if 0
    void *mmap(void *addr, size_t length, int prot, 
	    int flags,int fd, off_t offset);
    ----------------------------------------------------------------------------
    addr：表示建立内存映射区的，由linux内核制定，使用时，直接传递NULL
    ----------------------------------------------------------------------------
    length：欲创建映射取的大小
    (注意映射区的大小不能为0,也就是新创建的文件，要有大小，尽量不要用O_TRUNC)
    ----------------------------------------------------------------------------
    port：映射权限(要和open要一致)，4种参数如下
	PROT_EXEC：表示映射的这一段可执行，例如映射共享存储
	PROT_READ：表示映射的这一段可读
	PROT_WRITE：表示映射的这一段可写
	PROT_NONE：表示映射的这一段不可访问
    ----------------------------------------------------------------------------
    flag：标志为参数(常用于更新物理区域，设置共享，创建匿名映射区)，2种参数如下
	MAP_SHARED：会将映射区所做的操作反应到物理设备(磁盘上)
	多个进程对同一个文件的映射是共享的，一个进程对映射的内存做了修改，
	另一个进程也会看到这种变化
	MAP_PRIVATE：映射区所做的修改不会反应到物理设备上
	多个进程对同一个文件的映射不是共享的，一个进程对映射的内存做了修改，
	另一个进程并不会看到这种变化，也不会真的写到文件中去
    ----------------------------------------------------------------------------
    fd：用来建设映射区的文件描述符
    ----------------------------------------------------------------------------
    offset：映射文件偏移(4k)的正数倍[按页偏移]，每页4096字节
    ----------------------------------------------------------------------------
    注意：创建映射区大小必须小于或等于打开文件权限
	创建映射区的时候，隐含对文件的读操作
    ----------------------------------------------------------------------------
    返回值：成功返回磁盘文件映射在内存中的首地址，失败返回MAP_FAILED，
    当进程终止时，该进程的映射内存会自动删除，也可以调用munmap解除映射
    ****************************************************************************
    int munmap(void *addr, size_t length);
    参数1：磁盘文件映射在内存中的地址，参数2：文件大小
    解除磁盘文件映射在内存中的地址
    ----------------------------------------------------------------------------
    返回值：成功返回0,失败返回-1
#endif
int main(int argc,char *argv[]){
    if(argc<2){
	fprintf(stdout,"argc less\n");
	exit(-1);
    }
    int fd=open(argv[1],O_RDWR|O_CREAT,0640);
    if(fd==-1){
	printf("open fail\n");
	exit(-1);
    }
//    ftruncate(fd,4);
    ftruncate(fd,10);
//    int *mm=mmap(NULL,4,PROT_WRITE,MAP_SHARED,fd,0);
    char *mm=mmap(NULL,10,PROT_WRITE,MAP_SHARED,fd,0);
    if(mm==MAP_FAILED){
	printf("mmap fail\n");
	exit(-1);
    }
    close(fd);
    //*mm=65;
    char buf[]="1234567890";
    strcpy(mm,buf);
//    int ret=munmap(mm,4);
    int ret=munmap(mm,10);
    if(ret==-1){
	printf("munmap fail\n");
    }
    return 0;
}

