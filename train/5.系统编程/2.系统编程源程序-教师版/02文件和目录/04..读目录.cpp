opendir()函数打开由dirpath指定的目录, 并返回指向DIR类型结构的指针 (目录流指针), 供后续调用使用.
	┌───────────────────────────────────────────────────────────────┐
	│#include <dirent.h>                                            │
	│                                                               │
	│DIR* opendir(const char* dirpath)                              │
	│                                                               │
	│              Returns directory stream handle, or NULL on error│
	└───────────────────────────────────────────────────────────────┘

	readdir()函数从一个目录流中读取连续的条目。
	┌───────────────────────────────────────────────────────────────┐
	│#include <dirent.h>                                            │
	│                                                               │
	│DIR* readdir(DIR* dirp)                                        │
	│                                                               │
	│ Returns pointer to a statically allocated structure describing│
	│     next directory entry, or NULL on end-of-directory or error│
	└───────────────────────────────────────────────────────────────┘
	每调用 readdir() 一次，就会从dirp所指代的目录流中读取下一目录条目，并返回一枚指针, 指向经静态
	分配而得的dirent类型结构，内含与该条目相关的如下信息：
	struct dirent {
		ino_t d_ino:      /* File i-node number */
		char d_name[];    /* Null-terminated name of file */
	};
	每次调用 readdir() 都会覆盖该结构。
	
	调用 stat() (或者 lstat()，如果应对符号链接解引用时）可获得d_name所指向文件的更多信息，其中，路径名
	由之前调用 opendir() 时指定的dirpath参数与"/"字符以及d_name字段的返回值拼接组成。

	readdir() 返回时并未对文件名进行排序，而是按照文件在目录中出现的天然次序（这取决于文件系统向目录添加文
	件时所遵循的次序，及其在删除文件后对目录列表中空隙的填补方式）。（命令ls-f对文件列表的排列与调用readdir()
	时一样，均未做排序处理。）


	一旦遇到目录结尾或是出错，readdir() 将返回NULL，针对后一种情况，还会设置errno以示具体错误。为了区别
	这两种情况，可编码如下：

	errno = 0;
	direntp = readdir(dirp); 
	if (direntp == NULL) { 
		if (errno != 〇) {
			/* Handle error */
	 	} else {
			/* We reached end-of-directory */
		}
	}

	如果目录内容恰逢应用调用 readdir()扫描该目录时发生变化，那么应用程序可能无法观察到这些变动。SUSv3明确指出，
	对于 readdir() 是否会返回自上次调用 opendir()或 rewinddir()后在目录中增减的文件，规范不做要求。至于最
	后一次执行上述调用前就存在的文件，应确保其全部返回。

	closedir()函数将由dirp指代、处于打开状态的目录流关闭，同时释放流所使用的资源。
	┌───────────────────────────────────────────────────────────────┐
	│#include <dirent.h>                                            │
	│                                                               │
	│int closedir(DIR* dirp)                                        │
	│                                                               │
	│                         Returns 0 on success, or -1 on error│
	└───────────────────────────────────────────────────────────────┘


实例：
$ mkdir sub
$ touch sub/a sub/b
$ ./a.out sub
  sub/a
  sub/b


代码：
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

int main(int argc, char* argv[])
{
    struct dirent* dp = NULL;

	DIR* dirp = opendir(argv[1]);

    while (1) {
        errno = 0;
        dp = readdir(dirp);
        if (dp == NULL) {
            break;
        }
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }

        printf("%s/%s\n", argv[1], dp->d_name);
    }
    
    if (errno != 0) {
        printf("error\n");
    }
    if (closedir(dirp) == -1) {
        printf("closedir error\n");
    }
}
-----------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>

int getFileNum(char* root)
{
	//open dir
	DIR* dir = NULL;
	dir = opendir(root);
	if(dir == NULL){
		perror("opendir");
		exit(1);
	}
	//遍历当前的目录
	struct dirent* ptr = NULL;
	char path[1024] = {0};
	int total = 0;
	while((ptr = readdir(dir)) != NULL){
		//过滤掉..和.
		if(strcmp(ptr->d_name,".")==0 || strcmp(ptr->d_name,"..")==0){
			continue;
		}
		//如果是目录
		if(ptr->d_type == DT_DIR){
			//递归读目录
			sprintf(path,"%s/%s",root,ptr->name);
			total += getFileNum(path);
		}
		//如果是普通文件
		if(ptr->d_type == DT_REG){
			total++;
		}
	}
	closedir(dir);
	return total;
}

int main(int argc, char* argv[])
{
	if(argc < 2){
		printf("less 2");
		exit(-1);
	}
	int total = getFileNum(argv[1]);
	printf("filenum is %d\n",total);

	return 0;
}












