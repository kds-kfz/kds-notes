#include<iostream>
#include<string>
#include<stdio.h>

using namespace std;

typedef struct s{
    string name;
}TI;

int main(){
    //TI *p = new TI[5]{{"张三"}, {"李思"}, {"小二"}, {"王超"}, {"马汉"}};
    TI *p = new TI[5];
    for(int i = 0; i < 5; i++){
        p[i].name = "1";
    }
    for(int i = 0; i < 5; i++){
        printf("%s\n", p[i].name.c_str());
    }
    return 0;
}
