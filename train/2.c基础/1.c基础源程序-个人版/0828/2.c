#include"stdio.h"
//文件2.c
//输出火车票到票务信息

int t1=12,t2=13;
long int a=1001,b=1002;
char str1[]="shenzhen",str2[]="shanghai";
char c;
int d;
float e=19.342,g=24.667;
double f=998.88895;
char s1='A',s2;

int main(){

printf("车次\t\t起点\t\t终点\t\t全程时间\n");
printf("%ld\t\t%s\t%s\t%d\n",a,str1,str2,t1);
printf("%ld\t\t%s\t%s\t%d\n",b,str2,str1,t2);
printf("求一个字符到ascii\n");
s2=s1+32;
printf("%c\n",s2);
printf("%d\n",s1);

printf("电脑                外设                座椅\n");
printf("%lf%15.2f%21.3f\n",f,g,e);
return 0;
}







