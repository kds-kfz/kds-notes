#include<iostream>
//3.const_cast.cpp
using namespace std;

//const_cast 只处理const属性的指针或引用，不会转换数据类型

int main(){
#if 1
    int a=23;
    int b=1;
    int *p1=&a;
    int *const p3=&a;
#if 0
    const int *q1=const_cast<const int *>(p1);
    int *const q1=const_cast<int *const>(p1);
    q1=&b;
    *q1=88;
#endif
    int *q2=const_cast<int *>(p2);
    *q2=44;
    cout<<"*q2 = "<<*q2<<endl;
    q2=&b;
    *q2=77;
    cout<<b<<endl;
#endif
    return 0;
}
