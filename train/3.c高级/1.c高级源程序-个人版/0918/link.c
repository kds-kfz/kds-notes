#include<stdio.h>
#include<stdlib.h>
#include"link.h"
//link.c
//链表函数
ST *create_link(ST *head,Data d1){//尾插入
    ST *p=(ST *)malloc(sizeof(ST));   
    p->value=d1;
    p->next=NULL;
    if(head==NULL)
	return p;
    ST *tail=head;
    while(tail->next!=NULL)//找最后一个数据地址
	tail=tail->next;
    tail->next=p;//数据地址链接
    return head;
}

ST *create1_link(ST *head,Data d1){//头插入
    /*
    ST *p=(ST *)malloc(sizeof(ST));   
    p->value=d1;
    p->next=NULL;
    if(head==NULL)
	return p;
//    if(head!=NULL){
    p->next=head;    
    return p;
//    }
    */
    ST *p=(ST *)malloc(sizeof(ST));   
    p->value=d1;
    p->next=head;
    return p;
}

void printf_link(ST *head){
    while(head){
	printf("%d %s %c %d\n",
	head->value.num,head->value.name,head->value.sex,head->value.age);
	head=head->next;
    }
}

void free_link(ST *head){
    while(head){
	ST *s=head;
	head=head->next;
	free(s);
    }
    printf("free success\n");
}
ST *search_link(ST *head,int num){//查找
    /*
    while(head!=NULL){
	if(head->value.num==num)
	    break;
	else
	    head=head->next;
    }
    if(head==NULL)
	return NULL;
    else
        return head;
    */
    while(head!=NULL){
	if(head->value.num==num)
	    return head;
	else
	    head=head->next;
    }
    return NULL;
//    return head;//此时未找到，head为空
}

ST *del_link(ST *head,int num){//删除
    ST *s=search_link(head,num);
    if(s==NULL)
	return head;//此时没找到，head为NULL
    else if(s==head)//如果num在第一个
	head=head->next;
    else{	    //如果num在中间或结尾
	ST *t=head;
	while(t->next!=s)//找s前一个指针，即t
	    t=t->next;
	t->next=s->next;
    }
    free(s);
    return head;
}

ST *insert_link(ST *head,Data d1,int num){//前插入
    ST *p=(ST *)malloc(sizeof(ST));
    ST *s=search_link(head,num);
    ST *t=head;
    p->value=d1;
//    p->next=NULL;
    if(s==head){
	p->next=head;
	return p;}
    else{
    while(t->next!=s)//找s前一个指针，即t
	t=t->next;
    p->next=s;
    t->next=p;
    return head;
    }
}

ST *insert1_link(ST *head,Data d1,int num){//后插入
    ST *p=(ST *)malloc(sizeof(ST));
    ST *s=search_link(head,num);
    p->value=d1;
//    p->next=NULL;
    p->next=s->next;
    s->next=p;
    return head;
}
int len_link(ST *head){//长度
    int count=0;
    while(head!=0){
	head=head->next;
	count++;
    }
    return count;
}
