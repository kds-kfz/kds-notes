#include <stdio.h>
#include <stdlib.h>
#include "link.h"
ST* creat_linkback(ST* h,Data da)//尾插入
{
    ST* p = (ST*)malloc(sizeof(ST));
    p->value = da;
    if(h == NULL)
    {
	p->next = NULL;
	p->pre = NULL;
	return p;
    }
    ST* tail = h;
    while (tail->next != NULL)
	tail = tail->next;
    tail->next = p;
    p->pre = tail;
    p->next = NULL;
    return h;
}
ST* creat_linkfront(ST* h,Data da)//头插入
{
    ST* p = (ST*)malloc(sizeof(ST));
    p->value = da;
    if(h == NULL)
    {
	p->next = NULL;
	p->pre = NULL;
	return p;
    }
    p->next = h;
    h->pre = p;
    p->pre = NULL;
    return p;
}
ST* search_link(ST* h,int num)//查找
{
    while(h!= NULL)
    {
	if(h->value.num == num)
	    return h;
	h = h->next;
    }
    return NULL;
}
ST* del_link(ST* h,int num)//删除
{
    ST* s = search_link(h,num);
    if (s == NULL)
    {
	printf ("删除学号不存在\n");
	return h;
    }
    if (s == h)
    {
	h = h->next;
	if(h != NULL)
	    h->pre = NULL;
    }
    else if (s->next != NULL)
    {
	s->pre->next = s->next;
	s->next->pre = s->pre;
    }
    else
	s->pre->next = NULL;
    free(s);
    printf ("删除成功\n");
    return h;
}
ST* insert_link(ST* h,Data d1,int num)//插入
{
    ST* s = search_link(h,num);
    if (s == NULL)
    {
	printf ("插入的位置不存在\n");
	return h;
    }
    ST* p = (ST*)malloc(sizeof(ST));
    p->value = d1;
    if (s == h)
    {
	p->next = h;
	h->pre = p;
	p->pre = NULL;
	return p;
    }
    else
    {
	p->next = s;
	p->pre = s ->pre;
	s->pre->next = p;
	s->pre = p;
	return h;
    }
}
ST* select_link(ST* h)//选择排序
{
    ST* newh = NULL;
    ST* max = NULL;
    ST* p = NULL;
    while (h)
    {
	//找最大数
	max = h;
	for(p=h;p->next != NULL;p=p->next)
	{
	    if(p->next->value.num > max->value.num)
		max = p->next;
	}
	//删除
	if (max == h)
	{
	    h = h->next;
	    if(h != NULL)
	      h->pre = NULL;
	}
	else if(max->next != NULL)
	{
	    max->pre->next = max->next;
	    max->next->pre = max->pre;
	}
	else
	    max->pre->next = NULL;
	//生成新链表
	max->next = newh;
	max->pre = NULL;
	if (newh != NULL)
	    newh->pre = max;
	newh = max;
    }
    return newh;
}
void print_link(ST* h)//打印next
{
    while(h!=NULL)
    {
	printf ("%d %s %c\n",h->value.num,h->value.name,h->value.sex);
	h = h->next;
    }
}
void print1_link(ST* h)//打印pre
{
    ST* t = h;
    while (t->next != NULL)
	t = t->next;

    while(t != NULL)
    {
	printf ("%d %s %c\n",t->value.num,t->value.name,t->value.sex);
	t = t->pre;
    }
}
void free_link(ST* h)//释放
{
    ST* s = NULL;
    while (h != NULL)
    {
	s = h;
	h = h->next;
	free(s);
    }
}
