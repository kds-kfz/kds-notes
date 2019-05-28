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

//信息变更
void information_change(int x,int y,char arr[]){
//    fun_cursor_up(up);//光标上移up行
//    fun_cursor_right(right);//光标右移right列
    int flag;
    set_cursor(x,y);
    clear_cursor(sizeof(&arr));//视觉清空原有数据
    fun_cursor_left(sizeof(&arr));//将光标移回来
    
    fixed_input(arr,sizeof(&arr),&flag);
//    set_cursor(30,17);//输入完毕设置光标位置
//    menu_choose(20);//接收回车
}

void show_table(int mode,int color){
    draw_row_line(20,3,1,"+",color);
    draw_row_line(21,3,46,"-",color);
    draw_row_line(67,3,1,"+",color);
    
    draw_col_line(20,4,17,"|",color);
    if(mode==1)
	draw_col_line(43,7,9,"|",color);
    else if(mode==2)
	draw_col_line(43,7,7,"|",color);
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
    else if(mode=='2')//详细注册模式
	set_point_ch(23,15,"结  果:");
}

//获取注册模式函数
char register_mode(){
    while(1){
    char buf[20]={};
    int flag;
    interface(26,86,21);//画边框函数
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
void client_register(int fd){
    char mode=register_mode();//获取注册模式

    start_register(fd,mode);//开始注册
}

//打包注册数据并发送
void send_to_server(int fd,Custom *p,int mode){
    Msg_head msghead;
    char buff[2048];
    int ret;
    
    int c_len=sizeof(Msg_head)+sizeof(Custom);
    int_to_ch(c_len,msghead.len,10);
    strcpy(msghead.msg_type,"#3");
    memcpy(buff, &msghead, sizeof(msghead));
    memcpy(buff + sizeof(msghead), p, sizeof(Custom));
    ret = write(fd, buff, c_len);
    if(mode==1)
	set_cursor(31,19);
    else if(mode==2)
	set_cursor(31,15);
    if(ret<0){
	perror(red"client_register write to server fail\n");
	sleep(2);
    }
    else{
	printf(yellow"data send ok\n");
	sleep(2);
    }
}

void start_register(int fd,char mode){
    Custom custom;
    Custom *p=&custom;
    char buf[20]={};//选择缓存区
    int flag;

    interface(26,86,21);//画边框函数
    
    memset(&custom, 0, sizeof(custom));
    if(mode=='1'){
	printf(cursor_right2"客户快速注册界面\n");
	show_clause(mode,1);//显示客户注册的条款
	while(1){
	set_cursor(30,17);
	save_cursor();//保存光标位置
	clear_cursor(sizeof(buf));//视觉清空原有数据
	fun_cursor_left(sizeof(buf));//将光标移回来
	char choose=fixed_input1(buf,20,&flag);
	switch(choose){
	    case '0':return;
	    case '1':information_change(30,7,p->account);break;//帐号*
	    case '2':information_change(55,7,p->number);break;//编号 默认
	    case '3':information_change(30,9,p->password);break;//密码*
	    case '4':information_change(55,9,p->age);break;
	    case '5':information_change(30,11,p->telephone);break;//电话*
	    case '6':information_change(55,11,p->sex);break;
	    case '7':information_change(30,13,p->address);break;
	    case '8':information_change(55,13,p->status);break;//身份*
	    case '9':send_to_server(fd,p,1);return;//确定,打包数据并发送
	    default :
		    recover_cursor();//恢复光标位置
		    clear_cursor(sizeof(buf)-1);//视觉清空原有数据
		    break;
	    }
	}
    }
    else if(mode=='2'){
	printf(cursor_right3"客户详细注册界面\n");
	show_clause(mode,2);//显示客户注册的条款
	    
	information_change(30,7,p->account);//帐号*
	information_change(55,7,p->number);//编号 默认
	information_change(30,9,p->password);//密码*
	information_change(55,9,p->age);
	information_change(30,11,p->telephone);//电话*
	information_change(55,11,p->sex);
	information_change(30,13,p->address);
	information_change(55,13,p->status);
	send_to_server(fd,p,2);
    }
    
}

void client_login(int fd){
    printf("客户登录函数\n");
    Custom custom;
    Msg_head msghead;
    char buff[2048];
    int ret;

    memset(&custom, 0, sizeof(custom));
    printf("帐号：");
    scanf("%s",custom.account);
    printf("密码：");
    scanf("%s",custom.password);
    
    int c_len=sizeof(msghead)+sizeof(custom);
    int_to_ch(c_len,msghead.len,10);
    strcpy(msghead.msg_type,"#4");
    memcpy(buff, &msghead, sizeof(msghead));
    memcpy(buff + sizeof(msghead), &custom, sizeof(custom));
    ret = write(fd, buff, c_len);
    if(ret<0)
	perror("client_login write to server fail\n");
}

void *client_handler(void *arg){
    int *p=(int *)arg;
    int fd=*p;
    int flag=1;
    char buf[20]={};
    while(1){
	interface(26,86,21);//画边框函数
	save_cursor();
	show_table(0,5);
	recover_cursor();
	show_menu("客户端界面");
	char choose=fixed_input1(buf,20,&flag);//自定义标准输入，返回首字母
	switch(choose){
	    case '1':client_register(fd);break;	//注册
	    case '2':client_login(fd);break;	//登录
	    case '3':exit(-1);
	    default :break;
	}
    }
}

int main(){
    int cfd;
    int ret;
    struct sockaddr_in caddr;
    char buf[512];
    char read_buf[512];

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

    ret = pthread_create(&pth, NULL, client_handler, &cfd);
    char buff[2048]={};
    Msg_head msghead;
    char result[10]={};
    Custom custom;

    if(ret!=0){
	printf("pthread_cteate error : %s\n",strerror(ret));
	exit(-1);
    }
    while(1){
	memset(buff, 0, 2048);
	ret=read(cfd, buff, 2048);
	if(ret<0){
	    perror("read fd error\n");
	    exit(-1);
	}
	else if(ret==0)
	    break;
	else{
	    write(STDOUT_FILENO,buff,ret);
	    memcpy(&msghead, buff, sizeof(msghead));
	    if(!strcmp(msghead.msg_type,"#3")){//注册
		memcpy(result, buff + sizeof(msghead), sizeof(result));
		if(!strcmp(result,"#1"))
		    printf("注册成功\n");
		else if(!strcmp(result,"#2"))
		    printf("注册失败\n");
	    }
	    else if(!strcmp(msghead.msg_type,"#4")){//登录
		memcpy(result, buff + sizeof(msghead), sizeof(result));
		if(!strcmp(result,"#1")){
		    printf("登录成功\n");
		    memcpy(&custom,buff+sizeof(msghead)+sizeof(result),sizeof(custom));
		    printf("%s,%s\n",custom.account,custom.password);
		}
		else if(!strcmp(result,"#2")){
		    printf("登录失败\n");
		    if(!strcmp(result,"!1"))
			printf("帐号错误\n");
		    else if(!strcmp(result,"!2"))
			printf("密码错误\n");
		    else if(!strcmp(result,"!3"))
			printf("帐号与密码错误\n");
		}
	    }
	}
    }
    pthread_join(pth, NULL);
    return 0;
}
