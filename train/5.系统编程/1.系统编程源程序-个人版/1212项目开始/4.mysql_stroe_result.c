#include<stdio.h>
#include<stdlib.h>
#include<mysql/mysql.h>
//4.mysql_store_result.c
#if 0
    MYSQL_RES *STDCALL mysql_store_result(MYSQL *mysql);
    mysql：
    返回值：
    ************************************************************
    MYSQL_ROW STDCALL mysql_fetch_row(MYSQL_RES *result);
    result
    返回值：
    ************************************************************
    unsigned int STDCALL mysql_num_fields(MYSQL_RES *res);
    res
    返回值：
    ************************************************************
    MYSQL_ROW STDCALL mysql_fetch_row(MYSQL_RES *result);
    result
    返回值：
    ************************************************************
    MYSQL_FIELD * STDCALL mysql_fetch_fields(MYSQL_RES *res);
    res
    返回值：
    ************************************************************
    void STDCALL mysql_free_result(MYSQL_RES *result);
    result
    ************************************************************

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
    
    char *sql1="insert into s1(id,name,sex,chinese,english,math) values(2008,\'lisi\',\'y\',50,60,70)";
    char *sql2="insert into s1(id,name,sex,chinese,english,math) values(2009,\"lisi\",\"y\",50,60,70)";
    char *sql3="insert into s1(id,name,sex,chinese,english,math) values(2009,'lisi','y',50,60,70)";
    char *sql4="delete from s1 where name='lisi'";

    ret=mysql_query(connect,sql2);
    if(ret!=0){
	printf("mysql_query error : %d\n",ret);
	exit(-1);
    }
    printf("insert ok\n");

    char *look="select *from s1";
    ret=mysql_query(connect,look);
    if(ret!=0){
	printf("mysql_query error : %d\n",ret);
	exit(-1);
    }
    printf("select ok\n");

    //获取结果集
    MYSQL_RES *result=mysql_store_result(connect);
    
    //从结果集获取列
    int col=mysql_num_fields(result);
    
    //定义获取表头变量
    MYSQL_FIELD *fields;
    fields=mysql_fetch_fields(result);
    
    //打印表头条目
    for(int i=0;i<col;i++)
	printf("%s\t",fields[i].name);
    putchar(10);

    //从结构集获取行
    //二级指针//typedef char **MYSQL_ROW;
    MYSQL_ROW row;
    while(row=mysql_fetch_row(result)){
	for(int i=0;i<col;i++){
	    printf("%s\t",row[i]);
	}
    putchar(10);
    }
    //释放结果集内存
    mysql_free_result(result);
    //释放数据库
    mysql_close(connect);
    return 0;
}

