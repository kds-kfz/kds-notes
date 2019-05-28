#include <stdio.h>
#include "link.h"
#include <stdlib.h>
int main()
{
    system("clear");
    Data d1 = {1005,"zhang",'M'};
    Data d2 = {1003,"zhang",'M'};
    Data d3 = {1007,"zhang",'M'};
    Data d4 = {1001,"zhang",'M'};
    Data d5 = {1008,"zhang",'M'};
    Data d6 = {1002,"zhang",'M'};

    ST*head = NULL;
    head = creat_linkback(head,d1);
    head = creat_linkback(head,d2);
    head = creat_linkback(head,d3);
    head = creat_linkback(head,d4);
    head = creat_linkback(head,d5);
    head = creat_linkback(head,d6);

    printf ("next打印\n");
    print_link(head);

    printf("pre打印\n");
    print1_link(head);

    int n = 0;
    printf ("输入删除的学号\n");
    scanf("%d",&n);
    head = del_link(head,n);
    printf ("删除后打印\n");
    print_link(head);
    Data d7 = {1030,"wang",'M'};
    printf ("输入插入的学\n");
    scanf("%d",&n);
    head = insert_link(head,d7,n);
    printf ("插入后打印\n");
    print_link(head);

    head =  select_link(head);
    printf ("排序后打印\n");
    print_link(head);

    free_link(head);
    return 0;
}
