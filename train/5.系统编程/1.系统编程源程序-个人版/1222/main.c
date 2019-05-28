#include<stdio.h>
#include"Mysql.h"
#include"function.h"
#include<string.h>
//数据库封装测试
int main(){
    /*
    Custom per={"1001","lisi","key123","24","男","12345678901","深大","100","client"};
    Custom per2={"1002","lisi","key123","24","男","12345678901","深大","100","client"};
    Custom per3={"1003","lisi","key123","24","男","12345678901","深大","100","client"};
    Custom per4={"1004","lisi","key123","24","男","12345678901","深大","100","client"};
    Custom *p=&per;
    Custom *p2=&per2;
    */
    Mysql_t *mysql=(Mysql_t *)malloc(sizeof(Mysql_t));
    Mysql_init(mysql);
    Mysql_connect(mysql,"project");//链接已创建好的数据库

    //表格创建1次即可
//    create_all_table(mysql);//创建所有所有表格
    
//    insert_person(mysql,p2);//向person表格插入数据
//    char ret=check_person(mysql,p2);
//    printf("ret= %c\n",ret);
    /*
    char *ret=test_new_custom(mysql,p2);
    if(!strcmp(ret,"#1"))
	printf("ok\n");
    else if(!strcmp(ret,"#2"))
	printf("fail\n");

//    Mysql_show_data(mysql,"select *from person");
*/
    char *sql="select *from person where number='1001'";
    Mysql_query(mysql,sql);
    mysql->result=mysql_store_result(mysql->connect);
    mysql->col=mysql_num_fields(mysql->result);
    mysql->row=mysql_fetch_row(mysql->result);
    for(int i=0;i<mysql->col;i++)
    printf("%s\t",mysql->row[i]);
    My_free(mysql);
    return 0;
}
