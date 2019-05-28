#include<stdio.h>
#include<stdlib.h>
#include"link.h"
//link.c
//双向链表函数
//函数1
ST *create_link(ST *head,Data d1){//尾创建
    ST *p=(ST *)malloc(sizeof(ST));
    p->value=d1;
    if(head==NULL){
	p->pre=NULL;
	p->next=NULL;
	return p;
    }
    ST *tail=head;
	while(tail->next!=NULL)
	    tail=tail->next;
	tail->next=p;
	p->next=NULL;
	p->pre=tail;
    return head;
}
//函数2
ST *create1_link(ST *head,Data d1){//头创建
    ST *p=(ST *)malloc(sizeof(ST));
    p->value=d1;
    if(head==NULL){
	p->pre=NULL;
	p->next=NULL;
	return p;
    }
    p->next=head;
    head->pre=p;
    p->pre=NULL;
    return p;
}
//函数3
void printf_link_next(ST *head){//从尾打印
    if(head){
	/*
	ST *tail=head;
	while(tail->next!=NULL)
	    tail=tail->next;
	while(tail){
	printf("%d %s %c %d\n",
	tail->value.num,tail->value.name,tail->value.sex,tail->value.age);
	tail=tail->pre;
	}
	*/
	while(head->next!=NULL)
	    head=head->next;
	while(head){
	printf("%d %s %c %d\n",
	head->value.num,head->value.name,head->value.sex,head->value.age);
	head=head->pre;
	}
    }
}
//函数4
void printf_link_pre(ST *head){//从头打印
    while(head){
    printf("%d %s %c %d\n",
    head->value.num,head->value.name,head->value.sex,head->value.age);
    head=head->next;
    }
}
//函数5
void free_link(ST *head){
    if(head){
	ST *t=head;
	while(head){
	    t=head;
	    head=head->next;
	    free(t);
	}
	printf("free success\n");
    }
}
//函数6
ST *search_link(ST *head,int num){//查找
	while(head){
	    if(head->value.num==num)
		return head;
	    head=head->next;
	}
//	if(head==NULL)
//	    return NULL;
	return head;//此时为空
}
//函数7
ST *del_link(ST *head,int num){//删除
    if(head){
	ST *s=search_link(head,num);
	ST *t=head;
	if(s==NULL)
	    return head;
	/*
	else if(s==head){//如果删掉第一个
	    if (head->next)//如果链表至少有2个结构体
	    {
		head = head->next;
		head->pre = NULL;
//		s->next->pre=NULL;//第二个pre指向空
//		head=head->next;//头后移
		}
	    else{//链表只有1个结构体
		free(s);//把头释放
		return NULL;//返回空
		}
	    }
	    */
	    else if(s==head){
		head=head->next;
		if(head)
		    head->pre=NULL;
	    }
	    else if(s->next==NULL)//如果删掉最后一个
		s->pre->next=NULL;//前一个数据next指向空
	    else{		      //如果删掉中间
		s->pre->next=s->next;
		s->next->pre=s->pre;
		}
	free(s);
	return head;
	}
}
//函数8
ST *insert1_link(ST *head,Data d1,int num){//前插入
    ST *s=search_link(head,num);
    if(s==NULL)
	return head;
    else{
	ST *p=(ST *)malloc(sizeof(ST));
	p->value=d1;
	if(s==head){
	    p->next=head;
	    head->pre=p;
	    p->pre=NULL;
	    return p;
	    }
	else{		//注意链接顺序，先链接后断开
	    p->next=s;
	    s->pre->next=p;
	    s->pre=p;
	    p->pre=s->pre;
	}
	return head;
    }
}
//函数9
ST *insert_link(ST *head,Data d1,int num){//后插入
    ST *s=search_link(head,num);
    if(s==NULL)
	return head;
    else{
	ST *p=(ST *)malloc(sizeof(ST));
	p->value=d1;
	if(s->next!=NULL){//注意顺序，先链接后断开
	    p->next=s->next;
	    s->next=p;
	    s->next->pre=p;
	    p->pre=s;
	}
	else{
	    s->next=p;
	    p->next=NULL;
	    p->pre=s;
	}
    }
    return head;
}
//函数10
ST *select_sort_link(ST *head){//选择排序
    ST *new=NULL;
    while(head!=NULL){
    ST *pmax=head;  
    ST *p=NULL;  
	for(p=head;p->next!=NULL;p=p->next){
	    if(p->next->value.num > pmax->value.num)
		pmax=p->next;
	}
//	if(pmax==head){//单向取值，只需走head，指向下一个
	if(pmax->pre==NULL){//考虑双向，若pmax前一个不为空，必定是第1个
	    head=head->next;//head往后走
	    if(head)//如果不是空
	    head->pre=NULL;//必须将前一个置空，否则if判断永远不会成立
	}
	else if(pmax->next!=NULL){
	    pmax->pre->next=pmax->next;
	    pmax->next->pre=pmax->pre;
	}
	else
	    pmax->pre->next=NULL;
	/*
	if(new==NULL){
	    pmax->next=new;
	    pmax->pre=NULL;
	    pmax->next=new;
	    new=pmax;
	    }
	else{
	    pmax->next=new;
	    new->pre=pmax;
	    pmax->pre=NULL;
	    new=pmax;
	}
	*/
	pmax->next=new;
	pmax->pre=NULL;
	if(new)
	    new->pre=pmax;
	new=pmax;
    }
	return new;
}
