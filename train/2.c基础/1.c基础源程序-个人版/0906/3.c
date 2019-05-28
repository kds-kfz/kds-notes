#include"stdio.h"
//3.c
//用指针求平均数
int main(){
int arr[10]={1,2,3,4,5,6,7,8,9,0};
int *p1,*p2,*p3;
int i,n=0,num=0;
p2=&n;
p3=&num;
for(i=0;i<10;i++){
p1=&arr[i];
*p3+=*p1;
*p2+=1;
}
printf("%.4f = %d / %d\n",(float)*p3 / *p2,*p3,*p2);
return 0;
}

//输入两个数，输出最大值
int main2(){
/*
int a,b,max;
int *p1=&a,*p2=&b;
scanf("%d%d",p1,p2);
if(*p1>*p2)max=*p1;
else max=*p2;
printf("a=%d,b=%d,max=%d\n",*p1,*p2,max);
*/
/*
int a,b,max;
int *p1=&a,*p2=&b,*p3=&max;
scanf("%d%d",p1,p2);
*p3=*p1>*p2?*p1:*p2;
printf("a=%d,b=%d,max=%d\n",*p1,*p2,*p3);
*/
/*
int a,b;
int *p1=&a,*p2=&b,*p3=NULL;
scanf("%d%d",p1,p2);
if(*p1>*p2)p3=p1;
else p3=p2;
printf("a=%d,b=%d,max=%d\n",a,b,*p3);
*/
int a,b,max;
int *p1=&a,*p2=&b,*p3=NULL;
scanf("%d%d",p1,p2);
if(*p1>*p2)max=*p1;
else max=*p2;
printf("a=%d,b=%d,max=%d\n",*p1,*p2,max);
return 0;
}

//输入圆柱体r,h,输出圆柱体积
int main1(){
float a,b;
float *p1=&a,*p2=&b;
printf("input two number:\n");
scanf("%f%f",&a,&b);
printf("r=%5f,h=%5f,s=%lf\n",a,b,3.14**p1**p1**p2);
return 0;
}




