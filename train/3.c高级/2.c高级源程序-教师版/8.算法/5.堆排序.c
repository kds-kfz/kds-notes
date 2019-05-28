#include <stdio.h>
//把数组看成无序二叉树　　i　左孩子下标2*i+1 右孩子下标为2*i+2
//1.建成最大堆　
//2.堆顶元素和最后一个元素交换
// 建堆　交换　　直到无序堆剩一个元素为止
//O(nlogn)
 void swap(int* a,int* b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

void PercDown(int* s,int i,int n)
{
    int temp;
    int child;
    for(temp=s[i];2*i+1 < n;i=child)
    {
	child = 2*i+1;
	if (child != n-1 && s[child+1] > s[child])
	    child++;
	if(temp < s[child])
	    s[i] = s[child];
	else
	    break;
    }
    s[i] = temp;
}
void heap_sort(int* s,int n)
{
    int i;
    int j;
    for(i=n/2;i>=0;i--)
	PercDown(s,i,n);
    for(j=n-1;j>0;j--)
    {
	 swap(&s[0],&s[j]);
         PercDown(s,0,j);
    }
}
int  main()
{
    int a[8] = {8,10,3,25,4,7,9,27};

    heap_sort(a,8);

    int i = 0;
    for(;i<8;i++)
	printf("%d ",a[i]);
    printf ("\n");
}
