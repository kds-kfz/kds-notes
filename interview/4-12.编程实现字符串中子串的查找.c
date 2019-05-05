#include<stdio.h>

//4-12.编程实现字符串中子串的查找

char *my_strstr(char *src, char *sub){
    if(src == NULL || sub ==NULL){
        return NULL;
    }
    while(*src){
        char *bp = src, *sp = sub;
        do{
            if(!*sp)
                return src;
        }while(*bp++ == *sp++);
        src += 1;
    }
    return NULL;
}

int main(){
    char *str1 = "abcdGHefg";
    char *str2 = "GHe";
    my_strstr(str1, str2) != NULL ? printf("%s\n", my_strstr(str1, str2)) : printf("没找到\n");
    return 0;
}
