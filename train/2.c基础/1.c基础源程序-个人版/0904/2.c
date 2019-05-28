#include"stdio.h"
//2.c

int main(){

int arr[5]={1,2,3,4,5};
int i;
int a=10;

int *p=&arr[0];
for(i=0;i<5;i++){
printf("%p\n",&arr[i]);
printf("%d\n",*p++);
printf("%d\n",arr[i]);
}
printf("%p\n",&a);
printf("%d\n",*&a);
return 0;
}
