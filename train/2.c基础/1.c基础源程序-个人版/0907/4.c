#include"stdio.h"
//4.c
//打印报表
int main(){
int arr[][4]={
    1,2,3,4,
    5,6,7,8,
    9,10,11,12,
    13,14,15,16
    };
int *p[4]={arr[0],arr[1],arr[2],arr[3]};
int i,j;
int sum[4]={0};
printf("季度\t电力\t人力\t设备\t固有\n");
for(i=0;i<4;i++){
    printf("%d季度\t",i+1);
    for(j=0;j<4;j++)
//	printf("%d\t",*(p[i]+j));
	printf("%d\t",*(*(p+i)+j));
    printf("\n");
    }
for(i=0;i<4;i++)
    for(j=0;j<4;j++)
	sum[i]+=*(p[j]+i);
printf("合计\t%d\t%d\t%d\t%d\n",sum[0],sum[1],sum[2],sum[3]);
return 0;
}

//二维指针数组
int main1(){
    int arr[][3]={
    1,2,3,
    4,5,6,
    7,8,9
    };
 int *p[3]={arr[0],arr[1],arr[2]};
 int i,j;
for(i=0;i<3;i++){
    for(j=0;j<3;j++)
//	pritnf("%d ",arr[i][j]);
//	printf("%d ",*(arr[i]+j));
//	printf("%d ",*(p[i]+j));
	printf("%d ",*(*(p+i)+j));
 printf("\n");}
    return 0;
}
