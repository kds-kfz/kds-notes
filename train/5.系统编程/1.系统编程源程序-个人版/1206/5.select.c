#include<stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include<strings.h>
#include<stdlib.h>
#include<errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/select.h>
#include <sys/time.h>
#if 0
    
#endif
//#define FD_SETSIZE 1024

int main(int argc,char *argv[]){
    if(argc<2){
	perror("./a.out less");
	exit(-1);
    }
    int sfd,cfd;
    int ret;
    char buf[128];
    char ip_buf[40];
    socklen_t c_len;
    struct sockaddr_in saddr;
    struct sockaddr_in caddr;
    /*----------------------------------------------------------*/
    int maxi;
    int maxfd;
    int sockfd;
    fd_set rset,allset;
    int nread,client[FD_SETSIZE];
    /*----------------------------------------------------------*/


    sfd=socket(AF_INET,SOCK_STREAM,0);
    if(sfd==-1){
	perror("socket error\n");
	exit(-1);
    }
    
    //将端口设置为可以复用
    int opt=1;
    setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    /*----------------------------------------------------------*/

    bzero(&saddr,sizeof(saddr));
    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(atoi(argv[1]));
    inet_pton(AF_INET,"127.0.0.1",&saddr.sin_addr.s_addr);
    ret=bind(sfd,(struct sockaddr *)&saddr,sizeof(saddr));
    if(ret==-1){
	printf("bind error\n");
	exit(-1);
    }
    ret=listen(sfd,128);
    if(ret==-1){
	perror("listen fail\n");
	exit(-1);
    }

    /*----------------------------------------------------------*/
    maxfd=sfd;
    maxi=-1;
    int i;
    int x=0;
    for(i=0;i<FD_SETSIZE;i++)//将所有客户端数组初始化-1
	client[i]=-1;
    FD_ZERO(&allset);//将临时文件描述符集合所有位置0
    FD_SET(sfd,&allset);//第一次手动开启服务端套接字文件描述符
    /*----------------------------------------------------------*/
    while(1){
	rset=allset;
	nread=select(maxfd+1,&rset,NULL,NULL,NULL);//阻塞等待
	printf("maxfd+1 = %d\n",maxfd);
	printf("start nread = %d,maxfd = %d\n",nread,nread);
	if(nread==-1){//出错
	    perror("select error\n");
	    exit(-1);
	}
	/*----------------------------------------------------------*/
	if(FD_ISSET(sfd,&rset)){//当有新客户端接入才成立
	    x=0;
	    c_len=sizeof(caddr);
	    printf("serve is going\n");
	    cfd=accept(sfd,(struct sockaddr *)&caddr,&c_len);
	    printf("cfd = %d\n",cfd);
	    printf("client ip[%s],port[%d]\n",
		inet_ntop(AF_INET,&caddr.sin_addr.s_addr,ip_buf,sizeof(ip_buf)),
		ntohs(caddr.sin_port));
	    if(cfd==-1){
	    perror("accept error\n");
		exit(-1);
	    }
	    for(i=0;i<FD_SETSIZE;i++){
		if(client[i]<0){//找到不为-1的位置存cfd
		    client[i]=cfd;
		    printf("client[%d] = %d\n",i,client[i]);
		    break;
		}
	    }
	    if(i==FD_SETSIZE){
		printf("too many client\n");
		exit(-1);
	    }
	    FD_SET(cfd,&allset);//将新的套接字描述符添加到allset
	    if(cfd>maxfd)//如果新描述符比旧的大
		maxfd=cfd;
	    if(i>maxi)
		maxi=i;//更新client数组最大值下标
	    printf("maxi = %d,nread = %d\n",maxi,nread);
	    if(--nread==0)
		continue;
		//如果有新客户端接入时，返回1,nread--后继续轮循，
		//检测下依次是否有新的客户端接入，如果有继轮循，
		//如果没有，则是检测已将接入服务端的客户端是否有写数据，
		//有就让服务器读取数据并反发信息提示，说明服务器已经接到数据
		//若没有以上两种情况，则select是在阻塞等待以上两种情况的发生
	}
    /*----------------------------------------------------------*/
	for(i=0;i<=maxi;i++){//找client数组里新的或旧的文件描述符
	    x++;//统计次数
	    if((sockfd=client[i])<0){
		continue;
	    }
	    printf("maxi = %d--------i = %d--------sockfd = %d\n",maxi,i,sockfd);
	    printf("in fact socket = %d,x = %d\n",sockfd,x);//真正要操作的文件描述符
	    if(FD_ISSET(sockfd,&rset)){
		x=0;
		//判断该文件描述符是否开启，遍历client数组到下标为maxi止，
		//判断该位是否开启，开启则条件成立
		if((ret=read(sockfd,buf,sizeof(buf)))==0){
		    close(sockfd);
		    FD_CLR(sockfd,&allset);
		    client[i]=-1;
		    printf("客户端断开\n");
		}
		else if(ret>0){
		    write(STDOUT_FILENO,buf,ret);
		    write(sockfd,"I am client\n",sizeof("I am client\n"));
		}
		if(--nread==0)
		    break;
	    }
	}
	for(int j=0;j<10;j++)
	    printf("client[%d] = %d\n",j,client[j]);
    }
    close(sfd);
    return 0;
}
