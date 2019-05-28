#include"stdio.h"
//公鸡值5元，母鸡值3元，3只小鸡值1元，100元可买公鸡，母鸡，小鸡各多少只？

int main(){
int a,b,c,num=0;

for(a=1;a<=20;a++)
    for(b=1;b<=33;b++)
	for(c=1;c<=92;c++)
	if(100==a*5+b*3+c){
	num++;
	printf("100=%d*5+%d*3+%d*1\n",a,b,c);
	}
printf("num=%d\n",num);
return 0;
}
//如果三种鸡都必须要买到话，共有304种搭配











