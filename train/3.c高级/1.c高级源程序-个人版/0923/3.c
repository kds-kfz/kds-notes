#include<stdio.h>
//二分查找

int search(int *arr,int n,int num){
    int t=0;
    int s=n-1;
    while(t<=s){
	int m=(t+s)/2;
	if(num<arr[m])
	    s=m-1;
	else if(num>arr[m])
	    t=m+1;
	else
	    return m;
    }
    return -1;
}

int main(){
    int arr[30]={1,2,3,4,5,6,7,8,9,10,
		11,12,13,14,15,16,17,18,19,20,
		30,40,50,60,70,80,90,100,110,120};
    int n=0;
    printf("输入要查找的数\n");
    scanf("%d",&n);
    int i=search(arr,30,n);
    if(i==-1)
	printf("不存在\n");
    else
	printf("arr[%d]=%d\n",i,n);
    return 0;
}
