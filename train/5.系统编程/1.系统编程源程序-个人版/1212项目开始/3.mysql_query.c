#include<stdio.h>
#include<stdlib.h>
#include<mysql/mysql.h>
//3.mysql_query.c
#if 0
    int STDCALL mysql_query(MYSQL *mysql, const char *q);
    mysql
    q
    返回值
#endif
int main(){
    MYSQL mysql;
    MYSQL *connect;
    //初始化
    connect=mysql_init(&mysql);
    int ret;
    if(connect==NULL){
	ret=mysql_errno(&mysql);
	printf("mysql_init error : %d\n",ret);
	exit(-1);
    }
    printf("mysql_init ok\n");
    //链接数据库
    connect=mysql_real_connect(connect,"localhost","root","123","stu",0,NULL,0);
    if(connect==NULL){
	ret=mysql_errno(&mysql);
	printf("mysql_real_connect error : %d\n",ret);
	exit(-1);
    }
    printf("mysql_real_connect ok\n");
    char *sql="select *from s1";
    ret=mysql_query(connect,sql);
    if(ret!=0){
	printf("mysql_query error : %d\n",ret);
	exit(-1);
    }
    printf("mysql_query ok\n");

    mysql_close(connect);
    return 0;
}
