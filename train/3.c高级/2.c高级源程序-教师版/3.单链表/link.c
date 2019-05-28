#include <stdio.h>
#include <stdlib.h>
#include "link.h"
ST* creat_linkback(ST* h,Data da)
{
    ST* p = (ST*)malloc(sizeof(ST));
    p->value = da;
    p->next = NULL;
    if (h == NULL)
	return p;
    //循环找到链表最后一个数据(next == NULL)
    ST* s = h;
    while (s->next != NULL)
	s = s->next;
    s->next = p;
    return h;
}
ST* creat_linkfront(ST* h,Data da)
{
    ST* p = (ST*)malloc(sizeof(ST));
    p->value = da;
    p->next = h;
    return p;
}
ST* search_link(ST* h,int num)//查找
{
    while (h != NULL)
    {
	if (h->value.num == num)
	    return h;
	h = h->next;
    }
    return NULL;
}
ST* del_link(ST* h,int num)
{
    ST* s = search_link(h,num);
    if (s == NULL)
    {
	printf ("删除的学号不存在\n");
	return h;
    }
    if (s == h)
	h = h->next;
    else
    {
	ST* pre = h;
	while (pre->next != s)
	    pre = pre->next;
	pre->next = s->next;
    }
    printf ("删除成功\n");
    free(s);
    return h;
}
ST* select_link(ST* h)//选择排序
{
    //循环一次找到链表里面最大的数据　从原链表脱离　放到新链表最后一个(头插入)　在剩下的数据里面找最大数　删除　创建新链表　　直到原链表为空结束
    ST* newh = NULL;
    ST* p = NULL;
    while (h)
    {
	//找最大数和前一个
	ST* max = h;
	ST* pre = NULL;
	for(p = h;p->next != NULL;p = p->next)
	{
	    if(p->next->value.num > max->value.num)
	    {
		max = p->next;
		pre = p;
	    }
	}
	//删除
	if(pre == NULL)
	    h = h->next;
	else
	    pre->next = max->next;
	//生成新链表
	max->next = newh;
	newh = max;
    }
    return newh;
}
ST* reverst_link(ST* h)//倒置
{
    ST* n = NULL;
    ST* pre = NULL;
    while (h != NULL)
    {
	n = h->next;
	h->next = pre;
	pre = h;
	h = n;
    }
    return pre;
}
ST* insert_link(ST* h,Data d1,int num)
{
    ST* s = search_link(h,num);
    if (s == NULL)
    {
	printf ("学号不存在\n");
	return h;
    }
    ST* p = (ST*)malloc(sizeof(ST));
    p->value = d1;
    if (s == h)
    {
	p->next = h;
	printf ("插入成功\n");
	return p;
    }
    else
    {
	ST* pre = h;
	while (pre->next != s )
	    pre = pre->next;
	pre->next = p;
	p->next = s;
	printf ("插入成功\n");
	return h;
    }
}
int len_link(ST* h)
{
    int n = 0;
    while (h)
    {
	n++;
	h = h->next;
    }
    return n;
}
void print_link(ST* h)//打印
{
    while(h)
    {
	printf ("%d %s %c\n",h->value.num,h->value.name,h->value.sex);
	h = h->next;
    }
}

void free_link(ST* h)//释放
{
    ST* s = NULL;
    while (h)   
    {
	s = h;
	h = h->next; 
	free(s);
    }
}
ST* merge_link(ST* h1,ST* h2)
{
    if(h1 == NULL)
	return h2;
    if(h2 == NULL)
	return h1;
    ST* h = NULL;
    if (h1->value.num < h2->value.num)
    {
	h = h1;
	h1 = h1->next;
    }
    else
    {
	h = h2;
	h2 = h2->next;
    }
    ST* cur = h;
    while (h1 != NULL && h2 != NULL)
    {
	if(h1->value.num < h2->value.num)
	{
	    cur->next = h1;
	    cur = h1;
	    h1 = h1->next;
	}
	else
	{
	    cur->next = h2;
	    cur = h2;
	    h2 = h2->next;
	}
    }
    if (h1 == NULL)
	cur->next = h2;
    if(h2 == NULL)
	cur->next = h1;
    return h;
}

