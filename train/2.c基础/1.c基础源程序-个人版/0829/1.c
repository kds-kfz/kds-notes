#include"stdio.h"
//#1.c
int a,b;
int sum1,sum2,sum3;
float sum4;
float num1;
int main()
{
printf("please two number:\n");
scanf("%d %d",&a,&b);
sum1=a+b;
sum2=a-b;
sum3=(float)a*b;
sum4=(float)a/b;//运算中有大则结果转大,区分数学运算和计算机运算
num1=sum1;//大转小

printf("%.2f=a+b\n",num1);//正常转换
printf("%.2f=a-b\n",(float)sum2);//强制
printf("%.2f=a*b\n",sum3*1.0);//正常
printf("%.2f=a/b\n",sum4);//正常

return 0;
}













