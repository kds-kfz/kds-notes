#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<unistd.h>
#include<stdio.h>
#include<fcntl.h>
#include<stdlib.h>
#include<errno.h>

#include"function.h"
#include"font.h"
//function.c

//整型转数字字符
int int_to_ch(int m,char *dest,int size){
    memset(dest,0,size);
    char temp[10]="0123456789";
    int t=m;
    int i=0;

    for(;t!=0;i++)
	t/=10;
    i--;
    while(m){
	t=m%10;
	dest[i]=temp[t];
	m/=10;
	i--;
    }
    return 1;
}

//数字字符转整型
int ch_to_int(const char *src,int *m){
    *m=0;
    for(int i=0;src[i]!='\0';i++){
	if(src[i]>='0'&&src[i]<='9'){
	    *m*=10;
	    *m=(*m+src[i]-48);
	}
	else
	    return 0;
    }
    return 1;
}

//追加套接字，设置为非阻塞
void setnonblocking(int sock){
    int opts;
    opts=fcntl(sock,F_GETFL);
    if(opts<0){
	perror("fcntl(sock,GETFL)");
	exit(1);
    }
    opts = opts|O_NONBLOCK;
    if(fcntl(sock,F_SETFL,opts)<0){
	perror("fcntl(sock,SETFL,opts)");
	exit(1);
    }
}

//向套接字写入数据
int socket_write(int sockfd,char *buff,int len){
    int ret;
    int send_len = 0;
    while(1){
	ret = write(sockfd,buff+send_len,len-send_len);
	if(ret < 0){
	    if(errno == EINTR || errno == EAGAIN)
		continue;
	    else
		return -1;
	}
	send_len += ret;
	if(send_len == len)
	    break;
    }
    return send_len;
}

//读取套接字里数据
int socket_read(int sockfd,char *buff,int len){
    int ret;
    int recv_len=0;
    while(1){
	ret = read(sockfd,buff+recv_len,len-recv_len);
	if(ret < 0){
	    if(errno == EINTR || errno == EAGAIN)
		break;
	}
	recv_len += ret;
	if(recv_len == len)
	    break;
    }
    return recv_len;
}
void show_data(Custom *p){
    set_cursor(1,1);
    printf("%s,%s\n",p->account,p->number);
    printf("%s,%s\n",p->password,p->age);
    printf("%s,%s\n",p->telephone,p->sex);
    printf("%s,%s\n",p->address,p->status);
}

//界面显示
void show_menu(char *str){
    printf(cursor_right6"%s\n\n",str);
    printf(cursor_right2"1.注册\n\n");
    printf(cursor_right3"2.登录\n\n");
    printf(cursor_right4"3.退出\n\n");
    printf(cursor_right5"请输入您的选择:");
}

/*--------------------------------------------*/
//创建电影院链表
FILM *create_film_link(FILM *head,Film d1){//尾插入
    FILM *p=(FILM *)malloc(sizeof(FILM));   
    p->value=d1;
    p->next=NULL;
    if(head==NULL)
	return p;
    FILM *tail=head;
    while(tail->next!=NULL)//找最后一个数据地址
	tail=tail->next;
    tail->next=p;//数据地址链接
    return head;
}
void free_film_link(FILM *head){
    while(head){
	FILM *s=head;
	head=head->next;
	free(s);
	}
//    printf("free success\n");
}
void printf_film_link(FILM *head){
    FILM *s=head;
    printf(message_cursor5"编号\t\t影院名称\t数量\n");
    while(s){
	printf(message_cursor5"%d\t\t%s\t%d\n",
	s->value.film.cinema_number,
	s->value.film.cinema_name,
	s->value.film_num);
	s=s->next;
    }
}
int film_node_num(FILM *head){
    FILM *s=head;
    int i=0;
    while(s){i++;s=s->next;}
    return i;
}
int seek_film_num(FILM *head,int num){
    FILM *s=head;
    while(s){
	if(s->value.film.cinema_number==num)
	    return 1;
	s=s->next;
    }
    return 0;
}
/*--------------------------------------------*/
//创建电影链表
MOVIE *create_movie_link(MOVIE *head,Movie d1){//尾插入
    MOVIE *p=(MOVIE *)malloc(sizeof(MOVIE));   
    p->value=d1;
    p->next=NULL;
    if(head==NULL)
	return p;
    MOVIE *tail=head;
    while(tail->next!=NULL)//找最后一个数据地址
	tail=tail->next;
    tail->next=p;//数据地址链接
    return head;
}
void free_movie_link(MOVIE *head){
    while(head){
	MOVIE *s=head;
	head=head->next;
	free(s);
	}
//    printf("free success\n");
}
void printf_movie_link(MOVIE *head){
    MOVIE *s=head;
    printf(message_cursor4"编号  电影名字   院号  电影院名  播放时长   上映时间   价格  票数  场地\n");
    while(s){
	printf(message_cursor4"%d  %s  %d  %s  %s   %s   %d    %d    %d\n",
	    s->value.movie_number,s->value.movie_name,
	    s->value.c_number,s->value.c_name,
	    s->value.play_time,s->value.show_time,
	    s->value.movie_price,s->value.ticket_count,s->value.play_yard);
	s=s->next;
    }
}

int movie_node_num(MOVIE *head){
    MOVIE *s=head;
    int i=0;
    while(s){i++;s=s->next;}
    return i;
}

MOVIE *seek_movie_num(MOVIE *head,int num){
    MOVIE *s=head;
    while(s){
	if(s->value.movie_number==num)
	    return s;
	s=s->next;
    }
    return NULL;
}

int seek_seat_ok(char *s,int n){
    return s[n-1]=='0';
}
/*----------------------------------------------------*/
//创建用户链表
CUSTOM *create_custom_link(CUSTOM *head,Custom d1){//尾插入
    CUSTOM *p=(CUSTOM *)malloc(sizeof(CUSTOM));   
    p->value=d1;
    p->next=NULL;
    if(head==NULL)
	return p;
    CUSTOM *tail=head;
    while(tail->next!=NULL)//找最后一个数据地址
	tail=tail->next;
    tail->next=p;//数据地址链接
    return head;
}
void free_custom_link(CUSTOM *head){
    while(head){
	CUSTOM *s=head;
	head=head->next;
	free(s);
	}
//    printf("free success\n");
}
/*----------------------------------------------------*/



