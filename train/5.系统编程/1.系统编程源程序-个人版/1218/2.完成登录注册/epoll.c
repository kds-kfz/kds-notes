#include<stdio.h>
#include<sys/epoll.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<arpa/inet.h>
#include<sys/select.h>
#include<sys/epoll.h>
#include <pthread.h>

//#include<mysql/mysql.h>
#include"thread_pool.h"
#include"function.h"
#include"Mysql.h"
//epoll.c

#define SERVER_PORT 8888
#define OPEN_SIZE 5000

Mysql_t *mysql;

void *client_task(void *arg){
    Total_msg *p=(Total_msg *)arg;
    Msg_head msghead;
    Custom *custom=(Custom *)malloc(sizeof(Custom));
    Custom *c;

    memset(&msghead,0,sizeof(Msg_head));
    memcpy(&msghead, p->buff, sizeof(msghead));

    printf("%s\n",msghead.msg_type);

    //服务器回馈客户
    char send_buf[2048]={};

//    char result[10]={};
    int ret;

    if(!strcmp(msghead.msg_type,"#3")){
	memset(custom,0,sizeof(Custom));
	memcpy(custom, p->buff + sizeof(msghead), sizeof(Custom));
	c=custom;
	printf("new client 注册 帐号:%s,密码:%s\n",custom->account,custom->password);
//	show_data(custom);//测试，显示客户结构体所有内容
//	printf("注册成功\n");
    
    //回馈客户内容
    memset(&msghead,0,sizeof(msghead));
    int c_len=sizeof(msghead)+sizeof(Custom);
    int_to_ch(c_len,msghead.len,sizeof(msghead.len));
    
    
    //此时需要读取数据库信息，
    //判断新用户是否已存在，不存在，则存到数据库，注册成功，否则注册失败
    //查找数据库后再确定结果值
    char read_table=check_person(mysql,custom);
    printf("read_table : %c\n",read_table);
    if(read_table=='0')
	strcpy(msghead.result,"#1");//表格没有该数据，允许注册
    else	
	strcpy(msghead.result,"#2");

//    strcpy(msghead.result,"#1");
    strcpy(msghead.msg_type,"#3");

    memcpy(send_buf, &msghead, sizeof(msghead));
    memcpy(send_buf+sizeof(msghead), custom, sizeof(Custom));
    
    if(read_table!='1')//插入数据
	insert_person(mysql,custom);
    
    ret = write(p->cfd,send_buf, c_len);
    if(ret<0){
	printf("服务器回馈客户信息失败\n");
	sleep(2);
    }
    }
    if(!strcmp(msghead.msg_type,"#4")){
	//此时需要读取数据库
	//判断该用户是否已经注册
	//如已注册，则判断帐号和密码与数据库的是否一致
	//一致则登录成功
	//否则登录失败
	//printf("数据库查询，请等待 ...\n");
	
	memset(custom,0,sizeof(Custom));
	memcpy(custom, p->buff + sizeof(msghead), sizeof(Custom));
	memset(&msghead,0,sizeof(msghead));
	int c_len=sizeof(msghead)+sizeof(Custom);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	
	char *test_table=test_new_custom(mysql,custom);
	printf("test_table %s\n",test_table);
	if(!strcmp(test_table,"#1"))
	    strcpy(msghead.result,"#1");
	else if(!strcmp(test_table,"#2"))
	    strcpy(msghead.result,"#2");

//	strcpy(msghead.result,"#2");
	strcpy(msghead.msg_type,"#4");

	//查找数据库后再确定结果值
    //    strcpy(result,"#1");
	memcpy(send_buf, &msghead, sizeof(msghead));
	memcpy(send_buf+sizeof(msghead), custom, sizeof(Custom));

	ret = write(p->cfd,send_buf, c_len);
	if(ret<0){
	    printf("服务器回馈客户信息失败\n");
	    sleep(2);
	}
    }
    sleep(1);
//    return NULL;
}

#if 1
int main(){
    mysql=(Mysql_t *)malloc(sizeof(Mysql_t));
    Mysql_init(mysql);
    Mysql_connect(mysql,"project");//链接已创建好的数据库

    
    int sfd,cfd;
    int ret;
    struct sockaddr_in saddr;
    struct sockaddr_in caddr;
    socklen_t c_len;

    int i;
    int efd;
    int sockfd;
    int nread;
    char line_buf[2048];
    char ip_buf[INET_ADDRSTRLEN];
    struct epoll_event tep,ep[OPEN_SIZE];
    
    sfd=socket(AF_INET,SOCK_STREAM,0);
    if(sfd==-1){
	perror("socket error\n");
	exit(-1);
    }

    int opt=1;
    ret=setsockopt(sfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));

    saddr.sin_family=AF_INET;
    saddr.sin_port=htons(SERVER_PORT);
    //saddr.sin_addr.s_addr=htonl(INADDR_ANY);
    inet_pton(AF_INET,"127.0.0.1",&saddr.sin_addr.s_addr);

    ret=bind(sfd,(struct sockaddr *)&saddr,sizeof(saddr));
    if(ret==-1){
	perror("bind fail\n");
	exit(-1);
    }
    ret=listen(sfd,128);
    if(ret==-1){
	perror("listen fail\n");
	exit(-1);
    }

    efd=epoll_create(OPEN_SIZE);
    if(efd==-1){
	perror("epoll_create errorn");
	exit(-1);
    }
    tep.events=EPOLLIN;
    tep.data.fd=sfd;
    ret=epoll_ctl(efd,EPOLL_CTL_ADD,sfd,&tep);
    if(ret==-1){
	perror("epoll_ctl sfd error\n");
	exit(-1);
    }

//    pthread_mutex_t lock_h;
    pool_init(10,20);
//    pthread_mutex_init(&lock_h, NULL);
    Total_msg *news=NULL;
    
    /*---------------------------------------------*/
    //数据库初始化
    /*
    conn = mysql_init(&mysq);
    if(connect == NULL){
	ret = mysql_errno(&mysq);
	printf("mysql_init err:%d\n",ret);
	return ret;
    }
*/
    //表格创建1次即可
//    create_all_table(mysql);//创建所有所有表格


    while(1){
//	pthread_mutex_lock(&lock_h);
	nread=epoll_wait(efd,ep,OPEN_SIZE,-1);
	if(nread==-1){
	    perror("epoll_wait error\n");
	    exit(-1);
	}
	for(i=0;i<nread;i++){
	    if(!(ep[i].events&EPOLLIN))
		continue;
	    if(ep[i].data.fd==sfd){
		c_len=sizeof(saddr);
		cfd=accept(sfd,(struct sockaddr *)&caddr,&c_len);
		setnonblocking(cfd);//设置为非阻塞
		printf("client ip[%s],port[%d]\n",
		    inet_ntop(AF_INET,&caddr.sin_addr.s_addr,ip_buf,INET_ADDRSTRLEN),
		    caddr.sin_port);
		if(cfd==-1){
		    perror("accept error\n");
		    exit(-1);
		}
		tep.data.fd=cfd;
		ret=epoll_ctl(efd,EPOLL_CTL_ADD,cfd,&tep);
		if(ret==-1){
		    perror("epoll_ctl add cfd error\n");
		    exit(-1);
		}
	    }
	    else if(ep[i].events & EPOLLIN){
		if((sockfd = ep[i].data.fd) < 0)
		    continue;
//		sockfd=ep[i].data.fd;
		ret=read(sockfd,line_buf,sizeof(line_buf));
//		ret=socket_read(sockfd,line_buf,sizeof(line_buf));//非阻塞，循环读
		if(ret==0){
		    int res=epoll_ctl(efd,EPOLL_CTL_DEL,sockfd,NULL);
		    if(res==-1){
			perror("epoll_ctl del cfd error\n");
			exit(-1);
		    }
		    close(sockfd);
		    printf("客户端断开\n");
		}
		else if(ret<0){
		    if(errno==EINTR)
			continue;
		    perror("read cfd error\n");
		    close(cfd);
		}
		else{
		    news=(Total_msg *)malloc(sizeof(Total_msg));
		    memset(news, 0, sizeof(Total_msg));
		    memcpy(news->buff, line_buf, sizeof(line_buf));
		    news->cfd=sockfd;

		    pool_add_task(client_task, news);
		    
		    tep.data.fd=sockfd;
		    tep.events=EPOLLIN|EPOLLET;//设置成边沿触发
		    epoll_ctl(efd,EPOLL_CTL_MOD,sockfd,&tep);
		}
	    }
	    else if(ep[i].events & EPOLLOUT){
		sockfd=ep[i].data.fd;
		write(sockfd, line_buf, sizeof(line_buf));
		tep.data.fd=sockfd;
		tep.events=EPOLLIN|EPOLLET;//设置成边沿触发
		epoll_ctl(efd,EPOLL_CTL_MOD,sockfd,&tep);
	    }
	}
//	pthread_mutex_unlock(&lock_h);
    }
    close(sfd);
    pool_destroy(pool);
    return 0;
}

#endif
#if 0
int main(){
    int a=1234;
    char buf[20]={};
    if(int_to_ch(a,buf,20))
	printf("int to ch ok : %s\n",buf);
    else
	printf("int to ch fail\n");
    printf("a=%d\n",a);

    char buf1[20]="4567a123";
    int b=0;
    if(ch_to_int(buf1,&b))
	printf("ch to int ok : %d\n",b);
    else
	printf("ch to int fail\n");

}
#endif
