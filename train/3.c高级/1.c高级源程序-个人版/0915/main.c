#include"/usr/include/stdio.h"
//main.c
//宏定义与预编译
#include"fun1.h"
#include"fun2.h"

#define TRUE 2
#define FALSE 0
#define windows1 

#undef windows1 
int main(){
#if TRUE == 2
    printf("宏定义----------------\n");
    fun1();
    fun2();
#ifdef windows1
    printf("windows1--------------\n");
#else
    printf("no window1------------\n");
#endif

#ifndef linux1
    printf("linux1----------------\n");
#endif
    printf("end-------------------\n");
#endif
    return 0;
}
