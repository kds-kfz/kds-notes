#include<stdio.h>

int main(){
    int arr_1[5] = {1, 2, 3, 4, 5}, i = 0;
    int arr_2[][3] = {6, 5, 4, 3, 2, 1}, j = 0;

    //定义一个指向int型的指针变量。指向arr_1的首元素地址
    int *p_1 = arr_1;
    for(i = 0 ; i < 5; i++)
        //printf("%d ", *(p_1 + i));
        printf("%d ", *(arr_1 + i));
    putchar(10);

    //一维数组指针
    //定义一个整型的一维数组指针，本质是个指针。指向arr_1[4]的首元素地址
    int (*p_2)[] = &arr_1;
    for(i = 0 ; i < 5; i++)
        printf("%d ", *(*p_2 + i));
    putchar(10);

    //二维数组指针，指向一维数组，指向arr_1数组里连续的20个字节
    int (*q)[5] = &arr_1;
    printf("sizeof q = %ld\n", sizeof(*q));//20

    //二维数组指针，指向二维数组
    int (*p_3)[3] = arr_2;
    for(i = 0; i < 2; i++)
        for(j = 0; j < 3; j++)
            //printf("%d ", *(p_3[i] + j));
            printf("%d ", *(*(p_3 + i) + j));
    putchar(10);

    /*==========================================*/

    //指针数组，存的是数组元素的地址
    //一维指针数组，数组里存的是指针
    int *p_4[5];
    //每个指针占8字节，5 * 8 = 40
    printf("szieof p_4 = %ld\n", sizeof(p_4));
    for(i = 0 ; i < 5; i++) p_4[i] = &arr_1[i];
    for(i = 0 ; i < 5; i++) printf("%d ", *p_4[i]);
    putchar(10);
    for(i = 0 ; i < 5; i++) printf("%d ", *(*p_4 + i));
    putchar(10);

    //二维指针数组
    //以数组形式可以直接表示元素的地址，不是数组形式，则是指针变量地址
    int *p_5[2] = {arr_2[0], arr_2[1]};
    printf("szieof p_5[0] = %ld\n", sizeof(p_5[0]));// 8 
    for(i = 0; i < 2; i++)
        for(j = 0; j< 3 ;j++)
            //printf("%d ", *(p_5[i] + j));
            //printf("%d ", *(arr_2[i] + j));
            printf("%d ", *(*(p_5 + i) + j));
    putchar(10);

    /*==========================================*/

    //指针数组指向二维数组
    int *p_6[][3] = {*arr_2, *arr_2 + 1, *arr_2 + 2, *arr_2 + 3, *arr_2 + 4, *arr_2 + 5};
    //二级指针指向指针数组
    int **p_7 = *p_6;
    for(i = 0; i < 6; i++, p_7++)
        printf("%d ", **p_7);
    putchar(10);
    
    return 0;
}
