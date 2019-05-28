#include<stdio.h>
#include<stdlib.h>
#include"link.h"
//main.c
//单链表与文件操作
//文件以文本文件读写 与 文本以二进制文件读写 
int main(){
    ST *head=NULL;
    head=read1_link();
    if(head){
	printf_link(head);
	save1_link(head);
	free_link(head);
    }
    else
	printf("数据不存在\n");
    /*
    ST *p=NULL;
    Data d1={10017,"lisi",'m',34};
    Data d2={10013,"lisi",'m',34};
    Data d3={10010,"lisi",'m',34};
    Data d4={10041,"lisi",'m',34};
    Data d5={10056,"lisi",'m',34};
    Data d6={10076,"lisi",'m',34};
    Data d7={10090,"lisi",'m',34};
    //尾创建
    head=create_link(head,d1);
    head=create_link(head,d2);
    //头创建
    head=create1_link(head,d3);
    head=create1_link(head,d4);
    printf_link(head);
  //查找  
    p=search_link(head,1002);
    printf("%d %s %c %d\n",
	    p->value.num,p->value.name,p->value.sex,p->value.age);

    putchar(012);
    //数据前插入1
//    head=insert1_link(head,d5,1001);
    //数据前插入2
//    head=insert2_link(head,d6,1002);
    //数据后插入
//    head=insert_link(head,d7,1003);
    printf("选择排序后打印\n");
    head=select_sort_link(head);
    printf_link(head);
    putchar(10);

    printf("倒置后打印\n");
    head=reverse_link(head);
    printf_link(head);
    putchar(10);

    printf("插入排序后打印\n");
    head=insert_sort_link(head);
    printf_link(head);
    putchar(10);

    //计算链表长度
    printf("len=%d\n",len_link(head));
    //释放
    free_link(head);
    */
    return 0;
}
