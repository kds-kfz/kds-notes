#include<iostream>
//6.cpp
using namespace std;

int fun(){
    return 1;
}

int main(){

    int a=90;
    //c++11,auto通过值推断数据类型
    auto b=10;//int
    auto c=1.1;//double
    auto d=1.2f;//folat

    auto e="hello";//默认是const char *类型
    auto f=string("hello");
    cout<<"f_size="<<f.size()<<endl;

    auto m=fun();

    int &r=a;
    auto x=r;//不能直接推出引用类型
    x=99;
    cout<<"a="<<a<<endl;//a=90

    auto p=&a;
    *p=56;
    cout<<"a="<<a<<endl;//a=56
    
    int *const q1=&a;//不能推出顶层const
    int const *q2=&a;//能推出底层const
//    auto s1=q1;
//    s1=33;
    cout<<"a="<<a<<endl;//a=33
//    *q2=22;//报错，底层const内容不可变
//    cout<<"a="<<a<<endl;//a=22

    //如果想定义顶层const，需添加const关键字
    auto const p1=q1;//
    *p1=11;
    cout<<"a="<<a<<endl;//a=11

    return 0;
}
