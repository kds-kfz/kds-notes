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
	
	mysql_close(&mysql);
	return 0;
}

