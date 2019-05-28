#include"stdio.h"
//5.c
//排序
int fun1(int *p,int x){
    int i,j,t;
    for(i=0;i<x;i++)
	for(j=0;j<x-1-i;j++)
	    if(*(p+j)>*(p+j+1)){
		t=*(p+j);*(p+j)=*(p+j+1);*(p+j+1)=t;}
    for(i=0;i<5;i++)
	printf("%d ",*(p+i));
}
//计数字符个数
int fun2(){
    char ch;
    int num=0;
    while((ch=getchar())!='\n') num++;
    return num;
}
//算平均值
float fun3(int a,int b){
return (a+b)/2.0;
}

float fun4(int *a,int *b){
return (*a+*b)/2.0;
}
//改写pow
int fun5(int a,int b){
int i,c=1;
for(i=1;i<=b;i++)
c*=a;
return c;
}
//字符转换
char fun6(char a){
if(a>='a'&&a<'z')return a-32;
else return a+32;
}

int main(){
    int arr[5]={0};
    int a,b,c,d,e,i,*p=arr;
    int n;
    float ave;
    char ch;
    printf("input 5 bit number:\n");
    for(i=0;i<5;i++)
	scanf("%d",p+i);
//    for(i=0;i<5;i++)
//	printf("%d ",*(p+i));
//    printf("\n");
    fun1(p,5);
    printf("\n");
    getchar();

    printf("input n charater:\n");
    n=fun2();
    printf("n=%d\n",n);

    printf("input two number:\n");
    scanf("%d%d",&a,&b);
    ave=fun3(a,b);
    printf("ave=%.4f\n",ave);

    printf("input two number:\n");
    scanf("%d%d",&a,&b);
    ave=fun4(&a,&b);
    printf("ave=%.4f\n",ave);

    printf("input two number:\n");
    scanf("%d%d",&a,&b);
    e=fun5(a,b);
    printf("a=%d,b=%d,a^b=%d\n",a,b,e);
    getchar();

    printf("input one charater:\n");
    ch=getchar();
    printf("ch=%c,ch=%c\n",ch,fun6(ch));
    return 0;
}
