#include"stdio.h"
//3.c
//stdio.h文件里的宏定义，可用于调试程序
int main(){
    printf("%s\n",__func__);//当前执行的函数名称
    printf("%s\n",__FILE__);//当前文件名
    printf("%s\n",__DATE__);//编译日期
    printf("%s\n",__TIME__);//编译时间
    printf("%d\n",__LINE__);//当前行号
    return 0;
}
