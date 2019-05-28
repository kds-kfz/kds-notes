#include<stdio.h>
//1.cpp

namespace B{
    int b=200;
}

void fun(){
    printf("b=%d\n",B::b);
}
