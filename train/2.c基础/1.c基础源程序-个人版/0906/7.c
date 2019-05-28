#include"stdio.h"
//7.c
//数组指针
int main(){

int arr[5]={0};
int (*p)[5]=&arr;
int i;
for(i=0;i<5;i++)
    printf("%d ",*(*p+i));


return 0;
}
