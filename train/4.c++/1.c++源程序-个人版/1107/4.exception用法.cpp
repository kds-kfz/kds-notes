#include<iostream>
//4.exception用法.cpp
using namespace std;
//exception标准库异常基类，内部有一what()个函数，
//进行错误的描写,bad_ballc继承这个基类，
//在处理标准库的异常时，可以只用基类异常类型捕获
class Exception1:public exception{
    public:
    const char *what(){
	return "Exception1";
    }
};
int main(){
    try{
//	throw Exception1();
	new int[100000000000];
    }
    catch(Exception1 &e1){//要加引用，否则不能实现多态
	cout<<e1.what()<<endl;
    }
    catch(exception &e){//自定义不能实现多态
	cout<<e.what()<<endl;
    }
    return 0;
}
