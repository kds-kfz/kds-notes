#include"stdio.h"
//5.c
int main(){
int a=10;
int *p1=&a;
int **p2=&p1;
//a的值
printf("a=%d,*p1=%d,**p2=%d",a,*p1,**p2);
printf("\n");

//a值地址
printf("a_addr=%p,p1_addr=%p,p2_addr=%p\n",&a,p1,*p2);
printf("\n");

//p1
printf("p1_addr=%p,*p2_addr=%p",&p1,p2);
printf("\n");

//p2
printf("**p2_addr=%p\n",&p2);
//**p2指向a的地址
printf("**p2_addr=%p\n",&**p2);
printf("\n");

return 0;
}




