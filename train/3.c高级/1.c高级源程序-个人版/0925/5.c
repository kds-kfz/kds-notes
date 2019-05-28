#include<stdio.h>
//5.c
//文件操作，学习5
//文件操作学习结束

//读取空文件，默认有文件结束符EOF
//若不dd删除，直接读取，则文件第1个字符是回车，第二个是EOF
//fseek移动文件指针到末尾，只能移动到文件最后一个字符后，EOF文件结束符前

int main(){
    FILE *fp=fopen("5.txt","r");
//    fseek(fp,0,SEEK_END);
    char c=fgetc(fp);
    printf("c=%d\n",c);
    c=fgetc(fp);
    printf("c=%d\n",c);
    fclose(fp);
    return 0;
}
