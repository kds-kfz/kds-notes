一个进程的当前工作目录 (current working directory) 定义了该进程解析相对路径名的起点。
新进程的当前工作目录继承自其父进程。
	进程可使用 getcwd() 来获取当前工作目录。
	┌───────────────────────────────────────────────────────────────┐
	│#include <unistd.h>                                            │
	│                                                               │
	│char* getcwd(char* cwdbuf, size_t size)                        │
	│                                                               │
	│                    Returns cwdbuf on success, or NULL on error│
	└───────────────────────────────────────────────────────────────┘
	(1) getcwd()函数将内含当前工作目录绝对路径的字符串（包括结尾空字符）置于cwdbuf指向的已分配缓冲区中。
	    调用者必须为cwdbuf缓冲区分配至少size个字节的空间。（通常，cwdbuf的大小与PATH_MAX常量相当。）

	(2) 一旦调用成功，getcwd()将返回一枚指向cwdbuf的指针。如果当前工作目录的路径名长度超过size个字节，
	    那么 getcwd()会返回NULL, 并将ermo置为ERANGE。

	(3) 在Linux/x86-32系统中，getcwd() 返回指针所指向的字符串最大长度可达4096个字节。 如果当前工作目
	    录(以及cwdbuf和size)突破了这一限制，那么就会直接对路径名做截断处理，移去始于起点的整个目录前缀
	    (字符串仍以空字符结尾）。换言之，当当前工作目录的绝对路径超出这一限制时，getcwd()的行为也不再可靠。

	(4) [1] 若cwdbuf为NULL,且size为0，则 glibc 封装函数会为 getcwd()按需分配一个缓冲区， 并将指向
	        该缓冲区的指针作为函数的返回值。为避免内存泄漏，调用者之后必须调用 free() 来释放这一缓冲区。
	        对可移植性有所要求的应用程序应当避免依赖该特性。大多数其他实现则针 对SUSv3规范提供了一个更为
	        简单的扩展。
	    
	    [2] 如果cwdbuf是NULL,且size非0, 那么getcwd()将分配一个大小为size字节的缓冲区，用于向调用者
	        返回结果。glibc 的 getcwd()也实现了这一特性。


	chdir() 系统调用将调用进程的当前工作目录改变为由pathname指定的相对或绝对路径名 (如属于符号链接，
	还会对其解除引用）。
	┌───────────────────────────────────────────────────────────────┐
	│#include <unistd.h>                                            │
	│                                                               │
	│int chdir(const char* pathname)                                │
	│                                                               │
	│                           Returns 0 on success, or -1 on error│
	└───────────────────────────────────────────────────────────────┘

	fchdir()系统调用 与 chdir() 作用相同，只是在指定目录时使用了文件描述符，而该描述符是之前调用open()
	打开相应目录时获得的。
	┌───────────────────────────────────────────────────────────────┐
	│#define _XOPEN_SOURCE 500                                      │
	│#include <unistd.h>                                            │
	│                                                               │
	│int fchdir(int fd)                                             │
	│                                                               │
	│                           Returns 0 on success, or -1 on error│
	└───────────────────────────────────────────────────────────────┘

实例：
#define _XOPEN_SOURCE 600
#include <stdio.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
    char buf[1024] = {'\0'};
    getcwd(buf, 1024);
    printf("buf = %s\n", buf);

    chdir("/home/zm");
    char nbf[1024] = {'\0'};
    getcwd(nbf, 1024);
    printf("nbf = %s\n", nbf);

    chdir(buf);
}

-------------------------------------------------------
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	if(argc < 2){
		printf("less than 2\n");
		exit(-1);
	}
	int ret = chdir(argv[1]);
	if(ret == -1){
		perror("chdir");
		exit(-1);
	}
	int fd = open("chdir.txt",O_CREAT|O_RDWR,0777);
	if(fd == -1){
		perror("open");
		exit(-1);
	}
	close(fd);
	char buf[128];
	getcwd(buf,sizeof(buf));
	printf("current dir: %s\n",buf);
	
	return 0;
}