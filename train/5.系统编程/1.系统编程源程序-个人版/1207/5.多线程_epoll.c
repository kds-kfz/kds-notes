#include<sys/epoll.h>
#include<sys/types.h> 
#include<sys/socket.h>
#include<arpa/inet.h>
#include <pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<strings.h>
#include<string.h>
//5.多线程.epoll.c
#if 0
    int epoll_create(int size);
    size：用来告诉内核这个监听的数目一共有多大
    返回值：返回创建一个epoll的句柄
    ********************************************************************
    int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);    
    epfd：是epoll_create()的返回值
    ---------------------------------------------------------------------	
    op：表示动作
	EPOLL_CTL_ADD：注册新的fd到epfd中
	EPOLL_CTL_MOD：修改已经注册的fd的监听事件
	EPOLL_CTL_DEL：从epfd中删除一一个fd
    fd：需要监听的fd
    ---------------------------------------------------------------------	
    event：
    struct epoll_event {
	uint32_t     events;
	epoll_data_t data;
    };
    typedef union epoll_data {
	void        *ptr;
	int          fd;
	uint32_t     u32;
	uint64_t     u64;
    } epoll_data_t;
    ---------------------------------------------------------------------	
    events：
	EPOLLIN：表示示对应的文件描述符可以读
	EPOLLOUT：表示示对应的文件描述符可以写
	EPOLLERR：表示示对应的文件描述符发生生错误
    ---------------------------------------------------------------------	
    返回值：
    *********************************************************************
    int epoll_wait(int epfd, struct epoll_event *events,int maxevents, int timeout);
    epfd：是epoll_create()的返回值
    events：从内核得到事件的集合
    maxevents：告诉内核这个events有多大
    timeout：超时时间
	==-1：永久等待
	==0：不等待，测试所有描述符并立即返回
	>0：等待timeout毫秒，时间到期时立即返回
    返回值：准备就绪的描述符数目，超时返回0,错误返回-1
    ---------------------------------------------------------------------	

#endif
#define OPEN_MAX 128
typedef struct{
    int efd;
    int cfd;
    char cip[40];
    in_port_t cport;
}Epoll;

void *read_handle(void *arg){
    while(1){
    char buf[256];
    int ret;
    Epoll *poll=(Epoll *)arg;
	ret=read(poll->cfd,buf,sizeof(buf));
	if(ret==0){
	    ret=epoll_ctl(poll->efd,EPOLL_CTL_DEL,poll->cfd,NULL);
	    if(ret==0){
		printf("客户端断开\n");
		close(poll->cfd);
//		exit(-1);//当客户端断开后不要结束程序
	    }
	    else if(ret<0){
		if(errno==EINTR)
		    pthread_exit(NULL);	
	    }
	}
	else{
	    printf("客户端信息:\n");
	    printf("cfd = %d,cport = %d,cip = %s\n",
	    poll->cfd,poll->cport,poll->cip);
	    write(STDOUT_FILENO,buf,ret);
	    write(poll->cfd,"server receive data ok\n",
		    sizeof("server receive data ok\n"));
	    sleep(1);
	}
    }
//    pthread_exit(NULL);	
//    return NULL;
}

int main(int argc,char *argv[]){
    if(argc<2){
	perror("./a.out less");
	exit(-1);
    }
    int efd,lfd,cfd;
    int ret;
    char buf[128];
    char ip_buf[40];
    socklen_t c_len;
    struct sockaddr_in saddr;
    struct sockaddr_in caddr;
    /*----------------------------------------------------------*/
    int i;
    int sockfd;
    int nread;
    pthread_t pth;
    Epoll epoll[OPEN_MAX];
    struct epoll_event tep,ep[OPEN_MAX];
    /*----------------------------------------------------------*/
    lfd=socket(AF_INET,SOCK_STREAM,0);
    if(lfd==-1){
	perror("socket error\n");
	exit(-1);
    }
    //将端口设置为可以复用
    int opt=1;
    setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    /*----------------------------------------------------------*/
    bzero(&saddr,sizeof(saddr));
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(atoi(argv[1]));
    inet_pton(AF_INET,"127.0.0.1",&saddr.sin_addr.s_addr);
    ret=bind(lfd,(struct sockaddr *)&saddr,sizeof(saddr));
    if(ret==-1){
	printf("bind error\n");
	exit(-1);
    }
    ret=listen(lfd,128);
    if(ret==-1){
	perror("listen fail\n");
	exit(-1);
    }
    /*----------------------------------------------------------*/
    efd=epoll_create(OPEN_MAX);
    if(efd==-1){
	perror("epoll_create fail\n");
	exit(-1);
    }
    tep.events = EPOLLIN;
    tep.data.fd = lfd;
    ret=epoll_ctl(efd,EPOLL_CTL_ADD,lfd,&tep);
    if(ret==-1){
	printf("epoll_ctl fail\n");
	exit(-1);
    }
    /*----------------------------------------------------------*/
    while(1){
	nread=epoll_wait(efd,ep,OPEN_MAX,-1);
	if(nread==-1){
	    printf("epoll_wait fail\n");
	    exit(-1);
	}
	for(i=0;i<nread;i++){
	    if(!(ep[i].events &EPOLLIN))
		continue;
	    if(ep[i].data.fd==lfd){
		c_len=sizeof(caddr);
		cfd=accept(lfd,(struct sockaddr *)&caddr,&c_len);
		printf("client ip[%s],port[%d]\n",
		    inet_ntop(AF_INET,&caddr.sin_addr.s_addr,ip_buf,sizeof(ip_buf)),
		    ntohs(caddr.sin_port));
		
		epoll[i].efd=efd;
		epoll[i].cfd=cfd;
		strcpy(epoll[i].cip,ip_buf);
		epoll[i].cport=ntohs(caddr.sin_port);

		if(cfd==-1){
		perror("accept error\n");
		exit(-1);
		}
		tep.events=EPOLLIN;
		tep.data.fd=cfd;
		ret=epoll_ctl(efd,EPOLL_CTL_ADD,cfd,&tep);
		if(ret==-1){
		    printf("epoll_ctl error\n");
		    exit(-1);
		}
	    }
    /*----------------------------------------------------------*/
	    else{//当不是新客户端接入服务器时，需要处理旧客户端的读写请求
		pthread_create(&pth,NULL,read_handle,(void *)&epoll[i]);
//		pthread_detach(pth);
		if(i>=OPEN_MAX){
		    close(lfd);
		    exit(-1);
		}
		/*
		sockfd=ep[i].data.fd;
		ret=read(sockfd,buf,sizeof(buf));
		if(ret==0){
		    ret=epoll_ctl(efd,EPOLL_CTL_DEL,sockfd,NULL);
		    if(ret==0){
			printf("epoll_ctl fail\n");
//			exit(-1);//当客户端断开后不要结束程序
		    }
		    else if(ret<0){
			if(errno==EINTR)

		    }
		}
		else{
		    write(STDOUT_FILENO,buf,ret);
		    write(sockfd,"server receive data ok\n",
			    sizeof("server receive data ok\n"));
		}
		*/   
	    }
	}
    }
    return 0;
}
