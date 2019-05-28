#include <stdio.h>
//把数组看成两部分　把数据第０个元素看成是排好序的数组
//剩下的就是未排序的数组　把没有排序的元素一个一个取插入到排好序的数组
//O(n*n)
void insert_sort(int *s,int n)
{
    int i = 0;
    int j = 0;
    int key = 0;
    for(i=1;i<n;i++)
    {
	//取数据
	key = s[i];
	//找位置
	/*
	for(j=i;key<s[j-1]&&j>0;j--)
	     s[j] = s[j-1];
	  s[j]  = key;
	*/
	for(j=i-1;key<s[j]&&j>=0;j--)
		s[j+1]=s[j];
    	s[j+1] = key;
    }
}
int main()
{
    int a[5] = {7,2,3,0,4};
    insert_sort(a,5);
    int i = 0;
    for(;i<5;i++)
	printf ("%d ",a[i]);
    printf ("\n");
    return 0;
}
