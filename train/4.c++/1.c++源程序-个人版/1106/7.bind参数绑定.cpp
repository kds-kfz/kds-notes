#include<iostream>
#include<functional>//c++关于函数的头文件
//7.bind参数绑定.cpp
using namespace std;
using namespace std::placeholders;//这个命名空间下定义类站位符

int sum(int a,int b,int c){
    cout<<"a = "<<a<<endl;
    cout<<"b = "<<b<<endl;
    cout<<"c = "<<c<<endl;
    return a+b+c;
}
//auto p=bind(callback,args);

int main(){
    auto p1=bind(sum,-1,_1,_2);
    auto p2=bind(sum,_1,-2,_2);
    auto p3=bind(sum,_1,_2,-3);
    auto p4=bind(sum,0,0,0);

    cout<<p1(13,14)<<endl;
    cout<<"---------------------------"<<endl;
    cout<<p2(23,24)<<endl;
    cout<<"---------------------------"<<endl;
    cout<<p3(33,34)<<endl;
    cout<<"---------------------------"<<endl;
    cout<<p4()<<endl;
    return 0;
}
