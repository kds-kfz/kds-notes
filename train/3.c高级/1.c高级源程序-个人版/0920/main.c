#include<stdio.h>
#include<stdlib.h>
#include"link.h"
//main.c
//环形链表,学习结束
int main(){
    ST *head=NULL;
    Data d1={920,"lisi",'m',34};
    Data d2={921,"lisi",'m',34};
    Data d3={922,"lisi",'m',34};
    Data d4={923,"lisi",'m',34};
    Data d5={924,"lisi",'m',34};
    Data d6={925,"lisi",'m',34};
    Data d7={926,"lisi",'m',34};
    //尾创建
    head=create_link(head,d1);
    head=create_link(head,d2);
    //头创建
    head=create_link(head,d3);
    head=create_link(head,d4);
//    head=create1_link(head,d3);
/*    //查找
    head=search_link(head,922);
    if(head)
    printf("%d %s %c %d\n",
	head->value.num,head->value.name,head->value.sex,head->value.age);
    */
    //数据前插入
    head=insert_link(head,d5,923);
    //数据后插入
    head=insert1_link(head,d4,923);
    //删除
    //head=del_link(head,920);
    //打印
    if(head)
    printf_link(head);
    //释放
    if(head)
    free_link(head);
    return 0;
}
