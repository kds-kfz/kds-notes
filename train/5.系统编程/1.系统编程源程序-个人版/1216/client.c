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

bool rec_ok=false;

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
void client_register(int fd){
    char mode=register_mode();//获取注册模式

    start_register(fd,mode);//开始注册
}

//打包注册数据并发送
int send_to_server(int fd,Custom *p,int mode,int *right){
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
	while(!rec_ok)
	pthread_cond_;
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

void start_register(int fd,char mode){
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
		     send_to_server(fd,p,1,&right);return ;
//		     res=send_to_server(fd,p,1,&right);
//		     if(res)return;//确定,打包数据并发送
//		     else break;
	    default :
		    recover_cursor();//恢复光标位置
		    clear_cursor(sizeof(buf)-1);//视觉清空原有数据
		    break;
	    }
//	interface(26,86,21);//画边框函数
	}
    }
    else if(mode=='2'){
	while(1){
	right=1;
	show_clause(mode,2);//显示客户注册的条款
	set_cursor(1,5);
	printf(cursor_right3"客户详细注册界面");
	
	information_change(30,7,'1',p->account,12,&right);//帐号*
	information_change(55,7,'2',p->number,12,&right);//编号 默认
	information_change(30,9,'3',p->password,12,&right);//密码*
	information_change(55,9,'4',p->age,12,&right);
	information_change(30,11,'5',p->telephone,12,&right);
	information_change(55,11,'6',p->sex,12,&right);
	information_change(30,13,'7',p->address,12,&right);
	information_change(55,13,'8',p->status,12,&right);//身份*
	
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
	    send_to_server(fd,p,2,&right);
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

void client_login(int fd){
    show_table(3,6);//模式3,普通表格
//    set_cursor(1,5);
//    printf(cursor_right3"客户登录界面\n\n");
    set_point_ch(33,5,"客户登录界面");
    set_point_ch(33,7,"帐号:");
    set_point_ch(33,9,"密码:");

    Custom custom;
    Msg_head msghead;
    char buff[2048];
    int ret;
    int flag;
    char account[20]={};
    char key[15]={};

    memset(&custom, 0, sizeof(custom));
    set_cursor(38,7);
    fixed_input(account,20,&flag);
    set_cursor(38,9);
    fixed_input(key,15,&flag);

//    printf(cursor_right4"帐号:\n\n");
//    printf(cursor_right5"密码:\n\n");

//    scanf("%s",custom.account);
//    scanf("%s",custom.password);

    strcpy(custom.account,account);
    strcpy(custom.password,key);
    
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
//	interface(26,86,21);//画边框函数
	set_cursor(1,5);
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
//    char result[10]={};
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
//	    write(STDOUT_FILENO,buff,ret);
	    memcpy(&msghead, buff, sizeof(msghead));
	    if(!strcmp(msghead.msg_type,"#3")){//注册
		if(!strcmp(msghead.result,"#1"))
		    set_point_ch(31,19,"注册成功");
		else if(!strcmp(msghead.result,"#2"))
		    set_point_ch(31,19,"注册失败");
	    }
	    else if(!strcmp(msghead.msg_type,"#4")){//登录
		if(!strcmp(msghead.result,"#1")){
		    set_point_ch(31,19,"登录成功");
		    memcpy(&custom,buff+sizeof(msghead),sizeof(custom));
		    printf("%s,%s\n",custom.account,custom.password);
		}
		else if(!strcmp(msghead.result,"#2")){
		    set_point_ch(31,19,"登录失败");
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
