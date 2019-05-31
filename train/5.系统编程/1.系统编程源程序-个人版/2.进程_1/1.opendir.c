#include<unistd.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<fcntl.h>
#include<dirent.h>
#include<errno.h>
//1.opendir.c
#if 0
    DIR *opendir(const char *name);
    函数打开由name指定的目录，
    并返回指向DIR类型结构体的指针(目录流指针)，供后序调用使用
    成功返回目录流指针，失败返回空指针
    ---------------------------------------------------------------------------
    struct dirent *readdir(DIR *dirp);
    从一个目录流中读取连续的条目
    每调用一次，就会从dirp所指代的目录流中读取下一目录条目，
    并返回指针，指向经静态分配而得到的dirent结构类型，内含与该条目相关信息
    struct dirent {
	ino_t          d_ino;       /* inode number */
	off_t          d_off;       /* not an offset; see NOTES */
	unsigned short d_reclen;    /* length of this record */
	unsigned char  d_type;      /* type of file; not supported
				   by all filesystem types */
	char           d_name[256]; /* filename */
    };
    每次调用都会覆盖该结构
    ---------------------------------------------------------------------------
    调用stat()(或者lstat(),如果应对符号链接解引用时)
    可以得d_name所指向文件的更多信息
    其中，路径名
    由之前调用opendir()时指定的dirpath参数与"/"字符以及d_name字段的返回值拼接组成
    readdir()返回时并未对文件名进行排序，
    而是按照文件在目录中出现的天然次序(这取决于文件系统向目录添加文件时所遵循的次序
    及其在删除文件后对目录列表中空隙的填补方式)
    (命令ls-f对文件列表的排列与调用readdir()时一样，均未做排序处理)
    ---------------------------------------------------------------------------
    一旦遇到目录结尾或是出错，readdir()将返回NULL，指针后一种情况，
    还会设置errno表示具体错误。为了区别这两种情况，可编码如下：
    errno = 0;
    direntp = readdir(dirp);
    if(direntp == NULL){ 
	if(errno != NULL){
	/* Handle error */} 
	else{
	/* We reached end-of-directory */}
    }
    ---------------------------------------------------------------------------
    如果目录内容恰逢应用调用 readdir()扫描该目录时发生变化，
    那么应用程序可能无法观察到这些变动。SUSv3明确指出，对于readdir()
    是否会返回自上次调用opendir()或rewinddir()后在目录中增减的文件
    规范不做要求。至于最后一次执行上述调用前就存在的文件，应确保其全部返回
    closedir()函数将由dirp指代，处于打开状态的目录流关闭，同时释放流所使用的资源
    ---------------------------------------------------------------------------
    int closedir(DIR *dirp);
    关闭目录流指针
    返回值：成功返回0,错误返回-1
#endif
#if 0
    实例：
    mkdir sub
    touch sub/a sub/b
    ./a.out sub
    sub/a
    sub/b
#endif
int main(int argc,char *argv[]){
    struct dirent *dp=NULL;
    DIR *dirp=opendir(argv[1]);
    while(1){
	errno=0;
	dp=readdir(dirp);
	if(dp==NULL)
	    break;
	if(!strcmp(dp->d_name,".")||!strcmp(dp->d_name,".."))
	    continue;
	printf("%s/%s\n",argv[1],dp->d_name);
    }
    if(errno!=0)
	printf("error\n");
    if(closedir(dirp)==-1)
	printf("closedir error\n");
    return 0;
}
