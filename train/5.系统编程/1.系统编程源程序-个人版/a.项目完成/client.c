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

//读取服务器内容
//文件描述符+read_buf
//用于登录和注册发送数据
int read_server_data(int fd,Custom *p){
    int c_len;
    int ret;
    char read_buf[1024];
    char buf[512];
    Msg_head msghead;
    
    memset(read_buf,0,1024);
    ret=read(fd, read_buf, 1024);
    
    if(ret<0)
	return -1;
    else if(ret==0)
	return 0;
    else{//当读取成功，向服务器发送"#"
	memcpy(&msghead,read_buf,sizeof(Msg_head));//将读到的内容放到msghead
	if(!strcmp(msghead.msg_type,"#3")){//是注册
	    if(!strcmp(msghead.result,"#1")){//成功
		return 1;//返回成功
	    }
	    else if(!strcmp(msghead.result,"#2")){
		return 2;//返回失败
	    }
	}
	else if(!strcmp(msghead.msg_type,"#4")){//是登录
	    if(p)//如果不是空
		memcpy(p,read_buf+sizeof(Msg_head),sizeof(Custom));
	    if(!strcmp(msghead.result,"#1"))//成功
		return 1;//返回成功
	    else if(!strcmp(msghead.result,"#2"))
		return 2;//返回失败
	}
    }
}

int read_new_number(int fd){
    int ret;
    char read_buf[1024];
    
    Msg_head msghead;
    memset(read_buf,0,1024);
    ret=read(fd, read_buf, 1024);
    
    if(ret<0)
	return -1;
    else if(ret==0)
	return 0;
    else{
	memcpy(&msghead,read_buf,sizeof(Msg_head));
	if(!strcmp(msghead.msg_type,"#10"))//是新客户编号
	    msghead.new_number=ntohl(msghead.new_number);
	else if(!strcmp(msghead.msg_type,"#11"))//是新影院编号
	    msghead.new_number=ntohl(msghead.new_number);
	else if(!strcmp(msghead.msg_type,"#12"))//是新电影编号
	    msghead.new_number=ntohl(msghead.new_number);
    }
    return msghead.new_number;
}

//用于获取电影院信息
FILM *read_flim_data(int fd,FILM *head){
    char read_buf[2048];
    Msg_head msghead;
    Film d1;	    
    int ret;
    int i=0;
    int msg_len;
    int c_len=sizeof(d1);
    ret=read(fd, read_buf, 2048);//读所有
    if(ret<0){
	perror("read fd error\n");
	return NULL;
    }
    else if(ret==0)
	return NULL;
    else{
	memcpy(&msghead,read_buf,sizeof(Msg_head));//将读到的内容放到msghead
	ch_to_int(msghead.len,&msg_len);
	while(1){
	    if((sizeof(msghead)+c_len*i)>=msg_len)
		break;
	    memset(&d1,0,sizeof(Film));
//	    if(!strcmp(msghead.msg_type,"#5")){
		memcpy(&d1,read_buf+sizeof(Msg_head)+i*c_len,c_len);
		d1.film.cinema_number=ntohl(d1.film.cinema_number);
		d1.film_num=ntohl(d1.film_num);
		head=create_film_link(head,d1);
//	    }
	    i++;
	}
    }
    return head;
}

//用于获取电影电影院
MOVIE *read_movie_data(int fd,MOVIE *head){
    char read_buf[2048];
    Msg_head msghead;
    Movie d1;	    
    int ret;
    int i=0;
    int msg_len;
    int c_len=sizeof(d1);
    ret=read(fd, read_buf, 1024);
    if(ret<0){
	perror("read fd error\n");
	return NULL;
    }
    else if(ret==0)
	return NULL;
    else{
	memcpy(&msghead,read_buf,sizeof(Msg_head));//将读到的内容放到msghead
	ch_to_int(msghead.len,&msg_len);
	while(1){
	    if((sizeof(msghead)+c_len*i)>=msg_len)
		break;
	    memset(&d1,0,sizeof(Movie));
//	    if(!strcmp(msghead.msg_type,"#6")){
		memcpy(&d1,read_buf+sizeof(Msg_head)+i*c_len,c_len);
	    
		d1.movie_number=ntohl(d1.movie_number);
		d1.c_number=ntohl(d1.c_number);
		d1.movie_price=ntohl(d1.movie_price);
		d1.ticket_count=ntohl(d1.ticket_count);
		d1.play_yard=ntohl(d1.play_yard);

		head=create_movie_link(head,d1);
		i++;
//	    }
	}
    }
    return head;
}

//获取座位字符串
int read_movie_seat(int fd,char *s,int size){
    char read_buf[1024];
    int ret;
    Msg_head msghead;
    ret=read(fd, read_buf, 1024);
    if(ret<0){
	perror("read fd error\n");
	return -1;
    }
    else if(ret==0)
	return 0;
    else{
	memcpy(&msghead,read_buf,sizeof(Msg_head));//将读到的内容放到msghead
	if(!strcmp(msghead.msg_type,"#8"))
	    memcpy(s,read_buf+sizeof(Msg_head),size);
	else
	    return 0;
    }
    printf("%s\n",s);
    return 1;
}

int read_film_one(int fd,Film *f){
    char read_buf[515];
    Msg_head msghead;
    int ret;
    ret=read(fd, read_buf, 2048);//读所有
    if(ret<0){
	perror("read fd error\n");
	return -1;
    }
    else if(ret==0)
	return 0;
    else{
	memcpy(&msghead,read_buf,sizeof(Msg_head));//将读到的内容放到msghead
//	if(msghead.result,"#16"){
	    if(f){
		memcpy(f,read_buf+sizeof(Msg_head),sizeof(Film));
//		f->film.cinema_number=ntohl(f->film.cinema_number);
//		f->film_num=ntohl(f->film_num);
//		f->film.cinema_number=f->film.cinema_number;
//		f->film_num=f->film_num;
	    }
	    if(!strcmp(msghead.result,"#1")){//成功
		return 1;//返回成功
	    }
	    else if(!strcmp(msghead.result,"#2")){
		return 2;//返回失败
	    }
//	}
    }
}

int read_movie_one(int fd,Movie *m){
    char read_buf[515];
    Msg_head msghead;
    int ret;
    ret=read(fd, read_buf, 2048);//读所有
    if(ret<0){
	perror("read fd error\n");
	return -1;
    }
    else if(ret==0)
	return 0;
    else{
	memcpy(&msghead,read_buf,sizeof(Msg_head));//将读到的内容放到msghead
//	if(msghead.result,"#16"){
	    if(m){
		memcpy(m,read_buf+sizeof(Msg_head),sizeof(Movie));
//		f->film.cinema_number=ntohl(f->film.cinema_number);
//		f->film_num=ntohl(f->film_num);
	    }
	    if(!strcmp(msghead.result,"#1")){//成功
		return 1;//返回成功
	    }
	    else if(!strcmp(msghead.result,"#2")){
		return 2;//返回失败
	    }
//	}
    }
}

//向服务器发送内容
//文件描述符+包头+模式
int send_client_data(int fd,void *arg,char *type,int num,int yard){
    int c_len;
    char buf[1024]={};
    int ret;
    Msg_head msghead;
    memset(&msghead,0,sizeof(Msg_head));
    //注册：#3，登录：#4，查看电影院：#5，查看该电影院所有电影：#6
    if(!strcmp(type,"#3")){//注册
	c_len=sizeof(Msg_head)+sizeof(Custom);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
	memcpy(buf+sizeof(Msg_head),(Custom *)arg,sizeof(Custom));
    }
    else if(!strcmp(type,"#4")){//登录，只需客户端入的是：帐号和密码
	c_len=sizeof(Msg_head)+sizeof(Custom);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
	memcpy(buf+sizeof(Msg_head),(Custom *)arg,sizeof(Custom));
    }
    else if(!strcmp(type,"#5")){//查看电影院，只发命令
	c_len=sizeof(Msg_head);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
    }
    else if(!strcmp(type,"#6")){//查看该电影院所有电影，只发命令
	c_len=sizeof(Msg_head);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	int_to_ch(num,msghead.result,sizeof(msghead.result));//影院编号
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
    }
    else if(!strcmp(type,"#7")){//查看符合条件的电影电影，只发命令
	c_len=sizeof(Msg_head);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	int_to_ch(num,msghead.result,sizeof(msghead.result));//影院编号
	int_to_ch(yard,msghead.yard,sizeof(msghead.yard));//电影编号
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
    }
    else if(!strcmp(type,"#8")){//查看座位
	c_len=sizeof(Msg_head);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	int_to_ch(num,msghead.result,sizeof(msghead.result));//影院编号
	int_to_ch(yard,msghead.yard,sizeof(msghead.yard));   //片场编号
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
    }
    else if(!strcmp(type,"#9")){//发送购票信息
	c_len=sizeof(Msg_head)+sizeof(Ticket);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
	memcpy(buf+sizeof(Msg_head),(Ticket *)arg,sizeof(Ticket));
    }
    else if(!strcmp(type,"#10")){//获取客户新编号
	c_len=sizeof(Msg_head);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
    }
    else if(!strcmp(type,"#11")){//获取影院新编号
	c_len=sizeof(Msg_head);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
    }
    else if(!strcmp(type,"#12")){//获取电影新编号
	c_len=sizeof(Msg_head);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
    }
    else if(!strcmp(type,"#13")){//查看所有电影
	c_len=sizeof(Msg_head);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
    }
    else if(!strcmp(type,"#14")){//插入新电影院
	c_len=sizeof(Msg_head)+sizeof(Film);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
	memcpy(buf+sizeof(Msg_head),(Film *)arg,sizeof(Film));
    }
    else if(!strcmp(type,"#15")){//删除影院
	c_len=sizeof(Msg_head)+sizeof(Film);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
	memcpy(buf+sizeof(Msg_head),(Film *)arg,sizeof(Film));
    }
    else if(!strcmp(type,"#16")){//查询影院
	c_len=sizeof(Msg_head)+sizeof(Film);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
	memcpy(buf+sizeof(Msg_head),(Film *)arg,sizeof(Film));
    }
    else if(!strcmp(type,"#17")){//修改影院名字
	c_len=sizeof(Msg_head)+sizeof(Film);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
	memcpy(buf+sizeof(Msg_head),(Film *)arg,sizeof(Film));
    }
    else if(!strcmp(type,"#18")){//增加新电影到影院
	c_len=sizeof(Msg_head)+sizeof(Movie);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
	memcpy(buf+sizeof(Msg_head),(Movie *)arg,sizeof(Movie));
	memcpy(buf+sizeof(Msg_head)+sizeof(Movie),&num,4);
    }
    else if(!strcmp(type,"#19")){//增加新电影到影库
	c_len=sizeof(Msg_head)+sizeof(Movie);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
	memcpy(buf+sizeof(Msg_head),(Movie *)arg,sizeof(Movie));
    }
    else if(!strcmp(type,"#20")){//查询电影
	c_len=sizeof(Msg_head)+sizeof(Movie);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
	memcpy(buf+sizeof(Msg_head),(Movie *)arg,sizeof(Movie));
    }
    else if(!strcmp(type,"#21")){//插入已修改的电影
	c_len=sizeof(Msg_head)+sizeof(Movie);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
	memcpy(buf+sizeof(Msg_head),(Movie *)arg,sizeof(Movie));
    }
    else if(!strcmp(type,"#22")){//删除电影
	c_len=sizeof(Msg_head)+sizeof(Movie);
	int_to_ch(c_len,msghead.len,sizeof(msghead.len));
	strcpy(msghead.msg_type,type);
	memcpy(buf,&msghead,sizeof(Msg_head));
	memcpy(buf+sizeof(Msg_head),(Movie *)arg,sizeof(Movie));
    }

    ret=write(fd,buf,c_len);
    if(ret<0)
	return 0;//发送失败
    else
	return 1;//发送成功
}

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
//电影信息变更
void movie_information_change(int x,int y,char choose,Movie *m,int size,int *right){
    int flag=1;
    char buf[20]={};
    int num;
    set_cursor(x,y);
    clear_cursor(size);//视觉清空原有数据
    set_cursor(x,y);
    if(choose=='5'||choose=='8'){
	fixed_input(buf,size,&flag);
	int res=ch_to_int(buf,&num);
	if(res){
	    m->ticket_count=num;
	    *right=1;
	}
	else
	    *right=0;
    }
    else if(choose=='9'){
	fixed_input(buf,size,&flag);
	int res=ch_to_int(buf,&num);
	if(res){
	    if(num>=1&&num<=10){
		m->ticket_count=num;
		*right=1;
	    }
	    else
		*right=0;
	}
	else
	    *right=0;
    }
    else if(choose=='2'){
	fixed_input(m->movie_name,size,&flag);
	*right=1;
    }
    else if(choose=='3'){
	fixed_input(m->play_time,size,&flag);
	*right=1;
    }
    else if(choose=='3'){
	fixed_input(m->show_time,size,&flag);
	*right=1;
    }
    else if(choose=='4'){
	fixed_input(m->show_time,size,&flag);
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
	set_point_ch(23,15,"9.确定");set_point_ch(48,15,"0.返回 1/3/5/8要填");
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

void start_register(char mode,int fd){
    Custom custom;
    Custom *p=&custom;
    strcpy(p->wallet,"1000");
    char buf[20]={};//选择缓存区
    int flag;
    int res;
    int right;
    int new_number;
    //获取客户编号
    memset(&custom, 0, sizeof(custom));
    send_client_data(fd,NULL,"#10",0,0);
    new_number=read_new_number(fd);
    int_to_ch(new_number,p->number,sizeof(p->number));
    strcpy(p->wallet,"999");
//    if(new_number>100)
//	printf("有效编号%d\n",new_number);
    if(mode=='1'){
	while(1){
//	interface(26,86,21);//画边框函数
	show_clause(mode,1);//显示客户注册的条款
	set_cursor(1,5);
	printf(cursor_right2"客户快速注册界面\n");
	
	updata_table(mode,p);
//	show_data(p);
	
	set_cursor(30,17);
	save_cursor();//保存光标位置
	clear_cursor(sizeof(buf));//视觉清空原有数据
	fun_cursor_left(sizeof(buf));//将光标移回来
	char choose=fixed_input1(buf,20,&flag);
	switch(choose){
	    case '0':return;
	    case '1':information_change(30,7,choose,p->account,12,&right);break;//帐号*
	    case '2':
//		     information_change(55,7,choose,p->number,12,&right);
		     break;//编号
	    case '3':information_change(30,9,choose,p->password,12,&right);break;//密码
	    case '4':information_change(55,9,choose,p->age,12,&right);break;
	    case '5':information_change(30,11,choose,p->telephone,12,&right);break;
	    case '6':information_change(55,11,choose,p->sex,12,&right);break;
	    case '7':information_change(30,13,choose,p->address,12,&right);break;
	    case '8':information_change(55,13,choose,p->status,12,&right);break;//身份*
	    case '9':
		     if(!strcmp(p->account,"")||
			!strcmp(p->password,"")||
			!strcmp(p->status,"")){
			 set_cursor(31,19);
			 printf(red"信息不完整,注册失败\n");
			 sleep(2);
			 break;
		     }
		     if(right){
			send_client_data(fd,p,"#3",0,0);
			int res=read_server_data(fd,NULL);
			if(res==1)
			    set_point_ch(31,19,"注册成功");
			else if(res==2)
			    set_point_ch(31,19,"注册失败");
		     }
		     else{ 
			 set_cursor(31,19);
			 printf(red"输入有误,请检查\n");
			 sleep(2);
		     }
		    break;
	    default :
		    recover_cursor();//恢复光标位置
		    clear_cursor(sizeof(buf)-1);//视觉清空原有数据
		    break;
	    }
	}
    }
    else if(mode=='2'){
	while(1){
	right=1;
	show_clause(mode,2);//显示客户注册的条款
	set_point_ch(55,7,p->number);//显示编号
	set_cursor(1,5);
	printf(cursor_right3"客户详细注册界面");
	int num[8]={0};
	
	information_change(30,7,'1',p->account,12,&right);num[0]=right;
//	information_change(55,7,'2',p->number,12,&right);
	num[1]=right;
	information_change(30,9,'3',p->password,12,&right);num[2]=right;
	information_change(55,9,'4',p->age,12,&right);num[3]=right;
	information_change(30,11,'5',p->telephone,12,&right);num[4]=right;
	information_change(55,11,'6',p->sex,12,&right);num[5]=right;
	information_change(30,13,'7',p->address,12,&right);num[6]=right;
	information_change(55,13,'8',p->status,12,&right);num[7]=right;
	
	show_clause(mode,2);//显示客户注册的条款
	set_cursor(1,5);
	printf(cursor_right3"客户详细注册界面");
	while(1){
	updata_table(mode,p);
//	show_data(p);//测试
	set_cursor(30,17);
	clear_cursor(strlen(buf));//视觉清空原有数据
	fun_cursor_left(strlen(buf));//将光标移回来
	
	char choose=fixed_input1(buf,20,&flag);
	if(choose=='9'){
	    for(int i=0;i<8;i++)
		if(num[i]!=1){
		    right=num[i];
			break;
		}
	    if(right){
//		send_to_server(p,2,&right,t);
		send_client_data(fd,p,"#3",0,0);
		int res=read_server_data(fd,NULL);
		if(res==1)
		    set_point_ch(31,19,"注册成功");
		else if(res==2)
		    set_point_ch(31,19,"注册失败");
		continue;
	    }
	    else{
		set_cursor(31,19);
		printf(red"输入有误,请重新注册\n");
		sleep(2);
	    }
	    return;
	}
	else if(choose=='0')
	    return;
	else
	    break;
	    }
	}
    }
}

//客户注册函数：获取注册模式+开始注册
void client_register(int fd){
    char mode=register_mode();//获取注册模式

    start_register(mode,fd);//开始注册
}

void client_login(int fd){
    while(1){
    show_table(3,6);//模式3,普通表格
    set_point_ch(33,5,"客户登录界面");
    set_point_ch(33,7,"帐号:");
    set_point_ch(33,9,"密码:");
    set_point_ch(33,11,"结果:");
    set_point_ch(33,13,"提示:1.继续 2.返回");
    set_point_ch(33,15,"请输入您的选择:");

    Custom custom;
    Custom *p=&custom;
    Custom *p2=(Custom *)malloc(sizeof(Custom));
    Msg_head msghead;
    char buff[2048];
    int ret;
    int flag;
    char buf[20]={};

    memset(&custom, 0, sizeof(custom));
    set_cursor(38,7);
    fixed_input(custom.account,20,&flag);
    set_cursor(38,9);
    fixed_input(custom.password,15,&flag);

    send_client_data(fd,p,"#4",0,0);
    int res=read_server_data(fd,p2);//读回用户所有信息
    
    if(res==1){
//	set_point_ch(38,11,"登录成功");
	if(!strcmp(p2->status,"admin"))//判断登录者身份
	    show_admin(fd,p2);//进入管理界面
	else if(!strcmp(p2->status,"client"))
	    show_cinema(fd,p2);//进入购票界面
	return;
    }
    else if(res==2)
	set_point_ch(38,11,"登录失败");

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

int buy_ticket(int fd,MOVIE *m,Custom *p,int seat){
    //显示购买电影票信息
    //确认购买
    //发送命令，更新座位，票数量，电影票表格
    int flag;
    char buf[20];
    interface(26,86,23);
    set_point_ch(33,4,"您购买的电影票信息");
    printf("\n\n"message_cursor4"编号  电影名字   院号  电影院名  播放时长   上映时间   价格  座位  片场\n");
    printf(message_cursor4"%d  %s  %d  %s  %s   %s   %d    %d     %d\n",
	m->value.movie_number,m->value.movie_name,
	m->value.c_number,m->value.c_name,
	m->value.play_time,m->value.show_time,
	m->value.movie_price,seat,m->value.play_yard);

    /*--------------------------------------*/
    //显示客户信息
    int w;
    ch_to_int(p->wallet,&w);
    set_point_ch(33,9,"您的钱余额:");
    printf("%d 元",w);
    set_point_ch(33,10,"提示:0.退出 1.购买");
    set_point_ch(33,11,"购买情况:");
    set_point_ch(33,12,"请输入您的选择:");
    while(1){
    set_cursor(48,12);
    clear_cursor(strlen(buf));
    set_cursor(48,12);
    char choose=fixed_input1(buf,20,&flag);
    if(choose=='1'){
	int n=w;
	n-=m->value.movie_price;
	if(n>=0&&m->value.ticket_count>0){//判断有足够的钱和必须有电影票
	    w=n;
	    set_cursor(42,11);
	    clear_cursor(20);
	    set_point_ch(42,11,"购买成功");
	    set_cursor(44,9);
	    clear_cursor(strlen(p->wallet));
	    set_point_num(44,9,w);
	    int_to_ch(w,p->wallet,sizeof(p->wallet));
	    sleep(1);
	    //打包数据发送，更新：钱包，票价，座位，插入电影票表格
	    Ticket my_ticket;
	    Ticket *tic=&my_ticket;
	    struct tm *showTime;
	    time_t t1;
	    time(&t1);
	    TIME time;

	    showTime =localtime(&t1);
	    
	    int_to_ch(showTime->tm_year+1900,time.year,10);
	    int_to_ch(showTime->tm_mon+1,time.month,10);
	    int_to_ch(showTime->tm_mday,time.day,10);
	    int_to_ch(showTime->tm_hour,time.hour,10);
	    int_to_ch(showTime->tm_min,time.minute,10);
	    int_to_ch(showTime->tm_sec,time.second,10);
	    
	    my_ticket.number=htonl(atoi(p->number));
	    strcpy(my_ticket.name,p->account);
	    my_ticket.ticket.movie_number=htonl(m->value.movie_number);
	    strcpy(my_ticket.ticket.movie_name,m->value.movie_name);
	    my_ticket.ticket.c_number=htonl(m->value.c_number);
	    strcpy(my_ticket.ticket.c_name,m->value.c_name);
	    strcpy(my_ticket.ticket.play_time,m->value.play_time);
	    strcpy(my_ticket.ticket.show_time,m->value.show_time);
	    my_ticket.ticket.movie_price=htonl(m->value.movie_price);
	    my_ticket.ticket.play_yard=htonl(m->value.play_yard);
	    my_ticket.t_seat=htonl(seat);

	    sprintf(my_ticket.d_date,"%s-%s-%s",
		    time.year,time.month,time.day);
	    sprintf(my_ticket.t_time,"%s:%s:%s",
		    time.hour,time.minute,time.second);

//	    printf("%s,%s\n",my_ticket.d_date,my_ticket.t_time);
	    
	    send_client_data(fd,tic,"#9",0,0);//发送命令
	    //接收服务器操作结果
	    if(read_server_data(fd,NULL))
		printf("数据保存成功");
	    else{
		printf("数据保存失败");
		return -1;
	    }
	    sleep(5);
	    return 1;
	}
	else{
	    set_cursor(42,11);
	    clear_cursor(20);
	    set_point_ch(42,11,"购买失败，余额不足");
	}
    }
    else if(choose=='0')
	return 0;
    }
}

void show_movie_seat(int fd,char *s,MOVIE *t,Custom *p){
    while(1){
//	show_table(3,3);//模式3,普通表格
	int flag;
	char buf[20];
	interface(26,86,23);
	set_point_ch(25,5,"影院名称");
	set_point_ch(43,5,"电影名称");
	set_point_ch(61,5,"电影片场");
	set_point_ch(25,6,t->value.c_name);
	set_point_ch(43,6,t->value.movie_name);
	set_point_num(65,6,t->value.play_yard);
	
	printf("\n"message_cursor2);
	for(int i=1;i<=50;i++){
	    if(i<10)
		printf("[0%d][%c]   ",i,s[i-1]);
	    else
		printf("[%d][%c]   ",i,s[i-1]);
	    if(i%5==0)
		printf("\n"message_cursor2);
	}
	int n;
	set_point_ch(43,18,"提示:0.退出");
	set_point_ch(43,19,"请输入座位:");
	    
//	printf("%s,%s\n",p->account,p->password);
//	printf("%d,%s\n",t->value.movie_number,t->value.movie_name);

	char choose=fixed_input1(buf,20,&flag);
	if(choose=='0')
	    return;
	else{
	    ch_to_int(buf,&n);
	    //查找看该作为是否有人
	    if(seek_seat_ok(s,n)){
		//显示电影票信息+购票界面
		if(buy_ticket(fd,t,p,n)==1)//购买成功，退出该函数
		    return;
	    }
	}
    }		
}

void show_all_movie(int fd,MOVIE *head,Custom *p){
    int flag;
    char buf[20];
    int n;
    char seat_buf[51];
    while(1){
    interface(26,86,23);
    set_point_ch(33,5,"所有电影院信息");
    set_cursor(1,7);
    printf_movie_link(head);

    int i=movie_node_num(head);
    /*
    printf("\n\n");
    printf(cursor_right3"提示:0.退出\n");
    printf(cursor_right4"请输入电影编号:");
    save_cursor();
    */
    set_point_ch(33,10+i,"提示:0.退出");
    set_point_ch(33,11+i,"请输入电影编号:");
//    set_cursor(48,11+i);
	char choose=fixed_input1(buf,20,&flag);
	if(choose=='0')
	    return;
	else{
	    ch_to_int(buf,&n);
	    MOVIE *r=seek_movie_num(head,n);
	    if(r){
		//如果有该电影编号,则显示该电影的片场和座位
		send_client_data(fd,NULL,"#8",r->value.c_number,r->value.play_yard);
		//进入显示购票界面
		read_movie_seat(fd,seat_buf,sizeof(seat_buf));
		show_movie_seat(fd,seat_buf,r,p);
	    }
	}	
    }
}

void show_all_cinema(int fd,FILM *head,Custom *p){ 
    while(1){
	show_table(3,4);//模式3,普通表格
	set_point_ch(37,5,"~所有电影院简示~");
	set_cursor(1,7);
	/*
	set_point_ch(27,7,"影院编号");
	set_point_ch(41,7,"影院名称");
	set_point_ch(55,7,"电影数量");
	*/
	printf_film_link(head);
	int i=film_node_num(head);
	int n;
	MOVIE *head2=NULL;
	char buf[20];
	int flag;

	set_point_ch(35,10+i,"提示:0.退出");
	set_point_ch(35,11+i,"请选择影院编号:");
//	    set_cursor(50,11+i);
	    char choose=fixed_input1(buf,20,&flag);
	    if(choose=='0')
		return;
	    else{
		ch_to_int(buf,&n);
		if(seek_film_num(head,n)){
		    free_movie_link(head2);
		    head2=NULL;
		    send_client_data(fd,NULL,"#6",n,0);
		    head2=read_movie_data(fd,head2);
		    //显示所有电影
		    show_all_movie(fd,head2,p);
		}
		else{
		    set_cursor(50,11+i);
		    clear_cursor(strlen(buf));//视觉清空原有数据
		    fun_cursor_left(strlen(buf));//将光标移回来	
		    continue;
		}
	    }
	}

}
//添加影院
void add_cinema(int fd){
    int flag;
    char buf[20];
    Film f;
    Film *f2=&f;
    
	send_client_data(fd,NULL,"#11",0,0);//获取影院唯一编号
	int new_number=read_new_number(fd);
	f.film.cinema_number=htonl(new_number);
	f.film_num=htonl(0);

    while(1){

	show_table(3,3);//模式3,普通表格
	set_point_ch(33,5,"添加电影院界面");
	set_point_ch(33,7,"影院编号:");printf("%d",new_number);
	set_point_ch(33,9,"影院名称:");
	
	set_point_ch(33,11,"操作结果:");
	set_point_ch(33,13,"提示:0.返回 1.确定 2.修改");
	set_point_ch(33,15,"请输入选择:");
	set_cursor(42,9);
	fixed_input(f.film.cinema_name,20,&flag);
	while(1){
	set_cursor(44,15);
	clear_cursor(strlen(buf));
	set_cursor(44,15);
	char choose=fixed_input1(buf,20,&flag);
	if(choose=='0')
	    return;
	else if(choose=='1'){//发送数据,插入新电影院
	    send_client_data(fd,f2,"#14",0,0);   
	    int res=read_film_one(fd,NULL);
	    if(res==1){
		set_point_ch(42,11,"新影院插入成功");
//		return;
	    }
	    else if(res==2){
		set_point_ch(42,11,"出现同名,插入失败");
	    }
	}
	else if(choose=='2')
	    break;
	}
    }
}
//删除电影院
void delete_cinema(int fd){
    Film f;
    Film *f2=&f;
    int flag;
    char buf[20];
    char num[15];
    while(1){

	show_table(3,3);//模式3,普通表格
	set_point_ch(33,5,"删除电影院界面");
	set_point_ch(33,7,"影院编号:");
	
	set_point_ch(33,9,"操作结果:");
	set_point_ch(33,11,"提示:0.返回 1.确定 2.修改");
	set_point_ch(33,13,"请输入选择:");
	set_cursor(42,7);
	fixed_input(num,15,&flag);
	int ret=ch_to_int(num,&(f2->film.cinema_number));
	while(1){
	set_cursor(44,13);
	clear_cursor(strlen(buf));
	set_cursor(44,13);
	char choose=fixed_input1(buf,20,&flag);
	if(choose=='0')
	    return;
	else if(choose=='1'){//发送数据,删除影院
	    if(ret){//数据有效
	    send_client_data(fd,f2,"#15",0,0);   
	    int res=read_film_one(fd,NULL);
	    if(res==1){
		set_point_ch(42,9,"删除影院成功");
//		return;
	    }
	    else if(res==2){
		set_point_ch(42,9,"没有该影院");
	    }
	    }
	    else{
		set_point_ch(42,9,"输入数据无效");
	    }
	}
	else if(choose=='2')
	    break;
	}
    }
}
//修改电影院
void change_cinema(int fd){
    Film f;
    Film *f2=&f;
    int flag;
    char buf[20];
    char num[15];
    while(1){

	show_table(3,3);//模式3,普通表格
	set_point_ch(33,5,"修改电影院界面");
	set_point_ch(33,7,"影院编号:");
	
	set_point_ch(33,9,"操作结果:");
	set_point_ch(33,11,"提示:0.返回 1.确定 2.修改");
	set_point_ch(33,13,"请输入选择:");
	set_cursor(42,7);
	fixed_input(num,15,&flag);
	int ret=ch_to_int(num,&(f2->film.cinema_number));
	while(1){
	set_cursor(44,13);
	clear_cursor(strlen(buf));
	set_cursor(44,13);
	char choose=fixed_input1(buf,20,&flag);
	if(choose=='0')
	    return;
	else if(choose=='1'){//发送数据,删除影院
	    if(ret){//数据有效
	    send_client_data(fd,f2,"#16",0,0);   
	    int res=read_film_one(fd,NULL);//重写读取影院函数
	    if(res==1){
		set_point_ch(42,9,"该影院存在");
		//修改影院名字函数
		change_cinema_name(fd,f2);
//		return;
	    }
	    else if(res==2){
		set_point_ch(42,9,"没有该影院");
	    }
	    }
	    else{
		set_point_ch(42,9,"输入数据无效");
	    }
	}
	else if(choose=='2')
	    break;
	}
    }
}
//
void change_cinema_name(int fd,Film *f){
    int flag;
    char buf[20];
    while(1){

	show_table(3,3);//模式3,普通表格
	set_point_ch(33,5,"修改电影院界面");
	set_point_ch(33,7,"影院新名字:");
	
	set_point_ch(33,9,"操作结果:");
	set_point_ch(33,11,"提示:0.返回 1.确定 2.修改");
	set_point_ch(33,13,"请输入选择:");
	set_cursor(44,7);
	fixed_input(f->film.cinema_name,20,&flag);
	while(1){
	set_cursor(44,13);
	clear_cursor(strlen(buf));
	set_cursor(44,13);
	char choose=fixed_input1(buf,20,&flag);
	if(choose=='0')
	    return;
	else if(choose=='1'){//发送数据,删除影院
	    send_client_data(fd,f,"#17",0,0);   
	    int res=read_film_one(fd,NULL);//重写读取影院函数 ok
	    if(res==1){
		set_point_ch(42,9,"修改成功");
//		return;
	    }
	    else if(res==2){
		set_point_ch(42,9,"修改失败");
	    }
	}
	else if(choose=='2')
	    break;
	}
    }
    
}
//编辑电影院界面
void edit_cinema(int fd){
    while(1){
    int flag;
    char buf[20];
    show_table(3,7);//模式3,普通表格
    set_point_ch(33,5,"管理电影院界面");
    set_point_ch(33,7,"1.添加影院");
    set_point_ch(33,9,"2.修改影院");
    set_point_ch(33,11,"3.删除影院");
    set_point_ch(33,13,"4.返回");
    set_point_ch(33,15,"请输入选择:");
    char choose=fixed_input1(buf,20,&flag);
    switch(choose){
	case '1':add_cinema(fd);break;
	case '2':change_cinema(fd);break;
	case '3':delete_cinema(fd);break;
	case '4':return;
	default:
		set_cursor(44,13);
		clear_cursor(strlen(buf));//视觉清空原有数据
//		fun_cursor_left(strlen(buf));//将光标移回来	
		break;
	}
    }
}
//显示电影各项条款
void show_movie_clause(){
    show_table(3,4);
    set_point_ch(39,5,"电影信息");
    set_point_ch(22,7,"1.电影编号:");set_point_ch(46,7,"6.影院编号:");
    set_point_ch(22,9,"2.电影名字:");set_point_ch(46,9,"7.影院名字:");
    set_point_ch(22,11,"3.播放时长:");set_point_ch(46,11,"8.电影票价:");
    set_point_ch(22,13,"4.上映时间:");set_point_ch(46,13,"9.播放场地:");
    set_point_ch(46,15,"5.电影票数:");set_point_ch(42,20,"0.返回 q.确定 p.重输入");
}
void add_movie1(int fd,Film *f){
    Movie m;
    Movie *m2=&m;
    char num[15];
    int flag;
    char buf[20];
    
    m.c_number=htonl(f->film.cinema_number);	    //影院编号
    strcpy(m.c_name,f->film.cinema_name);   //影院名称
    //获取电影编号
    send_client_data(fd,NULL,"#12",0,0);
    int new_number=read_new_number(fd);
    m.movie_number=htonl(new_number);
    while(1){
	show_movie_clause();
	set_point_ch(22,15,"*.操作结果:");
	set_point_ch(22,17,"*.请输入选择:");

	set_point_num(33,7,new_number);set_point_num(57,7,f->film.cinema_number);
	set_point_ch(57,9,m2->c_name);

	set_cursor(33,9);
	fixed_input(m2->movie_name,20,&flag);
	set_cursor(33,11);
	fixed_input(m2->play_time,15,&flag);
	set_cursor(33,13);
	fixed_input(m2->show_time,15,&flag);
	
	set_cursor(57,11);
	fixed_input(num,15,&flag);
	int ret2=ch_to_int(num,&(m2->movie_price));
	m2->movie_price=htonl(m2->movie_price);
	
	set_cursor(57,13);
	fixed_input(num,15,&flag);
	int ret3=ch_to_int(num,&(m2->play_yard));
	m2->play_yard=htonl(m2->play_yard);
	
	set_cursor(57,15);
	fixed_input(num,15,&flag);
	int ret1=ch_to_int(num,&(m2->ticket_count));
	m2->ticket_count=htonl(m2->ticket_count);

	while(1){
	    set_cursor(33,17);
	    clear_cursor(strlen(buf));//视觉清空原有数据
	    set_cursor(33,17);
	    char choose=fixed_input1(buf,20,&flag);
	    if(choose=='0')
		return;
	    else if(choose=='q'){
		if(ret1==1&&ret2==1&&ret3>=1&&ret3<=10)//输入数字有效
		    //发送添加电影命令
		    send_client_data(fd,m2,"#18",f->film_num,0);
		    int res=read_server_data(fd,NULL);//获取添加电影结果
		    if(res==1){
			set_point_ch(33,15,"电影添加成功");
		    }
		    else if(res==2){
			set_point_ch(33,15,"电影添加失败");
		    }
	    }
	    else if(choose=='p'){
		break;
	    }
	    else{
		set_cursor(33,17);
		clear_cursor(strlen(buf));//视觉清空原有数据
//		fun_cursor_left(strlen(buf));//将光标移回来	
	    }
	}
    }
}
void add_movie_mode1(int fd){
    int flag;
    char buf[20];
    char num[15];
    Film f;
    Film *f2=(Film *)malloc(sizeof(Film));
    while(1){
	show_table(3,4);//模式3,普通表格
	set_point_ch(33,5,"影院编号:");
	set_point_ch(33,7,"操作结果:");
	set_point_ch(33,9,"提示:0.返回 1.确定 2.重输");
	set_point_ch(33,11,"请输入选择:");
	set_cursor(42,5);
	
	fixed_input(num,15,&flag);
	int ret=ch_to_int(num,&(f2->film.cinema_number));
	while(1){
	set_cursor(44,11);
	clear_cursor(strlen(buf));
	set_cursor(44,11);
	char choose=fixed_input1(buf,20,&flag);
	if(choose=='0')
	    return;
	else if(choose=='1'){//发送数据,删除影院
	    if(ret){//数据有效
	    send_client_data(fd,f2,"#16",0,0);   
	    int res=read_film_one(fd,f2);//重写读取影院函数
	    if(res==1){
//		set_point_ch(42,7,"该影院存在");
		//添加电影函数
		add_movie1(fd,f2);
		return;
	    }
	    else if(res==2){
		set_point_ch(42,7,"没有该影院");
		
		}
	    }
	    else{
		set_point_ch(42,7,"输入数据无效");
	    }
	}
	else if(choose=='2')
	    break;
	}
    }
}
void add_movie_mode2(int fd){
    Movie m;
    Movie *m2=&m;
    char num[15];
    int flag;
    char buf[20];
    
    //获取电影编号
    send_client_data(fd,NULL,"#12",0,0);
    int new_number=read_new_number(fd);
    m.movie_number=htonl(new_number);
    while(1){
	show_movie_clause();
	set_point_ch(24,15,"操作结果:");
	set_point_ch(24,17,"请输入选择:");

	set_point_num(33,7,new_number);

	set_cursor(33,9);
	fixed_input(m2->movie_name,20,&flag);
	set_cursor(33,11);
	fixed_input(m2->play_time,15,&flag);
	set_cursor(33,13);
	fixed_input(m2->show_time,15,&flag);
	
	set_cursor(59,11);
	fixed_input(num,15,&flag);
	int ret2=ch_to_int(num,&(m2->movie_price));
	m2->movie_price=htonl(m2->movie_price);
	
	set_cursor(59,13);
	fixed_input(num,15,&flag);
	int ret3=ch_to_int(num,&(m2->play_yard));
	m2->play_yard=htonl(m2->play_yard);
	
	set_cursor(59,15);
	fixed_input(num,15,&flag);
	int ret1=ch_to_int(num,&(m2->ticket_count));
	m2->ticket_count=htonl(m2->ticket_count);

	while(1){
	    set_cursor(35,17);
	    clear_cursor(strlen(buf));//视觉清空原有数据
	    set_cursor(35,17);
	    char choose=fixed_input1(buf,20,&flag);
	    if(choose=='0')
		return;
	    else if(choose=='1'){
		if(ret1==1&&ret2==1&&ret3>=1&&ret3<=10)//输入数字有效
		    //发送添加电影命令
		    send_client_data(fd,m2,"#19",0,0);
		    int res=read_movie_one(fd,NULL);//获取添加电影结果
		    if(res==1){
			set_point_ch(33,15,"电影添加成功");
		    }
		    else if(res==2){
			set_point_ch(33,15,"电影添加失败");
		    }
	    }
	    else if(choose=='2'){
		break;
	    }
	    else{
		set_cursor(35,17);
		clear_cursor(strlen(buf));//视觉清空原有数据
//		fun_cursor_left(strlen(buf));//将光标移回来	
	    }
	}
    }

}
//添加新电影
void add_movie(int fd){
    while(1){	
	int flag;
	char buf[20];
	show_table(3,5);//模式3,普通表格
	set_point_ch(33,5,"选择添加模式");
	set_point_ch(33,7,"1.添加到某影院中");
	set_point_ch(33,9,"2.添加到影片库中");
	set_point_ch(33,11,"3.返回");
	set_point_ch(33,13,"请输入选择");
	char choose=fixed_input1(buf,20,&flag);
	switch(choose){
	    case '1':add_movie_mode1(fd);break;
	    case '2':add_movie_mode2(fd);break;
	    case '3':return;
	    default:
		set_cursor(44,13);
		clear_cursor(strlen(buf));//视觉清空原有数据
//		fun_cursor_left(strlen(buf));//将光标移回来	
		break;
	}
    }
}
//修改电影信息
void change_movie_new(int fd,Movie *m){
    while(1){
    int flag;
    int flag2=1;
    int right=1;
    char buf[20];
    show_movie_clause();
    set_point_num(33,7,m->movie_number);set_point_num(57,7,m->c_number);
    set_point_ch(33,9,m->movie_name);set_point_ch(57,9,m->c_name);
    set_point_ch(33,11,m->play_time);set_point_num(57,11,m->movie_price);
    set_point_ch(33,13,m->show_time);set_point_num(57,13,m->play_yard);
    set_point_num(57,15,m->ticket_count);

    int res;
    set_point_ch(22,15,"*.操作结果:");
    set_point_ch(22,17,"*.请输入选择:");
    while(flag2){
	set_cursor(35,17);
	clear_cursor(strlen(buf));
	set_cursor(35,17);
	char choose=fixed_input1(buf,20,&flag);
	switch(choose){
	    case '0':return;
	    case 'q':
		     send_client_data(fd,m,"#21",0,0);
		     res=read_movie_one(fd,NULL);//获取插入电影结果
		     if(res==1){
			 set_point_ch(33,15,"插入成功");
			 break;
		     }
		     else if(res==2){
			 set_point_ch(33,15,"插入失败");
		     break;
		     }
	    case 'p':flag2=0;break;
	    case '2':movie_information_change(33,9,choose,m,10,&right);break;
	    case '3':movie_information_change(33,11,choose,m,10,&right);break;
	    case '4':movie_information_change(33,13,choose,m,10,&right);break;
	    case '5':movie_information_change(57,15,choose,m,10,&right);break;
	    case '8':movie_information_change(57,11,choose,m,10,&right);break;
	    case '9':movie_information_change(57,13,choose,m,10,&right);break;
	    default:break;
	    }
	}
    }
}

//修改电影
void change_movie(int fd){
    int flag;
    char buf[20];
    char num[15];
    Movie m;
    Movie *m2=(Movie *)malloc(sizeof(Movie));
    while(1){
	show_table(3,4);//模式3,普通表格
	set_point_ch(33,5,"电影编号:");
	set_point_ch(33,7,"操作结果:");
	set_point_ch(33,9,"提示:0.返回 1.确定 2.重输");
	set_point_ch(33,11,"请输入选择:");
	set_cursor(42,5);
	
	fixed_input(num,15,&flag);
	int ret=ch_to_int(num,&(m2->movie_number));
	while(1){
	set_cursor(44,11);
	clear_cursor(strlen(buf));
	set_cursor(44,11);
	char choose=fixed_input1(buf,20,&flag);
	if(choose=='0')
	    return;
	else if(choose=='1'){//发送数据,删除影院
	    if(ret){//数据有效
	    send_client_data(fd,m2,"#20",0,0);   
	    int res=read_movie_one(fd,m2);//判断是否有该电影
	    printf("---------%s\n",m2->movie_name);
	    sleep(2);
	    if(res==1){
//		set_point_ch(42,7,"该电影存在");
		//添加电影函数
		change_movie_new(fd,m2);
		return;
	    }
	    else if(res==2){
		set_point_ch(42,7,"没有该电影");
		
		}
	    }
	    else{
		set_point_ch(42,7,"输入数据无效");
	    }
	}
	else if(choose=='2')
	    break;
	}
    }
}
//删除电影
void delete_movie(int fd){
    Movie m;
    Movie *m2=&m;
    int flag;
    char buf[20];
    char num[15];
    while(1){

	show_table(3,3);//模式3,普通表格
	set_point_ch(39,5,"删除电影界面");
	set_point_ch(33,7,"电影编号:");
	
	set_point_ch(33,9,"操作结果:");
	set_point_ch(33,11,"提示:0.返回 1.确定 2.修改");
	set_point_ch(33,13,"请输入选择:");
	set_cursor(42,7);
	fixed_input(num,15,&flag);
	int ret=ch_to_int(num,&(m2->movie_number));
	while(1){
	set_cursor(44,13);
	clear_cursor(strlen(buf));
	set_cursor(44,13);
	char choose=fixed_input1(buf,20,&flag);
	if(choose=='0')
	    return;
	else if(choose=='1'){//发送数据,删除电影
	    if(ret){//数据有效
	    send_client_data(fd,m2,"#20",0,0);   //先查询
	    int res=read_movie_one(fd,m2);
	    if(res==1){
		send_client_data(fd,m2,"#22",0,0);//删除电影
		res=read_movie_one(fd,NULL);
		if(res==1)
		    set_point_ch(42,9,"删除电影成功");
		else if(res==2)
		    set_point_ch(42,9,"删除电影失败");
//		return;
	    }
	    else if(res==2){
		set_point_ch(42,9,"没有该电影");
	    }
	    }
	    else{
		set_point_ch(42,9,"输入数据无效");
	    }
	}
	else if(choose=='2')
	    break;
	}
    }
}
//编辑电影界面
void edit_movie(int fd){
    while(1){
	int flag;
	char buf[20];
	show_table(3,7);//模式3,普通表格
	set_point_ch(33,5,"管理电影界面");
	set_point_ch(33,7,"1.添加电影");
	set_point_ch(33,9,"2.修改电影");
	set_point_ch(33,11,"3.删除电影");
	set_point_ch(33,13,"4.返回");
	set_point_ch(33,15,"请输入选择:");
	char choose=fixed_input1(buf,20,&flag);
	switch(choose){
	    case '1':add_movie(fd);break;
	    case '2':change_movie(fd);break;
	    case '3':delete_movie(fd);break;
	    case '4':return;
	    default:
		set_cursor(44,15);
		clear_cursor(strlen(buf));//视觉清空原有数据
//		fun_cursor_left(strlen(buf));//将光标移回来	
		break;
	}
    }
}

//管理员界面
void show_admin(int fd,Custom *p){
    int flag;
    char buf[20]={};
    FILM *head=NULL;
    MOVIE *head2=NULL;
	while(1){
	show_table(3,7);//模式3,普通表格
	set_point_ch(33,5,"管理员界面");
	set_point_ch(33,7,"1.查看所有影院");
	set_point_ch(33,9,"2.查看所有电影");
	set_point_ch(33,11,"3.查看个人信息");
	set_point_ch(33,13,"4.编辑电影院");
	set_point_ch(33,15,"5.编辑电影");
	set_point_ch(33,17,"6.退出");

	set_point_ch(33,19,"请输入选择:");
	char choose=fixed_input1(buf,20,&flag);
	switch(choose){
	    case '1':
		    free_film_link(head);
		    head=NULL;
		    send_client_data(fd,NULL,"#5",0,0);
		    head=read_flim_data(fd,head);
		    show_all_cinema(fd,head,p);
		    break;
	    case '2':
		    free_movie_link(head2);
		    head2=NULL;
		    send_client_data(fd,NULL,"#13",0,0);
		    head2=read_movie_data(fd,head2);
		    show_all_movie(fd,head2,p);
		    break;
	    case '3':show_own_news(p);break;
	    case '4':edit_cinema(fd);break;
	    case '5':edit_movie(fd);break;
	    case '6':return;
	    default:
		    set_cursor(44,19);
		    clear_cursor(strlen(buf));//视觉清空原有数据
		    fun_cursor_left(strlen(buf));//将光标移回来	
		    break;
	}
    }
}
//显示个人信息
void show_own_news(Custom *p){
    show_table(3,5);//模式3,普通表格
    set_point_ch(33,5,"个人详细信息");
    set_point_ch(23,7,"1.帐号:");set_point_ch(48,7,"2.编号:");
    set_point_ch(23,9,"3.密码:");set_point_ch(48,9,"4.年龄:");
    set_point_ch(23,11,"5.电话:");set_point_ch(48,11,"6.性别:");
    set_point_ch(23,13,"7.住址:");set_point_ch(48,13,"8.职业:");
    updata_table('0',p);
    set_point_ch(33,15,"提示:0.返回");
    set_point_ch(33,17,"请做出选择:");
    char buf[20];
    int flag;
    while(1){
	char choose=fixed_input1(buf,20,&flag);//自定义标准输入，返回首字母
	if(choose=='0')
	    return;
	else{
	    set_cursor(44,17);
	    clear_cursor(strlen(buf));//视觉清空原有数据
	    fun_cursor_left(strlen(buf));//将光标移回来	
	    continue;	
	}
    }
}

//客户搜索电影界面
void seek_movie_menu(int fd,Custom *p){
    while(1){
    show_table(3,3);//模式3,普通表格
    set_point_ch(33,5,"客户搜索电影界面");
    set_point_ch(33,7,"1.影院编号:");
    set_point_ch(33,9,"2.电影编号:");
    set_point_ch(33,11,"3.输入结果:");
    set_point_ch(33,13,"提示:3.确定 4.修改 0.返回");
    set_point_ch(33,15,"请做出选择:");

    MOVIE *head=NULL;
    char buf[20];
    char buf2[20];
    char buf3[20];
    int flag;
    int c_num;
    int m_num;
    information_change2(44,7,buf2);//定点你输入
    information_change2(44,9,buf3);
    set_cursor(44,15);
    char choose=fixed_input1(buf,20,&flag);//自定义标准输入，返回首字母
    if(choose=='3'){
	if(ch_to_int(buf2,&c_num)&&ch_to_int(buf3,&m_num)){//判断是有效数据
	    printf("%d,%d\n",c_num,m_num);
	    send_client_data(fd,NULL,"#7",c_num,m_num);
	    free_movie_link(head);
	    head=NULL;
	    head=read_movie_data(fd,head);//最终获取电影
	    show_all_movie(fd,head,p);
	}
	else{
	    set_point_ch(44,11,"无效输入,请重输");
	    sleep(2);
	    continue;
	}
    }
    else if(choose=='4')
	continue;
    else if(choose=='0')
	return;
    else{
	set_cursor(44,15);
	clear_cursor(strlen(buf));//视觉清空原有数据
//	fun_cursor_left(strlen(buf));//将光标移回来	
//	continue;	
	}
    }
}

//客户购票界面---显示所有电影院
void show_cinema(int fd,Custom *p){
    int ret;
    int flag;
    char buf[20]={};
    FILM *head=NULL;
    MOVIE *head2=NULL;
    while(1){	
    show_table(3,3);//模式3,普通表格
    set_point_ch(33,5,"欢迎进入购票界面");
    set_point_ch(33,7,"1.查看所有影院");
    set_point_ch(33,9,"2.查看所有电影");
    set_point_ch(33,11,"3.搜索电影");
    set_point_ch(33,13,"4.个人信息");

    set_point_ch(33,15,"5.返回");
    set_point_ch(33,17,"请输入选择:");
	
    char choose=fixed_input1(buf,20,&flag);//自定义标准输入，返回首字母
    if(choose=='1'){
	free_film_link(head);
	head=NULL;
	send_client_data(fd,NULL,"#5",0,0);
	head=read_flim_data(fd,head);
	show_all_cinema(fd,head,p);
    }
    else if(choose=='2'){
	free_movie_link(head2);
	head2=NULL;
	send_client_data(fd,NULL,"#13",0,0);
	head2=read_movie_data(fd,head2);
	show_all_movie(fd,head2,p);
    }
    else if(choose=='3'){
	seek_movie_menu(fd,p);
    }
    else if(choose=='4'){
	show_own_news(p);//显示个人信息
    }
    else if(choose=='5')
	return;
    else{
	set_cursor(44,17);
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

    if(ret!=0){
	printf("pthread_cteate error : %s\n",strerror(ret));
	exit(-1);
    }
    /*-------------------------------------------------------*/
    int flag=1;
    char buf[20]={};
    //主循环
    while(1){
	set_cursor(1,5);
	save_cursor();
	show_table(0,5);
	recover_cursor();
	show_menu("客户端界面");
	char choose=fixed_input1(buf,20,&flag);//自定义标准输入，返回首字母
	switch(choose){
	    case '1':client_register(cfd);break;	//注册
	    case '2':client_login(cfd);break;	//登录
	    case '3':exit(-1);
	    default :break;
	}
    }
    return 0;
}
