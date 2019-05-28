#include"stdio.h"
//6.c
//5
/*
 输入n，输出1+1/(1+2)+1/(1+2+3)+...+1/(1+2+3+...+n)
 */
int main(){
int i,num,sum=0;
float sum1=0;
printf("please input a number:\n");
scanf("%d",&num);
for(i=1;i<=num;i++){
sum+=i;
sum1+=(float)1/sum;
}
printf("sum1=%.2f\n",sum1);
return 0;
}

//4
//100-200,输出素数
int main4(){
    int i,j;
    for(i=100;i<=200;i++){
	for(j=2;i%j!=0;j++);
    if(i==j)printf("%d ",i);
    }
printf("\n");
return 0;
}
//3
/*
    *
   ***
  *****
   ***
    *
 */
int main3(){
int i,j,k;
for(i=0;i<3;i++){
    for(j=0;j<=2-i;j++)
    printf(" ");
    for(k=0;k<2*i+1;k++)
    printf("*");
    printf("\n");
    }

for(i=1;i<=2;i++){
    for(j=0;j<=i;j++)
    printf(" ");
    for(k=1;k<=5-2*i;k++)
    printf("*");
    printf("\n");
}
return 0;
}

//2
/*
 ****
  ***
   **
    *
 */
int main2(){
int i,j,k;
for(i=0;i<4;i++){
    for(j=0;j<i;j++)
    printf(" ");
    for(k=0;k<4-i;k++)
    printf("*");
    printf("\n");
}
return 0;
}

//1
/*
     *
    **
   ***
  ****
*/
int main1(){
int i,j,k;
for(i=0;i<=4;i++){
    for(k=1;k<=4-i;k++)
    printf(" ");
    for(j=1;j<=i;j++)
    printf("*");
    printf("\n");

}
return 0;
}

