#include"stdio.h"
//3.c
//输入10进制，输出2进制
int main(){

int arr[30];
int num;
int i=0,j;
scanf("%d",&num);
while(num){
arr[i++]=num%2;
num/=2;
}
for(j=i-1;j>=0;j--)
    printf("%d",arr[j]);
printf("\n");
return 0;
}

