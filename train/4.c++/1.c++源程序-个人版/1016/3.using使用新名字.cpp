#include<iostream>
//3.cpp
using namespace std;

//使用1个新名字
using F64=double;
using F32=float *;

int main(){
    F64 p1;
    F32 p2,p3;//float *p2,*p3;
    cout<<"p1_size="<<sizeof(p1)<<endl;
    cout<<"p2_size="<<sizeof(p2)<<endl;
    
    return 0;
}
