#include<stdio.h>
#include<stdlib.h>
#include<mysql/mysql.h>
//2.mysql_real_connect.c
#if 0
    MYSQL *STDCALL mysql_real_connect(
	MYSQL *mysql, const char *host,const char *user,
	    const char *passwd,const char *db,unsigned int port,
		const char *unix_socket,unsigned long clientflag);
    mysql：mysql_init返回的指针
    ----------------------------------------------------------------------------
    host：的值必须是主机名或IP地址。如果“host”是NULL或字符串"localhost"，
    连接将被视为与本地主机的连接。如果操作系统支持套接字(Unix)或
    命名管道(Windows)，将使用它们而不是TCP/IP连接到服务器。
    ----------------------------------------------------------------------------
    user：参数包含用户的MySQL登录ID。如果“user”是NULL或空字符串""，
    用户将被视为当前用户。在UNIX环境下，它是当前的登录名。在Windows ODBC下，
    必须明确指定当前用户名。请参见26.1.9.2节， “在Windows上配置MyODBC DSN”。
    ----------------------------------------------------------------------------
    passwd：参数包含用户的密码。 如果“passwd”是NULL， 
    仅会对该用户的(拥有1个空密码字段的)用户表中的条目进行匹配检查。
    这样，数据库管理员就能按特定的方式设置MySQL权限系统，
    根据用户是否拥有指定的密码，用户将获得不同的权限。
    注释：调用mysql_real_connect()之前，不要尝试加密密码，
    密码加密将由客户端API自动处理
    ----------------------------------------------------------------------------
    db：是数据库名称。如果db为NULL， 连接会将默认的数据库设为该值。
    如果port不是0， 其值将用作TCP/IP连接的端口号。
    注意，host参数决定了连接的类型。
    如果unix_socket不是NULL，该字符串描述了应使用的套接字或命名管道。
    注意，host参数决定了连接的类型。
    ----------------------------------------------------------------------------
    client_flag：值通常为0，但是，也能将其设置为下述标志的组合，以允许特定功能
    ----------------------------------------------------------------------------
    返回值：
    当链接成功时，返回MYSQL连接句柄(此时与该函数第一个参数地址相同)，失败返回NULL
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
    mysql_close(connect);
    return 0;
}
