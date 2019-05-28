#ifndef _FUNCTION_
#define _FUNCTION_
#include <pthread.h>
typedef struct{
    int cfd;	    //监听到的客户文件套接字
    char buff[2048];//Msg_head + Custom
}Total_msg;

typedef struct{
    char len[12];	//sizeof(Custom) + sizeof(Msg_head)
    char msg_type[10];	//客户发送给服务器 #3 注册  #4 登录
    char result[10];	//服务器相应客户端 #1 成功  #2 失败
}Msg_head;		

typedef struct{
    char account[20];	//帐号
    char number[15];	//编号
    char password[15];	//密码
    char age[15];	//年龄
    char sex[15];	//性别
    char telephone[20];	//电话
    char address[20];	//住址
    char status[15];	//身份  client or admin
}Custom;

typedef struct{
//     pthread_cond_t cond;
     pthread_cond_t rec_cond;
     pthread_cond_t sen_cond;

     pthread_mutex_t mutex;
     int cfd;
     int rec_ok;
     int sen_ok;
 }Thread_t;

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

void information_change(int x,int y,char choose,char arr[],int size,int *right);
void information_change2(int x,int y,char arr[]);

void information_status(char arr[]);

void show_table(int mode,int color);

void show_clause(char mode,int m);

char register_mode();

void client_register(Thread_t *p);

int send_to_server(Custom *p,int mode,int *right,Thread_t *t);

void start_register(char mode,Thread_t *t);

void client_login(Thread_t *t);

void *client_handler(void *arg);

void show_data(Custom *p);

void *client_task(void *arg);
#endif
