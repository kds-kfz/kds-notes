#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define SERV_PORT 8888

void fun(int sig)
{
	wait(NULL);
	//while(waitpid(-1,NULL,WNOHANG));
	return;
}

int main()
{
	pid_t pid;
	int ret;
	char ip_buf[40];
	char buf[256];
	int lfd/*用于监听*/,cfd/*与客户端进行数据交互*/;
	struct sockaddr_in saddr, caddr;
	socklen_t c_len;

	//创建socket套接字
	lfd = socket(AF_INET,SOCK_STREAM,0);
	
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(SERV_PORT);
	//saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	inet_pton(AF_INET,"127.0.0.1",&saddr.sin_addr.s_addr);

	//绑定本地地址和端口
	bind(lfd,(struct sockaddr*)&saddr,sizeof(saddr));

	//监听
	listen(lfd,128);

/*-------------------------------------------------------------------------*/

	//创建子进程用来处理任务(数据的读写),父进程负责处理连接请求
	while(1){
		c_len = sizeof(caddr);
		printf("before accept ...\n");
		
		cfd = accept(lfd,(struct sockaddr*)&caddr,&c_len);
		
		printf("------------accept ok-----------------\n");
		printf("client:ip[%s]:port[%d]\n",inet_ntop(AF_INET,&caddr.sin_addr.s_addr,ip_buf,sizeof(ip_buf)),ntohs(caddr.sin_port));
		
		pid = fork();
		if(pid < 0){
			printf("fork error...\n");
		}else if(pid > 0){
			//父进程
			close(cfd);
			signal(SIGCHLD,fun);
			
		}else if(pid == 0){
			//子进程
			close(lfd);
			break;
		}
	}

	if(pid == 0){
		while(1){
			ret = read(cfd,buf,sizeof(buf));
			if(ret == 0)
				printf("服务端断开连接\n");
			ret = write(STDOUT_FILENO,buf,ret);
			write(cfd,"I'am client\n",12);		
		}
	}
	return 0;
}
