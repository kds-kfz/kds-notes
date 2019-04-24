#include<malloc.h>
#include"binary_tree.h"

//创建
T *create1_tree(T *root, int num){
    T *p = (T *)malloc(sizeof(T));
    p->num = num;
    p->left = NULL;
    p->right = NULL;
    if(root == NULL)
        return p;
    T *t = root;
    T *pre = NULL;
    while(t){
        pre = t;
        if(p->num < t->num)
            t = t->left;
        else
            t = t->right;
    }
    if(p->num < pre->num)
        pre->left = p;
    else
        pre->right = p;
    return root;
}

//递归创建
T *create2_tree(T *root, int num){

} 

//查找
T *search1_tree(T *root, int num){

} 

//递归查找
T *search2_tree(T *root, int num){

}

//前序遍历
void show1_tree(T *root){
    if(root){
        printf("%d ", root->num);
        show1_tree(root->left);
        show1_tree(root->right);
    }else
        putchar(10);
}

//中序遍历
void show2_tree(T *root){

}

//后序遍历
void show3_tree(T *root){

}

//释放
void free_tree(T *root){

}

//删除
T *del_tree(T *root){

}

