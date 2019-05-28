#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

int main(int argc, char* argv[])
{
    if(argc < 2){
        printf("./a.out filename\n");
        exit(-1);
    }
    struct stat buf;

    int ret = stat(argv[1],&buf);
    if(ret == -1){
        perror("stat");
        exit(1);
    }
    //文件类型和访问权限
    char perms[11] = {0};
    //&上一个掩码,它的值是017000,可以用来过滤出前四位表示的文件类型
    switch(buf.st_mode & S_IFMT)
    {
        case S_IFLNK:
            perms[0] = 'l';
            break;
        case S_IFDIR:
            perms[0] = 'd';
            break;
        case S_IFREG:
            perms[0] = '-';
            break;
        case S_IFBLK:
            perms[0] = 'b';
            break;
        case S_IFCHR:
            perms[0] = 'c';
            break;
        case S_IFSOCK:
            perms[0] = 's';
            break;
        case S_IFIFO:
            perms[0] = 'p';
            break;
        default:
            perms[0] = '?';
            break;                     
    }
    //判断文件的权限
    //文件的所有者
	buf.st_mode |= S_IROTH;
	buf.st_mode |= S_IWOTH;

    chmod(argv[1],buf.st_mode);

	perms[1] = (buf.st_mode & S_IRUSR) ? 'r' : '-';
    perms[2] = (buf.st_mode & S_IWUSR) ? 'w' : '-';
    perms[3] = (buf.st_mode & S_IXUSR) ? 'x' : '-';
    //文件所属组
    perms[4] = (buf.st_mode & S_IRGRP) ? 'r' : '-';
    perms[5] = (buf.st_mode & S_IWGRP) ? 'w' : '-';
    perms[6] = (buf.st_mode & S_IXGRP) ? 'x' : '-';
    //其他人
    perms[7] = (buf.st_mode & S_IROTH) ? 'r' : '-';
    perms[8] = (buf.st_mode & S_IWOTH) ? 'w' : '-';
    perms[9] = (buf.st_mode & S_IXOTH) ? 'x' : '-';
	

    //硬链接计数
    int linkNum = buf.st_nlink;
    //文件所有者
    char* fileUser = getpwuid(buf.st_uid)->pw_name;
    //文件所属组
    char* fileGrp = getgrgid(buf.st_gid)->gr_name;
    //文件大小
    int fileSize = (int)buf.st_size;
    //修改时间
    char* time = ctime(&buf.st_mtime);
    char mtime[512] = {0};
    strncpy(mtime,time,strlen(time)-1);

    char buf1[1024];
    sprintf(buf1,"%s %d %s %s %d %s %s",perms,linkNum,fileUser,fileGrp,fileSize,mtime,argv[1]);

    printf("%s\n",buf1);

    return 0;
}










