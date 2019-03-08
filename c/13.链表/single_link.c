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
    while(head){
        if(head->value.num == num)
            return head;
        else
            head = head->next;
    }
    return head;
}

ST *delete_link(ST *head, int num){
    ST *obj = search_link(head, num);
    if(!obj) return obj;
    else if(obj == head){
        head = obj->next;
    }else{
        ST *pre = head;
        while(pre->next != obj)
            pre = pre->next;
        pre->next = obj->next;
    }
    free(obj);
    return head;
}

ST *insert_1_link(ST *head, Data d1, int num){
    ST *p = (ST *)malloc(sizeof(ST));
    p->value = d1;
    p->next = NULL;
    ST *obj = search_link(head, num);
    if(obj == head){
        p->next = head;
        head = p;
    }else if(obj){
        ST *pre = head;
        while(pre->next != obj)
            pre = pre->next;
        pre->next = p;
        p->next = obj;
    }
    return head;   
}

ST *insert_2_link(ST *head, Data d1, int num){
    ST *p = (ST *)malloc(sizeof(ST));
    p->value = d1;
    p->next = NULL;
    ST *obj = search_link(head, num);
    if(!obj) 
        return head;
    else if(obj->next == NULL){
        obj->next = p;
    }else if(obj == head){
        p->next = head;
        head = p;
    }else{
        p->next = obj->next;//注意先要连接后面的节点
        obj->next = p;
    }
    return head;   
}

ST *insert_sort_link(ST *head){
    ST *new = head;
    head = head->next;
    new->next = NULL;
    while(head){
        ST *n = head;
        head=head->next;
        ST *p = new;
        ST *t = NULL;
        while(p && n->value.score > p->value.score){
            t = p;
            p = p->next;
        }
        if(!t){
            n->next = new;
            new = n;
        }else{
            n->next = t->next;
            t->next = n;
        }
    }
    return new;
}

ST *select_sort_link(ST *head){
    ST *new = NULL;
    while(head){
        ST *pmax = head;
        ST *p = NULL;
        ST *t = NULL;//保存当前指针
        for(p = head; p->next != NULL; p = p->next){
            if(p->next->value.num > pmax->value.num){
                pmax = p->next;
                t = p;
            }
        }
        if(!t)
            head = head->next;
        else
            t->next = pmax->next;
        pmax->next = new;
        new = pmax;
    }
    return new;
}

ST *revers_link(ST *head){
    ST *t = NULL;
    ST *n = NULL;
    while(head){
        n = head->next;
        head->next = t;
        t = head;
        head = n;
    }
    return t;
}
int getlen_link(ST *head){
    int count = 0 ;
    for(; head != NULL; count++, head = head->next);
    return count;
}

void output_link(ST *head){
    if(!head) return ;
    putchar(10);
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

