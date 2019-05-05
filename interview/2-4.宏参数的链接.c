#include<stdio.h>

#define STR(s) #s
#define CONS(a, b) (int)(a##e##b)
int main(){
    printf(STR(hello world));
    putchar(10);
    //2 * 10^3
    printf("%d\n", CONS(2, 3));
    return 0;
}
