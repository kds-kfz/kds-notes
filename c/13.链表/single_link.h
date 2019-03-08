#ifndef __SINGLE_LINK_H__
#define __SINGLE_LINK_H__
#include<stdio.h>
#include<stdbool.h>

typedef union {
    char sex;
}Sex;

typedef struct{
    Sex SEX;
    double score;
    char name[20];
    unsigned int  age;
    unsigned int  num;
}Data;

typedef struct s{
    Data value;
    struct s *next;
}ST;

ST *create_link(ST *head, Data d1);
ST *search_link(ST *head, int num);
ST *delete_link(ST *head, int num);
ST *insert_1_link(ST *head, Data d1, int num);//头插入
ST *insert_2_link(ST *head, Data d1, int num);//尾插入
ST *insert_sort_link(ST *head);
ST *select_sort_link(ST *head);
ST *revers_link(ST *head);
int getlen_link(ST *head);
void output_link(ST *head);
bool free_link(ST *head);


#endif
