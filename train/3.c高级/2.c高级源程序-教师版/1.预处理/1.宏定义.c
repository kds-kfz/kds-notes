#include <stdio.h>
#define PI 3.14
#define N 100+200
#define  S "abcdef"
//在预处理阶段完成操作　只做替换
int main()
{
    float a = PI;
    printf ("%.2f\n",a);
#undef PI
    int arr[N] = {0};
    int n = 0;
    printf ("输入数组长度\n");
    scanf("%d",&n);
    int arr1[n];
    int c = N;
    printf ("%s\n",S);


    return 0;
}
