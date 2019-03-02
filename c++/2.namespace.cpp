#include<stdio.h>
namespace B{
    int b =20;
}
void fun(){
    printf("b = %d\n", B::b);
}
