#include<stdio.h>
#include<stdlib.h>
#include<mysql/mysql.h>

#include"Mysql.h"
#include"function.h"
#include<string.h>

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
    Mysql_query(m,"set names utf8");//设置支持插入中文
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

void Mysql_insert_data(Mysql_t *m,const char *s1,const char *s2){
    Mysql_query(m,s1);
}
//删除某个表格满足条件的内容
void delete_table(Mysql_t *m,const char *s1,const char *s2){
    char sql_buf[1024];
    sprintf(sql_buf,"delete from %s where %s;",s1,s2);
    Mysql_query(m,sql_buf);
}

//清空某个表格内容
void clear_table(Mysql_t *m,const char *s){
    Mysql_query(m,s);
}
//删除所有表格
void delete_all_table(Mysql_t *m){
    char *sql1="drop table person";
    Mysql_query(m,sql1);
}

//创建所有表格
void create_all_table(Mysql_t *m){
    //创建person表格,存放顾客和管理员信息
    char *sql1="create table person(number varchar(15),account varchar(20),password varchar(15),age varchar(15),sex varchar(15),telephone varchar(20),address varchar(20),wallet varchar(15),status varchar(15))";

    Mysql_query(m,sql1);//创建person表格
}

//向person表格增加内容
void insert_person(Mysql_t *m,Custom *p){
    //增加前先删掉该条信息
    char delete_buf[1024];
    sprintf(delete_buf,"delete from person where number='%s';",p->number);
    Mysql_query(m,delete_buf);

    char sql_buf[1024];
    sprintf(sql_buf,"insert into person(number,account,password,age,sex,telephone,address,wallet,status) values('%s','%s','%s','%s','%s','%s','%s','%s','%s');",
    p->number,p->account,p->password,p->age,p->sex,
    p->telephone,p->address,p->wallet,p->status);
    Mysql_query(m,"set names utf8");//设置支持插入中文
    Mysql_query(m,sql_buf);
}
//检查数person表格number与帐号是否存在
char check_person(Mysql_t *m,Custom *p){
    char check_buf[1024];
    sprintf(check_buf,"select count(*) from person where number='%s' and account='%s';",p->number,p->account);
    Mysql_query(m,check_buf);
    
    m->result=mysql_store_result(m->connect);
    MYSQL_ROW ret=mysql_fetch_row(m->result);//获取满足条件的行数
    return *ret[0];
}
//查找数据库，编号+帐号+密码 与 登录者是否完全相同
char *test_new_custom(Mysql_t *m,Custom *p){
    Mysql_query(m,"set names utf8");
    char test_buf[1024];
    //sprintf(test_buf,"select *from person where account='%s' and password='%s' and number='%s';",
    sprintf(test_buf,"select *from person;");

    Mysql_query(m,test_buf);

    m->result=mysql_store_result(m->connect);
    m->col=mysql_num_fields(m->result);
    
    int a,b;//帐号，密码标志位
    while(m->row=mysql_fetch_row(m->result)){
	a=0;b=0;
	for(int i=0;i<m->col;i++){
	    if(!strcmp(p->account,m->row[i])&&i==1)
		a=1;
	    else if(!strcmp(p->password,m->row[i])&&i==2)
		b=1;
	}
	if(a==1&&b==1)
	    break;
    }
    if(a==1&&b==1)
	return "#1";
    else
	return "#2";
}

