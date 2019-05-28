#include <stdio.h>
//把数组第０个元素当作关键字　把小于关键字的元素放到左边大于关键字的放到右边 依次递归
//O(nlogn)
void quick_sort(int* s,int low,int hight)
{
    if (low < hight)
    {
	int l = low;
	int h = hight;
	int key = s[l];
	while (l < h)
	{
	    while(key < s[h] && l < h)
		h--;
	    if(l < h )
		s[l++] = s[h];
	    while(key > s[l] && l < h)
		l++;
	    if(l < h)
		s[h--] = s[l];
	}
	s[l]=key;
	quick_sort(s,low,l-1);
	quick_sort(s,l+1,hight);
    }
}
int main()
{
    int a[15] = {0,34,23,6,7,45,11,9,4,0,2,8,1,6,3};
    quick_sort(a,0,14);
    int i = 0;
    for(;i<15;i++)
	printf ("%d ",a[i]);
    printf ("\n");
    return 0;
}
