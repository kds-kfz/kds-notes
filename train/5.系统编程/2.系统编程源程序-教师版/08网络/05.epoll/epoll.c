/*用线程池来处理*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>

#define SERV_PORT 8888
#define OPEN_MAX  1000

/*每有一个新的客户端请求*/
typedef struct msg{
	char buf[1024];
	struct msg* next;
}MSG;


int main()
{
	int ret;
	int n;
	int lfd, cfd, sockfd;/*socket返回的文件描述符*/
	struct sockaddr_in caddr, saddr;/*用于绑定本地的和接收客户端的*/
	socklen_t c_len = sizeof(caddr);
	/*-用于epoll的操作定义在这里-----------------*/
	int i;
	int nready;
	int epfd;/*epoll 的操作句柄*/
	struct epoll_event ep,epv[OPEN_MAX];

	/*-------------------------------------------*/
	
	poll_init(10);

	lfd = socket(AF_INET,SOCK_STREAM,0);
	if(lfd == -1){
		perror("socket error\n");
		exit(1);
	}

	bzero(&saddr,sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(SERV_PORT);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);

	ret = bind(lfd,(struct sockaddr*)&saddr,sizeof(saddr));
	if(ret == -1){
		perror("bind error");
		exit(1);
	}
	listen(lfd,128);
	
	
	epfd = epoll_create(OPEN_MAX);/*创建一个epoll句柄*/
	
	ep.events =	EPOLLIN;
	ep.data.fd = lfd;
	ret = epoll_ctl(epfd,EPOLL_CTL_ADD,lfd,&ep);/*注册*/

	for(;;){
		nready = epoll_wait(epfd,epv,OPEN_MAX,-1);
		for(i=0; i<nready; i++){
			if(epv[i].data.fd == lfd){
				cfd = accept(lfd,(struct sockaddr*)&caddr,&c_len);				
				printf("@@@@@@@@@@\n");
				ep.events = EPOLLIN;
				ep.data.fd = cfd;
				ret = epoll_ctl(epfd,EPOLL_CTL_ADD,cfd,&ep);
				if(ret < 0){
					perror("epoll_ctl error");					
					exit(1);
				}
			}else if(epv[i].events == EPOLLIN){
				
				n = read();
				if(n == 0){
					epoll_ctl(epfd,EPOLL_CTL_DEL,epv[i].data.fd,&epv[i]);
				}else if(n < 0){
					if(errno == EINTR){
						continue;
					}				
				}else {
					MSG* p = (MSG*)malloc(sizeof(MSG));
					
					
				}
		}else if(evp[i].events == EPOLLOUT){
				sockfd = evp[i].data.fd;	
				
				write(sockfd,,);

				evp[i].enents = EPOLLIN;
				epoll_ctl(epfd,EPOLL_CTL_MOD,evp[i].data.fd,&evp[i]);
			}		
		}/*nready--的终止*/
	}/*最外层for(;;)*/
		
	return 0;
}







