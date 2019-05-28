#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
//5.work-stu.c
/*
 O_RDONLY   只读
 o_WRONLY   只写
 O_CREART   创建（文件加权限）
 O_TRUNC    清空
 O_EXCL	    判断文件是否存在
 O_APPEND   追加写
 O_RDWR	    读写
 stdin	STDIN_FIFONO	标准输入
 stdout	STDOUT_FIFENO	标准输出
 stderr	STDERR_FIFENO	标准错误
 */
//练习：从终端输入保存到文件，从文件读出数据并打印
struct stu{
    int id;
    char name[20];
};
int main(){
    struct stu d1;
    struct stu d2;
    char buf[512];
    fscanf(stdin,"%d %s",&d1.id,d1.name);
//    int fd=open("stu.txt",O_RDWR|O_TRUNC|O_CREAT,0640);
    int fd=open("stu.txt",O_RDWR|O_APPEND,0640);
    if(fd==-1){
	printf("open error\n");
	exit(-1);
    }
//    strcpy(buf,);
    int ret=write(fd,&d1,sizeof(d1));
    if(ret==-1){
	printf("write error\n");
	exit(-1);
    }
    lseek(fd,0,SEEK_SET);
    while(ret==sizeof(d2)){
    ret=read(fd,&d2,sizeof(d2));
    if(ret==-1){
	printf("read error\n");
	close(fd);
	exit(-1);
    }
    printf("id=%d,name=%s\n",d2.id,d2.name);
    }
    return 0;
}
