#include"stdio.h"
//main.c，分文件1阶
//1.算1个字符串长度
int my_strlen(const char *arr);

//2.返回一个较长的字符串地址
const char * compare_str(const char *p1,const char *p2);

//3.复制字符串p2-->p1
char *copy_str(char *p1,const char *p2);

//4.字符串拼接p2-->p1
char *cat_str(char *p1,char *p2);

int main(){
    /*
    char str[10]="0";
    char str1[10]="0";
    char str2[10]="0";

    const char *p=str;
    
    scanf("%s",str1);
    scanf("%s",str2);

    p=compare_str(str1,str2);

//    printf("%s\n",compare_str(str1,str2));
    printf("%s\n",p);
*/
    char a[100];
    char b[100];
    char *c;

    scanf("%s",a);
    scanf("%s",b);
    printf("a=%s\n",a);
    printf("b=%s\n",b);

//    copy_str(a,b);
    c=cat_str(a,b);

    printf("a=%s\n",c);

    return 0;
}
