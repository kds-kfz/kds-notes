#include <stdio.h>
int main()
{
    // 与运算符 & 　0&1=0 0&0=0 1&1=1
    //任意数每一位跟1相与是本身
    //任意数跟０相与是０
    int a = 258;//1 0000 0010  
    int b = 15;//0  0000 1111
    int c = a & b;
    printf ("c=%d\n",c);

    a = 8; //1000
    b = 7;//0111
    c = a&b;
    printf ("c = %d\n",c);

    a = 88888;//循环次数是88888的二进制1的个数
    int n = 0;
    while (a)
    {
	a =  a & (a-1);
	n++;
    }
    printf ("n = %d\n",n);

    printf ("输入一个整数\n");
    scanf("%d",&a);
    if(a&1)
	printf ("奇数\n");
    else
	printf ("偶数\n");
     return 0;
}
