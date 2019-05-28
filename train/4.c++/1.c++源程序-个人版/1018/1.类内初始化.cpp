#include<iostream>
//1.cpp
using namespace std;

//c++类内初始化
struct Stu{
    int id;//=0;
    string name;//="lisi";
    int score;
};

void show(Stu d){
    cout<<d.id<<" ";
    cout<<d.name<<" ";
    cout<<d.score;
    cout<<endl;
}

int main(){

    //s1是一个变量，c++里叫做对象，也可以叫做实例
    Stu s1;
    show(s1);
    //类内初始化和变量初始化只能使用一种
    Stu s2={1001,"liwu",90};
    show(s2);
    return 0;
}
