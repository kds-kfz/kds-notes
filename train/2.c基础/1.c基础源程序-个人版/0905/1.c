#include"stdio.h"
//1.c
int main(){
int arr[2][3];
char arr1[2][3];
double arr2[2][3];

printf("address:%p\n",arr);
printf("address:%p\n",&arr);
printf("address:%p\n",arr[0]);
printf("address:%p\n",&arr[0]);
printf("address:%p\n",&arr[0][0]);

printf("\n");
printf("address:%p\n",&arr[1][1]);
printf("address:%p\n",&arr[1][0]+1);
printf("address:%p\n",arr[1]+1);
printf("address:%p\n",arr[0]+4);

printf("\n");
printf("address:%p\n",&arr[0]+1);

printf("\n");
printf("address:%p\n",arr+2);
printf("address:%p\n",&arr[0]+2);

printf("\n");
printf("long:%ld\n",sizeof(arr));
printf("long:%ld\n",sizeof(arr[0]));
printf("long:%ld\n",sizeof(arr[0][0]));

printf("\n");
printf("long:%ld\n",sizeof(arr1));
printf("long:%ld\n",sizeof(arr1[0]));
printf("long:%ld\n",sizeof(arr1[0][0]));

printf("\n");
printf("long:%ld\n",sizeof(arr2));
printf("long:%ld\n",sizeof(arr2[0]));
printf("long:%ld\n",sizeof(arr2[0][0]));

return 0;
}



