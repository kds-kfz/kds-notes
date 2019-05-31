#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
//6.环境变量.c
#if 0
    环境变量是指操作系统中用来指定操作系统运行环境的一些参数
    具有一以下特征
    1.字符串
    2.有统一的格式name=value,多形式以:分隔
    /usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin
    3.value用来描述进程环境信息
    -----------------------------------------------------------------------------
    存储形式：与命令行参数类似，char *[]数组，
    数组名environ，内部存储字符串，NULL作为结束标志
    -----------------------------------------------------------------------------
    使用形式：与命令行参数类似
    加载位置：与命令行参数类似，位于用户区，在栈区
    引用环境变量表，需要声明环境变量，extern char **environ
#endif

//打印当前进程所有环境变量
extern char **environ;
int main(){
    for(int i=0;environ[i]!=NULL;i++)
	printf("%s\n",environ[i]);
    return 0;
}

