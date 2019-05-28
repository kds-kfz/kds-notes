#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<unistd.h>
#include<stdio.h>
#include<fcntl.h>
#include<stdlib.h>
#include<errno.h>

#include"function.h"
#include"font.h"
//function.c

//整型转数字字符
int int_to_ch(int m,char *dest,int size){
    memset(dest,0,size);
    char temp[10]="0123456789";
    int t=m;
    int i=0;

    for(;t!=0;i++)
	t/=10;
    i--;
    while(m){
	t=m%10;
	dest[i]=temp[t];
	m/=10;
	i--;
    }
    return 1;
}

//数字字符转整型
int ch_to_int(const char *src,int *m){
    *m=0;
    for(int i=0;src[i]!='\0';i++){
	if(src[i]>='0'&&src[i]<='9'){
	    *m*=10;
	    *m=(*m+src[i]-48);
	}
	else
	    return 0;
    }
    return 1;
}

//追加套接字，设置为非阻塞
void setnonblocking(int sock){
    int opts;
    opts=fcntl(sock,F_GETFL);
    if(opts<0){
	perror("fcntl(sock,GETFL)");
	exit(1);
    }
    opts = opts|O_NONBLOCK;
    if(fcntl(sock,F_SETFL,opts)<0){
	perror("fcntl(sock,SETFL,opts)");
	exit(1);
    }
}

//向套接字写入数据
int socket_write(int sockfd,char *buff,int len){
    int ret;
    int send_len = 0;
    while(1){
	ret = write(sockfd,buff+send_len,len-send_len);
	if(ret < 0){
	    if(errno == EINTR || errno == EAGAIN)
		continue;
	    else
		return -1;
	}
	send_len += ret;
	if(send_len == len)
	    break;
    }
    return send_len;
}

//读取套接字里数据
int socket_read(int sockfd,char *buff,int len){
    int ret;
    int recv_len=0;
    while(1){
	ret = read(sockfd,buff+recv_len,len-recv_len);
	if(ret < 0){
	    if(errno == EINTR || errno == EAGAIN)
		break;
	}
	recv_len += ret;
	if(recv_len == len)
	    break;
    }
    return recv_len;
}
void show_data(Custom *p){
    set_cursor(1,1);
    printf("%s,%s\n",p->account,p->number);
    printf("%s,%s\n",p->password,p->age);
    printf("%s,%s\n",p->telephone,p->sex);
    printf("%s,%s\n",p->address,p->status);
}

//界面显示
void show_menu(char *str){
    printf(cursor_right6"%s\n\n",str);
    printf(cursor_right2"1.注册\n\n");
    printf(cursor_right3"2.登录\n\n");
    printf(cursor_right4"3.退出\n\n");
    printf(cursor_right5"请输入您的选择:");
}
