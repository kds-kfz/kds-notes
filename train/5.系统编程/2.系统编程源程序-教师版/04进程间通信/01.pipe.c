/*  
 管道:
	1.其本质是一个伪文件
	2.由两个文件描述符引用,一个表示读端(fd[0]),一个表示写端(fd[1])
	3.规定数据从管道的写端流入,从读端流出
注意:
	1.数据自己读不能自己写
	2.数据一旦被读走,便在管道中不存在
	　不可反复读取
	3.数据只能在一个方向上流动
	4.有血缘关系的进程间通信	
 */

//pipe由称为无名管道
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main()
{
	pid_t pid;
	int fd[2];/*fd[0]读 fd[1]写 */
	int ret;
	char buf1[20] = "hello world\n";
	char buf2[20];

	ret = pipe(fd);
	if(ret == -1){
		printf("pipe error\n");
	}
	
	pid = fork();
	if(pid == -1){
	
	}else if(pid > 0){
		//父进程写数据
		close(fd[0]);
		ret = write(fd[1],buf1,strlen(buf1));
		if(ret == -1){
		
		}
		close(fd[1]);
	}else{
		sleep(5);
		//
		close(fd[1]);
		ret = read(fd[0],buf2,sizeof(buf2)-1);
		if(ret == -1){
		
		}
		buf2[ret] = '\0';
		ret = write(1,buf2,ret);
		if(ret == -1){
		
		}
		close(fd[0]);
	}
	
	return 0;
}

