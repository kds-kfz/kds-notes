#include<stdio.h>
#include<stdlib.h>
#include"link.h"
//main.c
//双向链表
int main(){
    ST *head=NULL;
    Data d1={920,"lisi",'m',34};
    Data d2={921,"lisi",'m',34};
    Data d3={922,"lisi",'m',34};
    Data d4={923,"lisi",'m',34};
    Data d5={924,"lisi",'m',34};
    Data d6={925,"lisi",'m',34};
    Data d7={926,"lisi",'m',34};
    Data d8={927,"lisi",'m',34};
    Data d9={928,"lisi",'m',34};
    printf("尾创建\n");
    head=create_link(head,d6);
    head=create_link(head,d7);
    head=create_link(head,d9);
    head=create_link(head,d2);
    /*
    head=create_link(head,d5);
    head=create_link(head,d2);
    head=create_link(head,d9);
    head=create_link(head,d8);
    head=create_link(head,d1);
    head=create_link(head,d3);*/
    putchar(10);
    /*
    printf("头创建\n");
    head=create1_link(head,d4);
    head=create1_link(head,d5);
    head=create1_link(head,d6);
    putchar(10);
    */
    /*
    printf("查找\n");
    head=search_link(head,924);
    if(head)
	printf("%d %s %c %d\n",
	head->value.num,head->value.name,head->value.sex,head->value.age);
    putchar(10);
    */
    printf("从头打印\n");
    printf_link_pre(head);
    putchar(10);

    //数据前插入
//    head=insert_link(head,d9,921);
    //数据后插入
/*
    printf("删除\n");
    head=del_link(head,920);
    */

    printf("从头打印\n");
    printf_link_pre(head);
    putchar(10);

    head=select_sort_link(head);
    printf("从头打印\n");
    printf_link_pre(head);
    putchar(10);
    /*
    printf("从尾打印\n");
    printf_link_next(head);
    putchar(10);
    */
    //释放
    free_link(head);
    return 0;
}
