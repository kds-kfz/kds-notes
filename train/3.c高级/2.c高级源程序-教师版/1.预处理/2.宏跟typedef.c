#include <stdio.h>
#define IN int*
typedef int* I;
int main()
{
    IN a,b; //int* a,b;
    I a1,a2;
    int a_1 = 10;
    a = &a_1;
    b = &a_1;
    a1 = &a_1;
    a2 = &a_1;
    return 0;
}
