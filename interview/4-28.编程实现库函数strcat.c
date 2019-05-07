#include<stdio.h>

char *mystrcat(char *obj, const char *src){
    char *op = obj;
    while(*op++);
    op--;
    while(*op++ = *src++);
    return obj;
}

int main(){
    char str1[] = "abc";
    const char *str2 = "hello";
    printf("拼接前: %s\n", str1);
    printf("拼接后: %s\n", mystrcat(str1, str2));
    return 0;
}
