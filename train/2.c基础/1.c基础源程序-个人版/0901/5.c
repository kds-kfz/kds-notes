#include"stdio.h"
//5.c
//输出魔方阵
int main(){
int arr[15][15]={0};
int i,j,k,n;

printf("input a number\n");
scanf("%d",&n);

if(n%2==0)n++;

for(i=1;i<=n;i++)
    for(j=1;j<=n;j++)
	arr[i][j]=0;
i=1;
j=(n+1)/2;
arr[i][j]=1;

for(k=2;k<=n*n;k++){
    i--;
    j++;

    if((i<1)&&(j>n)){i+=2;j--;}

    if(i<1)i=n;
    if(j>n)j=1;

    if(arr[i][j]!=0){i+=2;j--;}
    arr[i][j]=k;
}
for(i=1;i<=n;i++){
    for(j=1;j<=n;j++)
    printf("%5d",arr[i][j]);
    printf("\n");}

return 0;
}

//输出杨辉三角
int main1(){

int arr[10][10]={0};
int i,j;

for(i=0;i<10;i++)
    arr[i][0]=1;
for(j=1;j<10;j++)
    arr[j][j]=1;

for(i=0;i<10;i++){
    for(j=0;j<i+1;j++){
	if(i>1&&j>0)
	    arr[i][j]=arr[i-1][j-1]+arr[i-1][j];
	printf("%d  ",arr[i][j]);
    }
    printf("\n");
    }
return 0;
}


