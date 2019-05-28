#ifndef _TREE_H
#define _TREE_H
//二叉树，数据结构定义，函数声明
typedef struct s{
    int num;
    struct s *left;
    struct s *right;
}T; 

extern T *create_tree(T *root,int num);//创建
extern T *create1_tree(T *root,int num);//递归创建

extern T *search_tree(T *root,int num);//查找
extern T *search1_tree(T *root,int num);//递归查找

extern void show1_tree(T *root);//前序遍历
extern void show2_tree(T *root);//中序遍历
extern void show3_tree(T *root);//后序遍历

extern void free_tree(T *root);//释放
extern T *del_tree(T *root,int num);//删除

#endif
