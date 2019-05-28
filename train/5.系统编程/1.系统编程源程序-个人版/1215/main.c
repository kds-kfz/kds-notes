#include<stdio.h>
#include"Mysql.h"

//数据库封装测试
int main(){
    Mysql_t *mysql=(Mysql_t *)malloc(sizeof(Mysql_t));
    Mysql_init(mysql);
    Mysql_connect(mysql,"stu");
    Mysql_query(mysql,"set names utf8");
    Mysql_show_data(mysql,"select *from s1");
    My_free(mysql);
    return 0;
}
