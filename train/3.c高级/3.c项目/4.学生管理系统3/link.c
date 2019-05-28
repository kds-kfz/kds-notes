#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include"link.h"
//link.c
//双向链表函数
//函数1(学生)
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
//函数1(教师)
ST1 *create2_link(ST1 *head,Data1 d1){//尾创建
    ST1 *p=(ST1 *)malloc(sizeof(ST1));
    p->value1=d1;
    if(head==NULL){
	p->pre=NULL;
	p->next=NULL;
	return p;
    }
    ST1 *tail=head;
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
	printf("%d %s %s %d\n",
	head->value.num,head->value.name,head->value.sex,head->value.age);
	head=head->pre;
	}
    }
}
//函数4(学生)
void printf_link_pre(ST *head){//从头打印
    while(head){
    printf("\033[3C");
    printf("%s\t%d\t%s\t%d\t%s\t%.2f\t%.2f\t%.2f\t%s\t%s\t%s\n",
    head->value.number,head->value.num,head->value.name,
    head->value.age,head->value.sex,
    head->value.chinese,head->value.math,head->value.english,
    head->value.clas1,head->value.job,head->value.clas2);
    head=head->next;
    }
}
//函数4(教师)
void printf1_link_pre(ST1 *head){//从头打印
    while(head){
    printf("\033[12C");
    printf("%s\t%d\t%s\t%d\t%s\t%s\t%s\t%s\n",
    head->value1.number,head->value1.num,head->value1.name,
    head->value1.age,head->value1.sex,head->value1.job,
    head->value1.clas,head->value1.course);
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
//函数6(学生)
ST *search_link(ST *head,char arr[]){//帐号查找
	while(head){
	    if(!strcmp(head->value.number,arr))
		return head;
	    head=head->next;
	}
//	if(head==NULL)
//	    return NULL;
	return head;//此时为空
}
//函数6(学生)
ST *search2_link(ST *head,int num){//学号查找
	while(head){
	    if(head->value.num==num)
		return head;
	    head=head->next;
	}
//	if(head==NULL)
//	    return NULL;
	return head;//此时为空
}
//函数6(教师)
ST1 *search1_link(ST1 *head,char arr[]){//帐号查找
	while(head){
	    if(!strcmp(head->value1.number,arr))
		return head;
	    head=head->next;
	}
//	if(head==NULL)
//	    return NULL;
	return head;//此时为空
}
//函数6(教师)
ST1 *search3_link(ST1 *head,int num){//帐号查找
	while(head){
	    if(head->value1.num==num)
		return head;
	    head=head->next;
	}
//	if(head==NULL)
//	    return NULL;
	return head;//此时为空
}
//函数7
ST *del_link(ST *head,int num){//以学号删除(学生)
    if(head){
	ST *s=search2_link(head,num);
	ST *t=head;
	if(s==NULL){
	    printf("\033[\t\t\t\tC");
	    printf("该学号学生不存在\n");
	    sleep(2);
	    return head;
	}
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
	printf("\033[\t\t\t\tC");
	printf("注销学生成功\n");
	sleep(2);
	return head;
	}
}
//函数7
ST1 *del1_link(ST1 *head,int num){//以工号删除(教师)
    if(head){
	ST1 *s=search3_link(head,num);
	ST1 *t=head;
	if(s==NULL){
	    printf("\033[\t\t\t\tC");
	    printf("该工号教师不存在\n");
	    sleep(2);
	    return head;
	}
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
	printf("\033[\t\t\t\tC");
	printf("注销教师成功\n");
	sleep(2);
	return head;
	}
}
//函数8
ST *insert1_link(ST *head,Data d1,char arr[]){//前插入
    ST *s=search_link(head,arr);
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
ST *insert_link(ST *head,Data d1,char arr[]){//后插入
    ST *s=search_link(head,arr);
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
	    if(p->next->value.total < pmax->value.total)
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
//函数11,数据把握存到文件(学生)
void save_link(ST *head){
    FILE *fp=fopen("student.txt","w+");
    while(head){
	head->value.total=head->value.chinese+head->value.math+head->value.english;
	fprintf(fp,"%s %s %d %s %d %s %.2f %.2f %.2f %.2f %s %s %s\n",
	head->value.number,head->value.key,head->value.num,
	head->value.name,head->value.age,head->value.sex,
	head->value.chinese,head->value.math,head->value.english,
	head->value.total,
	head->value.clas1,head->value.job,head->value.clas2);
	head=head->next;
    }
    printf("\033[\t\t\t\tC");
    printf("数据保存成功\n");
    sleep(1);
    fclose(fp);
}
//函数11,数据把握存到文件(学生)
void save2_link(ST *head){
    int rank=0;
    FILE *fp=fopen("st_rank.txt","w");
    fprintf(fp,"\t\t\t所有学生成绩排名\n");
    fprintf(fp,"学号\t姓名\t班级\t语文\t数学\t英语\t总分\t名次\n");
    while(head){
	rank++;
	fprintf(fp,"%d\t%s\t%s\t%.2f\t%.2f\t%.2f\t%.2f\t%d\n",
	head->value.num,head->value.name,head->value.clas1,
	head->value.chinese,head->value.math,head->value.english,
	head->value.total,rank);
	head=head->next;
    }
    printf("\033[\t\t\t\tC");
    printf("数据保存成功\n");
    sleep(1);
    fclose(fp);
}

//函数12,从文件读数据，并创链表(学生)
ST *read_link(){
    FILE *fp=fopen("student.txt","r+");
    if(fp){
	printf("\033[\t\t\t\tC");
	printf("文件存在，打开成功\n");
	Data d1={0};
	int ret=0;
	ST *head=NULL;
	while(1){
	    ret=fscanf(fp,"%s %s %d %s %d %s %f %f %f %f %s %s %s",
		d1.number,d1.key,&d1.num,d1.name,
		&d1.age,d1.sex,
		&d1.chinese,&d1.math,&d1.english,&d1.total,
		d1.clas1,d1.job,d1.clas2);
	    if(ret==13)
		head=create_link(head,d1);
	    else
		break;
	}
	fclose(fp);
	return head;
    }
    else{
	printf("\033[\t\t\t\tC");
	printf("文件不存在\n");
	return	NULL;
    }
}

//函数13,数据把握存到文件(教师)
void save1_link(ST1 *head){
    FILE *fp=fopen("teacher.txt","w");
    while(head){
	fprintf(fp,"%s %s %d %s %d %s %s %s %s\n",
	head->value1.number,head->value1.key,head->value1.num,
	head->value1.name,head->value1.age,head->value1.sex,head->value1.job,
	head->value1.clas,head->value1.course);
	head=head->next;
    }
    printf("\033[\t\t\t\tC");
    printf("数据保存成功\n");
    sleep(1);
    fclose(fp);
}

//函数14,从文件读数据，并创链表(教师)
ST1 *read1_link(){
    FILE *fp=fopen("teacher.txt","r");
    if(fp){
	printf("\033[\t\t\t\tC");
	printf("文件存在，打开成功\n");
	Data1 d1={0};
	int ret=0;
	ST1 *head=NULL;
	while(1){
	    ret=fscanf(fp,"%s %s %d %s %d %s %s %s %s",
		d1.number,d1.key,&d1.num,d1.name,
		&d1.age,d1.sex,d1.job,d1.clas,d1.course);
	    if(ret==9)
		head=create2_link(head,d1);
	    else
		break;
	}
	fclose(fp);
	return head;
    }
    else{
	printf("\033[\t\t\t\tC");
	printf("文件不存在\n");
	return NULL;
    }
}

