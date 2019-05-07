#include<stdio.h>

char *deleteChar(char *src, char c){
    if(src == NULL)
        return src;
    char *head = src;
    char *p = src;
    while(*p++){
        if(*p != c){
            *head++ = *p;
        }
    }
    *head = '\0';
    return src;
}

int main(){
    char str[] = "abcdGcpcoOc";
    printf("删除前: %s\n", str);
    printf("删除后: %s\n", deleteChar(str, 'c'));
    return 0;
}
