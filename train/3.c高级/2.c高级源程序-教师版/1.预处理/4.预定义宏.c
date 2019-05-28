#include <stdio.h>
#define JSON_NULL 0
#define JSON_String 1
int main()
{
    printf ("%d\n",__LINE__);//行数
    printf ("%s\n",__func__);//当前函数名
    printf ("%s\n",__DATE__);//日期
    printf ("%s\n",__TIME__);//时间
    printf ("%s\n",__FILE__);//文件名
    //NULL 0   EOF -1
    return 0;
}
