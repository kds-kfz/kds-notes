#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXLINE 1024

int main()
{	
	char buf[MAXLINE];
	strcpy(buf,"hello world");

	FILE* fp = fopen("file","w+");
	if(fp == NULL){
		printf("file open failed\n");\
		exit(-1);	
	}
	int ret = fprintf(fp,"%s",buf);
	if(ret < 0){
		printf("fprintf error\n");
		exit(-1);
	}		

/*
	stdin   标准输入
	stdout　标准输出
	stderr  标准错误
*/
	printf("--------------------\n");
	fprintf(stdout,"hello world\n");

	return 0;
}	
