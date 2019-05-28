#include"stdio.h"
//printf.c

int main(){
    int i=8;

    printf("%d %d %d %d %d\n",i++,i++,i++,i++,i++);
    i=8;
    printf("%d %d %d %d %d\n",++i,++i,++i,++i,++i);
    i=8;
    printf("%d %d %d %d %d\n",i++,++i,i++,++i,i++);
    i=8;
    printf("%d %d %d %d %d\n",++i,i++,++i,i++,++i);
    i=8;
    printf("%d %d %d %d %d\n",i++,i++,++i,++i,i++);

    return 0;
}
