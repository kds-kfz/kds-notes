#include"stdio.h"
#include"stdlib.h"
//1.c
//单链表 学习1
struct st{
    int num;
    struct st *next;
};

struct st *data(){
    /*
    struct st *p=(struct st *)malloc(sizeof(struct st)*3);
    p[0].num=1001;
    p[1].num=1002;
    p[2].num=1003;
    p[0].next=&p[1];
    p[1].next=&p[2];
    p[2].next=NULL;
    return p;
    */
    struct st *p1=(struct st *)malloc(sizeof(struct st));
    struct st *p2=(struct st *)malloc(sizeof(struct st));
    struct st *p3=(struct st *)malloc(sizeof(struct st));
    p1->num=1004;
    p2->num=1005;
    p3->num=1006;
    p1->next=p2;
    p2->next=p3;
    p3->next=NULL;
    return p1;
}

int main(){
    struct st *p=data();
    struct st *p1=p,*p2=p1;//需三个指针保存首地址
    int i=0;
    while(p){
	printf("num=%d\n",p->num);
	p=p->next;
    }
    /*
    free(p1);//申请1片连续的内存，保存三个结构体数据，需释放3次
    for(;i<3;i++){
	printf("num=%d\n",p1->num);//只释放1次，则后两数据不变
	p1=p1->next;
    }
    */
    i=0;
    for(;i<3;i++){//申请的3个结构体起始地址不一样，需释放3次
	free(p1);
	p1=p1->next;
    }
    i=0;
    while(i<3)
    {
	printf("num=%d\n",p2->num);//打印这三个结构体的数据
	p2=p2->next;
	i++;
    }
    return 0;
}
