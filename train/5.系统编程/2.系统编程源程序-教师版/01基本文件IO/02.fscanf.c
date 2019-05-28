#include <stdio.h>
#include <stdlib.h>

#define MAXLINE 1024

int main()
{
	char buf[MAXLINE];

	FILE* fp = fopen("file","r");
	if(fp == NULL){
		printf("file open failed\n");
		exit(-1);
	}
	int ret = fscanf(fp,"%s",buf);
	if(ret < 0){
		printf("fscanf error\n");
		exit(-1);
	}
	printf("buf = %s\n",buf);
	

	//用fscanf从终端读数据




























	scanf();

	return 0;
}
