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
    char yard[10];	//片场
    int new_number;	//新编号
}Msg_head;		

typedef struct{
    char number[15];	//编号
    char account[20];	//帐号
    char password[15];	//密码
    char age[15];	//年龄
    char sex[15];	//性别
    char telephone[20];	//电话
    char address[20];	//住址
    char wallet[15];	//钱包
    char status[15];	//身份  client or admin
}Custom;

typedef struct c{
    Custom value;
    struct c *next; 
}CUSTOM;

typedef struct{
//     pthread_cond_t cond;
     pthread_cond_t rec_cond;
     pthread_cond_t sen_cond;
     pthread_cond_t show_cond;

     pthread_mutex_t mutex;
     int cfd;
     int rec_ok;
     int sen_ok;
     int show_ok;
 }Thread_t;
/*-------------------------------------------------------------*/
typedef struct{
    int cinema_number; //电影院编号
    char cinema_name[20];   //电影院名字
}Cinema;

//电影院--结构体
typedef struct{
    Cinema film;	    //电影院编号+电影院名字
    int film_num;	    //电影院影片数
}Film;

typedef struct{
    int c_number;	    //电影院编号
    int film_yard[10];	    //所有场地 1-10场地
    int film_seat[50];	    //所有座位 1-50
}Film_t;

//电影院节点--结构体
typedef struct f{
    Film value;
    struct f *next;
}FILM;

/*-------------------------------------------------------------*/
//电影--结构体
typedef struct{
    int movie_number;	    //电影编号
    char movie_name[20];    //电影名字
//    Cinema movie_new;	    //电影院编号+电影院名字
    int c_number;	    //电影院编号
    char c_name[20];	    //电影院名字

    char play_time[15];	    //播放时长  45分钟
    char show_time[15];	    //上映时间
    int movie_price;	    //电影票价 
    int ticket_count;	    //电影票数
    int play_yard;	    //播放场地
}Movie;

//电影节点--结构体
typedef struct m{
    Movie value;
    struct m *next;
}MOVIE;

/*-------------------------------------------------------------*/
//电影票--结构体--购买记录存根

typedef struct{
    char year[10];
    char month[10];
    char day[10];
    char hour[10];
    char minute[10];
    char second[10];
}TIME;

typedef struct{
    int number;		//客户编号
    char name[20];	//客户名字
    Movie ticket;	//电影所有信息
    int t_seat;		//座位  1-50
    char d_date[15];	//购买日期
    char t_time[15];	//购买时间
}Ticket;

//电影票节点--结构体
typedef struct t{
    Ticket value;
//    int t_seat;
    struct t *next;
}TICKET;

/*-------------------------------------------------------------*/
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

void client_register(int fd);

int send_to_server(Custom *p,int mode,int *right,Thread_t *t);

void start_register(char mode,int fd);

void client_login(int fd);

void *client_handler(void *arg);

void show_data(Custom *p);

void *client_task(void *arg);

/*----------------------------------------------*/

void show_cinema(int fd,Custom *p);
void show_all_cinema(int fd,FILM *head,Custom *p);


//void send_client_data(int fd,Msg_head *m,void *arg,char *type);
int send_client_data(int fd,void *arg,char *type,int num,int yard);
int read_server_data(int fd,Custom *p);//用于登录注册，读取数据

FILM * read_flim_data(int fd,FILM *head);//用于读取film表格
FILM *create_film_link(FILM *head,Film d1);//尾插入
void free_film_link(FILM *head);
void printf_film_link(FILM *head);
int film_node_num(FILM *head);
int seek_film_num(FILM *head,int num);
void show_all_cinema(int fd,FILM *head,Custom *p);

MOVIE *read_movie_data(int fd,MOVIE *head);
MOVIE *create_movie_link(MOVIE *head,Movie d1);
void free_movie_link(MOVIE *head);
void printf_movie_link(MOVIE *head);
MOVIE *seek_movie_num(MOVIE *head,int num);
int movie_node_num(MOVIE *head);

int read_movie_seat(int fd,char *s,int size);
void show_movie_seat(int fd,char *s,MOVIE *t,Custom *p);
int seek_seat_ok(char *s,int n);
int buy_ticket(int fd,MOVIE *m,Custom *p,int seat);
void show_all_movie(int fd,MOVIE *head,Custom *p);

CUSTOM *create_custom_link(CUSTOM *head,Custom d1);
void free_custom_link(CUSTOM *head);

int read_new_number(int fd);

void show_admin(int fd,Custom *p);
void seek_movie_menu(int fd,Custom *p);
void show_own_news(Custom *p);
void edit_cinema(int fd);
void edit_movie(int fd);
void add_cinema(int fd);
void delete_cinema(int fd);
void change_cinema(int fd);
void change_cinema_name(int fd,Film *f);

void add_movie(int fd);
void add_movie_mode1(int fd);
void add_movie1(int fd,Film *f);
void show_movie_clause();

int read_film_one(int fd,Film *f);
int read_movie_one(int fd,Movie *m);

void change_movie(int fd);
void change_movie_news(int fd,Movie *m);
void movie_information_change(int x,int y,char choose,Movie *m,int size,int *right);
#endif
