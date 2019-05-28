#ifndef _LINK_H
#define _LINK_H
//link.h
//环形链表数据声明，函数声明
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
extern ST *insert_link(ST *head,Data d1,int num);//前插入
extern ST *insert1_link(ST *head,Data d1,int num);//后插入

extern void printf_link(ST *head);//打印环形链表
extern void free_link(ST *head);//释放环形链表
#endif
