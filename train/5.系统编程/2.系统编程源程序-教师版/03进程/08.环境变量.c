/*
环境变量是指操作系统中用来指定操作系统运行环境的一些参数:
具有以下特征,
1.字符串
2.有统一的格式name=value[:]
3.value用来描述进程环境信息

存储形式:与命令行参数类似,char*[]数组,数组名environ,内部存储字符串,
		NULL作为结束标志
使用形式:与命令行参数类似
加载位置:与命令行参数类似　位于用户区,在栈区上
引入环境变量表,须声明环境变量,extern char** environ
*/

//打印当前进程所有环境变量

#include <stdio.h>

extern char** environ;

int main(int argc, char* argv[])
{
	for(int i=0; environ[i] != NULL; i++){
		printf("%s\n",environ[i]);
	}

	return 0;
}

