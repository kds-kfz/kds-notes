#include<stdio.h>
#include<string.h>

char *deleteChar(char *src, int pos, int n){
    char *p = src + pos - 1;
    while(*(p + n)){
        *p = *(p + n);
        p++;
    }
    *p = '\0';
    return src;
}

int main(){
    char str[] = "abcdefg";
    printf("删除前: %s\n", str);
    printf("删除后: %s\n", deleteChar(str, 2, 2));
    return 0;
}
