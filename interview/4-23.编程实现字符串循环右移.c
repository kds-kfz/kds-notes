#include<stdio.h>
#include<stdlib.h>

void loopMove(char *src, int n){
    char *sp = src;
    while(*sp++);
    int len = sp - src - 1;
    n %= len;
    char *tmp = (char *)malloc(len + 1);
    int i = 0;
    for(; i < n; i++){      //保存后n个字符
        tmp[i] = src[len - n +i];
    }
    for(i = len - 1; i >= n; i--){//将原字符串前n位字符后移
        src[i] = src[i -n];
    }
    for(i = 0; i < n; i++){//替换原字符串的前n个字符
        src[i] = tmp[i];
    }
    free(tmp);
}

int main(){
    char str[] = "123456";
    printf("移动%d前: %s\n", 3,str);
    loopMove(str, 3);
    printf("移动%d后: %s\n", 3,str);
    return 0;
}
