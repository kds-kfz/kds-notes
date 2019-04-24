#ifndef __BINARY_TREE_H__
#define __BINARY_TREE_H__
#include<stdio.h>
typedef struct s{
    int num;
    struct s *left;
    struct s *right;
}T;

T *create1_tree(T *root, int num); //创建
T *create2_tree(T *root, int num); //递归创建
T *search1_tree(T *root, int num); //查找
T *search2_tree(T *root, int num); //递归查找

void show1_tree(T *root); //前序遍历
void show2_tree(T *root); //中序遍历
void show3_tree(T *root); //后序遍历
void free_tree(T *root);  //释放
T *del_tree(T *root);     //删除

#endif
