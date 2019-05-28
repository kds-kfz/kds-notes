#include"stdio.h"
//4.c

//输出200-299,个十百和为12,积为42的值
int main(){
int i,j,k,m,l=0;
int *p1=&i,*p2=&j,*p3=&k,*p4=&m,*p5=&l;
for(*p1=1;*p1<=99;(*p1)++){
    *p2=*p1 / 10%10;
    *p3=*p1%10;
    if(42/2==*p2**p3&&12-2==*p2+*p3){
	*p4=200+*p1;
	printf("*p1=%d ",*p4);
    }
    }
printf("\n");
return 0;
}

//输出200-299,个十百和为12,积为42的值
int main4(){
int i,j,k,m;
int *p1=&i,*p2=&j,*p3=&k,*p4=&m;
for(*p1=100;*p1<300;(*p1)++){
*p2=*p1/100;
*p3=*p1/10%10;
*p4=*p1%10;
if((*p2+*p3+*p4==12)&&(*p2**p3**p4==42))
    printf("%d ",*p1);}
printf("\n");
return 0;
}

//输出1-3+5-7+9
int main3(){
int i,j=0,k=1;
int *p1=&i,*p2=&j,*p3=&k;
for(*p1=1;*p1<=9;(*p1)+=2){
    *p2+=*p1**p3;
    *p3*=(-1);}
printf("*p2=%d\n",*p2);
return 0;
}

//输出1+1/3+1/5+1/7+1/9
int main2(){
int i;
double n=0;
int *p1=&i;
double *p2=&n;

for(*p1=1;*p1<=9;(*p1)+=2)
    *p2=*p2 + 1.0 / *p1;
printf("*p2=%.4lf\n",*p2);

return 0;
}

//输出2000-2888年所有闰年，每隔5个换行
int main1(){
int i,j=0;
int *p1=&i,*p2=&j;
for(*p1=2000;*p1<=2888;(*p1)++)
    if(*p1%4==0&&*p1%100==100||*p1%400!=0){
    *p2+=1;
    if(*p2%5==0)printf("\n");
    printf("%d ",*p1);
    }
printf("\n");
return 0;
}
