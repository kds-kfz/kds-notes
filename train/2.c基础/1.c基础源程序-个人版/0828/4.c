#include"stdio.h"
#include"string.h"
//#1，键盘输入三个字符，然后输出
//#2,输入数字，输出对应字符
//#3,改写火车票，用键盘输入
char a,b,c;
int  num;
char p1[10];
char p2[10];
double a1=358.456,a2=476.564;
int main()
{
//#1，键盘输入三个字符，然后输出
/*
a=getchar();
getchar();
putchar(a);
putchar('\n');

b=getchar();
getchar();
putchar(b);
putchar('\n');

c=getchar();
getchar();
putchar(c);
putchar('\n');
*/

//#2,输入数字，输出对应字符
/*scanf("%d",&num);
printf("num=%c\n",num);
putchar(num);
*/
char str1[]="1001\tbeijin  \tguangzhou\t2.4h";
char str2[]="1002\tshanghai\tnanning  \t4.8h";

//#3,改写火车票，用键盘输入
printf("please input origin and end point:\n");
scanf("%s %s",p1,p2);
printf("origin:%s--->end point:%s\n",p1,p2);
if(strcmp(p1,"beijin")==0&&strcmp(p2,"guangzhou")==0)
    printf("%s\t%.2lf$\n",str1,a1);
else if(strcmp(p1,"shanghai")==0&&strcmp(p2,"nanning")==0)
    printf("%s\t%.2lf$\n",str2,a2);
else
    printf("input error!\n");
return 0;
}





