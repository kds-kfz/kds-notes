#include<stdio.h>

int main(){
    int i;
    char ch[10];
    char ch2[10];
    char ch3[10];
#if 0
    for(i=0;i<10;i++){
        ch[i] = getchar();
        //接收回车
        getchar();
    }
    for(i=0;i<10;i++)
        printf("%c", ch[i]);
    
    //遇到回车结束输入，丢弃回车，可接收超过定义的数组长度
    scanf("%s",ch2);
    //若字符串中间有空格，则只能打印空格之前的内容
    printf("%s", ch2);

    //遇到回车结束输入，丢弃回车，可接收超过定义的数组长度
    gets(ch3);
    //输出自带回车，可打印空格
    puts(ch3);
#endif
    
    return 0;
}
