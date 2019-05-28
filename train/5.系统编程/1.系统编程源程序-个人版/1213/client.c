#include<stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include"function.h"

//client.c
#define CLIENT_PORT 8888

void client_register(int fd){
    printf("客户注册界面\n");
    Custom custom;
    Msg_head msghead;
    char buff[2048];
    int ret;

    memset(&custom, 0, sizeof(custom));
    printf("帐号：");
    scanf("%s",custom.account);
    printf("密码：");
    scanf("%s",custom.password);
    
    int c_len=sizeof(msghead)+sizeof(custom);
    int_to_ch(c_len,msghead.len,10);
    strcpy(msghead.msg_type,"#3");
    memcpy(buff, &msghead, sizeof(msghead));
    memcpy(buff + sizeof(msghead), &custom, sizeof(custom));
    ret = write(fd, buff, c_len);
    if(ret<0)
	perror("client_register write to server fail\n");
}

void client_login(int fd){
    printf("客户登录函数\n");
    Custom custom;
    Msg_head msghead;
    char buff[2048];
    int ret;

    memset(&custom, 0, sizeof(custom));
    printf("帐号：");
    scanf("%s",custom.account);
    printf("密码：");
    scanf("%s",custom.password);
    
    int c_len=sizeof(msghead)+sizeof(custom);
    int_to_ch(c_len,msghead.len,10);
    strcpy(msghead.msg_type,"#4");
    memcpy(buff, &msghead, sizeof(msghead));
    memcpy(buff + sizeof(msghead), &custom, sizeof(custom));
    ret = write(fd, buff, c_len);
    if(ret<0)
	perror("client_login write to server fail\n");
}

void *client_handler(void *arg){
    int *p=(int *)arg;
    int fd=*p;
    int choose;
    while(1){
	interface("客户端界面");
	scanf("%d",&choose);
	if(choose==1)
	    client_register(fd);
	else if(choose==2)
	    client_login(fd);
	else
	    printf("选择错误\n");
    }
}

int main(){
    int cfd;
    int ret;
    struct sockaddr_in caddr;
    char buf[512];
    char read_buf[512];

    pthread_t pth;

    cfd=socket(AF_INET,SOCK_STREAM,0);
    if(cfd==-1){
	perror("client socket error\n");
	exit(-1);
    }
    caddr.sin_family=AF_INET;
    caddr.sin_port=htons(CLIENT_PORT);
    //caddr.sin_addr.s_addr=htonl(INADDR_ANY);
    inet_pton(AF_INET,"127.0.0.1",&caddr.sin_addr.s_addr);
    ret=connect(cfd,(struct sockaddr *)&caddr,sizeof(caddr));
    if(ret==-1){
	perror("connect error\n");
	exit(-1);
    }

    ret = pthread_create(&pth, NULL, client_handler, &cfd);
    char buff[2048]={};
    Msg_head msghead;
    char result[10]={};
    Custom custom;

    if(ret!=0){
	printf("pthread_cteate error : %s\n",strerror(ret));
	exit(-1);
    }
    while(1){
	memset(buff, 0, 2048);
	ret=read(cfd, buff, 2048);
	if(ret<0){
	    perror("read fd error\n");
	    exit(-1);
	}
	else if(ret==0)
	    break;
	else{
	    write(STDOUT_FILENO,buff,ret);
	    memcpy(&msghead, buff, sizeof(msghead));
	    if(!strcmp(msghead.msg_type,"#3")){//注册
		memcpy(result, buff + sizeof(msghead), sizeof(result));
		if(!strcmp(result,"#1"))
		    printf("注册成功\n");
		else if(!strcmp(result,"#2"))
		    printf("注册失败\n");
	    }
	    else if(!strcmp(msghead.msg_type,"#4")){//登录
		memcpy(result, buff + sizeof(msghead), sizeof(result));
		if(!strcmp(result,"#1")){
		    printf("登录成功\n");
		    memcpy(&custom,buff+sizeof(msghead)+sizeof(result),sizeof(custom));
		    printf("%s,%s\n",custom.account,custom.password);
		}
		else if(!strcmp(result,"#2")){
		    printf("登录失败\n");
		    if(!strcmp(result,"!1"))
			printf("帐号错误\n");
		    else if(!strcmp(result,"!2"))
			printf("密码错误\n");
		    else if(!strcmp(result,"!3"))
			printf("帐号与密码错误\n");
		}
	    }
	}
    }
    pthread_join(pth, NULL);
    return 0;
}
