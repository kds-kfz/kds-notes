#include<stdio.h>

int main(){
    //const 保护、限定

    //常量a，只读，不可写
    //const int a = 10;
    int const a = 10;
    int b = 20;
    int const c = 30;
    //a = 2; //错误

    //指向常量的指针，保护值，不可改
    int const *p_1 = &a;
    //*p_1 = 30;//错误
    p_1 = &b;//正确

    //常指针，保护地址，不可改
    //int *const p_2 = &b;//错误，常量a的值只读，而指针变量p_2可以写
    int *const p_2 = &b;
    //p_2 = &a;//错误
    *p_2 = 40;

    //指向常量的常指针，保护地址和值，指针变量不可再指向其他变量地址
    int const *const p_3 = &a;
    //p_3 = &c;//错误
    //*p_3 = 50;//错误

    return 0;
}
