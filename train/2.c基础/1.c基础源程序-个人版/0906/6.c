#include"stdio.h"
//6.c
//用指针输入，输出数组
int main(){
int n;
int i,j;
printf("input n bit :\n");
int arr[n];
int *p=arr;
for(i=0;i<5;i++)
    scanf("%d",p+i);
//取地址输出
for(i=0;i<5;i++)
    printf("%d ",*(p+i));
printf("\n");
//改变地址输出
while(p<arr+5){
    printf("%d ",*p);
    p++;}
printf("\n");
//改变地址输出
for(p=arr;p<arr+5;p++)
    printf("%d ",*p);
printf("\n");
return 0;
}

