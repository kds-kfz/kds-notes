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
ST *delete_link(ST *head, Data d1);
ST *insert_1_link(ST *head, Data d1);
ST *insert_2_link(ST *head, Data d1);
ST *insert_sort_link(ST *head, Data d1);
ST *select_sort_link(ST *head, Data d1);
ST *revers_link(ST *head, Data d1);
int getlen_link(ST *head, Data d1);
void output_link(ST *head);
bool free_link(ST *head);


#endif
