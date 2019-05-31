#ifndef _MYSQL_H_
#define _MYSQL_H_
#include<mysql/mysql.h>
#include"function.h"
//自定义数据库结构体
typedef struct{
    MYSQL mysql;
    MYSQL *connect;
    MYSQL_RES *result;
    MYSQL_FIELD *fields;
    MYSQL_ROW row;
    int col;
}Mysql_t;


//数据库初始化
extern void Mysql_init(Mysql_t *m);
//数据库连接
extern void Mysql_connect(Mysql_t *m,const char *s);
//发送命令
extern void Mysql_query(Mysql_t *m,const char *s);
//打印数据
extern void Mysql_show_data(Mysql_t *m,const char *s);
//释放数据库
extern void My_free(Mysql_t *m);
//创建一个表格
extern void Mysql_create_table(Mysql_t *m,const char *s1,const char *s2);
extern void Mysql_insert_data(Mysql_t *m,const char *s1,const char *s2);
//向表格插入数据

/*-------------------------------------------------*/
extern void insert_person(Mysql_t *m,Custom *p);
//向person表格增加内容
/*-------------------------------------------------*/
extern void create_all_table(Mysql_t *m);
//创建所有表格
/*-------------------------------------------------*/
extern void delete_all_table(Mysql_t *m);
//删除所有表格
/*-------------------------------------------------*/
extern void clear_table(Mysql_t *m,const char *s);
//清空某个表格内容
/*-------------------------------------------------*/
extern void delete_table(Mysql_t *m,const char *s1,const char *s2);
//删除某个表格满足条件的内容
/*-------------------------------------------------*/
extern char check_person(Mysql_t *m,Custom *p);
//用于注册
//返回值：字符1说明已注册，字符0说明没注册
/*-------------------------------------------------*/
extern char *test_new_custom(Mysql_t *m,Custom *p);
//用于：用户登录
//返回值：#1:成功,#2:失败

void insert_film(Mysql_t *m,Film *p);

void insert_movie(Mysql_t *m,Movie *p);

void insert_filmyard(Mysql_t *m,Film_t *p);

void insert_filmseat(Mysql_t *m,Film_t *p);

void insert_all_data(Mysql_t *m);

void read_film_table(Mysql_t *m,char buf[]);

void seek_custom_news(Mysql_t *m,Custom *p1,Custom *p2);

void insert_record_data(Mysql_t *m,Ticket *t);

void updata_film_seat(Mysql_t *m,int c_num,int y_num,int s_num);

void updata_movie_data(Mysql_t *m,int m_num,int n);

void updata_custom_data(Mysql_t *m,int m_num,int n);

/*-----------------------------------------*/
int Max(int a,int b);
int back_person_data(Mysql_t *m);
int back_film_data(Mysql_t *m);
int back_movie_data(Mysql_t *m);

char check_film(Mysql_t *m,Film *f);
char check_film_by_num(Mysql_t *m,Film *f);
char check_movie_by_name(Mysql_t *m,Movie *m2);
void get_flim_news(Mysql_t *m,Film *f);

char check_movie_by_num(Mysql_t *m,Movie *m2);
void get_movie_news(Mysql_t *m,Movie *m2);
#endif
