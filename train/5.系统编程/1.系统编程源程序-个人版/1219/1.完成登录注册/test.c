#include<stdio.h>
#include<string.h>

typedef struct{
    int a;
    int num[10];
}SA;

int main(){
    SA sw={1001,{0}};
    SA *p=&sw;
    char buf[10];
    for(int i=0;i<10;i++)
	if(p->num[i]==0)
	    buf[i]='2';
	else
	    buf[i]='1';
    printf("buf %s\n",buf);
}
