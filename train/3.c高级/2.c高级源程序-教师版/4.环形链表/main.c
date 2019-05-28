#include "link.h"
#include <stdio.h>
int main()
{
    Data d1 = {1001,"zhang"};
    Data d2 = {1002,"zhang"};
    Data d3 = {1003,"zhang"};
    Data d4 = {1004,"zhang"};
    Data d5 = {1005,"zhang"};

    ST* head = NULL;
    head = creat_linkback(head,d1);
    head = creat_linkback(head,d2);
    head = creat_linkback(head,d3);
    head = creat_linkback(head,d4);
    head = creat_linkback(head,d5);

    int n = 0;
    printf ("输入插入的学号\n");
    scanf("%d",&n);
    Data d6 = {1020,"wang"};
    head = insert_link(head,d6,n);
    /*
    ST* s = search_link(head,n);
    if (s)
	printf ("%d %s\n",s->value.num,s->value.name);
    else
	printf ("不存在\n");
    printf ("---------------------------\n");
    */
    printf ("插入后打印\n");
    print_link(head);
    free_link(head);
    return 0;

}
