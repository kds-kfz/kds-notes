#include<stdio.h>
//5.test.c
typedef void(*Fun)(int);

void fun1(int n){printf("n1 = %d\n",n);}
void fun2(int n){printf("n2 = %d\n",n);}
void fun3(int n){printf("n3 = %d\n",n);}

typedef struct{
    Fun p;
    int n;
}SIG;

int main(){
    SIG Sig[3]={{fun1,110},{fun2,119},{fun3,120}};
    int choose=0;
    printf("please input sig : ");
    while(1){
	scanf("%d",&choose);
	for(int i=0;i<3;i++)
	    if(choose==Sig[i].n)
		Sig[i].p(choose);
    }
    return 0;
}
