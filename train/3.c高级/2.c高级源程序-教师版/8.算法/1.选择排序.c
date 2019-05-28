#include <stdio.h>
//循环找数组中最大值和最大值下标　跟数组最后n-1元素交换
//在剩下数据中继续找最大数和最大数下标　跟数组n-2交换
//依次循环到排好序为止
//时间复杂度　O(n*n)　　O(最坏情况下排序算法的循环次数)
void select_sort(int *s,int n)
{
    int i = 0;
    int j = 0;
    int Index = 0;
    for(;i<n-1;i++)
    {
	Index = 0;
	for(j=1;j<n-i;j++)
	{
	    if(s[Index] < s[j])
		Index = j;
	}
	if (n-1-i != Index)
	{
	    int temp = s[n-i-1];
	    s[n-i-1] = s[Index];
	    s[Index] = temp;
	}
    }
}
int main()
{
    int a[10] = {12,43,23,67,56,34,87,1,8,0};
    select_sort(a,10);

    int i = 0;
    for(;i<10;i++)
	printf ("%d ",a[i]);

    printf ("\n");
    return 0;
}
