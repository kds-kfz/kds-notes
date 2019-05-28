#include"stdio.h"
//1.c
//教师版，1，冒泡排序2,选择排序3,插入排序
//输入n个长度数组
void get_data(int *p,int n){
int i;
for(i=0;i<n;i++)
    scanf("%d",p+i);
}
//输出n个长度数组
void put_data(int *p,int n){
int i=0;
for(;i<n;i++)
//    printf("%d ",*(p+i));
    printf("%d ",p[i]);
putchar(10);
}
//冒泡排序
void bubble_sort(int arr[],int n){
    int i,j,flag;
    for(i=0;i<n-1;i++){
	flag=0;
	for(j=0;j<n-1-i;j++){
	    if(arr[j]>arr[j+1]){
		flag=1;
		int temp=arr[j];
		arr[j]=arr[j+1];
		arr[j+1]=temp;
	    }
	}
	if(flag==0)break;
    }
}

//选择排序
void select_sort(int arr[],int n){
int i,j;
for(j=0;j<n-1;j++){
    int max=arr[0];//默认第1个为最大
    int maxindex=0;
    for(i=1;i<n-j;i++)
    if(max<arr[i]){
	max=arr[i];
	maxindex=i;//先找最大值元素和下标
    }
    if(maxindex!=n-1-j){//若最大元素下标不等于第n-1-j个元素时
    arr[maxindex]=arr[n-1-j];//最大值与第n-1-i个元素交换
    arr[n-1-j]=max;
    }
}

//插入排序
void insert_sort(int arr[],int n){
int i,j,temp;
    for(i=1;i<n;i++){
	temp=arr[i];//取第i个元素给temp
    for(j=i;j>0&&temp<arr[j-1];j--)//若第j-1个元素值小于第i个元素且j>0时
	arr[j]=arr[j-1];//第j-1个元素后移1位
    arr[j]=temp;//第j个元素插入temp
/*//不提倡
    for(j=i-1;j>=0&&temp<arr[j];j--)
	arr[j+1]=arr[j];
    arr[j+1]=temp;
    */
    }
}

int main(){

int n=0;
char ch;
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

