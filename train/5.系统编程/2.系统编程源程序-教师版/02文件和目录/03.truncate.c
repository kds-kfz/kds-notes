#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>

int main()
{
	//文件截断
	int ret = truncate("access",5);
	if(ret == -1){
		printf("access error\n");
	}
	return 0;
}

stat获取文件信息  struct stat buf;

chmod 更改文件权限
chown

link创建连接(硬链接)
unlink删除链接
symlink

//目录操作
opendir
readdir
chdir

//复制文件描述符
dup



