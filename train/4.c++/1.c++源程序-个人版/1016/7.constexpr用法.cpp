#include<iostream>
//7.cpp
using namespace std;

//c++11 constexpr函数内部不能有分支，if,switch不可以
//可以递归，要求传入的参数是const的
constexpr int pro(int n){
#if 0
    //错误
    if(n<0)
	return -1;
    else if(n<=1)
	return 1;
    else
	return n*pro(n-1);
#endif
    //三元运算符
    return n<=1 ? 1 : n*pro(n-1);
}

int main(){
#if 0
    int a=10;
    //常量表达式，在编译的时候，知道结果
    constexpr int b=a;//错误，a不是一个const常量
#endif
    const int a=10;
    //常量表达式，在编译的时候，知道结果
    //节省执行的时间，还可以检验某个变量或表达式是不是始终是常量
    constexpr int b=1+2;//

    int n=5;
    constexpr int c=pro(5);
    cout<<c<<endl;
    return 0;
}

