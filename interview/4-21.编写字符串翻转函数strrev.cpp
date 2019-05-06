#include<iostream>
#include<cstring>

using namespace std;

//编写字符串反转函数：strrev。要求时间和空间效率都尽量高。

//方式一：交换首位字符
char *strrev_1(const char *src){
    int len = strlen(src);
    char *sp = new char[len + 1];
    strcpy(sp, src);
    for(int i = 0; i < len / 2; i++){
        char c = sp[i];
        sp[i] = sp[len - i -1];
        sp[len - 1 -i] = c;
    }
    return sp;
}

//方式二：指针方式交换首位字符
char *strrev_2(const char *src){
    char *sp = new char[strlen(src) + 1];
    strcpy(sp, src);
    char *ret = sp;
    char *end = sp + strlen(src) - 1;
    while(end > ret){
        char c = *ret;
        *ret = *end;
        *end = c;
        ret++;
        end--;
    }
    return sp;
}

//方式三：异或交换首尾字符
char *strrev_3(const char *src){
    char *sp = new char[strlen(src) + 1];
    strcpy(sp, src);
    char *ret = sp;
    char *end = sp + strlen(src) - 1;
    while(end > ret){
        *end ^= *ret;
        *ret ^= *end;
        *end ^= *ret;
        ret++;
        end--;
    }
    return sp;
}

//方式四：加减法交换首尾字符
char *strrev_4(const char *src){
    char *sp = new char[strlen(src) + 1];
    strcpy(sp, src);
    char *ret = sp;
    char *end = sp + strlen(src) - 1;
    while(end > ret){
        *end += *ret;
        *ret = *end - *ret;
        *end = *end - *ret;
        ret++;
        end--;
    }
    return sp;
}

//方式五：递归方式交换首位字符
char *strrev_5(char *src, int len){
    if(len <= 1)
        return src;
    char t = *src;
    *src = *(src + len - 1);
    *(src + len - 1) = t;
    return (strrev_5(src + 1, len - 2) - 1);
}

int main(){
    //定义一个常指针
    const char *p = "abcd", *q = NULL;
    cout << "翻转前: " << p << endl;
    cout << "翻转后: " << (q = strrev_4(p)) << endl;
    //定义一个字符串
    char p1[] = "abcd";
    cout << "翻转前: " << p1 << endl;
    //p2不用释放
    char *p2 = strrev_5(p1, strlen(p1));
    cout << "翻转后: " << p2 << endl;
    delete []q;
    return 0;
}
