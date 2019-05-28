#include<iostream>
//5.nocexcept用法.cpp
using namespace std;
//noexcept阻止抛出异常，阻止我们异常处理，终端来处理这个异常
int Dir(int a,int b){//没有说明符，异常自己处理
//int Dir(int a,int b)noexcept(false){//说明符，异常自己处理
//int Dir(int a,int b)noexcept(true){//说明符，异常终端处理
    if(b==0)
	throw 0;
    return a/b;
}

//运算符，判断函数是否有noexcept描述符号,有则true,假则false
//noexcept(Dir(2,0))
//函数Dir没有noexcept描述符则noexcept(Dir(2,0))值是0
//函数Dir有noexcept(false)描述符则noexcept(Dir(2,0))值是0
//函数Dir有noexcept(true)描述符则noexcept(Dir(2,0))值是1

int main(){
    try{
//	Dir(2,1);
	cout<<"是否有描述符 : "<<noexcept(Dir(2,0))<<endl;
	Dir(2,0);
    }
    catch(int n){//接收抛出的值，即0
	cout<<"自己处理,接收throw抛出的值 "<<n<<endl;
	return 0;
    }
    cout<<"main end"<<endl;
    return 0;
}
