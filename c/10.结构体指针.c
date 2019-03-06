#include<stdio.h>
#include<string.h>

//结构体定义
struct st_1{
    int num;
};

//结构体初始化
struct st_2{
    int num;
}st2 = {200};

//无名结构体
//之后不能定义变量
//定义的同时可以定义变量
struct {
    int num;
}st3;

//结构体嵌套
struct te{
    struct st_1 value;
    int num;
    char name[32];
};

int main(){
    int i;
    struct st_2 st2_1 = {150};
    struct te te_1 = {10, 20};

    struct st_2 *p_1 = &st2_1;
    struct te *p_2 = &te_1;
    printf("%d\n", p_1->num);
    printf("%d\n", st2_1.num);
    printf("%d, %d\n", (*p_2).value.num, p_2->num);

    //结构体数组
    struct te st3[5] = {0};
    for(i = 0; i < 5; i++){
        char str_1[] = "ab";
        char str_2[] = "po";
        strcat(str_1, str_2);
        st3[i].num = 10 + i;
        strcpy(st3[i].name, str_1);
        printf("%d, name = %s\n", st3[i].num, st3[i].name);
    }

    //结构体指针
    struct te *p = st3, *q = p;

    //() . [] -> * ++ --
    //先取值，然后地址+1
    //printf("num = %d, p_addr = %p, &num = %p\n", p++->num, &(p->num), &st3[0].num);
    //printf("num = %d, p_addr = %p, &num = %p\n", (*p).num++, &(p->num), &st3[0].num);
    //printf("num = %d, p_addr = %p, &num = %p\n", (*p++).num, &(p->num), &st3[0].num);
    //num = 10, q_addr = 0x7ffe1c8b98a4, &num = 0x7ffe1c8b98a4

    //先取值，然后值+1
    //num = 11, q_addr = 0x7fffdc279604, &num = 0x7fffdc279604
    //printf("num = %d, p_addr = %p, &num = %p\n", ++p->num, &(p->num), &st3[0].num);
    
    //先地址+1，再取值
    //num = 12, q_addr = 0x7ffc16272c5c, &num = 0x7ffc16272c5c
    //printf("num = %d, p_addr = %p, &num = %p\n", (*++p).num, &(*++p).num, &st3[1].num);

    //先取值，然后值+1
    //num = 11, q_addr = 0x7fff5029eb04, &num = 0x7fff5029eb04
    //printf("num = %d, p_addr = %p, &num = %p\n", ++(*p).num, ++(*p).num, &st3[0].num);
    
    return 0;
}
