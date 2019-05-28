#include"stdio.h"


int main(){
int a=10;
int *p=&a;
*p=6;

printf("%d\n",*p);



return 0;
}
