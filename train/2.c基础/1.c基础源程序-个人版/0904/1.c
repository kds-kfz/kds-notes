#include"stdio.h"
//1.c

int main(){
printf("please input 10 bit charater:\n");
char ch,ch1='\0',arr[10]={'\0'};
int i,j,k;

for(i=0;i<10;i++){
ch=getchar();
getchar();
arr[i]=ch;}

for(j=0;j<10;j++)
    for(k=0;k<9-j;k++)
	if(arr[k]>arr[k+1]){
	ch1=arr[k];arr[k]=arr[k+1];arr[k+1]=ch1;
	}
for(i=0;i<10;i++)
    printf("%c ",arr[i]);
printf("\n");
return 0;
}

int main1(){
int a[10]={2,4,6,9,56,0,-9,-67,99,20};
int i,j,t;

for(i=0;i<10;i++)
    for(j=0;j<10-i-1;j++)
    if(a[j]>a[j+1]){
    t=a[j];a[j]=a[j+1];a[j+1]=t;
    }

for(i=0;i<10;i++)
    printf("%d ",a[i]);
printf("\n");


return 0;
}



