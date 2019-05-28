#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

struct info{
	int cfd;
	struct sockaddr_in caddr;
};

void* thread_handle(void* arg)
{
	int n;
	char ip_buf[40];
	char buf[256];
	struct info *ts = (struct info*)arg;
	printf("client:IP[%s]:PORT[%d]\n",inet_ntop(AF_INET,&(ts->caddr.sin_addr.s_addr),ip_buf,sizeof(ip_buf)),ntohs(ts->caddr.sin_port));
	while(1){
		n = read(ts->cfd,buf,sizeof(buf));
		if(n == 0){
			printf("客户单断开连接\n");
			return (void*)-1;
		}else if(n == -1){
			perror("read error");
			exit(-1);
		}
		write(STDOUT_FILENO,buf,strlen(buf));
		write(ts->cfd,"I'am server...\n",15);
	}
	close(ts->cfd);

}

int main(int argc, char* argv[])
{
	if(argc < 2){
		printf("./server less...\n");
		exit(-1);
	}

	int i = 0;
	struct info ts[250];

	pthread_t pth;
	int lfd/*用于监听*/,cfd/*与客户端进行数据交互*/;
	struct sockaddr_in saddr, caddr;
	socklen_t c_len;

	//创建socket套接字
	lfd = socket(AF_INET,SOCK_STREAM,0);

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(atoi(argv[1]));
	//saddr.sin_addr.s_addr = htonl(INADDR_ANY);
	inet_pton(AF_INET,"127.0.0.1",&saddr.sin_addr.s_addr);

	//绑定本地地址和端口
	bind(lfd,(struct sockaddr*)&saddr,sizeof(saddr));

	//监听
	listen(lfd,128);

	/*-------------------------------------------------------------------------*/

	while(1){
		c_len = sizeof(caddr);
		printf("before accept ...\n");
		cfd = accept(lfd,(struct sockaddr*)&caddr,&c_len);
		printf("------------accept ok-----------------\n");
		/*这里要将accept返回的cfd和客户端的地址及ip传个线程*/
		ts[i].cfd = cfd;
		ts[i].caddr = caddr;
		pthread_create(&pth,NULL,thread_handle,(void*)&ts[i]);
		pthread_detach(pth);

		i++;
		if(i >= 250)
			break;
	}
	
	close(lfd);
	return 0;
}
