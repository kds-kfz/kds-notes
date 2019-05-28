#include <stdio.h>
#include <stdlib.h>
#include  "tree.h"
T* creat_tree(T* root,int n)
{
    T* p = (T*)malloc(sizeof(T));
    p->num = n;
    p->left = NULL;
    p->right = NULL;
    if(root == NULL)
	return p;
    T* t = root;
    T* pre = NULL;
    while(t != NULL)
    {
	pre = t;
	if(n < t->num)
	    t = t->left;
	else
	    t = t->right;
    }
    if(n < pre->num)
	pre->left = p;
    else
	pre->right = p;
    return root;
}
void show1_tree(T* root)//前序
{
    if(root != NULL)
    {
	printf("%d ",root->num);
	show1_tree(root->left);
	show1_tree(root->right);
    }
}
void show2_tree(T* root)//中序
{
    if(root != NULL)
    {
	show2_tree(root->left);
	printf("%d ",root->num);
	show2_tree(root->right);
    }
}
void show3_tree(T* root)//后序
{
    if(root != NULL)
    {
	show3_tree(root->left);
	show3_tree(root->right);
	printf ("%d ",root->num);
    }
}
int len_tree(T* root)
{
    if(root == NULL)
	return 0;
    int leftnum = len_tree(root->left);
    int rightnum = len_tree(root->right);
    return leftnum+rightnum+1;
}
void free_tree(T* root)
{
    if(root != NULL)
    {
	free_tree(root->left);
	free_tree(root->right);
	free(root);
    }
}
T* search_tree(T* root,int n)
{

    while(root != NULL)
    {
	if(n < root->num)
	    root = root->left;
	else if (n > root->num)
	    root = root->right;
	else
	    return root;
    }
    return NULL;
}
T* search1_tree(T* root,int n)
{
    if(root == NULL)
    {
	return NULL;
    }
    if(n < root->num)
	return search1_tree(root->left,n);
    else if (n > root->num)
	return search1_tree(root->right,n);
    else
	return root;
}
T* del_tree(T* root,int n)
{
    T* s = search_tree(root,n);
    if (s == NULL)
    {
	printf("删除的数据不存在\n");
	return root;
    }
    //找前一个
    T* pre = NULL;
    T* t = root;
    while (t != NULL)
    {
	if(n < t->num)
	{
	    pre = t;
	    t = t->left;
	}
	else if (n > t->num)
	{
	    pre = t;
	    t = t->right;
	}
	else
	    break;
    }
    //删除
    if(s->left==NULL && s->right == NULL)
    {
	if (pre == NULL)
	    root = NULL;
	else if (n < pre->num)
	    pre->left = NULL;
	else
	    pre->right = NULL;
    }
    else if (s->left != NULL && s->right == NULL)
    {
	if(pre == NULL)
	    root = s->left;
	else if(n < pre->num)
	    pre->left = s->left;
	else
	    pre->right = s->left;
    }
    else if(s->left == NULL&& s->right != NULL)
    {
	if(pre == NULL)
	    root = s->right;
	else if(n < pre->num)
	    pre->left = s->right;
	else
	    pre->right = s->right;
    }
    else if(s->left != NULL &&s->right != NULL)
    {
	if(pre == NULL)
	    root = s->right;
	else if (n < pre->num)
	    pre->left = s->right;
	else
	    pre->right = s->right;

	T* t = s->right;
	while (t->left != NULL)
	    t = t->left;
	t->left = s->left;
    }
    free(s);
    return root;
}
