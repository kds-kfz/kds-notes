#include <stdio.h>
void print(int a)
{
    int size = sizeof a * 8;
    int i  = 0;
    while (size--)
    {
	int n = a>>size&1;
	printf ("%d",n);
	i++;
	if (i == 4)
	{
	    printf (" ");
	    i = 0;
	}
    }
    printf ("\n");
}
int main()
{
    int n  = 0;
    printf ("输入一个整数\n");
    scanf("%d",&n);
    print(n);
}
