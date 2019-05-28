#include"stdio.h"
//输入a，b两个数交换
int main(){
int a,b,c;
printf("input two number\n");
scanf("%d%d",&a,&b);
printf("a=%d,b=%d\n",a,b);
c=a;
a=b;
b=c;
printf("a=%d,b=%d\n",a,b);
return 0;
}

//输入年与日，求该天是本年到第几天
int main3(){
int a,b,c;
int i,day=0;

printf("year:");
scanf("%d",&a);
printf("%d\n",a);
printf("month:");
scanf("%d",&b);
printf("%d\n",b);
printf("day:");
scanf("%d",&c);
printf("%d\n",c);

int arr[]={31,0,31,30,31,30,31,31,30,31,30,31};
if(a%4==0&&a%100!=0||a%400==0)
{printf("%d is leap year\n",a);arr[1]=29;}
else
{printf("%d is common year\n",a);arr[1]=28;}

for(i=0;i<b-1;i++)
    day+=arr[i];
printf("day is %dth\n",day+c);
return 0;
}

//定义数组，求最大值，最小值，与其下标
int main2(){
int max,min,n1,n2,i,n;
printf("input arr[n]\n");
scanf("%d",&n);
int arr[n];
max=arr[0];
min=arr[0];
for(i=0;i<n;i++)
    scanf("%d",&arr[i]);
for(i=0;i<n;i++)
    if(max<arr[i]){max=arr[i];n1=i;}
for(i=0;i<n;i++)
    if(min>arr[i]){min=arr[i];n2=i;}
for(i=0;i<n;i++)
    printf("%d ",arr[i]);
printf("\n");
printf("max[%d]=%d,min[%d]=%d\n",n1,max,n2,min);
return 0;
}

//买东西，买几件，每件多少钱，求总价钱
int main1(){
int n,i,sum=0; 
printf("input number arr[n]\n");
scanf("%d",&n);
int arr[n];
for(i=0;i<n;i++){
    scanf("%d",&arr[i]);
    sum+=arr[i];}
printf("sum=%d\n",sum);
return 0;
}

