#include<sys/stat.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<pwd.h>
#include<grp.h>
#include<time.h>
#if 0
    使用stat函数最多的地方可能是ls -l命令，
    用其他可以获得有关一个文件的所有信息
    利用系统调用stat(),可获取与文件有关的信息，其中大部分提取自文件i节点
    #include<sys/stat.h>
    int stat(const char *pathname, struct stat *buf);
    //参数1：文件路径，参数2：struct stat类型的结构体
    返回值：成功反壶0,出错返回-1
    -----------------------------------------------------------------------------
    struct stat {
    dev_t     st_dev;         // ID of device containing file 
    ino_t     st_ino;         // inode number
    mode_t    st_mode;        // protection
    nlink_t   st_nlink;       // number of hard links
    uid_t     st_uid;         // user ID of owner
    gid_t     st_gid;         // group ID of owner
    dev_t     st_rdev;        // device ID (if special file)
    off_t     st_size;        // total size, in bytes
    blksize_t st_blksize;     // blocksize for filesystem I/O
    blkcnt_t  st_blocks;      // number of 512B blocks allocated

    struct timespec st_atim;  // time of last access 
    struct timespec st_mtim;  // time of last modification 
    struct timespec st_ctim;  // time of last status change 
    
    #define st_atime st_atim.tv_sec      // Backward compatibility 
    #define st_mtime st_mtim.tv_sec
    #define st_ctime st_ctim.tv_sec
   };
    -----------------------------------------------------------------------------
    st_dev：标识文件所驻留的设备
    st_ino：包含了文件的i节点号
    st_rdev：如果是指针对设备的i节点，那么st_rdev字段则包含设备的主，副id
    st_uid：标识文件的属主(用户id)
    st_gid：标识文件的属组(组id)
    st_nlink：包含了指向文件的(硬)链接数
    st_mode：内含有位掩码，起标识文件类型和指定文件权限的双重作用，如下
    -----------------------------------------------------------------------------
    文件类型|		   权限
    0 0 0 0 | u g t | r w x | r w x | r w x
		       当前    分组    其它
    为该字段所含各位的布局情况。st_mode字段的低12位定义了文件权限，目前，只要知道其
    中最低9位分别用来表示文件属主，属组以及其他用户的读，写，执行权限
    -----------------------------------------------------------------------------
    针对stat结构中的st_mode来检查文件类型的宏
      常量	 测试宏		文件类型	
    S_IFREG	S_ISREG()	常规文件	1
    S_IFDIR	S_ISDIR()	  目录		2
    S_IFCHR	S_ISCHR()	字符设备	3
    S_IFBLK	S_ISBLK()	块设备		4
    S_IFIFO	S_ISIFO()	FIFO或管道	5
    S_IFLNK	S_ISLNK()	符号链接	6
    S_IFSOCK	S_ISSOCK()	套接字		7
    -----------------------------------------------------------------------------
    如下所示：07000000...
    if((buf.st_mode & S_IFMT) == S_IFREG)
	printf("regular file\n");
    if(S_ISREG(buf.st_mode))
	printf("regular file\n");
    -----------------------------------------------------------------------------
    st_size：
    表示文件的字节数。对于符号链接，则表示链接所指路径名的长度，以字节为单位
    对于共享内存对象，该字段则表示对象的大小
    ------------------------------------------------------------------------------
    st_blocks：
    字段表示分配给文件的总块数，块大小为512字节，其中包括了为指针块所分配的空间
    之所以选择512字节大小的块作为度量单位，有其历史原因——对于UNIX所实现的任何文
    件系统而言，最小的块大小即为512字节。更为现代的UNIX文件系统 则使用更大尺寸
    的逻辑块。例如，对于ext2文件系统，取决于其逻辑块大小为1024，2048还是4096字节
    st_blocks的取值将总是2，4，8的倍数
    st_blocks字段记录了实际分配给文件的磁盘块数量。如果文件内含空洞，该值将小于
    从相应文件字节数字段(st_size)的值
    -----------------------------------------------------------------------------
    st_blksize：
    命名多少有些令人费解。其所指并非底层文件系统的块大小，而是针对文件系统上文件
    进行I/O操作时的最优块大小(以字节为单位)。若I/O所采用的块大小小于该值，则被视
    为低效。一般而言，st_blksize的返回值为4096
    -----------------------------------------------------------------------------
    st_atime：
    st_mtime：
    st_ctime：
    这三个字段分别记录了对文件的上次访问时间，上次修改时间，以及文件状态发生改变的
    上次时间。这3个字段的类型均属，是标准的UNIX时间格式，记录了自Epoch以来的秒数
    -----------------------------------------------------------------------------
#endif
int main(int argc,char *argv[]){
    if(argc!=2){
	fprintf(stdout,"参数错误\n");
    }
    struct stat buf;
    int ret=stat(argv[1],&buf);
    if(ret==-1){
	fprintf(stdout,"error\n");
	exit(-1);
    }
    //文件类型和访问权限
    char type[11];
    //&上一个掩码,它的值是017000,可以用来过滤出前四位表示的文件类型
    switch(buf.st_mode & S_IFMT){
	case S_IFLNK:
	    type[0]='l';//链接文件
	    break;
	case S_IFDIR:
	    type[0]='d';//目录文件
	    break;
	case S_IFREG:
	    type[0]='-';//普通文件
	    break;
	case S_IFBLK:
	    type[0]='b';//块设备文件
	    break;
	case S_IFCHR:
	    type[0]='c';//字符设备文件
	    break;
	case S_IFSOCK:
	    type[0]='s';//套接字文件
	    break;
	case S_IFIFO:
	    type[0]='p';//管道文件
	    break;
	default:
	    type[0]='?';
	    break;
    }
    //当前用户
    type[1] = (buf.st_mode & S_IRUSR) ? 'r' : '-';
    type[2] = (buf.st_mode & S_IWUSR) ? 'w' : '-';
    type[3] = (buf.st_mode & S_IXUSR) ? 'x' : '-';
    //分组用户
    type[4] = (buf.st_mode & S_IRGRP) ? 'r' : '-';
    type[5] = (buf.st_mode & S_IWGRP) ? 'w' : '-';
    type[6] = (buf.st_mode & S_IXGRP) ? 'x' : '-';
    //其它用户
    type[7] = (buf.st_mode & S_IROTH) ? 'r' : '-';
    type[8] = (buf.st_mode & S_IWOTH) ? 'w' : '-';
    type[9] = (buf.st_mode & S_IXOTH) ? 'x' : '-';

    type[10]='\0';
//    fprintf(stdout,"%s\n",type);
    //硬链接计数
    int link_num=buf.st_nlink;
    //文件所有者
    char *file_user=getpwuid(buf.st_uid)->pw_name;
    //文件所属者
    char *file_grup=getgrgid(buf.st_gid)->gr_name;
    //文件大小
    int file_size=(int)buf.st_size;
    //修改时间
    char *time=ctime(&buf.st_mtime);
    char mtime[512]={0};
    strncpy(mtime,time,strlen(time)-1);

    char buf1[1024];
    sprintf(buf1,"%s %d %s %s %d %s %s",type,link_num,file_user,file_grup,file_size,mtime,argv[1]);
    fprintf(stdout,"%s\n",buf1);
    
    //修改权限
    buf.st_mode&=S_IXUSR;
    chmod(argv[1],buf.st_mode);

    return 0;
}

