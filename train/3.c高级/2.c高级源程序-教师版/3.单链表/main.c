#include <stdio.h>
#include <stdlib.h>
#include "link.h"
int main()
{
    system("clear");
    Data d1 = {1001,"zhangsan",'M'};
    Data d2 = {1007,"zhangsan",'M'};
    Data d3 = {1004,"zhangsan",'M'};
    Data d4 = {1002,"zhangsan",'M'};
    Data d5 = {1006,"zhangsan",'M'};
    Data d6 = {1008,"zhangsan",'M'};
    Data d7 = {1003,"zhangsan",'M'};
    Data d8 = {1005,"zhangsan",'M'};
    Data d9 = {1010,"zhangsan",'M'};
    Data d10 = {1009,"zhangsan",'M'};

    ST* head = NULL;
    ST* head1 = NULL;
    head = creat_linkback(head,d1);
    head = creat_linkback(head,d2);
    head = creat_linkback(head,d3);
    head = creat_linkback(head,d4);
    head = creat_linkback(head,d5);

    head1 = creat_linkback(head1,d6);
    head1 = creat_linkback(head1,d7);
    head1 = creat_linkback(head1,d8);
    head1 = creat_linkback(head1,d9);
    head1 = creat_linkback(head1,d10);

    head = select_link(head);
    printf ("head--------------\n");
    print_link(head);

    head1 = select_link(head1);
    printf ("head1--------------\n");
    print_link(head1);

    ST* H = merge_link(head,head1);
    printf ("合并后打印\n");
    print_link(H);
    free_link(H);
#if 0


    print_link(head);
/*
    int n = 0;
    printf ("输入删除的学号\n");
    scanf("%d",&n);
     head =  del_link(head,n);
    ST* s = search_link(head,n);
    if (s != NULL)
	printf ("%d %s %c\n",s->value.num,s->value.name,s->value.sex);
    else
	printf ("不存在\n");
    printf ("删除后打印\n");
    int len = len_link(head);
    printf ("表长为%d\n",len);
*/
    head = select_link(head);
    printf ("排序后打印\n");
    print_link(head);

    head = reverst_link(head);
    printf ("倒置后打印\n");
    print_link(head);
    Data s1 = {0};
    printf ("输入学号\t姓名\t性别\t\n");
    scanf("%d %s %c",&s1.num,s1.name,&s1.sex);
    int n = 0;
    printf ("输入要插入哪一个学号前面\n");
    scanf("%d",&n);
    head = insert_link(head,s1,n);
    printf ("插入后打印\n");
    print_link(head);
    free_link(head);
#endif
    return 0;
}
