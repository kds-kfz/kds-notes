#include"stdio.h"
//输入一个数判断是否是素数
//5.c
int a,a1;
int i;
int b,b1,b2,b3,b4,j;
int main(){
/*
while(1){

printf("please input a number:\n");
i=2;
while((scanf("%d",&a))==0){printf("input error\n");getchar();}
do{
    if(a%i==0&&i<a){a1=0;break;}
    else i++;
    if(i==a){a1=1;break;}
}while(1);

if(a1)printf("%d is prime number!\n",a);
else printf("%d is not prime number!\n",a);
}
*/
//输出1-99同构数
printf("1-99 which is isomorph\n");
for(j=2;j<1000;j++)
{
//    if(j==(j*j)%(j<10?10:100))
//	printf("%d ",j);
  b1=j*j%10;
  b2=j*j%100;
  b3=j*j%1000;
  if(b3>0&&j==b3)printf("%d ",j);
    if(b2>0&&j==b2)printf("%d ",j);
	if(b1>0&&j==b1)printf("%d ",j);

}
printf("is isomorph\n");

return 0;
}
