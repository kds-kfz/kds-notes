#include"stdio.h"
#include"stdlib.h"
#include"unistd.h"
//1.c
int main(){

//订票
int select;
while(1){
system("clear");
printf("1:booking ticket,2:return ticket,3:alter ticket,4:quit\n");

while(scanf("%d",&select)==0){//判断输入是否是整型，
printf("choose error\n");
getchar();			//不是整型则接收回车
sleep(2);
system("clear");
printf("1:booking ticket,2:return ticket,3:alter ticket,4:quit\n");
printf("please input again!\n");
}

switch(select){
    case 1:printf("booking success!\n");sleep(2);break;
    case 2:printf("return success!\n");sleep(2);break;
    case 3:printf("alter success!\n");sleep(2);break;
    case 4:printf("quit success!\n");exit(-1);break;
    default:printf("choose error\n");sleep(2);break;
}
}
return 0;
}

