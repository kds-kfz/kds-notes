#include"stdio.h"
//6.c
int main(){
int arr[][4]={
	1,2,3,4,
	5,6,7,8,
	8,5,3,7
};
int *p1[3][4]={
    *arr,*arr+1,*arr+2,*arr+3,
    *arr+4,*arr+5,*arr+6,*arr+7,
    *arr+8,*arr+9,*arr+10,*arr+11
    };
int i;
int **p2;
p2=p1[0];
for(i=0;i<12;i++)
{
printf("%4d",**p2);
p2++;
}
printf("\n");
return 0;
}
