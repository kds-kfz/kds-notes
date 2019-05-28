#include"stdio.h"
//3.c

int a=0,i=0;
int b1=1,b2,b3;
char str[20];
int j=0,c=0;

int d=0,d1=0;
int e=100,e1,e2,e3,e4;

int f,f1=2;
char ch;
int ch1=0;

int n,p=2;

int main(){

printf("title:1**********\n");
//1-100的偶数和
while(i<=100){
    a=a+i;
    i+=2;
}
printf("sum=%d\n",a);

printf("title:2**********\n");
//99乘法表
while(b1<10){
    b2=1;
    while(b2<=b1){
    b3=b1*b2;
    printf("%d*%d=%d ",b1,b2,b3);
    b2++;
    }
    printf("\n");
    b1++;
}

printf("title:3**********\n");
//输入字符，统计字符个数
/*
printf("please input n character:\n");
//scanf("%s",str);
gets(str);
while(j<sizeof(str)){
if(str[j]!='\0')c++;
    j++;
}
printf("n=%d\n",c);

*/


while(1)
{
ch=getchar();
if(ch=='\n')break;
ch1++;
}
printf("n=%d\n",ch1);

printf("title:4**********\n");
//计算1-100,个位数是5的不加
while(d1<100){
d1++;
if(d1%10==5)continue;
d=d+d1;
}
printf("d=%d\n",d);

printf("title:5**********\n");
//1-1000,水仙花数，打印abc,a*a*a+b*b*b+c*c*c=abc
while(e<1000){
e1=e/100;
e2=e/10%10;
e3=e%10;
e4=(e==e1*e1*e1+e2*e2*e2+e3*e3*e3);

if(e4)
    printf("%d ",e);
e++;
}
printf("\n");

printf("title:6**********\n");
//因式分解
printf("please input a number:\n");
/*
scanf("%d",&f);
while(f1<=f){

if(f%f1==0){printf("%d*",f1);f=f/f1;f1=1;}
f1++;
}
printf("%d ",f);
printf("\n");
*/

scanf("%d",&n);
while(p*p<=n){
if(n%p==0){
printf("%d*",p);
n/=p;
}
else p++;
}
printf("%d",n);
printf("\n");

return 0;
}


