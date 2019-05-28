#include<stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/epoll.h>
#if 0
    int setsockopt(int sockfd, int level, int optname,
	    const void *optval, socklen_t optlen);
    sockfd：
    level：
    optname：
    optval：
    optlen：
    返回值：

    int epoll_create(int size);
    int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
    int epoll_wait(int epfd, struct epoll_event *events,int maxevents, int timeout);

#endif

#define OPEN_MAX 5000

int main(int argc,char *argv[]){
    int sockfd;
    int efd,lfd,cfd;
    int ret;
    int nread;
    char ip_buf[128];
    char buf[1024];
    socklen_t c_len;

    struct sockaddr_in saddr;
    struct sockaddr_in caddr;

    struct epoll_event tep,ep[OPEN_MAX];
    lfd=socket(AF_INET,SOCK_STREAM,0);
    if(lfd==-1){
	perror("socket error\n");
	exit(-1);
    }
    int opt=1;
    ret=setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    if(ret==-1){
	perror("setsockopt error\n");
	exit(-1);
    }
    bzero(&saddr,sizeof(saddr));
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(atoi(argv[1]));
    inet_pton(AF_INET,"127.0.0.1",&saddr.sin_addr.s_addr);
    
    ret=bind(lfd,(struct sockaddr *)&saddr,sizeof(saddr));
    if(ret==-1){
	perror("bind error\n");
	exit(-1);
    }
    ret=listen(lfd,128);
    if(ret==-1){
	perror("listen error\n");
	exit(-1);
    }
    efd=epoll_create(OPEN_MAX);
    if(efd==-1){
	perror("epoll_creat error\n");
	exit(-1);
    }
    tep.events=EPOLLIN;
    tep.data.fd=lfd;
    ret=epoll_ctl(efd,EPOLL_CTL_ADD,lfd,&tep);
    if(ret==-1){
	perror("epoll_ctl error\n");
	exit(-1);
    }
    while(1){
	nread=epoll_wait(efd,ep,OPEN_MAX,-1);
	if(nread==-1){
	    perror("epoll_wait error\n");
	    exit(-1);
	}
	for(int i=0;i<nread;i++){
	    //新客户端
	    if(!(ep[i].events&EPOLLIN))
		continue;
	    if(ep[i].data.fd==lfd){
		c_len=sizeof(caddr);
		cfd=accept(lfd,(struct sockaddr *)&caddr,&c_len);
		if(cfd==-1){
		    perror("accept error\n");
//		    exit(-1);
		}
		printf("client ip[%s],port[%d]\n",
		    inet_ntop(AF_INET,&caddr.sin_addr.s_addr,ip_buf,sizeof(ip_buf)),
			ntohs(caddr.sin_port));
//		tep.events=EPOLLIN;
		tep.data.fd=cfd;
		ret=epoll_ctl(efd,EPOLL_CTL_ADD,cfd,&tep);
		if(ret==-1){
		    perror("new client epoll_ctl error\n");
		    exit(-1);
		}
	    }
	    //旧客户端
	    else if(ep[i].data.fd & EPOLLIN){
		sockfd=ep[i].data.fd;
		ret=read(sockfd,buf,sizeof(buf));
		if(ret==0){
		    ret=epoll_ctl(efd,EPOLL_CTL_DEL,sockfd,&ep[i]);
		    if(ret==-1)
			perror("delete client error\n");
		    close(sockfd);
		    printf("客户端断开\n");
		}
		else if(ret<0){
		    if(errno==EINTR)
			continue;
		    else{
			ret=epoll_ctl(efd,EPOLL_CTL_DEL,sockfd,&ep[i]);
			if(ret==-1)
			    perror("delete client error\n");
		    }
		}
		else{
		    /*
		    write(STDOUT_FILENO,buf,ret);
		    write(sockfd,"server receice data!\n",
			    sizeof("server receice data!\n"));
			    */
		    
		    add_task(work,buf);
		}
	    }
	    else if(ep[i].data.fd & EPOLLOUT){
		
	    }


	}
    }

    return 0;
}

