#include<stdio.h>
//1.c
//堆排序
void swap(int *a,int *b){
    int c=*a;
    *a=*b;
    *b=c;
}
//建堆
void percdown(int *arr,int i,int n){
    int temp=0;
    int child=0;
    //temp保存需要操作的数据
    //2*i+1<n，确保需要操作有左孩子
    //i=child，每次循环后i保存最大孩子的下标
    for(temp=arr[i];2*i+1<n;i=child){
	//从左孩子开始
	child=2*i+1;
	//如果左孩子不是最后一个数，同时，左孩子的值小于右孩子
	if(child!=n-1&&arr[child]<arr[child+1])
	    //条件成立，child保存右孩子下标
	    child+=1;
	//如果temp比最大孩子值小
	if(temp<arr[child])
	    //将最大孩子的值覆盖操作数的值
	    arr[i]=arr[child];
	else
	    //如果temp比最大孩子的值大或相等，则跳出for
	    break;
    }
    //for循环跳出有2种情况
    //1，操作数没有左孩子，即2*i+1<n不成立
    //2，第一次循环操作数temp大于或等于最大孩子值，break跳出，
    //	 此时i不变，即操作数的值赋给自己
    //	 第二次以后循环，i保存了最大孩子的下标，若再次进for循环，
    //	 再判断child是否存在左孩子，不存在则退出循环，此时操作数值覆盖
    //	 原最大孩子的值
    arr[i]=temp;
}

void heapsort(int *arr,int n){
    //第一次建堆
    int i=0;
    for(i=n/2;i>=0;i--)
	percdown(arr,i,n);
    for(i=n-1;i>0;i--){
    //数据交换
	swap(&arr[0],&arr[i]);
    //数据交换后再建堆
	percdown(arr,0,i);
    }
}

int main(){
    int arr[8]={7,2,5,9,45,20,34,0};
    heapsort(arr,8);
    int i=0;
    for(i=0;i<8;i++)
	printf("%d ",arr[i]);
    putchar(10);
    return 0;
}

