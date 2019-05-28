#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <mysql/mysql.h>

#define MAXLINE 1024
//#define OPEN_MAX 100
#define LISTENQ 10000
#define SERV_PORT 5000
#define INFTIM 1000

int clientFd[1000]={0};


typedef struct msginfo
{
    int fd;
    char buff[1024];
    struct msginfo * next;
}NODE;

typedef struct msg
{
    int fd;
    char buff[1024];
}MSG;

typedef struct msghead
{
    int len;/*总体数据长度*/
    int msgtype;
}MSGHEAD_T;

typedef struct myregister{
    char account[20];
    char passwd[20];
}REGISTER_T;

NODE *H = NULL;
pthread_mutex_t lock_h;
void setnonblocking(int sock)
{
    int opts;
    opts=fcntl(sock,F_GETFL);
    if(opts<0)
    {
	perror("fcntl(sock,GETFL)");
	exit(1);
    }
    opts = opts|O_NONBLOCK;
    if(fcntl(sock,F_SETFL,opts)<0)
    {
	perror("fcntl(sock,SETFL,opts)");
	exit(1);
    }
}


void clear_clientFd(int socketFd)
{
    int i;
    for(i = 0; i<1000; i++)
    {
	if(clientFd[i] == socketFd)
	{
	    clientFd[i] = 0;
	}
    }

    return;
}

int socket_write(int sockfd, char *buff, int len)
{
    int ret;
    int sendLen = 0;

    while(1)
    {
	ret = write(sockfd, buff + sendLen, len - sendLen);
	if(ret < 0)
	{
	    if(errno == EINTR || errno == EAGAIN)
	    {
		continue;
	    }
	    else
	    {
		return -1;
	    }
	}

	sendLen += ret;
	if(sendLen == len)
	{
	    break;
	}
    }
}

int socket_read(int sockfd, char *buff, int len)
{
    int ret;
    int recvLen = 0;

    while(1)
    {
	ret = read(sockfd, buff+recvLen, len - recvLen);

	if(ret < 0)
	{
	    if(errno == EINTR || errno == EAGAIN)
	    {
		break;
	    }
	}
	recvLen += ret;
	if(recvLen == len)
	{
	    break;
	}
    }
    return recvLen;
}

void send_msg_result(int fd, int result, int msgtype,REGISTER_T *pRe)
{
    char buff[1024] = {0};
    MSGHEAD_T msghead;
    int ret;
    int len;

    len = sizeof(msghead) + 4 + sizeof(*pRe);
    msghead.len = len;
    msghead.msgtype = msgtype;

    msghead.len = htonl(msghead.len);
    msghead.msgtype = htonl(msghead.msgtype);

    memcpy(buff, &msghead, sizeof(msghead));

    int temp;
    temp = result;
    temp = htonl(temp);
    memcpy(buff + sizeof(msghead), &temp, 4);

    memcpy(buff+sizeof(msghead)+4,pRe,sizeof(*pRe));
    //完成发送数据的组装

    ret = write(fd, buff, len);
    if(ret < 0)
    {
	perror("write send");
    }
    else
    {
	printf("send a result ok\n");
    }


}

void myregister(int fd, REGISTER_T * pRe)
{
    MYSQL db;
    MYSQL *pdb;
    int ret;
    /*
       mysql_init(&db);

       pdb = mysql_real_connect(&db, "localhost", "root", "123", "stu", 3306, NULL, 0);
       if(pdb == NULL)
       {
       perror("mysql_real_connect");
    //		send_msg_result(fd, 1, 2);
    return;
    }

    char sql[1024] = {0};
    sprintf(sql, "insert into student values(\'%s\', \'%s\');", pRe->account, pRe->passwd);

    ret = mysql_query(pdb, sql);
    if(ret == 0)
    {
    printf("insert ok\n");
    //		send_msg_result(fd, 0, 2);
    }
    else
    {
    printf("insert error\n");
    //		send_msg_result(fd, 1, 2);
    }

    mysql_close(pdb);
     */
    return;

}

void login(int fd, REGISTER_T *pRe)
{
    /*
       MYSQL db;
       MYSQL *pdb;

       mysql_init(&db);

       pdb = mysql_real_connect(&db, "localhost", "root", "123", "stu", 3306, NULL, 0);
       if(pdb == NULL)
       {
       perror("mysql_real_connect");
       send_msg_result(fd, 1, 4,pRe);
       return;
       }

       char sql[1024];
       sprintf(sql, "select * from student where account = \'%s\';", pRe->account);

       int ret;

       ret = mysql_query(pdb, sql);
       if(ret < 0)
       {
       printf("select error\n");
       send_msg_result(fd, 1, 4,pRe);
       mysql_close(pdb);
       return;
       }
       MYSQL_RES *res;


       res = mysql_store_result(pdb);

       int rows;

       rows = mysql_num_rows(res);

       REGISTER_T myre;
       if(rows == 0)
       {
       printf("hhhh\n");
       send_msg_result(fd, 1, 4,pRe);//1:用户名出错   2：密码出错
       }
       else
       {
       printf("ffff\n");
       MYSQL_ROW result;
       result = mysql_fetch_row(res);
       if(strcmp(pRe->passwd, result[1]) == 0)
       {
       strcpy(myre.account,result[0]);
       strcpy(myre.passwd,result[1]);
    //		memcpy(&myre,result,sizeof(myre));
    //		printf("%ld\n",sizeof(result));
    //		printf("%ld\n",sizeof(myre));
    //		printf("%s  %s",myre.account,myre.passwd);
    send_msg_result(fd, 0, 4,&myre);
    }
    else
    {
    send_msg_result(fd, 2, 4,pRe);
    }

    }

    mysql_close(pdb);
     */
    return;
}

void *handler(void* arg)
{
    NODE *p;

    MSGHEAD_T msghead;
    REGISTER_T myre;
    while(1)
    {
	pthread_mutex_lock(&lock_h);
	if(H != NULL)
	{
	    p = H;
	    H = H->next;
	    pthread_mutex_unlock(&lock_h);

	    memcpy(&msghead, p->buff, sizeof(msghead));
	    msghead.len = ntohl(msghead.len);
	    msghead.msgtype = ntohl(msghead.msgtype);

	    if(msghead.msgtype == 1)
	    {
		memcpy(&myre, p->buff + sizeof(msghead),  sizeof(myre));
		myregister(p->fd, &myre);
	    }
	    else if(msghead.msgtype == 3)
	    {
		memcpy(&myre, p->buff + sizeof(msghead), sizeof(myre));
		login(p->fd, &myre);
	    }

	    free(p);

	}
    }
}

void *handlerA(void * arg)
{
    MSG *p = (MSG *)arg;
    MSGHEAD_T msghead;
    REGISTER_T myre; 

    memcpy(&msghead, p->buff, sizeof(msghead));
    msghead.len = ntohl(msghead.len);
    msghead.msgtype = ntohl(msghead.msgtype);

    if(msghead.msgtype == 1)//注册
    {
	memcpy(&myre, p->buff + sizeof(msghead), sizeof(myre));
	myregister(p->fd, &myre);
    }
    else if(msghead.msgtype == 3)//登录
    {
	memcpy(&myre, p->buff + sizeof(msghead), sizeof(myre));
	login(p->fd, &myre);
    }

    return NULL;

}
int main(int argc, char* argv[])
{
    int i, maxi, listenfd, connfd, sockfd,epfd,nfds, portnumber;
    ssize_t n;
    char line[MAXLINE];
    socklen_t clilen;

    //STU_T stu;
    MSGHEAD_T msghead;
    REGISTER_T myre;

    if ( 2 == argc )
    {
	if( (portnumber = atoi(argv[1])) < 0 )
	{
	    printf("para erro\n");
	    return 1;
	}
    }
    else
    {
	printf("para erro\n");
	return 1;
    }

    pool_init(5);
    pthread_mutex_init(&lock_h, NULL);
    struct epoll_event ev,events[20];

    struct sockaddr_in clientaddr;
    struct sockaddr_in serveraddr;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);

    clilen = sizeof(clientaddr);


    epfd = epoll_create(10000);

    ev.data.fd=listenfd;
    ev.events=EPOLLIN|EPOLLET;

    epoll_ctl(epfd,EPOLL_CTL_ADD,listenfd,&ev);
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    char *local_addr="127.0.0.1";
    inet_aton(local_addr,&(serveraddr.sin_addr));//htons(portnumber);

    serveraddr.sin_port=htons(portnumber);
    bind(listenfd,(struct sockaddr *)&serveraddr, sizeof(serveraddr));
    listen(listenfd, LISTENQ);//10000  3000
    int j;
    pthread_t pid;
    int ret;


    MSG *p;

    for (; ; ) {

	nfds=epoll_wait(epfd,events,20,500);//1：有新连接 2：管道里面有数据
	for(i=0;i<nfds;++i)
	{
	    printf("@@@@@@@\n");
	    if(events[i].data.fd==listenfd)//处理新连接
	    {
		printf("start to connnet a new client\n");
		connfd = accept(listenfd,(struct sockaddr *)&clientaddr, &clilen);
		//生成一个新的客户端的描述符
		if(connfd<0){
		    perror("connfd<0");
		    exit(1);
		}

		//				setnonblocking(connfd);
		for(j = 0; j<1000; j++)
		{
		    if(clientFd[j] == 0)
		    {
			clientFd[j] = connfd;
			break;
		    }
		}
		char *str = inet_ntoa(clientaddr.sin_addr);
		printf("accept a connection from %s port=%d\n", str,ntohs(clientaddr.sin_port));

		ev.data.fd=connfd;

		ev.events=EPOLLIN|EPOLLET;


		epoll_ctl(epfd,EPOLL_CTL_ADD,connfd,&ev);
		printf("connect a new clinet ok, fd = %d\n", connfd);
	    }
	    else if(events[i].events&EPOLLIN)
	    {
		printf("recv a new msg\n");
		if ( (sockfd = events[i].data.fd) < 0)
		    continue;
		if ( (n = read(sockfd, line, MAXLINE)) < 0) {
		    if (errno == ECONNRESET) {
			clear_clientFd(sockfd);
			close(sockfd);
			events[i].data.fd = -1;
			continue;
		    } else
		    {
			printf("readline error\n");
			clear_clientFd(sockfd);
			continue;
		    }

		} else if (n == 0) {
		    close(sockfd);
		    events[i].data.fd = -1;
		    clear_clientFd(sockfd);
		    continue;
		}

		p = (MSG*)malloc(sizeof(MSG));
		memset(p, 0, sizeof(MSG));
		memcpy(p->buff, line, n);
		p->fd = sockfd;

		pool_add_worker(handlerA, p);

		ev.data.fd=sockfd;

		ev.events=EPOLLIN|EPOLLET;
		epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);

	    }
	    else if(events[i].events&EPOLLOUT)//通知管道可写
	    {
		sockfd = events[i].data.fd;
		write(sockfd, line, n);

		ev.data.fd=sockfd;

		ev.events=EPOLLIN|EPOLLET;

		epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
	    }
	}
    }
    return 0;
}
