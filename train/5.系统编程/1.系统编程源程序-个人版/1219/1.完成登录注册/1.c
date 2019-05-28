#include<stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include <arpa/inet.h>
#include <pthread.h>

#include"function.h"
#include"font.h"

//client.c
#define CLIENT_PORT 8888
char qbuff[2048]={};
Msg_head qmsghead;
Custom qcustom;
Film qfilm;

int login_ok=0;
int cinema_ok=0;
int n=0;


int telephone_ok(char *s,int size){
    for(int i=0;i<size&&s[i]!='\0';i++)
	if(s[i]<'0'||s[i]>'9')
	    return 0;
    return 1;
}

void updata_table(char mode,Custom *p){
    set_point_ch(30,7,p->account);set_point_ch(55,7,p->number);
    set_point_ch(30,9,p->password);set_point_ch(55,9,p->age);
    set_point_ch(30,11,p->telephone);set_point_ch(55,11,p->sex);
    set_point_ch(30,13,p->address);set_point_ch(55,13,p->status);
//    set_point_ch(30,19,p->address);
}

//信息变更
void information_change(int x,int y,char choose,char arr[],int size,int *right){
    int flag=1;
    char buf[20]={};
    set_cursor(x,y);
    clear_cursor(size);//视觉清空原有数据
    set_cursor(x,y);
    if(choose=='8'){
	information_status(arr);
	set_cursor(x,y);
	clear_cursor(size);
	set_point_ch(x,y,arr);
	*right=1;
    }
    else if(choose=='5'){
	fixed_input(buf,size,&flag);
	strcpy(arr,buf);
	int count=0;
	for(;buf[count]!='\0';count++);//统计电话位数
	if(count==11&&telephone_ok(buf,size)){
	    strcpy(arr,buf);
	    *right=1;
	}
	else
	    *right=0;
    }
    else{
	fixed_input(buf,size,&flag);
	strcpy(arr,buf);
	*right=1;
    }
}

void information_change2(int x,int y,char arr[]){
    int flag;
    set_cursor(x,y);
    clear_cursor(12);//视觉清空原有数据
    set_cursor(x,y);
    fixed_input(arr,12,&flag);
}

void information_status(char arr[]){
    int flag;
    char buf[20]={};

    while(1){
    show_table(3,4);
    set_cursor(1,5);
    printf(cursor_right2"身份选择界面\n\n");
    printf(cursor_right3"1.客户\n\n");
    printf(cursor_right4"2.管理员\n\n");
    printf(cursor_right5"输入您的选择:");
    char choose=fixed_input1(buf,20,&flag);//自定义标准输入，返回首字母
    if(choose=='1'){
	strcpy(arr,"client");
	break;
    }
    else if(choose=='2'){
	strcpy(arr,"admin");
	break;
    }
    else
	continue;
    }
    /*
    if(!strcmp(arr,"admin")){
	//每个客户端连上服务器的同时，服务器向客户端发送数据，
	//表明当前数据库的信息状态
	//当注册是管理员时，判断数据库里是否已经有管理员
	//若没有，则可以注册，否则不能注册，
    }
    */
}

void show_table(int mode,int color){
    system("clear");//清屏

    draw_row_line(20,3,1,"+",color);
    draw_row_line(21,3,46,"-",color);
    draw_row_line(67,3,1,"+",color);
    
    draw_col_line(20,4,17,"|",color);
    
    if(mode==1)
	draw_col_line(43,7,9,"|",color);
    else if(mode==2)
	draw_col_line(43,7,9,"|",color);
	
    draw_col_line(67,4,17,"|",color);

    draw_row_line(20,21,1,"+",color);
    draw_row_line(21,21,46,"-",color);
    draw_row_line(67,21,1,"+",color);
}

//显示客户注册各个信息
void show_clause(char mode,int m){
    if(m==1)
	show_table(1,3);
    else if(m==2)
	show_table(2,3);
    set_point_ch(23,7,"1.帐号:");set_point_ch(48,7,"2.编号:");
    set_point_ch(23,9,"3.密码:");set_point_ch(48,9,"4.年龄:");
    set_point_ch(23,11,"5.电话:");set_point_ch(48,11,"6.性别:");
    set_point_ch(23,13,"7.住址:");set_point_ch(48,13,"8.职业:");
    if(mode=='1'){//快速注册模式
	set_point_ch(23,15,"9.确定");set_point_ch(48,15,"0.返回");
	set_point_ch(23,17,"选  择:");
	set_point_ch(23,19,"结  果:");
    }
    else if(mode=='2'){//详细注册模式
//	set_point_ch(23,15,"结  果:");

	set_point_ch(23,15,"9.确定");set_point_ch(48,15,"0.返回");
	set_point_ch(23,17,"选  择:");set_point_ch(48,17,"其它:继续");
	set_point_ch(23,19,"结  果:");
    }
}

//获取注册模式函数
char register_mode(){
    while(1){
    char buf[20]={};
    int flag;
//    interface(26,86,21);//画边框函数
    set_cursor(1,5);
    save_cursor();
    show_table(0,4);
    recover_cursor();
    printf(cursor_right4"客 户 注 册 模 式\n\n");
    printf(cursor_right3"1.快速注册\n\n");
    printf(cursor_right2"2.详细注册\n\n");
    printf(cursor_right5"输入您的选择:");
    char mode=fixed_input1(buf,20,&flag);//自定义标准输入，返回首字母
    if(mode=='1'||mode=='2')
	return mode;
    else
	continue;
    }
}

//客户注册函数：获取注册模式+开始注册
void client_register(Thread_t *p){
    char mode=register_mode();//获取注册模式

    start_register(mode,p);//开始注册
}

//打包注册数据并发送
int send_to_server(Custom *p,int mode,int *right,Thread_t *t){
    int fd=t->cfd;
    if(*right){
    Msg_head msghead;
    char buff[2048];
    int ret;
    
//    show_data(p);//测试
    
    int c_len=sizeof(Msg_head)+sizeof(Custom);
    int_to_ch(c_len,msghead.len,sizeof(msghead.len));
    strcpy(msghead.msg_type,"#3");
    memcpy(buff, &msghead, sizeof(msghead));
    memcpy(buff + sizeof(msghead), p, sizeof(Custom));
    ret = write(fd, buff, c_len);
    if(mode==1)
	set_cursor(31,19);
    else if(mode==2)
	set_cursor(31,19);
    if(ret<0){
	printf(red"client_register write to server fail\n");
	sleep(2);
	return 0;
    }
    else{
	//发送条件变量信号
//	pthread_mutex_lock(&(t->mutex));
	while(t->rec_ok)//等待服务器反馈
	    pthread_cond_wait(&(t->rec_cond),&(t->mutex));
	    if(!t->rec_ok){
		t->rec_ok=1;
	    if(!strcmp(qmsghead.result,"#1"))
		set_point_ch(31,19,"注册成功");
	    else if(!strcmp(qmsghead.result,"#2"))
		set_point_ch(31,19,"注册失败");
//	    pthread_mutex_unlock(&(t->mutex));
	    }
	    memset(qbuff, 0, 2048);
	    return 1;
	}
    }
    else{
	set_cursor(31,19);
	printf(red"输入有误,请检查\n");
	sleep(2);
	return 0;
    }
}


void client_login(Thread_t *t){
    int fd=t->cfd;
    while(1){
    show_table(3,6);//模式3,普通表格
//    set_cursor(1,5);
//    printf(cursor_right3"客户登录界面\n\n");
    set_point_ch(33,5,"客户登录界面");
    set_point_ch(33,7,"帐号:");
    set_point_ch(33,9,"密码:");
    set_point_ch(33,11,"结果:");
    set_point_ch(33,13,"提示:1.继续 2.返回");
    set_point_ch(33,15,"请输入您的选择:");

    Custom custom;
    Msg_head msghead;
    char buff[2048];
    int ret;
    int flag;
//    char account[20]={};
//    char key[15]={};
    char buf[20]={};

    memset(&custom, 0, sizeof(custom));
    set_cursor(38,7);
    fixed_input(custom.account,20,&flag);
    set_cursor(38,9);
    fixed_input(custom.password,15,&flag);

    int c_len=sizeof(msghead)+sizeof(custom);
    int_to_ch(c_len,msghead.len,10);
    strcpy(msghead.msg_type,"#4");
    memcpy(buff, &msghead, sizeof(msghead));
    memcpy(buff + sizeof(msghead), &custom, sizeof(custom));
    ret = write(fd, buff, c_len);
    if(ret<0)
	perror("client_login write to server fail\n");

//    pthread_mutex_lock(&(t->mutex));
    while(t->sen_ok)
	pthread_cond_wait(&(t->sen_cond),&(t->mutex));
	
    if(!t->sen_ok){
	t->sen_ok=1;
	if(!strcmp(qmsghead.result,"#1")){
	memset(qbuff, 0, 2048);
//	    set_point_ch(38,11,"登录成功");
	    login_ok=1;
	    return;
	}
	else if(!strcmp(qmsghead.result,"#2")){
	memset(qbuff, 0, 2048);
	    set_point_ch(38,11,"登录失败");
	    login_ok=0;
	}
//	pthread_mutex_unlock(&(t->mutex));	    
    }
    	
    set_cursor(48,15);
    while(1){
	char choose=fixed_input1(buf,20,&flag);//自定义标准输入，返回首字母
	if(choose=='1')
	    break;
	else if(choose=='2')
	    return;
	else{
	    set_cursor(48,15);
	    clear_cursor(strlen(buf));//视觉清空原有数据
	    fun_cursor_left(strlen(buf));//将光标移回来	
	    continue;
	    }
	}
    }
}

void *client_handler(void *arg){
    Thread_t *t=(Thread_t *)arg;
    
    int flag=1;
    char buf[20]={};
    while(1){
	pthread_mutex_lock(&(t->mutex));
//	interface(26,86,21);//画边框函数
	set_cursor(1,5);
	save_cursor();
	show_table(0,5);
	recover_cursor();
	show_menu("客户端界面");
	char choose=fixed_input1(buf,20,&flag);//自定义标准输入，返回首字母
	switch(choose){
	    case '1':client_register(t);break;	//注册
	    case '2':client_login(t);break;	//登录
	    case '3':exit(-1);
	    default :break;
	}
	if(login_ok)
	    show_cinema(t);
	pthread_mutex_unlock(&(t->mutex));
    }
}

void show_all_cinema(Thread_t *t){
    while(1){
    if(n==1){
    show_table(3,4);//模式3,普通表格
    set_point_ch(37,5,"~所有电影院简示~");
    set_point_ch(27,7,"影院编号");
    set_point_ch(41,7,"影院名称");
    set_point_ch(55,7,"电影数量");
    }
    
    set_point_num(29,9+n-1,qfilm.film.cinema_number);
    set_point_ch(41,9+n-1,qfilm.film.cinema_name);
    set_point_num(57,9+n-1,qfilm.film_num);printf("部");
    }
    /*
    printf(new_cursor3"%d\t%s\t%d",
	qfilm.film.cinema_number,qfilm.film.cinema_name,qfilm.film_num);
	*/
}

//购票界面
void show_cinema(Thread_t *t){
    int fd=t->cfd; 
    int ret;
    int flag;
    char buf[20]={};
    show_table(3,3);//模式3,普通表格
    set_point_ch(33,5,"欢迎进入购票界面");
    set_point_ch(33,7,"1.查看所有影院");
    set_point_ch(33,9,"2.返回");
    set_point_ch(33,11,"请输入选择:");
	
    while(1){	
    char choose=fixed_input1(buf,20,&flag);//自定义标准输入，返回首字母
    if(choose=='1'){
	Msg_head msghead;
	char buff[2048];
	int ret;
    
	int c_len=sizeof(Msg_head);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,"#5");//发送查询film标志
	memcpy(buff, &msghead, sizeof(msghead));
    
	ret = write(fd, buff, c_len);
	if(ret<0)
	    perror("show_all_cinema write to server fail\n");
    
	while(t->show_ok)
	    pthread_cond_wait(&(t->show_cond),&(t->mutex));//等待查询所有电影院
	
	if(!t->show_ok){
	    t->show_ok=1;
	    if(!strcmp(qmsghead.result,"#1")){
		cinema_ok=1;
		/*
		while(1){	
		    choose=fixed_input1(buf,20,&flag);
		    if(choose=='2')
			break;
		}
		*/
		
//		show_all_cinema(t);

	    }
	    else if(!strcmp(qmsghead.result,"#2")){
		cinema_ok=0;
	    }   
//	pthread_mutex_unlock(&(t->mutex));	    
	}
    }
    else if(choose=='2'){
	login_ok=0;
	return;
    }
    else{
	set_cursor(44,11);
	clear_cursor(strlen(buf));//视觉清空原有数据
	fun_cursor_left(strlen(buf));//将光标移回来	
	continue;
	}
    }

}

int main(){
    int cfd;
    int ret;
    struct sockaddr_in caddr;

    pthread_t pth;

    cfd=socket(AF_INET,SOCK_STREAM,0);
    if(cfd==-1){
	perror("client socket error\n");
	exit(-1);
    }
    caddr.sin_family=AF_INET;
    caddr.sin_port=htons(CLIENT_PORT);
    //caddr.sin_addr.s_addr=htonl(INADDR_ANY);
    inet_pton(AF_INET,"127.0.0.1",&caddr.sin_addr.s_addr);
    ret=connect(cfd,(struct sockaddr *)&caddr,sizeof(caddr));
    if(ret==-1){
	perror("connect error\n");
	exit(-1);
    }

    Thread_t *p=(Thread_t *)malloc(sizeof(Thread_t));
    p->rec_ok=1;
    p->sen_ok=1;
    p->show_ok=1;
    p->cfd=cfd;
    pthread_mutex_init(&(p->mutex),NULL);
    pthread_cond_init(&(p->rec_cond),NULL);
    pthread_cond_init(&(p->sen_cond),NULL);
    pthread_cond_init(&(p->show_cond),NULL);

    ret = pthread_create(&pth, NULL, client_handler, (void *)p);
    Msg_head msghead;
//    char result[10]={};
    Custom custom;

    if(ret!=0){
	printf("pthread_cteate error : %s\n",strerror(ret));
	exit(-1);
    }
    while(1){
//	memset(&qmsghead,0,sizeof(Msg_head));
	ret=read(cfd, qbuff, 2048);
//	printf("%s\n",qbuff);
	memcpy(&qmsghead,qbuff,sizeof(Msg_head));
	if(cinema_ok==0)
	    memcpy(&qcustom,qbuff+sizeof(Msg_head),sizeof(Custom));
	else if(cinema_ok==1){
	    n++;
	    memcpy(&qfilm,qbuff+sizeof(Msg_head),sizeof(qfilm));
	    qfilm.film.cinema_number=ntohl(qfilm.film.cinema_number);
	    qfilm.film_num=ntohl(qfilm.film_num);
	}
    printf("%d\t%s\t%d\n",
	qfilm.film.cinema_number,qfilm.film.cinema_name,qfilm.film_num);
	pthread_mutex_lock(&(p->mutex));
	if(ret<0){
	    perror("read fd error\n");
	    exit(-1);
	}
	else if(ret==0)
	    break;
	else{
	    if(p->rec_ok==1){
		pthread_cond_signal(&(p->rec_cond));
		p->rec_ok=0;
	    }
	    if(p->sen_ok==1){
		pthread_cond_signal(&(p->sen_cond));
		p->sen_ok=0;
	    }
	    if(p->show_ok==1){
		pthread_cond_signal(&(p->show_cond));
		p->show_ok=0;
	    }
	    pthread_mutex_unlock(&(p->mutex));
	    }
    }
    pthread_join(pth, NULL);
    return 0;
}
