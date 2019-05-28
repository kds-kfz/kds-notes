#include<stdio.h>
#include<stdlib.h>
//2.c库函数文件文件描述符.c
int main(){
    char buf[1024];
    FILE *fp=fopen("file1.txt","r");
    if(fp==NULL){
	printf("open error\n");
	exit(-1);
    }
    //从文件读取内容，遇到空格结束
    int ret=fscanf(fp,"%s",buf);
    if(ret<0){
	printf("fscanf fail\n");
	exit(-1);
    }
    printf("%s\n",buf);
    
    ret=fscanf(stdin,"%s %s",buf,&buf[10]);
    if(ret!=2){
	printf("stdin fail\n");
	exit(-1);
    }
    //从文件读出来的内容放在缓存区buf里
    //后面从终端stdin输入，读取内容也存到buf里
    //之前buf里的内容被清空,只有最新存入的内容
    printf("%s\n",buf);
    printf("%s\n",&buf[10]);

    fclose(fp);
    return 0;
}
