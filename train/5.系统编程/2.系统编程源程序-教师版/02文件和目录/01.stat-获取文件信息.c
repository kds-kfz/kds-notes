使用stat函数最多的地方可能就是ls -l 命令,用其可以获得有关一个文件的所有信息

利用系统调用stat(),可获取与文件有关的信息,其中大部分提取自文件i节点
#include <sys/stat.h>
int stat(const char* pathname, struct stat* buf)
返回值:
	若成功,返回0;若出错,返回-1

1.stat()会返回所命名文件的相关信息
struct stat {
	dev_t     st_dev;     /* IDs of device on which file resides */
	ino_t     st_ino;     /* I-node number of file */
	mode_t    st_mode;    /* File type and permissions */
	nlink_t   st_nlink;   /* Number o千(hard) links to file */
	uid_t     st_uid;     /* User ID of file owner */
	gid_t     st_gid;     /* Group ID of file owner */
	dev_t     st_rdev;    /* IDs for device special files */
	off_t     st_size;    /* Total file size (bytes) */
	blksize_t st_blksize; /* Optimal block size for I/O (bytes) */
	blkcnt_t  st_blocks;  /* Number of (512B) blocks allocated */
	time_t    st_atime;   /* Time of last file access */
	time_t    st_mtime;   /* Time of last file modification */
	time_t    st_ctime;   /* Time of last status change */”
};

st_dev:标识文件所驻留的设备
st_ino:包含了文件的i节点号
st_rdev:如果是针对设备的i节点,那么st_rdev字段则包含设备的主、辅ID
st_uid:标识文件的属主(用户ID)
st_gid:标识文件的属组(组ID)
st_nlink:包含了指向文件的(硬)链接数
st_mode:内含有位掩码,起标识文件类型和指定文件权限的双重作用，如下
			|────文件类型────|────────────────────权限───────────────────────|
			┌───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┬───┐
			│   │   │   │   │ U │ G │ T │ R │ W │ X │ R │ W │ X │ R │ W │ X │
			└───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┴───┘
			                                 用户          组         其他
		   为该字段所含各位的布局情况。st_mode字段的低12位定义了文件权限，目前，只要知道其中
		   最低9位分别用来表示文件属主、属组以及其他用户的读、写、执行权限。

		   针对stat结构中的st_mode来检查文件类型的宏
			┌────────┬──────────┬─────────┐
			│   常量     测试宏     文件类型  
			├────────┼──────────┼─────────┤
			│S_IFREG │S_ISREG() │ 常规文件 
			├────────┼──────────┼─────────┤
			│S_IFDIR │S_ISDIR() │   目录 
			├────────┼──────────┼─────────┤
			│S_IFCHR │S_ISCHR() │ 字符设备 
			├────────┼──────────┼─────────┤
			│S_IFBLK │S_ISBLK() │  块设备 
			├────────┼──────────┼─────────┤
			│S_IFIFO │S_ISIFO() │FIFO或管道 
			├────────┼──────────┼─────────┤
			│S_IFSOCK│S_ISSOCK()│  套接字 
			├────────┼──────────┼─────────┤
			│S_IFLNK │S_ISLNK() │ 符号链接 
			└────────┴──────────┴─────────┘
                       111  
			如下所示：07000000..
			if((buf.st_mode & S_IFMT) == S_IFREG){
				printf("regular file\n");
			}
			if(S_ISREG(buf.st_mode)){
				printf("regular file\n");
			}
st_size   : 表示文件的字节数。对于符号链接，则表示链接所指路径名的长度，以字节为单位。
		   对于共享内存对象，该字段则表示对象的大小。 
st_blocks: 字段表示分配给文件的总块数，块大小为512字节，其中包括了为指针块所分配的空间。
		   之所以选择512字节大小的块作为度量单位，有其历史原因——对于UNIX所实现的任何
		   文件系统而言，最小的块大小即为512字节。更为现代的UNIX文件系统 则使用更大尺
		   寸的逻辑块。例如，对于ext2文件系统，取决于其逻辑块大小为1024、2048还
		   是4096字节，st_blocks的取值将总是2、4、8的倍数。
		   st_blocks字段记录了实际分配给文件的磁盘块数量。如果文件内含空洞，该值将小于
		   从相应文件字节数字段 (st_size) 的值。

st_blksize: 命名多少有些令人费解。其所指并非底层文件系统的块大小，而是针对文件系统上文件
			进行I/O操作时的最优块大小（以字节为单位）。若I/O所采用的块大小小于该值，
			则被视为低效。一般而言，st_blksize的返回值为4096。 



st_atime  :
st_mtime  :
st_ctime  : 这三个字段分别记录了对文件的上次访问时间、上次修改时间， 以及文件状态发生改变
			的上次时间。这3个字段的类型均属，是标准的UNIX时间格式， 记录了自Epoch以来的秒数。
