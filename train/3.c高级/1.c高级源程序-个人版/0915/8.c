#include"stdio.h"

int main(){
    int arr[6]={7,6,9,30,67,45};
    int i=0,*p=arr,*p1=p;
    int max=*p;
    for(;i<6;i++)
	printf("%d ",arr[i]);
    putchar(10);
/*
    for(i=0;i<5;i++,p++)
	if(*p>max)
	max=*p;
    p--;
    if(p!=p1){
	*p=*p1;
	*p1=max;
    }
*/
    int max1=arr[0],max1id=0;
    for(i=0;i<5;i++)
	if(max1<arr[i]){
	    max1=arr[i];
	    max1id=i;
	}
    if(max1id!=0){
	arr[max1id]=arr[0];
	arr[0]=max1;
    }
    printf("max=%d\n",max1);
    for(i=0;i<6;i++,p1++)
	printf("%d ",*p1);
    putchar(10);
return 0;
}
