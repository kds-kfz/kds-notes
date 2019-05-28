#include<iostream>
#include<functional>
//8.函数包装器.cpp
using namespace std;
using namespace std::placeholders;

int sum(int a,int b){
    return a+b;
}

class Value{
    public:
    int operator()(){
	cout<<"no"<<endl;
	return 1;
    }
    int operator()(int n){
	return -n;
    }
};

int main(){
    int n=10;
//    void (*p)=[n](){cout<<n<<endl;}//错误，不可以把int定义成void *p类型

    //可调用对象：函数，函数指针，lambda表达式，
    //bind的返回值，重载了()运算符的对象
    //函数包装器，可以指向任意类型的可调用对象
    function<int(int,int)> ps=sum;
    function<void()> p=[n](){
	cout<<n<<endl;
    };
    p();

    function<int(int)> pb=bind(sum,_1,99);
    cout<<pb(5)<<endl;
    cout<<"--------------------"<<endl;
    
    Value v;

    //v是个可调用对象
    function<int(int)> pv1=v;//有参

    //定义一个没名字的对象
    function<int(int)> pv2=Value();//有参
    cout<<pv1(7)<<endl;
    cout<<pv2(7)<<endl;
    cout<<"--------------------"<<endl;

    function<int()> pv3=v;//无参
    cout<<pv3()<<endl;

    return 0;
}
