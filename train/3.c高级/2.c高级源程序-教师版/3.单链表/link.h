#ifndef _LINK_H
#define _LINK_H 1
typedef struct {
    int num;
    char name[20];
    char sex;
}Data;

typedef struct st{
    Data value;
    struct st* next;
}ST;

extern ST* creat_linkback(ST* h,Data da);//尾插入
extern ST* creat_linkfront(ST* h,Data da);//头插入
extern ST* search_link(ST* h,int num);//查找
extern ST* del_link(ST* h,int num);//删除
extern ST* insert_link(ST* h,Data d1,int num);//插入
extern ST* select_link(ST* h);//选择排序
extern ST* reverst_link(ST* h);//倒置
extern int len_link(ST* h);//表长
extern ST* merge_link(ST* h1,ST* h2);//合并链表
extern void print_link(ST* h);//打印
extern void free_link(ST* h);//释放
#endif
