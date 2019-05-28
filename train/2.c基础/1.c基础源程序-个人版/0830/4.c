#include"stdio.h"
//4.c

int a;
int b,b1,b2,b3,b4;

float c,c1;
int d1,d2,d3,e1,e2,e3,e4,e5;

int main(){
int value(int a,int b);
/*
//1,写火车票菜单
printf("title:1**********\n");
printf("1:booking tickets,2:return tickets,3:alter tickets,4:quit\n");
scanf("%d",&a);
switch(a)
{
    case 1:
	printf("booking tickets...\n");
	break;
    case 2:
	printf("return tickets...\n");
	break;
    case 3:
	printf("alter tickets...\n");
	break;
    case 4:
	printf("quit...\n");
	break;
    default:
	printf("input number error!\n");
	break;
}


//2,输入数字判断是哪个季节
printf("title:2**********\n");
printf("please input a month:\n");
scanf("%d",&b);
b1=b>=3&&b<=5;
b2=b>=6&&b<=8;
b3=b>=9&&b<=11;
b4=b==1||b==2||b==12;
if(b1)printf("spring\n");
if(b2)printf("summer\n");
if(b3)printf("autumn\n");
if(b4)printf("winter\n");


//3,去超市，超过50打八折，超过100打七折，超过200打五折
printf("title:3**********\n");
printf("please input your total price:\n");
scanf("%f",&c);
if(c<0)printf("input error!\n");
else if(c>=200)c=c*0.5;
else if(c>=100)c=c*0.7;
else if(c>=50)c=c*0.8;
printf("you should pay for :%.2f\n",c);
*/


//4,输入三条边的长，判断是什么三角形,a+b>c&&a-b<c,or a+b<=c
while(1){

printf("title:4**********\n");
printf("输入三角形三条边长:\n");
scanf("%d%d%d",&d1,&d2,&d3);

e1=d1*d1==d2*d2+d3*d3;
e2=d2*d2==d1*d1+d3*d3;
e3=d3*d3==d2*d2+d1*d1;

e4=d1+d2>d3&&d1+d3>d2&&d2+d3>d1;
e5=value(d1,d2)<d3&&value(d1,d3)<d2&&value(d2,d3)<d1;


printf("d1=%d,d2=%d,d3=%d\n",d1,d2,d3);
if(e4&&e5){
if(d1==d2&&d2==d3)printf("等边三角形\n");
else if(d1==d2||d1==d3||d2==d3)printf("等腰三角形\n");
else if(e1||e2||e3)printf("直角三角形\n");
else printf("钝角三角形\n");
}
else printf("不是三角形：\n");

}

return 0;
}

int value(int a,int b){
int c;
c=a-b;
if(c>=0)return c;
else return c*(-1);

}


