#include<stdio.h>

int main(){
    int a = 10;
    int *p = &a;
    //p_addr = 0x7ffdde1876dc, p_value = 10
    printf("p_addr = %p, p_value = %d\n", p, *p);
    //p++;
    //0x7ffd92f0e7d8, p_value = -1829705768
    //printf("p_addr = %p, p_value = %d\n", p, *p);
    int **q = &p;
    //a_addr = 0x7ffdde1876dc, q_addr = 0x7ffdde1876dc, p_addr = 0x7ffdde1876dc, q_value = 10
    printf("a_addr = %p, q_addr = %p, p_addr = %p, q_value = %d\n", &a, *q, p, **q);

    //a的地址，p指针变量地址，q指针变量地址
    //&a = 0x7ffdde1876dc, &p = 0x7ffdde1876d0, &q = 0x7ffdde1876c8
    printf("&a = %p, &p = %p, &q = %p\n", &a, &p, &q);
    return 0;
}
