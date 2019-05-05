#include<stdio.h>

#if 0
函数1：int scanf(const char *format, ...);
函数2：void setbuf(FILE *stream, char *buf);
#endif
int b=0;

int main(){
    char output[BUFSIZ];
    fflush(stdout);
    /* 将outbuf与stdout输出流相连接 */
    setbuf(stdout, output);
    /* 向stdout中放入一些字符串 */
    puts("output to stdout");
    fprintf(stdout,"hello world");
    /*刷新流*/
    fflush(stdout);
    /* 以下是outbuf中的内容 */
    puts(output);
    return 0;
}
