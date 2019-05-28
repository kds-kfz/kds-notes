#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <string.h>
#include <strings.h>

#include "threadpool.h"

#define SERV_PORT 8888
#define SERV_IP "127.0.0.1"

#define OPEN_MAX 5000

int main(int argc, char* argv[])
{
	int n;
	int ret, i;
	int lfd, cfd, sockfd;
	int epfd;/*epoll的操作句柄*/
	int nready;
	char buf[2048];
	socklen_t c_len;
	struct epoll_event ep,epv[OPEN_MAX];
	struct sockaddr* saddr, caddr;

	lfd = socket(AF_INET,SOCK_STREAM,0);
	if(lfd == -1){
		perror("socket error");
		exit(1);
	}

	bzero(&saddr,sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(SERV_PORT);
	inet_pton(AF_INET,SERV_IP,&saddr.sin_addr.s_addr);

	ret = bind(lfd,(struct sockaddr*)&saddr,sizeof(saddr));
	if(ret == -1){
		perror("bind error");
		exit(1);
	}
	ret = listen(lfd,128);
	if(ret == -1){
		perror("listen error");
		exit(1);
	}
	
	epfd = epoll_create(OPEN_MAX);
	if(epfd == -1){
		perror("epoll_create error");
		exit(1);
	}
	
	ep.events = EPOLLIN;
	ep.data.fd = lfd;
	ret = epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&ep);/*注册*/

	for(;;){
		nready = epoll_wait(epfd,epv,OPEN_MAX,-1);
		if(ret == -1){
			perror("epoll_wait error");
		}
		for(i=0; i<nready; i++){
			if(epv[i].data.fd == lfd){
				c_len = sizeof(caddr);
				cfd = accept(lfd,(struct sockaddr*)&caddr,&c_len);
				if(cfd == -1){
					perror("accept error");
				}

				ep.events = EPOLLIN;
				ep.data.fd = cfd;
				ret = epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&ep);
				if(ret == -1){
					perror("epoll_ctl error");
					exit(1);
				}
			}else if(epv[i].events & EPOLLIN){
				sockfd = epv[i].data.fd;
				n = read(sockfd,buf,sizeof(buf));
				if(n == 0){
					printf("@@@@@@客户端断开@@@@@@\n");
					
					ret = epoll_ctl(epfd,EPOLL_CTL_DEL,sockfd,&evp[i]);
					if(ret == -1){
						perror("epoll_ ctl error");
						exit(1);
					}
				}else if(n < 0){
					if(errno == EINTR)
						continue;
					else
						ret = epoll_ctl(epfd,EPOLL_CTL_DEL,sockfd,&evp[i]);
				}else {
					/*从客户端读数据*/
					/*1.登录; 2.注册,1.注册,3,注册*/
					buf--->提取信息
					void* process(void* arg)
					{
						/*提取buf的信息*/						
					}
					addThreadWork(process,arg);
					
					ep.events = EPOLLOUT;
					ep.data.fd = sockfd;
					ret = epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ep); 
				}
			
			}else if(epv[i].events & EPOLLOUT){
				sockfd = epv[i].data.fd;
				/*给客户端写数据*/			
				/*buf 可以是我们封装后,返回给客户端的信息*/
				ret = write(sockfd,buf,strlen(buf));
				if(ret == -1){
					perror("write error");
					exit(1);
				}
				ep.events = EPOLLIN;
				ep.data.fd = sockfd;
				ret = epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ep);
				if(ret == -1){
					perror("epoll_ctl error");
					exit(1);
				}				
			}
		}
	}
	return 0;	
}














