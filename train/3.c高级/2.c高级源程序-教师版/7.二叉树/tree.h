#ifndef _TREE_H
#define _TREE_H 1
typedef struct tree{
    int num;
    struct tree* left;
    struct tree* right;
}T;
extern T* creat_tree(T* root,int n);
extern void show1_tree(T* root);
extern void show2_tree(T* root);
extern void show3_tree(T* root);
extern int len_tree(T* root);
extern void free_tree(T* root);
extern T* search_tree(T* root,int n);
extern T* search1_tree(T* root,int n);
extern T* del_tree(T* root,int n);
#endif

