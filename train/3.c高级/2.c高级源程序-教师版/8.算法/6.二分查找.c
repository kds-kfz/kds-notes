#include <stdio.h>
int search(int *s,int n,int num)
{
    int i = 0;
    for(;i < n;i++)
    {
	if(s[i] == num)
	    return i;
    }
    return -1;
}
int search1(int *s,int n,int num)
{
    int l = 0;
    int h = n-1;
    while( l <= h)
    {
	count++;
	int m = (l+h)/2;
	if(num > s[m])
	    l = m+1;
	else if (num < s[m])
	    h = m-1;
	else
	    return m;
    }
    return -1;
}
int main()
{
    int a[10] = {3,4,5,6,7,8,9,10,11,12};
    int n = 0;
    printf ("输入查找的数据\n");
    scanf("%d",&n);
    int i = search1(a,10,n);
    if (i == -1)
	printf ("不存在\n");
    else
	printf("数组中第%d个元素\n",i);
    return 0;
}
