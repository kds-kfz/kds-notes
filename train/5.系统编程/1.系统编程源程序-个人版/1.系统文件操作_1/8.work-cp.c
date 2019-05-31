#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/stat.h>
//8.work-cp.c
//练习：实现cp指令
int main(int argc,char *argv[]){
    if(argc!=3){
	fprintf(stdout,"参数错误\n");
	exit(-1);
    }
    int fd=open(argv[1],O_RDONLY);
    if(fd==-1){
	fprintf(stdout,"open file1 fail\n");
	exit(-1);
    }
    if(strcmp(argv[1],argv[2])==0){
	fprintf(stdout,"file1 and file2 alinke\n");
	exit(-1);
    }
    char buf[30];
    int fd2=open(argv[2],O_WRONLY|O_TRUNC);
    if(fd2==-1){
	fprintf(stdout,"open file2 fail\n");
	exit(-1);
    }
    /*
    int fd3=open(argv[1],O_RDONLY);
    if(fd3==-1){
	fprintf(stdout,"open file1 fail\n");
	exit(-1);
    }
    */
    int count=0;
    int ret;
    char ch;
    #if 0
    //方法一，思路：每次读n个字节
    while(1){
	ret=read(fd,buf,30);
	if(ret==-1){
	    fprintf(stdout,"read file1 fail\n");
	    exit(-1);
	}
	ret=write(fd2,buf,ret);
	count++;//统计写入次数
	if(ret==-1){
	    fprintf(stdout,"file1 write to file2 fail\n");
	    exit(-1);
	}
	if(ret==0){//如果最后依次读取是0,即读取失败，跳出循环
	    //例如：文件1有80字节，每次读30个字节，
	    //三次可以读完，第三次读取类20个字节，第4次读取时，
	    //读取失败，返回数取个数是0
	    fprintf(stdout,"ret = %d\n",ret);
	    fprintf(stdout,"count = %d\n",count);
	    break;
	}
    }
    #endif
    #if 1
    //方法二，思路：每次读1个字节，以下列出3种情况

    //    lseek(fd3,0,SEEK_END);
    //试图打开同一个文件，将文件描述符fd3的文件偏移量移到文件末尾，
    //通过判断文件描述符fd与fd3文件偏移量相等，则读取文件到末尾，
    //但这里两个文件描述符是不会相等的
    while(1){
	ret=read(fd,&ch,1);
	if(ret==-1){
	    fprintf(stdout,"read file1 fail\n");
	    exit(-1);
	}
	ret=write(fd2,&ch,ret);
	count++;
	if(ret==-1){
	    fprintf(stdout,"file1 write to file2 fail\n");
	    exit(-1);
	}
	#if 0
	//情况1：
	//试图判断文件指针相等，
	//即读到文件末尾，但这里是文件文件描述符，该方法不可行
	if(fd==fd3){//文件描述符不会等
	    fprintf(stdout,"ret = %d\n",ret);
	    fprintf(stdout,"count = %d\n",count);
	    break;
	}
	#endif
	#if 1
	//情况2：
	//通过判断读回来的字节可知道是否读取成功，
	//按1个字节读取，成功返回1,即字节个数，失败返回0,即没有读取成功	
	if(ret==0){//这里也可写作：ret!=1
	    fprintf(stdout,"ret = %d\n",ret);
	    fprintf(stdout,"count = %d\n",count);
	    break;
	}
	#endif
	#if 0
	//情况3：
	//试图判断是否能读到文件末尾，即读到文件描述符EOF
	//单这里是操作文件描述符，读取成功返回读取个数，失败返回读取个数是0
	if(ch==EOF){//不会读到EOF，该方法不行
	    fprintf(stdout,"ret = %d\n",ret);
	    fprintf(stdout,"count = %d\n",count);
	    break;
	}
	#endif
    }
    close(fd);
    close(fd2);
//    close(fd3);
    return 0;
}
