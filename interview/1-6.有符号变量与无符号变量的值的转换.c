#include<stdio.h>
#include<stdlib.h>
char getChar(int x, int y){
    char c;
    unsigned int a = x;
    //printf("%d, %d, %d\n", a, (unsigned int)y, a+y);//7 -8 -1
    return (a + y > 10) ? (c = 1) : (c = 2);
}
void install_1(){
    printf("程序退出，注册函数1\n");
}
void install_2(){
    printf("程序退出，注册函数2，后注册的先调用\n");
}
int main(){
    //[-7, 3]返回2，y被转换成很大的数，与x相加后溢出，即0；其他返回1
    atexit(install_1);
    atexit(install_2);
    printf("test_1 : x = 7, y = -8 , result = %d\n", getChar(7, -8));
    printf("program over ...\n");
    return 0;
}

