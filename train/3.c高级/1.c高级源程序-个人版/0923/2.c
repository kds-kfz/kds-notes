#include<stdio.h>
//2.c
//快速排序
void quick_sort(int *s,int low,int high){
    if(low<high){
	int l=low;
	int h=high;
	int key=s[l];
	while(l<h){
	    while(s[h]>key&&l<h)
		h--;
	    if(l<h)
		s[l++]=s[h];
	    while(s[l]<key&&l<h)
		l++;
	    if(l<h)
		s[h--]=s[l];
	}//-9 4 0 3 -3 -8 -8 8 10 7    l=6
	s[l]=key;//-9 4 0 3 -3 -8 5 8 10 7
	quick_sort(s,low,l-1);//排前0-5个
	quick_sort(s,l+1,high);//排后6-9个
    }
}
int main(){
    /*
    printf("莫让幽怨记心头\n");
    printf("你我不过半壶酒\n");
    printf("策马奔腾何处走\n");
    printf("我来世还复休\n");
    */
    int a[10]={5,4,0,7,-3,10,-8,8,3,-9};
    quick_sort(a,0,9);
    int i=0;
    for(;i<10;i++)
	printf("%d ",a[i]);
    putchar(10);
    return 0;
}

