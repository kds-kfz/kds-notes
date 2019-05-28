#include"stdio.h"
//1.c
int a,b;
char ch,ch1;

int main(){
/*
printf("input hours:\n");
scanf("%d",&a);
if(a>=40) b=a*200;
else if(a>0&&a<40) b=7000;
else printf("input error!\n");
printf("gets money:%d\n",b);
*/
printf("input a character:\n");
//scanf("%c",ch);
ch=getchar();
getchar();
if(ch>='a'&&ch<='z')
{
    ch1=ch-32;
    printf("%c is small letter\n",ch);
    printf("big letter:%c\n",ch1);
}
else if(ch>='A'&&ch<='Z')
{
    ch1=ch+32;
    printf("%c is big letter\n",ch);
    printf("big letter:%c\n",ch1);

}
else if(ch>=1&&ch<=31||ch==127)
    printf("other\n");
    else
    printf("error!\n");





return 0;
}







