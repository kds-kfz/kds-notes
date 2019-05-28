#ifndef _LINK_H
#define _LINK_H  1
typedef struct {
    int num;
    char name[20];
}Data;
typedef struct st{
    Data value;
    struct st* next;
}ST;
extern ST* creat_linkback(ST* h,Data da);
extern ST* creat_linkfront(ST* h,Data da);
extern ST* search_link(ST* h,int num);
extern ST* del_link(ST* h,int num);
extern ST* insert_link(ST* h,Data d1,int num);
extern void print_link(ST* h);
extern void free_link(ST* h);
#endif
