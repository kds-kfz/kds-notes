#include<iostream>
//4.右值引用2.cpp
using namespace std;

template <typename T>
int fun(T &a){//只能引用左值
    cout<<"& "<<a<<endl;
}

template <typename T>
int fun(T &&a){//模板里，&&既可以引用左值又可以引用右值
    cout<<"&& "<<a<<endl;
}

template <typename T>
int test(T &&a){//在编译器中会把有名字的右值识别成左值
    fun(a);//一定会把a当作左值处理
    fun(std::move(a));//一定是右值
    fun(std::forward<T>(a));//把a本身的类型进行完美转发，识别a本身
}

int main (){
    //调用了右值引用
    fun(1);
    int a=90;
    //调用类左值引用
    fun(a);
    cout<<"--------------------------"<<endl;
    test(a);
    cout<<"--------------------------"<<endl;
    test(7);
    return 0;
}
