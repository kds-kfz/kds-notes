#include<stdio.h>
#include<stdlib.h>
#include"link.h"
//link.c

//环形链表函数
//函数1
ST *create_link(ST *head,Data d1){//尾创建
    ST *p=(ST *)malloc(sizeof(ST));
    p->value=d1;
    if(head==NULL){//第一次头为空
	p->next=p;//自己指向自己
	return p;
    }
    ST *tail=head;
    while(tail->next!=head)//找尾
	tail=tail->next;
    tail->next=p;//尾next指向新数据
    p->next=head;//新数据next指向头
    return head;
}
//函数2
ST *create1_link(ST *head,Data d1){//头创建
    ST *p=(ST *)malloc(sizeof(ST));
    p->value=d1;
    if(head==NULL){//第一次头为空
	p->next=p;//自己指向自己
    return p;
    }
    else{
	p->next=head;//新数据next指向头
	ST *s=head;
	while(s->next!=head)//找头的前一个
	    s=s->next;
	s->next=p;//头前一个next指向新数据
	return p;//返回新数据做头
    }
    /*
    p->next=head;
    ST *tail=head;
    while(tail->next!=head)
	tail=tail->next;
    tail->next=p;
    return p;
    */
}
//函数3
void printf_link(ST *head){
    /*
    if(head!=NULL){
	ST *s=head;
	while(s->next!=head){//如果下一个是头，说明现在是尾。此时不成立
	printf("%d %s %c %d\n",
		s->value.num,s->value.name,s->value.sex,s->value.age);
	s=s->next;//但此时，s走向尾
	}
	printf("%d %s %c %d\n",//打印尾数据
		s->value.num,s->value.name,s->value.sex,s->value.age);
    }
    */
    if(head!=NULL){
	ST *s=head;
	do{
	    printf("%d %s %c %d\n",
		s->value.num,s->value.name,s->value.sex,s->value.age);
	    s=s->next;
	}while(s!=head);//如果当前不是头，说明现在是尾，此时已经打印
    }
}
//函数4
void free_link(ST *head){
    if(head!=NULL){
	ST *s=head;
	ST *t=NULL;
	do{
	    t=s;//保存当前
	    s=s->next;//指针往下走
	    free(t);//释放当前
	}while(s!=head);//如果当前指针不是头，说明是尾
	printf("free success\n");
    }
}
//函数5
ST *search_link(ST *head,int num){//查找
    if(head!=NULL){
	ST *s=head;
	do{
	    if(s->value.num==num)//如果找到
		return s;//返回当前地址
	    s=s->next;
	}while(s!=head);
	return NULL;
    }
}
//函数6
ST *del_link(ST *head,int num){//删除
    if(head){
	ST *s=search_link(head,num);
	if(s==NULL)//没找到数据
	    return head;
	/*
	else if(s==head){
	    ST *tail=head;
	    while(tail->next!=head)
		tail=tail->next;
	    head=head->next;
	    tail->next=head;
	    }
	    */
	else{
	    ST *t=head;
	    while(t->next!=s)
		t=t->next;
	    t->next=s->next;//将前一个与后一个链接
	    if(s==head)//如果是第一个数据
		head=head->next;//将头移到下个数据
	    }
    free(s);//释放当前
    return head;
    }
}
//函数7
ST *insert1_link(ST *head,Data d1,int num){//前插入
    if(head){
	ST *p=(ST *)malloc(sizeof(ST));
	ST *s=search_link(head,num);
	p->value=d1;
	if(s==NULL)//没找到数据
	    return head;
	else{
	    ST *t=head;
	    while(t->next!=s)
		t=t->next;
	    t->next=p;
	    p->next=s;
	    if(s==head)//如果插入的是第一个数据前
		return p;//返回新的头
	    else
		return head;//否则返回旧的头
	}
    }
}
//函数8
ST *insert_link(ST *head,Data d1,int num){//后插入
    if(head){
	ST *p=(ST *)malloc(sizeof(ST));
	ST *s=search_link(head,num);
	p->value=d1;
	if(s==NULL)
	    return head;
	else{
//	    if(s->next==head)//如果插入的是最后一个，返回新的头
//		head=p;
	    p->next=s->next;
	    s->next=p;
	    return head;//返回旧的头
	}
    }
}
