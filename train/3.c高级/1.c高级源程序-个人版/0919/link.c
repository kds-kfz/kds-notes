#include<stdio.h>
#include<stdlib.h>
#include"link.h"
//link.c

//链表函数
//函数1
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
//函数2
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
//函数3
void printf_link(ST *head){
    while(head){
	printf("%d %s %c %d\n",
	head->value.num,head->value.name,head->value.sex,head->value.age);
	head=head->next;
    }
}
//函数4
void free_link(ST *head){
    while(head){
	ST *s=head;
	head=head->next;
	free(s);
    }
    printf("free success\n");
}
//函数5
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
//函数6
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
//函数7
ST *insert1_link(ST *head,Data d1,int num){//前插入(方法1，个人版)
    ST *p=(ST *)malloc(sizeof(ST));
    ST *s=search_link(head,num);
    ST *t=head;
    p->value=d1;
    p->next=NULL;
    if(s==NULL)
	return head;
    else if(s==head){
	p->next=head;
	return p;
    }
    else{
    while(t->next!=s)//找s前一个指针，即t
	t=t->next;
    p->next=s;
    t->next=p;
    return head;
    }
}
//函数8
ST *insert2_link(ST *head,Data d1,int num){//前插入(方法2，教师版)
    ST *s=search_link(head,num);
    if(s==NULL)
	return head;
    else if(s==head)
	head=create1_link(head,d1);
    else{
	ST *p=(ST *)malloc(sizeof(ST));
	p->value=d1;
	p->next=NULL;
	ST *t=head;
	while(t->next!=s)
	    t=t->next;
	p->next=s;
	t->next=p;
    }
    return head;
}
//函数9
ST *insert_link(ST *head,Data d1,int num){//后插入
    ST *p=(ST *)malloc(sizeof(ST));
    ST *s=search_link(head,num);
    p->value=d1;
    p->next=NULL;
    if(s==NULL)
	return head;
    else{
	p->next=s->next;
	s->next=p;
	return head;
    }
}
//函数10
int len_link(ST *head){//长度
    int count=0;
    while(head!=0){
	count++;
	head=head->next;
    }
    return count;
}
//函数11
ST *reverse_link(ST *head){//链表倒置
    ST *n=NULL;//
    ST *t=NULL;//刚开始，head前一个指针为空
    while(head!=NULL){
	n=head->next;//保存下一个指针
	head->next=t;
	t=head;//保存head指针
	head=n;//head移到n指针
    }
    return t;
}
//函数12
ST *select_sort_link(ST *head){//选择排序
//每次循环找出链表中最大数和最大数的前1个
//把最大数从原链表删除（不释放）
//用头插入生成新链表
    ST *new=NULL;
    while(head!=NULL){
	ST *pmax=head;//每次默认第1个数据为最大
	ST *p=NULL;//当前指针p
	ST *t=NULL;//保存p前一个指针
	for(p=head;p->next!=NULL;p=p->next){
	    if(p->next->value.num > pmax ->value.num){
		//如果下一个指针数据大于当前head的数据
		pmax=p->next;//保存最大数据指针
		t=p;//保存当前head指针
	    }
	}
	if(t==NULL)//表示第1个数据为最大值
	    head=head->next;//改变大循环条件
	else	    //最大值在中间
	//将t->next(当前head指针下个指向)与pmax->next(最大数据下个指针)链接
	    t->next=pmax->next;
	//如果第一个数据最大，pmax->next指向空
	//第二次进来，则保存第二个最大值
	pmax->next=new;
	new=pmax;//保存第一个数据，生成新链表(取出pmax数据)
    }
    return new;
}
//函数13
//  3->2->1->7->5->NULL
//     new       n
//     3->NULL   2->   1->   7->   5->NULL
//  t  p
ST *insert_sort_link(ST *head){//插入排序
    ST *new=head;//定义new取head第一个数据
    head=head->next;//从第二个数据开始
    new->next=NULL;
    while(head!=NULL){//从第二个数据开始到结尾
	//取数据
	ST *n=head;//n保存head数据
	head=head->next;//head指向下一个数据
	//找位置
	ST *p=new;//p保存new数据
	ST *t=NULL;//刚开始新链表只有1个数据，t保存新链表前一个数据
	//第一次p不为空，但数据大小判断不成立走if语句
	//若p没有去到最后一个数，但数据大小判断成立，进入while
	//旧链表数据n->value.num与新链表p->value.num循环比较
	while(p!=NULL&&n->value.num > p->value.num){
	    t=p;//t保存新链表p的数据
	    p=p->next;//p取下个数据
	}
	//放数据
	if(t==NULL){//第二个数据比第一个小，同时新链表只有1个数据，即t为空
	    n->next=new;//第二个数据指针指向第一个
	    new=n;//指针new指向新的数据
	}//如果新数据一直比新链表new数据小，就一直走if语句，t永远为空
	else{//此时，数据插入在中间
	    n->next=t->next;//新数据连t的下个数据
	    t->next=n;//t的下个数据保存n的数据
	}
    }
    return new;
}
//函数14
ST *exchange_link(ST *head,int d1,int d2){//链表数据交换
    ST *p=head;
    ST *arr[len_link(head)];
    return NULL;
}

