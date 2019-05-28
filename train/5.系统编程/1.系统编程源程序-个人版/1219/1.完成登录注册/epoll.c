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
    /*----------------------*/
    Film f;
    Movie m;
    char film_seat[60];


    /*----------------------*/

    memset(&msghead,0,sizeof(Msg_head));
    memcpy(&msghead, p->buff, sizeof(msghead));
	
    int seek_num=atoi(msghead.result);
    int seek_yard=atoi(msghead.yard);

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
//    int c_len=sizeof(msghead)+sizeof(Custom);
    int c_len=sizeof(msghead);
    int_to_ch(c_len,msghead.len,sizeof(msghead.len));
    
    char read_table=check_person(mysql,custom);
    printf("read_table : %c\n",read_table);
    if(read_table=='0')
	strcpy(msghead.result,"#1");//表格没有该数据，允许注册
    else	
	strcpy(msghead.result,"#2");

//    strcpy(msghead.result,"#1");
    strcpy(msghead.msg_type,"#3");

    memcpy(send_buf, &msghead, sizeof(msghead));
//    memcpy(send_buf+sizeof(msghead), custom, sizeof(Custom));
    
    if(read_table!='1')//插入数据
	insert_person(mysql,custom);
    
    ret = write(p->cfd,send_buf, c_len);
    if(ret<0){
	printf("服务器回馈客户信息失败\n");
	sleep(2);
	}
    }
    if(!strcmp(msghead.msg_type,"#4")){
	memset(custom,0,sizeof(Custom));
	memcpy(custom, p->buff + sizeof(msghead), sizeof(Custom));
	memset(&msghead,0,sizeof(msghead));
//	int c_len=sizeof(msghead)+sizeof(Custom);
	int c_len=sizeof(msghead);
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
//	memcpy(send_buf+sizeof(msghead), custom, sizeof(Custom));

	ret = write(p->cfd,send_buf, c_len);
	if(ret<0){
	    printf("服务器回馈客户信息失败\n");
	    sleep(2);
	}
    }
    /*-----------------------------------------查找film表格*/
    else if(!strcmp(msghead.msg_type,"#5")){
	memset(&msghead,0,sizeof(msghead));
	int c_len=sizeof(msghead)+sizeof(f);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	
	//查询film表格
	char buf[512];
//	read_film_table(mysql,buf);

	Mysql_query(mysql,"set names utf8");
	Mysql_query(mysql,"select *from film");
	mysql->result=mysql_store_result(mysql->connect);
	mysql->col=mysql_num_fields(mysql->result);
	while(mysql->row=mysql_fetch_row(mysql->result)){
	    memset(&f,0,sizeof(f));
	    f.film.cinema_number=atoi(mysql->row[0]);
	    strcpy(f.film.cinema_name,mysql->row[1]);
	    f.film_num=atoi(mysql->row[2]);
	    printf("%d,%s,%d\n",f.film.cinema_number,f.film.cinema_name,f.film_num);
	    
	    f.film.cinema_number=htonl(f.film.cinema_number);
	    f.film_num=htonl(f.film_num);

	    strcpy(msghead.result,"*1");//数据有效
	    memcpy(send_buf, &msghead, sizeof(msghead));
	    memcpy(send_buf+sizeof(msghead), &f, sizeof(f));
	
	    ret = write(p->cfd,send_buf, c_len);
//	    sleep(1);
	    usleep(200);
	    if(ret<0){
		printf("服务器回馈客户信息失败\n");
		sleep(2);
	    }
	}
	//数据无效
	    sleep(1);
	memset(&msghead,0,sizeof(msghead));
	c_len=sizeof(msghead);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	
	strcpy(msghead.result,"*0");//读取成功
	memcpy(send_buf, &msghead, sizeof(msghead));
	ret = write(p->cfd,send_buf, c_len);
	if(ret<0){
	    printf("服务器回馈客户信息失败\n");
	    sleep(2);
	}
    }
    else if(!strcmp(msghead.msg_type,"#6")){//获取某电影院所有电影
	memset(&msghead,0,sizeof(msghead));
	int c_len=sizeof(msghead)+sizeof(m);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	
	//查询film表格
	char buf[1024];
	char seek_buf[128];
	sprintf(seek_buf,"select *from movie where c_number=%d",seek_num);
	
	Mysql_query(mysql,"set names utf8");
	Mysql_query(mysql,seek_buf);
	mysql->result=mysql_store_result(mysql->connect);
	mysql->col=mysql_num_fields(mysql->result);
	while(mysql->row=mysql_fetch_row(mysql->result)){
	    memset(&f,0,sizeof(f));

	    m.movie_number=atoi(mysql->row[0]);
	    strcpy(m.movie_name,mysql->row[1]);
	    m.c_number=atoi(mysql->row[2]);
	    strcpy(m.c_name,mysql->row[3]);

	    strcpy(m.play_time,mysql->row[4]);
	    strcpy(m.show_time,mysql->row[5]);
	    m.movie_price=atoi(mysql->row[6]);
	    m.ticket_count=atoi(mysql->row[7]);
	    m.play_yard=atoi(mysql->row[8]);
	    
	    strcpy(msghead.result,"*1");//数据有效

	    printf("%d,%s,%d,%s,%s,%s,%d,%d,%d\n",
		m.movie_number,m.movie_name,m.c_number,m.c_name,
		m.play_time,m.show_time,m.movie_price,m.ticket_count,m.play_yard);
	    
	    m.movie_number=htonl(m.movie_number);
	    m.c_number=htonl(m.c_number);
	    m.movie_price=htonl(m.movie_price);
	    m.ticket_count=htonl(m.ticket_count);
	    m.play_yard=htonl(m.play_yard);
	    
	    memcpy(send_buf, &msghead, sizeof(msghead));
	    memcpy(send_buf+sizeof(msghead), &m, sizeof(m));
	
	    ret = write(p->cfd,send_buf, c_len);
	    sleep(1);
//	    usleep(200);
	    if(ret<0){
		printf("服务器回馈客户信息失败\n");
		sleep(2);
	    }
	}
	//数据无效
//	sleep(1);
//	usleep(200);
	memset(&msghead,0,sizeof(msghead));
	c_len=sizeof(msghead);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	
	strcpy(msghead.result,"*0");//无效数据
	memcpy(send_buf, &msghead, sizeof(msghead));
	ret = write(p->cfd,send_buf, c_len);
	if(ret<0){
	    printf("服务器回馈客户信息失败\n");
	    sleep(2);
	}
    }
    else if(!strcmp(msghead.msg_type,"#7")){//获取符合条件的电影
	memset(&msghead,0,sizeof(msghead));
	int c_len=sizeof(msghead)+sizeof(m);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	
	//查询film表格
	char buf[1024];
	char seek_buf[128];
	sprintf(seek_buf,"select *from movie where m_number=%d",seek_num);
	
	Mysql_query(mysql,"set names utf8");
	Mysql_query(mysql,seek_buf);
	mysql->result=mysql_store_result(mysql->connect);
	mysql->col=mysql_num_fields(mysql->result);
	while(mysql->row=mysql_fetch_row(mysql->result)){
	    memset(&f,0,sizeof(f));

	    m.movie_number=atoi(mysql->row[0]);
	    strcpy(m.movie_name,mysql->row[1]);
	    m.c_number=atoi(mysql->row[2]);
	    strcpy(m.c_name,mysql->row[3]);

	    strcpy(m.play_time,mysql->row[4]);
	    strcpy(m.show_time,mysql->row[5]);
	    m.movie_price=atoi(mysql->row[6]);
	    m.ticket_count=atoi(mysql->row[7]);
	    m.play_yard=atoi(mysql->row[8]);
	    
	    strcpy(msghead.result,"*1");//数据有效

	    printf("%d,%s,%d,%s,%s,%s,%d,%d,%d\n",
		m.movie_number,m.movie_name,m.c_number,m.c_name,
		m.play_time,m.show_time,m.movie_price,m.ticket_count,m.play_yard);
	    
	    m.movie_number=htonl(m.movie_number);
	    m.c_number=htonl(m.c_number);
	    m.movie_price=htonl(m.movie_price);
	    m.ticket_count=htonl(m.ticket_count);
	    m.play_yard=htonl(m.play_yard);
	    
	    memcpy(send_buf, &msghead, sizeof(msghead));
	    memcpy(send_buf+sizeof(msghead), &m, sizeof(m));
	
	    ret = write(p->cfd,send_buf, c_len);
	    sleep(1);
//	    usleep(200);
	    if(ret<0){
		printf("服务器回馈客户信息失败\n");
		sleep(2);
	    }
	}
	//数据无效
//	sleep(1);
//	usleep(200);
	memset(&msghead,0,sizeof(msghead));
	c_len=sizeof(msghead);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	
	strcpy(msghead.result,"*0");//无效数据
	memcpy(send_buf, &msghead, sizeof(msghead));
	ret = write(p->cfd,send_buf, c_len);
	if(ret<0){
	    printf("服务器回馈客户信息失败\n");
	    sleep(2);
	}
    }
    else if(!strcmp(msghead.msg_type,"#8")){//获取符合条件的电影
	memset(&msghead,0,sizeof(msghead));
	int c_len=sizeof(msghead)+sizeof(film_seat);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	
	//查询film表格
	char buf[1024];
	char seek_buf[128];
	//影院编号+片场编号
	sprintf(seek_buf,"select *from filmseat where c_number=%d and c_yard=%d",seek_num,seek_yard);
	
	Mysql_query(mysql,"set names utf8");
	Mysql_query(mysql,seek_buf);
	mysql->result=mysql_store_result(mysql->connect);
	mysql->col=mysql_num_fields(mysql->result);

	mysql->row=mysql_fetch_row(mysql->result);
	strcpy(film_seat,mysql->row[2]);
	
	memcpy(send_buf, &msghead, sizeof(msghead));
	memcpy(send_buf+sizeof(msghead), film_seat, sizeof(film_seat));//发送座位字符串
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
    //表格创建1次即可
//    create_all_table(mysql);//创建所有所有表格
//    insert_all_data(mysql);
    
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
//		setnonblocking(cfd);//设置为非阻塞
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
