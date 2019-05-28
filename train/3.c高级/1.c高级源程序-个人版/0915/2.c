#include<stdio.h>
//3.c
//宏定义
#define PI 3.1415926
#define	Max(a,b) a>b?a:b
#define	MAx(a,b) a++>b++?a++:b++

#define Pow(n) ((n)*(n)) 
//a = 360 / ((5+1) * (5+1))
/*
#define Pow(n) n*n
//a = 360 / 5+1 * 5+1 error
#define Pow(n) (n)*(n)
//a = 360 / (5+1) * (5+1) error
*/
#define P(c) \
do {if(c=='y'||c=='Y')\
    printf("c=%c\n",c);}\
	while(0);
int max(int a,int b){
    return a>b?a:b;
}

int max1(int a,int b){
    return a++>b++?a++:b++;
}

int poW(int n){
    return n*n;
}

int main(){
printf("1-----------------------\n");
    printf("PI=%lf\n",PI);
#undef	PI
//    printf("result value=%.8lf\n",PI*100);

printf("2-----------------------\n");
    int a=10,b=20;
    int	Max=Max(a,b);
    printf("Max=%d\n",Max);

    int m=max(a,b);
    printf("max=%d\n",m);

printf("3-----------------------\n");
    a=10;b=20;
    int Max1=MAx(a,b);
    printf("Max1=%d,a=%d,b=%d\n",Max1,a,b);

    a=10;b=20;
    int m1=max1(a,b);
    printf("max1=%d,a=%d,b=%d\n",m1,a,b);

printf("4-----------------------\n");
    a=360/Pow(5+1);
    printf("Pow=%d\n",a);

    b=360/poW(5+1);
    printf("poW=%d\n",b);

printf("5-----------------------\n");
    printf("input a charater:\n");
    char c;
    scanf("%c",&c);
    P(c)
    return 0;
}
