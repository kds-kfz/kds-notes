#include"stdio.h"
//3.c
//1,判断奇偶
//2,判断一个数是否同时被357整除，能被两个整除，能被其中一个整除
//3,输入三个数，求最大值
//4,判断元音字母
//5,输入天数，判断是第几周第几天
//6,输入三个数判断关系


int a;
int b,b1,b2,b3;
int c,c1,c2,c3;
int day,d1,d2;
int i,j,k,e,num[10];
char ch,ch1;

int main(){

/*    
printf("title:1**********\n");
printf("please input a number:\n");
scanf("%d",&a);
if(a%2==0)
    printf("%d is even number\n",a);
else 
    printf("%d is odd number\n",a);

printf("title:2**********\n");
printf("please input a number:\n");
scanf("%d",&b);
b1=b%3;
b2=b%5;
b3=b%7;
if(b1==0&&b2==0&&b3==0){
    printf("%d can be 3 5 7 exact division\n",b);
    printf("%d can be 3 5  exact division\n",b);
    printf("%d can be 3 7 exact division\n",b);
    printf("%d can be 5 7 exact division\n",b);
    printf("%d can be 3 exact division\n",b);
    printf("%d can be 5 exact division\n",b);
    printf("%d can be 7 exact division\n",b);}
else if(b1==0&&b2==0){
    printf("%d can be 3 5 exact division\n",b);
    printf("%d can be 3 exact division\n",b);
    printf("%d can be 5 exact division\n",b);}
else if(b2==0&&b3==0){
    printf("%d can be 5 7 exact division\n",b);
    printf("%d can be 5 exact division\n",b);
    printf("%d can be 7 exact division\n",b);}
else if(b1==0&&b3==0){
    printf("%d can be 3 7 exact division\n",b);
    printf("%d can be 3 exact division\n",b);
    printf("%d can be 7 exact division\n",b);}
else if(b1==0)
    printf("%d can be 3  exact division\n",b);
else if(b2==0)    
    printf("%d can be 5 exact division\n",b);
else if(b3==0)
    printf("%d can be 7 exact division\n",b);
else
    printf("%d cannot be 3 5 7 exact division\n",b);



printf("title:3**********\n");
printf("please input three number:\n");
scanf("%d%d%d",&c1,&c2,&c3);
c=c1;
if(c<c2){c=c2;if(c<c3)c=c3;printf("max=%d\n",c);}
else if(c<c3){c=c3;if(c<c2)c=c2;printf("max=%d\n",c);}
else printf("three is equal,max=%d\n",c);

printf("title:4**********\n");
printf("please input a days:\n");
scanf("%d",&day);
d1=day/7;
d2=day%7;
if(d2==0)
    printf("the day is %dth week,%dth day\n",d1,7);
else if(d1>0&&d2!=0 || d1==0 &&d2!=0)
    printf("the day is %dth week,%dth day\n",d1+1,d2);
else 
    printf("errpr!\n");

printf("title:5**********\n");
printf("please input three number:\n");
for(i=0;i<3;i++)
scanf("%d",&num[i]);

for(i=0;i<3;i++)

    for(j=0;j<2-i;j++)
    if(num[i]>num[i+1])
    {e=num[i];num[i]=num[i+1];num[i+1]=e;}

for(k=0;k<3;k++)
printf("%d ",num[k]);
printf("\n");

*/
printf("title:6**********\n");
printf("please input a charater:\n");
ch=getchar();
ch1=(ch=='a'||ch=='e'||ch=='i'||ch=='o'||ch=='u')||
    (ch=='A'||ch=='E'||ch=='I'||ch=='O'||ch=='U');
    if(ch1)
	printf("yes\n");
    else
	printf("no\n");





return 0;
}











