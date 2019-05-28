#include"stdio.h"


int main()
{

int a=5,b=9;
int c;
c=(a++*2-++b)>1||!a&&a++;
printf("a=%d,b=%d,c=%d\n",a,b,c);

return 0;
}




