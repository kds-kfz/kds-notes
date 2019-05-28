#include<stdio.h>
#include"Mysql.h"

char *sql="create table custom(account varchar(20),number varchar(15),password varchar(15),age varchar(15),sex varchar(15),telephone varchar(20),address varchar(20),status varchar(15))";

char *sql1="insert into custom(account,number,password,age,sex,telephone,address,status) values('lisi','1001','key123','20',\'男\','12345678901',\'新家波\','client');";
//数据库封装测试
int main(){
    Mysql_t *mysql=(Mysql_t *)malloc(sizeof(Mysql_t));
    Mysql_init(mysql);
    Mysql_connect(mysql,"project");
//    Mysql_create_table(mysql,sql);
    Mysql_query(mysql,"set names utf8");
    Mysql_insert_data(mysql,sql1);
    Mysql_query(mysql,"set names utf8");
    Mysql_show_data(mysql,"select *from custom");
    My_free(mysql);
    return 0;
}
