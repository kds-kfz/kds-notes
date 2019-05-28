#include"stdio.h"
//8.c
//二维指针数组 与 二维数组指针
int main(){
int arr[3][4]={
1,2,3,2,
4,5,6,5,
7,8,9,8
};
int *p1[3]={arr[0],arr[1],arr[2]};
//指针数组

int (*p2)[4]=arr;
//数组指针

printf("*p1=%ld\n",sizeof(*p1));//*p1指针本身长度8,可以是不连续的，指向arr[0]地址
printf("*p1+1=%ld\n",sizeof(*p1+1));//*p1+1指向arr[0]地址的下个地址，即arr[0][1]
printf("p1=%ld\n",sizeof(p1));//p1保存了3个长度为8的指针地址
printf("p1+1=%ld\n",sizeof(p1+1));//p1指针指向下个地址，相差8

printf("\n");

printf("*p2=%ld\n",sizeof(*p2));//*p2指向二维连续的16个长度地址到首地址，即arr[0][0]
printf("*p2+1=%ld\n",sizeof(*p2+1));//指针指向下个元素到地址，即arr[0][1]，本身长度8
printf("p2=%ld\n",sizeof(p2));//p2指针本身长度8
printf("p2+1=%ld\n",sizeof(p2+2));//指针本身指向下个地址，相差8

printf("\n");


printf("*p1=%p\n",*p1);//*p1指向arr[0][0]的地址
printf("*p1+1=%p\n",*p1+1);//*p2+1指针arr[0][1]地址,arr[0][0]与arr[0][1]相差4,地址相差4
printf("p1=%p\n",p1);//p1指针本身地址
printf("p1+1=%p\n",p1+1);//p1本身地址指向下个地址，相差8

printf("\n");

printf("*p2=%p\n",*p2);//*p2指向二维连续16个长度地址的首地址，即arr[0][0]
printf("*p2+1=%p\n",*p2+1);//*p2+1指向arr[0][1]地址,arr[0][0]与arr[0][1]相差4,地址相差4
printf("p2=%p\n",p2);//p2指针指向二维连续的16个长度地址到首地址
printf("p2+1=%p\n",p2+1);//p2指针指向下一个连续的20个长度地址的首地址
return 0;
}
