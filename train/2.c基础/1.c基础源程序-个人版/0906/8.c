#include"stdio.h"

//有序数组a[200]={1,3,6,8,10},输入1个数，插入数组，保持顺序，后面的数向后移

int main(){
int a[100]={1,3,5,7};
int *p=a;
int num,i,j,count=4;
scanf("%d",&num);
    while(num){
    for(i=0;i<count;i++){
	if(num<*(p+i)){
	for(j=count-1;j>=i;j--)
	    *(p+j+1)=*(p+j);
	*(p+i)=num;
	count++;
	break;
	}
    }
    if(count<=i){
    *(p+i)=num;
    count++;
    }
    printf("continue?(0:quit)\n");
    scanf("%d",&num);
    }
for(i=0;i<count;i++)
    printf("%d ",a[i]);
printf("\n");

return 0;
}


int main4(){
int arr[200]={1,3,6,8,10};
int i,j,k=0,n;
int *p1=arr,*p2=NULL,*p3=&arr[4];

for(i=0;i<5;i++)
    printf("%d ",*(p1+i));
printf("\n");

while(1){
printf("input a number:\n");
scanf("%d",&n);
k++;
if(n==0)break;
if(n<=*(p3+k-1)){
    for(i=0;i<5-1+k;i++)
    if(n<=*(p1+i)){
	p2=p1+i;
	break;}
for(j=5-1+k;j>i;j--)
    *(p1+j)=*(p1+j-1);
*p2=n;}
else
    *(p3+k)=n;
for(i=0;i<5+k;i++)
    printf("%d ",*(p1+i));
printf("\n");
}
return 0;
}

//数组a[10]={0,1,2,3,4,5,6,7,8,9}--->>a[10]={1,2,3,4,5,6,7,8,9,0};
int main3(){
/*
//数组a[10]={0,1,2,3,4,5,6,7,8,9}--->>a[10]={9,8,7,6,5,4,3,2,1,0};
int arr[]={0,1,2,3,4,5,6,7,8,9};
int i,j;
int *p1=arr,*p2=&arr[9];
for(i=0;i<10;i++)
    printf("%d ",*(p1+i));
printf("\n");
for(i=0;i<5;i++){
j=*(p2-i);*(p2-i)=*(p1+i);*(p1+i)=j;
}
for(i=0;i<10;i++)
    printf("%d ",arr[i]);
printf("\n");
*/
int arr[]={0,1,2,3,4,5,6,7,8,9};
int i,j,k;
int (*p)[10]=&arr;
k=*(*p);
for(i=0;i<10;i++)
    *(*p+i)=*(*p+i+1);
*(*p+9)=k;
for(i=0;i<10;i++)
    printf("%d ",arr[i]);
printf("\n");
return 0;
}

//用指针。打印a[15]={1,1,2,3,5,8,...};
int main2(){
int arr[15]={0};

arr[0]=1;
arr[1]=1;

int *p=arr;
int i;
for(i=0;i<15;i++)
    printf("%d ",*(p+i));
printf("\n");
for(i=2;i<15;i++)
    *(p+i)=*(p+i-1)+*(p+i-2);
for(i=0;i<15;i++)
    printf("%d ",*(p+i));
printf("\n");
return 0;
}


//用指针排序数组
int main1(){
int i,j,k;
int arr[5]={2,6,8,4,1};
int *p1=arr;
for(i=0;i<5;i++)
    printf("%d ",*(p1+i));
printf("\n");

for(j=0;j<5;j++)
for(i=0;i<5-1-j;i++)
    /*
    if(arr[i]<arr[i+1]){
    k=arr[i];arr[i]=arr[i+1];arr[i+1]=k;
    }*/
    if(*(p1+i)<*(p1+1+i)){
    k=*(p1+i);*(p1+i)=*(p1+i+1);*(p1+i+1)=k;
    }

for(i=0;i<5;i++)
    printf("%d ",arr[i]);
printf("\n");

return 0;
}
