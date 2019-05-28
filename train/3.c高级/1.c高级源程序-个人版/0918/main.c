#include<stdio.h>
#include<stdlib.h>
#include"link.h"
//main.c
//单链表,学习2

int main(){
    ST *head=NULL;
    ST *p=NULL;
    Data d1={1001,"lisi",'m',34};
    Data d2={1002,"lisi",'m',34};
    Data d3={1003,"lisi",'m',34};
    Data d4={1004,"lisi",'m',34};
    Data d5={1005,"lisi",'m',34};
    head=create_link(head,d1);
    head=create_link(head,d2);
    head=create_link(head,d3);
    printf_link(head);
/*    
    p=search_link(head,1002);
    printf("%d %s %c %d\n",
	    p->value.num,p->value.name,p->value.sex,p->value.age);
*/
    putchar(012);
    head=insert_link(head,d5,1001);
    head=insert1_link(head,d4,1003);
    printf_link(head);
    putchar(10);
    printf("len=%d\n",len_link(head));
    free_link(head);
    return 0;
}
