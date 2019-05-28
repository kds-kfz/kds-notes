#include "link.h"
#include <stdlib.h>
#include <stdio.h>
ST* creat_linkback(ST* h,Data da)
{
    ST* p = (ST*)malloc(sizeof(ST));
    p->value = da;
    if (h == NULL)
    {
	p->next = p;
	return p;
    }
    ST* tail = h;
    while (tail->next != h)
	tail = tail->next;
    tail->next = p;
    p->next = h;
    return h;
}
ST* creat_linkfront(ST* h,Data da)
{
    ST* p = (ST*)malloc(sizeof(ST));
    p->value = da;
    if (h == NULL)
    {
	p->next = p;
	return p;
    }
    p->next = h;
    ST* tail = h;
    while (tail->next != h)
	tail = tail->next;
    tail->next = p;
    return p;
}
ST* search_link(ST* h,int num)
{
    if (h == NULL)
	return h;
    ST* s = h;
    do{
	if(s->value.num == num)
	    return s;
	s = s->next;
    }while(s != h);
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
    ST* pre = h;
    while(pre->next != s)
	pre = pre->next;
    pre ->next = s->next;
    if(s == h)
	h = s->next;
    free(s);
    return h;
}
ST* insert_link(ST* h,Data d1,int num)
{
    ST* s = search_link(h,num);
    if (s == NULL)
	return h;
    ST* p = (ST*)malloc(sizeof(ST));
    p->value = d1;
    ST* pre = h;
    while (pre->next != s)
	pre = pre->next;
    p->next = s;
    pre->next = p;
    if (s == h)
	return p;
    else
	return h;
}
void print_link(ST* h)
{
    if (h == NULL)
	return ;
    ST* s = h;
    do{
	printf ("%d %s\n",s->value.num,s->value.name);
	s = s->next;
    }while(s!=h);
}
 void free_link(ST* h)
{
    if (h == NULL)
	return ;
    ST* s = h;
    ST* t = NULL;
    do{
	t = s;
	s = s->next;
	free(t);
    }while(s!=h);
}
