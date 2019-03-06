#include<stdio.h>
#include<stdarg.h>

static int Simple(int num, ...);
static void myprintf(char *fmt, ...);

int main(){
    // 函数1：int strcmp(const char *s1, const char *s2);
    // 说明：两个字符串自左向右逐个字符相比（按ASCII值大小相比较），直到出现不同的字符或遇’\0’为止
    // 返回值：根据编译器决定，有两种结果
    // 结果1：相等返回0，不等返回差值，正数（a>b），负数（a<b）
    // 结果2：相等返回0，不等则，1（a>b），-1（a<b）
    //
    // 函数2：char *strcpy(char *dest, const char *src);
    // 将src复制到dest，包括\0，成功返回dest，失败返回null
    // 若src长度大于dest空间大小，则没有\0会越界，变成了字符数组，而不是字符串
    // 暂且认为没有以\0结尾的字符串是字符数组 TODO
    //
    // 函数3：char *strcat(char *dest, const char *src);
    // 将src连接到dest，包括\0，删除dest的\0，从次数开始连接，要求dest足够大
    //
    // 函数4：int snprintf(char *str, size_t size, const char *format, ...);
    // 最多从源串中拷贝n－1个字符到目标串中，然后再在后面加一个\0
    // 若str够大，最多拷贝size - 1个，返回值则按源串中实际拷贝的长度
    //
    // 函数5：int strncmp(const char *s1, const char *s2, size_t n);
    // 比较前n个字符是否相等，用法与strcmp相似
    
    char *src = "1234567890";
    char dest_1[20] = {0};
    char dest_2[5] = {0};

    int ret = snprintf(dest_1, 5, "%s", src);
    printf("dest = %s, ret = %d\n", dest_1, ret);//1234, 10

    ret = snprintf(dest_2, 8, "%s", src);
    printf("dest = %s, ret = %d\n", dest_2, ret);//1234567, 10
    
    //可变参数用法
    Simple(5, 2, 3, 4, 1, 8);

    myprintf("src = %s, src[0] = %c, sizeof(dest_1) = %d\n", src, src[0], sizeof(dest_1));
    return 0;
}

/*
 va_list是用于存放参数列表的数据结构。

 va_start函数根据初始化last来初始化参数列表。

 va_arg函数用于从参数列表中取出一个参数，参数类型由type指定。

 va_copy函数用于复制参数列表。

 va_end函数执行清理参数列表的工作。
 */

static int Simple(int num, ...){
    int i, result=0;
    // va_list指针，用于va_start取可变参数，为char*
    va_list v1;
    
    // 取得可变参数列表中的第一个值
    va_start(v1, num); 
    printf("%d, %d\n", num, *v1);
    
    // 这里num表示可变参数列表中有多少个参数
    for (i = 0; i < num ; ++i){
        // 这里把v1往后跳过4个字节（sizeof(int)大小）指向下一个参数
        // 返回的是当前参数，而非下一个参数
        result = va_arg(v1, int);
        printf("result: %d, *v1: %d\n", result, *v1);
    }
    
    va_end(v1);// 结束标志
    return result;
}

static void myprintf(char *fmt, ...){
    va_list v1;
    int d;
    double f;
    char c, flag, *s;
    va_start(v1, fmt);
    while(*fmt){
        flag = *fmt++;
        if(flag != '%'){
            putchar(flag);
            continue;
        }
        flag = *fmt++;
        switch(flag){
            case 's':
                s = va_arg(v1, char *);
                printf("%s", s);
                break;
            case 'd':
                d = va_arg(v1, int);
                printf("%d", d);
                break;
            case 'c':
                // 这里需要强制转换，因为va_arg只接受完全提升的类型
                c = va_arg(v1, int);
                printf("%c", c);
                break;
            default:
                putchar(flag);
                break;
        }
    }
    va_end(v1);
}

