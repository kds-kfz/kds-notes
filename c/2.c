#include <stdio.h>
#include <stdlib.h>
int a = 0;
void myprint(int *a);
int main(){
    while(a<10){
        if(a==5)
            goto loop;
        printf("正常\n");
loop:
    myprint(&a);
    }    
    return 0;
}

void myprint(int *a){
    a++;
    printf("ok\n");
}

