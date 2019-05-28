#ifndef _LINK_H
#define _LINK_H
//link.h
//链表数据声明，函数声明
typedef struct{
    int num;
    char name[20];
    char sex;
    int age;
}Data;

typedef struct s{
    Data value;
    struct s *next;
}ST;

extern ST *create_link(ST *head,Data d1);//尾插入
extern ST *create1_link(ST *head,Data d1);//头插入

extern ST *search_link(ST *head,int num);//查找
extern ST *del_link(ST *head,int num);//删除
extern ST *insert1_link(ST *head,Data d1,int num);//前插入(方法1，个人版)
extern ST *insert2_link(ST *head,Data d1,int num);//前插入(方法2，教师版)
extern ST *insert_link(ST *head,Data d1,int num);//后插入

extern ST *select_sort_link(ST *head);//选择排序
extern ST *insert_sort_link(ST *head);//插入排序

extern ST *reverse_link(ST *head);//链表倒置
extern int len_link(ST *head);//长度

extern ST *exchange_link(ST *head,int d1,int d2);//链表数据交换

extern void printf_link(ST *head);//打印单链表
extern void free_link(ST *head);//释放单链表

#endif
