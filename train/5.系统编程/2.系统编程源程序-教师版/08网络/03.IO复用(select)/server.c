#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <ctype.h>


#define SERV_PORT 6666

/*
     ---> maxi(-1)
[c1] ---> maxi(0)
[c2] ---> maxi(1)
[c3] ---> maxi(2)
[c4] ---> maxi(3)
[c5] ---> maxi(4)
[c6] ---> maxi(5)
[c7] ---> maxi(6)
FD_ISSET(c4,&rset)
*/

int main(int argc, char* argv[])
{
	int n;
	int i, maxi;	
	int lfd, cfd, sockfd;
	int maxfd;
	int nready, client[FD_SETSIZE];
	char buf[1024];
	char ip_buf[40];
	socklen_t c_len;
	fd_set rset, allset;/*定义两个集合,用于监听读文件描述符集合,allset这个集合也就是相当于临时变量的作用*/
	
	struct sockaddr_in caddr, saddr;

/*---------------------------------------------------------*/
	lfd = socket(AF_INET,SOCK_STREAM,0);
	
	/*将端口设置成可以复用*/
	int opt = 1;
	setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

	bzero(&saddr,sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(SERV_PORT);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);

	bind(lfd,(struct sockaddr*)&saddr,sizeof(saddr));
	listen(lfd,128);

/*----------------------------------------------------------*/	
	maxfd = lfd;/*起初的最大文件描述符即为lfd*/

	maxi = -1;/*用来指示client[]这个数组的下标,这个数组用来存储,当前可用的文件描述符,防止我们监听后遍历整个1024个容量的数组,减少工作量*/
	for(i=0; i<FD_SETSIZE; i++)
		client[i] = -1;/*这样可以保证初始的情况下,maxi指向的是首元素的上一个位置,也就是这里面暂时没有可用的文件描述符*/
	FD_ZERO(&allset);/*将文件描述符集合清零*/
	FD_SET(lfd,&allset);/*将lfd添加到描述符集合中,构造出select监听的集合*/

/*-----------------------------------------------------------*/	
	while(1){
		rset = allset;/*将allset赋值给rset,每次循环时,都要重新设置select监控的文件描述符集合*/
		nready = select(maxfd+1/*注意这里是最大文件描述符加1*/,&rset,NULL,NULL,NULL); 
		if(nready < 0){
			perror("select error");
			exit(-1);
		}
/*------------------------------------------------------*/		
		/*判断监听的集合中是否有lfd,即是否有新连接*/
		if(FD_ISSET(lfd,&rset)){
			c_len = sizeof(caddr);
			cfd = accept(lfd,(struct sockaddr*)&caddr,&c_len);
			printf("client:IP[%s]:PORT:[%d]\n",inet_ntop(AF_INET,&caddr.sin_addr.s_addr,ip_buf,sizeof(ip_buf)),ntohs(caddr.sin_port));

			for(i=0; i<FD_SETSIZE;i++)
				if(client[i] < 0){/*循环遍历找打client[]中没有使用的位置*/
					client[i] = cfd;
					break;
				}
			if(i == FD_SETSIZE){
				fprintf(stderr,"too many clients\n");
				exit(-1);
			}
			FD_SET(cfd,&allset);/*将返回的cfd,添加到文件描述符集合中,用于下次循环中监听*/
			if(cfd > maxfd)
				maxfd = cfd;
			if(i > maxi)
				maxi = i;/*数组中指示文件描述符的下标向后移动一位*/

			if(--nready == 0)
				continue;
		}
/*--------------------------------------------------------*/		
		for(i=0; i<= maxi; i++){
			/*注意理解,client[]这个数组中存放的是什么*/
			if((sockfd = client[i]) < 0)/*这行代码的意思是,可能有客户端断开连接,这样,它对应的描述符集合中的位置会被清零*/
				continue;
			/*用于判断监听的描述符集合中是否有客户端发送数据*/
			if(FD_ISSET(sockfd,&rset)){				
				if((n = read(sockfd,buf,sizeof(buf))) == 0){
					close(sockfd);
					FD_CLR(sockfd,&allset);
					client[i] = -1;
				}else if(n > 0){
					write(STDOUT_FILENO,buf,n);
					write(sockfd,"i'am server\n",12);
				}
				if(--nready == 0)
					break;
			}	
		}
		/*注意这里还没有出while循环,又回去继续区监听,此时的监听集合,可能已经改变*/
	}

	close(lfd);

	return 0;
}















