#include<stdio.h>
#include<stdlib.h>
#include<mysql/mysql.h>

#include"Mysql.h"


//数据库初始化
void Mysql_init(Mysql_t *m){
    m->connect=mysql_init(&(m->mysql));
    if(m->connect==NULL){
	int ret=mysql_errno(&(m->mysql));
	printf("mysql_init error : %d\n",ret);
	m->connect=NULL;
    }
}

//数据库连接
void Mysql_connect(Mysql_t *m,const char *s){
    m->connect=mysql_real_connect(m->connect,"localhost","root","123",s,0,NULL,0);
    if(m->connect==NULL){
	int ret=mysql_errno(&(m->mysql));
	printf("mysql_real_connect error : %d\n",ret);
	m->connect= NULL;
    }
}

//发送命令
void Mysql_query(Mysql_t *m,const char *s){
    int ret=mysql_query(m->connect,s);
    if(ret!=0){
    printf("mysql_query error : %d\n",ret);
    m->connect=NULL;
    }
}

//打印数据
void Mysql_show_data(Mysql_t *m,const char *s){
    Mysql_query(m,s);

    m->result=mysql_store_result(m->connect);
    m->col=mysql_num_fields(m->result);

    m->fields=mysql_fetch_fields(m->result);

    for(int i=0;i<m->col;i++)
	printf("%s\t",m->fields[i].name);
    putchar(10);

    while(m->row=mysql_fetch_row(m->result)){
	for(int i=0;i<m->col;i++)
	    printf("%s\t",m->row[i]);
	putchar(10);
    }
}

//释放数据库
void My_free(Mysql_t *m){
    mysql_free_result(m->result);
    mysql_close(m->connect);
}

//创建表格
//char *sql1=
//"insert into s1(id,name,sex,chinese,english,math) 
//values(2008,\'lisi\',\'y\',50,60,70)";

//数据库名称+表格+格式
//create table custom(account varchar(20);
void Mysql_create_table(Mysql_t *m,const char *s1,const char *s2){
    char sql_buf[1024];
    sprintf(sql_buf,"create table %s(%s);",s1,s2);
    Mysql_query(m,sql_buf);
}
void Mysql_insert_data(Mysql_t *m,const char *s1,const char *s2){
    Mysql_query(m,s);
}

