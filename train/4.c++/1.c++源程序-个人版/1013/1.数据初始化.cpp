#include<iostream>
//1.cpp
using namespace std;

//栈区(局部基本数据类型变量)，堆区默认初始化都是随机值
//静态变量，数据取，默认为0
//string,自定义类型，默认为空，有自己的初始化方式

int num;//全局变量，默认初始化0
static int s_num;//全局静态变量，默认初始化0
string s;//空

int main(){
#if 0
    //c++ 变量初始化
    int i=0;	//拷贝初始化
    int j(2);	//直接初始化

    string s1="hello";	//拷贝初始化
    //s1=string("hello");
    string s2("world");	//直接初始画
    string s3(s2);	//拷贝初始化

    //c++11标准 编译加-std=c++11或-std=c++1y
    int k={3};	//列表初始化
    int m{4};	//列表初始化

    double d=1.23;
    int a=d;	//能初始化，精度缺失
    int b={d};	//错误，因为d的值赋给b有丢失信息的风险
		//用{}复制要保证没有精度缺失(小转大)，否则报警告(大转小)
    
    cout<<"i="<<i<<",j="<<j<<endl;
    cout<<"s1="<<s1<<",s2="<<s2<<",s3="<<s3<<endl;
    cout<<"k="<<k<<",m="<<m<<endl;
    cout<<"d="<<d<<",a="<<a<<",b="<<b<<endl;

#endif

    int num1;//随机值//栈区
    static int s_num1;//局部静态变量，默认初始化0//数据区
    string s1;//空
    
    cout<<"num="<<num<<",s_num="<<s_num<<",s="<<s<<endl;
    
    cout<<"num1="<<num1<<",s_num1="<<s_num1<<",s1="<<s1<<endl;

    return 0;
}
