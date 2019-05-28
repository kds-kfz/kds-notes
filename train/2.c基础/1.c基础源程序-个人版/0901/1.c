#include"stdio.h"
//1.c
int main(){

void fun(int *p1,int *p2);
void fun1(int *p1,int *p2);
int a,b;
int *a1,*a2;
a1=&a;
a2=&b;
scanf("%d%d",&a,&b);
printf("a=%d,b=%d\n",a,b);
fun(a1,a2);
printf("a=%d,b=%d\n",*a1,*a2);
printf("a=%d,b=%d\n",a,b);
return 0;
}

void fun(int *p1,int *p2){
int temp;
temp=*p1;//p1指针变量指向地址，*p1是p1指针变量指向的值
*p1=*p2;//
*p2=temp;//
}

void fun1(int *p1,int *p2){
int *temp;
temp=p1;
p1=p2;//
p2=temp;//
}

