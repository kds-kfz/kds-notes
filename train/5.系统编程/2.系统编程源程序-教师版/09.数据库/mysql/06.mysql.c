#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mysql/mysql.h>

int main()
{
		int ret, i;
		MYSQL mysql;
		MYSQL *connect;
		connect = mysql_init(&mysql);	
		if(connect == NULL){
				ret = mysql_errno(&mysql);
				printf("mysql_init err:%d\n",ret);
				return ret;
		}
		printf("connect ok\n");

		connect = mysql_real_connect(connect,"localhost","root","123","mydb",0,NULL,0);
		if(connect == NULL){
				ret = mysql_errno(&mysql);
				printf("mysql_init err:%d\n",ret);
				return ret;
		}
		printf("mysql_real_connect ok\n");
		
		char *sql = "select *from employ";
		ret = mysql_query(connect,sql);
		if(ret != 0){
				printf("mysql_query err\n");
				return ret;
		}
		printf("query ok\n");

		MYSQL_RES *result = mysql_store_result(connect);
		if(result == NULL){
				ret = mysql_errno(connect);
				printf("mysql_query err:%d\n",ret);
				return ret;
		}
		printf("store ok\n");
	
		/*  获取多少列
		    unsigned int mysql_field_count(MYSQL *mysql); 从句柄获取多少列
			unsigned int mysql_num_fields(MYSQL_RES *result);从结果集中获取多少列
		*/
		
		int num = mysql_field_count(connect);
		
		//表头
		MYSQL_FIELD  *fields;
		fields = mysql_fetch_fields(result);
		for(i=0; i<num; i++){
			printf("%s\t",fields[i].name);
		}
		printf("\n");
		
		MYSQL_ROW row = NULL;
		while(row = mysql_fetch_row(result))
		{	
			for(i=0; i<num; i++){
				printf("%s\t",row[i]);			
		}
			printf("\n");
		}
		mysql_free_result(result);

		mysql_close(&mysql);
		return 0;
}







