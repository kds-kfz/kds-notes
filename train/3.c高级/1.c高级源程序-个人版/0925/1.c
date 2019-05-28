#include<stdio.h>
//1.c
//文件操作，学习2
/*
int fprintf(FILE *stream, const char *format, ...);
int fclose(FILE *stream);
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
void rewind(FILE *stream);
int getc(FILE *stream);
int fputc(int c, FILE *stream);
*/
int main(){
    char ch1[20]="123456";
    char ch2[20]="0";
    
    FILE *fp=fopen("1.txt","w");//创一个1.txt文件,以写形式打开
//    FILE *fp=fopen("1.txt","w+");//创一个1.txt文件，移写读形式打开
    if(fp!=NULL)
	printf("open success\n");
    int i=0;
    char c='0';
    fprintf(fp,"%s",ch1);
//    for(;ch1[i]!='\0';i++)//向1.txt文件写入ch1字符串
//	fputc(ch1[i],fp);
    fclose(fp);
    fp=fopen("1.txt","r");
//    fseek(fp,0,SEEK_SET);
//使用此句注意文件指针打开形式，
//w+：写读打开，移动文件指针后可以使用fgetc读文件字符
//w：读打开，即使移动指针到开头，fgetc读出的是EOF，依旧是文件末尾
    
//    long int len=ftell(fp);//计算文件指针当前位置到文件末尾字节大小
//    printf("len=%ld\n",len);
    
    //将1.txt文件内容复制到2.txt
    FILE *fr=fopen("2.txt","w");//创一个2.txt文件
    if(fr!=NULL)
	printf("create 2.txt success\n");
/*
    for(i=0;;i++){//方法1
	c=fgetc(fp);//每次进入循环，接收文件字符
	if(c!=EOF)//如果字符不是文件末尾
	    fputc(c,fr);//将接收的字符打印到fr，实现文件间复制
	else
	    break;
    }
    */
    while((c=fgetc(fp))!=EOF)//方法2
	fputc(c,fr);
/*
    while(!feof(fp)){//禁用,最后一位是乱码
	c=fgetc(fp);
	fputc(c,fr);
    }
*/
    fclose(fp);
    fclose(fr);
    return 0;
}
