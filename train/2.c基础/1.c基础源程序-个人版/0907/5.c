#include"stdio.h"
//5.c
//二维数组指针
int main(){
int arr[][3]={
    1,2,3,
    4,5,6};
int (*p)[3]=arr;
int i,j;
for(i=0;i<2;i++){
    for(j=0;j<3;j++)
//	printf("%d ",*(*(p+i)+j));
	printf("%d ",*(p[i]+j));
    printf("\n");
}

return 0;
}
