#include"stdio.h"
#include"stdlib.h"
//3.c
/*  main5,main4,main3
    1 1 1 1 1 1 1
    1 2 2 2 2 2 1
    1 2 3 3 3 2 1
    1 2 3 4 3 2 1
    1 2 3 3 3 2 1
    1 2 2 2 2 2 1
    1 1 1 1 1 1 1
   */
int main5(){
int i,j;
int a,b,c,d;
scanf("%d%d%d%d",&a,&b,&c,&d);
for(i=0;i<7;i++)
    for(j=0;j<7;j++){
    arr[i][j]=a;
    if(i>0&&i<6 && j>0&&j<6)
	arr[i][j]=b;
    if(i>1&&i<5 && j>1&&j<5)
	arr[i][j]=c;
    }
    arr[3][3]=d;
return 0;
}

int main4(){
int a[4]={1,2,3,4};
int arr[7][7];

int i,j,k;
for(i=0;i<4;i++)
    for(j=i;j<7-i;j++)
	for(k=i;k<7-i;k++)
	    arr[j][k]=a[i];

for(i=0;i<7;i++){
    for(j=0;j<7;j++)
	printf("%d ",arr[i][j]);
    printf("\n");}
}


int main3(){
int a,b,c,d;
scanf("%d%d%d%d",&a,&b,&c,&d);

int arr[7][7]={0};
int i,j;
for(i=0;i<7;i++)
    for(j=0;j<7;j++){
    if(i==0||i==6||j==0||j==6)arr[i][j]=a;
    if(i==1&&j!=0&&j!=6||i==5&&j!=0&&j!=6||j==1&&i>1&&i<5||j==5&&i>1&&i<5)arr[i][j]=b;
    if(i==2&&j>1&&j<5||i==4&&j>1&&j<5||j==2&&i>2&&i<4||j==4&&i>2&&i<<4)arr[i][j]=c; 
    arr[3][3]=d;
    }
for(i=0;i<7;i++){
    for(j=0;j<7;j++)
	printf("%d ",arr[i][j]);
    printf("\n");}
return 0;
}
//main2,输入连续字符，输出加3后对应字母，超过Z或z的返回A或a依次输出
int main2(){
char arr[5];
int i,j;
for(i=0;i<5;i++)
scanf("%c",&arr[i]);

for(i=0;i<5;i++){
    if(arr[i]>='a'&&arr[i]<='z'||arr[i]>='A'&&arr[i]<='Z')
	arr[i]=arr[i]+3;
    if(arr[i]>'z'||arr[i]>'Z'&&arr[i]<='Z'+3)
	arr[i]-=26;}
for(i=0;i<5;i++)
printf("%c ",arr[i]);
printf("\n");
return 0;

}
/*main
    -5  4   3  2   1      ---->       -5  4    3   2   1
    -6  7   8  9   10                 -6  10   8   9   7
    11  12  13 14  -15                11  12  -15  14  13
   -20  16  17 18  19                 18  16   17 -20  19
    21  22  45 34  25                 21  22   25  34  45
   */

int main(){
int arr1[][5]={
    1,2,-3,4,-5,
    6,7,8,9,-10,
    11,12,13,14,
    -15,-16,17,-18,19,-20,
    21,22,-25,24,23};
int arr[][5]={
-5,-8,0,3,5,
82,10,-19,6,4,
7,9,4,0,-6,
5,9,-6,-99,100

};

int i,j,k,t,p,max;
for(i=0;i<5;i++){
    for(j=0;j<5;j++)
	printf("%6d",arr[i][j]);
	printf("\n");}
printf("\n\n");
for(j=0;j<5;j++){
    max=arr[j][0];
    t=0;
for(i=0;i<5;i++)
    if(abs(max)<abs(arr[j][i])){
    max=arr[j][i];
	t=i;}
    p=arr[j][j];arr[j][j]=arr[j][t];arr[j][t]=p;

    }

for(i=0;i<5;i++){
    for(j=0;j<5;j++)
	printf("%6d",arr[i][j]);
	printf("\n");}
return 0;
}
