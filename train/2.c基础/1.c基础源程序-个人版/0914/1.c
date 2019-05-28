#include"stdio.h"
#include<string.h>
//1.c
//结构体指针
struct st{
    int num;
    char name[20];
    int age;
};

struct te{
    int num;
    char name[20];
    int age;
    struct st value;
};

int main(){
//定义结构体指针，指向结构体变量首地址
    struct st s1,*p=&s1;

//变量访问结构体成员只有1种方式
    s1.num=1001;

//指针访问结构体成员只有2种方式    
    strcpy((*p).name,"lisi");
//    strcpy(p->name,"liqi");
    p->age=24;
    
//    printf("s1:%d %s %d\n",p->num,p->name,p->age);
    printf("s1:%d %s %d\n",(*p).num,(*p).name,(*p).age);
 
    struct te t1={1701,"wang",34,s1};
    struct te *p1=&t1;

//    printf("t1:%d %s %d , %d %s %d\n",(*p1).num,(*p1).name,
//	    (*p1).age,(*p1).value.num,(*p1).value.name,(*p1).value.age);

    printf("t1:%d %s %d , %d %s %d\n",p1->num,p1->name,
	    p1->age,p1->value.num,p1->value.name,p1->value.age);
    

    return 0;
}

