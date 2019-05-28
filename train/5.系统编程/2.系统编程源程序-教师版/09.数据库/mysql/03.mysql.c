#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mysql/mysql.h>

int main()
{
    int ret;
    MYSQL mysql;
    MYSQL *connect;
    connect = mysql_init(&mysql);	
    if(connect == NULL){
	ret = mysql_errno(&mysql);
	printf("mysql_init err:%d\n",ret);
	return ret;
    }
    printf("connect ok\n");

    connect = mysql_real_connect(connect,"localhost","root","123","worker",0,NULL,0);
    if(connect == NULL){
	ret = mysql_errno(&mysql);
	printf("mysql_init err:%d\n",ret);
	return ret;
    }
    printf("mysql_real_connect ok\n");

    char *sql = "select *from w1";
    ret = mysql_query(connect,sql);
    if(ret != 0){
	printf("mysql_query err\n");
	return ret;
    }
    printf("query ok\n");

    mysql_close(&mysql);
    return 0;
}

