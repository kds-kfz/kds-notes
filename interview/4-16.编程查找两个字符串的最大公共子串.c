#include<stdio.h>
#include<stdlib.h>
#include<string.h>

//4-16.编程查找两个字符串的最大公共子串
//换一种问法是：一个字符串有多少种相连的情况

char *common_maxstr(char *src, char *des){
    if(src == NULL || des == NULL){
        return NULL;
    }
    char *longstr = NULL, *shortstr = NULL;
    if(strlen(src)> strlen(des)){
        longstr = src;
        shortstr = des;
    }else{
        longstr = des;
        shortstr = src;
    }
    //初次判断短字符串在长字符串中
    if(strstr(longstr, shortstr) != NULL)
        return shortstr;
    char *temp = (char *)malloc(sizeof(char) * (strlen(shortstr)+1) );
    int i, j;
    for(i =  strlen(shortstr); i>0; i--){
        for(j = 0; j<strlen(shortstr)-i+1; j++){
            memcpy(temp, &shortstr[j], i);
            temp[i] = '\0';
            if(strstr(longstr, temp) != NULL)
                return temp;
        }
    }
    return NULL;
}
#if 0
i = 9, abcdABCDe
i = 8, abcdABCD
i = 8,  bcdABCDe
i = 7, abcdABC
i = 7,  bcdABCD
i = 7,   cdABCDe
i = 6, abcdAB
i = 6,  bcdABC
i = 6,   cdABCD
i = 6,    dABCDe
i = 5, abcdA
i = 5,  bcdAB
i = 5,   cdABC
i = 5,    dABCD
i = 5,     ABCDe
i = 4, abcd
i = 4,  bcdA
i = 4,   cdAB
i = 4,    dABC
i = 4,     ABCD
i = 4,      BCDe 找到了
#endif
int main(){
    char *str1 = "abcdABCDe";
    char *str2 = "eABCDeabc";
    common_maxstr(str1, str2) != NULL ? printf("%s\n", common_maxstr(str1, str2)) : printf("没找到\n");
    return 0;
}
