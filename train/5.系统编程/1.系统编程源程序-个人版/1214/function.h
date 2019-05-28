#ifndef _FUNCTION_
#define _FUNCTION_

typedef struct{
    int cfd;	    //监听到的客户文件套接字
    char buff[2048];//Msg_head + Custom
}Total_msg;

typedef struct{
    char len[10];	//sizeof(Custom) + sizeof(Msg_head)
    char msg_type[10];	//客户发送给服务器 #3 注册  #4 登录
}Msg_head;		//服务器相应客户端 #1 成功  #2 失败

typedef struct{
    char account[20];	//帐号
    char number[10];	//编号
    char password[10];	//密码
    char age[10];	//年龄
    char sex[10];	//性别
    char telephone[20];	//电话
    char address[20];	//住址
    char status[10];	//身份  client or admin
}Custom;


//函数声明
//整型转数字字符
int int_to_ch(int m,char *dest,int size);
//数字字符转整型
int ch_to_int(const char *src,int *m);
//显示菜单
void show_menu(char *str);
//追加套接字，设置为非阻塞
void setnonblocking(int sock);
//向套接字写入数据
int socket_write(int sockfd,char *buff,int len);
//读取套接字里数据
int socket_read(int sockfd,char *buff,int len);
//客户开始注册
void start_register(int fd,char mode);

#endif
