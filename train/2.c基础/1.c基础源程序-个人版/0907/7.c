#include"stdio.h"
//7.c
//一维指针数组 与 一维数组指针
int main(){
int arr[5]={1,2,3,4,5};

int *p1[5]={&arr[0],&arr[1],&arr[2],&arr[3],&arr[4]};
//指针数组，保存指针对应数组元素的地址

int (*p2)[5]=&arr;
//数组指针，指向arr数组里连续的20个字节

printf("*p1=%ld\n",sizeof(*p1));//*p1指针本身长度8，可以是不连续的
printf("*p1+1=%ld\n",sizeof(*p1+1));//arr[1]指针地址长度为8
printf("p1=%ld\n",sizeof(p1));//p1保存了5个长度为8的指针地址
printf("p1+1=%ld\n",sizeof(p1+1));//p1指针指向下个地址，相差8

printf("\n");

printf("*p2=%ld\n",sizeof(*p2));//*p2指向arr数组连续20个长度的地址
printf("*p2+1=%ld\n",sizeof(*p2+1));//指针指向下个元素地址，本身指针是长度8
printf("p2=%ld\n",sizeof(p2));//p2指针本身长度8
printf("p2+1=%ld\n",sizeof(p2+1));//p2指针本身指向下个地址，相差8

printf("\n");

printf("*p1=%p\n",*p1);//*p1指向arr首地址，即arr[0]地址
printf("*p1+1=%p\n",*p1+1);//*p1+1指向arr[1]地址，arr[0]与arr[1]相差4字节，地址相差4
printf("p1=%p\n",p1);//p1指针本身地址
printf("p1+1=%p\n",p1+1);//p1指针本身指向下个地址，相差8

printf("\n");

printf("*p2=%p\n",*p2);//*p2指向arr数组连续20个长度地址的首地址，即arr[0]地址
printf("*p2+1=%p\n",*p2+1);//*p2+1指向arr[1]地址，arr[0]与arr[1]相差4字节，地址相差4
printf("p2=%p\n",p2);//p2指针指向arr数组连续的20个长度地址的首地址
printf("p2+1=%p\n",p2+1);//p2指针指向下个连续的20个长度地址的首地址

return 0;
}
