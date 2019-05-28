#include <stdio.h>
#define M(a) ((a)*(a))
#define Ma(x,y) x++ > y++ ? x++ : y++;

int max(int x,int y)
{
    return x++ > y++ ? x++ : y++;
}
int main()
{
    int a = 8;
    int b = 9;

    int c = Ma(a,b);//a++ > b++ ? a++ : b++
    printf ("c = %d a = %d b = %d\n",c,a,b);

    a = 8;
    b = 9;
    c = max(a,b);
    printf ("c = %d a = %d b = %d\n",c,a,b);

 /*
    int b = 10;
    int c =  M(b);//b*b
    printf ("c=%d\n",c);

    c = 400/max(b+3);//400/((b+3)*(b+3))
    printf ("c = %d\n",c);
    */
    return 0;
}
