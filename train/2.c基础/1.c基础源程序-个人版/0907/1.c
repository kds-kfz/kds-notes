#include"stdio.h"
//1.c
int main(){

char ch[5]={'a','b','c','d','e'};
char (*p1)[5]=&ch;
char *p2="abcde";
int i;


for(i=0;i<5;i++)
    printf("%c ",**p1+i);


return 0;
}

