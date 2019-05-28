#include"stdio.h"
//2.c
//个人版，1,冒泡排序，2,选择排序，3,插入排序
void get_data(int arr[],int n){
    int i;
    for(i=0;i<n;i++)
	scanf("%d",&arr[i]);
}

void put_data(int *p,int n){
    int i;
    for(i=0;i<n;i++)
	printf("%d ",*(p+i));
    //    printf("%d ",p[i]);
    putchar(10);
}
//my冒泡排序
void bubble_sort(int arr[],int n){
    int i,j,temp=0;
    int flag=0;
    for(i=0;i<n-1;i++){
	flag=0;
	for(j=0;j<n-1-i;j++){
	    if(arr[j]>arr[j+1]){
		flag=1;
		temp=arr[j];arr[j]=arr[j+1];arr[j+1]=temp;}
	}
	if(flag==0)break;
    }
}

//my选择排序
void select_sort(int arr[],int n){
    int i,j;
    int flag;
    int min,minid=0;
    for(i=0;i<n-1;i++){
	min=arr[i];
	flag=0;
	for(j=i+1;j<=n-1;j++)
	    //先找最小值，接收最小值和下标，从第i+1个元素开始
	    if(min>arr[j]){
		min=arr[j];//接收最小值和下标
		minid=j;
		flag=1;
	    }
	if(flag==1)//若j+1个元素比i+1个元素小时
	{
	    arr[minid]=arr[i];//最小值temp与第i个元素交换
	    arr[i]=min;
	}
    put_data(arr,n);
    }
}
/*
int max,temp=0;
int k=n-1;
for(i=0;i<n-1;i++){
    max=0;

    for(j=1;j<n-i;j++)
    if(arr[max]<arr[j]){//找最大值，从第1个元素开始
	temp=arr[j];//接收最大值和下标j
	max=j;}

    if(max!=k){//如果最大值下标j不与k相等时
    arr[max]=arr[k];//最大值temp与第k个元素交换
    arr[k]=temp;
    k--;}
    }
    */
//结合插入排序的冒泡 
void insert_sort(int arr[],int n){
    int i,j;
    int temp=0;
    for(i=1;i<n;i++)
    {
	temp=arr[i];//取第i个元素，i从1开始
	for(j=i;j>0;j--)
	if(arr[j-1]>temp){//第j-1(第i-1个元素)个元素比temp(第i个元素)大时
	    arr[j]=arr[j-1];//第i个元素与第i-1个元素交换
	    arr[j-1]=temp;
	}
    }
}

int main(){

    int n=0;
    printf("input array long :\n");
    scanf("%d",&n);
    int arr[n];

    get_data(arr,n);
    //bubble_sort(arr,n);
    select_sort(arr,n);
    //insert_sort(arr,n);
    put_data(arr,n);

    return 0;
}

