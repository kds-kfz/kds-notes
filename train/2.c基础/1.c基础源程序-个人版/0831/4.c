#include"stdio.h"
//4.c
int main(){
/*
//fflush(stdin);
int i;
int i=0;
fflush(stdin);
do{
printf("*\n");
i++;
}while(i<5);
printf("%d\n",i);
*/

int i=1,j,num;

int b1,b2,b3;

int c1,c2,c3;

int d1,d2,d3;

int e1,e2,e3,e4;

int f=0,f1=0,f2=0,sum=0;

double ave=0;

int g,g1;
float gsum=0;

/*
     *
    **
   ***
  ****
*/
printf("tilte:1**********\n");
do{
    j=5-i;
    num=i;
    do{
    printf(" ");
    j--;
    }while(j>0);
    do{
    printf("*");
    num--;
    }while(num>0);
    printf("\n");
    i++;
}while(i<5);

/*
   ****
    ***
     **
      *
*/
printf("tilte:2**********\n");
for(b1=0;b1<4;b1++){
    for(b2=4;b2-b1>0;b2--)
	printf("*");
    for(b3=b1;b3>0;b3--)
	printf(" ");
    printf("\n");
}

/*
     *
    ***
   *****
    ***
     *
*/

printf("tilte:3**********\n");
c1=1;
while(c1<=5){
    c3=5-1;
    c2=2*c1-1;
    if(c1<4){
    while(c3>c1){
    printf(" ");
    c3--;
    }
    while(c2>0){
    printf("*");
    c2--;
    }
    printf("\n");
    }
    else if(c1==4)
    {
	for(c2=c1-2;c2>0;c2--)
	    printf(" ");
	for(c3=5-c1+2;c3>0;c3--)
	    printf("*");
	printf("\n");
    }
    else
    {
    for(c2=0;c2<3;c2++)
	printf(" ");
    printf("*");
    printf("\n");
    }
    c1++;
}

//输出100-200素数

printf("tilte:4**********\n");
for(d1=100;d1<=200;d1++){
    for(d2=2;d2<d1;d2++)
	if(d1%d2==0){d3=0;break;}
	else d3=1;
	if(d3==1)printf("%d ",d1);
}
printf("\n");

//3个人去吃饭，共花50块，男人3块，女人2块，小孩1块

printf("tilte:5**********\n");
/*
for(e1=0;e1<=50;e1++)
    for(e2=0;e2<=25;e2++)
	for(e3=0;e3<=16;e3++)
	if(50==e1+2*e2+3*e3){
	    e4++;printf("50=1*%d+2*%d+3*%d\n",e1,e2,e3);
	    printf("man:%d,woman:%d,child:%d\n",e3,e2,e1);
	}
printf("%d\n",e4);
*/

//30个人去吃饭，共花50块，男人3块，女人2块，小孩1块，求有多少种组合

for(e1=0;e1<=16;e1++)
    for(e2=1;e2<=25;e2++)
	if(50==e1*3+e2*2+(30-e1-e2)){
	    e4++;
	    printf("man:%d,woman:%d,child:%d\n",e1,e2,30-e1-e2);}
printf("%d\n",e4);

//输入数字，输入0时结束，输出平均数;选择是否继续

printf("tilte:6**********\n");

while(1){

while((scanf("%d",&f))==0){
printf("input error\n");
printf("please input again\n");
}
f2++;
sum=sum+f;
if(f==0){
f2--;
printf("if you continue?(1/0)\n");
scanf("%d",&f1);
if(f1==0)break;
else if(f1==1)continue;}
}
if(f2>1)
ave=(double)(sum)/(f2-1);
if(f2==1)ave=(double)sum/f2;
printf("ave=%.4lf=%d/%d\n",ave,sum,f2);

//输入n，计算1+1/(1+2)+1/(1+2+3)+1/(1+2+3+...+n)的值

printf("tilte:7**********\n");
printf("please input a number:\n");
scanf("%d",&g);
if(g==1){gsum=1;printf("1/gsum=%.6f\n",1/gsum);}
else
{
for(g1=1;g1<=g;g1++)
    gsum=gsum+g1;
printf("1+1/gsum=%.6f\n",1+1/gsum);
}




return 0;

}

