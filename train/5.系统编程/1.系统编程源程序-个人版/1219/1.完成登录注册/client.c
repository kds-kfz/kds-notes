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
int read_server_data(int fd,char *type){
    int c_len;
    int ret;
    char read_buf[1024];
    char buf[512];
    Msg_head msghead;
//    while(1){
    memset(read_buf,0,1024);
    ret=read(fd, read_buf, 1024);
    
//    printf("ret=%d\n",ret);
    if(ret<0)
	return -1;
    else if(ret==0)
	return 0;
    else{//当读取成功，向服务器发送"#"
	memcpy(&msghead,read_buf,sizeof(Msg_head));//将读到的内容放到msghead
//	printf("msghead.result : %s\n",msghead.result);
	if(!strcmp(msghead.msg_type,type)){//是注册
	    if(!strcmp(msghead.result,"#1"))//成功
		return 1;//返回成功
	    else if(!strcmp(msghead.result,"#2"))
		return 2;//返回失败
	}
	else if(!strcmp(msghead.msg_type,type)){//是登录
	    if(!strcmp(msghead.result,"#1"))//成功
		return 1;//返回成功
	    else if(!strcmp(msghead.result,"#2"))
		return 2;//返回失败
	}
	/*
	else if(!strcmp(msghead.msg_type,"#5")){//是查询film表格
	    //处理read_buf内容
	    if

	    c_len=sizeof(Msg_head);
	    int_to_ch(c_len,m->len,sizeof(m->len));
	    strcpy(m->msg_type,"#");
	    memcpy(buf,m,sizeof(Msg_head));
	    ret=write(fd,buf,c_len);//向服务器发送"#"
	    }
	}
	*/
    }
}
//用于获取电影院信息
FILM *read_flim_data(int fd,FILM *head){
    char read_buf[1024];
    Msg_head msghead;
    Film d1;	    
    int ret;
    while(1){
    ret=read(fd, read_buf, 1024);
    if(ret<0){
	perror("read fd error\n");
	return NULL;
    }
    else if(ret==0)
	break;
    else{
	memset(&d1,0,sizeof(Film));
	memcpy(&msghead,read_buf,sizeof(Msg_head));//将读到的内容放到msghead
	if(!strcmp(msghead.result,"*1")){
    
	    memcpy(&d1,read_buf+sizeof(Msg_head),sizeof(Film));
	    d1.film.cinema_number=ntohl(d1.film.cinema_number);
	    d1.film_num=ntohl(d1.film_num);

	    head=create_film_link(head,d1);
	    memset(&msghead,0,sizeof(msghead));
	}
	else if(!strcmp(msghead.result,"*0"))//无效信息
	    break;
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
    while(1){
    ret=read(fd, read_buf, 1024);
    if(ret<0){
	perror("read fd error\n");
	return NULL;
    }
    else if(ret==0)
	break;
    else{
	memset(&d1,0,sizeof(Movie));
	memcpy(&msghead,read_buf,sizeof(Msg_head));//将读到的内容放到msghead
//	printf("result %s\n",msghead.result);
	if(!strcmp(msghead.result,"*1")){
    
	    memcpy(&d1,read_buf+sizeof(Msg_head),sizeof(Movie));
	    
	    d1.movie_number=ntohl(d1.movie_number);
	    d1.c_number=ntohl(d1.c_number);
	    d1.movie_price=ntohl(d1.movie_price);
	    d1.ticket_count=ntohl(d1.ticket_count);
	    d1.play_yard=ntohl(d1.play_yard);

	    head=create_movie_link(head,d1);
	    memset(&msghead,0,sizeof(msghead));
	}
	else if(!strcmp(msghead.result,"*0"))//无效信息
	    break;
	}
    }
    return head;
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
	int_to_ch(num,msghead.result,sizeof(msghead.result));//电影编号
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

void start_register(char mode,int fd){
    Custom custom;
    Custom *p=&custom;
    char buf[20]={};//选择缓存区
    int flag;
    int res;
    int right;
    
    memset(&custom, 0, sizeof(custom));
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
	    case '2':information_change(55,7,choose,p->number,12,&right);break;//编号
	    case '3':information_change(30,9,choose,p->password,12,&right);break;//密码
	    case '4':information_change(55,9,choose,p->age,12,&right);break;
	    case '5':information_change(30,11,choose,p->telephone,12,&right);break;
	    case '6':information_change(55,11,choose,p->sex,12,&right);break;
	    case '7':information_change(30,13,choose,p->address,12,&right);break;
	    case '8':information_change(55,13,choose,p->status,12,&right);break;//身份*
	    case '9':
		     if(right){
			send_client_data(fd,p,"#3",0,0);
			int res=read_server_data(fd,"#3");
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
	set_cursor(1,5);
	printf(cursor_right3"客户详细注册界面");
	int num[8]={0};
	
	information_change(30,7,'1',p->account,12,&right);num[0]=right;
	information_change(55,7,'2',p->number,12,&right);num[1]=right;
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
		int res=read_server_data(fd,"#3");
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
    int res=read_server_data(fd,"#4");
    if(res==1){
//	set_point_ch(38,11,"登录成功");
	show_cinema(fd);
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

void show_all_movie(int fd,MOVIE *head){
    int flag;
    char buf[20];
    int n;
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
	    if(seek_movie_num(head,n)){
		//如果有该电影编号,则显示该电影的片场和座位

	    }
	    /*
	    else{
//		recover_cursor();
		set_cursor(48,11+i);
		clear_cursor(strlen(buf));//视觉清空原有数据
		fun_cursor_left(strlen(buf));//将光标移回来	
		continue;
	    }
	    */
	}	
    }
}

void show_all_cinema(int fd,FILM *head){ 
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
		    show_all_movie(fd,head2);
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

//显示所有电影院
void show_cinema(int fd){
    int ret;
    int flag;
    char buf[20]={};
    FILM *head=NULL;
    while(1){	
    show_table(3,3);//模式3,普通表格
    set_point_ch(33,5,"欢迎进入购票界面");
    set_point_ch(33,7,"1.查看所有影院");
    set_point_ch(33,9,"2.返回");
    set_point_ch(33,11,"请输入选择:");
	
    char choose=fixed_input1(buf,20,&flag);//自定义标准输入，返回首字母
    if(choose=='1'){
	free_film_link(head);
	head=NULL;
	send_client_data(fd,NULL,"#5",0,0);
	head=read_flim_data(fd,head);
	show_all_cinema(fd,head);
    }
    else if(choose=='2')
	return;
    else{
	set_cursor(44,11);
	clear_cursor(strlen(buf));//视觉清空原有数据
	fun_cursor_left(strlen(buf));//将光标移回来	
	continue;
	}
    }

}

//	if(login_ok)
//	    show_cinema(t);
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
