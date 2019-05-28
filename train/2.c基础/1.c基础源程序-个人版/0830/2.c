#include"stdio.h"
#include"stdlib.h"
//2.c
int main(){

//订票
int select;
system("clear");
printf("1:booking,2:return,3:golin,4:quit\n");
scanf("%d",&select);

switch(select){
    case 1:printf("booking success!\n");break;
    case 2:printf("return success!\n");break;
    case 3:printf("gloin success!\n");break;
    case 4:printf("quit success!\n");break;
    default:printf("choose error\n");break;
}

//学生成绩
unsigned short int mark;
printf("please input mark(0-100):\n");
scanf("%d",&mark);
if(mark<0||mark>100) 
    printf("input mark error!\n");
else if(mark>=90) 
    printf("student mark is : A\n");
else if(mark>=80) 
    printf("student mark is : B\n");
else if(mark>=70) 
    printf("student mark is : C\n");
else
    printf("student mark is : E\n");

    printf("student mark is : E\n");
    

return 0;
}



