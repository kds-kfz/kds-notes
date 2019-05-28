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
#include"font.h"

//client.c
#define CLIENT_PORT 8888

//读取服务器内容
//文件描述符+read_buf
//用于登录和注册发送数据
int read_server_data(int fd,char *type){
    int c_len;
    int ret;
    char read_buf[1024];
    char buf[512];
    Msg_head msghead;
//    while(1){
    memset(read_buf,0,1024);
    ret=read(fd, read_buf, 1024);
    
//    printf("ret=%d\n",ret);
    if(ret<0)
	return -1;
    else if(ret==0)
	return 0;
    else{//当读取成功，向服务器发送"#"
	memcpy(&msghead,read_buf,sizeof(Msg_head));//将读到的内容放到msghead
//	printf("msghead.result : %s\n",msghead.result);
	if(!strcmp(msghead.msg_type,type)){//是注册
	    if(!strcmp(msghead.result,"#1"))//成功
		return 1;//返回成功
	    else if(!strcmp(msghead.result,"#2"))
		return 2;//返回失败
	}
	else if(!strcmp(msghead.msg_type,type)){//是登录
	    if(!strcmp(msghead.result,"#1"))//成功
		return 1;//返回成功
	    else if(!strcmp(msghead.result,"#2"))
		return 2;//返回失败
	}
	/*
	else if(!strcmp(msghead.msg_type,"#5")){//是查询film表格
	    //处理read_buf内容
	    if

	    c_len=sizeof(Msg_head);
	    int_to_ch(c_len,m->len,sizeof(m->len));
	    strcpy(m->msg_type,"#");
	    memcpy(buf,m,sizeof(Msg_head));
	    ret=write(fd,buf,c_len);//向服务器发送"#"
	    }
	}
	*/
    }
}
//用于获取电影院信息
FILM *read_flim_data(int fd,FILM *head){
    char read_buf[1024];
    Msg_head msghead;
    Film d1;	    
    int ret;
    while(1){
    ret=read(fd, read_buf, 1024);
    if(ret<0){
	perror("read fd error\n");
	return NULL;
    }
    else if(ret==0)
	break;
    else{
	memset(&d1,0,sizeof(Film));
	memcpy(&msghead,read_buf,sizeof(Msg_head));//将读到的内容放到msghead
	if(!strcmp(msghead.result,"*1")){
    
	    memcpy(&d1,read_buf+sizeof(Msg_head),sizeof(Film));
	    d1.film.cinema_number=ntohl(d1.film.cinema_number);
	    d1.film_num=ntohl(d1.film_num);

	    head=create_film_link(head,d1);
	    memset(&msghead,0,sizeof(msghead));
	}
	else if(!strcmp(msghead.result,"*0"))//无效信息
	    break;
	}
    }
    return head;
}

//用于获取电影电影院
MOVIE *read_movie_data(int fd,MOVIE *head){
    char read_buf[2048];
    Msg_head msghead;
    Movie d1;	    
    int ret;
    while(1){
    ret=read(fd, read_buf, 1024);
    if(ret<0){
	perror("read fd error\n");
	return NULL;
    }
    else if(ret==0)
	break;
    else{
	memset(&d1,0,sizeof(Movie));
	memcpy(&msghead,read_buf,sizeof(Msg_head));//将读到的内容放到msghead
	printf("result %s\n",msghead.result);
	if(!strcmp(msghead.result,"*1")){
    
	    memcpy(&d1,read_buf+sizeof(Msg_head),sizeof(Movie));
	    
	    d1.movie_number=ntohl(d1.movie_number);
	    d1.c_number=ntohl(d1.c_number);
	    d1.movie_price=ntohl(d1.movie_price);
	    d1.ticket_count=ntohl(d1.ticket_count);
	    d1.play_yard=ntohl(d1.play_yard);

	    head=create_movie_link(head,d1);
	    memset(&msghead,0,sizeof(msghead));
	}
	else if(!strcmp(msghead.result,"*0"))//无效信息
	    break;
	}
    }
    return head;
}

//向服务器发送内容
//文件描述符+包头+模式
int send_client_data(int fd,void *arg,char *type,int num){
    int c_len;
    char buf[1024]={};
    int ret;
    Msg_head msghead;
    memset(&msghead,0,sizeof(Msg_head));
    //注册：#3，登录：#4，查看电影院：#5，查看该电影院所有电影：#6
    if(!strcmp(type,"#3")){//注册
	c_len=sizeof(Msg_head)+sizeof(Custom);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
	memcpy(buf+sizeof(Msg_head),(Custom *)arg,sizeof(Custom));
    }
    else if(!strcmp(type,"#4")){//登录，只需客户端入的是：帐号和密码
	c_len=sizeof(Msg_head)+sizeof(Custom);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
	memcpy(buf+sizeof(Msg_head),(Custom *)arg,sizeof(Custom));
    }
    else if(!strcmp(type,"#5")){//查看电影院，只发命令
	c_len=sizeof(Msg_head);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
    }
    else if(!strcmp(type,"#6")){//查看该电影院所有电影，只发命令
	c_len=sizeof(Msg_head);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	int_to_ch(num,msghead.result,sizeof(msghead.result));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
    }

    ret=write(fd,buf,c_len);
    if(ret<0)
	return 0;//发送失败
    else
	return 1;//发送成功
}

int main(){
    int cfd;
    int ret;
    struct sockaddr_in caddr;

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

    if(ret!=0){
	printf("pthread_cteate error : %s\n",strerror(ret));
	exit(-1);
    }
    /*-------------------------------------------------------*/
    Custom custom={"1003","popo","133","55","女","12345678901","苏浙","100","client"};
    Custom *p2=&custom;

    
    printf("已发送数据\n");
    send_client_data(cfd,p2,"#3",0);
    ret=read_server_data(cfd,"#3");
    if(ret==1)
	printf("注册成功\n");
    else
	printf("注册失败\n");
    
    send_client_data(cfd,p2,"#4",0);
    ret=read_server_data(cfd,"#4");
    if(ret==1)
	printf("登录成功\n");
    else
	printf("登录失败\n");

    FILM *head1=NULL;
    send_client_data(cfd,NULL,"#5",0);
    head1=read_flim_data(cfd,head1);
    printf_film_link(head1);
    free_film_link(head1);

    MOVIE *head2=NULL;
    send_client_data(cfd,NULL,"#6",3001);
    head2=read_movie_data(cfd,head2);
    printf_movie_link(head2);
    free_movie_link(head2);
//    while(1);

    return 0;
}
