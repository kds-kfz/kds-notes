#include"stdio.h"
//1.c
int main(){
int *p;
double *p1;
int a=10;
int *p2;
printf("p_add=%p\n",&p);
printf("\n");
printf("p_add=%p\n",p);
//printf("p_value=%d\n",*p);
//printf("p2_value=%d\n",*p2);
//printf("p1_value=%lf\n",*p1);
printf("a_add=%p\n",&a);
printf("\n");
p=&a;
printf("p_add=%p\n",p);
printf("a_add=%p\n",&a);
printf("\n");
p++;
printf("p_add=%p\n",p);
//printf("p_add=%p\n",&p+1);
return 0;
}



