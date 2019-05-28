#include"stdio.h"
//#3.c
//1,输入天数，判断是第几周第几天
//2,输入圆柱体到地面半径和高，求体积
//3,某水池，注水10,出水5,水池100,求时间
//4,输入3个数，求最小值和最大值

int a1,a2;
unsigned int day;
float b1,b2;
double b3;
int c1=10,c2=5;
float c3;
int d1,d2,d3,d4,d5;

int main(){
printf("title:1**********\n");

printf("please input a day:\n");
scanf("%d",&day);
a1=day/7+1;
a2=day%7;
printf("week=%d,day=%d\n",a1,a2);

printf("title:2**********\n");
printf("input two number:\r\n");
scanf("%f %f",&b1,&b2);
b3=3.14*b1*b1*b2;
printf("radius=%f,hide=%f,area=%.4lf\n",b1,b2,b3);

printf("title:3**********\n");
c3=(float)100/(c1-c2);
printf("c1=10,c2=5,h=%.2f\n",c3);

printf("title:4**********\n");
printf("please input three number:\n");
scanf("%d %d %d",&d1,&d2,&d3);

d4=d1>d2?d1:d2;
d4=d4>d3?d4:d3;

d5=d1<d2?d1:d2;
d5=d5<d3?d5:d3;

printf("max=%d,min=%d\n",d4,d5);
return 0;
}


