#ifndef _MYSQL_H_
#define _MYSQL_H_
#include<mysql/mysql.h>

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
void Mysql_init(Mysql_t *m);
//数据库连接
void Mysql_connect(Mysql_t *m,const char *s);
//发送命令
void Mysql_query(Mysql_t *m,const char *s);
//打印数据
void Mysql_show_data(Mysql_t *m,const char *s);
//释放数据库
void My_free(Mysql_t *m);
//创建一个表格
void Mysql_create_table(Mysql_t *m,const char *s1,const char *s2);
//向表格插入数据
void Mysql_insert_data(Mysql_t *m,const char *s);

#endif
