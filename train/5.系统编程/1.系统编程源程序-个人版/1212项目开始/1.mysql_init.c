#include<stdio.h>
#include<stdlib.h>
#include<mysql/mysql.h>
//1.mysql_init.c
#if 0
    MYSQL *STDCALL mysql_init(MYSQL *mysql);
    mysql：
    -----------------------------------------------------------
    返回值：
    ***********************************************************
    void STDCALL mysql_close(MYSQL *sock);
    sock：
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
    return 0;
}
