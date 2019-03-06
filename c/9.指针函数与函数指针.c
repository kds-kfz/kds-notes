#include<stdio.h>

// 指针函数通常是指函数返回值是指针的一类函数

int *MaxInt(int *x, int *y){
    return *x > *y ? x :y ;
}

// 函数指针是指指向某个具体函数的指针变量，
// 在程序设计时可以用来调用某个特定函数或者做某个函数的参数
// 函数指针就是函数的指针。它是一个指针，指向一个函数

const char *MaxStr(const char *p1, const char *p2){
    return !strcmp(p1, p2) ? p1 
        : strcmp(p1, p2) > 0 ? p1 
        : strcmp(p1, p2) < 0 ? p2 
        : p1;
}

int main(){
    int a = 123, b = 145;
    int *(*p1)(int *, int *);
    p1 = &MaxInt;
    const char *(*p2)(const char *, const char *) = MaxStr;
    
    //指针函数
    printf("%d\n", *MaxInt(&a, &b));
    
    //简单的函数指针
    printf("%d\n", *p1(&a, &b));
    printf("%s\n", p2("124", "123"));

    //函数指针数组
    int *(*p3[3])(int * ,int *);
    p3[0] = MaxInt;
    p3[1] = &MaxInt;
    printf("%d\n", *p3[0](&b, &a));

    //函数指针数组指针
    int *(*(*pf)[3])(int * ,int *);
    pf = &p3;
    printf("%d\n", *pf[0][1](&a, &b));
 
#if 0
    (*(void(*) ())0)();
    第一步：void(*) ()，可以明白这是一个函数指针类型。这个函数没有参数，没有返回值。
    第二步：(void(*) ())0，这是将0强制转换为函数指针类型，0是一个地址，也就是说一个函数存在首地址为0的一段区域内。
    第三步：(*(void(*) ())0)，这是取0地址开始的一段内存里面的内容，其内容就是保存在首地址为0的一段区域内的函数。
    第四步：(*(void(*) ())0)()，这是函数调用。
#endif
    //(*(char**(*) (char **,char **))0) ( char **,char **);
    return 0;
}

