#include<stdio.h>
#include<string.h>
//1.c指针函数
void fun(){
    printf("********printf***********\n");
}
void fun1(int a,int b){
    printf("a=%d,b=%d\n",a,b);
}
int fun2(int a,int b){
    return a>b?a:b;
}
char *fun3(char *p1,const char *p2){
    return strcpy(p1,p2);
}

int main(){
    void (*p)();
    p=fun;
    p();
    printf("1--------------------\n\n");

    void (*p1)(int,int)=fun1;
    p1(3,7);
    printf("2--------------------\n\n");

    int (*p2)(int,int)=fun2;
    printf("max=%d\n",p2(5,9));
    printf("3--------------------\n\n");

    char *(*p3)(char *,const char *)=fun3;
    char arr[100]="0";
    char *str=p3(arr,"fdfsdf");
    printf("%s\n",str);
    printf("4--------------------\n\n");

    return 0;
}
