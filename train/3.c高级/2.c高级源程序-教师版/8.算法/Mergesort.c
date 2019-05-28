#include <stdio.h>
#include <stdlib.h>
//时间复杂度 nlogn
void Merge(int s[],int tmpArray[],int L,int R,int RightEnd)
{
    int LeftEnd = R -1;
    int NumElements = RightEnd - L +1;
    int TmpPos = L;
    while (L <= LeftEnd && R <= RightEnd)
    {
	if (s[L] <= s[R])
	    tmpArray[TmpPos++] = s[L++];
	else
	    tmpArray[TmpPos++] = s[R++];
    }
    while (L <= LeftEnd)
	tmpArray[TmpPos++] = s[L++];
    while (R <= RightEnd)
	tmpArray[TmpPos++] = s[R++];
    int i;
    for(i=0;i<NumElements;i++,RightEnd--)
	s[RightEnd] = tmpArray[RightEnd];
}

void Msort(int s[],int TmpArray[],int left,int right)
{
    int Center;
    if(left < right)
    {
	Center = (left + right) / 2;
	Msort(s,TmpArray,left,Center);
	Msort(s,TmpArray,Center + 1,right);
	Merge(s,TmpArray,left,Center+1,right);	
    }
}
void Mergesort(int s[],int N)
{
    int* tmparray;
    tmparray = (int*)malloc(N*sizeof(int));
    if ( tmparray != NULL)
    {
	Msort(s,tmparray,0,N-1);
	free(tmparray);
    }
    else
	printf ("申请内存失败\n");
}

int main()
{
    int a[10] = {2,5,7,3,1,8,6,9,10,0};
    Mergesort(a,10);
    int i = 0;
    for(i=0;i<10;i++)
	printf ("%d ",a[i]);
    printf ("\n");
    return 0;
}
