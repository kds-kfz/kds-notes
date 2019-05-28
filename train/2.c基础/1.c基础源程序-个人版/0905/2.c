#include"stdio.h"
#include"stdlib.h"
#include"math.h"
//2.c
int main(){
char ch[10];
char ch1[10];
char ch3[10];
char ch2;
int i;
//for(i=0;i<10;i++)
//    scanf("%c",&ch[i]);

for(i=0;i<10;i++){
    ch2=getchar();
//    getchar();
	ch3[i]=ch2;}

//scanf("%s",ch1);
//gets(ch1);

for(i=0;i<10;i++)
    printf("%c",ch3[i]);
//printf("%s",ch1);
//puts(ch);
return 0;
}




int main1(){
int a;
double b,c;
scanf("%d",&a);
scanf("%lf",&b);
c=pow(2,3);
printf("a=%d\n",abs(a));
printf("b=%lf\n",fabs(b));
printf("c=%lf\n",c);

return 0;
}



