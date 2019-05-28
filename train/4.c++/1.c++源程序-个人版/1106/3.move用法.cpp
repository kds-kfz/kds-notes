#include<iostream>
//2.move用法.cpp
using namespace std;
int sum(int &&a,int &&b){
    return a+b;
}


int main(){
    int i=10;//有一个内存，取个名字脚i，存类10
    int j=20;
//    cout<<sum(i,j)<<endl;//错误,传参是右值引用，i,j是左值
    //std::move()函数，把左值转换成右值
    cout<<sum(std::move(i),std::move(j))<<endl;

    int &r1=i;//r1是i的别名
    int &&r2=std::move(i);
    r2=90;
    cout<<&r2<<" "<<&i<<endl;
    return 0;
}
