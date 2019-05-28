#include"stdio.h"
//3.c
//指针数组

int main2(){
    int arr[]={1,2,3,4,5};
    int *p[5];
    int i,j,t;
    for(i=0;i<5;i++)
	p[i]=&arr[i];

    for(i=0;i<5;i++)
	for(j=0;j<5-1-i;j++)
	    if(*p[j]<*p[j+1]){
	    t=*p[j];*p[j]=*p[j+1];*p[j+1]=t;}

    for(i=0;i<5;i++)
	printf("%d ",*p[i]);
    printf("\n");

 return 0;   
}



int main1(){
    int arr[]={1,2,3,4,5};
    int *p[5];
    int i;
    for(i=0;i<5;i++)
	p[i]=&arr[i];

    for(i=0;i<5;i++)
    printf("%d ",*p[i]);
    printf("\n");
    
    for(i=0;i<5;i++)
    printf("%d ",*(*p+i));
    printf("\n");

 return 0;   
}
