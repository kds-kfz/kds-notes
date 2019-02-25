#include<stdio.h>

int arr_1[2][3];
char arr_2[2][3];
double arr_3[2][3];

int main(){
    //地址范围 0x600b60 - 0x600b78
    printf("arr_1 address:%p - %p\n", arr_1, arr_1[0] + 6);

    //arr_1[0][0] 0x600b60
    printf("arr_1 address: %p\n", arr_1);
    printf("arr_1 address: %p\n", &arr_1);
    printf("arr_1 address: %p\n", arr_1[0]);
    printf("arr_1 address: %p\n", &arr_1[0]);
    putchar(10);

    //arr_1[1][1] 0x600b70
    printf("arr_1 address:%p\n", &arr_1[1][0] + 1);
    printf("arr_1 address:%p\n", arr_1[1] + 1);
    printf("arr_1 address:%p\n", arr_1[0] + 4);
    putchar('\n');

    //arr_1[1][0] 0x600b6c 加上1列
    printf("arr_1 address:%p\n", arr_1 + 1);
    printf("arr_1 address:%p\n", &arr_1[0] + 1);
    putchar(10);

    printf("arr_1 int :%ld\n", sizeof(arr_1));       //4 * 6 = 24
    printf("arr_1 int :%ld\n", sizeof(arr_1[0]));    //4 * 3 = 12
    printf("arr_1 int :%ld\n", sizeof(arr_1[0][0])); //4
    putchar(10);

    printf("arr_2 long :%ld\n", sizeof(arr_2));       //1 * 6 = 6
    printf("arr_2 long :%ld\n", sizeof(arr_2[0]));    //1 * 3 = 3
    printf("arr_2 long :%ld\n", sizeof(arr_2[0][0])); //1
    putchar(10);

    printf("arr_3 double :%ld\n", sizeof(arr_3));      //8 * 6 = 48
    printf("arr_3 double :%ld\n", sizeof(arr_3[0]));   //8 * 3 = 24
    printf("arr_3 double :%ld\n", sizeof(arr_3[0][0]));//8
    return 0;
}
