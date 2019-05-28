#ifndef _LINK_H_
#define _LINK_H_
//单链表
#include<iostream>
using namespace std;

struct Stu{
    int id;
    string name;
    int score;
    Stu *next;
};

//单链表尾创建
extern Stu *creat_link_beh(Stu *head,Stu d1);
//单链表头创建
extern Stu *creat_link_pre(Stu *head,Stu d1);
//单链表打印
extern void print_link(Stu *head);
//单链表查找
extern Stu *seek_link(Stu *head,int id);
//单链表头插入
extern Stu *insert_pre(Stu *head,Stu d1,int id);
//单链表尾插入
extern Stu *insert_beh(Stu *head,Stu d1,int id);
//单链表删除
extern Stu *del_link(Stu *head,int id);
//单链表倒置
extern Stu *reverse_link(Stu *head);
//单链表释放
extern void delete_link(Stu *head);
//单链表选择排序
extern Stu *select_sort_link(Stu *head);
//单链表插入排序
extern Stu *insert_sort_link(Stu *head);
//string转换称int，只转换string里的字符数字
extern int string_int(string str);

//修改链表里某个数据函数
//extern bool *insert_sort_link(Stu *head);

#endif

