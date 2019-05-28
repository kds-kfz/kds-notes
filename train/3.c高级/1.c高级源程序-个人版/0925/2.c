#include<stdio.h>
//2.c
//文件操作，学习3

//int fputs(const char *s, FILE *stream);
//从s地址开始写入到\0结束

//char *fgets(char *s, int size, FILE *stream);
//s指向的地址必须可改（字符数组）
//size读取的字符个数，10,读取9个字符，自带1个\0
//遇见\n停止接收，接收回车并再加\0
//返回值是s首地址

int main(){
    char *p="abcdefg,hijk";
    char a[20]="adcd\naf";
    FILE *fp=fopen("3.txt","w+");
    fputs(a,fp);

    rewind(fp);//移动文件指针到开头
    p=a;
    fgets(p,4,fp);
    long int len=ftell(fp);
    printf("len=%ld\n",len);

    printf("p=%s\n",p);
    printf("a[3]=%d\n",a[3]);
    return 0;
}
