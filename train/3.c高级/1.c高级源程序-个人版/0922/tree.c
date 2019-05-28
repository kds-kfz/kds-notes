#include<stdio.h>
#include<stdlib.h>
#include"tree.h"
//二叉树函数
//函数1
T *create_tree(T *root,int num){//创建
    T *p=(T *)malloc(sizeof(T));
    p->num=num;
    p->left=NULL;
    p->right=NULL;
    if(root==NULL)
	return p;
    T *t=root;
    T *pre=NULL;
    while(t){
	pre=t;
	if(p->num < t->num)
	    t=t->left;
	else
	    t=t->right;
    }
    if(p->num < pre->num)
	pre->left=p;
    else
	pre->right=p;
    return root;
}
//函数2
T *create1_tree(T *root,int num){//递归创建
    if(root==NULL){
	root = (T *)malloc(sizeof(T));
	root->num=num;
	root->left=NULL;
	root->right=NULL;
    }
    else if(num < root->num)
	root->left = create1_tree(root->left,num);
    else
	root->right = create1_tree(root->right,num);
    return root;
}
//函数3
T *search_tree(T *root,int num){//查找
    while(root){
	if(num < root->num)
	    root=root->left;
	else if(num > root->num)
	    root=root->right;
	else
	    return root;
    }
    return NULL;
}
//函数4
T *search1_tree(T *root,int num){//递归查找
    if(root==NULL)
	return NULL;
    if(num < root->num)
	return search1_tree(root->left,num);
    else if(num > root->num)
	return search1_tree(root->right,num);
    else
	return root;
}
//函数5
void show1_tree(T *root){//前序遍历
    if(root){
	printf("%d ",root->num);
	show1_tree(root->left);
	show1_tree(root->right);
    }
}
//函数6
void show2_tree(T *root){//中序遍历
    if(root){
	show2_tree(root->left);
	printf("%d ",root->num);
	show2_tree(root->right);
    }
}
//函数7
void show3_tree(T *root){//后序遍历
    if(root){
	show3_tree(root->left);
	show3_tree(root->right);
	printf("%d ",root->num);
    }
}
//函数8
void free_tree(T *root){//释放
    if(root){
	free_tree(root->left);
	free_tree(root->right);
	free(root);
    }
}
//函数9
/*
T *del_tree(T *root,int num){//删除
    T *s=root;
    T *pre=root;
    while(s){
	if(num < s->num){
	    pre=s;
	    s=s->left;
	}
	else if(num > s->num){
	    pre=s;
	    s=s->right;
	}
	else
	    break;
    }
    if(s==NULL){//1
	return root;
    }
    else if(s->left==NULL && s->right==NULL){//2
	if(pre!=s){
	    if(num < pre->num)
		pre->left=NULL;
	    else 
		pre->right=NULL;
	}
	else
	    root=NULL;
    }
    else if(s->left!=NULL && s->right==NULL){//3
	if(pre!=s){
	    if(num < pre->num)
		pre->left=s->left;
	    else
		pre->right=s->left;
	}
	else
	    root=s->left;
    }
    else if(s->left==NULL && s->right!=NULL){//4
	if(pre!=s){
	    if(num < pre->num)
		pre->left=s->right;
	    else
		pre->right=s->right;
	}
	else
	    root=s->right;
    }
    else if(s->left!=NULL && s->right!=NULL){//5
	if(pre!=s){
	    if(num < pre->num){
		pre->left=s->right;
		s->right->left=s->left;
	    }
	    else{
		s->right->left=s->left->right;
		s->left->right=s->right;
		pre->right=s->left;
	    }
	}
	else{//删除根,找新根
	    T *tail=s->right;
	    while(tail->left!=NULL)
		tail=tail->left;
	    
	    tail->left=s->left;
	    root=s->right;
	}
    }
    free(s);
    return root;
}
*/
//函数10
T *del_tree(T *root,int num){//删除
    T *s=root;
    T *pre=NULL;
    while(s){
	if(num < s->num){
	    pre=s;
	    s=s->left;
	}
	else if(num > s->num){
	    pre=s;
	    s=s->right;
	}
	else
	    break;
    }
    if(s==NULL){//1
	return root;
    }
    else if(s->left==NULL&&s->right==NULL){//2
	if(pre==NULL)
	    root=NULL;
	else if(num < pre->num)
	    pre->left=NULL;
	else 
	    pre->right=NULL;
    }
    else if(s->left!=NULL&&s->right==NULL){//3
	if(pre==NULL)
	    root=s->left;
	else if(num < pre->num)
	    pre->left=s->left;
	else
	    pre->right=s->left;
    }
    else if(s->left==NULL&&s->right!=NULL){//4
	if(pre==NULL)
	    root=s->right;
	else if(num < pre->num)
	    pre->left=s->right;
	else
	    pre->right=s->right;
    }
    else if(s->left!=NULL&&s->right!=NULL){//5
	if(pre==NULL)
	    root=s->right;
	else if(s->num < pre->num)
	    pre->left=s->right;
	else
	    pre->right=s->right;

	T *t=s->right;
	while(t->left!=NULL)
	    t=t->left;
	t->left=s->left;
    }
    free(s);
    return root;
}

