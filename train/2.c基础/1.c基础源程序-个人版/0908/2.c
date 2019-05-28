#include"stdio.h"
//2.c
int fun1(int a,int b);
int fun2(int *a,int *b);
void fun3(int a,int b,int e);
void fun4(int *a,int *b,int *c);

int main(){
int a,b,c,d,e,f;
int *p1=&a,*p2=&b;
scanf("%d%d",&a,&b);
c=fun1(a,b);
d=fun2(&a,&b);
fun3(a,b,e);
fun4(p1,p2,&f);
printf("a=%d,b=%d,c=%d,d=%d,e=%d,f=%d\n",a,b,c,d,e,f);

return 0;
}

int fun1(int a,int b){
int c;
if(a>b)c=a-b;
else c=b-a;
return c;
}
int fun2(int *a,int *b){
int c;
c=*a**b;
return c;
}

void fun3(int a,int b,int e){
//    int c;
    if(a>=b)e=a*3+b;
    else e=a+b*3;
//    e=c;
}
void fun4(int *a,int *b,int *c){
//    int n;
    if(a>=b)*c=*a*3+*b;
    else *c=*a+*b*3;
//    *c=n;
}




