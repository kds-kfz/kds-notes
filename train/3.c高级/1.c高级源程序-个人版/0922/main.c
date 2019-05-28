#include<stdio.h>
#include"tree.h"
//2.c
//二叉树
//学习结束(20170921--20170922)
int main(){
    T *root=NULL;
    T *s=NULL;
    int n=0;
    //创建
    root=create_tree(root,80);
    root=create_tree(root,75);
    root=create_tree(root,85);
    root=create_tree(root,60);
    root=create_tree(root,76);
    //递归创建
    root=create1_tree(root,50);
    root=create1_tree(root,82);
    root=create1_tree(root,87);
    root=create1_tree(root,84);
    root=create1_tree(root,93);
    //查找
//    s=search_tree(root,80);
//    if(s)
//	printf("value=%d\n",s->num);
    //递归查找
while(1){
    //删除
    show2_tree(root);
    printf("\n");
    printf("输入要删除的数\n");
    scanf("%d",&n);
    root=del_tree(root,n);
//    printf("value=%d\n",root->num);
    //前序
//    show1_tree(root);
//    printf("\n");
    //中序
    show2_tree(root);
    printf("\n");
}
    //后序
//    show3_tree(root);
//    printf("\n");
    //释放
//    free_tree(root);
//    printf("\n");
    return 0;
}
