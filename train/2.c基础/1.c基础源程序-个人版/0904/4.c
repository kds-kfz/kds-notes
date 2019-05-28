#include"stdio.h"
//4.c

//4,[4][4]，求对角线和
int main4(){
int arr[][4]={0,2,3,90,5,6,7,8,9,10,11,12,13,14,15,20};
int i,j,sum1=0,sum2=0;

for(i=0;i<4;i++)
    sum1+=arr[i][i];
for(i=0;i<4;i++)
    sum2+=arr[i][3-i];
for(i=0;i<4;i++){
    for(j=0;j<4;j++)
	printf("%d  ",arr[i][j]);
	printf("\n");}

printf("sum1=%d,sum2=%d\n",sum1,sum2);
return 0;
}

//3,[3][2]，列的数值，大小排序
int main3(){
int arr[][2]={1,7,3,10,9,6};
int arr1[6];
int i,j,k=0,t=0;
for(i=0;i<3;i++)
    for(j=0;j<2;j++)
	arr1[k++]=arr[i][j];

for(k=0;k<6;k++)
    for(j=0;j<5-k;j++)
    if(arr1[j]<arr1[j+1]){
    t=arr1[j+1];arr1[j+1]=arr1[j];arr1[j]=t;
    }

for(k=0;k<6;k++)
{
    printf("%d ",arr1[k]);
    if(i%2==0)
    printf("\n");
    i++;
}

return 0;
}

//2,[2][3]，最大值下标
int main2(){
int arr[][3]={1,2,3,4,5,6,};
int max,i,j,mi,mj;
max=arr[0][0];
for(i=0;i<2;i++)
    for(j=0;j<3;j++)
	if(arr[i][j]>max){
	    max=arr[i][j];
	    mi=i;mj=j;}
printf("max[%d][%d]=%d\n",mi,mj,max);

return 0;
}

//1,写报表，4个季度，人工，设备，电力，各支出总和
int main(){
int arr[3][4]={{1,2,3,4},{5,6,7,8},{9,10,11,12}};
int sum1=0,sum2=0,sum3=0;
int i,j;
for(i=0;i<4;i++){
    sum1+=arr[0][i];
    sum2+=arr[1][i];
    sum3+=arr[2][i];
}
for(i=1;i<5;i++)
	printf("\t第%d季度\t",i);
    printf("\t四季和");
	printf("\n");
for(i=0;i<3;i++){
    switch(i){
	case 0:printf("人工");break;
	case 1:printf("设备");break;
	case 2:printf("电力");break;
    }
    for(j=0;j<4;j++){
	printf("\t%d\t",arr[i][j]);}
    switch(i){
	case 0:printf("\t%d",sum1);break;
	case 1:printf("\t%d",sum2);break;
	case 2:printf("\t%d",sum3);break;
    }
    printf("\n");
    }
//printf("%d=%d+%d+%d\n",sum1+sum2+sum3,sum1,sum2,sum3);
return 0;

}
