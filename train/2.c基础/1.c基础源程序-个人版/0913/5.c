#include"stdio.h"
//5.c
//无名结构体
//没有名称到结构体，在定义的同时可以定义变量，离开定义后就不能定义变量
struct {
    int num;
    char sex;
}a1={1001,'m'},a2={1002,'n'};

struct{
    int num;
    char sex;
}b1,b2;

int main(){

    int i;

    b1.num=1003;
    b1.sex='m';
    b2.num=1004;
    b2.sex='n';

    printf("%d %c\n",a1.num,a1.sex);
    printf("%d %c\n",a2.num,a2.sex);

    printf("%d %c\n",b1.num,b1.sex);
    printf("%d %c\n",b2.num,b2.sex);

    return 0;
}
