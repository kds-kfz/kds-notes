#include<iostream>
//1.cpp
using namespace std;
//函数默认参数，从后往前写b=5,从后往前对齐
//如果不传参，形参的值是默认值，

//前置返回类型
int sum(int a,int b=5){
    return a+b;
}
//后置返回类型
auto sub(int a,int b)->int{
    return a-b;
}

//练习：写一个函数，求最大值
//既可以求两个屋敷号数的最大值
//也可以求3个数最大值
using U32 =unsigned int;

int Max(int a,int b,int c=0x80000000){
    int k=a>b?a:b;
    k=k>c?k:c;
    return k;
}
int Max1(U32 a,int b,U32 c=0){
    int k=a>b?a:b;
    k=k>c?k:c;
    return k;
}
int main(){
//    cout<<sum(2)<<endl;
//    cout<<sub(2,3)<<endl;
    cout<<Max1(012,2,3)<<endl;
    cout<<Max1(-2,-9,0)<<endl;
    return 0;
}

