#include<stdio.h>
#include<stdlib.h>
#include<string.h>
//1.c库函数文件操作.c
int main(){

    char buf[1024];
    strcpy(buf,"hello world");
    FILE *fp=fopen("file1.txt","w+");
    if(fp==NULL){
	printf("open fail\n");
	exit(-1);
    }
    int ret=fprintf(fp,"%s",buf);
    if(ret<0){
	printf("fprintf error\n");
	exit(-1);
    }
    fprintf(stdout,"%s\n",buf);
    fprintf(stdout,"hello world\n");
    /*
       stdin 标准输入
       stdout 标准输出
       stderr 标准错误
       */

    fclose(fp);
    return 0;
}
