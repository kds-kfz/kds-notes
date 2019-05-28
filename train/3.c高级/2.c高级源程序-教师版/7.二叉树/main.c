#include <stdio.h>
#include "tree.h"
int main()
{
    T* root = NULL;
    root = creat_tree(root,100);
    root = creat_tree(root,80);
    root = creat_tree(root,110);
    root = creat_tree(root,76);
    root = creat_tree(root,85);
    root = creat_tree(root,105);
    root = creat_tree(root,120);
    root = creat_tree(root,70);
    root = creat_tree(root,90);
    root = creat_tree(root,115);
    root = creat_tree(root,87);

    printf ("前序打印\n");
    show1_tree(root);
    printf ("\n");

    printf ("中序打印\n");
    show2_tree(root);
    printf ("\n");

    printf ("后序打印\n");
    show3_tree(root);
    printf ("\n");


    int len = len_tree(root);
    printf ("len = %d\n",len);


    int n = 0;
    printf ("输入删除的数据\n");
    scanf("%d",&n);
    root = del_tree(root,n);
    printf ("删除后打印\n");
    show2_tree(root);
    printf ("\n");
    /*
    int n = 0;
    printf ("输入要找的数据\n");
    scanf("%d",&n);
    T* s = search1_tree(root,n);
    if ( s != NULL)
	printf ("%d \n",s->num);
    else
	printf ("不存在\n");
	*/
    free_tree(root);
    return 0;
}
