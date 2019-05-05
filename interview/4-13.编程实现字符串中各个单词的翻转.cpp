#include<iostream>

using namespace std;

//编写函数，将"I am from Shanghai"倒置为"Shanghai from am I"
//即句子中的单词位置倒置，而不改变内部的结构

void Revstr(char *src){
    char *sp = src, *start = src, *end = NULL;
    while(*sp++){
        if(*sp == ' ' || *sp == '\0'){
            end = sp - 1;
            //将一个单词翻转
            while(start < end)
                swap(*start++, *end--);
            //处理完一个单词将 start指向下一个单词首地址
            start = sp + 1;
        }
    }
    start = src;
    //sp - 2 : 此时sp已经加到字符串外，要把\0去掉，所有要 -2
    end = sp - 2;
    //将整个字符串翻转
    while(start < end)
        swap(*start++, *end--);
}

int main(){
    char str[] = "Shanghai from am I";
    cout<<"翻转前: "<<str<<endl;
    Revstr(str);
    cout<<"翻转后: "<<str<<endl;
    return 0;
}
