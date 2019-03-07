#include<malloc.h>
#include"single_link.h"

ST *create_link(ST *head, Data d1){
    ST *p = (ST *)malloc(sizeof(ST));
    p->value = d1;
    p->next = NULL;
    if(!head) return p;
    ST *t = head;
    while(t->next)
        t = t->next;
    t->next = p;
    return head;
}

ST *search_link(ST *head, int num){
    
}

ST *delete_link(ST *head, Data d1){

}

ST *insert_1_link(ST *head, Data d1){

}

ST *insert_2_link(ST *head, Data d1){

}

ST *insert_sort_link(ST *head, Data d1){

}

ST *select_sort_link(ST *head, Data d1){

}

ST *revers_link(ST *head, Data d1){

}

int getlen_link(ST *head, Data d1){

}

void output_link(ST *head){
    if(!head) return ;
    while(head){
        printf("num = %d, name = %s, age = %d, score = %.3lf, sex = %c\n",
                head->value.num, head->value.name, head->value.age, 
                head->value.score, head->value.SEX.sex);
        head = head->next;
    }
}

bool free_link(ST *head){
    if(!head) return false;
    while(head){
        ST *t = head;
        free(t);
        head = head->next;
    }
    return true;
}

