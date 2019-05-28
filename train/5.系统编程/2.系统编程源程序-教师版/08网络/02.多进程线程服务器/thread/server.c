#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>

#define SERV_PORT 8888

struct info{
	int cfd;
	struct sockaddr_in caddr;
};

void* thread_handle(void* arg)
{
	int ret;
	char buf[256];
	char ip_buf[20];
	struct info *info = (struct info*)arg;
	printf("client:ip[%s]:port[%d]\n",inet_ntop(AF_INET,&info->caddr.sin_addr.s_addr,ip_buf,sizeof(ip_buf)),ntohs(info->caddr.sin_port));

	while(1){
		ret = read(info->cfd,buf,sizeof(buf));
		if(ret == 0)
			printf("服务端断开连接\n");
		ret = write(STDOUT_FILENO,buf,ret);
		write(info->cfd,"I'am client\n",12);		
	}
	return	NULL;
}

int main()
{
	pthread_t pth;
	int i = 0;
	int ret;
	char buf[256];
	int lfd/*用于监听*/,cfd/*与客户端进行数据交互*/;
	struct info info[200];/*每台主机的线程有最大上限,代码参考线程创建的最后一个文件*/
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

	while(1){
		c_len = sizeof(caddr);
		printf("before accept ...\n");
		cfd = accept(lfd,(struct sockaddr*)&caddr,&c_len);
		printf("------------accept ok-----------------\n");
		/*这里要将accept返回的cfd和客户端的地址及ip传个线程*/
		info[i].cfd = cfd;
		info[i].caddr = caddr;
		
		pthread_create(&pth,NULL,thread_handle,(void*)&info[i]);
		pthread_detach(pth);/*将线程设置为分离态,注意和进程的回收相互比较*/
		close(cfd);
		i++;
	}

	return 0;
}
